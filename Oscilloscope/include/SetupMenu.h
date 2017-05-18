#ifndef SETUPMENU_H_
#define SETUPMENU_H_

#include "graphics/TaskCanvas.h"
#include "Sampler.h"
#include "Config.h"
#include "MeasurementData.h"
#include "InputDeviceHandler.h"

#define SETUPMENU_COLOR_TITLE                 COLOR_YELLOW
#define SETUPMENU_COLOR_LINE                  COLOR_WHITE
#define SETUPMENU_COLOR_MENU_FRAME            COLOR_WHITE
#define SETUPMENU_COLOR_FOCUSED_MENU_FRAME    0xFFFF80
#define SETUPMENU_COLOR_TEXT                  COLOR_WHITE
#define SETUPMENU_COLOR_MENU                  COLOR_BLUE
#define SETUPMENU_COLOR_MENU_DISABLED         COLOR_GRAY
#define SETUPMENU_COLOR_TOGGLED_MENU          COLOR_ORANGE
#define SETUPMENU_COLOR_CLOSE_BUTTON          COLOR_RED
#define SETUPMENU_COLOR_SAVE_BUTTON           0x00AF00
#define SETUPMENU_COLOR_SUNKEN_BUTTON         0xBfBFBF
#define SETUPMENU_COLOR_FOCUSED_SUNKEN_BUTTON 0xDfDF00
#define SETUPMENU_COLOR_LABEL                 COLOR_YELLOW
#define SETUPMENU_COLOR_DYNAMIC_LABEL         COLOR_ORANGE
#define SETUPMENU_COLOR_ERROR_LABEL           0xFF2020
#define SETUPMENU_COLOR_OK_LABEL              COLOR_GREEN
#define SETUPMENU_COLOR_VALUE                 COLOR_GREEN

#define MAX_MENU_ITEMS                 27

typedef enum
{
	MENUTYPE_SUBMENU,
	MENUTYPE_RADIO,
	MENUTYPE_RADIO_IMAGE,
	MENUTYPE_TOGGLE,
	MENUTYPE_TEXT_BUTTON,
	MENUTYPE_IMAGE_BUTTON,
	MENUTYPE_LABEL,
	MENUTYPE_DYNAMIC_LABEL,
	MENUTYPE_VALUE,
	MENUTYPE_VALUE_SAVER,
	MENUTYPE_SEPARATOR_LINE,
} MenuType;

typedef enum
{
	CALLBACKTYPE_START,
	CALLBACKTYPE_LOOP,
	CALLBACKTYPE_EXIT,
} MenuCallbackType;

typedef struct
{
	const MenuType     type;
	const char       * title;
	const short        x;
	const short        y;
	const short        width;
	const short        reserved;
	const void       * descriptor;
} Menu;

class SetupMenu;
typedef void (*ItemPressCallback)(SetupMenu *, const Menu *, unsigned);

typedef struct
{
	const unsigned       number;
	const Menu * const * submenus;
	const char *         headerText;
	void                (* const callback)(MenuCallbackType type) ;
} SubMenu;

typedef struct
{
	unsigned        (Config::* const getValue)() ;
	void            (Config::* const setValue)(unsigned value);

	const short        radioId;
	const short        radioValue;
} RadioMenu;

typedef struct
{
	bool            (Config::* const getValue)() ;
	void            (Config::* const setValue)(bool value);
} ToggleMenu;

typedef struct
{
	const ItemPressCallback callback;
	uint32_t                color;
} PushButtonMenu;

typedef struct
{
	Value           (* const converter)(unsigned value) ;
	unsigned        (Config::* const getValue)() ;
	void            (Config::* const setValue)(unsigned value);
	bool            preferInteger;
} ValueMenu;

typedef struct
{
	void            (* const save)(Config * newConfig);
} ValueSaverMenu;

typedef struct
{
	void            (* const getLabel)(char * buffer, int maxSize, uint32_t * color);
} DynamicLabelMenu;

typedef struct
{
	short               x;
	short               y;
	short               w;
	short               h;
	const Menu        * menu;
} MenuItemDescriptor;

class SetupMenu
{
private:
	unsigned numberOfItems = 0;
	MenuItemDescriptor menuItems[MAX_MENU_ITEMS];

	unsigned     focusedItem = 0xFFFFFFFF;
	unsigned     clickedItem = 0xFFFFFFFF;
	unsigned     exiting = false;

	const Menu * currentlyDisplayedMenu = 0;

	TaskCanvas * canvas;

	void         drawMenuPage( const Menu * menu );
	void         drawMenuItem( unsigned ndx );
	unsigned     registerMenuItem( const Menu * menu );
	void         drawAll();
	void         moveFocus(unsigned id);
	void         animateClick( unsigned ndx );
	void         clickSubMenu( const Menu * menu );
	void         delay(unsigned ms);
	void         handlePress( unsigned ndx );
	void         calculateFocus(InputDeviceEvent event);
	bool         isFocusable(MenuType type);

	void         saveMenuItem(Config * config, const Menu * menu);

public:
	SetupMenu(TaskCanvas * canvasIn) : canvas(canvasIn) {}
	void         showMenu( const Menu * menu, const Menu * autoClickOn = 0 );
	void         handleInputDeviceEvent(InputDeviceEvent joystickEvent);

	void         close();
	void         save();

