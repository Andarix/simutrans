/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include "../../simdebug.h"

#include "../gui_frame.h"
#include "gui_scrollpane.h"
#include "gui_scrollbar.h"
#include "gui_button.h"

#include "../../display/simgraph.h"
#include "../../simcolor.h"


/**
 * @param komp, the scrolling component
 * @author Hj. Malthaner
 */
gui_scrollpane_t::gui_scrollpane_t(gui_komponente_t *komp) :
	scroll_x(scrollbar_t::horizontal),
	scroll_y(scrollbar_t::vertical)
{
	this->komp = komp;

	b_show_scroll_x = false;
	set_scroll_discrete_x(false);
	b_show_scroll_y = true;
	b_has_size_corner = true;

	old_komp_groesse = koord::invalid;
}


/**
 * recalc the scroll bar sizes
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::recalc_sliders(koord groesse)
{
	scroll_x.set_pos( koord(0, groesse.y-D_SCROLLBAR_HEIGHT) );
	scroll_y.set_pos( koord(groesse.x-D_SCROLLBAR_WIDTH, 0) );
	if(  b_show_scroll_y  &&  scroll_y.is_visible()  ) {
		scroll_x.set_groesse( groesse-D_SCROLLBAR_SIZE );
		scroll_x.set_knob( groesse.x-D_SCROLLBAR_WIDTH, komp->get_groesse().x + komp->get_pos().x );
	}
	else if(  b_has_size_corner  ) {
		scroll_x.set_groesse( groesse-D_SCROLLBAR_SIZE );
		scroll_x.set_knob( groesse.x, komp->get_groesse().x + komp->get_pos().x );
	}
	else {
		scroll_x.set_groesse( groesse-D_SCROLLBAR_SIZE );
		scroll_x.set_knob( groesse.x, komp->get_groesse().x + komp->get_pos().x );
	}

	if(  b_show_scroll_x  &&  scroll_x.is_visible()  ) {
		scroll_y.set_groesse( groesse-D_SCROLLBAR_SIZE );
		scroll_y.set_knob( groesse.y-D_SCROLLBAR_HEIGHT, komp->get_groesse().y + komp->get_pos().y );
	}
	else if(  b_has_size_corner  ) {
		scroll_y.set_groesse( groesse-D_SCROLLBAR_SIZE );
		scroll_y.set_knob( groesse.y, komp->get_groesse().y + komp->get_pos().y );
	}
	else {
		scroll_y.set_groesse( groesse-koord(D_SCROLLBAR_WIDTH,0) );
		scroll_y.set_knob( groesse.y, komp->get_groesse().y + komp->get_pos().y );
	}

	old_komp_groesse = komp->get_groesse()+komp->get_pos();
}


/**
 * Scrollpanes _must_ be used in this method to set the size
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::set_groesse(koord groesse)
{
	gui_komponente_t::set_groesse(groesse);
	// automatically increase/decrease slider area
	koord k = komp->get_groesse()+komp->get_pos();
	scroll_x.set_visible( (k.x > groesse.x)  &&  b_show_scroll_x  );
	scroll_y.set_visible(  (k.y > groesse.y)  &&  b_show_scroll_y  );
	// and then resize slider
	recalc_sliders(groesse);
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_scrollpane_t::infowin_event(const event_t *ev)
{
	if(   (b_show_scroll_y  &&  scroll_y.is_visible())  &&  ev->ev_class!=EVENT_KEYBOARD  &&  (scroll_y.getroffen(ev->mx, ev->my) || scroll_y.getroffen(ev->cx, ev->cy)) ) {
		event_t ev2 = *ev;
		translate_event(&ev2, -scroll_y.get_pos().x, -scroll_y.get_pos().y);
		return scroll_y.infowin_event(&ev2);
	}
	else if(  (b_show_scroll_x  &&  scroll_x.is_visible())  &&  ev->ev_class!=EVENT_KEYBOARD  &&  (scroll_x.getroffen(ev->mx, ev->my) || scroll_x.getroffen(ev->cx, ev->cy))) {
		event_t ev2 = *ev;
		translate_event(&ev2, -scroll_x.get_pos().x, -scroll_x.get_pos().y);
		return scroll_x.infowin_event(&ev2);
	}
	else if((IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev))  &&  (((b_show_scroll_y  &&  scroll_y.is_visible())  &&  !IS_SHIFT_PRESSED(ev))  ||  ((b_show_scroll_x  &&  scroll_x.is_visible())  &&  IS_SHIFT_PRESSED(ev)))) {
		// otherwise these events are only registered where directly over the scroll region
		// (and sometime even not then ... )
		return IS_SHIFT_PRESSED(ev) ? scroll_x.infowin_event(ev) : scroll_y.infowin_event(ev);
	}
	else {
		// translate according to scrolled position
		bool swallow;
		event_t ev2 = *ev;
		translate_event(&ev2, scroll_x.get_knob_offset() - komp->get_pos().x, scroll_y.get_knob_offset() - komp->get_pos().y);

		// hand event to component
		swallow = komp->infowin_event(&ev2);

		// Knightly : check if we need to scroll to the focused component
		if(  IS_LEFTCLICK(ev)  ||  (ev->ev_class==EVENT_KEYBOARD  &&  ev->ev_code==9)  ) {
			const gui_komponente_t *const focused_komp = komp->get_focus();
			if(  focused_komp  ) {
				const koord komp_size = focused_komp->get_groesse();
				const koord relative_pos = komp->get_focus_pos();
				if(  b_show_scroll_x  ) {
					const sint32 knob_offset_x = scroll_x.get_knob_offset();
					const sint32 view_width = groesse.x-D_SCROLLBAR_WIDTH;
					if(  relative_pos.x<knob_offset_x  ) {
						scroll_x.set_knob_offset(relative_pos.x);
					}
					else if(  relative_pos.x+komp_size.x>knob_offset_x+view_width  ) {
						scroll_x.set_knob_offset(relative_pos.x+komp_size.x-view_width);
					}
				}
				if(  b_show_scroll_y  ) {
					const sint32 knob_offset_y = scroll_y.get_knob_offset();
					const sint32 view_height = (b_has_size_corner || b_show_scroll_x) ? groesse.y-D_SCROLLBAR_HEIGHT : groesse.y;
					if(  relative_pos.y<knob_offset_y  ) {
						scroll_y.set_knob_offset(relative_pos.y);
					}
					else if(  relative_pos.y+komp_size.y>knob_offset_y+view_height  ) {
						scroll_y.set_knob_offset(relative_pos.y+komp_size.y-view_height);
					}
				}
			}
		}

		// Hajo: hack: component could have changed size
		// this recalculates the scrollbars
		if(  old_komp_groesse!=komp->get_groesse()  ) {
			recalc_sliders(get_groesse());
		}
		return swallow;
	}
	return false;
}


/**
 * Set the position of the Scrollbars
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::set_scroll_position(int x, int y)
{
	scroll_x.set_knob_offset(x);
	scroll_y.set_knob_offset(y);
}


int gui_scrollpane_t::get_scroll_x() const
{
	return scroll_x.get_knob_offset();
}


int gui_scrollpane_t::get_scroll_y() const
{
	return scroll_y.get_knob_offset();
}


scr_rect gui_scrollpane_t::get_client( void )
{
	scr_rect client( pos, groesse );
	if(  b_show_scroll_x  &&  scroll_x.is_visible()  ) {
		client.h -= D_SCROLLBAR_HEIGHT;
	}
	if(  b_show_scroll_y  &&  scroll_y.is_visible()  ) {
		client.w -= D_SCROLLBAR_WIDTH;
	}
	return client;
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::zeichnen(koord pos)
{
	// check, if we need to recalc slider size
	if(  old_komp_groesse  !=  komp->get_groesse()  ) {
		recalc_sliders( groesse );
	}

	// get client area (scroll panel - scrollbars)
	scr_rect client = get_client() + scr_coord(pos);

	PUSH_CLIP( client.x, client.y, client.w, client.h )
		komp->zeichnen( client.get_pos()-scr_coord(scroll_x.get_knob_offset(), scroll_y.get_knob_offset()) );
	POP_CLIP()

	// sliding bar background color is now handled by the scrollbar!
	if(  b_show_scroll_x  &&  scroll_x.is_visible()  ) {
		scroll_x.zeichnen( pos+get_pos() );
	}
	if(  b_show_scroll_y  &&  scroll_y.is_visible()  ) {
		scroll_y.zeichnen( pos+get_pos() );
	}
}
