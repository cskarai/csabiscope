#include "pins.h"

bool readPin(STM32Pin pin)
{
	uint32_t mask = GPIO_Pin_0 << GPIO_PIN_NUMBER(pin);

	return GPIO_ReadInputData(GPIO_PORT(pin)) & mask;
}

void writePin(STM32Pin pin, bool value)
{
	uint32_t mask = GPIO_Pin_0 << GPIO_PIN_NUMBER(pin);

	GPIO_WriteBit(GPIO_PORT(pin), mask, (BitAction)value);
}

void pinMode(STM32Pin pin, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed)
{
	GPIO_InitTypeDef GPIO_InitStructure; //Variable used to setup the GPIO pins
	GPIO_StructInit(&GPIO_InitStructure); // Reset init structure, if not it can cause issues...
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 << GPIO_PIN_NUMBER(pin);
	GPIO_InitStructure.GPIO_Mode = mode;
	GPIO_InitStructure.GPIO_Speed = speed;
	GPIO_Init(GPIO_PORT(pin), &GPIO_InitStructure);
}

void enableRCC(STM32Pin pin)
{
	RCC_APB2PeriphClockCmd(GPIO_RCC_MASK(pin), ENABLE);
}

int getADCChannel(STM32Pin pin)
{
	switch(pin)
	{
	case PA0:
	case PA1:
	case PA2:
	case PA3:
	case PA4:
	case PA5:
	case PA6:
	case PA7:
		return (pin & 7) + ADC_Channel_0;
	case PB0:
	case PB1:
		return (pin & 1) + ADC_Channel_8;
	default:
		return -1;
	}
}
