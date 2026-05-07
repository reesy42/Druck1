/**
 * @file    ms5525dso.c
 * @brief   Implementierung des MS5525DSO-Treibers
 *
 * Kompensationsformeln nach Datenblatt DS-MS5525DSO (Tabelle 1).
 * Alle Zwischenwerte werden in 64-Bit-Integer gehalten, um
 * Überläufe bei der vorgeschriebenen Skalierung zu vermeiden.
 */

#include "ms5525dso.h"
#include "cmsis_os.h"
//#include "../inc/test.h"
/*------Testpin---------*/
void TPWrite(){
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, GPIO_PIN_SET);
}
void TPClear(){
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, GPIO_PIN_RESET);
}
void TPInit(){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_GPIOI_CLK_ENABLE();
		GPIO_InitStruct.Pin   = GPIO_PIN_2;
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, GPIO_PIN_RESET);
}	
/* ------------------------------------------------------------------ */
/*  Interne Hilfsfunktionen                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief  Sendet ein 1-Byte-Kommando (CS wird intern gesteuert).
 */
static HAL_StatusTypeDef spi_send_cmd(MS5525_Handle_t *dev, uint8_t cmd)
{
    HAL_StatusTypeDef ret;
    MS5525_CS_LOW();
    ret = HAL_SPI_Transmit(dev->hspi, &cmd, 1, 10);
    MS5525_CS_HIGH();
    return ret;
}

/**
 * @brief  Liest einen 16-Bit-PROM-Wert (Adresse 0..7).
 */
static HAL_StatusTypeDef prom_read(MS5525_Handle_t *dev, uint8_t addr, uint16_t *value)
{
    uint8_t cmd  = MS5525_CMD_PROM_READ | (addr << 1);
    uint8_t buf[2] = {0};

    MS5525_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(dev->hspi, &cmd, 1, 10);
    if (ret == HAL_OK)
        ret = HAL_SPI_Receive(dev->hspi, buf, 2, 10);
    MS5525_CS_HIGH();

    *value = ((uint16_t)buf[0] << 8) | buf[1];
    return ret;
}

/**
 * @brief  Startet eine Wandlung und liest danach 24-Bit ADC-Wert.
 * @param  conv_cmd  Kommando (z.B. MS5525_CMD_CONV_D1_4096)
 * @param  delay_ms  Wartezeit in ms bis Wandlung fertig
 * @param  result    Zeiger auf 32-Bit-Ergebnis
 */
static HAL_StatusTypeDef adc_convert(MS5525_Handle_t *dev,
                                     uint8_t conv_cmd,
                                     uint32_t delay_ms,
                                     uint32_t *result)
{
	   TPWrite();
    /* 1. Wandlungskommando senden */
    HAL_StatusTypeDef ret = spi_send_cmd(dev, conv_cmd);
    if (ret != HAL_OK) return ret;
		TPWrite();
    /* 2. Auf Wandlungsende warten (osDelay gibt CPU an andere Tasks frei) */
    osDelay(delay_ms);
    TPClear();
    /* 3. ADC auslesen: 1 Byte Kommando + 3 Bytes Daten */
    uint8_t cmd  = MS5525_CMD_ADC_READ;
    uint8_t buf[3] = {0};

    MS5525_CS_LOW();
    ret = HAL_SPI_Transmit(dev->hspi, &cmd, 1, 10);
    if (ret == HAL_OK)
        ret = HAL_SPI_Receive(dev->hspi, buf, 3, 10);
    MS5525_CS_HIGH();
    
    *result = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    return ret;
}

/**
 * @brief  CRC-4 Prüfung des PROM-Inhalts (Datenblatt Abschnitt PROM CRC).
 * @retval true  = CRC korrekt
 */
static bool crc4_check(uint16_t prom[MS5525_PROM_SIZE])
{
    uint16_t n_rem    = 0;
    uint16_t crc_read = prom[7] & 0x000F;

    prom[7] = prom[7] & 0xFF00;   /* CRC-Nibble löschen */

    for (int cnt = 0; cnt < 16; cnt++)
    {
        uint16_t word = (cnt % 2 == 1) ? (prom[cnt >> 1] & 0x00FF)
                                       : (prom[cnt >> 1] >> 8);
        n_rem ^= (uint16_t)(word << 8);
        for (int n_bit = 8; n_bit > 0; n_bit--)
        {
            if (n_rem & 0x8000)
                n_rem = (n_rem << 1) ^ 0x3000;
            else
                n_rem <<= 1;
        }
    }

    n_rem     = (n_rem >> 12) & 0x000F;
    prom[7]  |= crc_read;          /* CRC-Nibble wiederherstellen */
    return (n_rem == crc_read);
}

/**
 * @brief  OSR → Konvertierungskommando-Offset (0,2,4,6,8).
 */
static uint8_t osr_offset(MS5525_OSR_t osr)
{
    return (uint8_t)(osr * 2);
}

/**
 * @brief  OSR → Wandlungszeit in ms (mit 10 % Reserve).
 *         256→1ms, 512→2ms, 1024→3ms, 2048→5ms, 4096→10ms
 */
