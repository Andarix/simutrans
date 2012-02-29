/*
 * Scrollbar class
 * Niel Roest
 */

#include "../../simdebug.h"

#include "gui_scrollbar.h"
#include "action_listener.h"

#include "../../simcolor.h"
#include "../../simgraph.h"

#include "../../simskin.h"
#include "../../besch/skin_besch.h"


sint16 scrollbar_t::BAR_SIZE = 0;

scrollbar_t::scrollbar_t(enum type type) :
	type(type),
	knob_offset(0),
	knob_size(10),
	knob_area(20),
	knob_scroll_amount(LINESPACE), // equals one line
	knob_scroll_discrete(true)
{
	// init size of scroll bars (might be changed in between due to loading of new paks ...
	if(  button_t::scrollbar_slider_center != IMG_LEER  ) {
		BAR_SIZE = skinverwaltung_t::window_skin->get_bild(35)->get_pic()->w;
	}
	else if(  button_t::arrow_up_pushed != IMG_LEER  ) {
		BAR_SIZE = skinverwaltung_t::window_skin->get_bild(19)->get_pic()->w;
	}
	else {
		BAR_SIZE = 10;
	}

	if (type == vertical) {
		groesse = koord(BAR_SIZE,40);
		if(  button_t::arrow_up_pushed==IMG_LEER  ) {
			button_def[0].set_visible( false );
			button_def[1].set_visible( false );
		}
		button_def[0].set_typ(button_t::arrowup);
		button_def[1].set_typ(button_t::arrowdown);
		button_def[2].set_typ(button_t::scrollbar_vertical);
		button_def[3].set_typ(button_t::scrollbar_vertical);
	}
	else { // horizontal
		if(  button_t::arrow_left_pushed==IMG_LEER  ) {
			button_def[0].set_visible( false );
			button_def[1].set_visible( false );
		}
		groesse = koord(40,BAR_SIZE);
		button_def[0].set_typ(button_t::arrowleft);
		button_def[1].set_typ(button_t::arrowright);
		button_def[2].set_typ(button_t::scrollbar_horizontal);
		button_def[3].set_typ(button_t::scrollbar_horizontal);
	}

	button_def[0].set_pos(koord(0,0));
	button_def[1].set_pos(koord(0,0));
	reposition_buttons();
}



void scrollbar_t::set_groesse(koord groesse)
{
	if (type == vertical) {
		this->groesse.y = groesse.y;
	}
	else {
		this->groesse.x = groesse.x;
	}
	reposition_buttons();
}



void scrollbar_t::set_knob(sint32 size, sint32 area)
{
	if(size<1  ||  area<1) {
//		dbg->warning("scrollbar_t::set_knob()","size=%i, area=%i not in 1...x",size,area);
	}
	knob_size = max(1,size);
	knob_area = max(1,area);
	reposition_buttons();
}



