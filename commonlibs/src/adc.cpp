#include "adc.h"

bool calibrated[3] = {false, false, false};

bool calibrateADC( ADC_TypeDef * ADC )
{
	int adc = 0;
	if( ADC == ADC1 )
		adc = 0;
	else if( ADC == ADC2 )
		adc = 1;
	if( ADC == ADC3 )
		adc = 2;

	if(!calibrated[adc])
	{
		//Enable ADC reset calibration register
		ADC_ResetCalibration(ADC);
		//Check the end of ADC reset calibration register
		while(ADC_GetResetCalibrationStatus(ADC));
		//Start ADC calibration
		ADC_StartCalibration(ADC);
		//Check the end of ADC calibration
		while(ADC_GetCalibrationStatus(ADC));

		calibrated[adc] = true;

		return true;
	}
	return false;
}

bool isCalibrated( ADC_TypeDef * ADC )
{
	int adc = 0;
	if( ADC == ADC1 )
		adc = 0;
	else if( ADC == ADC2 )
		adc = 1;
	if( ADC == ADC3 )
		adc = 2;

	return calibrated[adc];
}
