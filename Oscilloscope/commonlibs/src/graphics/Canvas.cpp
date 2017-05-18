#include "graphics/Canvas.h"
#include <stdlib.h>
#include <string.h>

#define MAX_MEMORY_BUFFER 512
#define SEGMENT_SIZE       32

ColorBuffer * Canvas::allocateColorBuffer(unsigned x, unsigned y, unsigned w, unsigned h, Angle angle)
{
	ColorBuffer * buf = new ColorBuffer();
	buf->x = x;
	buf->y = y;
	buf->w = w;
	buf->h = h;
	buf->angle = angle;

	unsigned size = getColorSizeInBytes() * (w+1) * (h+1);
	buf->buffer.data8 = (uint8_t *)malloc(size);
	buf->deviceData = 0;
	return buf;
}

void Canvas::fillScreen(uint32_t color)
{
	unsigned sizex = getWidth();
	unsigned sizey = getHeight();

	floodColor(0, 0, sizex-1, sizey-1, color);
}

void Canvas::fillRect(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color)
{
	clipCoordinates(x, y, w, h);
	floodColor(x, y, w, h, color);
}

void Canvas::clipCoordinates(unsigned &x, unsigned &y, unsigned &w, unsigned &h)
{
	if( x >= getWidth() )
		x = getWidth() - 1;
	if( y >= getHeight() )
		y = getHeight() - 1;

	if( x + w >= getWidth() )
		w = getWidth() - 1 - x;
	if( y + h >= getHeight() )
		h = getHeight() - 1 - y;
}

void Canvas::drawHLine(unsigned x, unsigned y, unsigned w, uint32_t color)
{
	unsigned h = 0;
	clipCoordinates(x, y, w, h);
	floodColor(x, y, w, h, color);
}

void Canvas::drawVLine(unsigned x, unsigned y, unsigned h, uint32_t color)
{
	unsigned w = 0;
	clipCoordinates(x, y, w, h);
	floodColor(x, y, w, h, color);
}

void Canvas::drawPixel(unsigned x, unsigned y, uint32_t color)
{
	unsigned w = 0, h = 0;
	clipCoordinates(x, y, w, h);
	floodColor(x, y, 0, 0, color);
}

void Canvas::drawRect(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color)
{
	drawHLine(x,y,w,color);
	drawHLine(x,y+h,w,color);
	drawVLine(x,y,h,color);
	drawVLine(x+w,y,h,color);
}

void Canvas::drawBitmapSegments(BitmapSegmenterDescriptor * desc)
{
	for(unsigned i=0; i < desc->segments; i++)
		drawBitmapSegment(desc, desc->segmenterData+i);
	free(desc);
}

void Canvas::drawBitmapSegment(BitmapSegmenterDescriptor * desc, BitmapSegmenterData * data)
{
	unsigned w = desc->w;
	unsigned h = data->h;

	ColorBuffer * buffer = allocateColorBuffer(data->x, data->y, w, h, desc->angle);
	uint8_t * colors = buffer->buffer.data8;

	unsigned pos = 0;
	unsigned rowWidth = (w + 8) >> 3;
	for(unsigned y=0; y <= h; y++ )
	{
		const uint8_t *mapLoc = desc->map + data->pos + (y * rowWidth);
		for(unsigned x=0; x <= w; x++ )
		{
			uint8_t offs = x >> 3;
			uint8_t msk = 128 >> (x&7);
			if( mapLoc[offs] & msk )
				memcpy( colors+pos, &desc->color_fore, getColorSizeInBytes() );
			else
				memcpy( colors+pos, &desc->color_bck, getColorSizeInBytes() );
			pos += getColorSizeInBytes();
		}
	}

	fillColors(buffer);
}