static uint32_t osr_delay_ms(MS5525_OSR_t osr)
{
    static const uint32_t delays[] = {1, 2, 3, 5, 10};
    return delays[(int)osr];
}

/* ------------------------------------------------------------------ */
/*  Öffentliche Funktionen                                             */
/* ------------------------------------------------------------------ */

HAL_StatusTypeDef MS5525_Init(MS5525_Handle_t *dev)
{
    dev->initialized = false;

    /* Reset senden und 10 ms warten (Datenblatt: min. 8,2 ms) */
    HAL_StatusTypeDef ret = spi_send_cmd(dev, MS5525_CMD_RESET);
    if (ret != HAL_OK) return ret;
    HAL_Delay(10);

    /* PROM lesen (8 × 16 Bit, Adressen 0..7) */
    for (uint8_t i = 0; i < MS5525_PROM_SIZE; i++)
    {
        ret = prom_read(dev, i, &dev->prom[i]);
        if (ret != HAL_OK) return ret;
    }

    /* CRC prüfen */
    if (!crc4_check(dev->prom))
        return HAL_ERROR;

    dev->initialized = true;
    return HAL_OK;
}

HAL_StatusTypeDef MS5525_Read(MS5525_Handle_t *dev, MS5525_OSR_t osr)
{   
	  /*-------Werte für 5525DSO -pp001DS-----------------------*/
	  #define Q1 15
		#define Q2 17
		#define Q3 7
		#define Q4 5
		#define Q5 7
		#define Q6 21
	
    if (!dev->initialized)
        return HAL_ERROR;

    uint8_t  off = osr_offset(osr);
    uint32_t dly = osr_delay_ms(osr);
    uint32_t D1  = 0;   /* Roh-Druckwert    */
    uint32_t D2  = 0;   /* Roh-Temperaturwert */

    /* Druckwandlung */
    HAL_StatusTypeDef ret = adc_convert(dev,MS5525_CMD_CONV_D1_256 + off, dly, &D1);
    if (ret != HAL_OK) return ret;

    /* Temperaturwandlung */
    ret = adc_convert(dev, MS5525_CMD_CONV_D2_256 + off, dly, &D2);
    if (ret != HAL_OK) return ret;

    /* ---------------------------------------------------------------- *
     * Kompensation laut Datenblatt                                      *
     * Koeffizienten:                                                     *
     *   C1 = prom[1]  Druckempfindlichkeit                              *
     *   C2 = prom[2]  Offset bei Referenztemperatur                     *
     *   C3 = prom[3]  Temperaturkoeffizient des Drucks                  *
     *   C4 = prom[4]  Temperaturkoeffizient des Offsets                 *
     *   C5 = prom[5]  Referenztemperatur                                *
     *   C6 = prom[6]  Temperaturkoeffizient der Empfindlichkeit         *
		 *   
     * ---------------------------------------------------------------- */
    int64_t C1 = dev->prom[1];
    int64_t C2 = dev->prom[2];
    int64_t C3 = dev->prom[3];
    int64_t C4 = dev->prom[4];
    int64_t C5 = dev->prom[5];
    int64_t C6 = dev->prom[6];

    /* Temperaturdifferenz zum Referenzwert */
    int32_t dT = (int32_t)D2 - (int32_t)(C5 << Q5);

    /* Tatsächliche Temperatur in m°C */
    int32_t TEMP = 2000 + (int32_t)(((int64_t)dT * C6) >> Q6);

    /* Offset bei aktueller Temperatur */
    int64_t OFF  = (C2 << Q2) + ((C4 * (int64_t)dT) >> Q4);

    /* Empfindlichkeit bei aktueller Temperatur */
    int64_t SENS = (C1 << Q1) + ((C3 * (int64_t)dT) >> Q3);

    /* Second order Kompensation (niedrige Temperaturen < 20 °C)  ???*/
    if (TEMP < 2000)
    {
        int32_t T2    = (int32_t)(((int64_t)dT * dT) >> 31);
        int64_t diff  = TEMP - 2000;
        int64_t OFF2  = (5 * diff * diff) >> 1;
        int64_t SENS2 = (5 * diff * diff) >> 2;

        if (TEMP < -1500)
        {
            int64_t diff2 = TEMP + 1500;
            OFF2  += 7 * diff2 * diff2;
            SENS2 += (11 * diff2 * diff2) >> 1;
        }

        TEMP -= T2;
        OFF  -= OFF2;
        SENS -= SENS2;
    }

    /* Kompensierter Druck in Pa (÷ 100 für mbar) */
    int32_t P = (int32_t)(((((int64_t)D1 * SENS) >> 21) - OFF) >> 15);

    dev->pressure_Pa       = P;
    dev->temperature_mDegC = TEMP;

    return HAL_OK;
}

int32_t MS5525_GetPressure_Pa(const MS5525_Handle_t *dev)
{
    return dev->pressure_Pa;
}

float MS5525_GetPressure_mbar(const MS5525_Handle_t *dev)
{
	  float f=0;
	  f= (float)dev->pressure_Pa / 100.0f;
    return f;
}

int32_t MS5525_GetTemperature_mDegC(const MS5525_Handle_t *dev)
{   int32_t t=0;
	  t= dev->temperature_mDegC;
    return t;
}
