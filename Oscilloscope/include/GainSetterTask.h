#ifndef GAINSETTERTASK_H_
#define GAINSETTERTASK_H_

#include "task/AbstractTask.h"
#include "graphics/TaskCanvas.h"
#include "driver/AbstractDriver.h"
#include "driver/SPIDriver.h"
#include "pins.h"
#include "Config.h"

const char *  getGainName(AmplifierGain name);
int           getGainValue(AmplifierGain name);
AmplifierGain currentGain();
AmplifierGain previousGain(AmplifierGain gain);
AmplifierGain nextGain(AmplifierGain gain);

class GainSetterTask : public virtual AbstractTask
{
private:
	static AbstractDriverContext * driverContext;
	static TaskCanvas            * taskCanvas;

	AmplifierGain gainToSet;
	uint8_t command[2];
	bool completed = false;

	static void transferCompleteCb(void *arg);

public:
	static void init(TaskCanvas * canvas, SPIDriver * driver, STM32Pin cspin);

	GainSetterTask(AmplifierGain gain) : gainToSet(gain) {}

	virtual void start();
	virtual uint32_t requiredResources() {return driverContext->getDriver()->requiredResources(); }
};


#endif /* GAINSETTERTASK_H_ */
