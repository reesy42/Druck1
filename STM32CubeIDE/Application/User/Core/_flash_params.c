#include "flash_params.h"
#include "stm32f7xx_hal.h"

#define FLASH_SECTOR_PARAMS  FLASH_SECTOR_7
#define FLASH_ADDR_PARAMS    0x080C0000UL
#define FLASH_MAGIC          0xA55A1234UL

/* Standard-Werte falls noch nichts gespeichert */
#define DEFAULT_KP       0.5f
#define DEFAULT_KI       1.5f
#define DEFAULT_KD       0.05f
#define DEFAULT_SETPOINT 50.0f

typedef struct {
    uint32_t magic;
    float    kp;
    float    ki;
    float    kd;
    float    setpoint;
} PidFlashParams_t;

void FlashParams_Load(float *kp, float *ki, float *kd, float *setpoint)
{
    const PidFlashParams_t *p = (const PidFlashParams_t *)FLASH_ADDR_PARAMS;

    if (p->magic == FLASH_MAGIC)
    {
        *kp       = p->kp;
        *ki       = p->ki;
        *kd       = p->kd;
        *setpoint = p->setpoint;
    }
    else
    {
        *kp       = DEFAULT_KP;
        *ki       = DEFAULT_KI;
        *kd       = DEFAULT_KD;
        *setpoint = DEFAULT_SETPOINT;
    }
}

int FlashParams_Save(float kp, float ki, float kd, float setpoint)
{
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = FLASH_SECTOR_PARAMS;
    erase.NbSectors    = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    uint32_t sectorError = 0;

    if (HAL_FLASHEx_Erase(&erase, &sectorError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return -1;
    }

    uint32_t addr    = FLASH_ADDR_PARAMS;
    uint32_t words[] = {
        FLASH_MAGIC,
        *(uint32_t *)&kp,
        *(uint32_t *)&ki,
        *(uint32_t *)&kd,
        *(uint32_t *)&setpoint
    };

    for (int i = 0; i < (int)(sizeof(words) / sizeof(words[0])); ++i)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, words[i]) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return -1;
        }
        addr += 4;
    }

    HAL_FLASH_Lock();
    return 0;
}
