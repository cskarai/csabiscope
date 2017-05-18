#ifndef CONFIG_H_
#define CONFIG_H_

#define MAX_PROBE_HEADS           3

#define CALIBRATION_REFERENCE  1494

#define CALIBRATED_VALUE_1    12575
#define CALIBRATED_VALUE_2    50000
#define CALIBRATED_VALUE_3    50000

#include <inttypes.h>

enum CustomMeter
{
	EffectiveValue,
	ChannelError,
	PowerError,
};

enum AmplifierGain
{
	GAIN_1X=0,
	GAIN_2X,
	GAIN_4X,
	GAIN_5X,
	GAIN_8X,
	GAIN_10X,
	GAIN_16X,
	GAIN_32X,
};

enum FFTMagnify
{
	FFT_MAGNIFY_AUTO=0,
	FFT_MAGNIFY_1X=2,
	FFT_MAGNIFY_1_5X=3,
	FFT_MAGNIFY_2X=4,
	FFT_MAGNIFY_2_5X=5,
	FFT_MAGNIFY_3X=6,
	FFT_MAGNIFY_3_5X=7,
	FFT_MAGNIFY_4X=8,
};

enum TriggerType
{
	TRIGGER_TYPE_MASK=0x0F,

	TRIGGER_NONE=0,
	TRIGGER_RISING_EDGE,
	TRIGGER_FALLING_EDGE,
	TRIGGER_CHANGING_EDGE,
	TRIGGER_POSITIVE_PEAK,
	TRIGGER_NEGATIVE_PEAK,
	TRIGGER_EXTERNAL,
	TRIGGER_NONE_FFT,

	TRIGGER_SINGLESHOT=0x10,
	TRIGGER_SINGLESHOT_NONE          = TRIGGER_SINGLESHOT+TRIGGER_NONE,
	TRIGGER_SINGLESHOT_RISING_EDGE   = TRIGGER_SINGLESHOT+TRIGGER_RISING_EDGE,
	TRIGGER_SINGLESHOT_FALLING_EDGE  = TRIGGER_SINGLESHOT+TRIGGER_FALLING_EDGE,
	TRIGGER_SINGLESHOT_CHANGING_EDGE = TRIGGER_SINGLESHOT+TRIGGER_CHANGING_EDGE,
	TRIGGER_SINGLESHOT_POSITIVE_PEAK = TRIGGER_SINGLESHOT+TRIGGER_POSITIVE_PEAK,
	TRIGGER_SINGLESHOT_NEGATIVE_PEAK = TRIGGER_SINGLESHOT+TRIGGER_NEGATIVE_PEAK,
	TRIGGER_SINGLESHOT_EXTERNAL      = TRIGGER_SINGLESHOT+TRIGGER_EXTERNAL,
	TRIGGER_SINGLESHOT_NONE_FFT      = TRIGGER_SINGLESHOT+TRIGGER_NONE_FFT,
};

enum TransmitType
{
	TRANSMIT_8_BIT_SINGLE_CHANNEL = 0,
	TRANSMIT_8_BIT_DUAL_CHANNEL = 1,
	TRANSMIT_12_BIT_SINGLE_CHANNEL = 2,
	TRANSMIT_12_BIT_DUAL_CHANNEL = 3,
};

class Config
{
private:
	unsigned       numberOfChannels            =               2;
	unsigned       frequencyMeasuringChannel   =               0;
	unsigned       samplingInterval            =             100;
	unsigned       oscilloscopeUpdateFrequency =             500;
	CustomMeter    customMeter                 =  EffectiveValue;
	bool           testSignalEnabled           =            true;
	unsigned       testSignalFrequency         =            5000;
	unsigned       testSignalDuty              =              50;
	bool           autoZoom                    =           false;
	AmplifierGain  currentGain                 =         GAIN_1X;
	unsigned       currentProbeHead            =               0;
	TriggerType    triggerType                 =    TRIGGER_NONE;
	unsigned       triggerChannel              =               0;
	unsigned       triggerOffset               =               0;
	unsigned       triggerPercent              =              10;
	FFTMagnify     fftMagnification            =FFT_MAGNIFY_AUTO;

	unsigned       calibrationValue[MAX_PROBE_HEADS][2] =  {{CALIBRATED_VALUE_1, CALIBRATED_VALUE_1},
	                                                        {CALIBRATED_VALUE_2, CALIBRATED_VALUE_2},
	                                                        {CALIBRATED_VALUE_3, CALIBRATED_VALUE_3} };

	TransmitType   transmitType                = TRANSMIT_12_BIT_SINGLE_CHANNEL;
	unsigned       transmitSamplingInterval    =             100;

	uint32_t       crc                         =               0;

public:
	unsigned       getNumberOfChannels()                        { return numberOfChannels; }
	void           setNumberOfChannels(unsigned val)            { numberOfChannels = val; }

	unsigned       getSamplingInterval()                        { return samplingInterval; }
	void           setSamplingInterval(unsigned val)            { samplingInterval=val; }

	unsigned       getOscilloscopeUpdateFrequency()             { return oscilloscopeUpdateFrequency; }
	void           setOscilloscopeUpdateFrequency(unsigned val) { oscilloscopeUpdateFrequency=val; }

