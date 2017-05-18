#include "MeasurementData.h"
#include "xprintf.h"
#include "isqrt.h"
#include "Config.h"
#include "GainSetterTask.h"
#include <string.h>
#include "fix_fft.h"

#define MAX_LCD_HEIGHT   101

#define FREQUENCY_MIN_DISTANCE 100
#define FREQUENCY_PERCENT       10

#define FFT_N              8
#define FFT_SIZE ( 1 << FFT_N)

const char * scaleToString(ValueScale scale)
{
	switch(scale)
	{
	case ValueScale::NANO:
		return "n";
	case ValueScale::MICRO:
		return "µ";
	case ValueScale::MILLI:
		return "m";
	case ValueScale::NONE:
		return "";
	case ValueScale::KILO:
		return "k";
	case ValueScale::MEGA:
		return "M";
	case ValueScale::GIGA:
		return "G";

	default:
		return "";
	}
}

const char * unitToString(ValueUnit unit)
{
	switch(unit)
	{
	case ValueUnit::VOLTAGE:
		return "V";
	case ValueUnit::TIME:
		return "s";
	case ValueUnit::FREQUENCY:
		return "Hz";
	case ValueUnit::BYTE:
		return "Byte";
	case ValueUnit::BYTE_PER_SEC:
		return "Byte/s";
	case ValueUnit::PERCENT:
		return "%";
	case ValueUnit::INVALID:
	case ValueUnit::NONE:
		return "";
	default:
		return "";
	}
}

char buf[20];

int digits(int value)
{
	if( value < 0)
		value = -value;

	if( value >= 256000 - 1280 )    // >1000 - subtract rounding constant
		return 3;
	else if( value >= 25600 - 128 ) // >100
		return 2;
	else if( value >= 2560 - 13 )   // >10
		return 1;
	else if( value >= 256 - 1 )     // >1
		return 0;
	else if( value >= 25 )          // >0.1
		return -1;
	else if( value >= 2 )           // >0.01
		return -2;
	return -2;
}

int roundValue(int value, int digits)
{
	bool negative = false;

	if( value < 0 ) {
		value = -value;
		negative = true;
	}

	switch(digits)
	{
	case 3:
	case 2:
	case 1:
		value = ((value + 1280) / 2560 ) * 2560; // round
		break;
	case 0:
		value = ((value + 128) / 256 ) * 256; // round
		break;
	case -1:
		value = ((( value * 10 + 128 ) / 256 ) * 256 + 5 ) / 10; // round
		break;
	case -2:
		value = ((( value * 100 + 128 ) / 256 ) * 256 + 50) / 100; // round
		break;
	}

	if( negative )
		value = -value;

	return value;
}

int roundValueUp(int value, int digits)
{
	bool negative = false;

	if( value < 0 ) {
		value = -value;
		negative = true;
	}

	switch(digits)
	{
	case 3:
	case 2:
	case 1:
		value = ((value + 2560-1) / 2560 ) * 2560; // round
		break;
	case 0:
		value = ((value + 256-1) / 256 ) * 256; // round
		break;
	case -1:
		value = ((( value * 10 + 256-1 ) / 256 ) * 256 + 5 ) / 10; // round
		break;
	case -2:
		value = ((( value * 100 + 256-1 ) / 256 ) * 256 + 50) / 100; // round
		break;
	}

	if( negative )
		value = -value;

	return value;
}


Value scaleValue(int value, ValueScale lowestScale, ValueUnit unit)
{
	int sign = 1;
	if( value < 0 )
	{
		value = -value;
		sign = -1;
	}
	if( value >= 256000000 )
	{
		value /= 1000000;
		return Value(sign*value, (ValueScale)((int)lowestScale + 2), unit );
	}
	else if( value >= 256000 )
	{
		value /= 1000;
		return Value(sign*value, (ValueScale)((int)lowestScale + 1), unit );
	}
	return Value(sign*value, lowestScale, unit );
}

