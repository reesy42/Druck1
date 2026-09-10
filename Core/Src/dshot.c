#include "dshot.h"
#include "main.h"   /* HAL + Error_Handler() */

/* ---------------------------------------------------------------------------
 * Unidirektionales DShot600 (Sendeseite) auf PA15 (Arduino D9), TIM2_CH1 (AF1),
 * DMA1 Stream5 Kanal 3. Keine Telemetrie (keine eRPM-Rueckmeldung).
 *
 * TIM2 haengt an APB1, Timertakt 108 MHz:
 *   Bitzeit = 1 / 600 kHz = 1.667 us  ->  180 Ticks   => PSC = 0, ARR = 179
 *   "0"-Bit: 37.5 % aktiv -> 68 Ticks (629.6 ns, gerundet von 67.5 -
 *            180 ist nicht durch 8 teilbar, per Scope verifiziert: 631.2 ns
 *            gemessen, praktisch identisch mit dem Sollwert)
 *   "1"-Bit: 75.0 % aktiv -> 135 Ticks (1.25 us, exakt)
 *   Normale Polaritaet: Leitung ruht LOW, Bits ziehen sie HIGH.
 *
 * Frame (16 Bit, MSB zuerst):
 *   [15..5] 11 Bit Throttle | [4] Telemetrie-Anforderung | [3..0] CRC
 *
 * Der Puffer haengt zwei Nullwoerter an. Dadurch geht die Leitung nach dem
 * letzten Bit in den Ruhezustand (LOW) und der ESC erkennt das Frame-Ende.
 * ------------------------------------------------------------------------- */

#define DSHOT_ARR         179U
#define DSHOT_BIT0        68U
#define DSHOT_BIT1        135U

#define DSHOT_FRAME_BITS   16
#define DSHOT_BUF_LEN     (DSHOT_FRAME_BITS + 2)

#define DSHOT_MIN_THROTTLE  48U
#define DSHOT_MAX_THROTTLE  2047U

static TIM_HandleTypeDef htim2;
static DMA_HandleTypeDef hdma_tim2_ch1;

/* TIM2 ist ein 32-Bit-Timer -> CCR1 wortbreit uebertragen.
 * 32-Byte-Alignment, damit SCB_CleanDCache_by_Addr sauber auf Cache-Zeilen passt. */
static uint32_t dshotBuf[DSHOT_BUF_LEN] __attribute__((aligned(32)));

/* ---------------------------------------------------------------------------
 * DMA1 Stream5 Kanal 3 = TIM2_CH1
 * ------------------------------------------------------------------------- */
