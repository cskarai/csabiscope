#include "SetupMenu.h"
#include "task/TaskHandler.h"
#include "graphics/Fonts.h"
#include "systick/SysTime.h"
#include "Sampler.h"
#include "TriggerWidget.h"
#include "USBHandler.h"
#include <string.h>

#define CLOSE_BUTTON_ID  0
#define SAVE_BUTTON_ID   1
#define BUTTON_ID_OFFSET 2

#define FOCUS_Y_THRESHOLD   10
#define Y_OFFSET            25

#define FONT_WIDTH           5
#define FONT_HEIGHT          7

#define MAX_TEST_FREQUENCY 500000
#define MAX_CALIBRATION    500000

Value SetupMenu::channel1Avg;
Value SetupMenu::channel2Avg;

extern const Menu closeButton;
extern const Menu saveButton;

const uint8_t bmp_close [] = {
	6,
	8,
	0b00000000,
	0b10000010,
	0b01000100,
	0b00101000,
	0b00010000,
	0b00101000,
	0b01000100,
	0b10000010,
	0b00000000,
};

const uint8_t bmp_save [] = {
	8,
	8,
	0b11111111,0b10000000,
	0b10100010,0b10000000,
	0b10100010,0b10000000,
	0b10100010,0b10000000,
	0b10111110,0b10000000,
	0b10000000,0b10000000,
	0b10000000,0b10000000,
	0b10000000,0b10000000,
	0b11111111,0b10000000,
};

void processDrawingTasks();

SetupMenu * currentSetupMenu = 0;

void SetupMenu::showMenu( const Menu * menu, const Menu * autoClickOn )
{
	InputDeviceCallback oldInputCallback = inputDeviceHandler.getInputDeviceCallback();
	RefreshCallback oldRefreshCallback = Sampler::getInstance()->getRefreshCallback();
	SetupMenu * oldSetupMenu = currentSetupMenu;
	currentSetupMenu = this;

	Sampler::getInstance()->setRefreshCallback( [] ( RefreshType type ) {
		if( type == REFRESH_MEASUREMENT_VALUE )
		{
			channel1Avg = Sampler::getInstance()->getMeasurementData().getAverage(0);
			channel2Avg = Sampler::getInstance()->getMeasurementData().getAverage(1);
		}
	});
	inputDeviceHandler.setInputDeviceCallback([](InputDeviceEvent event) {
		currentSetupMenu->handleInputDeviceEvent(event);
	});

	drawMenuPage(menu);

	uint64_t lastTime = SysTime::getMillis();

	SubMenu * submenu = (SubMenu *)(menu->descriptor);

	if( submenu->callback ) submenu->callback(CALLBACKTYPE_START);

	if( autoClickOn != 0 )
		clickSubMenu(autoClickOn);

	while(!exiting)
	{
		TaskHandler::getInstance().loop();
		Sampler::getInstance()->loop();
		inputDeviceHandler.loop();
		if( SysTime::getMillis() - lastTime > 500 )
		{
			lastTime = SysTime::getMillis();
			redrawValues();
		}
		if( submenu->callback ) submenu->callback(CALLBACKTYPE_LOOP);
	}

	if( submenu->callback ) submenu->callback(CALLBACKTYPE_EXIT);

	processDrawingTasks();
	currentSetupMenu = oldSetupMenu;
	inputDeviceHandler.setInputDeviceCallback(oldInputCallback);
	Sampler::getInstance()->setRefreshCallback(oldRefreshCallback);
}

void SetupMenu::close()
{
	exiting = true;
}

void SetupMenu::save()
{
	Config newConfig;
	Config::loadConfig(&newConfig);

	for( unsigned i=0; i < numberOfItems; i++ )
		saveMenuItem(&newConfig, menuItems[i].menu);

	Config::saveConfig(&newConfig);
}

void SetupMenu::saveMenuItem(Config * newConfig, const Menu * menu)
{
	switch(menu->type)
	{
	case MENUTYPE_SUBMENU:
		{
			SubMenu * subMenu = (SubMenu *)(menu->descriptor);
			for(unsigned i=0; i < subMenu->number; i++)
				saveMenuItem(newConfig, subMenu->submenus[i]);
		}
		break;
	case MENUTYPE_RADIO:
	case MENUTYPE_RADIO_IMAGE:
		{
			const RadioMenu * radioMenu = (const RadioMenu *)(menu->descriptor);
			// copy value from config
			unsigned value = (config.*(radioMenu->getValue))();
			(newConfig->*(radioMenu->setValue))(value);
		}
		break;
	case MENUTYPE_TOGGLE:
		{
			const ToggleMenu * toggleMenu = (const ToggleMenu *)(menu->descriptor);
			// copy value from config
			bool value = (config.*(toggleMenu->getValue))();
			(newConfig->*(toggleMenu->setValue))(value);
		}
		break;
	case MENUTYPE_VALUE:
		{
			const ValueMenu * valueMenu = (const ValueMenu *)(menu->descriptor);
			unsigned value = (config.*(valueMenu->getValue))();
			(newConfig->*(valueMenu->setValue))(value);
		}
		break;
	case MENUTYPE_VALUE_SAVER:
		{
			const ValueSaverMenu * valueSaverMenu = (const ValueSaverMenu *)(menu->descriptor);
			valueSaverMenu->save(newConfig);
		}
		break;
	default:
		break;
	}
}

void SetupMenu::handleInputDeviceEvent(InputDeviceEvent inputDeviceEvent)
{
	if( focusedItem == 0xFFFFFFFF )
		return;

	MenuItemDescriptor * currentItem = menuItems + focusedItem;
	const Menu * currentMenu = currentItem->menu;

	switch(inputDeviceEvent)
	{
	case JOY_REPEAT_FIRE:
		// enable repeat for <<, <, >, >>
		if( currentMenu->type != MENUTYPE_TEXT_BUTTON )
			break;
	case JOY_FIRE:
		if( currentMenu->type != MENUTYPE_TOGGLE && currentMenu->type != MENUTYPE_RADIO && currentMenu->type != MENUTYPE_RADIO_IMAGE )
			animateClick( focusedItem );
		handlePress( focusedItem );
		break;
	case JOY_REPEAT_RIGHT:
	case JOY_REPEAT_LEFT:
	case JOY_REPEAT_UP:
	case JOY_REPEAT_DOWN:
	case JOY_RIGHT:
	case JOY_LEFT:
	case JOY_UP:
	case JOY_DOWN:
		calculateFocus(inputDeviceEvent);
		break;
	case USB_COMMAND_START_TRANSMISSION:
		if( currentlyDisplayedMenu == &usbTransmitMenu )
			clickSubMenu(&usbStartTransmission);
		else
			showMenu(&usbTransmitMenu, &usbStartTransmission);
		break;
	default:
		break;
	}
}

bool SetupMenu::isFocusable(MenuType type)
{
	switch(type)
	{
	case MENUTYPE_LABEL:
	case MENUTYPE_DYNAMIC_LABEL:
	case MENUTYPE_VALUE:
	case MENUTYPE_VALUE_SAVER:
	case MENUTYPE_SEPARATOR_LINE:
		return false;
	default:
		return true;
	}
}

