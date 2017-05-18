#include "TriggerWidget.h"
#include "graphics/Fonts.h"
#include "Colors.h"
#include "xprintf.h"
#include "Config.h"
#include "Sampler.h"
#include <string.h>

#define TRIGGER_LOCATION_X 139
#define TRIGGER_LOCATION_Y  53

const uint8_t bmp_trigger_none [] = {
	12,
	8,
	0b00000000,0b00000000,
	0b00000000,0b00000000,
	0b00000000,0b00000000,
	0b00000000,0b00000000,
	0b00011111,0b10000000,
	0b00000000,0b00000000,
	0b00000000,0b00000000,
	0b00000000,0b00000000,
	0b00000000,0b00000000,
};

const uint8_t bmp_trigger_rising_edge [] = {
	12,
	8,
	0b00000011,0b11000000,
	0b00000010,0b00000000,
	0b00000010,0b00000000,
	0b00000010,0b00000000,
	0b00000111,0b00000000,
	0b00001111,0b10000000,
	0b00000010,0b00000000,
	0b00000010,0b00000000,
	0b00011110,0b00000000,
};

const uint8_t bmp_trigger_falling_edge [] = {
	12,
	8,
	0b00011110,0b00000000,
	0b00000010,0b00000000,
	0b00000010,0b00000000,
	0b00000010,0b00000000,
	0b00001111,0b10000000,
	0b00000111,0b00000000,
	0b00000010,0b00000000,
	0b00000010,0b00000000,
	0b00000011,0b11000000,
};

const uint8_t bmp_trigger_changing_edge [] = {
	12,
	8,
	0b00000001,0b11000000,
	0b00000000,0b11000000,
	0b00000001,0b01000000,
	0b00000010,0b00000000,
	0b01111100,0b00000000,
	0b00000010,0b00000000,
	0b00000001,0b01000000,
	0b00000000,0b11000000,
	0b00000001,0b11000000,
};

const uint8_t bmp_trigger_positive_peak [] = {
	12,
	8,
	0b00011111,0b11000000,
	0b00010000,0b01000000,
	0b00010000,0b01000000,
	0b00010000,0b01000000,
	0b00111000,0b01000000,
	0b01111100,0b01000000,
	0b00010000,0b01000000,
	0b00010000,0b01000000,
	0b11110000,0b01111000,
};

const uint8_t bmp_trigger_negative_peak [] = {
	12,
	8,
	0b11110000,0b01111000,
	0b00010000,0b01000000,
	0b00010000,0b01000000,
	0b01111100,0b01000000,
	0b00111000,0b01000000,
	0b00010000,0b01000000,
	0b00010000,0b01000000,
	0b00010000,0b01000000,
	0b00011111,0b11000000,
};

const uint8_t bmp_trigger_external [] = {
	12,
	8,
	0b00000000,0b00100000,
	0b01111100,0b01000000,
	0b01000000,0b10000000,
	0b01000001,0b00000000,
	0b01001010,0b00100000,
	0b01001100,0b00100000,
	0b01001110,0b00100000,
	0b01000000,0b00100000,
	0b01111111,0b11100000,
};


const uint8_t bmp_trigger_none_fft [] = {
	12,
	8,
	0b00000000,0b00000000,
	0b00000000,0b00000000,
	0b01110111,0b01110000,
	0b01000100,0b00100000,
	0b01100110,0b00100000,
	0b01000100,0b00100000,
	0b01000100,0b00100000,
	0b00000000,0b00000000,
	0b00000000,0b00000000,
};

TriggerWidget::TriggerWidget(MeasurementData *dataIn) :
	data(dataIn)
{
}

