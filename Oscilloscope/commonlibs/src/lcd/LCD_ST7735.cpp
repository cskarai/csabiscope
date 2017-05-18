#include "lcd/LCD_ST7735.h"
#include <stdlib.h>
#include <string.h>

#define ST7735_TFTWIDTH  160
#define ST7735_TFTHEIGHT 128

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

static const uint8_t initCmd[] = {     // Init for 7735R, part 1 (red or green tab)
	SC_COMMAND + 0, ST7735_SWRESET,    //  1: Software reset, 0 args, w/delay
	SC_DELAY, 150,                     //     150 ms delay
	SC_COMMAND + 0, ST7735_SLPOUT,     //  2: Out of sleep mode, 0 args, w/delay
	SC_DELAY, 255,                     //     500 ms delay
	SC_COMMAND + 3, ST7735_FRMCTR1,    //  3: Frame rate ctrl - normal mode, 3 args:
		0x01, 0x2C, 0x2D,              //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
	SC_COMMAND + 3, ST7735_FRMCTR2,    //  4: Frame rate control - idle mode, 3 args:
		0x01, 0x2C, 0x2D,              //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
	SC_COMMAND + 6, ST7735_FRMCTR3,    //  5: Frame rate ctrl - partial mode, 6 args:
		0x01, 0x2C, 0x2D,              //     Dot inversion mode
		0x01, 0x2C, 0x2D,              //     Line inversion mode
	SC_COMMAND + 1, ST7735_INVCTR,     //  6: Display inversion ctrl, 1 arg, no delay:
		0x07,                          //     No inversion
	SC_COMMAND + 3, ST7735_PWCTR1,     //  7: Power control, 3 args, no delay:
		0xA2,
		0x02,                          //     -4.6V
		0x84,                          //     AUTO mode
	SC_COMMAND + 1, ST7735_PWCTR2,     //  8: Power control, 1 arg, no delay:
		0xC5,                          //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
	SC_COMMAND + 2, ST7735_PWCTR3,     //  9: Power control, 2 args, no delay:
		0x0A,                          //     Opamp current small
		0x00,                          //     Boost frequency
	SC_COMMAND + 2, ST7735_PWCTR4,     // 10: Power control, 2 args, no delay:
		0x8A,                          //     BCLK/2, Opamp current small & Medium low
		0x2A,
	SC_COMMAND + 2, ST7735_PWCTR5,     // 11: Power control, 2 args, no delay:
		0x8A, 0xEE,
	SC_COMMAND + 1, ST7735_VMCTR1,     // 12: Power control, 1 arg, no delay:
		0x0E,
	SC_COMMAND + 0, ST7735_INVOFF,     // 13: Don't invert display, no args, no delay
	SC_COMMAND + 1, ST7735_MADCTL,     // 14: Memory access control (directions), 1 arg:
		0x60,                          //     row addr/col addr, bottom to top refresh
	SC_COMMAND + 1, ST7735_COLMOD,     // 15: set color mode, 1 arg, no delay:
		0x05,                          //     16-bit color
	SC_COMMAND + 4, ST7735_CASET,      // 16: Column addr set, 4 args, no delay:
		0x00, 0x00,                    //     XSTART = 0
		0x00, 0x9F,                    //     XEND = 159
	SC_COMMAND + 4, ST7735_RASET,      // 17: Row addr set, 4 args, no delay:
		0x00, 0x00,                    //     XSTART = 0
		0x00, 0x7F,                    //     XEND = 127
	SC_COMMAND + 16, ST7735_GMCTRP1,   // 18: Magical unicorn dust, 16 args, no delay:
		0x02, 0x1c, 0x07, 0x12,
		0x37, 0x32, 0x29, 0x2d,
		0x29, 0x25, 0x2B, 0x39,
		0x00, 0x01, 0x03, 0x10,
	SC_COMMAND + 16, ST7735_GMCTRN1,   // 19: Sparkles and rainbows, 16 args, no delay:
		0x03, 0x1d, 0x07, 0x06,
		0x2E, 0x2C, 0x29, 0x2D,
		0x2E, 0x2E, 0x37, 0x3F,
		0x00, 0x00, 0x02, 0x10,
	SC_COMMAND + 0, ST7735_NORON,      // 20: Normal display on, no args, w/delay
	SC_DELAY, 10,                      //     10 ms delay
	SC_COMMAND + 0, ST7735_DISPON,     // 21: Main screen turn on, no args w/delay
	SC_DELAY, 100,                     //     100 ms delay
	SC_DONE };

void LCD_ST7735::init(SPIDriver * driver, STM32Pin a0, STM32Pin cs, STM32Pin backlight)
{
	spiContext = driver->createSPILCDContext(cs, a0, backlight, SPI_BaudRatePrescaler_4);
	playScenario( ScenarioPlayerTask::createScenarioPlayer( getColorSizeInBytes(), spiContext, initCmd) );
}

unsigned LCD_ST7735::getWidth()
{
	return ST7735_TFTWIDTH;
}

unsigned LCD_ST7735::getHeight()
{
	return ST7735_TFTHEIGHT;
}

unsigned LCD_ST7735::getColorSizeInBytes()
{
	return 2;
}

unsigned LCD_ST7735::addWindowAddrScenario(uint8_t * buf, unsigned x, unsigned y, unsigned w, unsigned h, Angle angle)
{
	if( buf == 0 )
		return 17;

	uint8_t madctl;
	unsigned x0, y0, x1, y1;

	switch(angle)
	{
	case ANGLE_270:
		madctl = 0x00;
		x0 = getHeight()-1-y;
		x1 = getHeight()-1-y+w;
		y0 = x;
		y1 = x+h;
		break;
	case ANGLE_180:
		madctl = 0xA0;
		x0 = getWidth()-1-x;
		x1 = getWidth()-1-x+w;
		y0 = getHeight()-1-y;
		y1 = getHeight()-1-y+h;
		break;
	case ANGLE_90:
		madctl = 0xC0;
		x0 = y;
		x1 = y+w;
		y0 = getWidth()-1-x;
		y1 = getWidth()-1-x+h;
		break;
	case ANGLE_0:
	default:
		madctl = 0x60;
		x0 = x;
		y0 = y;
		x1 = x + w;
		y1 = y + h;
	}

	buf[ 0] = SC_COMMAND + 1;
	buf[ 1] = ST7735_MADCTL;
	buf[ 2] = madctl;
	buf[ 3] = SC_COMMAND + 4;
	buf[ 4] = ST7735_CASET; // Column addr set
	buf[ 5] = x0 >> 8;
	buf[ 6] = x0 & 255;
	buf[ 7] = x1 >> 8;
	buf[ 8] = x1 & 255;
	buf[ 9] = SC_COMMAND + 4;
	buf[10] = ST7735_RASET; // Row addr set
	buf[11] = y0 >> 8;
	buf[12] = y0 & 255;
	buf[13] = y1 >> 8;
	buf[14] = y1 & 255;
	buf[15] = SC_COMMAND + 0;
	buf[16] = ST7735_RAMWR;
	return 17;
}

uint32_t LCD_ST7735::rgb(uint32_t color)
{
	return  (( color & 0xF80000 ) >> 8 ) |
			(( color & 0x00FC00 ) >> 5 ) |
			(( color & 0x0000F8 ) >> 3 );
}
