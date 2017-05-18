#include "systick/SysTickHandler.h"
#include "systick/SysTime.h"

#include <cmsis/cmsis_device.h>

volatile uint64_t SysTickHandler::ticks = 0;
SysTickCallback   SysTickHandler::sysTickCallback = 0;
SysTickHandler *  SysTickHandler::instance = 0;

SysTickHandler::SysTickHandler()
{
	instance = this;
	SysTime::setMillisGetter([]() {
			return SysTickHandler::getInstance()->getMillis();
	});
}

extern "C" void SysTick_Handler(void)
{
	SysTickHandler::tick();
}

void SysTickHandler::init(unsigned frequencyIn, SysTickCallback cb)
{
	frequency = frequencyIn;
	sysTickCallback = cb;
	reinit();
}

void SysTickHandler::reinit()
{
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq (&RCC_Clocks);
	clocksPerSecond = RCC_Clocks.HCLK_Frequency;
	SysTick_Config (clocksPerTick = clocksPerSecond / frequency);
}

void SysTickHandler::sleep(uint32_t millisToWait)
{
	uint64_t end = getMillis() + millisToWait;

	while(getMillis() < end);
}

uint64_t SysTickHandler::getTicks()
{
	uint64_t t1, t2;
	do
	{
		t1 = ticks;
		t2 = ticks;
	}while(t1 != t2);
	return t1;
}

uint64_t SysTickHandler::getMillis()
{
	uint64_t millis = getTicks() * 1000 * clocksPerTick / clocksPerSecond;
	return millis;
}
