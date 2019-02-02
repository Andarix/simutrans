/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

/*
 * Attempt of graphics for the Simulation game
 * Hj. Malthaner, Aug. 1997
 *
 *
 * 3D, isometric representation
 */
#ifndef simgraph_h
#define simgraph_h

#include "../simcolor.h"
#include "../unicode.h"
#include "../simtypes.h"
#include "clip_num.h"
#include "simimg.h"
#include "scr_coord.h"


extern int large_font_ascent;
extern int large_font_total_height;

#define LINEASCENT (large_font_ascent)
#define LINESPACE (large_font_total_height)

/**
* Alignment enum to align controls against each other
* Vertical and horizontal alignment can be masked together
* Unused bits are reserved for future use, set to 0.
*
* @author Max Kielland
*/
enum control_alignments_t {

	ALIGN_NONE       = 0x00,

	ALIGN_TOP        = 0x01,
	ALIGN_CENTER_V   = 0x02,
	ALIGN_BOTTOM     = 0x03,
	ALIGN_INTERIOR_V = 0x00,
	ALIGN_EXTERIOR_V = 0x10,
	ALIGN_STRETCH_V  = 0x20,

	ALIGN_LEFT       = 0x04,
	ALIGN_CENTER_H   = 0x08,
	ALIGN_RIGHT      = 0x0C,
	ALIGN_INTERIOR_H = 0x00,
	ALIGN_EXTERIOR_H = 0x40,
	ALIGN_STRETCH_H  = 0x80,

	// These flags does not belong in here but
	// are defined here until we sorted this out.
	// They are only used in display_text_proportional_len_clip_rgb()
//	DT_DIRTY         = 0x8000,
	DT_CLIP          = 0x4000
};
typedef uint16 control_alignment_t;

// size of koordinates
typedef short KOORD_VAL;


struct clip_dimension {
	KOORD_VAL x, xx, w, y, yy, h;
};

// helper macros

// save the current clipping and set a new one
#define PUSH_CLIP(x,y,w,h) \
{\
clip_dimension const p_cr = display_get_clip_wh(); \
display_set_clip_wh(x, y, w, h);

// save the current clipping and set a new one
// fit it to old clipping region
#define PUSH_CLIP_FIT(x,y,w,h) \
{\
	clip_dimension const p_cr = display_get_clip_wh(); \
	display_set_clip_wh(x, y, w, h CLIP_NUM_DEFAULT, true);

// restore a saved clipping rect
#define POP_CLIP() \
display_set_clip_wh(p_cr.x, p_cr.y, p_cr.w, p_cr.h); \
}

/**
 *
 */
PIXVAL color_idx_to_rgb(PIXVAL idx);
PIXVAL color_rgb_to_idx(PIXVAL color);

/*
 * Get 24bit RGB888 colour from an index of the old 8bit palette
 */
uint32 get_color_rgb(uint8 idx);

/*
 * Environment colours from RGB888 to system format
 */
void env_t_rgb_to_system_colors();

/**
 * Helper functions for clipping along tile borders.
 * @author Dwachs
 */
void add_poly_clip(int x0_,int y0_, int x1, int y1, int ribi  CLIP_NUM_DEF);
void clear_all_poly_clip(CLIP_NUM_DEF0);
void activate_ribi_clip(int ribi  CLIP_NUM_DEF);

/* Do no access directly, use the get_tile_raster_width()
 * macro instead.
 * @author Hj. Malthaner
 */
#define get_tile_raster_width()    (tile_raster_width)
extern KOORD_VAL tile_raster_width;

#define get_base_tile_raster_width() (base_tile_raster_width)
extern KOORD_VAL base_tile_raster_width;

/* changes the raster width after loading */
KOORD_VAL display_set_base_raster_width(KOORD_VAL new_raster);


int zoom_factor_up();
int zoom_factor_down();


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
void simgraph_init(KOORD_VAL width, KOORD_VAL height, int fullscreen);
int is_display_init();
void simgraph_exit();
void simgraph_resize(KOORD_VAL w, KOORD_VAL h);
void reset_textur(void *new_textur);

/**
 * Loads the font, returns the number of characters in it
 * @param reload if true forces reload
 */
uint16 display_load_font(const char* fname, bool reload = false);

image_id get_image_count();
void register_image(class image_t *);

// delete all images above a certain number ...
void display_free_all_images_above( image_id above );

// unzoomed offsets
void display_get_base_image_offset( image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw );
// zoomed offsets
void display_get_image_offset( image_id image, KOORD_VAL *xoff, KOORD_VAL *yoff, KOORD_VAL *xw, KOORD_VAL *yw );
void display_mark_img_dirty( image_id image, KOORD_VAL x, KOORD_VAL y );

void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2); // clips to screen only
void mark_rect_dirty_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2  CLIP_NUM_DEF); // clips to clip_rect
void mark_screen_dirty();

