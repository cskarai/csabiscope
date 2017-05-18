#include "OscilloscopeScreen.h"
#include "OscilloscopeGrid.h"
#include "graphics/Fonts.h"
#include "task/TaskHandler.h"
#include "MeasurementValueWidget.h"
#include "AmplifierWidget.h"
#include "TriggerWidget.h"
#include "Colors.h"

#define CHANNEL_HEIGHT         7
#define CHANNEL_WIDTH         14
#define CHANNEL_1_X            0
#define CHANNEL_1_Y            0
#define CHANNEL_2_X            0
#define CHANNEL_2_Y            8

#define LOGO_X                54
#define LOGO_Y               120

const uint8_t image_interval_x [] = {
	12,
	4,
	0b01000000, 0b01000000,
	0b11111111, 0b11100000,
	0b01000100, 0b01000000,
	0b00000100, 0b00000000,
	0b00000111, 0b11111000,
};

const uint8_t image_interval_y [] = {
	5,
	13,
	0b10000000,
	0b10000000,
	0b10000000,
	0b10010000,
	0b10111000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b00010000,
	0b00010000,
	0b00111000,
	0b00010000,
};

const uint8_t image_min_value [] = {
	5,
	4,
	0b11111000,
	0b00001000,
	0b00001000,
	0b00011100,
	0b00001000,
};

const uint8_t image_max_value [] = {
		5,
		4,
		0b00001000,
		0b00011100,
		0b00001000,
		0b00001000,
		0b11111000,
};

OscilloscopeScreen::OscilloscopeScreen(MeasurementData *dataIn) : data(dataIn)
{
}

void OscilloscopeScreen::draw(TaskCanvas * canvas)
{
	// clear the screen
	canvas->fillScreen(COLOR_BLACK);

	// draw the channels
	canvas->setFont(font_5x7);
	canvas->fillRect(CHANNEL_1_X, CHANNEL_1_Y, CHANNEL_WIDTH, CHANNEL_HEIGHT, OSCILLOSCOPE_COLOR_CHANNEL_1_TEXT);
	canvas->fillRect(CHANNEL_2_X, CHANNEL_2_Y, CHANNEL_WIDTH, CHANNEL_HEIGHT, OSCILLOSCOPE_COLOR_CHANNEL_2_TEXT);
	canvas->drawText(CHANNEL_1_X + 1, CHANNEL_1_Y + 1, "Cs1", COLOR_BLACK, OSCILLOSCOPE_COLOR_CHANNEL_1_TEXT);
	canvas->drawText(CHANNEL_2_X + 1, CHANNEL_2_Y + 1, "Cs2", COLOR_BLACK, OSCILLOSCOPE_COLOR_CHANNEL_2_TEXT);

	canvas->drawText(LOGO_X, LOGO_Y, " Csabiszkóp ", OSCILLOSCOPE_COLOR_LOGO, OSCILLOSCOPE_COLOR_LOGO_BACKGROUND);

	// draw arrows for oscilloscope grid
	canvas->drawImage(GRID_X, GRID_Y + 102, image_interval_x, COLOR_WHITE, COLOR_BLACK);
	canvas->drawImage(GRID_X-5, GRID_Y+87, image_interval_y, COLOR_WHITE, COLOR_BLACK);
	canvas->drawImage(GRID_X+121, GRID_Y, image_min_value, COLOR_WHITE, COLOR_BLACK);
	canvas->drawImage(GRID_X+121, GRID_Y+96, image_max_value, COLOR_WHITE, COLOR_BLACK);

	MeasurementValueWidget * measurementWidget = new MeasurementValueWidget(data);
	scheduleWidget<MeasurementValueWidget>(canvas, measurementWidget);

	AmplifierWidget * amplifierWidget = new AmplifierWidget();
	scheduleWidget<AmplifierWidget>(canvas, amplifierWidget);

	TriggerWidget * triggerWidget = new TriggerWidget(data);
	scheduleWidget<TriggerWidget>(canvas, triggerWidget);

	OscilloscopeGrid *osc = new OscilloscopeGrid(canvas, data, GRID_X, GRID_Y);
	TaskHandler::getInstance().scheduleTask(osc);
}
