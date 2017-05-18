// TODO: frequency doubling with fast interleaved mode
// TODO: PC communication - double buffering

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <OscilloscopeScreen.h>
#include "systick/SysTickHandler.h"
#include "diag/Trace.h"
#include "Sampler.h"
#include "Config.h"
#include "GainSetterTask.h"
#include "OscilloscopeGrid.h"
#include "MeasurementValueWidget.h"

#include "driver/SPIDriver.h"
#include "task/AbstractTask.h"
#include "task/TaskHandler.h"
#include "pins.h"
#include "lcd/LCD_ST7735.h"
#include "graphics/Fonts.h"
#include "MeasurementData.h"
#include "TriggerHandler.h"
#include "TriggerWidget.h"
#include "SetupMenu.h"
#include "pwm.h"
#include "USBHandler.h"

#define TICK_FREQUENCY 1000

#define LCD_BACKLIGHT PA9
#define LCD_A0        PA8
#define LCD_CS        PA4
#define PGA_CS        PB8

#define TRIGGER_WAIT_LOCATION_X  140
#define TRIGGER_WAIT_LOCATION_Y   59

SysTickHandler sysTickHandler;
TaskHandler taskHandler;
SPIDriver spiDriver(1);
LCD_ST7735 lcd;
MeasurementData measurementData;
Sampler sampler(spiDriver.requiredResources(), measurementData);
SetupMenu setupMenu(&lcd);

const uint8_t bmp_trigger_search [] = {
	2,
	2,
	0b01000000,
	0b11100000,
	0b01000000,
};

void drawTriggerSearch(bool isSearch)
{
	uint32_t color = isSearch ? COLOR_YELLOW : COLOR_BLUE;
	lcd.drawImage(TRIGGER_WAIT_LOCATION_X, TRIGGER_WAIT_LOCATION_Y, bmp_trigger_search, color, COLOR_BLUE);
}

void displayHeap()
{
	struct mallinfo info = mallinfo();
	trace_printf("%d/%d/%d\n", info.uordblks, info.fordblks, info.arena);
}

void setupTestSignal()
{
	if( config.isTestSignalEnabled() > 0 )
	{
		pwmSetEnabled(PB7, true);
		pwmSetFrequency(PB7, config.getTestSignalFrequency());
		pwmSetDuty(PB7, config.getTestSignalDuty());
	}
	else
		pwmSetEnabled(PB7, false);
}

void refresh(RefreshType type){
	switch(type)
	{
	case REFRESH_OSCILLOSCOPE:
		{
			drawTriggerSearch(false);
			OscilloscopeGrid *osc = new OscilloscopeGrid(&lcd, &measurementData, GRID_X, GRID_Y);
			TaskHandler::getInstance().scheduleTask(osc);
		}
		break;
	case REFRESH_TRIGGER:
		{
			TriggerWidget * triggerWidget = new TriggerWidget(&measurementData);
			scheduleWidget<TriggerWidget>(&lcd, triggerWidget);
			drawTriggerSearch(true);
		}
		break;
	case REFRESH_MEASUREMENT_VALUE:
		{
			MeasurementValueWidget * measurementWidget = new MeasurementValueWidget(&measurementData);
			scheduleWidget<MeasurementValueWidget>(&lcd, measurementWidget);
			drawTriggerSearch(true);
		}
		break;
	default:
		break;
	}
}

void processDrawingTasks()
{
	while( TaskHandler::getInstance().getTasksToProcess(lcd.requiredResources()) > 0 )
	{
		TaskHandler::getInstance().loop();
		Sampler::getInstance()->loop();
		inputDeviceHandler.loop();
	}

}

int main(int, char**)
{
	usbHandler.init();
	Config::init();
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_2 );

	sysTickHandler.init(TICK_FREQUENCY);

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	pwmInit(PB7);
	// test PWM signal
	setupTestSignal();

	spiDriver.init();
	inputDeviceHandler.init([](InputDeviceEvent inputDeviceEvent) {
		if( inputDeviceEvent != JOY_NONE )
			sampler.enableSingleShotTrigger();

		switch(inputDeviceEvent)
		{
		case JOY_LEFT:
		case JOY_REPEAT_LEFT:
			{
				sampler.decreaseSamplingRate();
				sampler.restart();

				refresh(REFRESH_OSCILLOSCOPE);
			}
			break;
		case JOY_RIGHT:
		case JOY_REPEAT_RIGHT:
			{
				sampler.increaseSamplingRate();
				sampler.restart();

				refresh(REFRESH_OSCILLOSCOPE);
			}
			break;
		case JOY_UP:
		case JOY_REPEAT_UP:
			taskHandler.scheduleTask( new GainSetterTask(nextGain(currentGain())) );
			break;
		case JOY_DOWN:
		case JOY_REPEAT_DOWN:
			taskHandler.scheduleTask( new GainSetterTask(previousGain(currentGain())) );
			break;
		case JOY_FIRE_UP:
			{
				TriggerHandler::jumpToNextTrigger();
				refresh(REFRESH_TRIGGER);
				sampler.restart();
			}
			break;
		case JOY_FIRE_DOWN:
			{
				TriggerHandler::jumpToPreviousTrigger();
				refresh(REFRESH_TRIGGER);
				sampler.restart();
			}
			break;
		case JOY_FIRE_LEFT:
		case JOY_REPEAT_FIRE_LEFT:
			{
				int step = (inputDeviceEvent == JOY_REPEAT_FIRE_LEFT) ? 5 : 1;
				unsigned offset = config.getTriggerOffset() + step;
				if( offset > 120 )
					offset = 120;
				config.setTriggerOffset(offset);

				refresh(REFRESH_OSCILLOSCOPE);
			}
			break;
		case JOY_FIRE_RIGHT:
		case JOY_REPEAT_FIRE_RIGHT:
			{
				int step = (inputDeviceEvent == JOY_REPEAT_FIRE_RIGHT) ? 5 : 1;
				int offset = (int)config.getTriggerOffset();
				offset -= step;
				if( offset < 0 )
					offset = 0;
				config.setTriggerOffset((unsigned)offset);

				refresh(REFRESH_OSCILLOSCOPE);
			}
			break;
		case JOY_FIRE:
			// enable trigger
			break;
		case JOY_DOUBLE_FIRE:
		case USB_COMMAND_START_TRANSMISSION:
			{
				if( inputDeviceEvent == JOY_DOUBLE_FIRE )
					setupMenu.showMenu(&mainMenu);
				else
					setupMenu.showMenu(&usbTransmitMenu, &usbStartTransmission);

				sampler.restart();
				setupTestSignal();

				OscilloscopeScreen *oscilloscopeScreen = new OscilloscopeScreen( &measurementData );
				scheduleWidget<OscilloscopeScreen>(&lcd, oscilloscopeScreen);

				// apply config changes
				taskHandler.scheduleTask( new GainSetterTask(currentGain()) );
			}
			break;
		default:
			break;
		}
	});
	sampler.init(refresh);
	lcd.init(&spiDriver, LCD_A0, LCD_CS, LCD_BACKLIGHT);

	GainSetterTask::init(&lcd, &spiDriver, PGA_CS);
	taskHandler.scheduleTask( new GainSetterTask(currentGain()) );
	processDrawingTasks();

	OscilloscopeScreen *oscilloscopeScreen = new OscilloscopeScreen( &measurementData );
	scheduleWidget<OscilloscopeScreen>(&lcd, oscilloscopeScreen);

	while(1)
	{
		taskHandler.loop();
		sampler.loop();
		inputDeviceHandler.loop();
	}

	return 0;
}

