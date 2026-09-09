#include "pid.h"
#include <float.h>   /* FLT_MAX */

void PID_Init(PID_Handle_t *h, float kp, float ki, float kd, float setpoint, float dt)
{
    h->kp       = kp;
    h->ki       = ki;
    h->kd       = kd;
    h->setpoint = setpoint;
    h->dt       = dt;
    h->out_min  = 0.0f;
    h->out_max  = 100.0f;
    h->integral    = 0.0f;
    h->prev_error  = FLT_MAX;  /* Sentinel: D-Term im ersten Aufruf = 0 */
}

float PID_Compute(PID_Handle_t *h, float measured)
{
    if (measured < 0.0f) measured = 0.0f;
    float error = h->setpoint - measured;

    /* Erster Aufruf: keine Ableitung berechnen (Spike-Schutz) */
    float derivative = 0.0f;
    if (h->prev_error != FLT_MAX)
    {
        derivative = (error - h->prev_error) / h->dt;
    }

    h->integral += error * h->dt;

    float output = h->kp * error + h->ki * h->integral + h->kd * derivative;

    /* Clamp output and apply anti-windup */
    if (output > h->out_max) {
        output = h->out_max;
        if (error > 0.0f) h->integral -= error * h->dt;
    } else if (output < h->out_min) {
        output = h->out_min;
        if (error < 0.0f) h->integral -= error * h->dt;
    }

    h->prev_error = error;
    return output;
}

void PID_SetParams(PID_Handle_t *h, float kp, float ki, float kd, float setpoint)
{
    h->kp       = kp;
    h->ki       = ki;
    h->kd       = kd;
    h->setpoint = setpoint;
}

void PID_Reset(PID_Handle_t *h)
{
    h->integral   = 0.0f;
    h->prev_error = FLT_MAX;  /* Wie nach PID_Init: erster Compute-Aufruf ohne D-Term */
}
