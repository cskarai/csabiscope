#ifndef PWM_H_
#define PWM_H_

#include "pins.h"
#include "stm32f10x.h"

void pwmInit(STM32Pin pin);
void pwmSetFrequency(STM32Pin pin, uint32_t frequency);
void pwmSetDuty(STM32Pin pin, unsigned percent);
void pwmSetEnabled(STM32Pin pin, bool enabled);

void pwmTimerAndChannelForPin(STM32Pin pin, TIM_TypeDef ** tim, int * channel);

#endif /* PWM_H_ */
