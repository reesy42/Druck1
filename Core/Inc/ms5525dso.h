/**
 * @file    ms5525dso.h
 * @brief   Treiber für den TE MS5525DSO Differenzdrucksensor (SPI2)
 *          STM32F746G-Discovery Board
 *
 * Anschluss (Arduino-Header):
 *   VDD       -> 3.3V  (CN6 Pin 4)
 *   GND       -> GND   (CN6 Pin 6)
 *   SCLK      -> PI1   (CN7 Pin 10 / D13 / SPI2_SCK)
 *   SDO/MISO  -> PB14  (CN7 Pin 12 / D12 / SPI2_MISO)
 *   SDI/MOSI  -> PB15  (CN7 Pin 11 / D11 / SPI2_MOSI)
 *   CSB       -> PI0   (CN7 Pin 6  / D10 / SPI2_NSS, GPIO)
 *   PS        -> GND   (SPI-Protokoll wählen)
 */

#ifndef MS5525DSO_H
#define MS5525DSO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f7xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ */
/*  Kommandos laut Datenblatt                                          */
/* ------------------------------------------------------------------ */
#define MS5525_CMD_RESET        0x1E  /*!< Reset                          */
#define MS5525_CMD_CONV_D1_256  0x40  /*!< Druck OSR=256  (0.6ms)        */
#define MS5525_CMD_CONV_D1_512  0x42  /*!< Druck OSR=512  (1.2ms)        */
#define MS5525_CMD_CONV_D1_1024 0x44  /*!< Druck OSR=1024 (2.3ms)        */
#define MS5525_CMD_CONV_D1_2048 0x46  /*!< Druck OSR=2048 (4.6ms)        */
#define MS5525_CMD_CONV_D1_4096 0x48  /*!< Druck OSR=4096 (9.1ms)        */
#define MS5525_CMD_CONV_D2_256  0x50  /*!< Temperatur OSR=256             */
#define MS5525_CMD_CONV_D2_512  0x52  /*!< Temperatur OSR=512             */
#define MS5525_CMD_CONV_D2_1024 0x54  /*!< Temperatur OSR=1024            */
#define MS5525_CMD_CONV_D2_2048 0x56  /*!< Temperatur OSR=2048            */
#define MS5525_CMD_CONV_D2_4096 0x58  /*!< Temperatur OSR=4096            */
#define MS5525_CMD_ADC_READ     0x00  /*!< ADC-Ergebnis lesen (3 Bytes)   */
#define MS5525_CMD_PROM_READ    0xA0  /*!< PROM-Basisadresse (| addr<<1)  */

/* Anzahl PROM-Koeffizienten (C1..C6 + CRC)                              */
#define MS5525_PROM_SIZE        8

/* CS-Pin (Software-gesteuert) – GPIO anpassen falls nötig               */
//#define MS5525_CS_PORT          GPIOI
//#define MS5525_CS_PIN           GPIO_PIN_0
#define MS5525_CS_PORT   GPIOA
#define MS5525_CS_PIN    GPIO_PIN_8

/* Makros zum Setzen/Löschen des CS-Pins                                  */
#define MS5525_CS_LOW()   HAL_GPIO_WritePin(MS5525_CS_PORT, MS5525_CS_PIN, GPIO_PIN_RESET)
#define MS5525_CS_HIGH()  HAL_GPIO_WritePin(MS5525_CS_PORT, MS5525_CS_PIN, GPIO_PIN_SET)

/* ------------------------------------------------------------------ */
/*  OSR-Typ                                                            */
/* ------------------------------------------------------------------ */
typedef enum {
    MS5525_OSR_256  = 0,
    MS5525_OSR_512  = 1,
    MS5525_OSR_1024 = 2,
    MS5525_OSR_2048 = 3,
    MS5525_OSR_4096 = 4
} MS5525_OSR_t;

/* ------------------------------------------------------------------ */
/*  Sensorzustand                                                      */
/* ------------------------------------------------------------------ */
typedef struct {
    SPI_HandleTypeDef *hspi;            /*!< Zeiger auf SPI-Handle    */
    uint16_t prom[MS5525_PROM_SIZE];    /*!< Kalibrierkoeffizienten   */
    int32_t  pressure_Pa;               /*!< Kompensierter Druck [Pa] */
    int32_t  temperature_mDegC;         /*!< Temperatur [m°C]         */
    bool     initialized;
} MS5525_Handle_t;

/* ------------------------------------------------------------------ */
/*  Öffentliche Funktionen                                             */
/* ------------------------------------------------------------------ */
/**
 * @brief  Initialisierung: Reset, PROM lesen, CRC prüfen.
 * @param  dev   Zeiger auf Handle (hspi muss vorher gesetzt sein)
 * @retval HAL_OK bei Erfolg, sonst Fehlercode
 */
HAL_StatusTypeDef MS5525_Init(MS5525_Handle_t *dev);

/**
 * @brief  Einzelmessung: Druck + Temperatur lesen und kompensieren.
 * @param  dev   Initialisiertes Handle
 * @param  osr   Oversampling Ratio
 * @retval HAL_OK bei Erfolg
 */
HAL_StatusTypeDef MS5525_Read(MS5525_Handle_t *dev, MS5525_OSR_t osr);

/**
 * @brief  Druck in Pascal zurückgeben.
 */
int32_t MS5525_GetPressure_Pa(const MS5525_Handle_t *dev);

/**
 * @brief  Druck in mbar zurückgeben (1 Pa = 0,01 mbar → x/100).
 */
float   MS5525_GetPressure_mbar(const MS5525_Handle_t *dev);
/**
 * @brief  Druck in mmh2o zurückgeben (1 Pa = 0,102 mmH2O → x/9,80665).
 */
float MS5525_GetPressure_mmh2o(const MS5525_Handle_t *dev);
/**
 * @brief  Temperatur in m°C zurückgeben.
 */
int32_t MS5525_GetTemperature_mDegC(const MS5525_Handle_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* MS5525DSO_H */
