#include "AmplifierWidget.h"
#include "graphics/Fonts.h"
#include "GainSetterTask.h"
#include <string.h>

#define AMPLIFIER_LOCATION_X 139
#define AMPLIFIER_LOCATION_Y  25
#define AMPLIFIER_SIZE_X      19
#define AMPLIFIER_SIZE_Y      16

void AmplifierWidget::draw(TaskCanvas * canvas)
{
	canvas->setFont(font_5x7);
	canvas->fillRect(AMPLIFIER_LOCATION_X, AMPLIFIER_LOCATION_Y, AMPLIFIER_SIZE_X, AMPLIFIER_SIZE_Y, COLOR_BLUE);
	canvas->drawRect(AMPLIFIER_LOCATION_X, AMPLIFIER_LOCATION_Y, AMPLIFIER_SIZE_X, AMPLIFIER_SIZE_Y, COLOR_WHITE);

	const char * name = getGainName(currentGain());
	int loc = AMPLIFIER_LOCATION_X + 3;
	if( strlen(name) < 3 )
		loc += 2;
	canvas->drawText(loc, AMPLIFIER_LOCATION_Y+5, name, COLOR_WHITE, COLOR_BLUE);
}