Value scaleIntValue(int value, ValueScale lowestScale, ValueUnit unit)
{
	int sign = 1;
	if( value < 0 )
	{
		value = -value;
		sign = -1;
	}
	if( value >= 1000000000 )
	{
		value = ((value / 1000000) * 256) / 1000;
		return Value(sign*value, (ValueScale)((int)lowestScale + 3), unit );
	}
	if( value >= 1000000 )
	{
		value = ((value / 1000) * 256) / 1000;
		return Value(sign*value, (ValueScale)((int)lowestScale + 2), unit );
	}
	else if( value >= 1000 )
	{
		value = value * 256 / 1000;
		return Value(sign*value, (ValueScale)((int)lowestScale + 1), unit );
	}
	return Value(sign*value*256, lowestScale, unit );
}

const char * valueToString(Value v, bool preferInt)
{
	char * ptr = buf;

	if( v.unit == ValueUnit::INVALID )
	{
		buf[0] = 0;
		return buf;
	}

	int dt = v.data;
	if( dt < 0 )
	{
		dt = -dt;
		*ptr++ = '-';
	}

	// rounding here
	dt = roundValue(dt, digits(dt)-2); // 2 digit value

	int intpart = dt >> 8;
	ptr += xsprintf(ptr, "%d", intpart);

	if( intpart < 10 )
	{
		int fracpart = ((dt & 255) * 100 + 128) >> 8;
		if( !preferInt || fracpart != 0 )
			ptr += xsprintf(ptr, ".%02d", fracpart);
	}
	else if( intpart < 100 )
	{
		int fracpart = ((dt & 255) * 10 + 128) >> 8;
		if( !preferInt || fracpart != 0 )
			ptr += xsprintf(ptr, ".%01d", fracpart);
	}

	if(strlen(unitToString(v.unit)) > 3)
		ptr += xsprintf(ptr, " ");

	ptr += xsprintf(ptr, "%s%s", scaleToString(v.scale), unitToString(v.unit));
	return buf;
}

MeasurementData::MeasurementData() : gainValue(GAIN_1X), internalReferenceValue(0)
{
}

void MeasurementData::setSampleBuffer(uint32_t * sampleBuffer, unsigned maxSamples)
{
	int min = 0;
	int max = 0xFFF;
	bool multiChannel = config.getNumberOfChannels() > 1;

	if( config.isAutoZoom() )
	{
		max = 0;
		min = 0xFFFF;

		for(unsigned cnt=0; cnt < maxSamples; cnt++ )
		{
			int ch1 = (int)(sampleBuffer[cnt] & 0xFFFF);

			if( ch1 < min )
				min = ch1;
			if( ch1 > max )
				max = ch1;
			if( multiChannel )
			{
				int ch2 = (int)(sampleBuffer[cnt] >> 16);
				if( ch2 < min )
					min = ch2;
				if( ch2 > max )
					max = ch2;
			}
		}

		int diff;
		while(( diff = max - min) < MAX_LCD_HEIGHT )
		{
			if( diff & 1 )
				max++;
			else
				min--;
		}

		if(min < 0 )
		{
			max -= min;
			min = 0;
		}
		if(max > 0xFFF )
		{
			min -= (max-0xFFF);
			max = 0xFFF;
		}
	}

	scopeMin = adcToVoltage(min, 0);
	scopeMax = adcToVoltage(max, 0);

	Value scopeMinRound = scopeMin;
	scopeMinRound.data = roundValueUp( scopeMinRound.data, digits(scopeMinRound.data) - 1 );
	Value scopeMaxRound = scopeMax;
	scopeMaxRound.data = roundValueUp( scopeMaxRound.data, digits(scopeMaxRound.data) - 1 );

	scopeMin = scopeMinRound;
	scopeMax = scopeMaxRound;
	min = voltageToAdc(scopeMin, 0);
	max = voltageToAdc(scopeMax, 0);

	int diff = max-min;
	scopeScale = adcToVoltage(((diff+5)/10)+offset[0], 0);

	auto adcToLCD = [] (unsigned value, int min, int max) -> unsigned {
		int diff = max - min;
		return MAX_LCD_HEIGHT - 1 - (((value - min) * (MAX_LCD_HEIGHT - 1) + diff/2) / diff);
	};

	for(unsigned cnt=0; cnt < maxSamples; cnt++ )
	{
		uint8_t ch1 = adcToLCD( sampleBuffer[cnt] & 0xFFFF, min, max);
		uint8_t ch2 = adcToLCD( sampleBuffer[cnt] >> 16, min, max);
		channelData[0][cnt] = ch1;
		channelData[1][cnt] = ch2;
	}

	int offs_avg = (offset[0] + offset[1]) / 2;
	if( offs_avg >= min && offs_avg <= max )
		setZeroLine( adcToLCD(offs_avg,  min, max) );
	else
		setZeroLine( INVALID_VALUE );

	determineFrequency(sampleBuffer, maxSamples);
}

