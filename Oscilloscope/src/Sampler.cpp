#include <adc.h>
#include <Config.h>
#include <misc.h>
#include <stdint-gcc.h>
#include <stm32f10x.h>
#include <stm32f10x_adc.h>
#include <stm32f10x_dma.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_tim.h>
#include <systick/SysTime.h>
#include <Sampler.h>
#include <task/TaskHandler.h>
#include <TriggerHandler.h>

#define LCD_INTERVAL_SIZE 10

/* ADC ADON mask */
#define CR2_ADON_Set                ((uint32_t)0x00000001)
#define CR2_ADON_Reset              ((uint32_t)0xFFFFFFFE)
#define CR2_CONTINUOUS_Set          ((uint32_t)0x00000002)
#define CR2_CONTINUOUS_Reset        ((uint32_t)0xFFFFFFFD)

Sampler * Sampler::instance = 0;

volatile uint32_t cyclesToCollect = 0xFFFFFFFF;

void  (*sampler_irq)(int complete);

extern "C"
{

void DMA1_Channel1_IRQHandler()
{
	uint32_t isr = DMA1->ISR;
	if( isr & (DMA1_IT_TC1 | DMA1_IT_HT1) )
	{
		if( cyclesToCollect == 1 )
		{
			ADC1->CR2 &= CR2_ADON_Reset;
			ADC2->CR2 &= CR2_ADON_Reset;
		}

		DMA_ClearITPendingBit(DMA1_IT_GL1 | DMA1_IT_TC1 | DMA1_IT_HT1);

		(*sampler_irq)( isr & DMA1_IT_TC1 );
	}
}

}

void setADCSequenceLength(ADC_TypeDef* ADCx, int len)
{
	ADCx->SQR1 &= ~(0xF << 20);
	ADCx->SQR1 |= (len-1) << 20;
}

Sampler::Sampler(unsigned lcdResourcesIn, MeasurementData & data) : lcdResources(lcdResourcesIn), measurementData(data)
{
	instance = this;
}

void Sampler::init(RefreshCallback rf)
{
	refreshCallback=rf;

	enableRCC(PIN_JOYX);
	enableRCC(PIN_JOYY);
	enableRCC(PIN_VREF);
	enableRCC(PIN_CHANNEL_1);
	enableRCC(PIN_CHANNEL_2);

	pinMode(PIN_JOYX, GPIO_Mode_AIN);
	pinMode(PIN_JOYY, GPIO_Mode_AIN);
	pinMode(PIN_VREF, GPIO_Mode_AIN, GPIO_Speed_50MHz);
	pinMode(PIN_CHANNEL_1, GPIO_Mode_AIN, GPIO_Speed_50MHz);
	pinMode(PIN_CHANNEL_2, GPIO_Mode_AIN, GPIO_Speed_50MHz);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 , ENABLE);

	DMA_InitTypeDef     DMA_InitStructure;

	DMA_DeInit(DMA1_Channel1);

	// Configure DMA for ADC1
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&ADC1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)sampleBuffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	DMA_InitStructure.DMA_BufferSize = SAMPLE_BUFFER_SIZE;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_Init(DMA1_Channel1, &DMA_InitStructure);

	/* Enable DMA Channel1 */
	DMA_Cmd(DMA1_Channel1, ENABLE);

	// Enable ADC1 for sampling

	// Configure ADC1
	ADC_DeInit(ADC1);
	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode = ADC_Mode_RegInjecSimult;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_ExternalTrigConvCmd(ADC1, DISABLE);

	// ADC1 regular channels configuration
	ADC_RegularChannelConfig(ADC1, getADCChannel(PIN_VREF), 1, ADC_SampleTime_1Cycles5);

	// Enable ADC1 DMA
	ADC_DMACmd(ADC1, ENABLE);

	// Configure ADC2
	ADC_DeInit(ADC2);
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_ExternalTrigConvCmd(ADC2, ENABLE);

	// ADC2 regular channels configuration
	ADC_RegularChannelConfig(ADC2, getADCChannel(PIN_VREF), 1, ADC_SampleTime_1Cycles5);

	// injected channel for joystick
	ADC_InjectedSequencerLengthConfig(ADC1, 1);
	ADC_InjectedSequencerLengthConfig(ADC2, 1);
	ADC_InjectedChannelConfig( ADC1, getADCChannel(PIN_JOYX), 1, ADC_SampleTime_1Cycles5 );
	ADC_InjectedChannelConfig( ADC2, getADCChannel(PIN_JOYY), 1, ADC_SampleTime_1Cycles5 );
	ADC_ExternalTrigInjectedConvConfig( ADC1, ADC_ExternalTrigInjecConv_None );
	ADC_ExternalTrigInjectedConvConfig( ADC2, ADC_ExternalTrigInjecConv_None );
	ADC_ExternalTrigInjectedConvCmd( ADC1, ENABLE );
	ADC_ExternalTrigInjectedConvCmd( ADC2, ENABLE );

	ADC_TempSensorVrefintCmd(ENABLE);
	ADC_Cmd(ADC1, ENABLE);
	calibrateADC(ADC1);

	ADC_Cmd(ADC2, ENABLE);
	calibrateADC(ADC2);

	ADC_Cmd(ADC1, DISABLE);
	ADC_Cmd(ADC2, DISABLE);

	setSamplerPriority(0,0);

	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA1_Channel1, DMA_IT_HT, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	triggerHandler.init([](int cycles) {
		cyclesToCollect = cycles;
		instance->sampleState = ST_SAMPLE_LINES;
	});
}

