#include <OscilloscopeGrid.h>
#include <stdlib.h>
#include <string.h>
#include "graphics/Fonts.h"
#include "Config.h"
#include "Colors.h"
#include "xprintf.h"

#define OSCILLOSCOPE_HEIGHT   101
#define OSCILLOSCOPE_WIDTH    121
#define OSCILLOSCOPE_SEGMENT    4

OscilloscopeGrid::OscilloscopeGrid(TaskCanvas * canvasIn, MeasurementData * dataIn, unsigned xIn, unsigned yIn)
{
	canvas = canvasIn;
	data = dataIn;
	x = xIn;
	y = yIn;
	currentX = 0;

	new ScheduledAction( canvas, [] (TaskCanvas * taskCanvas, void * args) {
		OscilloscopeGrid * grid = (OscilloscopeGrid *)args;

		taskCanvas->setFont(font_4x6);

		char buf[15];
		xsprintf(buf, "%-7s", valueToString(grid->data->getScopeScale()));
		taskCanvas->drawText(1, 100, buf, OSCILLOSCOPE_COLOR_UNITS, COLOR_BLACK, ANGLE_270);

		Value timeorfreq;
		if( (config.getTriggerType() & TRIGGER_TYPE_MASK) == TRIGGER_NONE_FFT )
			timeorfreq = scaleValue(100000000 / grid->data->getSamplingInterval(), ValueScale::NONE, ValueUnit::FREQUENCY);
		else
			timeorfreq = scaleValue(256*grid->data->getSamplingInterval(), ValueScale::MICRO, ValueUnit::TIME);

		xsprintf(buf, "%-7s", valueToString(timeorfreq, true));
		taskCanvas->drawText(23, 121, buf, OSCILLOSCOPE_COLOR_UNITS, COLOR_BLACK);

		xsprintf(buf, "%-7s", valueToString(grid->data->getScopeMax()));
		taskCanvas->drawText(135, 25, buf, OSCILLOSCOPE_COLOR_UNITS, COLOR_BLACK, ANGLE_90);
		xsprintf(buf, "%7s", valueToString(grid->data->getScopeMin()));
		taskCanvas->drawText(135, 85, buf, OSCILLOSCOPE_COLOR_UNITS, COLOR_BLACK, ANGLE_90);

		Value freq = grid->data->getMeasuredFrequency();
		xsprintf(buf, "%8s", valueToString(freq));

		taskCanvas->setFont(font_5x7);
		taskCanvas->drawText(120, 120, buf, OSCILLOSCOPE_COLOR_FREQUENCY, COLOR_BLACK);
	}, this);
}

void OscilloscopeGrid::renderScopeGrid(unsigned currentX)
{
	unsigned segmSize = OSCILLOSCOPE_WIDTH - currentX;
	if( segmSize > OSCILLOSCOPE_SEGMENT )
		segmSize = OSCILLOSCOPE_SEGMENT;

	ColorBuffer * buffer = canvas->allocateColorBuffer(x+currentX, y, segmSize-1, OSCILLOSCOPE_HEIGHT-1);
	uint8_t * colors = buffer->buffer.data8;

	uint32_t color_bck = canvas->rgb(OSCILLOSCOPE_COLOR_BACKGROUND);
	uint32_t color_grid = canvas->rgb(OSCILLOSCOPE_COLOR_GRID);
	uint32_t color_zero = canvas->rgb(OSCILLOSCOPE_COLOR_ZERO);
	uint32_t color_trigger = canvas->rgb(OSCILLOSCOPE_COLOR_TRIGGER);

	unsigned colorSize = canvas->getColorSizeInBytes();

	ColorSetter setColor = canvas->getColorSetter();
	ColorFiller fillColor = canvas->getColorFiller();

	fillColor(colors, 0, segmSize * OSCILLOSCOPE_HEIGHT, color_bck);

	unsigned rowLen = segmSize * colorSize;
	for(unsigned yi=0; yi < OSCILLOSCOPE_HEIGHT; yi += 10 )
	{
		uint8_t * rowAddr = colors + (yi * rowLen);
		fillColor(rowAddr, 0, segmSize, color_grid);
	}
	for( unsigned xi=0; xi < segmSize; xi ++ )
	{
		if( (xi + currentX) % 10 == 0 )
		{
			unsigned rptr = xi;
			for(unsigned i=0; i < OSCILLOSCOPE_HEIGHT; i++)
			{
				setColor(colors, rptr, color_grid);
				rptr += segmSize;
			}
		}

		unsigned trg = config.getTriggerType() & TRIGGER_TYPE_MASK;
		if( trg != TRIGGER_NONE && trg != TRIGGER_NONE_FFT )
		{
			if( xi + currentX == config.getTriggerOffset() )
			{
				unsigned rptr = xi;
				for(unsigned i=0; i < OSCILLOSCOPE_HEIGHT; i+=3)
				{
					setColor(colors, rptr, color_trigger);
					rptr += segmSize*3;
				}
			}
		}
	}

	if( data->getZeroLine() < OSCILLOSCOPE_HEIGHT )
	{
		unsigned offs = segmSize * data->getZeroLine();
		fillColor(colors, offs, segmSize, color_zero);
	}

	renderChannel(1, colors, currentX, segmSize);
	if( config.getNumberOfChannels() >= 2 )
		renderChannel(2, colors, currentX, segmSize);

	canvas->fillColors(buffer);
}

void OscilloscopeGrid::renderChannel(unsigned channel, uint8_t * colors, unsigned currentX, unsigned segmSize)
{
	uint8_t * chdata = data->getChannelData(channel - 1);
	uint32_t  color;

	ColorSetter setColor = canvas->getColorSetter();

	switch(channel)
	{
	case 1:
		color = canvas->rgb(OSCILLOSCOPE_COLOR_CHANNEL_1);
		break;
	case 2:
		color = canvas->rgb(OSCILLOSCOPE_COLOR_CHANNEL_2);
		break;
	default:
		return;
	}

	if( chdata == 0 )
		return;

	for(unsigned i=0; i < segmSize; i++)
	{
		unsigned loc = currentX + i;
		unsigned dt = chdata[loc];
		unsigned nextDt = (loc == SAMPLE_NUMBER - 1) ? dt : chdata[loc+1];

		unsigned min = dt;
		unsigned max = dt;
		if( nextDt < min )
			min = nextDt;
		if( nextDt > max )
			max = nextDt;

		unsigned delta = min * segmSize + i;
		unsigned len = max - min;

		do
		{
			setColor(colors, delta, color);
			delta += segmSize;
		}while( len-- );
	}
}

void OscilloscopeGrid::buildCachedData()
{
	renderScopeGrid(currentX);
}

void OscilloscopeGrid::start()
{
	currentX += OSCILLOSCOPE_SEGMENT;

	if( currentX < OSCILLOSCOPE_WIDTH) {
		invalidateCache();
		resubmitTask();
	}
	else
		taskCompleted();
}