void MeasurementData::setFFTBuffer(uint32_t * sampleBuffer, unsigned maxSamples)
{
	fixed * fft_real = new fixed [FFT_SIZE];
	fixed * fft_imag = new fixed [FFT_SIZE];

	unsigned max = 0xFFF - offset[0];
	unsigned max2 = 0xFFF - offset[1];
	if( max2 > max )
		max = max2;
	if( offset[0] > max )
		max = offset[0];
	if( offset[1] > max )
		max = offset[1];

	unsigned magnify = config.getFFTMagnification();
	int pass = (magnify == FFT_MAGNIFY_AUTO) ? 1 : 2;
	int maxVal[2] = {0, 0};

	for(; pass < 3; pass++)
	{
		for(unsigned ch=0; ch < config.getNumberOfChannels(); ch++)
		{
			for(int i=0; i < FFT_SIZE; i++)
			{
				if(ch == 0)
					fft_real[i] = (fixed)((sampleBuffer[i] & 0xFFFF) - offset[0]);
				else
					fft_real[i] = (fixed)((sampleBuffer[i] >> 16) - offset[1]);

				fft_imag[i] = 0;
			}

			fix_fft(fft_real, fft_imag, FFT_N, 0);

			for(int i=0; i < FFT_SIZE/2; i++) {
				fft_real[i] = isqrt(fft_real[i]*fft_real[i] + fft_imag[i]*fft_imag[i]);
				if( pass == 1 )
				{
					if(fft_real[i] > maxVal[ch])
						maxVal[ch] = fft_real[i];
				}
				else
				{
					int val = fft_real[i] * (MAX_LCD_HEIGHT-1) / max;
					val = val * magnify / 2;
					if( val > MAX_LCD_HEIGHT - 1 )
						val = MAX_LCD_HEIGHT - 1;
					channelData[ch][i] = MAX_LCD_HEIGHT - 1 - val;
				}
			}
		}

		if( pass == 1 )
		{
			int maxc = (maxVal[0] > maxVal[1]) ? maxVal[0] : maxVal[1];
			if( maxc == 0 )
				magnify = FFT_MAGNIFY_4X;
			else
			{
				int amnt = 2 * max / maxc;
				if( amnt < 2 )
					magnify = FFT_MAGNIFY_1X;
				else if( amnt > 8 )
					magnify = FFT_MAGNIFY_4X;
				else
					magnify = amnt;
			}
		}
	}

	delete fft_real;
	delete fft_imag;

	unsigned fftmax = 512*max / magnify;
	scopeMin=Value(0, ValueScale::NONE, ValueUnit::NONE);
	scopeMax=Value(fftmax, ValueScale::NONE, ValueUnit::NONE);
	scopeScale=Value((fftmax+5)/10, ValueScale::NONE, ValueUnit::NONE);

	setZeroLine( INVALID_VALUE );
	determineFrequency(sampleBuffer, SAMPLE_NUMBER);
}