static void DShot_DmaInit(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();

  hdma_tim2_ch1.Instance                 = DMA1_Stream5;
  hdma_tim2_ch1.Init.Channel             = DMA_CHANNEL_3;
  hdma_tim2_ch1.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_tim2_ch1.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tim2_ch1.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tim2_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_tim2_ch1.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
  hdma_tim2_ch1.Init.Mode                = DMA_NORMAL;
  hdma_tim2_ch1.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
  /* FIFO aktiv, aber Schwelle HALFFULL (2 Worte) statt FULL (4): DSHOT_BUF_LEN=18
   * ist durch 2, aber nicht durch 4 teilbar - mit FULL entstand dadurch ein
   * FIFO-Underrun (FEIF) am Blockende, der zu abgeschnittenen/zu kurzen Frames
   * fuehrte (siehe project-odem-luefter-esc Memory, 2026-09-09/10). Test mit
   * FIFOMode=DISABLE (Direct Mode) hat das Problem NICHT behoben - FIFO-
   * Underrun ist also vermutlich nicht die (alleinige) Ursache, wird hier
   * trotzdem als naechster dokumentierter Versuch umgesetzt. */
  hdma_tim2_ch1.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
  hdma_tim2_ch1.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
  hdma_tim2_ch1.Init.MemBurst            = DMA_MBURST_SINGLE;
  hdma_tim2_ch1.Init.PeriphBurst         = DMA_PBURST_SINGLE;

  if (HAL_DMA_Init(&hdma_tim2_ch1) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_LINKDMA(&htim2, hdma[TIM_DMA_ID_CC1], hdma_tim2_ch1);

  /* Prioritaet 1 (oberhalb von configMAX_SYSCALL_INTERRUPT_PRIORITY = 5):
   * Der Handler ruft keine FreeRTOS-API auf, braucht die "syscall-sichere"
   * Zone (5..15) also nicht - und die lag vor osKernelStart() dauerhaft
   * maskiert (TIM6/HAL-Tick liegt mit Prioritaet 0 aus demselben Grund
   * ausserhalb dieser Zone und lief deshalb schon vorher zuverlaessig). */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

void DShot_Init(void)
{
  /* --- GPIO PA15, AF1 = TIM2_CH1 ---
   * PA15 ist nach Reset JTDI. Das Board debuggt ueber SWD (PA13/PA14),
   * die JTAG-Pins sind frei und werden hier einfach umkonfiguriert. */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin       = GPIO_PIN_15;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLDOWN;   /* Unidir-DShot: Leitung ruht LOW */
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;   /* wie Referenz: langsamere Flanke gegen Ringing/Unterschwinger an der ESC-Last */
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* --- TIM2: 600 kHz Bittakt --- */
  __HAL_RCC_TIM2_CLK_ENABLE();
  htim2.Instance               = TIM2;
  htim2.Init.Prescaler         = 0U;           /* 108 MHz ungeteilt */
  htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim2.Init.Period            = DSHOT_ARR;    /* 180 Ticks = 1.667 us */
  htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode     = TIM_OCMODE_PWM1;
  sConfigOC.Pulse      = 0U;                   /* CCR=0 -> Ausgang inaktiv (nach Polaritaet: LOW) */
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;   /* Normale Polaritaet: Leitung ruht LOW */
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  DShot_DmaInit();
}

/* ---------------------------------------------------------------------------
 * Frame bauen: 12 Bit Nutzdaten + 4 Bit CRC
 * ------------------------------------------------------------------------- */
static uint16_t DShot_MakeFrame(uint16_t value, uint8_t telemetry)
{
  uint16_t packet = (uint16_t)((value << 1) | (telemetry & 1U));  /* 12 Bit */

  uint16_t csum = 0U;
  uint16_t tmp  = packet;
  for (int i = 0; i < 3; ++i)
  {
    csum ^= tmp;
    tmp >>= 4;
  }
  return (uint16_t)((packet << 4) | (csum & 0x0FU));
}

void DShot_SendRaw(uint16_t value)
{
  if (value > DSHOT_MAX_THROTTLE)
  {
    value = DSHOT_MAX_THROTTLE;
  }

  /* Laeuft noch ein DMA-Transfer aus dshotBuf, darf der Puffer jetzt nicht
   * ueberschrieben werden - das waere ein "zerrissener" Frame (CPU schreibt
   * neue Bits, waehrend DMA die alten noch ausliest -> falsche CRC/Werte
   * auf der Leitung). Bei 4 kHz-Wiederholung (250 us Periode) reicht es,
   * diesen Aufruf einfach auszulassen; der naechste TIM5-Tick sendet dann. */
  if (hdma_tim2_ch1.State == HAL_DMA_STATE_BUSY)
  {
    return;
  }

  uint16_t frame = DShot_MakeFrame(value, 1U);

  for (int i = 0; i < DSHOT_FRAME_BITS; ++i)
  {
    dshotBuf[i] = (frame & (0x8000U >> i)) ? DSHOT_BIT1 : DSHOT_BIT0;
  }
  dshotBuf[DSHOT_FRAME_BITS]     = 0U;   /* Leitung nach dem Frame low halten */
  dshotBuf[DSHOT_FRAME_BITS + 1] = 0U;

  /* D-Cache ist aktiv: Puffer vor dem DMA-Zugriff aus dem Cache zurueckschreiben. */
  SCB_CleanDCache_by_Addr((uint32_t *)dshotBuf, (int32_t)sizeof(dshotBuf));

  /* Reihenfolge kritisch (Ursache fuer "zu kurze/verschobene Frames",
   * FIFO-unabhaengig - siehe project-odem-luefter-esc Memory, 2026-09-10):
   * TIM2 laeuft seit DShot_Init() durchgehend frei. Der erste DMA-Fetch
   * wird ueber einen Compare-Match bei CNT==CCR1 ausgeloest, wobei CCR1
   * vom letzten Frame noch auf 0 steht (die zwei Nullworte am Ende) - ein
   * Counter-Reset auf 0 erzeugt also sofort diesen Match. Stand bisher
   * SET_COUNTER VOR Start_DMA: zwischen den beiden Zeilen liegen mehrere
   * TIM2-Register-Schreibzugriffe in HAL_TIM_PWM_Start_DMA - verzoegert
   * sich das um mehr als eine Bitperiode (1,667 us, z.B. durch einen
   * Bus-Stall von TouchGFX/LTDC/DMA2D), ist der DMA-Request beim
   * Scharfschalten noch nicht aktiv und der erste Match wird verpasst ->
   * Frame startet (mind.) eine Periode zu spaet. Fix: DMA/Request zuerst
   * scharf schalten, Counter-Reset als letzter Schritt direkt davor -
   * dann bleibt keine Luecke mehr fuer einen Stall zwischen "Request aktiv"
   * und "Match-Bedingung erzeugen". */
  (void)HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, dshotBuf, DSHOT_BUF_LEN);
  __HAL_TIM_SET_COUNTER(&htim2, 0U);
}

void DShot_SendPercent(uint8_t percent)
{
  if (percent > 100U)
  {
    percent = 100U;
  }

  /* 0 % -> 48 (Nullschub, Motor steht), 100 % -> 2047 */
  uint16_t value = (uint16_t)(DSHOT_MIN_THROTTLE
                 + (((uint32_t)percent * (DSHOT_MAX_THROTTLE - DSHOT_MIN_THROTTLE)
                     + 50U) / 100U));
  DShot_SendRaw(value);
}

void DShot_Arm(uint32_t ms)
{
  uint32_t start = HAL_GetTick();
  while ((HAL_GetTick() - start) < ms)
  {
    DShot_SendRaw(0U);   /* 0 = disarmed/Stop */
    HAL_Delay(1U);
    TPToggle();			//Testpin für die Anzahl der Durchläufe
  }
}

/* ---------------------------------------------------------------------------
 * DMA-Interrupt. Muss vorhanden sein, weil HAL_TIM_PWM_Start_DMA die
 * Transfer-Complete-Interrupts aktiviert - ohne Handler landet der Vektor
 * im Default_Handler (Endlosschleife).
 * ------------------------------------------------------------------------- */
void DMA1_Stream5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_tim2_ch1);
}