void TriggerWidget::draw(TaskCanvas *canvas)
{
	canvas->setFont(font_5x7);

	canvas->fillRect(TRIGGER_LOCATION_X, TRIGGER_LOCATION_Y, 19, 53, COLOR_BLUE);

	bool singleShotWait = (config.getTriggerType() & TRIGGER_SINGLESHOT) && !Sampler::getInstance()->isSingleShotTriggerEnabled();
	if( singleShotWait )
		canvas->fillRect(TRIGGER_LOCATION_X, TRIGGER_LOCATION_Y+15, 19, 12, COLOR_ORANGE);

	uint32_t trigColor = singleShotWait ? COLOR_ORANGE : COLOR_BLUE;

	canvas->drawHLine(TRIGGER_LOCATION_X, TRIGGER_LOCATION_Y+14, 19, COLOR_WHITE);
	canvas->drawHLine(TRIGGER_LOCATION_X, TRIGGER_LOCATION_Y+27, 19, COLOR_WHITE);
	canvas->drawHLine(TRIGGER_LOCATION_X, TRIGGER_LOCATION_Y+40, 19, COLOR_WHITE);
	canvas->drawRect(TRIGGER_LOCATION_X, TRIGGER_LOCATION_Y, 19, 53, COLOR_WHITE);

	switch( config.getTriggerType() & TRIGGER_TYPE_MASK )
	{
	case TRIGGER_NONE:
		canvas->drawImage(TRIGGER_LOCATION_X+4, TRIGGER_LOCATION_Y+3, bmp_trigger_none, COLOR_PURPLE, COLOR_BLUE);
		break;
	case TRIGGER_RISING_EDGE:
		canvas->drawImage(TRIGGER_LOCATION_X+4, TRIGGER_LOCATION_Y+3, bmp_trigger_rising_edge, COLOR_PURPLE, COLOR_BLUE);
		break;
	case TRIGGER_FALLING_EDGE:
		canvas->drawImage(TRIGGER_LOCATION_X+4, TRIGGER_LOCATION_Y+3, bmp_trigger_falling_edge, COLOR_PURPLE, COLOR_BLUE);
		break;
	case TRIGGER_CHANGING_EDGE:
		canvas->drawImage(TRIGGER_LOCATION_X+4, TRIGGER_LOCATION_Y+3, bmp_trigger_changing_edge, COLOR_PURPLE, COLOR_BLUE);
		break;
	case TRIGGER_POSITIVE_PEAK:
		canvas->drawImage(TRIGGER_LOCATION_X+4, TRIGGER_LOCATION_Y+3, bmp_trigger_positive_peak, COLOR_PURPLE, COLOR_BLUE);
		break;
	case TRIGGER_NEGATIVE_PEAK:
		canvas->drawImage(TRIGGER_LOCATION_X+4, TRIGGER_LOCATION_Y+3, bmp_trigger_negative_peak, COLOR_PURPLE, COLOR_BLUE);
		break;
	case TRIGGER_EXTERNAL:
		canvas->drawImage(TRIGGER_LOCATION_X+4, TRIGGER_LOCATION_Y+3, bmp_trigger_external, COLOR_PURPLE, COLOR_BLUE);
		break;
	case TRIGGER_NONE_FFT:
		canvas->drawImage(TRIGGER_LOCATION_X+4, TRIGGER_LOCATION_Y+3, bmp_trigger_none_fft, COLOR_PURPLE, COLOR_BLUE);
		break;
	default:
		break;
	}

	if( config.getTriggerType() & TRIGGER_SINGLESHOT )
		canvas->drawText(TRIGGER_LOCATION_X+5, TRIGGER_LOCATION_Y+18, "1x", COLOR_WHITE, trigColor);
	else
		canvas->drawText(TRIGGER_LOCATION_X+8, TRIGGER_LOCATION_Y+20, "~", COLOR_WHITE, COLOR_BLUE);

	char buffer[10];
	xsprintf(buffer, "%d%%", config.getTriggerPercent());
	int loc = TRIGGER_LOCATION_X+3;
	if( strlen(buffer) < 3 )
		loc += 2;
	canvas->drawText(loc, TRIGGER_LOCATION_Y+31, buffer, COLOR_WHITE, COLOR_BLUE);

	xsprintf(buffer, "Cs%d", config.getTriggerChannel() + 1);
	canvas->drawText(TRIGGER_LOCATION_X+3, TRIGGER_LOCATION_Y+44, buffer, OSCILLOSCOPE_COLOR_CHANNEL_1_TEXT, COLOR_BLUE);
}