void Sampler::setSamplerPriority(int prio, int subPrio)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = prio;//Preemption Priority
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = subPrio; //Sub Priority
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
	NVIC_Init(&NVIC_InitStructure);
}

void Sampler::nextState()
{
	sampler_irq = &Sampler::handleIrq;

	switch(sampleState)
	{
	case ST_SAMPLE_VREF_AND_VCC:
		measurementData.reset();
		sampleState = ST_COLLECT_DATA;
		break;
	case ST_COLLECT_DATA:
		{
			if( timeExpired ) {
				timeExpired = false;

				if(config.getTriggerType() & TRIGGER_SINGLESHOT )
				{
					if( !singleShotTriggerEnable )
					{
						sampleState = ST_SAMPLE_VREF_AND_VCC;
						break;
					}
				}
				sampleState = ST_TRIGGER_WAIT;
			}
		}
		break;
	case ST_TRIGGER_WAIT:
		sampleState = ST_SAMPLE_LINES;
		break;
	case ST_SAMPLE_LINES:
	case ST_DATA_TRANSMIT:
	default:
		sampleState = ST_SAMPLE_VREF_AND_VCC;
		break;
	}
	setupState();
}

void Sampler::setupState()
{
	triggerHandler.destroyTrigger();
	bool continuous = samplingMethod == SM_CONTINUOUS;

	switch( sampleState )
	{
	case ST_SAMPLE_VREF_AND_VCC:
		if( measurementData.getSamplingInterval() != config.getSamplingInterval() )
		{
			setSamplingTimeMicro(config.getSamplingInterval());
			measurementData.setSamplingInterval( config.getSamplingInterval() );
		}
		// ADC1+ADC2 regular channels configuration
		setADCSequenceLength(ADC1, 2);
		setADCSequenceLength(ADC2, 2);
		ADC_RegularChannelConfig(ADC1, getADCChannel(PIN_VREF), 1, ADC_SampleTime_239Cycles5);
		ADC_RegularChannelConfig(ADC2, getADCChannel(PIN_VREF), 1, ADC_SampleTime_239Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 2, ADC_SampleTime_239Cycles5);
		ADC_RegularChannelConfig(ADC2, getADCChannel(PIN_VREF), 2, ADC_SampleTime_239Cycles5);
		continuous = true;

		cyclesToCollect = 2;
		break;
	case ST_DATA_TRANSMIT:
		setSamplingTimeMicro(config.getTransmitSamplingInterval());
		continuous = samplingMethod == SM_CONTINUOUS;
		measurementData.setSamplingInterval( config.getTransmitSamplingInterval() );

		// ADC1+ADC2 regular channels configuration
		setADCSequenceLength(ADC1, 1);
		setADCSequenceLength(ADC2, 1);
		ADC_RegularChannelConfig(ADC1, getADCChannel(PIN_CHANNEL_1), 1, ADC_SampleTime_1Cycles5);
		ADC_RegularChannelConfig(ADC2, getADCChannel(PIN_CHANNEL_2), 1, ADC_SampleTime_1Cycles5);

		cyclesToCollect = 0xFFFFFFFF;
		break;
	case ST_COLLECT_DATA:
		// ADC1+ADC2 regular channels configuration
		setADCSequenceLength(ADC1, 1);
		setADCSequenceLength(ADC2, 1);
		ADC_RegularChannelConfig(ADC1, getADCChannel(PIN_CHANNEL_1), 1, ADC_SampleTime_1Cycles5);
		ADC_RegularChannelConfig(ADC2, getADCChannel(PIN_CHANNEL_2), 1, ADC_SampleTime_1Cycles5);

		cyclesToCollect = 2;
		break;
	case ST_TRIGGER_WAIT:
		// ADC1+ADC2 regular channels configuration
		setADCSequenceLength(ADC1, 1);
		setADCSequenceLength(ADC2, 1);
		ADC_RegularChannelConfig(ADC1, getADCChannel(PIN_CHANNEL_1), 1, ADC_SampleTime_1Cycles5);
		ADC_RegularChannelConfig(ADC2, getADCChannel(PIN_CHANNEL_2), 1, ADC_SampleTime_1Cycles5);

		cyclesToCollect = 0xFFFFFFFF;

		triggerHandler.setupTrigger();
		break;
	case ST_SAMPLE_LINES:
	default:
		break;
	}

	DMA1_Channel1->CNDTR = SAMPLE_BUFFER_SIZE;
	DMA1_Channel1->CMAR = (u32)sampleBuffer;

	if( continuous ) {
		ADC1->CR2 |= CR2_CONTINUOUS_Set;
		ADC2->CR2 |= CR2_CONTINUOUS_Set;
	} else {
		ADC1->CR2 &= CR2_CONTINUOUS_Reset;
		ADC2->CR2 &= CR2_CONTINUOUS_Reset;
	}

	sampleDone = false;

	DMA_Cmd(DMA1_Channel1, ENABLE);

	/* Make sure, that ADC is not running */
	ADC1->CR2 &= CR2_ADON_Reset;
	ADC2->CR2 &= CR2_ADON_Reset;

	/* Enable ADC conversion */
	ADC2->CR2 |= CR2_ADON_Set;
	ADC1->CR2 |= CR2_ADON_Set;

	if( continuous )
	{
		/* Start ADC1 Conversion */
		ADC2->CR2 |= CR2_ADON_Set;
		ADC1->CR2 |= CR2_ADON_Set;
	}
	else
	{
		ADC_ExternalTrigConvCmd(ADC1, ENABLE);
		TIM1->CNT = 0;
		TIM_Cmd(TIM1, ENABLE);
	}
}

