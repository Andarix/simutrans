#ifndef gui_colorbox_h
#define gui_colorbox_h

#include "gui_component.h"
#include "../../simcolor.h"

/**
 * Draws a simple colored box.
 */
class gui_colorbox_t : public gui_component_t
{
	PIXVAL color;

	scr_size max_size;
public:
	gui_colorbox_t(PIXVAL c = 0);

	void draw(scr_coord offset) OVERRIDE;

	void set_color(PIXVAL c)
	{
		color = c;
	}

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	void set_max_size(scr_size s)
	{
		max_size = s;
	}
};

#endif