void SetupMenu::calculateFocus(InputDeviceEvent event)
{
	uint32_t error_focused = 0xFFFFFFFF;
	unsigned focused = focusedItem;

	for(uint8_t i=0; i < numberOfItems; i++ )
	{
		if( i == focusedItem )
			continue;
		if( !isFocusable( menuItems[i].menu->type ) )
			continue;

		uint32_t error = 0xFFFFFFFF;

		int16_t deltay = (int16_t)menuItems[i].y - (int16_t)menuItems[focusedItem].y;
		if( deltay < 0 )
			deltay = -deltay;
		int16_t deltax = (int16_t)menuItems[i].x - (int16_t)menuItems[focusedItem].x;
		if( deltax < 0 )
			deltax = -deltax;

		switch( event )
		{
		case JOY_REPEAT_RIGHT:
		case JOY_RIGHT:
			if( menuItems[i].x > menuItems[focusedItem].x )
			{
				if( deltay < FOCUS_Y_THRESHOLD )
					error = menuItems[i].x - menuItems[focusedItem].x;
			}
			break;
		case JOY_REPEAT_LEFT:
		case JOY_LEFT:
			if( menuItems[i].x < menuItems[focusedItem].x )
			{
				if( deltay < FOCUS_Y_THRESHOLD )
					error = menuItems[focusedItem].x - menuItems[i].x;
			}
			break;
		case JOY_REPEAT_UP:
		case JOY_UP:
			if( menuItems[i].y < menuItems[focusedItem].y )
			{
				if( deltay >= FOCUS_Y_THRESHOLD )
					error = ((uint32_t)menuItems[focusedItem].y - menuItems[i].y) * 1024 + deltax;
			}
			break;
		case JOY_REPEAT_DOWN:
		case JOY_DOWN:
			if( menuItems[i].y > menuItems[focusedItem].y )
			{
				if( deltay >= FOCUS_Y_THRESHOLD )
					error = ((uint32_t)menuItems[i].y - menuItems[focusedItem].y) * 1024 + deltax;
			}
			break;
		default:
			break;
		}

		if( error < error_focused )
		{
			focused = i;
			error_focused = error;
		}
	}
	moveFocus(focused);
}


void SetupMenu::handlePress( unsigned ndx )
{
	MenuItemDescriptor * pressedItem = menuItems + ndx;
	const Menu * pressedMenu = pressedItem->menu;

	switch( pressedMenu->type )
	{
	case MENUTYPE_TEXT_BUTTON:
	case MENUTYPE_IMAGE_BUTTON:
		{
			PushButtonMenu * pbmenu = ((PushButtonMenu *)pressedMenu->descriptor);
			pbmenu->callback( this, pressedMenu, ndx );

			redrawValues();
		}
		break;
	case MENUTYPE_SUBMENU:
		{
			const Menu * menu = currentlyDisplayedMenu;
			showMenu(pressedItem->menu);
			drawMenuPage(menu);
		}
		break;
	case MENUTYPE_RADIO:
	case MENUTYPE_RADIO_IMAGE:
		{
			const RadioMenu * radioMenu = (const RadioMenu *)(pressedMenu->descriptor);
			(config.*(radioMenu->setValue))((unsigned)radioMenu->radioValue);

			redrawRadioGroup( radioMenu->radioId );
		}
		break;
	case MENUTYPE_TOGGLE:
		{
			const ToggleMenu * toggleMenu = (const ToggleMenu *)(pressedMenu->descriptor);
			(config.*(toggleMenu->setValue))( !(config.*(toggleMenu->getValue))() );

			drawMenuItem(ndx);
		}
		break;
	case MENUTYPE_LABEL:
	case MENUTYPE_SEPARATOR_LINE:
	case MENUTYPE_DYNAMIC_LABEL:
	case MENUTYPE_VALUE:
	case MENUTYPE_VALUE_SAVER:
	default:
		break;
	}
}

void SetupMenu::animateClick( unsigned ndx )
{
	clickedItem = ndx;
	drawMenuItem(ndx);
	processDrawingTasks();
	delay(200);
	clickedItem = 0xFFFFFFFF;
	drawMenuItem(ndx);
	processDrawingTasks();
}

void SetupMenu::clickSubMenu( const Menu * menu )
{
	unsigned ndx = getMenuIndex(menu);
	if( ndx != 0 )
	{
		moveFocus(ndx);
		animateClick(ndx);
		handlePress( focusedItem );
	}
}

void SetupMenu::delay(unsigned ms)
{
	uint64_t endtime = SysTime::getMillis() + ms;

	while( SysTime::getMillis() < endtime )
	{
		TaskHandler::getInstance().loop();
		Sampler::getInstance()->loop();
		inputDeviceHandler.loop();
	}
}

void SetupMenu::drawMenuPage( const Menu * menu )
{
	processDrawingTasks();

	currentlyDisplayedMenu = menu;

	numberOfItems = 0;
	focusedItem = clickedItem = 0xFFFFFFFF;
	exiting = false;

	SubMenu * subMenu = (SubMenu *)(menu->descriptor);

	canvas->fillScreen( COLOR_BLACK );
	canvas->setFont(font_6x10);
	canvas->drawText(0, 3, subMenu->headerText, SETUPMENU_COLOR_TITLE, COLOR_BLACK);

	canvas->drawHLine(0, 17, canvas->getWidth(), SETUPMENU_COLOR_LINE);

	registerMenuItem(&closeButton);
	registerMenuItem(&saveButton);

	for(unsigned i=0; i < subMenu->number; i++ )
		registerMenuItem( subMenu->submenus[i]);

	drawAll();
	moveFocus(CLOSE_BUTTON_ID);
}

void SetupMenu::moveFocus(unsigned id)
{
	if( focusedItem != 0xFFFFFFFF )
	{
		MenuItemDescriptor * last = menuItems+focusedItem;
		canvas->drawRect(last->x, last->y, last->w, last->h, SETUPMENU_COLOR_MENU_FRAME);
		canvas->drawHLine(last->x + 1, last->y + last->h + 1, last->w, SETUPMENU_COLOR_SUNKEN_BUTTON);
		canvas->drawVLine(last->x + last->w + 1, last->y + 1, last->h, SETUPMENU_COLOR_SUNKEN_BUTTON);
	}

	focusedItem = id;
	MenuItemDescriptor * focused = menuItems+focusedItem;
	canvas->drawRect(focused->x, focused->y, focused->w, focused->h, SETUPMENU_COLOR_FOCUSED_MENU_FRAME);
	canvas->drawHLine(focused->x + 1, focused->y + focused->h + 1, focused->w, SETUPMENU_COLOR_FOCUSED_SUNKEN_BUTTON);
	canvas->drawVLine(focused->x + focused->w + 1, focused->y + 1, focused->h, SETUPMENU_COLOR_FOCUSED_SUNKEN_BUTTON);
}

void SetupMenu::drawAll()
{
	for( unsigned i=0; i < numberOfItems; i++ )
	{
		drawMenuItem( i );
		// this is required for memory issues
		if( (i % 5) == 4 )
			processDrawingTasks();
	}
	processDrawingTasks();
}

void SetupMenu::redrawRadioGroup(short id)
{
	for( unsigned i=0; i < numberOfItems; i++ )
	{
		if( ( menuItems[i].menu->type == MENUTYPE_RADIO ) || ( menuItems[i].menu->type == MENUTYPE_RADIO_IMAGE ) )
		{
			const RadioMenu * radioMenu = (const RadioMenu *)(menuItems[i].menu->descriptor);
			if( radioMenu->radioId == id )
				drawMenuItem(i);
		}
	}
}

void SetupMenu::redrawValues()
{
	for( unsigned i=0; i < numberOfItems; i++ )
	{
		if(( menuItems[i].menu->type == MENUTYPE_VALUE ) || ( menuItems[i].menu->type == MENUTYPE_DYNAMIC_LABEL ) )
			drawMenuItem(i);
	}
}

