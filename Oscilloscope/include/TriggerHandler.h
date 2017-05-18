#ifndef TRIGGERHANDLER_H_
#define TRIGGERHANDLER_H_

#include "Config.h"
#include <inttypes.h>

typedef void (*TriggerCallback)(int cycles_to_collect);

class TriggerHandler
{
private:
	bool inited = false;

	void initializeTrigger();
	void enableWatchDog();

public:
	void init(TriggerCallback callback);
	void loop();

	void setupTrigger();
	void destroyTrigger();

	void reorderSampleBuffer(uint32_t * buffer, int size, int circularReserve);

	static void jumpToNextTrigger();
	static void jumpToPreviousTrigger();
};


extern TriggerHandler triggerHandler;


#endif /* TRIGGERHANDLER_H_ */
