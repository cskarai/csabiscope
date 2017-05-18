#include "TriggerHandler.h"
#include "Sampler.h"
#include "stm32f10x.h"
#include "adc.h"
#include <string.h>

#define CHANGE_THRESHOLD                                     20

#define PIN_EXTERNAL_TRIGGER                                PB9
#define PIN_EXTERNAL_TRIGGER_PORT_SOURCE   GPIO_PortSourceGPIOB
#define PIN_EXTERNAL_TRIGGER_PIN_SOURCE         GPIO_PinSource9
#define PIN_EXTERNAL_TRIGGER_EXTI_LINE               EXTI_Line9

TriggerHandler triggerHandler;

extern "C"
{

void           (* irq_callback)();
void           (* irq_second_callback)();
ADC_TypeDef     * irq_ADC;
unsigned          irq_triggerThreshold;
volatile unsigned irq_dmaAddr = 0xFFFFFFFF;
TriggerCallback   irq_triggerCallback;

void callTrigger()
{
	if( irq_second_callback != 0 )
	{
		irq_second_callback();
		irq_second_callback = 0;
		return;
	}
	ADC_AnalogWatchdogCmd(irq_ADC, ADC_AnalogWatchdog_None);
	ADC_ITConfig(irq_ADC, ADC_IT_AWD, DISABLE);

	unsigned remaining = irq_dmaAddr;
	if( remaining > SAMPLE_BUFFER_SIZE/2 )
		remaining -= SAMPLE_BUFFER_SIZE/2;

	if(config.getTriggerOffset() + remaining > SAMPLE_NUMBER )
		irq_triggerCallback(1);
	else
		irq_triggerCallback(2);
}

void risingEdgeCallbackLoop()
{
	unsigned short adc_value = irq_ADC->DR;
	if( adc_value < irq_ADC->LTR )
	{
		irq_ADC->LTR = adc_value;
		irq_ADC->HTR = adc_value + irq_triggerThreshold;
	}
	else if( adc_value > irq_ADC->HTR )
	{
		irq_dmaAddr = DMA1_Channel1->CNDTR;
		callTrigger();
	}
	irq_ADC->SR = ~(uint32_t)(ADC_IT_AWD >> 8);
}

void risingEdgeCallback()
{
	unsigned short adc_value = irq_ADC->DR + irq_triggerThreshold;
	if( adc_value > 0xFFF )
		adc_value = 0xFFF;
	irq_ADC->HTR = adc_value;
	irq_ADC->LTR = adc_value - irq_triggerThreshold;

	irq_callback = risingEdgeCallbackLoop;
	irq_ADC->SR = ~(uint32_t)(ADC_IT_AWD >> 8);
}

void fallingEdgeCallbackLoop()
{
	unsigned short adc_value = irq_ADC->DR;
	if( adc_value > irq_ADC->HTR )
	{
		irq_ADC->LTR = adc_value - irq_triggerThreshold;
		irq_ADC->HTR = adc_value;
	}
	else if( adc_value < irq_ADC->LTR )
	{
		irq_dmaAddr = DMA1_Channel1->CNDTR;
		callTrigger();
	}
	irq_ADC->SR = ~(uint32_t)(ADC_IT_AWD >> 8);
}

void fallingEdgeCallback()
{
	short adc_value = irq_ADC->DR - irq_triggerThreshold;
	if( adc_value < 0 )
		adc_value = 0;
	irq_ADC->HTR = adc_value + irq_triggerThreshold;
	irq_ADC->LTR = adc_value;

	irq_callback = fallingEdgeCallbackLoop;
	irq_ADC->SR = ~(uint32_t)(ADC_IT_AWD >> 8);
}

void changingEdgeCallbackLoop()
{
	unsigned short adc_value = irq_ADC->DR;
	if( ( adc_value < irq_ADC->LTR ) || ( adc_value > irq_ADC->HTR ) )
	{
		irq_dmaAddr = DMA1_Channel1->CNDTR;
		callTrigger();
	}
	irq_ADC->SR = ~(uint32_t)(ADC_IT_AWD >> 8);
}

void changingEdgeCallback()
{
	short adc_value = irq_ADC->DR;
	short min = adc_value - irq_triggerThreshold;
	short max = adc_value + irq_triggerThreshold;
	if( min < 0 )
		min = 0;
	if( max > 0xFFF )
		max = 0xFFF;
	irq_ADC->HTR = max;
	irq_ADC->LTR = min;

	irq_callback = changingEdgeCallbackLoop;
	irq_ADC->SR = ~(uint32_t)(ADC_IT_AWD >> 8);
}

void ADC1_2_IRQHandler(void)
{
	// indirect jump to irq_callback
	asm volatile (
			"ldr  r3, [%0]  \n"
			"bx r3          \n"
			:
			: "r" (&irq_callback)
	);
}

void EXTI9_5_IRQHandler(void)
{
	irq_dmaAddr = DMA1_Channel1->CNDTR;

	EXTI->PR |= (1 << GPIO_PIN_NUMBER(PIN_EXTERNAL_TRIGGER));

	unsigned remaining = irq_dmaAddr;
	if( remaining > SAMPLE_BUFFER_SIZE/2 )
		remaining -= SAMPLE_BUFFER_SIZE/2;

	if(config.getTriggerOffset() + remaining > SAMPLE_NUMBER )
		irq_triggerCallback(1);
	else
		irq_triggerCallback(2);
}

}

