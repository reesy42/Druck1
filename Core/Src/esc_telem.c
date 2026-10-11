#include "esc_telem.h"
#include "main.h"   /* HAL */

/* ---------------------------------------------------------------------------
 * ESC-Telemetrie (KISS-Protokoll, von AM32 uebernommen) auf PC7 / USART6_RX.
 *
 * Elektrik: Der Telemetrie-Ausgang des ESC zieht aktiv nach LOW, die
 * steigende Flanke ist eine RC-Kurve (Oszi 2026-10-10, tau ~3-4 us). Ein
 * externer 2,2-kOhm-Pull-up hat das Signal verschlechtert und den LOW-Pegel
 * angehoben -> hier nur der interne Pull-up (~40 kOhm), der nur den Ruhepegel
 * definiert, wenn der ESC aus ist oder die Leitung offen ist.
 * Der USART tastet jedes Bit 3x in der Bitmitte ab (OVER16, ONEBIT=0, Mehrheit),
 * die langsame steigende Flanke ist bei 8,7 us Bitzeit dadurch unkritisch.
 *
 * Paket (10 Bytes, Big Endian):
 *   [0] Temperatur C | [1..2] Spannung 0,01 V | [3..4] Strom 0,01 A
 *   [5..6] Verbrauch mAh | [7..8] eRPM / 100 | [9] CRC8 (Poly 0x07, Init 0)
 *
 * Framing: Der ESC sendet nur auf Anforderung (Telemetrie-Bit im DShot-Frame,
 * 100 Hz). Zwischen zwei Paketen liegt die Leitung ~9 ms still -> die
 * IDLE-Erkennung des USART markiert das Paketende. Ein Burst zaehlt nur, wenn
 * er genau 10 Bytes hat, ohne Framing-/Overrun-Fehler und mit gueltiger CRC.
 *
 * Kein HAL-UART-Modul (in stm32f7xx_hal_conf.h deaktiviert), daher
 * registerbasiert. Der ISR ruft keine FreeRTOS-API auf.
 * ------------------------------------------------------------------------- */

#define ESC_TELEM_BAUD      115200U
#define ESC_TELEM_PKT_LEN   10U

static volatile uint8_t  s_rxBuf[ESC_TELEM_PKT_LEN];
static volatile uint8_t  s_rxLen;
static volatile uint8_t  s_rxBad;      /* Framing-/Overrun-Fehler im laufenden Burst */

static volatile EscTelem_Data  s_data;
static volatile uint8_t        s_dataValid;
static volatile EscTelem_Stats s_stats;

