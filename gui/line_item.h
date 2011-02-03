#ifndef line_scrollitem_h
#define line_scrollitem_h

#include "components/gui_scrolled_list.h"
#include "../linehandle_t.h"

/**
 * Container for list entries - consisting of text and color
 */
class line_scrollitem_t : public gui_scrolled_list_t::scrollitem_t
{
private:
	linehandle_t line;
public:
	line_scrollitem_t( linehandle_t l ) : gui_scrolled_list_t::scrollitem_t( COL_ORANGE ) { line = l; }
	COLOR_VAL get_color();
	linehandle_t get_line() const { return line; }
	void set_color(COLOR_VAL) { assert(false); }
	virtual const char *get_text();
	virtual void set_text(char *t);
	virtual bool is_valid() { return line.is_bound(); }	//  can be used to indicate invalid entries
};

#endif