unsigned SetupMenu::registerMenuItem(const Menu * menu)
{
	unsigned ndx = numberOfItems++;
	menuItems[ndx].menu = menu;
	menuItems[ndx].x = menu->x;
	menuItems[ndx].y = menu->y + Y_OFFSET;
	menuItems[ndx].w = menu->width;

	switch(menu->type)
	{
	case MENUTYPE_IMAGE_BUTTON:
	case MENUTYPE_RADIO_IMAGE:
		if( menu->width == 0 )
			menuItems[ndx].w = (uint8_t)menu->title[0] + 4;
		menuItems[ndx].h = (uint8_t)menu->title[1] + 4;
		break;
	case MENUTYPE_SUBMENU:
	case MENUTYPE_RADIO:
	case MENUTYPE_TOGGLE:
	case MENUTYPE_TEXT_BUTTON:
		if( menu->width == 0 )
		{
			canvas->setFont(font_5x7);
			menuItems[ndx].w = canvas->textWidth(menu->title) + 3;
		}
		menuItems[ndx].h = FONT_HEIGHT + 3;
		break;
	case MENUTYPE_LABEL:
	case MENUTYPE_DYNAMIC_LABEL:
	case MENUTYPE_VALUE:
	case MENUTYPE_VALUE_SAVER:
	case MENUTYPE_SEPARATOR_LINE:
	default:
		break;
	}

	return ndx;
}

void SetupMenu::drawMenuItem( unsigned ndx )
{
	const Menu * menu = menuItems[ndx].menu;

	uint32_t frame_color = (ndx == focusedItem) ? SETUPMENU_COLOR_FOCUSED_MENU_FRAME : SETUPMENU_COLOR_MENU_FRAME;
	uint32_t sunken_color = (ndx == focusedItem) ? SETUPMENU_COLOR_FOCUSED_SUNKEN_BUTTON : SETUPMENU_COLOR_SUNKEN_BUTTON;
	uint32_t clicked_color = (ndx == clickedItem) ? SETUPMENU_COLOR_TOGGLED_MENU : SETUPMENU_COLOR_MENU;

	switch(menu->type)
	{
	case MENUTYPE_IMAGE_BUTTON:
		canvas->drawImageButton(menu->x, menu->y + Y_OFFSET, (const uint8_t *)menu->title, SETUPMENU_COLOR_TEXT,
				(ndx == clickedItem) ? SETUPMENU_COLOR_TOGGLED_MENU : ((PushButtonMenu *)menu->descriptor)->color,
				frame_color, sunken_color, menu->width);
		break;
	case MENUTYPE_TEXT_BUTTON:
		canvas->drawTextButton(menu->x, menu->y + Y_OFFSET, menu->title, SETUPMENU_COLOR_TEXT,
				(ndx == clickedItem) ? SETUPMENU_COLOR_TOGGLED_MENU : ((PushButtonMenu *)menu->descriptor)->color,
				frame_color, sunken_color, menu->width);
		break;
	case MENUTYPE_SUBMENU:
		canvas->setFont( font_5x7 );
		canvas->drawTextButton(menu->x, menu->y + Y_OFFSET, menu->title, SETUPMENU_COLOR_TEXT,
				clicked_color, frame_color, sunken_color, menu->width);
		break;
	case MENUTYPE_LABEL:
		canvas->setFont( font_5x7 );
		canvas->drawText(menu->x, menu->y + Y_OFFSET, menu->title, SETUPMENU_COLOR_LABEL, COLOR_BLACK);
		break;
	case MENUTYPE_DYNAMIC_LABEL:
		{
			uint32_t color = SETUPMENU_COLOR_DYNAMIC_LABEL;
			char buffer[40];
			const DynamicLabelMenu * dynmenu = (const DynamicLabelMenu *)(menu->descriptor);
			dynmenu->getLabel(buffer, sizeof(buffer), &color);

			if( menu->width > 0 )
			{
				while((short)strlen(buffer) < menu->width )
					strcat(buffer, " ");
			}

			canvas->setFont( font_5x7 );
			canvas->drawText(menu->x, menu->y + Y_OFFSET, buffer, color, COLOR_BLACK);
		}
		break;
	case MENUTYPE_RADIO:
	case MENUTYPE_RADIO_IMAGE:
		{
			const RadioMenu * radioMenu = (const RadioMenu *)(menu->descriptor);
			unsigned value = (config.*(radioMenu->getValue))();

			uint32_t color = ( value == (unsigned)radioMenu->radioValue) ? SETUPMENU_COLOR_TOGGLED_MENU : SETUPMENU_COLOR_MENU;

			if( menu->type == MENUTYPE_RADIO )
			{
				canvas->setFont( font_5x7 );
				canvas->drawTextButton(menu->x, menu->y + Y_OFFSET, menu->title, SETUPMENU_COLOR_TEXT,
						color, frame_color, sunken_color, menu->width);
			}
			else
			{
				canvas->drawImageButton(menu->x, menu->y + Y_OFFSET, (const uint8_t *)menu->title, SETUPMENU_COLOR_TEXT,
						color, frame_color, sunken_color, menu->width);
			}
		}
		break;
	case MENUTYPE_TOGGLE:
		{
			const ToggleMenu * toggleMenu = (const ToggleMenu *)(menu->descriptor);
			bool value = (config.*(toggleMenu->getValue))();

			uint32_t color = value ? SETUPMENU_COLOR_TOGGLED_MENU : SETUPMENU_COLOR_MENU;

			canvas->setFont( font_5x7 );
			canvas->drawTextButton(menu->x, menu->y + Y_OFFSET, menu->title, SETUPMENU_COLOR_TEXT,
					color, frame_color, sunken_color, menu->width);
		}
		break;
	case MENUTYPE_VALUE:
		{
			const ValueMenu * valueMenu = (const ValueMenu *)(menu->descriptor);
			unsigned raw_value = (config.*(valueMenu->getValue))();
			Value value = (*(valueMenu->converter))(raw_value);
			const char * str = valueToString(value, valueMenu->preferInteger);
			int prespace = 0;
			int postspace = 0;
			if( (int)strlen(str) < menu->width )
			{
				int diff = menu->width - strlen(str);
				prespace = diff / 2;
				postspace = menu->width - strlen(str) - prespace;
			}

			char buf[20];
			for(int i=0; i < prespace; i++)
				buf[i] = ' ';
			buf[prespace] = 0;

			strcpy(buf+prespace, str);

			while(postspace--)
				strcat(buf, " ");

			canvas->setFont( font_5x7 );
			canvas->drawText(menu->x, menu->y + Y_OFFSET, buf, SETUPMENU_COLOR_VALUE, COLOR_BLACK);
		}
		break;
	case MENUTYPE_VALUE_SAVER:
		break;
	case MENUTYPE_SEPARATOR_LINE:
		canvas->drawHLine(0, menu->y + Y_OFFSET, canvas->getWidth(), SETUPMENU_COLOR_LINE);
		break;
	default:
		break;
	}
}

int SetupMenu::getMultiplier(unsigned freq, unsigned base)
{
	if( (int)freq < 0 )
		freq = 1;

	unsigned multiplier = 1;
	if( freq >= 1000000 )
	{
		multiplier = 1000000;
	}
	else if( freq >= 100000 )
	{
		multiplier = 100000;
	}
	else if( freq >= 10000 )
	{
		multiplier = 10000;
	}
	else if( freq >= 1000 )
	{
		multiplier = 1000;
	}
	else if( freq >= 100 )
	{
		multiplier = 100;
	}
	else if( freq >= 10 )
	{
		multiplier = 10;
	}

	multiplier /= base;
	if( multiplier == 0 )
		multiplier = 1;
	return multiplier;
}

void SetupMenu::adjustTestFrequency(int amount)
{
	int freq = (int)config.getTestSignalFrequency();
	int multiplier = (int)SetupMenu::getMultiplier(freq, 10);

	if( SetupMenu::getMultiplier(freq + multiplier * amount, 10) != multiplier )
	{
		if( amount > 0 )
			freq = multiplier * 10 * amount;
		else
			freq = multiplier * 10 + multiplier * amount / 10;
	}
	else
		freq += multiplier * amount;

	if( freq < 1 )
		freq = 1;
	if( freq > MAX_TEST_FREQUENCY )
		freq = MAX_TEST_FREQUENCY;

	config.setTestSignalFrequency(freq);
}

