/**
 * @file    abp2d.h
 * @brief   Treiber für den Honeywell ABP2DANT001 Differenzdrucksensor (SPI2)
 *          STM32F746G-Discovery Board – Ersatz für den MS5525DSO (ms5525dso.h)
 *
 * Anschluss (wie zuvor MS5525DSO, SPI2):
 *   VDD       -> 3.3V
 *   GND       -> GND
 *   SCLK      -> PI1   (SPI2_SCK)
 *   SDO/MISO  -> PB14  (SPI2_MISO)
 *   SDI/MOSI  -> PB15  (SPI2_MOSI)
 *   CSB       -> PA8   (GPIO, Software-CS)
 *
 * Protokoll laut Honeywell ABP2-Datenblatt (SPI Mode 0, MSB first, fSCLK 50..800 kHz):
 *   1. Messkommando 0xAA 0x00 0x00 senden -> Sensor verlässt Standby und startet Wandlung
 *   2. mind. 5 ms warten (oder Busy-Bit im Statusbyte pollen)
 *   3. NOP-Kommando 0xF0 + 6 Dummy-Bytes senden und dabei 7 Bytes empfangen:
 *        Byte 0    = Statusbyte
 *        Byte 1..3 = Druck,       24 bit
 *        Byte 4..6 = Temperatur,  24 bit
 *   Der Sensor liefert bereits kompensierte Werte (kein PROM/CRC-Handling wie beim MS5525DSO).
 */

#ifndef ABP2D_H
#define ABP2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Kommandos laut Datenblatt                                          */
/* ------------------------------------------------------------------ */
#define ABP2D_CMD_MEASURE_B0    0xAA  /*!< Messkommando, Byte 0 (gefolgt von 0x00 0x00) */
#define ABP2D_CMD_NOP           0xF0  /*!< NOP-Kommando zum Auslesen von Status/Druck/Temperatur */

/* ------------------------------------------------------------------ */
/*  Statusbyte-Bits (Bit 0 des empfangenen Bytes = LSB)                */
/* ------------------------------------------------------------------ */
#define ABP2D_STATUS_POWERED    (1u << 6)  /*!< 1 = Sensor mit Spannung versorgt          */
#define ABP2D_STATUS_BUSY       (1u << 5)  /*!< 1 = Wandlung noch nicht abgeschlossen      */
#define ABP2D_STATUS_MEM_ERROR  (1u << 2)  /*!< 1 = Checksummenfehler (nur nach Power-Up)  */
#define ABP2D_STATUS_MATH_SAT   (1u << 0)  /*!< 1 = interne Rechensättigung aufgetreten    */

/* ------------------------------------------------------------------ */
/*  Digitale Ausgangswerte (Transfer Function A: 10 % .. 90 % von      */
/*  2^24-1 Counts, siehe Datenblatt Tabelle 6 / Gleichung 2)           */
/* ------------------------------------------------------------------ */
#define ABP2D_OUTPUT_MIN        1677722u   /*!< 10 % von 0xFFFFFF */
#define ABP2D_OUTPUT_MAX        15099494u  /*!< 90 % von 0xFFFFFF */
#define ABP2D_OUTPUT_FULLSCALE  16777215   /*!< 2^24 - 1           */

/* Temperaturbereich ist fest (Datenblatt Gleichung 3): -50..+150 °C   */
#define ABP2D_TEMP_MIN_mDEGC    (-50000)
#define ABP2D_TEMP_MAX_mDEGC    (150000)

/* ------------------------------------------------------------------ */
/*  Druckbereich des Sensors (Pmin/Pmax laut Bestellcode)              */
/*  Vollständige Bestellnummer: ABP2DANT001PGSA3XX                     */
/*  "PG" = psi Gauge -> unidirektional 0..1 psi (kein Differenzsensor) */
/* ------------------------------------------------------------------ */
#define ABP2D_PMIN_PA           (    0)  /*!< 0 psi in Pa   (Gauge-Nullpunkt) */
#define ABP2D_PMAX_PA           ( 6895)  /*!< +1 psi in Pa */

/* CS-Pin (Software-gesteuert) – identisch zum bisherigen MS5525DSO-CS */
#define ABP2D_CS_PORT           GPIOA
#define ABP2D_CS_PIN            GPIO_PIN_8

/* Makros zum Setzen/Löschen des CS-Pins                                  */
#define ABP2D_CS_LOW()   HAL_GPIO_WritePin(ABP2D_CS_PORT, ABP2D_CS_PIN, GPIO_PIN_RESET)
#define ABP2D_CS_HIGH()  HAL_GPIO_WritePin(ABP2D_CS_PORT, ABP2D_CS_PIN, GPIO_PIN_SET)

/* Wartezeit nach dem Messkommando bis die Wandlung fertig ist (Datenblatt: min. 5 ms) */
#define ABP2D_CONV_DELAY_MS     6

/* ------------------------------------------------------------------ */
/*  Sensorzustand                                                      */
/* ------------------------------------------------------------------ */
typedef struct {
    SPI_HandleTypeDef *hspi;            /*!< Zeiger auf SPI-Handle    */
    uint8_t  status;                    /*!< zuletzt gelesenes Statusbyte */
    int32_t  pressure_Pa;               /*!< Kompensierter Druck [Pa] */
    int32_t  temperature_mDegC;         /*!< Temperatur [m°C]         */
    bool     initialized;
} ABP2D_Handle_t;

/* ------------------------------------------------------------------ */
/*  Öffentliche Funktionen                                             */
/* ------------------------------------------------------------------ */

/**
 * @brief  Initialisierung: eine Messung auslösen und Statusbyte prüfen
 *         (Spannungsversorgung ok, kein Speicherfehler). Der ABP2 besitzt
 *         kein PROM/CRC wie der MS5525DSO, die Kompensation läuft intern.
 * @param  dev   Zeiger auf Handle (hspi muss vorher gesetzt sein)
 * @retval HAL_OK bei Erfolg, sonst Fehlercode
 */
HAL_StatusTypeDef ABP2D_Init(ABP2D_Handle_t *dev);

/**
 * @brief  Einzelmessung: Druck + Temperatur lesen (bereits kompensiert).
 * @param  dev   Initialisiertes Handle
 * @retval HAL_OK bei Erfolg
 */
HAL_StatusTypeDef ABP2D_Read(ABP2D_Handle_t *dev);

/**
 * @brief  Druck in Pascal zurückgeben.
 */
int32_t ABP2D_GetPressure_Pa(const ABP2D_Handle_t *dev);

/**
 * @brief  Druck in mbar zurückgeben (1 Pa = 0,01 mbar → x/100).
 */
float   ABP2D_GetPressure_mbar(const ABP2D_Handle_t *dev);

/**
 * @brief  Druck in mmH2O zurückgeben (1 Pa = 0,102 mmH2O → x/9,80665).
 */
float   ABP2D_GetPressure_mmh2o(const ABP2D_Handle_t *dev);

/**
 * @brief  Temperatur in m°C zurückgeben.
 */
int32_t ABP2D_GetTemperature_mDegC(const ABP2D_Handle_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* ABP2D_H */