	static int   getMultiplier(unsigned val, unsigned base);
	static void  adjustTestFrequency(int amount);
	static void  adjustCalibration(int channel, int amount);
	static void  calculateRange(int channel, char * buf, int maxLen);
	unsigned     getMenuIndex(const Menu * menu);
	void         setUSBStartMenuColor();
	void         usbTransmit(unsigned index);

	void         redrawRadioGroup(short id);
	void         redrawValues();

	static Value channel1Avg;
	static Value channel2Avg;
};

#define DECLARE_MENU(__name, __x, __y, __w, __text, __items)                                                       \
	const SubMenu __name ## _submenus = { .number = sizeof(__items) / sizeof(const Menu *), .submenus = __items, .headerText = __text, .callback = 0 }; \
	const Menu __name = { .type = MENUTYPE_SUBMENU, .title = __text, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _submenus};

#define DECLARE_SUB_MENU(__name, __x, __y, __w, __text, __text2, __items)                                          \
	const SubMenu __name ## _submenus = { .number = sizeof(__items) / sizeof(const Menu *), .submenus = __items, .headerText = __text2, .callback = 0 }; \
	const Menu __name = { .type = MENUTYPE_SUBMENU, .title = __text, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _submenus};

#define DECLARE_SUB_MENU_CB(__name, __x, __y, __w, __text, __text2, __items, __callback)                           \
	const SubMenu __name ## _submenus = { .number = sizeof(__items) / sizeof(const Menu *), .submenus = __items, .headerText = __text2, .callback = __callback }; \
	const Menu __name = { .type = MENUTYPE_SUBMENU, .title = __text, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _submenus};

#define DECLARE_TEXT_BUTTON(__name, __x, __y, __w, __text, __callback)                                             \
	const PushButtonMenu __name ## _descriptor = { .callback = __callback, .color = SETUPMENU_COLOR_MENU };        \
	const Menu __name = { .type = MENUTYPE_TEXT_BUTTON, .title = __text, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _descriptor};

#define DECLARE_IMAGE_BUTTON(__name, __x, __y, __w, __image, __color, __callback)                                  \
	const PushButtonMenu __name ## _descriptor = { .callback = __callback, .color = __color };                     \
	const Menu __name = { .type = MENUTYPE_IMAGE_BUTTON, .title = (const char *)__image, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _descriptor};

#define DECLARE_LABEL(__name, __x, __y, __text)                                                                    \
	const Menu __name = { .type = MENUTYPE_LABEL, .title = __text, .x = __x, .y = __y, .width = 0, .reserved = 0, .descriptor = 0};

#define DECLARE_SEPARATOR_LINE(__name, __y)                                                                        \
	const Menu __name = { .type = MENUTYPE_SEPARATOR_LINE, .title = 0, .x = 0, .y = __y, .width = 0, .reserved = 0, .descriptor = 0};

#define DECLARE_DYNAMIC_LABEL(__name, __x, __y, __w, __getter)                                                           \
	const DynamicLabelMenu __name ## _dynlabel = { .getLabel = __getter };                                         \
	const Menu __name = { .type = MENUTYPE_DYNAMIC_LABEL, .title = 0, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _dynlabel };

#define DECLARE_RADIO_BUTTON(__name, __x, __y, __w, __text, __group, __value, __getter, __setter)                  \
	const RadioMenu __name ## _radio = { .getValue = __getter, .setValue = __setter, .radioId = __group, .radioValue = __value }; \
	const Menu __name = { .type = MENUTYPE_RADIO, .title = __text, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _radio};

#define DECLARE_RADIO_IMAGE(__name, __x, __y, __w, __image, __group, __value, __getter, __setter)                  \
	const RadioMenu __name ## _radio = { .getValue = __getter, .setValue = __setter, .radioId = __group, .radioValue = __value }; \
	const Menu __name = { .type = MENUTYPE_RADIO_IMAGE, .title = (const char *)__image, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _radio};

#define DECLARE_TOGGLE(__name, __x, __y, __w, __text, __getter, __setter)                                          \
	const ToggleMenu __name ## _toggle { .getValue = __getter, .setValue = __setter };                             \
	const Menu __name = { .type = MENUTYPE_TOGGLE, .title = __text, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _toggle};

#define DECLARE_VALUE(__name, __x, __y, __w, __getter, __setter, __converter)                                      \
	const ValueMenu __name ## _value { .converter = __converter, .getValue = __getter, .setValue = __setter, .preferInteger = false };     \
	const Menu __name = { .type = MENUTYPE_VALUE, .title = 0, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _value};

#define DECLARE_VALUE_INT(__name, __x, __y, __w, __getter, __setter, __converter)                                  \
	const ValueMenu __name ## _value { .converter = __converter, .getValue = __getter, .setValue = __setter, .preferInteger = true };     \
	const Menu __name = { .type = MENUTYPE_VALUE, .title = 0, .x = __x, .y = __y, .width = __w, .reserved = 0, .descriptor = (void *)& __name ## _value};

#define DECLARE_VALUE_SAVER(__name, __saver)                                                                       \
	const ValueSaverMenu __name ## _saver { .save = __saver };                                                    \
	const Menu __name = { .type = MENUTYPE_VALUE_SAVER, .title = 0, .x = 0, .y = 0, .width = 0, .reserved = 0, .descriptor = (void *)& __name ## _saver};

extern const Menu mainMenu;
extern const Menu usbTransmitMenu;
extern const Menu usbStartTransmission;

#endif /* SETUPMENU_H_ */
