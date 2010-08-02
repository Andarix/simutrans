/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include "../../simdebug.h"

#include "gui_scrollpane.h"
#include "gui_scrollbar.h"

#include "../../simgraph.h"
#include "../../simcolor.h"


/**
 * @param komp Die zu scrollende Komponente
 * @author Hj. Malthaner
 */
gui_scrollpane_t::gui_scrollpane_t(gui_komponente_t *komp) :
    scroll_x(scrollbar_t::horizontal),
    scroll_y(scrollbar_t::vertical)
{
	this->komp = komp;

	b_show_scroll_x = true;
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
	scroll_x.set_pos(koord(0, groesse.y-scrollbar_t::BAR_SIZE));
	scroll_x.set_groesse(groesse-koord(scrollbar_t::BAR_SIZE,scrollbar_t::BAR_SIZE));
	scroll_x.set_knob(groesse.x-scrollbar_t::BAR_SIZE, komp->get_groesse().x + komp->get_pos().x);	// set client/komp area

	if(b_has_size_corner  ||  b_show_scroll_x) {
		scroll_y.set_pos(koord(groesse.x-scrollbar_t::BAR_SIZE, 0));
		scroll_y.set_groesse(groesse-koord(scrollbar_t::BAR_SIZE,scrollbar_t::BAR_SIZE));
		scroll_y.set_knob(groesse.y-scrollbar_t::BAR_SIZE, komp->get_groesse().y + komp->get_pos().y);
	}
	else {
		scroll_y.set_pos(koord(groesse.x-scrollbar_t::BAR_SIZE, 0));
		scroll_y.set_groesse(groesse);
		scroll_y.set_knob(groesse.y, komp->get_groesse().y + komp->get_pos().y);
	}
	old_komp_groesse = komp->get_groesse();
}



/**
 * Bei Scrollpanes _muss_ diese Methode zum setzen der Groesse
 * benutzt werden.
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::set_groesse(koord groesse)
{
	gui_komponente_t::set_groesse(groesse);
	recalc_sliders(groesse);
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_scrollpane_t::infowin_event(const event_t *ev)
{
	if(b_show_scroll_y  &&  (scroll_y.getroffen(ev->mx, ev->my) || scroll_y.getroffen(ev->cx, ev->cy)) ) {
		event_t ev2 = *ev;
		translate_event(&ev2, -scroll_y.get_pos().x, -scroll_y.get_pos().y);
		return scroll_y.infowin_event(&ev2);
	}
	else if(b_show_scroll_x  &&  (scroll_x.getroffen(ev->mx, ev->my) || scroll_x.getroffen(ev->cx, ev->cy))) {
		event_t ev2 = *ev;
		translate_event(&ev2, -scroll_x.get_pos().x, -scroll_x.get_pos().y);
		return scroll_x.infowin_event(&ev2);
	}
	else if(b_show_scroll_y  &&  (IS_WHEELUP(ev)  ||  IS_WHEELDOWN(ev))) {
		// otherwise these events are only registered where directly over the scroll region
		// (and sometime even not then ... )
		return scroll_y.infowin_event(ev);
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
				const koord komp_size = focused_komp->groesse;
				const koord relative_pos = komp->get_focus_pos();
				if(  b_show_scroll_x  ) {
					const sint32 knob_offset_x = scroll_x.get_knob_offset();
					const sint32 view_width = groesse.x-scrollbar_t::BAR_SIZE;
					if(  relative_pos.x<knob_offset_x  ) {
						scroll_x.set_knob_offset(relative_pos.x);
					}
					else if(  relative_pos.x+komp_size.x>knob_offset_x+view_width  ) {
						scroll_x.set_knob_offset(relative_pos.x+komp_size.x-view_width);
					}
				}
				if(  b_show_scroll_y  ) {
					const sint32 knob_offset_y = scroll_y.get_knob_offset();
					const sint32 view_height = (b_has_size_corner || b_show_scroll_x) ? groesse.y-scrollbar_t::BAR_SIZE : groesse.y;
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
 * Setzt Positionen der Scrollbars
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



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void gui_scrollpane_t::zeichnen(koord pos)
{
	pos += this->pos;

	PUSH_CLIP(pos.x, pos.y, groesse.x-scrollbar_t::BAR_SIZE*b_show_scroll_y, groesse.y-scrollbar_t::BAR_SIZE*b_show_scroll_x );
	komp->zeichnen(pos - koord(scroll_x.get_knob_offset(), scroll_y.get_knob_offset()));
	POP_CLIP();

	// check, if we need to recalc slider size
	if(old_komp_groesse!=komp->get_groesse()) {
		recalc_sliders(get_groesse());
	}

	// sliding bar background color is now handled by scrollbar!
	if (b_show_scroll_x) {
		scroll_x.zeichnen(pos);
	}

	if (b_show_scroll_y) {
		scroll_y.zeichnen(pos);
	}
}
