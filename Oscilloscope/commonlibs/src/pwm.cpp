#include "pwm.h"

void pwmInit(STM32Pin pin)
{
	int channel = -1;
	TIM_TypeDef * timer = 0;

	pwmTimerAndChannelForPin(pin, &timer, &channel);
	if( timer != 0 )
	{
		if( timer == TIM1 )
			RCC_APB1PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);
		if( timer == TIM2 )
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);
		if( timer == TIM3 )
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 , ENABLE);
		if( timer == TIM4 )
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4 , ENABLE);

		enableRCC(pin);
		pinMode(pin, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);

		TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
		TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
		TIM_TimeBaseInit(timer, &TIM_TimeBaseStructure);

		TIM_Cmd(timer, ENABLE);
	}
}

void pwmSetEnabled(STM32Pin pin, bool enabled)
{
	int channel = -1;
	TIM_TypeDef * timer = 0;

	pwmTimerAndChannelForPin(pin, &timer, &channel);

	if( timer != 0 )
		TIM_Cmd(timer, enabled ? ENABLE : DISABLE);
}

void pwmSetFrequency(STM32Pin pin, uint32_t frequency)
{
	int channel = -1;
	TIM_TypeDef * timer = 0;

	pwmTimerAndChannelForPin(pin, &timer, &channel);
	if( timer != 0 && frequency > 0 )
	{
		uint32_t period = SystemCoreClock / frequency - 1;
		uint32_t prescaler = 0;

		while( period >= 0x10000 )
		{
			period = (period+1) / 2;
			prescaler = prescaler*2 + 1;
		}

		TIM_PrescalerConfig(timer, prescaler, TIM_PSCReloadMode_Update);
		TIM_SetAutoreload(timer, period);
	}
}

void pwmSetDuty(STM32Pin pin, unsigned percent)
{
	int channel = -1;
	TIM_TypeDef * timer = 0;

	pwmTimerAndChannelForPin(pin, &timer, &channel);
	if( timer != 0 )
	{
		unsigned arr = timer->ARR;
		unsigned pulse = arr * percent / 100;

		TIM_OCInitTypeDef  TIM_OCInitStructure;
		TIM_OCStructInit(&TIM_OCInitStructure);

		TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
		TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
		TIM_OCInitStructure.TIM_Pulse = pulse;


		switch(channel)
		{
		case 1:
			TIM_OC1Init(timer, &TIM_OCInitStructure);
			TIM_OC1PreloadConfig(timer, TIM_OCPreload_Enable);
			break;
		case 2:
			TIM_OC2Init(timer, &TIM_OCInitStructure);
			TIM_OC2PreloadConfig(timer, TIM_OCPreload_Enable);
			break;
		case 3:
			TIM_OC3Init(timer, &TIM_OCInitStructure);
			TIM_OC3PreloadConfig(timer, TIM_OCPreload_Enable);
			break;
		case 4:
			TIM_OC4Init(timer, &TIM_OCInitStructure);
			TIM_OC4PreloadConfig(timer, TIM_OCPreload_Enable);
			break;
		}
	}
}

void pwmTimerAndChannelForPin(STM32Pin pin, TIM_TypeDef ** tim, int * channel)
{
	switch( pin )
	{
	case PA8:
	case PA9:
	case PA10:
	case PA11:
		*tim = TIM1;
		*channel = pin - PA8 + 1;
		break;
	case PB6:
	case PB7:
	case PB8:
	case PB9:
		*tim = TIM4;
		*channel = pin - PB6 + 1;
		break;
	case PA6:
	case PA7:
		*tim = TIM3;
		*channel = pin - PA6 + 1;
		break;
	case PB0:
	case PB1:
		*tim = TIM3;
		*channel = pin - PB0 + 3;
		break;
	case PA0:
	case PA1:
	case PA2:
	case PA3:
		*tim = TIM2;
		*channel = pin - PA0 + 1;
		break;
	default:
		*channel = -1;
		*tim = 0;
		break;
	}
}
