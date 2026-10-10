#ifndef SCOPE_OUT_H
#define SCOPE_OUT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Analog-Monitorausgaenge fuers Oszilloskop.
 *
 * Der F746-DAC (PA4/PA5) liegt nicht auf dem Arduino-Header, deshalb:
 * 100-kHz-Hardware-PWM, Tastgrad proportional zum Messwert. Extern mit
 * RC-Tiefpass glaetten (z. B. 10 kOhm + 100 nF, tau = 1 ms, Restwelligkeit
 * ca. 8 mVss bei 100 kHz) -> Gleichspannung 0..3,3 V.
 *
 *   Arduino D3 = PB4  (TIM3_CH1,  AF2) : Druck    0..SCOPE_P_FS_MMH2O -> 0..3,3 V
 *   Arduino D6 = PH6  (TIM12_CH1, AF9) : Stellwert 0..100 %          -> 0..3,3 V
 *
 * Mit SCOPE_P_FS_MMH2O = 330 gilt: 10 mV = 1 mmH2O, 33 mV = 1 % Stellwert. */

#define SCOPE_P_FS_MMH2O   330.0f

void ScopeOut_Init(void);
void ScopeOut_SetPressure(float mmh2o);
void ScopeOut_SetOutput(float percent);

#ifdef __cplusplus
}
#endif

#endif /* SCOPE_OUT_H */