void MeasurementData::reset()
{
	gainValue = getGainValue(currentGain());
	counter = 0;

	for(int i=0; i < MAX_CHANNELS; i++)
	{
		average[i] = 0;
		min[i] = 0xFFFFFF;
		max[i] = 0;
		eff[i] = 0;
	}
}

void MeasurementData::determineFrequency(uint32_t * sampleBuffer, unsigned maxSamples)
{
	enum FreqDetectState {
		SEARCH_MAX,
		SEARCH_MIN,
		SEARCH_MAX_OR_MIN,
	};

	frequency = Value();
	bool channel2 = (config.getNumberOfChannels() > 1) && (config.getFrequencyMeasuringChannel() > 0);

	unsigned min = 0xFFFF;
	unsigned max = 0;

	for(unsigned i=0; i < maxSamples; i++)
	{
		unsigned ch = channel2 ? sampleBuffer[i] >> 16 : sampleBuffer[i] & 0xFFFF;

		if( ch > max )
			max = ch;
		if( ch < min )
			min = ch;
	}

	unsigned diff = max - min;
	if( diff < FREQUENCY_MIN_DISTANCE )
		return;

	unsigned thrs = diff * FREQUENCY_PERCENT / 100;
	min += thrs;
	max -= thrs;

	int first = -1;
	int last = -1;
	int periods = 0;

	FreqDetectState state = SEARCH_MAX_OR_MIN;
	FreqDetectState firstState = SEARCH_MAX_OR_MIN;
	for(int i=0; i < (int)maxSamples; i++)
	{
		unsigned ch = channel2 ? sampleBuffer[i] >> 16 : sampleBuffer[i] & 0xFFFF;

		bool belowMin = ( ch <= min ) && (state != SEARCH_MAX);
		bool aboveMax = ( ch >= max ) && (state != SEARCH_MIN);

		if( !belowMin && !aboveMax )
			continue;

		if( firstState == SEARCH_MAX_OR_MIN)
		{
			if( i!=0 )
			{
				first = i;
				firstState = aboveMax ? SEARCH_MIN : SEARCH_MAX;
			}
		}
		else if( firstState != state )
		{
			last = i;
			periods++;
		}
		state = aboveMax ? SEARCH_MIN : SEARCH_MAX;
	}

	if( periods > 0 )
	{
		int periodLength = 100 * (last - first) / periods;
		int micros = getSamplingInterval();
		int totalTime = micros*periodLength / 10;
		int freq = (int)((uint64_t)25600000000 / (uint64_t)totalTime);

		frequency = scaleValue(freq, ValueScale::NONE, ValueUnit::FREQUENCY);
	}
}

void MeasurementData::setCollectedData(uint32_t * sampleBuffer, unsigned maxSamples)
{
	for(unsigned cnt=0; cnt < maxSamples; cnt++ )
	{
		unsigned ch1 = sampleBuffer[cnt] & 0xFFFF;
		unsigned ch2 = sampleBuffer[cnt] >> 16;

		average[0] += ch1;
		average[1] += ch2;

		if( ch1 > max[0] )
			max[0] = ch1;
		if( ch2 > max[1] )
			max[1] = ch2;

		if( ch1 < min[0] )
			min[0] = ch1;
		if( ch2 < min[1] )
			min[1] = ch2;

		int d1 = ch1 - offset[0];
		int d2 = ch2 - offset[1];

		eff[0] += d1*d1;
		eff[1] += d2*d2;

		counter++;
	}
}

