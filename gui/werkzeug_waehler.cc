/*
 * Copyright (c) 1997 - 2004 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simcolor.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simwerkz.h"
#include "../besch/skin_besch.h"
#include "../besch/sound_besch.h"
#include "../dataobj/umgebung.h"
#include "werkzeug_waehler.h"



werkzeug_waehler_t::werkzeug_waehler_t(karte_t* welt, const char* titel, const char *helpfile, koord icon, bool allow_break) :
	gui_frame_t(titel), tools(0)
{
	this->allow_break = allow_break;
    this->welt = welt;
    this->hilfe_datei = helpfile;
	this->icon = icon;
	this->tool_icon_disp_start = 0;
	this->tool_icon_disp_end = 0;
	set_fenstergroesse( icon );
    dirty = true;
}


/**
 * Add a new tool with values and tooltip text.
 * w must be created by new werkzeug_t(copy_tool)!
 * @author Hj. Malthaner
 */
void werkzeug_waehler_t::add_werkzeug(werkzeug_t *w)
{
	if(w->get_icon(welt->get_active_player())==IMG_LEER  &&  w!=werkzeug_t::dummy) {
		return;
	}

	// only for non-empty icons ...
	tools.append(w);

	int ww = (display_get_width()/icon.x)-2;
	tool_icon_width = tools.get_count();
DBG_DEBUG("werkzeug_waehler_t::add_tool()","ww=%i, tool_icon_width=%i",ww,tool_icon_width);
	if(allow_break  &&  (ww<tool_icon_width  ||  (umgebung_t::toolbar_max_width>0  &&  umgebung_t::toolbar_max_width<tool_icon_width))) {
		//break them
		int rows = (tool_icon_width/ww)+1;
DBG_DEBUG("werkzeug_waehler_t::add_tool()","ww=%i, rows=%i",ww,rows);
		// assure equal distribution if more than a single row is needed
		tool_icon_width = (tool_icon_width+rows-1)/rows;
		if(  umgebung_t::toolbar_max_width > 0  ) {
			// At least, 3 rows is needed to drag toolbar
			tool_icon_width = min( tool_icon_width, max(umgebung_t::toolbar_max_width, 3) );
		}
  	}
	tool_icon_height = max( (display_get_height()/icon.y)-3, 1 );
	if(  umgebung_t::toolbar_max_height > 0  ) {
		tool_icon_height = min(tool_icon_height, umgebung_t::toolbar_max_height);
	}
	dirty = true;
	gui_frame_t::set_fenstergroesse( koord( tool_icon_width*icon.x, min(tool_icon_height, ((tools.get_count()-1)/tool_icon_width)+1)*icon.y+16 ) );
	tool_icon_disp_start = 0;
	tool_icon_disp_end = min( tool_icon_disp_start+tool_icon_width*tool_icon_height, tools.get_count() );
	has_prev_next = (tool_icon_width*tool_icon_height < tools.get_count());

DBG_DEBUG("werkzeug_waehler_t::add_tool()", "at position %i (width %i)", tools.get_count(), tool_icon_width);
}


// reset the tools to empty state
void werkzeug_waehler_t::reset_tools()
{
	welt->set_dirty();
	for(  int i=tools.get_count();  i>0;  ) {
		i--;
		tools.remove_at(i);
	}
	gui_frame_t::set_fenstergroesse( koord( icon.x, TITLEBAR_HEIGHT ) );
	tool_icon_width = 0;
	tool_icon_disp_start = 0;
	tool_icon_disp_end = 0;
}


bool werkzeug_waehler_t::getroffen(int x, int y)
{
	int dx = x/icon.x;
	int	dy = (y-16)/icon.y;
	if(  x>=0 && dx<tool_icon_width  &&  y>=0  &&  (y<TITLEBAR_HEIGHT  ||  dy<tool_icon_height)  ) {
		return y < TITLEBAR_HEIGHT || dx + tool_icon_width * dy + tool_icon_disp_start < (int)tools.get_count();
	}
	return false;
}


