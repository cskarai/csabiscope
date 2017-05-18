#include "GainSetterTask.h"
#include "AmplifierWidget.h"

const char * getGainName(AmplifierGain name)
{
	switch(name)
	{
	case GAIN_1X:
		return "1X";
	case GAIN_2X:
		return "2X";
	case GAIN_4X:
		return "4X";
	case GAIN_5X:
		return "5X";
	case GAIN_8X:
		return "8X";
	case GAIN_10X:
		return "10X";
	case GAIN_16X:
		return "16X";
	case GAIN_32X:
		return "32X";
	default:
		return "";
	}
}

int getGainValue(AmplifierGain name)
{
	switch(name)
	{
	case GAIN_1X:
		return 1;
	case GAIN_2X:
		return 2;
	case GAIN_4X:
		return 4;
	case GAIN_5X:
		return 5;
	case GAIN_8X:
		return 8;
	case GAIN_10X:
		return 10;
	case GAIN_16X:
		return 16;
	case GAIN_32X:
		return 32;
	default:
		return 1;
	}
}

AmplifierGain currentGain()
{
	return config.getCurrentGain();
}

AmplifierGain previousGain(AmplifierGain gain)
{
	if( gain==GAIN_1X )
		return gain;
	return (AmplifierGain)((int)gain - 1);
}

AmplifierGain nextGain(AmplifierGain gain)
{
	if( gain==GAIN_32X )
		return gain;
	return (AmplifierGain)((int)gain + 1);
}

AbstractDriverContext * GainSetterTask::driverContext = 0;
TaskCanvas            * GainSetterTask::taskCanvas = 0;

void GainSetterTask::init(TaskCanvas * canvas, SPIDriver * driver, STM32Pin cspin)
{
	taskCanvas = canvas;
	driverContext = driver->createSPIContext(cspin, SPI_BaudRatePrescaler_16);
}

void GainSetterTask::transferCompleteCb(void *arg)
{
	GainSetterTask * task = (GainSetterTask *)arg;
	task->taskCompleted();
	config.setCurrentGain( task->gainToSet );

	AmplifierWidget * amplifierWidget = new AmplifierWidget();
	scheduleWidget<AmplifierWidget>(taskCanvas, amplifierWidget);
}

void GainSetterTask::start()
{
	command[0] = 0x40; // gain to set
	command[1] = (uint8_t)gainToSet;

	driverContext->getDriver()->startDataTransfer(driverContext, 2, command, TT_DATA_8_BIT, GainSetterTask::transferCompleteCb, this);
}