/* ===========================================================================
 * Automatischer Wiederholungs-Mechanismus via TIM5
 *
 * TIM5 haengt an APB1, Timertakt 108 MHz.
 * PSC=107 -> TIM-Takt = 1 MHz; ARR = (1e6/rate_hz) - 1
 * Bei 4000 Hz: ARR = 249  ->  Periode = 250 us
 *
 * Der ISR ruft DShot_SendRaw() auf, das laeuft in <<< 10 us.
 * Der DShot-Frame selbst dauert ~33 us und laeuft per DMA im Hintergrund.
 * Mit 250 us Periode gibt es genug Abstand zum naechsten Frame.
 *
 * Prioritaet TIM5_IRQn = 2, DMA1_Stream5_IRQn = 1 (beide oberhalb von
 * configMAX_SYSCALL_INTERRUPT_PRIORITY = 5, siehe DShot_DmaInit()).
 * DMA (1) kann TIM5 (2) praeemptieren - aber nie umgekehrt.
 * Weil der DMA nach 33 us fertig ist und TIM5 erst nach 250 us wieder
 * feuert, gibt es keine zeitliche Ueberschneidung.
 * =========================================================================== */

static TIM_HandleTypeDef htim5_repeat;
static volatile uint16_t s_pending_raw = 0U;

void DShot_SetRaw(uint16_t value)
{
  if (value > DSHOT_MAX_THROTTLE) value = DSHOT_MAX_THROTTLE;
  s_pending_raw = value;
}

void DShot_SetPercent(uint8_t percent)
{
  if (percent > 100U) percent = 100U;
  uint16_t value = (uint16_t)(DSHOT_MIN_THROTTLE
                 + (((uint32_t)percent * (DSHOT_MAX_THROTTLE - DSHOT_MIN_THROTTLE)
                     + 50U) / 100U));
  s_pending_raw = value;
}

void TIM5_IRQHandler(void)
{
  /* Update-Flag pruefen und loeschen */
  if (__HAL_TIM_GET_FLAG(&htim5_repeat, TIM_FLAG_UPDATE) &&
      __HAL_TIM_GET_IT_SOURCE(&htim5_repeat, TIM_IT_UPDATE))
  {
    __HAL_TIM_CLEAR_IT(&htim5_repeat, TIM_IT_UPDATE);
    DShot_SendRaw(s_pending_raw);
  }
}

void DShot_StartRepeat(uint32_t rate_hz)
{
  __HAL_RCC_TIM5_CLK_ENABLE();

  htim5_repeat.Instance               = TIM5;
  htim5_repeat.Init.Prescaler         = 107U;                   /* 108 MHz / 108 = 1 MHz */
  htim5_repeat.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim5_repeat.Init.Period            = (1000000UL / rate_hz) - 1UL;
  htim5_repeat.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim5_repeat.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  if (HAL_TIM_Base_Init(&htim5_repeat) != HAL_OK)
  {
    Error_Handler();
  }

  /* Prioritaet 2 - DMA (1) kann diesen ISR praeemptieren, nicht umgekehrt */
  HAL_NVIC_SetPriority(TIM5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM5_IRQn);

  HAL_TIM_Base_Start_IT(&htim5_repeat);
}
