#ifndef ADC_H_
#define ADC_H_

#include "stm32f10x.h"

bool isCalibrated( ADC_TypeDef * ADC );
bool calibrateADC( ADC_TypeDef * ADC );

#endif /* ADC_H_ */
