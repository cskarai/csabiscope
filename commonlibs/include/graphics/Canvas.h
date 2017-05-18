#ifndef CANVAS_H_
#define CANVAS_H_

#include <inttypes.h>

typedef enum
{
	ANGLE_0=0,
	ANGLE_90=1,
	ANGLE_180=2,
	ANGLE_270=3,
} Angle;

typedef struct
{
	uint16_t x;
	uint16_t y;
	uint16_t h;
	uint16_t pos;
} BitmapSegmenterData;

typedef struct
{
	unsigned w;
	unsigned size;
	const uint8_t * map;
	uint32_t color_fore;
	uint32_t color_bck;
	Angle angle;
	unsigned userData;
	unsigned segments;
	BitmapSegmenterData segmenterData[0];
} BitmapSegmenterDescriptor;

typedef struct
{
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
	const uint8_t * map;
	const char *    text;
	const uint8_t * font;
	uint32_t color_fore;
	uint32_t color_bck;
	uint32_t color_frame;
	uint32_t color_sunken;
} ButtonDescriptor;

typedef struct
{
	uint16_t x;
	uint16_t y;
	uint8_t  ptr;
	uint8_t  len;
} TextSegmenterData;

typedef struct
{
	const uint8_t * font;
	const char * text;
	uint32_t color_fore;
	uint32_t color_bck;
	Angle angle;
	unsigned userData;
	unsigned segments;
	TextSegmenterData segmenterData[0];
} TextSegmenterDescriptor;


typedef void (*ColorSetter)(uint8_t *, unsigned, uint32_t);
typedef void (*ColorFiller)(uint8_t *, unsigned, unsigned, uint32_t);

#define COLOR_BLACK   0x000000
#define COLOR_WHITE   0xFFFFFF
#define COLOR_RED     0xFF0000
#define COLOR_GREEN   0x00FF00
#define COLOR_BLUE    0x0000FF
#define COLOR_YELLOW  0xFFFF00
#define COLOR_CYAN    0x00FFFF
#define COLOR_PURPLE  0xFF00FF
#define COLOR_MAGENTA 0xFF00FF
#define COLOR_ORANGE  0xFFA500
#define COLOR_GRAY    0xBfBFBF

typedef struct
{
	union {
		uint8_t  * data8;
		uint16_t * data16;
		uint32_t * data32;
	} buffer;

	unsigned x;
	unsigned y;
	unsigned w;
	unsigned h;
	Angle    angle;
	void *   deviceData;
} ColorBuffer;

class Canvas
{
private:
	const uint8_t * font;

	void drawButtonCommon(unsigned x, unsigned y, const uint8_t *map, const char * text, uint32_t color_fore, uint32_t color_bck, uint32_t color_frame, uint32_t color_sunken, int w, int h);

public:
	virtual unsigned      getWidth() = 0;
	virtual unsigned      getHeight() = 0;
	virtual unsigned      getColorSizeInBytes() = 0;

	virtual ColorBuffer * allocateColorBuffer(unsigned x, unsigned y, unsigned w, unsigned h, Angle angle = ANGLE_0);
	virtual void          fillColors(ColorBuffer * buffer) = 0;

	virtual void          floodColor(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color) = 0;

	virtual uint32_t      rgb(uint32_t color) = 0;

	void                  drawTextSegment(TextSegmenterDescriptor * desc, TextSegmenterData * data);
	virtual void          drawTextSegments(TextSegmenterDescriptor * desc);
	void                  drawBitmapSegment(BitmapSegmenterDescriptor * desc, BitmapSegmenterData * data);
	virtual void          drawBitmapSegments(BitmapSegmenterDescriptor * desc);

	virtual void          drawButton(ButtonDescriptor *);


public:
	void         clipCoordinates(unsigned &x, unsigned &y, unsigned &w, unsigned &h);
	uint8_t      parseUTF8(const char ** text);
	unsigned     textWidth(const char * text);

	void         fillRect(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color);
	void         fillScreen(uint32_t color);

	void         drawHLine(unsigned x, unsigned y, unsigned w, uint32_t color);
	void         drawVLine(unsigned x, unsigned y, unsigned h, uint32_t color);
	void         drawRect(unsigned x, unsigned y, unsigned w, unsigned h, uint32_t color);
	void         drawPixel(unsigned x, unsigned y, uint32_t color);

	void         drawColors(unsigned x, unsigned y, unsigned h, unsigned w, void *colors);

	void         setFont(const uint8_t * fontIn) { font = fontIn; }
	void         drawText(unsigned x, unsigned y, const char * text, uint32_t color_fore, uint32_t color_bck = 0, Angle angle = ANGLE_0 );
	void         drawBitmap(unsigned x, unsigned y, unsigned w, unsigned h, const uint8_t *map, uint32_t color_fore, uint32_t color_bck = 0, Angle angle = ANGLE_0);
	void         drawImage(unsigned x, unsigned y, const uint8_t *map, uint32_t color_fore, uint32_t color_bck = 0, Angle angle = ANGLE_0);

	void         drawTextButton(unsigned x, unsigned y, const char *text, uint32_t color_fore, uint32_t color_bck, uint32_t color_frame = COLOR_WHITE, uint32_t color_sunken = COLOR_GRAY, int w=0, int h=0);
	void         drawImageButton(unsigned x, unsigned y, const uint8_t *map, uint32_t color_fore, uint32_t color_bck, uint32_t color_frame = COLOR_WHITE, uint32_t color_sunken = COLOR_GRAY, int w=0, int h=0);

	ColorSetter  getColorSetter();
	ColorFiller  getColorFiller();

protected:
	~Canvas() {}
};

#endif /* CANVAS_H_ */
