/**
 * @file    abp2d.c
 * @brief   Implementierung des Honeywell ABP2DANT001-Treibers
 *
 * Kommunikationsablauf und Kompensationsformeln nach Honeywell
 * ABP2-Datenblatt (Abschnitt 7.0 SPI Communications, 8.1 Digital
 * Output Versions). Der Sensor liefert bereits temperaturkompensierte
 * 24-bit-Werte, daher entfällt die PROM/CRC-Behandlung des MS5525DSO.
 */

#include "abp2d.h"

/* ------------------------------------------------------------------ */
/*  Interne Hilfsfunktionen                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief  Sendet das Messkommando (0xAA 0x00 0x00), CS wird intern gesteuert.
 *         Die dabei empfangenen Bytes hängen vom vorherigen Kommando ab
 *         und werden verworfen (siehe Datenblatt Tabelle 27).
 */
static HAL_StatusTypeDef abp2d_send_measure_cmd(ABP2D_Handle_t *dev)
{
    uint8_t cmd[3]  = { ABP2D_CMD_MEASURE_B0, 0x00, 0x00 };
    uint8_t dump[3] = { 0 };

    ABP2D_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(dev->hspi, cmd, dump, 3, 10);
    ABP2D_CS_HIGH();
    return ret;
}

/**
 * @brief  Liest Statusbyte, Druck (24 bit) und Temperatur (24 bit) in einer
 *         Transaktion aus (NOP-Kommando + 6 Dummy-Bytes, Vollduplex).
 */
static HAL_StatusTypeDef abp2d_read_result(ABP2D_Handle_t *dev,
                                            uint8_t *status,
                                            uint32_t *pressCounts,
                                            uint32_t *tempCounts)
{
    uint8_t tx[7] = { ABP2D_CMD_NOP, 0, 0, 0, 0, 0, 0 };
    uint8_t rx[7] = { 0 };

    ABP2D_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 7, 10);
    ABP2D_CS_HIGH();
    if (ret != HAL_OK) {
        return ret;
    }

    *status      = rx[0];
    *pressCounts = ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
    *tempCounts  = ((uint32_t)rx[4] << 16) | ((uint32_t)rx[5] << 8) | rx[6];
    return HAL_OK;
}

/**
 * @brief  Führt eine vollständige Messung durch: Kommando senden, auf die
 *         Wandlung warten und Ergebnis auslesen.
 */
static HAL_StatusTypeDef abp2d_measure(ABP2D_Handle_t *dev,
                                        uint8_t *status,
                                        uint32_t *pressCounts,
                                        uint32_t *tempCounts)
{
    HAL_StatusTypeDef ret = abp2d_send_measure_cmd(dev);
    if (ret != HAL_OK) {
        return ret;
    }

    HAL_Delay(ABP2D_CONV_DELAY_MS);

    return abp2d_read_result(dev, status, pressCounts, tempCounts);
}

/* ------------------------------------------------------------------ */
/*  Öffentliche Funktionen                                             */
/* ------------------------------------------------------------------ */

HAL_StatusTypeDef ABP2D_Init(ABP2D_Handle_t *dev)
{
    dev->initialized = false;

    uint8_t  status      = 0;
    uint32_t pressCounts = 0;
    uint32_t tempCounts  = 0;

    HAL_StatusTypeDef ret = abp2d_measure(dev, &status, &pressCounts, &tempCounts);
    if (ret != HAL_OK) {
        return ret;
    }

    dev->status = status;
    if (status & ABP2D_STATUS_MEM_ERROR) {
        return HAL_ERROR;
    }
    if (!(status & ABP2D_STATUS_POWERED)) {
        return HAL_ERROR;
    }

    dev->initialized = true;
    return HAL_OK;
}

HAL_StatusTypeDef ABP2D_Read(ABP2D_Handle_t *dev)
{
    if (!dev->initialized) {
        return HAL_ERROR;
    }

    uint8_t  status      = 0;
    uint32_t pressCounts = 0;
    uint32_t tempCounts  = 0;

    HAL_StatusTypeDef ret = abp2d_measure(dev, &status, &pressCounts, &tempCounts);
    if (ret != HAL_OK) {
        return ret;
    }

    dev->status = status;
    if (status & ABP2D_STATUS_MEM_ERROR) {
        return HAL_ERROR;
    }

    /* Druck: Transfer Function A, 10 % .. 90 % von 2^24-1 Counts (Gleichung 2) */
    int64_t p = (int64_t)(pressCounts - ABP2D_OUTPUT_MIN) * (ABP2D_PMAX_PA - ABP2D_PMIN_PA);
    p = p / (int64_t)(ABP2D_OUTPUT_MAX - ABP2D_OUTPUT_MIN) + ABP2D_PMIN_PA;
    dev->pressure_Pa = (int32_t)p;

    /* Temperatur: -50..+150 °C über volle 24-bit-Skala (Gleichung 3) */
    int64_t t = (int64_t)tempCounts * (ABP2D_TEMP_MAX_mDEGC - ABP2D_TEMP_MIN_mDEGC) / ABP2D_OUTPUT_FULLSCALE;
    dev->temperature_mDegC = (int32_t)(t + ABP2D_TEMP_MIN_mDEGC);

    return HAL_OK;
}

int32_t ABP2D_GetPressure_Pa(const ABP2D_Handle_t *dev)
{
    return dev->pressure_Pa;
}

float ABP2D_GetPressure_mbar(const ABP2D_Handle_t *dev)
{
    return (float)dev->pressure_Pa / 100.0f;
}

float ABP2D_GetPressure_mmh2o(const ABP2D_Handle_t *dev)
{
    return (float)dev->pressure_Pa / 9.80665f;
}

int32_t ABP2D_GetTemperature_mDegC(const ABP2D_Handle_t *dev)
{
    return dev->temperature_mDegC;
}