bool werkzeug_waehler_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTRELEASE(ev)) {
		// tooltips?
		const int x = (ev->mx) / icon.x;
		const int y = (ev->my-16) / icon.y;

		if(x>=0 && x<tool_icon_width  &&  y>=0) {
			const int wz_idx = x+(tool_icon_width*y)+tool_icon_disp_start;

			if (wz_idx < (int)tools.get_count()) {
				welt->set_werkzeug( tools[wz_idx], welt->get_active_player() );
			}
			dirty = true;
			return true;
		}
	}
	/* this resets to query-tool, when closing toolsbar ... */
	else if(ev->ev_class==INFOWIN &&  ev->ev_code==WIN_CLOSE) {
		welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], welt->get_active_player() );
	}
	if(IS_WINDOW_CHOOSE_NEXT(ev)) {
		if(ev->ev_code==NEXT_WINDOW) {
			tool_icon_disp_start = (tool_icon_disp_start+tool_icon_width*tool_icon_height >= (int)tools.get_count()) ? tool_icon_disp_start : tool_icon_disp_start+tool_icon_width*tool_icon_height;
		}
		else {
			tool_icon_disp_start = (tool_icon_disp_start-tool_icon_width*tool_icon_height < 0) ? 0 : tool_icon_disp_start-tool_icon_width*tool_icon_height;
		}

		int xy = tool_icon_width*tool_icon_height;
		tool_icon_disp_end = min(tool_icon_disp_start+xy, tools.get_count());
		if(tool_icon_disp_end-tool_icon_disp_start<xy) {
			// Needs this to redraw empty space ?
			welt->set_dirty();
		}
		koord last = groesse;
		groesse = koord( tool_icon_width*icon.x, min(tool_icon_height, ((tools.get_count()-1-tool_icon_disp_start)/tool_icon_width+1))*icon.y+16);
		dirty = true;
	}
	return false;
}


void werkzeug_waehler_t::zeichnen(koord pos, koord)
{
	spieler_t *sp = welt->get_active_player();
	for(  uint i = tool_icon_disp_start;  i < tool_icon_disp_end;  i++  ) {
		const image_id icon_img = tools[i]->get_icon(sp);

		const koord draw_pos=pos+koord(((i-tool_icon_disp_start)%tool_icon_width)*icon.x,16+((i-tool_icon_disp_start)/tool_icon_width)*icon.y);
		if(icon_img == IMG_LEER) {
			// Hajo: no icon image available, draw a blank
			// DDD box as replacement

			// top
			display_fillbox_wh(draw_pos.x, draw_pos.y, icon.x, 1, MN_GREY4, dirty);
			// body
			display_fillbox_wh(draw_pos.x+1, draw_pos.y+1, icon.x-2, icon.y-2, MN_GREY2, dirty);
			// bottom
			display_fillbox_wh(draw_pos.x, draw_pos.y+icon.y-1, icon.x, 1, MN_GREY0, dirty);
			// Left
			display_fillbox_wh(draw_pos.x, draw_pos.y, 1, icon.y, MN_GREY4, dirty);
			// Right
			display_fillbox_wh(draw_pos.x+icon.x-1, draw_pos.y, 1, icon.y, MN_GREY0, dirty);
		}
		else {
			display_color_img(icon_img, draw_pos.x, draw_pos.y, 0, false, dirty);
			tools[i]->draw_after( welt, draw_pos );
		}
	}

	// tooltips?
	const sint16 mx = get_maus_x();
	const sint16 my = get_maus_y();
	const sint16 xdiff = (mx - pos.x) / icon.x;
	const sint16 ydiff = (my - pos.y - 16) / icon.y;
	if(xdiff>=0  &&  xdiff<tool_icon_width  &&  ydiff>=0  &&  mx>=pos.x  &&  my>=pos.y+16) {
		const int tipnr = xdiff+(tool_icon_width*ydiff)+tool_icon_disp_start;
		if (tipnr < (int)tool_icon_disp_end) {
			win_set_tooltip(get_maus_x() + 16, pos.y + 16 + ((ydiff+1)*icon.y) + 12, tools[tipnr]->get_tooltip(welt->get_active_player()), tools[tipnr], this);
		}
	}

	dirty = false;
}
