#ifndef INCLUDE_PINS_H_
#define INCLUDE_PINS_H_

#include "stm32f10x.h"

#define PORT_A  0
#define PORT_B 16
#define PORT_C 32

typedef enum
{
	PA0  = PORT_A + 0,
	PA1  = PORT_A + 1,
	PA2  = PORT_A + 2,
	PA3  = PORT_A + 3,
	PA4  = PORT_A + 4,
	PA5  = PORT_A + 5,
	PA6  = PORT_A + 6,
	PA7  = PORT_A + 7,
	PA8  = PORT_A + 8,
	PA9  = PORT_A + 9,
	PA10 = PORT_A + 10,
	PA11 = PORT_A + 11,
	PA12 = PORT_A + 12,
	PA13 = PORT_A + 13,
	PA14 = PORT_A + 14,
	PA15 = PORT_A + 15,
	PB0  = PORT_B + 0,
	PB1  = PORT_B + 1,
	PB2  = PORT_B + 2,
	PB3  = PORT_B + 3,
	PB4  = PORT_B + 4,
	PB5  = PORT_B + 5,
	PB6  = PORT_B + 6,
	PB7  = PORT_B + 7,
	PB8  = PORT_B + 8,
	PB9  = PORT_B + 9,
	PB10 = PORT_B + 10,
	PB11 = PORT_B + 11,
	PB12 = PORT_B + 12,
	PB13 = PORT_B + 13,
	PB14 = PORT_B + 14,
	PB15 = PORT_B + 15,
	PC13 = PORT_C + 13,
	PC14 = PORT_C + 14,
	PC15 = PORT_C + 15,
} STM32Pin;

#define GPIO_PIN_NUMBER(a)  ( (a) & 0xF )
#define GPIO_PIN_MASK(a)    ( 1 << ( a ) )
#define GPIO_PORT(a)        ( (GPIO_TypeDef *)(GPIOA_BASE + (GPIOB_BASE-GPIOA_BASE)*((a) >> 4)) )
#define GPIO_RCC_MASK(a)    ( RCC_APB2Periph_GPIOA << ((a) >> 4) )

void pinMode(STM32Pin pin, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed = GPIO_Speed_2MHz);
void writePin(STM32Pin pin, bool value);
bool readPin(STM32Pin pin);

void enableRCC(STM32Pin pin);
int  getADCChannel(STM32Pin pin);

#endif /* INCLUDE_PINS_H_ */