KOORD_VAL display_get_width();
KOORD_VAL display_get_height();
void      display_set_height(KOORD_VAL);
void      display_set_actual_width(KOORD_VAL);

// force a certain size on a image (for rescaling tool images)
void display_fit_img_to_width( const image_id n, sint16 new_w );

void display_day_night_shift(int night);

// scrolls horizontally, will ignore clipping etc.
void display_scroll_band( const KOORD_VAL start_y, const KOORD_VAL x_offset, const KOORD_VAL h );

// set first and second company color for player
void display_set_player_color_scheme(const int player, const uint8 col1, const uint8 col2 );

// only used for GUI, display image inside a rect
void display_img_aligned( const image_id n, scr_rect area, int align, const int dirty);

// display image with day and night change
void display_img_aux(const image_id n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty  CLIP_NUM_DEF);

/**
 * draws the images with alpha, either blended or as outline
 * @author kierongreen
 */
void display_rezoomed_img_blend(const image_id n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const int daynight, const int dirty  CLIP_NUM_DEF);
#define display_img_blend( n, x, y, c, dn, d ) display_rezoomed_img_blend( (n), (x), (y), 0, (c), (dn), (d)  CLIP_NUM_DEFAULT)

#define ALPHA_RED 0x1
#define ALPHA_GREEN 0x2
#define ALPHA_BLUE 0x4

void display_rezoomed_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const int daynight, const int dirty  CLIP_NUM_DEF);
#define display_img_alpha( n, a, f, x, y, c, dn, d ) display_rezoomed_img_alpha( (n), (a), (f), (x), (y), 0, (c), (dn), (d)  CLIP_NUM_DEFAULT)

// display image with color (if there) and optional day and night change
void display_color_img(const image_id n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

// display unzoomed image
void display_base_img(const image_id n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

typedef image_id stretch_map_t[3][3];

// this displays a 3x3 array of images to fit the scr_rect
void display_img_stretch( const stretch_map_t &imag, scr_rect area );

// this displays a 3x3 array of images to fit the scr_rect like above, but blend the color
void display_img_stretch_blend( const stretch_map_t &imag, scr_rect area, FLAGGED_PIXVAL color );

// Knightly : display unzoomed image with alpha, either blended or as outline
void display_base_img_blend(const image_id n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const int daynight, const int dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);
void display_base_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const int daynight, const int dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

// Knightly : pointer to image display procedures
typedef void (*display_image_proc)(const image_id n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const int daynight, const int dirty  CLIP_NUM_DEF);
typedef void (*display_blend_proc)(const image_id n, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const int daynight, const int dirty  CLIP_NUM_DEF);
typedef void (*display_alpha_proc)(const image_id n, const image_id alpha_n, const unsigned alpha_flags, KOORD_VAL xp, KOORD_VAL yp, const signed char player_nr, const FLAGGED_PIXVAL color_index, const int daynight, const int dirty  CLIP_NUM_DEF);

// Knightly : variables for storing currently used image procedure set and tile raster width
extern display_image_proc display_normal;
extern display_image_proc display_color;
extern display_blend_proc display_blend;
extern display_alpha_proc display_alpha;
extern signed short current_tile_raster_width;

// Knightly : call this instead of referring to current_tile_raster_width directly
#define get_current_tile_raster_width() (current_tile_raster_width)

// Knightly : for switching between image procedure sets and setting current tile raster width
inline void display_set_image_proc( bool is_global )
{
	if(  is_global  ) {
		display_normal = display_img_aux;
		display_color = display_color_img;
		display_blend = display_rezoomed_img_blend;
		display_alpha = display_rezoomed_img_alpha;
		current_tile_raster_width = get_tile_raster_width();
	}
	else {
		display_normal = display_base_img;
		display_color = display_base_img;
		display_blend = display_base_img_blend;
		display_alpha = display_base_img_alpha;
		current_tile_raster_width = get_base_tile_raster_width();
	}
}

// Blends two colors
PIXVAL display_blend_colors(PIXVAL background, PIXVAL foreground, int percent_blend);

// blends a rectangular region
void display_blend_wh_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, int percent_blend );

void display_fillbox_wh_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, bool dirty);

void display_fillbox_wh_clip_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

void display_vline_wh_clip_rgb(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, PIXVAL c, bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

void display_clear();

void display_flush_buffer();

void display_show_pointer(int yesno);
void display_set_pointer(int pointer);
void display_show_load_pointer(int loading);


void display_array_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, const PIXVAL *arr);