void SetupMenu::adjustCalibration(int channel, int amount)
{
	int calib = (int)config.getCalibrationValue(channel);
	int multiplier = (int)SetupMenu::getMultiplier(calib, 100);

	if( SetupMenu::getMultiplier(calib + multiplier * amount, 100) != multiplier )
	{
		if( amount > 0 )
			calib = multiplier * 100 * amount;
		else
			calib = multiplier * 100 + multiplier * amount / 100;
	}
	else
		calib += multiplier * amount;

	if( calib < 1 )
		calib = 1;
	if( calib > MAX_CALIBRATION )
		calib = MAX_CALIBRATION;

	config.setCalibrationValue(channel, calib);
}

void SetupMenu::calculateRange(int channel, char * buf, int /* maxLen */ )
{
	MeasurementData &data = Sampler::getInstance()->getMeasurementData();
	Value vmin = data.adcToVoltage(0, channel);
	Value vmax = data.adcToVoltage(0xFFF, channel);

	strcpy(buf, valueToString(vmin));
	strcat(buf, " - ");
	strcat(buf, valueToString(vmax));
}

unsigned SetupMenu::getMenuIndex(const Menu * menu)
{
	for(unsigned i=0; i < numberOfItems; i++)
	{
		if( menuItems[i].menu == menu )
			return i;
	}

	return 0;
}

uint64_t lastTimeStamp;
unsigned itemIndex;

void SetupMenu::usbTransmit(unsigned index)
{
	lastTimeStamp = SysTime::getMillis();
	itemIndex = index;


	usbHandler.transmit( []() {
		TaskHandler::getInstance().loop();
		if( SysTime::getMillis() - lastTimeStamp > 500 )
		{
			lastTimeStamp = SysTime::getMillis();
			currentSetupMenu->drawMenuItem(itemIndex);
		}
	});

	if( usbHandler.getTransmissionState() == STATE_USB_ABORT )
		close();
}

#define RADIO_CHANNEL_NUM_ID       1
#define RADIO_CHANNEL_AMPL_ID      2
#define RADIO_TRIGGER_TYPE_ID      3
#define RADIO_TRIGGER_CHANNEL_ID   4
#define RADIO_FREQUENCY_CHANNEL_ID 5
#define RADIO_CUSTOM_METER_ID      6
#define RADIO_UPDATE_FREQUENCY_ID  7
#define RADIO_DIVIDER_HEAD_ID      8
#define RADIO_TRANSMIT_BITS_ID     9
#define RADIO_FFT_ID              10

DECLARE_LABEL(channelSelectionLabel, 0, 0, "Csatornák száma:");

DECLARE_RADIO_BUTTON(channelNumRadio1,  0, 12, 64, "1 csatorna", RADIO_CHANNEL_NUM_ID,  1, &Config::getNumberOfChannels, &Config::setNumberOfChannels);
DECLARE_RADIO_BUTTON(channelNumRadio2, 70, 12, 64, "2 csatorna", RADIO_CHANNEL_NUM_ID,  2, &Config::getNumberOfChannels, &Config::setNumberOfChannels);

DECLARE_LABEL(channelAmplificationLabel, 0, 30, "Erősítés:");

DECLARE_RADIO_BUTTON(channelAmplRadio1X,   0, 42, 0, "1x",  RADIO_CHANNEL_AMPL_ID,  (short)GAIN_1X, &Config::getCurrentGainInt, &Config::setCurrentGainInt);
DECLARE_RADIO_BUTTON(channelAmplRadio2X,  18, 42, 0, "2x",  RADIO_CHANNEL_AMPL_ID,  (short)GAIN_2X, &Config::getCurrentGainInt, &Config::setCurrentGainInt);
DECLARE_RADIO_BUTTON(channelAmplRadio4X,  36, 42, 0, "4x",  RADIO_CHANNEL_AMPL_ID,  (short)GAIN_4X, &Config::getCurrentGainInt, &Config::setCurrentGainInt);
DECLARE_RADIO_BUTTON(channelAmplRadio5X,  54, 42, 0, "5x",  RADIO_CHANNEL_AMPL_ID,  (short)GAIN_5X, &Config::getCurrentGainInt, &Config::setCurrentGainInt);
DECLARE_RADIO_BUTTON(channelAmplRadio8X,  72, 42, 0, "8x",  RADIO_CHANNEL_AMPL_ID,  (short)GAIN_8X, &Config::getCurrentGainInt, &Config::setCurrentGainInt);
DECLARE_RADIO_BUTTON(channelAmplRadio10X, 90, 42, 0, "10x", RADIO_CHANNEL_AMPL_ID, (short)GAIN_10X, &Config::getCurrentGainInt, &Config::setCurrentGainInt);
DECLARE_RADIO_BUTTON(channelAmplRadio16X,113, 42, 0, "16x", RADIO_CHANNEL_AMPL_ID, (short)GAIN_16X, &Config::getCurrentGainInt, &Config::setCurrentGainInt);
DECLARE_RADIO_BUTTON(channelAmplRadio32X,136, 42, 0, "32x", RADIO_CHANNEL_AMPL_ID, (short)GAIN_32X, &Config::getCurrentGainInt, &Config::setCurrentGainInt);

DECLARE_LABEL(channelSamplingFrequencyLabel, 0, 60, "Mintavételi frekvencia:");

DECLARE_VALUE(channelSamplingFrequency, 55, 74, 10, &Config::getSamplingInterval,
		&Config::setSamplingInterval, [](unsigned value) -> Value {
	int freq = (int)((uint64_t)2560000000 / (uint64_t)value);

	return scaleValue(freq, ValueScale::NONE, ValueUnit::FREQUENCY);
});

DECLARE_TEXT_BUTTON(channelSamplingRewind, 45, 71, 0, "<", [](SetupMenu *, const Menu *, unsigned){
	Sampler::getInstance()->decreaseSamplingRate();
});

DECLARE_TEXT_BUTTON(channelSamplingFastRewind, 27, 71, 0, "<<", [](SetupMenu *, const Menu *, unsigned){
	for(int i=0; i < 5; i++)
		Sampler::getInstance()->decreaseSamplingRate();
});

DECLARE_TEXT_BUTTON(channelSamplingForward, 105, 71, 0, ">", [](SetupMenu *, const Menu *, unsigned){
	Sampler::getInstance()->increaseSamplingRate();
});

DECLARE_TEXT_BUTTON(channelSamplingFastForward, 118, 71, 0, ">>", [](SetupMenu *, const Menu *, unsigned){
	for(int i=0; i < 5; i++)
		Sampler::getInstance()->increaseSamplingRate();
});

DECLARE_TOGGLE(channelAutoZoom, 0, 90, 0, "Automatikus ránagyítás", &Config::isAutoZoom, &Config::setAutoZoom);

const Menu * const channelMenuTable[] = {
		&channelSelectionLabel,
		&channelNumRadio1,
		&channelNumRadio2,
		&channelAmplificationLabel,
		&channelAmplRadio1X,
		&channelAmplRadio2X,
		&channelAmplRadio4X,
		&channelAmplRadio5X,
		&channelAmplRadio8X,
		&channelAmplRadio10X,
		&channelAmplRadio16X,
		&channelAmplRadio32X,
		&channelSamplingFrequencyLabel,
		&channelSamplingFrequency,
		&channelSamplingFastRewind,
		&channelSamplingRewind,
		&channelSamplingForward,
		&channelSamplingFastForward,
		&channelAutoZoom,
};

DECLARE_SUB_MENU(channelMenu, 0, 0, 55, "Csatorna", "Csatorna beállítása", channelMenuTable);

DECLARE_IMAGE_BUTTON(closeButton, 146, 1 - Y_OFFSET, 12, bmp_close, SETUPMENU_COLOR_CLOSE_BUTTON,  [](SetupMenu * currentMenu, const Menu *, unsigned){
	currentMenu->close();
});

