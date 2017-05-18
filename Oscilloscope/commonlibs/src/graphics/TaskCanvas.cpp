#include "graphics/TaskCanvas.h"
#include "task/AbstractTask.h"
#include "task/TaskHandler.h"
#include <stm32f10x_flash.h>
#include <stdlib.h>
#include <string.h>

void TaskCanvas::playScenario(ScenarioPlayerTask * scpTask)
{
	TaskHandler::getInstance().scheduleTask(scpTask);
}

void TaskCanvas::processBitmapSegment(TaskCanvas * canvas, void * args)
{
	BitmapSegmenterDescriptor * desc = (BitmapSegmenterDescriptor *)args;

	canvas->drawBitmapSegment(desc, desc->segmenterData+desc->userData);
	desc->userData++;

	if( desc->userData < desc->segments )
	{
		new ScheduledAction(canvas, TaskCanvas::processBitmapSegment, desc );
	}
	else
	{
		if( !IS_FLASH_ADDRESS((uint32_t)desc->map) )
			free((uint8_t *)desc->map);
		free(desc);
	}
}

void TaskCanvas::drawBitmapSegments(BitmapSegmenterDescriptor * desc)
{
	if( desc->segments == 0 )
		return;

	if( !IS_FLASH_ADDRESS((uint32_t)desc->map) )
	{
		uint8_t * map = (uint8_t *)malloc(desc->size);
		memcpy(map, desc->map, desc->size);
		desc->map = map;
	}

	new ScheduledAction(this, TaskCanvas::processBitmapSegment, desc );
}

void TaskCanvas::processTextSegment(TaskCanvas * canvas, void * args)
{
	TextSegmenterDescriptor * desc = (TextSegmenterDescriptor *)args;

	canvas->drawTextSegment(desc, desc->segmenterData+desc->userData);
	desc->userData++;

	if( desc->userData < desc->segments )
	{
		new ScheduledAction(canvas, TaskCanvas::processTextSegment, desc );
	}
	else
	{
		if( !IS_FLASH_ADDRESS((uint32_t)desc->text) )
			free((uint8_t *)desc->text);
		free(desc);
	}
}

void TaskCanvas::drawTextSegments(TextSegmenterDescriptor * desc)
{
	if( desc->segments == 0 )
		return;

	if( !IS_FLASH_ADDRESS((uint32_t)desc->text) )
	{
		char * txt = (char *)malloc(strlen(desc->text)+1);
		strcpy(txt, desc->text);
		desc->text = txt;
	}

	new ScheduledAction(this, TaskCanvas::processTextSegment, desc );
}

void TaskCanvas::processButton(TaskCanvas * canvas, void * args)
{
	ButtonDescriptor * desc = (ButtonDescriptor *)args;

	uint32_t addr = (uint32_t)desc->text;
	if( addr == 0 )
		addr = (uint32_t)desc->map;

	canvas->Canvas::drawButton(desc);

	if( !IS_FLASH_ADDRESS(addr) )
		free((uint8_t *)addr);
}

void TaskCanvas::drawButton(ButtonDescriptor *btn)
{
	if( btn->map != 0 )
	{
		if( !IS_FLASH_ADDRESS((uint32_t)btn->map) )
		{
			unsigned size = ((btn->w + 8) >> 3) * (btn->h + 1)+2;

			uint8_t * map = (uint8_t *)malloc(size);
			memcpy(map, btn->map, size);
			btn->map = map;
		}
	}
	if( btn->text != 0 )
	{
		if( !IS_FLASH_ADDRESS((uint32_t)btn->text) )
		{
			char * txt = (char *)malloc(strlen(btn->text)+1);
			strcpy(txt, btn->text);
			btn->text = txt;
		}
	}

	new ScheduledAction(this, TaskCanvas::processButton, btn );
}

ColorBuffer * TaskCanvas::allocateColorBuffer(unsigned x, unsigned y, unsigned w, unsigned h, Angle angle)
{
	ColorBuffer * buffer = new ColorBuffer();
	buffer->x = x;
	buffer->y = y;
	buffer->w = w;
	buffer->h = h;
	buffer->angle = angle;

	unsigned num = (buffer->w+1) * (buffer->h+1);
	unsigned tsize = num * getColorSizeInBytes();
	unsigned winAddrLen = addWindowAddrScenario(0, buffer->x, buffer->y, buffer->w, buffer->h, buffer->angle);
	ScenarioPlayerTask * task = ScenarioPlayerTask::createScenarioPlayer( getColorSizeInBytes(), getDriverContext(), 12 + winAddrLen + tsize );
	uint8_t *buf = task->getScenario();
	uint8_t *ptr = buf;

	ptr += addWindowAddrScenario(ptr, buffer->x, buffer->y, buffer->w, buffer->h, buffer->angle);

	while( (((int)ptr) & 3 ) != 1 )
	{
		*ptr++ = SC_PAD;
	}
	*ptr++ = SC_SEND_BUFFER;
	memcpy(ptr, &num, sizeof(uint16_t));
	ptr += sizeof(uint16_t);

	buffer->buffer.data8 = ptr;
	ptr += tsize;
	*ptr++ = SC_DONE;

	buffer->deviceData = task;
	return buffer;
}

void TaskCanvas::fillColors(ColorBuffer * buffer)
{
	playScenario((ScenarioPlayerTask *)buffer->deviceData);
	delete buffer;
}

void TaskCanvas::floodColor(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color)
{
	unsigned winAddrLen = addWindowAddrScenario(0, x, y, w, h);
	ScenarioPlayerTask * task = ScenarioPlayerTask::createScenarioPlayer( getColorSizeInBytes(), getDriverContext(), 12 + winAddrLen );
	uint8_t *buf = task->getScenario();
	uint8_t *ptr = buf;
	uint32_t num = (h+1) * (w+1);

	ptr += addWindowAddrScenario(ptr, x, y, w, h);

	while( (((int)ptr) & 3 ) != 1 )
	{
		*ptr++ = SC_PAD;
	}
	*ptr++ = SC_FLOOD;
	memcpy(ptr, &num, sizeof(uint16_t));
	ptr += sizeof(uint16_t);
	color = rgb(color);
	memcpy(ptr, &color, sizeof(uint32_t));
	ptr += sizeof(uint32_t);
	*ptr++ = SC_DONE;

	playScenario(task);
}

uint32_t TaskCanvas::requiredResources()
{
	return getDriverContext()->getDriver()->requiredResources();
}
