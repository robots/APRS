

#include "da.h"

#include <io/stm32_ints.h>
#include <drv/gpio_stm32.h>
#include <drv/clock_stm32.h>
#include <drv/irq_cm3.h>

#include "stm32f10x_conf.h"

#define PTT_GPIO_BASE  ((struct stm32_gpio *)GPIOA_BASE)
#define PTT_PIN        BV(0)

#define DAC_GPIO_BASE    ((struct stm32_gpio *)GPIOA_BASE)
#define DAC_CH1_PIN      BV(4)

Afsk *dac_afsk;

static void DA_ISR()
{
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

	DAC->DHR8R1 = afsk_dac_isr(dac_afsk);
	DAC->SWTRIGR |= 1;
}

void DA_Init(Afsk *afsk)
{
	//GPIO_InitTypeDef GPIO_InitStructure;
	DAC_InitTypeDef DAC_InitStructure;

	RCC->APB1ENR |= RCC_APB1_TIM2;
	RCC->APB1ENR |= RCC_APB1_DAC;

//	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
//  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	stm32_gpioPinConfig(PTT_GPIO_BASE, PTT_PIN, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);

	// Analog input 
  //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  //GPIO_Init(GPIOA, &GPIO_InitStructure);

	stm32_gpioPinConfig(DAC_GPIO_BASE, DAC_CH1_PIN, GPIO_MODE_AIN, GPIO_SPEED_50MHZ);

  DAC_InitStructure.DAC_Trigger = DAC_Trigger_Software;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);
  DAC_Cmd(DAC_Channel_1, ENABLE);

	DAC->DHR8R1 = 0;
	DAC->SWTRIGR |= 1;

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

	//TIM_DeInit(TIM2);
	RCC->APB1RSTR |= RCC_APB1_TIM2;
	RCC->APB1RSTR &= ~RCC_APB1_TIM2;

	if ((period <= 3) || (prescaler <= 1)) {
		return;
	}

	/* Time Base configuration */
	TIM_TimeBaseStructure.TIM_Period = period - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = prescaler - 1;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

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

