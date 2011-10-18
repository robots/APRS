
#include "da.h"

#include <io/stm32_ints.h>
#include <drv/gpio_stm32.h>
#include <drv/clock_stm32.h>
#include <drv/irq_cm3.h>

#include "stm32f10x_conf.h"


Afsk *dac_afsk;

#define DAC_GPIO_BASE  ((struct stm32_gpio *)GPIOB_BASE)
#define PTT_GPIO_BASE  ((struct stm32_gpio *)GPIOA_BASE)

#define PTT_PIN        BV(12)
#define DAC_PIN_MASK  BV(1) | BV(2) | BV(5) | BV(6);

static void DA_ISR()
{
	static uint8_t dac;
	
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

	dac = afsk_dac_isr(dac_afsk);

	stm32_gpioPinWrite(DAC_GPIO_BASE, BV(1), dac & 0x80);
	stm32_gpioPinWrite(DAC_GPIO_BASE, BV(2), dac & 0x40);
	stm32_gpioPinWrite(DAC_GPIO_BASE, BV(5), dac & 0x20);
	stm32_gpioPinWrite(DAC_GPIO_BASE, BV(6), dac & 0x10);
}

void DA_Init(Afsk *afsk)
{
	RCC->APB1ENR |= RCC_APB1_TIM2;

	stm32_gpioPinConfig(DAC_GPIO_BASE, BV(1) | BV(2) | BV(5) | BV(6), GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);
	stm32_gpioPinConfig(PTT_GPIO_BASE, PTT_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);

	dac_afsk = afsk;

	sysirq_setHandler(TIM2_IRQHANDLER, DA_ISR);
}

void DA_SetTimer(uint16_t prescaler, uint16_t period)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {
		.TIM_ClockDivision = 0x0,
		.TIM_CounterMode = TIM_CounterMode_Up,
  	.TIM_RepetitionCounter = 0x0000
	};

	RCC->APB1RSTR |= RCC_APB1_TIM2;
	RCC->APB1RSTR &= ~RCC_APB1_TIM2;
	//TIM_DeInit(TIM2);

	if ((period <= 3) || (prescaler <= 1)) {
		return;
	}

	/* Time Base configuration */
	TIM_TimeBaseStructure.TIM_Period = period - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = prescaler - 1;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, DISABLE);

}

void DA_Start()
{
	stm32_gpioPinWrite(PTT_GPIO_BASE, PTT_PIN, 1);

	TIM_Cmd(TIM2, ENABLE);
}

void DA_Stop()
{
	stm32_gpioPinWrite(PTT_GPIO_BASE, PTT_PIN, 0);

	TIM_Cmd(TIM2, DISABLE);
}
