#ifndef FONT_H
#define FONT_H

#include "../simtypes.h"

class font_t
{
public:
	char fname[PATH_MAX];
	sint16	height;
	sint16	descent;
	uint16 num_chars;
	uint8 *screen_width;
	uint8 *char_data;

	enum fonttype { BINARY=1, BDF=2, FREETYPE=3 };

	font_t()
	: height(0), descent(0), num_chars(0), screen_width(NULL), char_data(NULL)
	{
		fname[0]= 0;
	}
};

/*
 * characters are stored dense in a array
 * 23 rows of bytes, second row for widths between 8 and 16
 * then the start offset for drawing
 * then a byte with the real width
 */
#define CHARACTER_LEN (48)
#define CHARACTER_HEIGHT (23)


/**
 * Loads a font
 */
bool load_font(font_t* font, const char* fname);

#endif
