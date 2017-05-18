#ifndef MEASUREMENTDATA_H_
#define MEASUREMENTDATA_H_

#define SAMPLE_NUMBER  150
#define MAX_VALUE      100
#define MAX_CHANNELS   2

#define INVALID_VALUE  ((unsigned)(-1))

#include <inttypes.h>

enum class ValueScale
{
	NANO,
	MICRO,
	MILLI,
	NONE,
	KILO,
	MEGA,
	GIGA,
};

enum class ValueUnit
{
	VOLTAGE,
	TIME,
	NONE,
	FREQUENCY,
	PERCENT,
	BYTE,
	BYTE_PER_SEC,
	INVALID,
};

typedef struct Value
{
	int        data : 24;
	ValueScale scale : 4;
	ValueUnit  unit :  4;

public:
	Value(int dataIn, ValueScale scaleIn, ValueUnit unitIn) : data(dataIn), scale(scaleIn), unit(unitIn) {}
	Value() : data(0), scale(ValueScale::NONE), unit(ValueUnit::INVALID) {}
} Value;

int digits(int value);
int roundValue(int val, int digits);
int roundValueUp(int val, int digits);

const char * scaleToString(ValueScale unit);
const char * unitToString(ValueUnit unit);
const char * valueToString(Value v, bool preferInt=false);
Value        scaleValue(int value, ValueScale lowestScale, ValueUnit unit);
Value        scaleIntValue(int value, ValueScale lowestScale, ValueUnit unit);

class MeasurementData
{
private:
	uint8_t    channelData[MAX_CHANNELS][SAMPLE_NUMBER];
	unsigned   offset[MAX_CHANNELS];
	unsigned   zeroLine = 0xFFFFFFFF;
	unsigned   counter = 0;
	unsigned   average[MAX_CHANNELS];
	unsigned   min[MAX_CHANNELS];
	unsigned   max[MAX_CHANNELS];
	uint64_t   eff[MAX_CHANNELS];
	Value      channelError[MAX_CHANNELS];
	Value      frequency;
	unsigned   samplingInterval = 0;
	Value      scopeMin;
	Value      scopeMax;
	Value      scopeScale;
	int        gainValue;

	unsigned   internalReferenceValue;
	Value      internalReferenceError;

	Value      convertValue(unsigned val, int divisor, int channel);
	void       determineFrequency(uint32_t * buffer, unsigned maxSamples);

public:
	MeasurementData();

	uint8_t *  getChannelData(unsigned chan)          {return channelData[chan];}
	unsigned   getZeroLine() { return zeroLine; }
	void       setZeroLine( unsigned val ) { zeroLine = val; }
	unsigned   getSamplingInterval() { return samplingInterval; }
	void       setSamplingInterval( unsigned val ) { samplingInterval = val; }

	unsigned   getOffset(unsigned chan)               {return offset[chan];}
	void       setOffset(unsigned chan, unsigned val) {offset[chan] = val; }

	void       reset();
	void       setCollectedData(uint32_t * buffer, unsigned maxSamples);
	void       setSampleBuffer(uint32_t * buffer, unsigned maxSamples);
	void       setFFTBuffer(uint32_t * buffer, unsigned maxSamples);
	void       setVrefAndVccBuffer(uint32_t * buffer, unsigned maxSamples);

	Value      getAverage(int channel) { return convertValue(average[channel], counter, channel); }
	Value      getMin(int channel) { return convertValue(min[channel], 1, channel); }
	Value      getMax(int channel) { return convertValue(max[channel], 1, channel); }
	Value      getEff(int channel);
	Value      adcToVoltage( unsigned adc, int channel);
	unsigned   voltageToAdc( Value voltage, int channel);

	Value      getMeasuredFrequency() { return frequency; }
	Value      getChannelError(int channel) { return channelError[channel]; }

	Value      getScopeMin() { return scopeMin; }
	Value      getScopeMax() { return scopeMax; }
	Value      getScopeScale() { return scopeScale; }

	unsigned   getInternalReferenceValue() { return internalReferenceValue; }
	void       setInternalReferenceValue(unsigned val) {internalReferenceValue = val; }
	Value      getInternalReferenceError() { return internalReferenceError; }
};

#endif /* MEASUREMENTDATA_H_ */
