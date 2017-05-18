#ifndef SCENARIOPLAYERTASK_H_
#define SCENARIOPLAYERTASK_H_

#include "task/AbstractTask.h"
#include "driver/AbstractDriver.h"

typedef enum
{
	SC_COMMAND     = 0,
	SC_DELAY       = 128,
	SC_FLOOD       = 129,
	SC_SEND_BUFFER = 130,
	SC_DONE        = 131,
	SC_PAD         = 132,
} Scene;

class ScenarioPlayerTask : public virtual AbstractTask
{
private:
	AbstractDriverContext  *driverContext;

	volatile const uint8_t *scenario = 0;
	volatile unsigned       state;
	volatile uint32_t       data = 0;
	unsigned                colorSize;

	static void transferCompleteCb(void *arg);
	void transferCompleted();

	ScenarioPlayerTask(unsigned colorSizeIn, AbstractDriverContext *driverContextIn, const uint8_t * scenarioIn);

protected:
	void jumpToNextCommand();
	void playCommand();

public:
	static ScenarioPlayerTask * createScenarioPlayer( unsigned colorSize, AbstractDriverContext *driverContextIn, const uint8_t * scenarioIn );
	static ScenarioPlayerTask * createScenarioPlayer( unsigned colorSize, AbstractDriverContext *driverContextIn, unsigned len );

	uint8_t * getScenario() { return (uint8_t *) scenario; }

	virtual uint32_t requiredResources() {return driverContext->getDriver()->requiredResources();}
	virtual void start();
	virtual void loop();
};

#endif /* SCENARIOPLAYERTASK_H_ */
