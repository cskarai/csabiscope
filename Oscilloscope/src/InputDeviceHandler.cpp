#include "InputDeviceHandler.h"
#include "Sampler.h"
#include "pins.h"
#include "systick/SysTime.h"

#define JOY_PRELL_TIME      30
#define JOY_REPEAT_TIME    500
#define JOY_POLL_MILLIS     10

#define JOY_CENTER        2048
#define JOY_ON_LIMIT      1024
#define JOY_FIRE_ON_LIMIT  400
#define JOY_HYSTERESIS     200

#define DOUBLE_FIRE_TIME   500

InputDeviceHandler inputDeviceHandler;

void InputDeviceHandler::init(InputDeviceCallback cb)
{
	inputDeviceCallback=cb;

	enableRCC(PIN_JOY_FIRE);
	pinMode(PIN_JOY_FIRE, GPIO_Mode_IPU);

}

InputDeviceEvent InputDeviceHandler::readJoystickState(unsigned time)
{
	unsigned evnt = JOY_NONE;
	if( ! readPin(PIN_JOY_FIRE) )
		evnt |= JOY_FIRE;

	 // read joystick
	unsigned joyx, joyy;
	if(( time - lastJoyReadStamp > JOY_POLL_MILLIS ) && Sampler::getInstance()->readJoy(&joyx, &joyy) )
	{
		unsigned joyOnLimit = ( evnt & JOY_FIRE ) ? JOY_FIRE_ON_LIMIT : JOY_ON_LIMIT;

		unsigned joyxMin = JOY_CENTER - joyOnLimit + ((lastEvent & JOY_LEFT) ? JOY_HYSTERESIS : -JOY_HYSTERESIS);
		unsigned joyxMax = JOY_CENTER + joyOnLimit + ((lastEvent & JOY_RIGHT) ? -JOY_HYSTERESIS : JOY_HYSTERESIS);

		if( joyx <= joyxMin )
			evnt |= JOY_RIGHT;
		if( joyx >= joyxMax )
			evnt |= JOY_LEFT;

		unsigned joyyMin = JOY_CENTER - joyOnLimit + ((lastEvent & JOY_DOWN) ? JOY_HYSTERESIS : -JOY_HYSTERESIS);
		unsigned joyyMax = JOY_CENTER + joyOnLimit + ((lastEvent & JOY_UP) ? -JOY_HYSTERESIS : JOY_HYSTERESIS);

		if( joyy <= joyyMin )
			evnt |= JOY_UP;
		if( joyy >= joyyMax )
			evnt |= JOY_DOWN;

		lastJoyReadStamp = time;
	}
	else
		evnt |= (lastEvent & (JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT));

	return (InputDeviceEvent)evnt;
}

void InputDeviceHandler::loop()
{
	unsigned time = (unsigned)SysTime::getMillis();
	InputDeviceEvent evnt = readJoystickState(time);
	if( evnt != lastEvent )
	{
		if( time - lastJoyEventStamp < JOY_PRELL_TIME )
			return;

		if( (evnt != JOY_NONE ) && (evnt != JOY_FIRE ) )
			doubleFireCounter = 0xFFFFFFFF;

		lastEvent = evnt;
		lastJoyEventStamp = time;

		if( evnt == JOY_FIRE )
		{
			if(( doubleFireCounter != 0xFFFFFFFF ) && ( time - doubleFireCounter < DOUBLE_FIRE_TIME ))
			{
				evnt = JOY_DOUBLE_FIRE;
				doubleFireCounter = 0xFFFFFFFF;
			}
			else
				doubleFireCounter = time;
		}



		if( inputDeviceCallback )
			inputDeviceCallback(evnt);
	}
	else
	{
		if( inputDeviceCallback && ( lastEvent != JOY_NONE ) &&
				( time - lastJoyEventStamp >= JOY_REPEAT_TIME ))
		{
			lastJoyEventStamp = time;
			inputDeviceCallback((InputDeviceEvent)(evnt | JOY_REPEAT));
		}
	}

	if( usbEvent != USB_COMMAND_NONE )
	{
		InputDeviceEvent event = usbEvent;
		usbEvent = USB_COMMAND_NONE;
		inputDeviceCallback(event);
	}
}

void InputDeviceHandler::noJoystickData()
{
	lastEvent = (InputDeviceEvent)(lastEvent & ~( JOY_UP | JOY_RIGHT | JOY_DOWN | JOY_LEFT ));
}