void Canvas::drawBitmap(unsigned x, unsigned y, unsigned w, unsigned h, const uint8_t *map, uint32_t color_fore, uint32_t color_bck, Angle angle)
{
	clipCoordinates(x, y, w, h);
	if( w == 0 || h == 0 )
		return;

	uint8_t * segmentBuf = (uint8_t *)malloc( sizeof(BitmapSegmenterDescriptor) + SEGMENT_SIZE );
	uint8_t * segmentEnd = segmentBuf + sizeof(BitmapSegmenterDescriptor) + SEGMENT_SIZE;

	BitmapSegmenterDescriptor * desc = (BitmapSegmenterDescriptor *)segmentBuf;
	desc->angle = angle;
	desc->w = w;
	desc->color_bck = rgb(color_bck);
	desc->color_fore = rgb(color_fore);
	desc->segments = 0;
	desc->map = map;
	desc->userData = 0;

	unsigned linesPerSegment = MAX_MEMORY_BUFFER / getColorSizeInBytes() / (w+1);
	unsigned rowWidth = (w + 8) >> 3;

	desc->size = rowWidth * (h+1);

	color_fore = rgb(color_fore);
	color_bck = rgb(color_bck);

	unsigned cline = 0;
	while( cline <= h )
	{
		unsigned newh = h-cline;
		if( newh >= linesPerSegment )
			newh = linesPerSegment - 1;

		if( (uint8_t *)&desc->segmenterData[desc->segments + 1] >= segmentEnd )
		{
			unsigned size = segmentEnd - segmentBuf + SEGMENT_SIZE;
			segmentBuf = (uint8_t *)realloc(segmentBuf, size);
			segmentEnd = segmentBuf + size;
		}

		unsigned ptr = desc->segments;
		desc->segmenterData[ptr].x = x;
		desc->segmenterData[ptr].y = y;
		desc->segmenterData[ptr].h = newh;
		desc->segmenterData[ptr].pos = map - desc->map;
		desc->segments++;

		newh++;
		switch(angle)
		{
		case ANGLE_270:
			x += newh;
			break;
		case ANGLE_180:
			y -= newh;
			break;
		case ANGLE_90:
			x -= newh;
			break;
		case ANGLE_0:
		default:
			y += newh;
		}
		map += (newh) * rowWidth;
		cline += newh;
	}

	drawBitmapSegments(desc);
}

const uint8_t hun_chars [] = "ÁÉÍÓÖŐÚÜŰáéíóöőúüűµ";

uint8_t Canvas::parseUTF8(const char ** text)
{
	uint8_t code = (uint8_t)**text;
	if( code == 0 )
		return code; // end of text

	(*text)++;

	if (code <= 0x7F) /* standard ascii */
	{
		return code;
	}
	else
	{
		uint8_t code2 = (uint8_t)**text;
		if( code2 == 0 )
			return code2;

		(*text)++;

		uint8_t retcode = 127;

		for(uint8_t i=0; hun_chars[i] != 0; i+=2, retcode++)
		{
			if(( hun_chars[i] == code ) && ( hun_chars[i+1] == code2 ))
				return retcode;
		}

		return 0;
	}
}

void Canvas::drawTextSegments(TextSegmenterDescriptor * desc)
{
	for(unsigned i=0; i < desc->segments; i++)
		drawTextSegment(desc, desc->segmenterData+i);
	free(desc);
}

unsigned Canvas::textWidth(const char * text)
{
	unsigned fontWidth = font[0];
	unsigned fontHeight = font[1];
	unsigned fontSize = ((fontWidth + 7)>>3) * fontHeight + 1;

	unsigned width = 0;
	const char * ptr = text;

	while(1)
	{
		uint8_t chr = parseUTF8(&ptr);
		if( chr == 0 )
			break;
		uint8_t fontw = font[2 + (chr - 32) * fontSize];

		width += fontw;
	}

	return width;
}

void Canvas::drawTextSegment(TextSegmenterDescriptor * desc, TextSegmenterData * data )
{
	unsigned fontWidth = desc->font[0];
	unsigned fontHeight = desc->font[1];
	unsigned fontSize = ((fontWidth + 7)>>3) * fontHeight + 1;

	const char * ptr = desc->text + data->ptr;
	const char * endptr = ptr + data->len;
	unsigned width = 0;

	while( ptr < endptr )
	{
		uint8_t chr = parseUTF8(&ptr);
		uint8_t fontw = desc->font[2 + (chr - 32) * fontSize];

		width += fontw;
	}

	unsigned rowSize = width * getColorSizeInBytes();
	ColorBuffer * buffer = allocateColorBuffer(data->x, data->y, width-1, fontHeight-1, desc->angle);
	uint8_t * colors = buffer->buffer.data8;

	ptr = desc->text + data->ptr;
	width = 0;
	while( ptr < endptr )
	{
		uint8_t chr = parseUTF8(&ptr);
		uint8_t fontw = desc->font[2 + (chr - 32) * fontSize];

		const uint8_t * fntDesc = desc->font + 3 + (chr - 32) * fontSize;
		uint8_t fntDelta = (fontWidth + 7) >> 3;
		for(uint8_t y=0; y < fontHeight; y++)
		{
			uint8_t * rowst = colors + rowSize * y + width* getColorSizeInBytes();
			uint8_t csize = getColorSizeInBytes();
			for(uint8_t x=0; x < fontw; x++, rowst += csize)
			{
				uint8_t ndx = x >> 3;
				uint8_t msk = (128 >> (x & 7));
				if( fntDesc[ndx] & msk )
					memcpy(rowst, &desc->color_fore, csize);
				else
					memcpy(rowst, &desc->color_bck, csize);
			}

			fntDesc += fntDelta;
		}
		width += fontw;
	}

	fillColors(buffer);
}

