#ifndef SYSTICKHANDLER_H_
#define SYSTICKHANDLER_H_

#include <inttypes.h>

typedef void (* SysTickCallback)();

class SysTickHandler
{
private:
	uint32_t                 clocksPerTick = 0;
	uint32_t                 clocksPerSecond = 0;
	uint32_t                 frequency = 0;
	static volatile uint64_t ticks;
	static SysTickCallback   sysTickCallback;
	static SysTickHandler *  instance;

public:
	SysTickHandler();

	void               init(unsigned frequency, SysTickCallback cb = 0);
	void               reinit();

	void               sleep(uint32_t millisToWait);
	uint64_t           getTicks();
	uint64_t           getMillis();

	inline static void tick()
	{
		ticks++;
		if( sysTickCallback )
			sysTickCallback();
	}

	inline static SysTickHandler * getInstance() { return instance; }
};

#endif /* SYSTICKHANDLER_H_ */