// compound painting routines
void display_outline_proportional_rgb(KOORD_VAL xpos, KOORD_VAL ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len=-1);
void display_shadow_proportional_rgb(KOORD_VAL xpos, KOORD_VAL ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len=-1);
void display_ddd_box_rgb(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PIXVAL tl_color, PIXVAL rd_color, bool dirty);
void display_ddd_box_clip_rgb(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PIXVAL tl_color, PIXVAL rd_color);


// unicode save moving in strings
size_t get_next_char(const char* text, size_t pos);
sint32 get_prev_char(const char* text, sint32 pos);

KOORD_VAL display_get_char_width(utf32 c);

/* returns true, if this is a valid character */
bool has_character( utf16 char_code );

/**
 * Returns the width of the widest character in a string.
 * @param text  pointer to a string of characters to evaluate.
 * @param len   length of text buffer to evaluate. If set to 0,
 *              evaluate until null termination.
 * @author      Max Kielland
 */
KOORD_VAL display_get_char_max_width(const char* text, size_t len=0);

/**
 * For the next logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer advances to point to the next logical character
 * @author Knightly
 */
utf32 get_next_char_with_metrics(const char* &text, unsigned char &byte_length, unsigned char &pixel_width);

/**
 * For the previous logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer recedes to point to the previous logical character
 * @author Knightly
 */
utf32 get_prev_char_with_metrics(const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width);

/*
 * returns the index of the last character that would fit within the width
 * If an ellipsis len is given, it will only return the last character up to this len if the full length cannot be fitted
 * @returns index of next character. if text[index]==0 the whole string fits
 */
size_t display_fit_proportional( const char *text, scr_coord_val max_width, scr_coord_val ellipsis_width=0 );

/* routines for string len (macros for compatibility with old calls) */
#define proportional_string_width(text)          display_calc_proportional_string_len_width(text, 0x7FFF)
#define proportional_string_len_width(text, len) display_calc_proportional_string_len_width(text, len)
// length of a string in pixel
int display_calc_proportional_string_len_width(const char* text, size_t len);

/*
 * len parameter added - use -1 for previous behaviour.
 * completely renovated for unicode and 10 bit width and variable height
 * @author Volker Meyer, prissi
 * @date  15.06.2003, 2.1.2005
 */

// #ifdef MULTI_THREAD
int display_text_proportional_len_clip_rgb(KOORD_VAL x, KOORD_VAL y, const char* txt, control_alignment_t flags, const PIXVAL color, bool dirty, sint32 len  CLIP_NUM_DEF  CLIP_NUM_DEFAULT_ZERO);
/* macro are for compatibility */
#define display_proportional_rgb(               x, y, txt, align, color, dirty)       display_text_proportional_len_clip_rgb( x, y, txt, align,           color, dirty, -1 )
#define display_proportional_clip_rgb(          x, y, txt, align, color, dirty)       display_text_proportional_len_clip_rgb( x, y, txt, align | DT_CLIP, color, dirty, -1 )


/*
 * Display a string that if abbreviated by the (language specific) ellipsis character if too wide
 * If enough space is given, it just display the full string
 * @returns screen_width
 */
KOORD_VAL display_proportional_ellipsis_rgb( scr_rect r, const char *text, int align, const PIXVAL color, const bool dirty, bool shadowed = false, PIXVAL shadow_color = 0 );

void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt, FLAGGED_PIXVAL ddd_farbe, FLAGGED_PIXVAL text_farbe, const char *text, int dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);


int display_multiline_text_rgb(KOORD_VAL x, KOORD_VAL y, const char *inbuf, PIXVAL color);

// line drawing primitives
void display_direct_line_rgb(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const PIXVAL color);
void display_direct_line_dotted_rgb(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const KOORD_VAL draw, const KOORD_VAL dontDraw, const PIXVAL color);
void display_circle_rgb( KOORD_VAL x0, KOORD_VAL  y0, int radius, const PIXVAL color );
void display_filled_circle_rgb( KOORD_VAL x0, KOORD_VAL  y0, int radius, const PIXVAL color );
void draw_bezier_rgb(KOORD_VAL Ax, KOORD_VAL Ay, KOORD_VAL Bx, KOORD_VAL By, KOORD_VAL ADx, KOORD_VAL ADy, KOORD_VAL BDx, KOORD_VAL BDy, const PIXVAL colore, KOORD_VAL draw, KOORD_VAL dontDraw);

void display_set_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO, bool fit = false);
clip_dimension display_get_clip_wh(CLIP_NUM_DEF0 CLIP_NUM_DEFAULT_ZERO);

void display_push_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);
void display_swap_clip_wh(CLIP_NUM_DEF0);
void display_pop_clip_wh(CLIP_NUM_DEF0);


void display_snapshot( int x, int y, int w, int h );

#if COLOUR_DEPTH != 0
extern uint8 display_day_lights[  LIGHT_COUNT * 3];
extern uint8 display_night_lights[LIGHT_COUNT * 3];
#endif

#endif