void Sampler::processResults()
{
	lcdWait = false;
	switch( sampleState )
	{
	case ST_SAMPLE_VREF_AND_VCC:
	case ST_COLLECT_DATA:
	case ST_SAMPLE_LINES:
		{
			switch( sampleState )
			{
			case ST_SAMPLE_VREF_AND_VCC:
				{
					measurementData.setVrefAndVccBuffer(sampleBuffer+CIRCULAR_RESERVE, SAMPLE_NUMBER & 0xFE);
				}
				break;
			case ST_COLLECT_DATA:
				{
					measurementData.setCollectedData(sampleBuffer+CIRCULAR_RESERVE, SAMPLE_NUMBER);

					unsigned time = (unsigned)SysTime::getMillis();
					if( time - lastTimeStamp > config.getOscilloscopeUpdateFrequency() )
					{
						lastTimeStamp = time;
						timeExpired = true;

						if( refreshCallback )
							refreshCallback(REFRESH_MEASUREMENT_VALUE);

						if( lastSingleShotEnabled != singleShotTriggerEnable )
						{
							lastSingleShotEnabled = singleShotTriggerEnable;
							if( refreshCallback )
								refreshCallback(REFRESH_TRIGGER);
						}

						lcdWait = true;
					}
				}
				break;
			case ST_SAMPLE_LINES:
				{
					triggerHandler.reorderSampleBuffer(sampleBuffer, SAMPLE_BUFFER_SIZE, CIRCULAR_RESERVE);

					if( ( config.getTriggerType() & TRIGGER_TYPE_MASK ) == TRIGGER_NONE_FFT )
						measurementData.setFFTBuffer(sampleBuffer+CIRCULAR_RESERVE, SAMPLE_BUFFER_SIZE - CIRCULAR_RESERVE);
					else
						measurementData.setSampleBuffer(sampleBuffer+CIRCULAR_RESERVE, SAMPLE_NUMBER);

					if( refreshCallback )
						refreshCallback(REFRESH_OSCILLOSCOPE);
					lcdWait = true;

					singleShotTriggerEnable = !(config.getTriggerType() & TRIGGER_SINGLESHOT );
				}
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}
}

void Sampler::handleIrq(int /* isCompleted */)
{
	if( ! --cyclesToCollect )
	{
		/* Stop ADC1 Software Conversion */
		DMA_Cmd(DMA1_Channel1, DISABLE);
		ADC_ExternalTrigConvCmd(ADC1, DISABLE);
		TIM_Cmd(TIM1, DISABLE);

		ADC1->CR2 &= CR2_ADON_Reset;
		ADC2->CR2 &= CR2_ADON_Reset;

		Sampler::getInstance()->setSampleDone();
	}
}

bool Sampler::readJoy(unsigned * joyx, unsigned *joyy)
{
	if(( sampleState == ST_SAMPLE_LINES ) || ( sampleState == ST_DATA_TRANSMIT ) )
		return false;

	ADC_SoftwareStartInjectedConvCmd( ADC1, ENABLE );
	while(ADC_GetSoftwareStartInjectedConvCmdStatus(ADC1) != RESET);

	*joyx = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
	*joyy = ADC_GetInjectedConversionValue(ADC2, ADC_InjectedChannel_1);
	return true;
}

void Sampler::loop()
{
	if( sampleDone )
	{
		if( !lcdWait )
		{
			processResults();
			if( ! lcdWait )
				nextState();
		}
		else
		{
			if( TaskHandler::getInstance().getTasksToProcess(lcdResources) == 0 )
			{
				nextState();
				lcdWait = false;
			}
		}
	}

	if( sampleState == ST_TRIGGER_WAIT )
		triggerHandler.loop();
}

void Sampler::setSamplingTimeMicro(unsigned micro)
{
	if( ( micro == 12 ) || ( micro == 8 ) )
		samplingMethod = SM_CONTINUOUS;
	else
		samplingMethod = SM_TIMER;

	if( micro < 12 )
		RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	else
		RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq (&RCC_Clocks);

	unsigned interval = RCC_Clocks.PCLK2_Frequency / 1000000 * micro / LCD_INTERVAL_SIZE;
	unsigned prescaler = 0;
	while( interval > 0xFFFF )
	{
		interval /= 2;
		prescaler = 2*prescaler + 1;
	}

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* Time Base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = interval - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM1, ENABLE);

	/* TIM1 counter disable */
	TIM_Cmd(TIM1, DISABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
}


const unsigned samplingRates[] = {
		8,      10,   12,   13,   15,   18,   20,   22,   25,   26,   28,
		30,     35,   40,   45,   50,   52,   57,   63,   75,   87,  100,
		104,   113,  125,  150,  175,  200,  208,  227,  250,  300,  350,
		400,   454,  500,  625,  800,  907, 1000, 1250, 1500, 1750, 2000,
		2500, 3000, 3500, 4000, 5000, 6250,    0,
};

unsigned Sampler::getSamplingRateIndex(unsigned rate)
{
	int i=0;
	do
	{
		unsigned crate = samplingRates[i];

		if( crate == 0 )
			return i-1;
		if( crate >= rate )
			return i;
		i++;
	}while(1);
}

unsigned Sampler::getNextSamplingRate(unsigned currentRate)
{
	unsigned ndx = getSamplingRateIndex(currentRate);
	ndx++;

	unsigned rate = samplingRates[ndx];
	if( rate == 0 )
		return currentRate;

	return rate;
}

unsigned Sampler::getPreviousSamplingRate(unsigned currentRate)
{
	unsigned ndx = getSamplingRateIndex(currentRate);
	if( ndx == 0 )
		return currentRate;
	ndx--;

	return samplingRates[ndx];
}

void Sampler::increaseSamplingRate()
{
	config.setSamplingInterval(getNextSamplingRate(config.getSamplingInterval()));
}

void Sampler::decreaseSamplingRate()
{
	config.setSamplingInterval(getPreviousSamplingRate(config.getSamplingInterval()));
}

void Sampler::restart()
{
	setSamplerPriority(0,0);
	triggerHandler.destroyTrigger();

	ADC1->CR2 &= CR2_ADON_Reset;
	ADC2->CR2 &= CR2_ADON_Reset;

	DMA_Cmd(DMA1_Channel1, DISABLE);

	sampleState = ST_NONE;
	nextState();
}

void Sampler::startTransmitter( void  (*transmitter_cb)(int complete) )
{
	/* Stop ADC1 Software Conversion */
	DMA_Cmd(DMA1_Channel1, DISABLE);
	ADC_ExternalTrigConvCmd(ADC1, DISABLE);
	TIM_Cmd(TIM1, DISABLE);

	sampler_irq = transmitter_cb;
	sampleState = ST_DATA_TRANSMIT;

	setupState();
}
