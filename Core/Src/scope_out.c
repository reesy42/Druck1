#include "scope_out.h"
#include "stm32f7xx_hal.h"

/* TIM3 und TIM12 haengen an APB1, Timertakt 108 MHz.
 * ARR = 1079 -> 100 kHz PWM, 1080 Stufen (~10 bit Aufloesung). */
#define SCOPE_ARR   1079U

/* Direkte Registerkonfiguration statt HAL_TIM_PWM_Init: kein Handle und
 * keine MspInit-Callbacks noetig, die mit dshot.c kollidieren koennten.
 * Die Timer laufen ohne Interrupt/DMA -> keine Last fuer CPU oder Bus. */
static void ScopeOut_TimInit(TIM_TypeDef *tim)
{
  tim->CR1   = TIM_CR1_ARPE;
  tim->PSC   = 0U;
  tim->ARR   = SCOPE_ARR;
  tim->CCR1  = 0U;
  tim->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1   /* PWM-Mode 1 */
             | TIM_CCMR1_OC1PE;                       /* CCR1-Preload */
  tim->CCER  = TIM_CCER_CC1E;
  tim->EGR   = TIM_EGR_UG;
  tim->CR1  |= TIM_CR1_CEN;
}

static uint32_t ScopeOut_Scale(float value, float fullscale)
{
  if (!(value > 0.0f)) return 0U;          /* faengt auch NaN ab */
  if (value >= fullscale) return SCOPE_ARR + 1U;  /* 100 % Tastgrad */
  return (uint32_t)(value * (float)(SCOPE_ARR + 1U) / fullscale + 0.5f);
}

void ScopeOut_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_TIM12_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  gpio.Mode  = GPIO_MODE_AF_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;

  gpio.Pin       = GPIO_PIN_4;             /* PB4 = Arduino D3 */
  gpio.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(GPIOB, &gpio);

  gpio.Pin       = GPIO_PIN_6;             /* PH6 = Arduino D6 */
  gpio.Alternate = GPIO_AF9_TIM12;
  HAL_GPIO_Init(GPIOH, &gpio);

  ScopeOut_TimInit(TIM3);
  ScopeOut_TimInit(TIM12);
}

void ScopeOut_SetPressure(float mmh2o)
{
  TIM3->CCR1 = ScopeOut_Scale(mmh2o, SCOPE_P_FS_MMH2O);
}

void ScopeOut_SetOutput(float percent)
{
  TIM12->CCR1 = ScopeOut_Scale(percent, 100.0f);
}