void Canvas::drawText(unsigned x, unsigned y, const char * text, uint32_t color_fore, uint32_t color_bck, Angle angle )
{
	uint8_t * segmentBuf = (uint8_t *)malloc( sizeof(TextSegmenterDescriptor) + SEGMENT_SIZE );
	uint8_t * segmentEnd = segmentBuf + sizeof(TextSegmenterDescriptor) + SEGMENT_SIZE;

	TextSegmenterDescriptor * desc = (TextSegmenterDescriptor *)segmentBuf;
	desc->angle = angle;
	desc->font = font;
	desc->color_bck = rgb(color_bck);
	desc->color_fore = rgb(color_fore);
	desc->segments = 0;
	desc->text = text;
	desc->userData = 0;

	unsigned fontWidth = font[0];
	unsigned fontHeight = font[1];
	unsigned fontSize = ((fontWidth + 7)>>3) * fontHeight + 1;

	unsigned requiredMemory = 0;
	unsigned renderedWidth = 0;

	unsigned xo = x, yo = y;

	const char * segment = text;
	const char * lastChar = segment;

	while(1)
	{
		uint8_t chr = parseUTF8(&segment);
		unsigned sz = 0;

		// check if last segment is to send
		bool sendSegment = false;
		if( chr >= ' ' )
		{
			uint8_t fontw = font[2 + (chr - 32) * fontSize];
			sz = fontw * fontHeight * getColorSizeInBytes();
			if( sz + requiredMemory >= MAX_MEMORY_BUFFER )
				sendSegment = true;
		}
		else
			sendSegment = true;

		if( sendSegment )
		{
			if( lastChar != text )
			{
				if( (uint8_t *)&desc->segmenterData[desc->segments + 1] >= segmentEnd )
				{
					unsigned size = segmentEnd - segmentBuf + SEGMENT_SIZE;
					segmentBuf = (uint8_t *)realloc(segmentBuf, size);
					segmentEnd = segmentBuf + size;
				}

				unsigned ptr = desc->segments;
				desc->segmenterData[ptr].x = x;
				desc->segmenterData[ptr].y = y;
				desc->segmenterData[ptr].ptr = text - desc->text;
				desc->segmenterData[ptr].len = lastChar - text;
				desc->segments++;
			}

			if( chr >= 32)
				text = lastChar;
			else
				text = segment;

			switch(angle)
			{
			case ANGLE_270:
				y -= renderedWidth;
				break;
			case ANGLE_180:
				x -= renderedWidth;
				break;
			case ANGLE_90:
				y += renderedWidth;
				break;
			case ANGLE_0:
			default:
				x += renderedWidth;
			}

			requiredMemory = 0;
			renderedWidth = 0;
		}
		if( chr >= ' ' )
		{
			uint8_t fontw = font[2 + (chr - 32) * fontSize];
			requiredMemory += sz;
			renderedWidth += fontw;
		}
		else if( chr == '\n' )
		{
			switch(angle)
			{
			case ANGLE_270:
				y = yo;
				x += fontHeight;
				break;
			case ANGLE_180:
				x = xo;
				y -= fontHeight;
				break;
			case ANGLE_90:
				y = yo;
				x -= fontHeight;
				break;
			case ANGLE_0:
			default:
				x = xo;
				y += fontHeight;
				break;
			}
		}
		else if( chr == 0 )
			break;
		lastChar = segment;
	}

	drawTextSegments(desc);
}

