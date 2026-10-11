#ifndef ESC_TELEM_H
#define ESC_TELEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* KISS-/AM32-Telemetrie des ESC (SQESC 28120) ueber den Telemetrie-Pad.
 * Pin PC7 = Arduino-Header D0, USART6_RX (AF8), 115200 8N1, nur Empfang,
 * interner Pull-up. Der ESC sendet ein 10-Byte-Paket, sobald ein DShot-Frame
 * mit gesetztem Telemetrie-Bit ankommt (siehe dshot.c, DSHOT_TELEM_DIVIDER). */

/* Anzahl Magnetpole des Motors: RPM = eRPM / (Pole / 2) */
#define ESC_TELEM_MOTOR_POLES   2U

/* Werte gelten als veraltet, wenn so lange kein gueltiges Paket kam */
#define ESC_TELEM_TIMEOUT_MS    500U

typedef struct
{
  int8_t   temp_c;          /* ESC-Temperatur in Grad C                */
  uint16_t voltage_cV;      /* Versorgungsspannung in 0,01 V           */
  uint16_t current_cA;      /* Strom in 0,01 A                         */
  uint16_t consumption_mAh; /* verbrauchte Ladung in mAh               */
  uint32_t erpm;            /* elektrische Drehzahl (Aufloesung 100)   */
  uint32_t tick;            /* HAL_GetTick() beim Empfang              */
} EscTelem_Data;

typedef struct
{
  uint32_t ok;              /* gueltige Pakete                         */
  uint32_t crcErr;          /* 10 Bytes, aber CRC falsch               */
  uint32_t lenErr;          /* Burst mit falscher Laenge               */
  uint32_t lineErr;         /* Framing-/Overrun-Fehler                 */
  uint32_t noise;           /* Noise-Flag (Daten evtl. trotzdem gut)   */
} EscTelem_Stats;

/* PC7 + USART6 konfigurieren und Empfang starten. Nach MX_GPIO_Init(). */
void EscTelem_Init(void);

/* Letztes gueltiges Paket kopieren. Rueckgabe 1 = aktuell, 0 = veraltet/keins. */
int EscTelem_GetData(EscTelem_Data *out);

/* Drehzahl in U/min, -1 = keine gueltige Telemetrie */
int32_t EscTelem_GetRPM(void);

/* Temperatur in Grad C, -128 = keine gueltige Telemetrie */
int32_t EscTelem_GetTempC(void);

void EscTelem_GetStats(EscTelem_Stats *out);

#ifdef __cplusplus
}
#endif

#endif /* ESC_TELEM_H */