extern volatile uint32_t cyclesToCollect;

void TriggerHandler::init(TriggerCallback callback)
{
	irq_triggerCallback = callback;

	// external trigger configuration

	enableRCC(PIN_EXTERNAL_TRIGGER);
	pinMode(PIN_EXTERNAL_TRIGGER, GPIO_Mode_IPU, GPIO_Speed_50MHz);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_EXTILineConfig(PIN_EXTERNAL_TRIGGER_PORT_SOURCE, PIN_EXTERNAL_TRIGGER_PIN_SOURCE);

	EXTI_InitTypeDef   EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = PIN_EXTERNAL_TRIGGER_EXTI_LINE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitTypeDef NVIC_InitStructure;
	/* Enable and set EXTIn Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;
	NVIC_Init(&NVIC_InitStructure);

	// ADC configuration

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//Preemption Priority
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //Sub Priority
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
	NVIC_Init(&NVIC_InitStructure);

}

void TriggerHandler::enableWatchDog()
{
	irq_triggerThreshold = 4095 * config.getTriggerPercent() / 100;

	int channel = config.getTriggerChannel();
	STM32Pin pin = (channel == 0) ? PIN_CHANNEL_1 : PIN_CHANNEL_2;
	irq_ADC = (channel == 0) ? ADC1 : ADC2;

	ADC_AnalogWatchdogCmd(irq_ADC, ADC_AnalogWatchdog_SingleRegEnable);
	ADC_AnalogWatchdogThresholdsConfig(irq_ADC, 0xFFF, 0xFFF);
	ADC_AnalogWatchdogSingleChannelConfig(irq_ADC, getADCChannel(pin));
	ADC_ITConfig(irq_ADC, ADC_IT_AWD, ENABLE);
}

void TriggerHandler::initializeTrigger()
{
	irq_dmaAddr = 0xFFFFFFFF;
	irq_second_callback = 0;

	switch( config.getTriggerType() & TRIGGER_TYPE_MASK )
	{
	case TRIGGER_NONE:
	case TRIGGER_NONE_FFT:
		// do nothing
		break;
	case TRIGGER_RISING_EDGE:
		irq_callback = risingEdgeCallback;
		enableWatchDog();
		break;
	case TRIGGER_FALLING_EDGE:
		irq_callback = fallingEdgeCallback;
		enableWatchDog();
		break;
	case TRIGGER_CHANGING_EDGE:
		irq_callback = changingEdgeCallback;
		enableWatchDog();
		break;
	case TRIGGER_POSITIVE_PEAK:
		irq_callback = risingEdgeCallback;
		irq_second_callback = fallingEdgeCallback;
		enableWatchDog();
		break;
	case TRIGGER_NEGATIVE_PEAK:
		irq_callback = fallingEdgeCallback;
		irq_second_callback = risingEdgeCallback;
		enableWatchDog();
		break;
	case TRIGGER_EXTERNAL:
		NVIC_EnableIRQ(EXTI9_5_IRQn);
		break;
	default:
		break;
	}
}

void TriggerHandler::loop()
{
	if( !inited && ( cyclesToCollect < 0xFFFFFFFF ) )
	{
		inited = true;
		initializeTrigger();
	}
}

void TriggerHandler::setupTrigger()
{
	irq_dmaAddr = 0xFFFFFFFF;
	unsigned trg = config.getTriggerType() & TRIGGER_TYPE_MASK;
	if( trg == TRIGGER_NONE || trg == TRIGGER_NONE_FFT )
	{
		irq_triggerCallback(trg == TRIGGER_NONE ? 1 : 2);
		inited = true;
	}
	else
		inited = false;
}

void TriggerHandler::destroyTrigger()
{
	ADC_AnalogWatchdogCmd(ADC1, ADC_AnalogWatchdog_None);
	ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);
	ADC_AnalogWatchdogCmd(ADC2, ADC_AnalogWatchdog_None);
	ADC_ITConfig(ADC2, ADC_IT_AWD, DISABLE);

	NVIC_DisableIRQ(EXTI9_5_IRQn);
}

void TriggerHandler::reorderSampleBuffer(uint32_t * buffer, int size, int circularReserve)
{
	if( irq_dmaAddr != 0xFFFFFFFF )
	{
		int loc = size - 1 - irq_dmaAddr;

		unsigned vals[SAMPLE_NUMBER];

		// configure the trigger offset
		loc = loc - config.getTriggerOffset();
		if( loc < 0 )
			loc += size;

		for( int i=0; i < SAMPLE_NUMBER; i++)
		{
			vals[i] = buffer[loc];
			if( ++loc == size )
				loc = 0;
		}

		memset(buffer, 0, size * sizeof(uint32_t) );

		for( int i=0; i < SAMPLE_NUMBER; i++ )
			buffer[i+circularReserve] = vals[i];
	}
}

const TriggerType triggerTypes[] = {
		TRIGGER_NONE,
		TRIGGER_RISING_EDGE,
		TRIGGER_FALLING_EDGE,
		TRIGGER_CHANGING_EDGE,
		TRIGGER_POSITIVE_PEAK,
		TRIGGER_NEGATIVE_PEAK,
		TRIGGER_EXTERNAL,
		TRIGGER_NONE_FFT,
		TRIGGER_SINGLESHOT_NONE,
		TRIGGER_SINGLESHOT_RISING_EDGE,
		TRIGGER_SINGLESHOT_FALLING_EDGE,
		TRIGGER_SINGLESHOT_CHANGING_EDGE,
		TRIGGER_SINGLESHOT_POSITIVE_PEAK,
		TRIGGER_SINGLESHOT_NEGATIVE_PEAK,
		TRIGGER_SINGLESHOT_EXTERNAL,
		TRIGGER_SINGLESHOT_NONE_FFT,
};

void TriggerHandler::jumpToPreviousTrigger()
{
	int ndx = 0;
	for( TriggerType t : triggerTypes )
	{
		if( t == config.getTriggerType() )
		{
			ndx--;
			if( ndx < 0 )
				ndx += sizeof(triggerTypes) / sizeof(TriggerType);
			config.setTriggerType(triggerTypes[ndx]);
			return;
		}
		ndx++;
	}
}

void TriggerHandler::jumpToNextTrigger()
{
	unsigned ndx = 0;
	for( TriggerType t : triggerTypes )
	{
		if( t == config.getTriggerType() )
		{
			ndx++;
			if( ndx >= sizeof(triggerTypes) / sizeof(TriggerType) )
				ndx = 0;
			config.setTriggerType(triggerTypes[ndx]);
			return;
		}
		ndx++;
	}
}