DECLARE_IMAGE_BUTTON(saveButton, 128, 1 - Y_OFFSET, 12, bmp_save, SETUPMENU_COLOR_SAVE_BUTTON,  [](SetupMenu * currentMenu, const Menu *, unsigned){
	currentMenu->save();
});

DECLARE_LABEL(triggerTypeLabel, 0, 0, "A trigger típusa:");

DECLARE_RADIO_IMAGE(triggerTypeRadio1,  0, 12, 0, bmp_trigger_none,          RADIO_TRIGGER_TYPE_ID,  (short)TRIGGER_NONE,          &Config::getTriggerTypeInt, &Config::setTriggerTypeInt);
DECLARE_RADIO_IMAGE(triggerTypeRadio2, 20, 12, 0, bmp_trigger_rising_edge,   RADIO_TRIGGER_TYPE_ID,  (short)TRIGGER_RISING_EDGE,   &Config::getTriggerTypeInt, &Config::setTriggerTypeInt);
DECLARE_RADIO_IMAGE(triggerTypeRadio3, 40, 12, 0, bmp_trigger_falling_edge,  RADIO_TRIGGER_TYPE_ID,  (short)TRIGGER_FALLING_EDGE,  &Config::getTriggerTypeInt, &Config::setTriggerTypeInt);
DECLARE_RADIO_IMAGE(triggerTypeRadio4, 60, 12, 0, bmp_trigger_changing_edge, RADIO_TRIGGER_TYPE_ID,  (short)TRIGGER_CHANGING_EDGE, &Config::getTriggerTypeInt, &Config::setTriggerTypeInt);
DECLARE_RADIO_IMAGE(triggerTypeRadio5, 80, 12, 0, bmp_trigger_positive_peak, RADIO_TRIGGER_TYPE_ID,  (short)TRIGGER_POSITIVE_PEAK, &Config::getTriggerTypeInt, &Config::setTriggerTypeInt);
DECLARE_RADIO_IMAGE(triggerTypeRadio6,100, 12, 0, bmp_trigger_negative_peak, RADIO_TRIGGER_TYPE_ID,  (short)TRIGGER_NEGATIVE_PEAK, &Config::getTriggerTypeInt, &Config::setTriggerTypeInt);
DECLARE_RADIO_IMAGE(triggerTypeRadio7,120, 12, 0, bmp_trigger_external,      RADIO_TRIGGER_TYPE_ID,  (short)TRIGGER_EXTERNAL,      &Config::getTriggerTypeInt, &Config::setTriggerTypeInt);
DECLARE_RADIO_IMAGE(triggerTypeRadio8,140, 12, 0, bmp_trigger_none_fft,      RADIO_TRIGGER_TYPE_ID,  (short)TRIGGER_NONE_FFT,      &Config::getTriggerTypeInt, &Config::setTriggerTypeInt);

DECLARE_TOGGLE(triggerTypeSingleShot, 0, 30, 0, "Egy lövéses", &Config::isTriggerSingleShot, &Config::setTriggerSingleShot);

DECLARE_LABEL(triggerChannelLabel, 0, 46, "Trigger csatorna:");
DECLARE_RADIO_BUTTON(triggerChannelRadio1,  0, 57, 70, "1-es csatorna", RADIO_TRIGGER_CHANNEL_ID,  0, &Config::getTriggerChannel, &Config::setTriggerChannel);
DECLARE_RADIO_BUTTON(triggerChannelRadio2, 80, 57, 70, "2-es csatorna", RADIO_TRIGGER_CHANNEL_ID,  1, &Config::getTriggerChannel, &Config::setTriggerChannel);

DECLARE_LABEL(triggerStrength, 0, 75, "Nagysága:");

DECLARE_VALUE_INT(triggerPercent, 29, 88, 3, &Config::getTriggerPercent,
		&Config::setTriggerPercent, [](unsigned value) -> Value {

	return scaleValue(256 * value, ValueScale::NONE, ValueUnit::PERCENT);
});

DECLARE_TEXT_BUTTON(triggerPercentRewind, 16, 85, 0, "<", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTriggerPercent() > 1 )
		config.setTriggerPercent( config.getTriggerPercent() - 1 );
});

DECLARE_TEXT_BUTTON(triggerPercentFastRewind, 0, 85, 0, "<<", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTriggerPercent() > 11 )
		config.setTriggerPercent( config.getTriggerPercent() - 10 );
	else
		config.setTriggerPercent( 1 );
});

DECLARE_TEXT_BUTTON(triggerPercentForward, 46, 85, 0, ">", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTriggerPercent() < 99 )
		config.setTriggerPercent( config.getTriggerPercent() + 1 );
});

DECLARE_TEXT_BUTTON(triggerPercentFastForward, 57, 85, 0, ">>", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTriggerPercent() < 90 )
		config.setTriggerPercent( config.getTriggerPercent() + 10 );
	else
		config.setTriggerPercent( 99 );
});

DECLARE_LABEL(triggerOffsetLabel, 80, 75, "Eltolása:");

DECLARE_VALUE_INT(triggerOffset, 109, 88, 3, &Config::getTriggerOffset,
		&Config::setTriggerOffset, [](unsigned value) -> Value {

	return scaleValue(256 * value, ValueScale::NONE, ValueUnit::NONE);
});

DECLARE_TEXT_BUTTON(triggerOffsetRewind, 96, 85, 0, "<", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTriggerOffset() > 0 )
		config.setTriggerOffset( config.getTriggerOffset() - 1 );
});

DECLARE_TEXT_BUTTON(triggerOffsetFastRewind, 80, 85, 0, "<<", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTriggerOffset() > 10 )
		config.setTriggerOffset( config.getTriggerOffset() - 10 );
	else
		config.setTriggerOffset( 0 );
});

DECLARE_TEXT_BUTTON(triggerOffsetForward, 126, 85, 0, ">", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTriggerOffset() < 120 )
		config.setTriggerOffset( config.getTriggerOffset() + 1 );
});

DECLARE_TEXT_BUTTON(triggerOffsetFastForward, 137, 85, 0, ">>", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTriggerOffset() < 110 )
		config.setTriggerOffset( config.getTriggerOffset() + 10 );
	else
		config.setTriggerOffset( 120 );
});

const Menu * const triggerMenuTable[] = {
	&triggerTypeLabel,
	&triggerTypeRadio1,
	&triggerTypeRadio2,
	&triggerTypeRadio3,
	&triggerTypeRadio4,
	&triggerTypeRadio5,
	&triggerTypeRadio6,
	&triggerTypeRadio7,
	&triggerTypeRadio8,
	&triggerTypeSingleShot,
	&triggerChannelLabel,
	&triggerChannelRadio1,
	&triggerChannelRadio2,
	&triggerStrength,
	&triggerPercent,
	&triggerPercentFastRewind,
	&triggerPercentRewind,
	&triggerPercentForward,
	&triggerPercentFastForward,
	&triggerOffsetLabel,
	&triggerOffset,
	&triggerOffsetFastRewind,
	&triggerOffsetRewind,
	&triggerOffsetForward,
	&triggerOffsetFastForward,
};

DECLARE_SUB_MENU(triggerMenu, 61, 0, 50, "Trigger", "Trigger beállítása", triggerMenuTable);

DECLARE_LABEL(testLabel, 0, 0, "A teszt jel engedélyezése");
DECLARE_LABEL(testLabel2, 0, 12, "megnövelheti a zajszintet.");

DECLARE_TOGGLE(testEnable, 0, 24, 0, "Engedélyezés", &Config::isTestSignalEnabled, &Config::setTestSignalEnabled);

DECLARE_LABEL(testFrequencyLabel, 0, 50, "Frekvencia:");

DECLARE_VALUE(testFrequency, 29, 63, 8, &Config::getTestSignalFrequency,
		&Config::setTestSignalFrequency, [](unsigned value) -> Value {

	return scaleValue(256 * value, ValueScale::NONE, ValueUnit::FREQUENCY);
});