	unsigned       getFrequencyMeasuringChannel()               { return frequencyMeasuringChannel; }
	void           setFrequencyMeasuringChannel(unsigned val)   { frequencyMeasuringChannel = val; }

	CustomMeter    getCustomMeter()                             { return customMeter; }
	void           setCustomMeter(CustomMeter val)              { customMeter = val; }

	bool           isTestSignalEnabled()                        { return testSignalEnabled; }
	void           setTestSignalEnabled(bool val)               { testSignalEnabled = val; }

	unsigned       getTestSignalFrequency()                     { return testSignalFrequency; }
	void           setTestSignalFrequency(unsigned val)         { testSignalFrequency = val; }

	unsigned       getTestSignalDuty()                          { return testSignalDuty; }
	void           setTestSignalDuty(unsigned val)              { testSignalDuty = val; }

	bool           isAutoZoom()                                 { return autoZoom; }
	void           setAutoZoom(bool val)                        { autoZoom = val; }

	FFTMagnify     getFFTMagnification()                        { return fftMagnification; }
	void           setFFTMagnification(FFTMagnify val)          { fftMagnification = val; }

	unsigned       getCalibrationValue(unsigned channel)        { return calibrationValue[currentProbeHead][channel & 1]; }
	void           setCalibrationValue(unsigned channel, unsigned val) { calibrationValue[currentProbeHead][channel & 1] = val; }

	unsigned       getCalibrationReference()                    { return CALIBRATION_REFERENCE; }

	AmplifierGain  getCurrentGain()                             { return currentGain; }
	void           setCurrentGain(AmplifierGain val)            { currentGain = val; }

	unsigned       getCurrentProbeHead()                        { return currentProbeHead; }
	void           setCurrentProbeHead(unsigned val)            { currentProbeHead = val; }

	TriggerType    getTriggerType()                             { return triggerType; }
	void           setTriggerType(TriggerType val)              { triggerType = val; }

	unsigned       getTriggerOffset()                           { return triggerOffset; }
	void           setTriggerOffset(unsigned val)               { triggerOffset = val; }

	unsigned       getTriggerChannel()                          { return triggerChannel; }
	void           setTriggerChannel(unsigned val)              { triggerChannel = val; }

	unsigned       getTriggerPercent()                          { return triggerPercent; }
	void           setTriggerPercent(unsigned val)              { triggerPercent = val; }

	TransmitType   getTransmitType()                            { return transmitType; }
	void           setTransmitType(TransmitType val)            { transmitType = val; }

	unsigned       getTransmitSamplingInterval()                { return transmitSamplingInterval; }
	void           setTransmitSamplingInterval(unsigned val)    { transmitSamplingInterval=val; }

	// helper methods for SetupMenu
public:
	unsigned       getCurrentGainInt()                          { return (unsigned)currentGain; }
	void           setCurrentGainInt(unsigned val)              { currentGain = (AmplifierGain)val; }
	unsigned       getTriggerTypeInt()                          { return (unsigned)triggerType & (unsigned)TRIGGER_TYPE_MASK; }
	void           setTriggerTypeInt(unsigned val)              { triggerType = (TriggerType)(((unsigned)triggerType & (~(unsigned)TRIGGER_TYPE_MASK)) | val); }
	unsigned       getCustomMeterInt()                          { return (unsigned)customMeter; }
	void           setCustomMeterInt(unsigned val)              { customMeter = (CustomMeter)val; }
	bool           isTriggerSingleShot()                        { return (unsigned)triggerType & (unsigned)TRIGGER_SINGLESHOT; }
	void           setTriggerSingleShot(bool val)               { if(val) triggerType = (TriggerType)((unsigned)triggerType | (unsigned)TRIGGER_SINGLESHOT); else triggerType = (TriggerType)((unsigned)triggerType & ~(unsigned)TRIGGER_SINGLESHOT); }
	unsigned       getCalibrationValue1()                       { return calibrationValue[currentProbeHead][0]; }
	void           setCalibrationValue1(unsigned val)           { calibrationValue[currentProbeHead][0] = val; }
	unsigned       getCalibrationValue2()                       { return calibrationValue[currentProbeHead][1]; }
	void           setCalibrationValue2(unsigned val)           { calibrationValue[currentProbeHead][1] = val; }
	unsigned       getCalibrationValueProbe(unsigned channel, unsigned probe)               { return calibrationValue[probe][channel & 1]; }
	void           setCalibrationValueProbe(unsigned channel, unsigned probe, unsigned val) { calibrationValue[probe][channel & 1] = val; }
	unsigned       getTransmitTypeInt()                         { return ((unsigned)transmitType & ~1); }
	void           setTransmitTypeInt(unsigned val)             { transmitType = (TransmitType)(((unsigned)transmitType & 1) | val); }
	bool           isDualChannelTransmit()                      { return ((unsigned)transmitType & 1) != 0; }
	void           setDualChannelTransmit( bool val )           { transmitType = (TransmitType)(((unsigned)transmitType & ~1) | (val ? 1 : 0)); }
	unsigned       getFFTMagnificationInt()                     { return (unsigned)fftMagnification; }
	void           setFFTMagnificationInt(unsigned val)         { fftMagnification = (FFTMagnify)val; }

	void           calculateCRC();

	static void    loadConfig(Config * config);
	static void    saveConfig(Config * config);
	static void    init();
};

extern Config config;

#endif /* CONFIG_H_ */
