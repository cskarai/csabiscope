#include "MeasurementValueWidget.h"
#include "graphics/Fonts.h"
#include "Colors.h"
#include "xprintf.h"
#include "Config.h"
#include <string.h>

#define INTERNAL_REFERENCE_VOLTAGE 1.21

MeasurementValueWidget::MeasurementValueWidget(MeasurementData *dataIn) :
	data(dataIn)
{
}

void MeasurementValueWidget::channel(char * buffer, int channel)
{
	Value average = data->getAverage(channel);
	Value min = data->getMin(channel);
	Value max = data->getMax(channel);
	Value cus;

	switch( config.getCustomMeter() )
	{
	case PowerError:
		cus = data->getInternalReferenceError();
		break;
	case ChannelError:
		cus = data->getChannelError(channel);
		break;
	case EffectiveValue:
	default:
		cus = data->getEff(channel);
		break;
	}

	min.unit = ValueUnit::NONE;
	max.unit = ValueUnit::NONE;
	cus.unit = ValueUnit::NONE;

	char avbuf[20];
	strcpy( avbuf, valueToString(average) );
	char minbuf[20];
	strcpy( minbuf, valueToString(min) );
	char maxbuf[20];
	strcpy( maxbuf, valueToString(max) );
	char cusbuf[20];
	strcpy( cusbuf, valueToString(cus) );

	xsprintf(buffer, "%7s [%s,%s]", avbuf, minbuf, maxbuf);

	while(strlen(buffer) < 23)
		strcat(buffer," ");

	switch( config.getCustomMeter() )
	{
	case EffectiveValue:
		xsprintf(buffer+strlen(buffer), "E%5s", cusbuf);
		break;
	case ChannelError:
		xsprintf(buffer+strlen(buffer), " %4s%c", cusbuf, '%');
		break;
	case PowerError:
		if( channel == 0 )
		{
			unsigned refVal = data->getInternalReferenceValue();
			Value refV(0xFFF00 * INTERNAL_REFERENCE_VOLTAGE / refVal, ValueScale::NONE, ValueUnit::VOLTAGE);
			strcpy( cusbuf, valueToString(refV) );
			xsprintf(buffer+strlen(buffer), " %5s", cusbuf );
		}
		else
			xsprintf(buffer+strlen(buffer), " %4s%c", cusbuf, '%');
		break;
	default:
		break;
	}
}

void MeasurementValueWidget::draw(TaskCanvas * canvas)
{
	char buf[40];
	canvas->setFont(font_5x7);
	channel(buf, 0);
	canvas->drawText(15, 1, buf, OSCILLOSCOPE_COLOR_CHANNEL_1_TEXT, COLOR_BLACK);

	if( config.getNumberOfChannels() < 2 )
	{
		memset(buf, ' ', 30);
		buf[29] = 0;
	}
	else
		channel(buf, 1);
	canvas->drawText(15, 9, buf, OSCILLOSCOPE_COLOR_CHANNEL_2_TEXT, COLOR_BLACK);
}