DECLARE_TEXT_BUTTON(testFrequencyRewind, 16, 60, 0, "<", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustTestFrequency(-1);
});

DECLARE_TEXT_BUTTON(testFrequencyFastRewind, 0, 60, 0, "<<", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustTestFrequency(-10);
});

DECLARE_TEXT_BUTTON(testFrequencyForward, 71, 60, 0, ">", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustTestFrequency(1);
});

DECLARE_TEXT_BUTTON(testFrequencyFastForward, 82, 60, 0, ">>", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustTestFrequency(10);
});

DECLARE_LABEL(testDutyLabel, 0, 79, "Kitöltési tényező:");

DECLARE_VALUE_INT(testDutyPercent, 29, 92, 3, &Config::getTestSignalDuty,
		&Config::setTestSignalDuty, [](unsigned value) -> Value {

	return scaleValue(256 * value, ValueScale::NONE, ValueUnit::PERCENT);
});

DECLARE_TEXT_BUTTON(testDutyPercentRewind, 16, 89, 0, "<", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTestSignalDuty() > 1 )
		config.setTestSignalDuty( config.getTestSignalDuty() - 1 );
});

DECLARE_TEXT_BUTTON(testDutyPercentFastRewind, 0, 89, 0, "<<", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTestSignalDuty() > 11 )
		config.setTestSignalDuty( config.getTestSignalDuty() - 10 );
	else
		config.setTestSignalDuty( 1 );
});

DECLARE_TEXT_BUTTON(testDutyPercentForward, 46, 89, 0, ">", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTestSignalDuty() < 99 )
		config.setTestSignalDuty( config.getTestSignalDuty() + 1 );
});

DECLARE_TEXT_BUTTON(testDutyPercentFastForward, 57, 89, 0, ">>", [](SetupMenu *, const Menu *, unsigned){
	if( config.getTestSignalDuty() < 90 )
		config.setTestSignalDuty( config.getTestSignalDuty() + 10 );
	else
		config.setTestSignalDuty( 99 );
});

const Menu * const testMenuTable[] = {
	&testLabel,
	&testLabel2,
	&testEnable,
	&testFrequencyLabel,
	&testFrequency,
	&testFrequencyFastRewind,
	&testFrequencyRewind,
	&testFrequencyForward,
	&testFrequencyFastForward,
	&testDutyLabel,
	&testDutyPercent,
	&testDutyPercentFastRewind,
	&testDutyPercentFastRewind,
	&testDutyPercentRewind,
	&testDutyPercentForward,
	&testDutyPercentFastForward,
};

DECLARE_SUB_MENU(testMenu, 118, 0, 40, "Teszt", "Teszt jel beállítása", testMenuTable);

DECLARE_LABEL(frequencyMeasuringChannelLabel, 0, 0, "Frekvencia-mérő csatorna:");
DECLARE_RADIO_BUTTON(frequencyMeasuringChannelLabelRadio1,  0, 10, 70, "1-es csatorna", RADIO_FREQUENCY_CHANNEL_ID,  0, &Config::getFrequencyMeasuringChannel, &Config::setFrequencyMeasuringChannel);
DECLARE_RADIO_BUTTON(frequencyMeasuringChannelLabelRadio2, 80, 10, 70, "2-es csatorna", RADIO_FREQUENCY_CHANNEL_ID,  1, &Config::getFrequencyMeasuringChannel, &Config::setFrequencyMeasuringChannel);

DECLARE_LABEL(customMeterLabel, 0, 27, "Választható mérő:");
DECLARE_RADIO_BUTTON(customMeterRadio1,  0, 37, 0, "Effektív érték", RADIO_CUSTOM_METER_ID, (short)EffectiveValue, &Config::getCustomMeterInt, &Config::setCustomMeterInt);
DECLARE_RADIO_BUTTON(customMeterRadio2,  0, 50, 0, "Referencia hiba", RADIO_CUSTOM_METER_ID, (short)ChannelError, &Config::getCustomMeterInt, &Config::setCustomMeterInt);
DECLARE_RADIO_BUTTON(customMeterRadio3,  0, 63, 0, "Tápfeszültség hiba", RADIO_CUSTOM_METER_ID, (short)PowerError, &Config::getCustomMeterInt, &Config::setCustomMeterInt);

DECLARE_LABEL(updateFrequencyLabel, 0, 80, "Képfrissítés gyakorisága:");
DECLARE_RADIO_BUTTON(updateFrequencyRadio1,   0, 90, 35, "250ms",  RADIO_UPDATE_FREQUENCY_ID,  250, &Config::getOscilloscopeUpdateFrequency, &Config::setOscilloscopeUpdateFrequency);
DECLARE_RADIO_BUTTON(updateFrequencyRadio2,  40, 90, 35, "500ms",  RADIO_UPDATE_FREQUENCY_ID,  500, &Config::getOscilloscopeUpdateFrequency, &Config::setOscilloscopeUpdateFrequency);
DECLARE_RADIO_BUTTON(updateFrequencyRadio3,  80, 90, 35, "1000ms", RADIO_UPDATE_FREQUENCY_ID, 1000, &Config::getOscilloscopeUpdateFrequency, &Config::setOscilloscopeUpdateFrequency);
DECLARE_RADIO_BUTTON(updateFrequencyRadio4, 120, 90, 35, "2000ms", RADIO_UPDATE_FREQUENCY_ID, 2000, &Config::getOscilloscopeUpdateFrequency, &Config::setOscilloscopeUpdateFrequency);

const Menu * const oscilloscopeMenuTable[] = {
	&frequencyMeasuringChannelLabel,
	&frequencyMeasuringChannelLabelRadio1,
	&frequencyMeasuringChannelLabelRadio2,
	&customMeterLabel,
	&customMeterRadio1,
	&customMeterRadio2,
	&customMeterRadio3,
	&updateFrequencyLabel,
	&updateFrequencyRadio1,
	&updateFrequencyRadio2,
	&updateFrequencyRadio3,
	&updateFrequencyRadio4,
};

DECLARE_SUB_MENU(oscilloscopeMenu, 0, 17, 80, "Oszcilloszkóp", "Oszcilloszkóp menü", oscilloscopeMenuTable);

DECLARE_LABEL(dividerHeadLabel, 0, 0, "Feszültségosztó fej:");

DECLARE_RADIO_BUTTON(dividerHeadRadio1,   0, 11, 35, "Fej 1",  RADIO_DIVIDER_HEAD_ID,  0, &Config::getCurrentProbeHead, &Config::setCurrentProbeHead);
DECLARE_RADIO_BUTTON(dividerHeadRadio2,  40, 11, 35, "Fej 2",  RADIO_DIVIDER_HEAD_ID,  1, &Config::getCurrentProbeHead, &Config::setCurrentProbeHead);
DECLARE_RADIO_BUTTON(dividerHeadRadio3,  80, 11, 35, "Fej 3",  RADIO_DIVIDER_HEAD_ID,  2, &Config::getCurrentProbeHead, &Config::setCurrentProbeHead);

DECLARE_LABEL(calibrateChannel1Label, 0, 30, "1. csatorna:");

DECLARE_DYNAMIC_LABEL(calibrateChannel1Range, 65, 30, 17, [](char * buf, int maxLen, uint32_t *) {
	SetupMenu::calculateRange(0, buf, maxLen);
});

DECLARE_VALUE(calibrateChannel1Value, 49, 43, 8, &Config::getCalibrationValue1,
		&Config::setCalibrationValue1, [](unsigned) -> Value {
	return SetupMenu::channel1Avg;
});

DECLARE_TEXT_BUTTON(calibrateChannel1VeryFastRewind, 0, 41, 0, "<<<", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(0, -100);
});

DECLARE_TEXT_BUTTON(calibrateChannel1FastRewind, 22, 41, 0, "<<", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(0, -10);
});