void Canvas::drawImage(unsigned x, unsigned y, const uint8_t *map, uint32_t color_fore, uint32_t color_bck, Angle angle)
{
	unsigned w = *map++;
	unsigned h = *map++;
	drawBitmap(x, y, w, h, map, color_fore, color_bck, angle);
}

ColorSetter Canvas::getColorSetter()
{
	uint8_t colorSize = getColorSizeInBytes();
	switch(colorSize)
	{
	case 1:
		return [] (uint8_t * buffer, unsigned index, uint32_t color) {
			buffer[index] = (uint8_t)color;
		};
	case 2:
		return [] (uint8_t * buffer, unsigned index, uint32_t color) {
			((uint16_t *)buffer)[index] = (uint16_t)color;
		};
	case 4:
		return [] (uint8_t * buffer, unsigned index, uint32_t color) {
			((uint32_t *)buffer)[index] = color;
		};
	}
	return 0;
}

ColorFiller Canvas::getColorFiller()
{
	uint8_t colorSize = getColorSizeInBytes();
	switch(colorSize)
	{
	case 1:
		return [] (uint8_t * buffer, unsigned index, unsigned num, uint32_t color) {
			memset(buffer + index, (uint8_t)color, num);
		};
		break;
	case 2:
		return [] (uint8_t * buffer, unsigned index, unsigned num, uint32_t color) {
			uint16_t * ptr = ((uint16_t *)buffer) + index;
			for(unsigned i=0; i < num; i++)
				*ptr++ = (uint16_t)color;
		};
		break;
	case 4:
		return [] (uint8_t * buffer, unsigned index, unsigned num, uint32_t color) {
			uint32_t * ptr = ((uint32_t *)buffer) + index;
			for(unsigned i=0; i < num; i++)
				*ptr++ = color;
		};
		break;
	}
	return 0;
}

void Canvas::drawButton(ButtonDescriptor *btn)
{
	drawRect(btn->x, btn->y, btn->w, btn->h, btn->color_frame);
	fillRect(btn->x+1, btn->y+1, btn->w-2, btn->h-2, btn->color_bck);
	drawHLine(btn->x+1, btn->y+btn->h+1, btn->w, btn->color_sunken);
	drawVLine(btn->x+btn->w+1, btn->y+1, btn->h, btn->color_sunken);

	if( btn->map != 0 )
	{
		int centerx = (btn->w - btn->map[0]) / 2;
		int centery = (btn->h - btn->map[1]) / 2;
		drawImage(btn->x+centerx, btn->y+centery, btn->map, btn->color_fore, btn->color_bck);
	}
	else if( btn->text != 0 )
	{
		setFont(btn->font);
		int centerx = (btn->w - textWidth(btn->text) + 1) / 2;
		int centery = (btn->h - font[1] + 1) / 2;
		drawText(btn->x+centerx, btn->y+centery, btn->text, btn->color_fore, btn->color_bck);
	}
	free(btn);
}

void Canvas::drawButtonCommon(unsigned x, unsigned y, const uint8_t *map, const char * text, uint32_t color_fore, uint32_t color_bck, uint32_t color_frame, uint32_t color_sunken, int w, int h)
{
	ButtonDescriptor * btn = new ButtonDescriptor();
	btn->x = x;
	btn->y = y;
	btn->w = w;
	btn->h = h;

	btn->text = text;
	btn->font = font;
	btn->map = map;

	btn->color_bck = color_bck;
	btn->color_fore = color_fore;
	btn->color_frame = color_frame;
	btn->color_sunken = color_sunken;

	drawButton(btn);

}

void Canvas::drawImageButton(unsigned x, unsigned y, const uint8_t *map, uint32_t color_fore, uint32_t color_bck, uint32_t color_frame, uint32_t color_sunken, int w, int h)
{
	if( w == 0 )
		w = map[0] + 4;

	if( h == 0 )
		h = map[1] + 4;

	drawButtonCommon(x, y, map, 0, color_fore, color_bck, color_frame, color_sunken, w, h);
}


void Canvas::drawTextButton(unsigned x, unsigned y, const char *text, uint32_t color_fore, uint32_t color_bck, uint32_t color_frame, uint32_t color_sunken, int w, int h)
{
	if( w == 0 )
		w = textWidth(text) + 3;

	if( h == 0 )
		h = font[1] + 3;

	drawButtonCommon(x, y, 0, text, color_fore, color_bck, color_frame, color_sunken, w, h);
}

