#ifndef SAMPLER_H_
#define SAMPLER_H_

#include "MeasurementData.h"
#include "pins.h"

#define CIRCULAR_RESERVE   10
#define SAMPLE_BUFFER_SIZE (2*(SAMPLE_NUMBER+CIRCULAR_RESERVE))

#define PIN_JOYY      PA0
#define PIN_JOYX      PA1
#define PIN_VREF      PA2
#define PIN_CHANNEL_1 PB0
#define PIN_CHANNEL_2 PB1

typedef enum
{
	ST_NONE,
	ST_SAMPLE_VREF_AND_VCC,
	ST_COLLECT_DATA,
	ST_TRIGGER_WAIT,
	ST_SAMPLE_LINES,

	ST_DATA_TRANSMIT,

	ST_MAX,
} SampleState;

typedef enum
{
	SM_TIMER,
	SM_CONTINUOUS,
	SM_FAST_INTERLEAVED,
} SamplingMethod;

typedef enum
{
	REFRESH_OSCILLOSCOPE,
	REFRESH_TRIGGER,
	REFRESH_MEASUREMENT_VALUE,
} RefreshType;

typedef void (*RefreshCallback)(RefreshType joy);

class TaskCanvas;

class Sampler
{
private:
	unsigned          lcdResources;
	MeasurementData & measurementData;
	uint32_t          sampleBuffer[SAMPLE_BUFFER_SIZE];
	volatile bool     sampleDone = true;
	SampleState       sampleState = ST_NONE;
	SamplingMethod    samplingMethod = SM_TIMER;
	unsigned          lastTimeStamp = 0xF0000000;
	bool              timeExpired = false;
	bool              lcdWait = false;
	bool              singleShotTriggerEnable = true;
	bool              lastSingleShotEnabled = false;

	RefreshCallback   refreshCallback = 0;

	static Sampler *  instance;

	void              nextState();
	void              setupState();
	void              processResults();

	static unsigned   getSamplingRateIndex(unsigned rate);

	void              setSamplerPriority(int prio, int subPrio);

public:
	Sampler(unsigned lcdResources, MeasurementData & data);

	static void       handleIrq(int isCompleted);

	void              init(RefreshCallback rf=0);
	void              loop();

	void              setSamplingTimeMicro(unsigned micro);

	static Sampler *  getInstance() { return instance; }

	static unsigned   getNextSamplingRate(unsigned rate);
	static unsigned   getPreviousSamplingRate(unsigned rate);

	void              increaseSamplingRate();
	void              decreaseSamplingRate();

	void              enableSingleShotTrigger() { singleShotTriggerEnable = true; }
	bool              isSingleShotTriggerEnabled() { return singleShotTriggerEnable; }

	void              restart();

	RefreshCallback   getRefreshCallback() { return refreshCallback; }
	void              setRefreshCallback( RefreshCallback cb ) { refreshCallback = cb; }

	MeasurementData & getMeasurementData() { return measurementData;}

	void              setSampleDone() { sampleDone = true; }

	void              startTransmitter( void  (*transmitter_cb)(int complete));
	void              setTransmitterPriority(int prio, int subPrio) {setSamplerPriority(prio, subPrio);}

	uint32_t *        getSampleBuffer() { return sampleBuffer; }

	bool              readJoy(unsigned * joyx, unsigned *joyy);
};

#endif /* SAMPLER_H_ */
