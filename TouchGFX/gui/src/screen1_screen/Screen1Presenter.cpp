#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{
}

void Screen1Presenter::activate()
{
}

void Screen1Presenter::deactivate()
{
}

void Screen1Presenter::setPidParams(float kp, float ki, float kd, float setpoint)
{
    model->setPidParams(kp, ki, kd, setpoint);
}

void Screen1Presenter::getPidParams(float& kp, float& ki, float& kd, float& setpoint)
{
    kp       = model->getPidKp();
    ki       = model->getPidKi();
    kd       = model->getPidKd();
    setpoint = model->getPidSetpoint();
}

void Screen1Presenter::pressureUpdated(float pressure_mmh2o, uint32_t time_ms)
{
    view.onPressureUpdated(pressure_mmh2o, time_ms);
}
