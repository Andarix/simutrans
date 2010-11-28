/*
 * Copyright (c) 1997 - 2004 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Intro and everything else
 */

#include "../simcolor.h"
#include "../simevent.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../simwin.h"
#include "../simsys.h"
#include "../simversion.h"
#include "../simgraph.h"
#include "../macros.h"
#include "../besch/skin_besch.h"
#include "../dataobj/umgebung.h"

#include "components/list_button.h"
#include "banner.h"
#include "loadsave_frame.h"
#include "server_frame.h"


banner_t::banner_t( karte_t *w) : gui_frame_t(""),
	logo( skinverwaltung_t::logosymbol->get_bild_nr(0), 0 ),
	welt(w)
{
	last_ms = dr_time();
	line = 0;
	logo.set_pos( koord( 238, 40 ) );
	add_komponente( &logo );
	set_fenstergroesse( koord( BUTTON_WIDTH*3+40, 16+111+11*LINESPACE+2*BUTTON_HEIGHT+12 ) );
	new_map.init( button_t::roundbox, "Neue Karte", koord( 10, 232 ), koord( BUTTON_WIDTH, BUTTON_HEIGHT ) );
	new_map.add_listener( this );
	add_komponente( &new_map );
	load_map.init( button_t::roundbox, "Load game", koord( 10+BUTTON_WIDTH+10, 232 ), koord( BUTTON_WIDTH, BUTTON_HEIGHT ) );
	load_map.add_listener( this );
	add_komponente( &load_map );
	join_map.init( button_t::roundbox, "join game", koord( 10+2*BUTTON_WIDTH+20, 232 ), koord( BUTTON_WIDTH, BUTTON_HEIGHT ) );
	join_map.add_listener( this );
	add_komponente( &join_map );
	quit.init( button_t::roundbox, "Beenden", koord( 10+2*BUTTON_WIDTH+20, 232+BUTTON_HEIGHT+5 ), koord( BUTTON_WIDTH, BUTTON_HEIGHT ) );
	quit.add_listener( this );
	add_komponente( &quit );
}



bool banner_t::infowin_event(const event_t *ev)
{
	if(  gui_frame_t::getroffen( ev->cx, ev->cy  )  ) {
		gui_frame_t::infowin_event( ev );
	}
	else if(  ev->ev_class==EVENT_RELEASE  ||  (ev->ev_class==EVENT_KEYBOARD  &&  ev->ev_code!=0)  ) {
		destroy_win(this);
		return true;
	}
	return false;
}



bool banner_t::action_triggered( gui_action_creator_t *komp, value_t extra)
{
	if(  komp == &quit  ) {
		umgebung_t::quit_simutrans = true;
	}
	else if(  komp == &new_map  ) {
		destroy_win(this);
	}
	else if(  komp == &load_map  ) {
		create_win( new loadsave_frame_t(welt, true), w_info, magic_load_t);
		destroy_win(this);
	}
	else if(  komp == &join_map  ) {
		create_win( new server_frame_t(welt), w_info, magic_server_frame_t );
		destroy_win(this);
	}
	return true;
}


void banner_t::zeichnen(koord pos, koord gr )
{
	gui_frame_t::zeichnen( pos, gr );
	KOORD_VAL yp = pos.y+22;
	display_shadow_proportional( pos.x+10, yp, 7, COL_BLACK, "This is a beta version of Simutrans:", true );
	yp += LINESPACE+5;
#ifdef REVISION
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "Version " VERSION_NUMBER " " VERSION_DATE " r" QUOTEME(REVISION), true );
#else
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "Version " VERSION_NUMBER " " VERSION_DATE, true );
#endif
	yp += LINESPACE+7;

	display_shadow_proportional( pos.x+10, yp, 7, COL_BLACK, "This version is developed by", true );
	yp += LINESPACE+5;
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "the simutrans team, based on", true );
	yp += LINESPACE+2;
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "Simutrans 0.84.21.2 by", true );
	yp += LINESPACE+2;
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "Hansj�rg Malthaner et al.", true );
	yp += LINESPACE+2;
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "under Artistic Licence.", true );
	yp += LINESPACE+7;

	display_shadow_proportional( pos.x+10, yp, 7, COL_BLACK, "For questions and support please visit:", true );
	yp += LINESPACE+5;
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "http://www.simutrans.com", true );
	yp += LINESPACE+2;
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "http://forum.simutrans.com", true );
	yp += LINESPACE+2;
	display_shadow_proportional( pos.x+10+24, yp, COL_WHITE, COL_BLACK, "http://wiki.simutrans-germany.com/", true );
	yp += LINESPACE+7;

	// now the scrolling
	static const char* const scrolltext[] = {
#include "../scrolltext.h"
	};

	const KOORD_VAL text_line = (line / 9) * 2;
	const KOORD_VAL text_offset = line % 9;
	const KOORD_VAL left = pos.x+10;
	const KOORD_VAL width = gr.x-20;

	display_fillbox_wh(left, yp, width, 52, COL_GREY1, true);
	display_fillbox_wh(left, yp - 1, width, 1, COL_GREY3, false);
	display_fillbox_wh(left, yp + 52, width, 1, COL_GREY6, false);

	PUSH_CLIP( left, yp, width, 52 );
	display_proportional_clip( left + 4, yp + 1 - text_offset, scrolltext[text_line + 0], ALIGN_LEFT, COL_WHITE, false);
	display_proportional_clip( left + width - 4, yp + 1 - text_offset, scrolltext[text_line + 1], ALIGN_RIGHT, COL_WHITE, false);
	display_proportional( left + 4, yp + 11 - text_offset, scrolltext[text_line + 2], ALIGN_LEFT, COL_WHITE, false);
	display_proportional( left + width - 4, yp + 11 - text_offset, scrolltext[text_line + 3], ALIGN_RIGHT, COL_WHITE, false);
	display_proportional( left + 4, yp + 21 - text_offset, scrolltext[text_line + 4], ALIGN_LEFT, COL_GREY6, false);
	display_proportional( left + width - 4, yp + 21 - text_offset, scrolltext[text_line + 5], ALIGN_RIGHT, COL_GREY6, false);
	display_proportional( left + 4, yp + 31 - text_offset, scrolltext[text_line + 6], ALIGN_LEFT, COL_GREY5, false);
	display_proportional( left + width - 4, yp + 31 - text_offset, scrolltext[text_line + 7], ALIGN_RIGHT, COL_GREY5, false);
	display_proportional( left + 4, yp + 41 - text_offset, scrolltext[text_line + 8], ALIGN_LEFT, COL_GREY4, false);
	display_proportional( left + width - 4, yp + 41 - text_offset, scrolltext[text_line + 9], ALIGN_RIGHT, COL_GREY4, false);
	display_proportional_clip( left + 4, yp + 51 - text_offset, scrolltext[text_line + 10], ALIGN_LEFT, COL_GREY3, false);
	display_proportional_clip( left + width - 4, yp + 51 - text_offset, scrolltext[text_line + 11], ALIGN_RIGHT, COL_GREY3, false);
	POP_CLIP();

	// scroll on every 70 ms
	if(dr_time()>last_ms+70u) {
		last_ms += 70u;
		line ++;
	}

	if (scrolltext[text_line + 12] == 0) {
		line = 0;
	}
}