// reset variable position and size values of the three buttons
void scrollbar_t::reposition_buttons()
{
	koord arrowsize;
	if(type == vertical) {
		arrowsize = button_t::arrow_down_normal!=IMG_LEER ? koord( skinverwaltung_t::window_skin->get_bild(20)->get_pic()->w, skinverwaltung_t::window_skin->get_bild(20)->get_pic()->h+1) : koord(0,0);
	}
	else {
		arrowsize = button_t::arrow_right_normal!=IMG_LEER ? koord( skinverwaltung_t::window_skin->get_bild(10)->get_pic()->w+1, skinverwaltung_t::window_skin->get_bild(10)->get_pic()->h) : koord(0,0);
	}
	const sint32 area = (type == vertical ? groesse.y-2*arrowsize.y : groesse.x-2*arrowsize.x); // area will be actual area knob can move in

	// check if scrollbar is too low
	if(  knob_size+knob_offset>knob_area+(knob_scroll_discrete?(knob_scroll_amount-1):0)  ) {
		knob_offset = max(0,(knob_area-knob_size));

		if(  knob_scroll_discrete  ) {
			knob_offset -= knob_offset%knob_scroll_amount;
		}
		call_listeners((long)knob_offset);
	}

	float ratio = (float)area / (float)knob_area;
	if (knob_area < knob_size) {
		ratio = (float)area / (float)knob_size;
	}

	sint32 offset;
	if(  knob_size+knob_offset>knob_area  ) {
		offset = max(0,((sint32)( (float)(knob_area-knob_size)*ratio+.5)));
	}
	else {
		offset = (sint32)( (float)knob_offset*ratio+.5);
	}

	sint32 size = (sint32)( (float)knob_size*ratio+.5);

	if(type == vertical) {
		button_def[0].set_pos( koord( (BAR_SIZE-arrowsize.x)/2, 0) );
		button_def[1].set_pos( koord( (BAR_SIZE-arrowsize.x)/2, groesse.y-arrowsize.y+1) );
		button_def[2].set_pos( koord( 0, arrowsize.y+offset ) );
		if(  button_t::scrollbar_left!=IMG_LEER  ) {
			size = max( size, skinverwaltung_t::window_skin->get_bild(33)->get_pic()->h+skinverwaltung_t::window_skin->get_bild(34)->get_pic()->h );
		}
		button_def[2].set_groesse( koord(BAR_SIZE,size) );
		button_def[3].set_pos( koord(0,arrowsize.y) );
		button_def[3].set_groesse( koord(BAR_SIZE,groesse.y-2*arrowsize.y) );
	}
	else { // horizontal
		button_def[0].set_pos( koord(0,(BAR_SIZE-arrowsize.y)/2) );
		button_def[1].set_pos( koord(groesse.x-arrowsize.x+1,(BAR_SIZE-arrowsize.y)/2) );
		button_def[2].set_pos( koord(arrowsize.x+offset,0) );
		if(  button_t::scrollbar_left!=IMG_LEER  ) {
			size = max( size, skinverwaltung_t::window_skin->get_bild(27)->get_pic()->w+skinverwaltung_t::window_skin->get_bild(28)->get_pic()->w );
		}
		button_def[2].set_groesse( koord(size,BAR_SIZE) );
		button_def[3].set_pos( koord(arrowsize.x,0) );
		button_def[3].set_groesse( koord(groesse.x-2*arrowsize.x,BAR_SIZE) );
	}
}



// signals slider drag. If slider hits end, returned amount is smaller.
sint32 scrollbar_t::slider_drag(sint32 amount)
{
	const sint32 area = (type == vertical ? groesse.y : groesse.x)-24; // area will be actual area knob can move in

	const float ratio = (float)area / (float)knob_area;
	amount = (int)( (float)amount / ratio );
	int proposed_offset = knob_offset + amount;

	// 0< possible if content is smaller than window
	const sint32 maximum = max(0,((knob_area-knob_size)+(knob_scroll_discrete?(knob_scroll_amount-1):0)));

	if (proposed_offset<0) {
		proposed_offset = 0;
	}
	if (proposed_offset>maximum) {
		proposed_offset = maximum;
	}

	if (proposed_offset != knob_offset) {
		sint32 o;
		knob_offset = proposed_offset;

		call_listeners((long)knob_offset);
		o = real_knob_position();
		reposition_buttons();
		return real_knob_position() - o;
	}
	return 0;
}



// either arrow buttons is just pressed (or long enough for a repeat event)
void scrollbar_t::button_press(sint32 number)
{
	// 0< possible if content is smaller than window
	const sint32 maximum = max(0,((knob_area-knob_size)+(knob_scroll_discrete?(knob_scroll_amount-1):0)));

	if (number == 0) {
		knob_offset -= knob_scroll_amount;
		if (knob_offset<0) {
			knob_offset = 0;
		}
	}
	else { // number == 1
		knob_offset += knob_scroll_amount;
		if (knob_offset>maximum) {
			knob_offset = maximum;
		}
	}

	if(  knob_scroll_discrete  ) {
		knob_offset -= knob_offset % knob_scroll_amount;
	}

	call_listeners((long)knob_offset);
	reposition_buttons();
}



