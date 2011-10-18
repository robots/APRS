/* note:
 *
 *
 * TODO:
 * - fix freq input
 */
#include "ad.h"

#include <io/stm32_ints.h>
#include <drv/gpio_stm32.h>
#include <drv/clock_stm32.h>
#include <drv/irq_cm3.h>

#include "stm32f10x_conf.h"

Afsk *ad_afsk;

#define ADC_GPIO_BASE    ((struct stm32_gpio *)GPIOB_BASE)
#define ADC_CH1_PIN      BV(1)
#define ADC_CH1_CHANNEL  ADC_Channel_1
 
static void ADC_Enable(ADC_TypeDef* ADCx)
{
	ADC_Cmd(ADCx, ENABLE);

	/* do calibration */
	ADC_ResetCalibration(ADCx);
	while(ADC_GetResetCalibrationStatus(ADCx));
	ADC_StartCalibration(ADCx);
	while(ADC_GetCalibrationStatus(ADCx));
}

static void AD_Reset()
{
	ADC_InitTypeDef ADC_InitStructure;

	//ADC_DeInit(ADC1);
	RCC->APB2RSTR |= RCC_APB2_ADC1;
	RCC->APB2RSTR &= ~RCC_APB2_ADC1;

	/* ADC configuration ------------------------------------------------------*/
	ADC_InitStructure.ADC_Mode = ADC_Mode_RegInjecSimult; //ADC_Mode_RegSimult;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;

	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_ExternalTrigConvCmd(ADC1, ENABLE);

	ADC_RegularChannelConfig(ADC1, ADC_CH1_CHANNEL, 1, ADC_SampleTime_239Cycles5);

	ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);

	/* enable and calibrate ADCs */
	ADC_Enable(ADC1);

}

void AD_SetTimer(uint16_t prescaler, uint16_t period)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {
		.TIM_ClockDivision = 0x0,
		.TIM_CounterMode = TIM_CounterMode_Up,
  	.TIM_RepetitionCounter = 0x0000
	};

	//TIM_DeInit(TIM3);
	RCC->APB1RSTR |= RCC_APB1_TIM3;
	RCC->APB1RSTR &= ~RCC_APB1_TIM3;

	if ((period <= 3) || (prescaler <= 1)) {
		return;
	}

	/* Time Base configuration */
	TIM_TimeBaseStructure.TIM_Period = period - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = prescaler - 1;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);
}

static void AD_ISR(void)
{
	ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);

	int16_t data = ((int16_t)((ADC1->DR) >> 4) - 128);
	afsk_adc_isr(ad_afsk, data);
}

void AD_Init(Afsk *afsk)
{
	//GPIO_InitTypeDef GPIO_InitStructure;

	// Analog input 
  //GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  //GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  //GPIO_Init(GPIOA, &GPIO_InitStructure);

	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	stm32_gpioPinConfig(ADC_GPIO_BASE, ADC_CH1_PIN, GPIO_MODE_AIN, GPIO_SPEED_50MHZ);

	RCC->APB2ENR |= RCC_APB2_ADC1;
	RCC->APB1ENR |= RCC_APB1_TIM3;

	/* init ad converters */
	AD_Reset();

	ad_afsk = afsk;

	sysirq_setHandler(ADC_IRQHANDLER, AD_ISR);
}

void AD_Start()
{
	/* TIM3 counter enable */
	TIM_Cmd(TIM3, ENABLE);
}

void AD_Stop()
{
	/* TIM3 counter disable */
	TIM_Cmd(TIM3, DISABLE);

	AD_Reset();
}


