#ifndef DSHOT_H
#define DSHOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* DShot600-Ausgabe fuer den Lueftertreiber (SQESC 28120).
 * Pin PA15 = Arduino-Header D9, TIM2_CH1 (AF1), DMA1 Stream5 Kanal 3. */

void DShot_Init(void);

/* Reglerausgang 0..100 %  ->  DShot-Throttle 48..2047 (sofort senden) */
void DShot_SendPercent(uint8_t percent);

/* Rohwert: 0 = Stop/disarmed, 1..47 = Sonderkommandos, 48..2047 = Schub (sofort senden) */
void DShot_SendRaw(uint16_t value);

/* Blockierend: sendet ms lang Nullframes, damit der ESC armt.
 * Nur vor dem RTOS-Start aufrufen (nutzt HAL_Delay). */
void DShot_Arm(uint32_t ms);

/* Startet TIM5 als 4-kHz-Wiederholungstimer.
 * Ab diesem Aufruf sendet DShot automatisch den zuletzt gesetzten Wert.
 * Soll nach DShot_Arm() und vor dem RTOS-Start aufgerufen werden. */
void DShot_StartRepeat(uint32_t rate_hz);

/* Setzt den Sollwert fuer den naechsten Wiederholungs-Frame (thread-safe).
 * Wird vom PID-Task aufgerufen; der TIM5-ISR liest diesen Wert. */
void DShot_SetPercent(uint8_t percent);
void DShot_SetRaw(uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* DSHOT_H */