static uint8_t EscTelem_Crc8(const volatile uint8_t *buf, uint32_t len)
{
  uint8_t crc = 0U;
  for (uint32_t i = 0U; i < len; ++i)
  {
    crc ^= buf[i];
    for (int b = 0; b < 8; ++b)
    {
      crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x07U) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

void EscTelem_Init(void)
{
  /* --- PC7 = USART6_RX (AF8), interner Pull-up --- */
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin       = GPIO_PIN_7;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;   /* nur RX, Pin wird nie getrieben */
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* --- USART6: Takt = PCLK2 (108 MHz), 115200 8N1, OVER16, nur Empfang --- */
  __HAL_RCC_USART6_CONFIG(RCC_USART6CLKSOURCE_PCLK2);
  __HAL_RCC_USART6_CLK_ENABLE();
  __HAL_RCC_USART6_FORCE_RESET();
  __HAL_RCC_USART6_RELEASE_RESET();

  uint32_t pclk = HAL_RCC_GetPCLK2Freq();
  USART6->BRR = (pclk + ESC_TELEM_BAUD / 2U) / ESC_TELEM_BAUD;   /* 108 MHz -> 938 */
  USART6->CR2 = 0U;   /* 1 Stoppbit */
  USART6->CR3 = 0U;   /* 3-fach-Abtastung (Mehrheitsentscheid), Overrun-Erkennung an */
  USART6->CR1 = USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_IDLEIE;
  USART6->CR1 |= USART_CR1_UE;

  s_rxLen = 0U;
  s_rxBad = 0U;

  /* Prioritaet 3: unterhalb von DShot (DMA 1, TIM5 2), damit Telemetrie das
   * DShot-Timing nie stoert; oberhalb der FreeRTOS-Syscall-Zone (5..15), die
   * vor osKernelStart() maskiert ist (siehe dshot.c). */
  HAL_NVIC_SetPriority(USART6_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(USART6_IRQn);
}

/* Burst nach IDLE auswerten (laeuft im ISR) */
static void EscTelem_ProcessBurst(void)
{
  if (s_rxBad)
  {
    return;   /* bereits als lineErr gezaehlt */
  }
  if (s_rxLen != ESC_TELEM_PKT_LEN)
  {
    s_stats.lenErr++;
    return;
  }
  if (EscTelem_Crc8(s_rxBuf, ESC_TELEM_PKT_LEN - 1U) != s_rxBuf[ESC_TELEM_PKT_LEN - 1U])
  {
    s_stats.crcErr++;
    return;
  }

  s_data.temp_c          = (int8_t)s_rxBuf[0];
  s_data.voltage_cV      = (uint16_t)((s_rxBuf[1] << 8) | s_rxBuf[2]);
  s_data.current_cA      = (uint16_t)((s_rxBuf[3] << 8) | s_rxBuf[4]);
  s_data.consumption_mAh = (uint16_t)((s_rxBuf[5] << 8) | s_rxBuf[6]);
  s_data.erpm            = (uint32_t)((s_rxBuf[7] << 8) | s_rxBuf[8]) * 100U;
  s_data.tick            = HAL_GetTick();
  s_dataValid = 1U;
  s_stats.ok++;
}

void USART6_IRQHandler(void)
{
  uint32_t isr = USART6->ISR;

  if (isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_PE))
  {
    USART6->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_PECF;
    if (!s_rxBad)
    {
      s_stats.lineErr++;
    }
    s_rxBad = 1U;
  }
  if (isr & USART_ISR_NE)
  {
    /* Nur Statistik: Mehrheitsentscheid hat das Bit trotzdem bestimmt,
     * ob das Paket stimmt, entscheidet die CRC. */
    USART6->ICR = USART_ICR_NCF;
    s_stats.noise++;
  }

  if (isr & USART_ISR_RXNE)
  {
    uint8_t b = (uint8_t)USART6->RDR;   /* Lesen loescht RXNE */
    if (s_rxLen < ESC_TELEM_PKT_LEN)
    {
      s_rxBuf[s_rxLen] = b;
    }
    if (s_rxLen < 0xFFU)
    {
      s_rxLen++;
    }
  }

  /* Nach RXNE pruefen: kommen letztes Byte und IDLE im selben Durchlauf an,
   * muss das Byte schon im Puffer sein. */
  if (isr & USART_ISR_IDLE)
  {
    USART6->ICR = USART_ICR_IDLECF;
    EscTelem_ProcessBurst();
    s_rxLen = 0U;
    s_rxBad = 0U;
  }
}

int EscTelem_GetData(EscTelem_Data *out)
{
  /* Nur den eigenen Interrupt sperren, DShot laeuft ungestoert weiter */
  HAL_NVIC_DisableIRQ(USART6_IRQn);
  out->temp_c          = s_data.temp_c;
  out->voltage_cV      = s_data.voltage_cV;
  out->current_cA      = s_data.current_cA;
  out->consumption_mAh = s_data.consumption_mAh;
  out->erpm            = s_data.erpm;
  out->tick            = s_data.tick;
  uint8_t valid        = s_dataValid;
  HAL_NVIC_EnableIRQ(USART6_IRQn);

  return (valid && (HAL_GetTick() - out->tick) <= ESC_TELEM_TIMEOUT_MS) ? 1 : 0;
}

int32_t EscTelem_GetRPM(void)
{
  EscTelem_Data d;
  if (!EscTelem_GetData(&d))
  {
    return -1;
  }
  return (int32_t)(d.erpm / (ESC_TELEM_MOTOR_POLES / 2U));
}

int32_t EscTelem_GetTempC(void)
{
  EscTelem_Data d;
  if (!EscTelem_GetData(&d))
  {
    return -128;
  }
  return (int32_t)d.temp_c;
}

void EscTelem_GetStats(EscTelem_Stats *out)
{
  HAL_NVIC_DisableIRQ(USART6_IRQn);
  out->ok      = s_stats.ok;
  out->crcErr  = s_stats.crcErr;
  out->lenErr  = s_stats.lenErr;
  out->lineErr = s_stats.lineErr;
  out->noise   = s_stats.noise;
  HAL_NVIC_EnableIRQ(USART6_IRQn);
}
