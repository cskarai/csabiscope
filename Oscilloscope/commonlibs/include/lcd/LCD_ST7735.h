#ifndef LCD_ST7735_H_
#define LCD_ST7735_H_

#include "graphics/TaskCanvas.h"
#include "task/TaskHandler.h"
#include "driver/SPIDriver.h"

class LCD_ST7735 : public virtual TaskCanvas
{
private:
	AbstractDriverContext * spiContext = 0;

public:
	virtual void init(SPIDriver * driver, STM32Pin a0, STM32Pin cs, STM32Pin backlight);
	virtual unsigned getWidth();
	virtual unsigned getHeight();
	virtual unsigned getColorSizeInBytes();

	virtual unsigned addWindowAddrScenario(uint8_t * dest, unsigned x, unsigned y, unsigned w, unsigned h, Angle angle = ANGLE_0);
	virtual AbstractDriverContext * getDriverContext() { return spiContext; }

	virtual uint32_t rgb(uint32_t color);

	virtual ~LCD_ST7735() {delete spiContext;}
};

#endif /* LCD_ST7735_H_ */