DECLARE_TEXT_BUTTON(calibrateChannel1Rewind, 39, 41, 0, "<", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(0, -1);
});

DECLARE_TEXT_BUTTON(calibrateChannel1Forward, 91, 41, 0, ">", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(0, 1);
});

DECLARE_TEXT_BUTTON(calibrateChannel1FastForward, 103, 41, 0, ">>", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(0, 10);
});

DECLARE_TEXT_BUTTON(calibrateChannel1VeryFastForward, 120, 41, 0, ">>>", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(0, 100);
});

DECLARE_LABEL(calibrateChannel2Label, 0, 62, "2. csatorna:");

DECLARE_DYNAMIC_LABEL(calibrateChannel2Range, 65, 62, 17, [](char * buf, int maxLen, uint32_t *) {
	SetupMenu::calculateRange(1, buf, maxLen);
});

DECLARE_VALUE(calibrateChannel2Value, 49, 75, 8, &Config::getCalibrationValue2,
		&Config::setCalibrationValue2, [](unsigned) -> Value {
	return SetupMenu::channel2Avg;
});

DECLARE_TEXT_BUTTON(calibrateChannel2VeryFastRewind, 0, 73, 0, "<<<", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(1, -100);
});

DECLARE_TEXT_BUTTON(calibrateChannel2FastRewind, 22, 73, 0, "<<", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(1, -10);
});

DECLARE_TEXT_BUTTON(calibrateChannel2Rewind, 39, 73, 0, "<", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(1, -1);
});

DECLARE_TEXT_BUTTON(calibrateChannel2Forward, 91, 73, 0, ">", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(1, 1);
});

DECLARE_TEXT_BUTTON(calibrateChannel2FastForward, 103, 73, 0, ">>", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(1, 10);
});

DECLARE_TEXT_BUTTON(calibrateChannel2VeryFastForward, 120, 73, 0, ">>>", [](SetupMenu *, const Menu *, unsigned){
	SetupMenu::adjustCalibration(1, 100);
});

DECLARE_VALUE_SAVER(calibrationSaver, [](Config * newConfig){
	newConfig->setCalibrationValueProbe(0, 0, config.getCalibrationValueProbe(0, 0) );
	newConfig->setCalibrationValueProbe(0, 1, config.getCalibrationValueProbe(0, 1) );
	newConfig->setCalibrationValueProbe(0, 2, config.getCalibrationValueProbe(0, 2) );
	newConfig->setCalibrationValueProbe(1, 0, config.getCalibrationValueProbe(1, 0) );
	newConfig->setCalibrationValueProbe(1, 1, config.getCalibrationValueProbe(1, 1) );
	newConfig->setCalibrationValueProbe(1, 2, config.getCalibrationValueProbe(1, 2) );
});

const Menu * const calibrationMenuTable[] = {
	&dividerHeadLabel,
	&dividerHeadRadio1,
	&dividerHeadRadio2,
	&dividerHeadRadio3,
	&calibrateChannel1Label,
	&calibrateChannel1Range,
	&calibrateChannel1Value,
	&calibrateChannel1VeryFastRewind,
	&calibrateChannel1FastRewind,
	&calibrateChannel1Rewind,
	&calibrateChannel1Forward,
	&calibrateChannel1FastForward,
	&calibrateChannel1VeryFastForward,
	&calibrateChannel2Label,
	&calibrateChannel2Range,
	&calibrateChannel2Value,
	&calibrateChannel2VeryFastRewind,
	&calibrateChannel2FastRewind,
	&calibrateChannel2Rewind,
	&calibrateChannel2Forward,
	&calibrateChannel2FastForward,
	&calibrateChannel2VeryFastForward,
	&calibrationSaver,
};

DECLARE_SUB_MENU(calibrationMenu, 88, 17, 70, "Kalibrálás", "Kalibráció", calibrationMenuTable);

bool hasValidTransmissionAmount = false;
bool hasUSBConnection = false;
bool isTransmitting = false;
bool lastMenuState = false;

DECLARE_DYNAMIC_LABEL(usbTransmittedByteCount, 67, 70, 17, [](char * buf, int /*maxLen*/, uint32_t * /*color*/) {
	Value val = scaleIntValue((int)usbHandler.getTransmittedBytes(), ValueScale::NONE, ValueUnit::BYTE);
	strcpy(buf, valueToString(val, true));
});

PushButtonMenu usbStartTransmission_descriptor = { .callback = [](SetupMenu *, const Menu *, unsigned){
	if( hasValidTransmissionAmount && hasUSBConnection )
	{
		isTransmitting = true;
		currentSetupMenu->setUSBStartMenuColor();
		usbHandler.resetTransmissionResult();
		currentSetupMenu->redrawValues();
		processDrawingTasks();
		currentSetupMenu->usbTransmit(currentSetupMenu->getMenuIndex(&usbTransmittedByteCount));
		isTransmitting = false;
		currentSetupMenu->setUSBStartMenuColor();
		processDrawingTasks();
	}
}, .color = SETUPMENU_COLOR_MENU_DISABLED };

const Menu usbStartTransmission = { .type = MENUTYPE_TEXT_BUTTON, .title = "Adatok küldése", .x = 36, .y = 89, .width = 80,
		.reserved = 0, .descriptor = (void *)&usbStartTransmission_descriptor};

void SetupMenu::setUSBStartMenuColor()
{
	bool state = hasValidTransmissionAmount && hasUSBConnection && !isTransmitting;

	uint32_t color = state ? SETUPMENU_COLOR_MENU : SETUPMENU_COLOR_MENU_DISABLED;
	usbStartTransmission_descriptor.color = color;

	if( state != lastMenuState )
	{
		drawMenuItem(getMenuIndex(&usbStartTransmission));
		lastMenuState = state;
	}
}

DECLARE_LABEL(usbTransmitLable, 0, 0, "Az átvitel módja:");

DECLARE_RADIO_BUTTON(usbTransmitRadio1,   0, 11, 30, "8 bit",  RADIO_TRANSMIT_BITS_ID, (short)TRANSMIT_8_BIT_SINGLE_CHANNEL,  &Config::getTransmitTypeInt, &Config::setTransmitTypeInt);
DECLARE_RADIO_BUTTON(usbTransmitRadio2,  37, 11, 35, "12 bit", RADIO_TRANSMIT_BITS_ID, (short)TRANSMIT_12_BIT_SINGLE_CHANNEL, &Config::getTransmitTypeInt, &Config::setTransmitTypeInt);

DECLARE_TOGGLE(usbTransmitDualChannel, 94, 11, 0, "Két csatorna", &Config::isDualChannelTransmit, &Config::setDualChannelTransmit);

DECLARE_LABEL(usbTransmitSamplingFrequencyLabel, 0, 28, "Mintavétel:");

DECLARE_VALUE(usbTransmitSamplingFrequency, 85, 28, 9, &Config::getTransmitSamplingInterval,
		&Config::setTransmitSamplingInterval, [](unsigned value) -> Value {
	int freq = (int)((uint64_t)2560000000 / (uint64_t)value);

	return scaleValue(freq, ValueScale::NONE, ValueUnit::FREQUENCY);
});

DECLARE_TEXT_BUTTON(usbTransmitSamplingRewind, 74, 25, 0, "<", [](SetupMenu *, const Menu *, unsigned){
	config.setTransmitSamplingInterval(Sampler::getPreviousSamplingRate(config.getTransmitSamplingInterval()));
});

DECLARE_TEXT_BUTTON(usbTransmitSamplingFastRewind, 57, 25, 0, "<<", [](SetupMenu *, const Menu *, unsigned){
	for(int i=0; i < 5; i++)
		config.setTransmitSamplingInterval(Sampler::getPreviousSamplingRate(config.getTransmitSamplingInterval()));
});

