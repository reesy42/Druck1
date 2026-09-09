#ifndef PID_H
#define PID_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp;
    float ki;
    float kd;
    float setpoint;      // Sollwert in mmH2O
    float integral;
    float prev_error;
    float dt;            // Abtastzeit in Sekunden
    float out_min;       // Ausgang-Minimum (0.0)
    float out_max;       // Ausgang-Maximum (100.0)
} PID_Handle_t;

void PID_Init(PID_Handle_t *h, float kp, float ki, float kd, float setpoint, float dt);
float PID_Compute(PID_Handle_t *h, float measured);
void PID_SetParams(PID_Handle_t *h, float kp, float ki, float kd, float setpoint);
void PID_Reset(PID_Handle_t *h);

#ifdef __cplusplus
}
#endif

#endif /* PID_H */