void scrollbar_t::space_press(sint32 updown) // 0: scroll up/left, 1: scroll down/right
{
	// 0< possible if content is smaller than window
	const sint32 maximum = max(0,((knob_area-knob_size)+(knob_scroll_discrete?(knob_scroll_amount-1):0)));

	if (updown == 0) {
		knob_offset -= knob_size;
		if (knob_offset<0) {
			knob_offset = 0;
		}
	}
	else { // updown == 1
		knob_offset += knob_size;
		if (knob_offset>maximum) {
			knob_offset = maximum;
		}
	}

	if(  knob_scroll_discrete  ) {
		knob_offset -= knob_offset % knob_scroll_amount;
	}

	call_listeners((long)knob_offset);
	reposition_buttons();
}



bool scrollbar_t::infowin_event(const event_t *ev)
{
	const int x = ev->cx;
	const int y = ev->cy;
	int i;
	bool b_button_hit = false;

	// 2003-11-04 hsiegeln added wheelsupport
	// prissi: repaired it, was never doing something ...
	if (IS_WHEELUP(ev) && (type == vertical) != IS_SHIFT_PRESSED(ev)) {
		button_press(0);
	}
	else if (IS_WHEELDOWN(ev) && (type == vertical) != IS_SHIFT_PRESSED(ev)) {
		button_press(1);
	}
	else if (IS_LEFTCLICK(ev)  ||  IS_LEFTREPEAT(ev)) {
		for (i=0;i<3;i++) {
			if (button_def[i].getroffen(x, y)) {
				button_def[i].pressed = true;
				if (i<2) {
					button_press(i);
				}
				b_button_hit = true;
			}
		}

		if (!b_button_hit) {
			if (type == vertical) {
				if (y < real_knob_position()+12) {
					space_press(0);
				}
				else {
					space_press(1);
				}
			}
			else {
				if (x < real_knob_position()+12) {
					space_press(0);
				}
				else {
					space_press(1);
				}
			}
		}
	} else if (IS_LEFTDRAG(ev)) {
		if (button_def[2].getroffen(x,y)) {
			sint32 delta;

			// Hajo: added vertical/horizontal check
			if(type == vertical) {
				delta = ev->my - ev->cy;
			}
			else {
				delta = ev->mx - ev->cx;
			}

			delta = slider_drag(delta);

			// Hajo: added vertical/horizontal check
			if(type == vertical) {
				change_drag_start(0, delta);
			}
			else {
				change_drag_start(delta, 0);
			}
		}
	} else if (IS_LEFTRELEASE(ev)) {
		for (i=0;i<3;i++) {
			if (button_def[i].getroffen(x, y)) {
				button_def[i].pressed = false;
			}
		}

		if(  knob_scroll_discrete  ) {
			knob_offset -= knob_offset % knob_scroll_amount;
		}
	}
	return false;
}



void scrollbar_t::zeichnen(koord pos)
{
	pos += this->pos;
/*
	// if opaque style, display GREY sliding bar backgrounds
	if (type == vertical) {
		display_fillbox_wh(pos.x, pos.y+12, BAR_WIDTH, groesse.y-24, MN_GREY1, true);
	}
	else {
		display_fillbox_wh(pos.x+12, pos.y, groesse.x-24, BAR_WIDTH, MN_GREY1, true);
	}
*/
	button_def[0].zeichnen(pos);
	button_def[1].zeichnen(pos);
	button_def[3].pressed = true;
	button_def[3].zeichnen(pos);
	button_def[2].pressed = false;
	button_def[2].zeichnen(pos);
}