DECLARE_TEXT_BUTTON(usbTransmitSamplingForward, 132, 25, 0, ">", [](SetupMenu *, const Menu *, unsigned){
	config.setTransmitSamplingInterval(Sampler::getNextSamplingRate(config.getTransmitSamplingInterval()));
});

DECLARE_TEXT_BUTTON(usbTransmitSamplingFastForward, 144, 25, 0, ">>", [](SetupMenu *, const Menu *, unsigned){
	for(int i=0; i < 5; i++)
		config.setTransmitSamplingInterval(Sampler::getNextSamplingRate(config.getTransmitSamplingInterval()));
});

DECLARE_LABEL(usbTransmitBandwidthLabel, 0, 41, "Sávszélesség:");

DECLARE_DYNAMIC_LABEL(usbTransmitBandwidth, 67, 41, 18, [](char * buf, int /*maxLen*/, uint32_t * color) {
	unsigned requiredBytes = 2560000000 / config.getTransmitSamplingInterval();

	switch(config.getTransmitType())
	{
	case TRANSMIT_8_BIT_DUAL_CHANNEL:
		requiredBytes *= 2;
		break;
	case TRANSMIT_12_BIT_SINGLE_CHANNEL:
		requiredBytes = requiredBytes * 3 / 2;
		break;
	case TRANSMIT_12_BIT_DUAL_CHANNEL:
		requiredBytes = requiredBytes * 3;
		break;
	case TRANSMIT_8_BIT_SINGLE_CHANNEL:
	default:
		break;
	}

	Value val = scaleValue((int)requiredBytes, ValueScale::NONE, ValueUnit::BYTE_PER_SEC);
	strcpy(buf, valueToString(val, true));

	hasValidTransmissionAmount = true;
	if( requiredBytes > 256 * MAX_USB_TRANSMISSION_RATE )
	{
		hasValidTransmissionAmount = false;
		*color = SETUPMENU_COLOR_ERROR_LABEL;
		strcat(buf, " (sok)");
	}

	currentSetupMenu->setUSBStartMenuColor();
});

DECLARE_SEPARATOR_LINE(usbTransmitSeparator, 53);

DECLARE_LABEL(usbStateLabel, 0, 59, "USB állapot:");

DECLARE_DYNAMIC_LABEL(usbState, 67, 59, 17, [](char * buf, int /*maxLen*/, uint32_t * color) {
	*color = SETUPMENU_COLOR_ERROR_LABEL;

	const char * txt = "";
	hasUSBConnection = false;

	switch( usbHandler.getState() )
	{
	case UNCONNECTED:
		txt = "nincs csatolva";
		break;
	case ATTACHED:
		txt = "csatlakozik";
		break;
	case SUSPENDED:
		txt = "felfüggesztett";
		break;
	case ADDRESSED:
		txt = "megcímzett";
		break;
	case CONFIGURED:
		txt = "felkonfigurált";
		*color = SETUPMENU_COLOR_OK_LABEL;
		hasUSBConnection = true;
		break;

	default:
		break;
	}

	strcpy(buf, txt);
	currentSetupMenu->setUSBStartMenuColor();
});

DECLARE_LABEL(usbTransmittedByteCountLabel, 0, 70, "Átküldve:");

DECLARE_DYNAMIC_LABEL(usbTransmissionResult, 0, 80, 20, [](char * buf, int /*maxLen*/, uint32_t * color) {
	*color = SETUPMENU_COLOR_ERROR_LABEL;
	buf[0] = 0;

	switch( usbHandler.getTransmissionState() )
	{
	case STATE_USER_ABORT:
	case STATE_USB_ABORT:
		strcpy(buf, "Megszakítva");
		*color = SETUPMENU_COLOR_DYNAMIC_LABEL;
		break;
	case STATE_BUFFER_OVERFLOW:
		strcpy(buf, "Puffer túlcsordulás");
		break;
	default:
		break;
	}
});



const Menu * const usbTransmitMenuTable[] = {
	&usbTransmitLable,
	&usbTransmitRadio1,
	&usbTransmitRadio2,
	&usbTransmitDualChannel,
	&usbTransmitSamplingFrequencyLabel,
	&usbTransmitSamplingFrequency,
	&usbTransmitSamplingRewind,
	&usbTransmitSamplingFastRewind,
	&usbTransmitSamplingForward,
	&usbTransmitSamplingFastForward,
	&usbTransmitBandwidthLabel,
	&usbTransmitBandwidth,
	&usbTransmitSeparator,
	&usbStateLabel,
	&usbState,
	&usbTransmittedByteCountLabel,
	&usbTransmittedByteCount,
	&usbStartTransmission,
	&usbTransmissionResult,
};

DECLARE_SUB_MENU_CB(usbTransmitMenu, 0, 34, 70, "USB átvitel", "USB adatátvitel", usbTransmitMenuTable, [] (MenuCallbackType type) {
	switch( type )
	{
	case CALLBACKTYPE_START:
		usbHandler.attach();
		break;
	case CALLBACKTYPE_EXIT:
		usbHandler.detach();
		break;
	default:
		break;
	}
} );

DECLARE_LABEL(fftMagnifyLabel, 0, 0, "Nagyítás:");

DECLARE_RADIO_BUTTON(fftRadio2,   0, 13, 25, "1x",   RADIO_FFT_ID,  (short)FFT_MAGNIFY_1X,   &Config::getFFTMagnificationInt, &Config::setFFTMagnificationInt);
DECLARE_RADIO_BUTTON(fftRadio3,  30, 13, 25, "1.5x", RADIO_FFT_ID,  (short)FFT_MAGNIFY_1_5X, &Config::getFFTMagnificationInt, &Config::setFFTMagnificationInt);
DECLARE_RADIO_BUTTON(fftRadio4,  60, 13, 25, "2x",   RADIO_FFT_ID,  (short)FFT_MAGNIFY_2X,   &Config::getFFTMagnificationInt, &Config::setFFTMagnificationInt);
DECLARE_RADIO_BUTTON(fftRadio5,  90, 13, 25, "2.5x", RADIO_FFT_ID,  (short)FFT_MAGNIFY_2_5X, &Config::getFFTMagnificationInt, &Config::setFFTMagnificationInt);
DECLARE_RADIO_BUTTON(fftRadio6,   0, 30, 25, "3x",   RADIO_FFT_ID,  (short)FFT_MAGNIFY_3X,   &Config::getFFTMagnificationInt, &Config::setFFTMagnificationInt);
DECLARE_RADIO_BUTTON(fftRadio7,  30, 30, 25, "3.5x", RADIO_FFT_ID,  (short)FFT_MAGNIFY_3_5X, &Config::getFFTMagnificationInt, &Config::setFFTMagnificationInt);
DECLARE_RADIO_BUTTON(fftRadio8,  60, 30, 25, "4x",   RADIO_FFT_ID,  (short)FFT_MAGNIFY_4X,   &Config::getFFTMagnificationInt, &Config::setFFTMagnificationInt);
DECLARE_RADIO_BUTTON(fftRadio1,  90, 30, 25, "Auto", RADIO_FFT_ID,  (short)FFT_MAGNIFY_AUTO, &Config::getFFTMagnificationInt, &Config::setFFTMagnificationInt);


const Menu * const fftMenuTable[] = {
	&fftMagnifyLabel,
	&fftRadio1,
	&fftRadio2,
	&fftRadio3,
	&fftRadio4,
	&fftRadio5,
	&fftRadio6,
	&fftRadio7,
	&fftRadio8,
};

DECLARE_SUB_MENU(fftMenu, 77, 34, 30, "FFT", "FFT menü", fftMenuTable);

const Menu * const mainMenuTable[] = {
	&channelMenu,
	&triggerMenu,
	&testMenu,
	&oscilloscopeMenu,
	&calibrationMenu,
	&usbTransmitMenu,
	&fftMenu,
};

DECLARE_MENU(mainMenu, 0, 0, 50, "Beállítások", mainMenuTable);
