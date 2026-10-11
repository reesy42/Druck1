#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <touchgfx/hal/HAL.hpp>
#include "main.h"
#include "pid.h"
#ifndef SIMULATOR
#include "esc_telem.h"
#endif

#ifdef SIMULATOR
#include <ctime>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#else
static volatile long lastUs;
extern uint32_t SystemCoreClock;
static int freqMHz;
#endif

Model::Model() : modelListener(0),
                 pressureMmh2o_(0.0f), tempDegC_(0.0f), pidOutput_(0.0f),
                 pidKp_((float)g_pid_kp), pidKi_((float)g_pid_ki),
                 pidKd_((float)g_pid_kd), pidSetpoint_((float)g_pid_setpoint),
                 pressureMax_(0.0f), pressureMin_(0.0f), pressureMinMaxValid_(false),
                 fanRpm_(-1), escTempC_(-128)
{
#ifndef SIMULATOR
    lastUs  = touchgfx::HAL::getInstance()->getCPUCycles();
    freqMHz = SystemCoreClock / 1000000;
#endif
}

void Model::tick()
{
    Time previousTime = currentTime;

#ifdef SIMULATOR
#ifdef _MSC_VER
    time_t rawtime;
    struct tm timenow;
    time(&rawtime);
    localtime_s(&timenow, &rawtime);
    currentTime.hours        = timenow.tm_hour;
    currentTime.minutes      = timenow.tm_min;
    currentTime.seconds      = timenow.tm_sec;
    currentTime.milliseconds = 0;
#else
    timeval timenow;
    gettimeofday(&timenow, NULL);
    currentTime.hours        = (timenow.tv_sec / 60 / 60) % 24;
    currentTime.minutes      = (timenow.tv_sec / 60) % 60;
    currentTime.seconds      = timenow.tv_sec % 60;
    currentTime.milliseconds = timenow.tv_usec / 1000;
#endif
#else
    static int milliseconds = 0;
    long now = touchgfx::HAL::getInstance()->getCPUCycles();
    milliseconds += (now - lastUs + freqMHz / 2) / freqMHz / 1000;
    lastUs = now;
    currentTime.hours        = (milliseconds / 1000 / 60 / 60) % 24;
    currentTime.minutes      = (milliseconds / 1000 / 60) % 60;
    currentTime.seconds      = (milliseconds / 1000) % 60;
    currentTime.milliseconds = milliseconds % 1000;

    pressureMmh2o_ = g_pressure_mmh2o;
    tempDegC_      = g_temp_degc;
    pidOutput_     = g_pid_output;
    fanRpm_        = EscTelem_GetRPM();
    escTempC_      = EscTelem_GetTempC();
#endif

    /* pmax/pmin nachfuehren */
    if (!pressureMinMaxValid_)
    {
        pressureMax_ = pressureMmh2o_;
        pressureMin_ = pressureMmh2o_;
        pressureMinMaxValid_ = true;
    }
    else
    {
        if (pressureMmh2o_ > pressureMax_) pressureMax_ = pressureMmh2o_;
        if (pressureMmh2o_ < pressureMin_) pressureMin_ = pressureMmh2o_;
    }

    if (currentTime.seconds != previousTime.seconds)
    {
        if (modelListener)
            modelListener->timeUpdated(currentTime);
    }

    /* Druckwert bei jedem tick (~16ms) an View weiterleiten */
    if (modelListener)
    {
        uint32_t ms = (uint32_t)(currentTime.hours   * 3600000u
                               + currentTime.minutes * 60000u
                               + currentTime.seconds * 1000u
                               + currentTime.milliseconds);
        modelListener->pwmUpdated(pidOutput_);
        modelListener->pressureUpdated(pressureMmh2o_, ms);
    }
}

void Model::setPidParams(float kp, float ki, float kd, float setpoint)
{
    pidKp_       = kp;
    pidKi_       = ki;
    pidKd_       = kd;
    pidSetpoint_ = setpoint;

#ifndef SIMULATOR
    g_pid_kp       = kp;
    g_pid_ki       = ki;
    g_pid_kd       = kd;
    g_pid_setpoint = setpoint;
#else
    (void)kp; (void)ki; (void)kd; (void)setpoint;
#endif
}