void MeasurementData::setVrefAndVccBuffer(uint32_t * sampleBuffer, unsigned maxSamples)
{
	unsigned res_vref_0 = 0, res_vref_1 = 0, res_vcc = 0;
	unsigned sampleCnt = maxSamples / 2;

	for(unsigned cnt=0; cnt < maxSamples; cnt+=2 )
	{
		unsigned ch_vref_0 = sampleBuffer[cnt] & 0xFFFF;
		unsigned ch_vref_1 = sampleBuffer[cnt] >> 16;
		unsigned ch_vcc    = sampleBuffer[cnt+1] & 0xFFFF;
		res_vref_0 += ch_vref_0;
		res_vref_1 += ch_vref_1;
		res_vcc  += ch_vcc;
	}

	res_vref_0 = (res_vref_0 + sampleCnt/2) / sampleCnt;
	res_vref_1 = (res_vref_1 + sampleCnt/2) / sampleCnt;
	res_vcc = (res_vcc + sampleCnt/2) / sampleCnt;

	setOffset(0, res_vref_0);
	setOffset(1, res_vref_1);
	setInternalReferenceValue(res_vcc);

	unsigned err_vref_0 = 0, err_vref_1 = 0, err_vcc = 0;

	for(unsigned cnt=0; cnt < maxSamples; cnt+=2 )
	{
		unsigned ch_vref_0 = sampleBuffer[cnt] & 0xFFFF;
		unsigned ch_vref_1 = sampleBuffer[cnt] >> 16;
		unsigned ch_vcc    = sampleBuffer[cnt+1] & 0xFFFF;
		err_vref_0 += (ch_vref_0-res_vref_0) * (ch_vref_0-res_vref_0);
		err_vref_1 += (ch_vref_1-res_vref_1) * (ch_vref_1-res_vref_1);
		err_vcc += (ch_vcc-res_vcc) * (ch_vcc-res_vcc);
	}

	err_vref_0 = isqrt((err_vref_0 + sampleCnt/2) / sampleCnt);
	err_vref_1 = isqrt((err_vref_1 + sampleCnt/2) / sampleCnt);
	err_vcc = isqrt((err_vcc + sampleCnt/2) / sampleCnt);

	channelError[0] = Value(25600 * err_vref_0 / 0xFFF, ValueScale::NONE, ValueUnit::NONE);
	channelError[1] = Value(25600 * err_vref_1 / 0xFFF, ValueScale::NONE, ValueUnit::NONE);
	internalReferenceError = Value(25600 * err_vcc / 0xFFF, ValueScale::NONE, ValueUnit::NONE);
}

Value MeasurementData::convertValue(unsigned val, int divisor, int channel)
{
	unsigned v = (val + divisor/2) / divisor;
	return adcToVoltage(v, channel);
}

Value MeasurementData::getEff(int channel)
{
	unsigned effsqr = (unsigned)((eff[channel] + counter/2) / counter);
	unsigned eff = isqrt(effsqr);
	return adcToVoltage(eff + offset[channel], channel);
}

Value MeasurementData::adcToVoltage( unsigned adc, int channel)
{
	bool negate = false;

	int64_t dti = (int)adc - (int)offset[channel];
	if( dti < 0 )
	{
		dti = -dti;
		negate = true;
	}

	int calib = config.getCalibrationValue(channel) * config.getCalibrationReference() / internalReferenceValue * 256 / gainValue;
	int dt = (int)((dti * calib + 0x7FF) / 0xFFF);
	if( negate )
		dt = -dt;

	return scaleValue(dt, ValueScale::MILLI, ValueUnit::VOLTAGE);
}

unsigned MeasurementData::voltageToAdc( Value voltage, int channel)
{
	bool negate = false;

	int num = voltage.data;
	if( num < 0 )
	{
		num = -num;
		negate = true;
	}

	if( voltage.scale == ValueScale::NONE )
		num *= 1000;

	int calib = config.getCalibrationValue(channel) * config.getCalibrationReference() / internalReferenceValue * 256 / gainValue;
	int val = ( (int64_t)num * 0xFFF + calib/2) / calib;

	if( negate )
		val = -val;

	return (unsigned)(val + offset[channel]);
}
