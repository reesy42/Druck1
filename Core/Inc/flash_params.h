#ifndef FLASH_PARAMS_H
#define FLASH_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* PID-Parameter im letzten Flash-Sektor (Sector 7, 0x080C0000, 256KB) speichern. */

void FlashParams_Load(float *kp, float *ki, float *kd, float *setpoint);
int  FlashParams_Save(float  kp, float  ki, float  kd, float  setpoint);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_PARAMS_H */
