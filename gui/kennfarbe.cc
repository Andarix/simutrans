/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "../simevent.h"
#include "../simimg.h"
#include "../besch/skin_besch.h"
#include "../simskin.h"
#include "../dataobj/translator.h"
#include "kennfarbe.h"
#include "../player/simplay.h"



farbengui_t::farbengui_t(spieler_t *sp) :
	gui_frame_t("Meldung",sp),
	txt(&buf),
	bild(skinverwaltung_t::color_options->get_bild_nr(0),PLAYER_FLAG|sp->get_player_nr())
{
	this->sp = sp;
	buf.clear();
	buf.append(translator::translate("COLOR_CHOOSE\n"));
	set_fenstergroesse( koord(180, 17+6*28) );
	txt.set_pos( koord(10,9) );
	add_komponente( &txt );
	bild.set_pos( koord(25, 70) );
	add_komponente( &bild );
	// player color 1
	for(unsigned i=0;  i<28;  i++) {
		player_color_1[i].init( button_t::box_state, "", koord(130,i*6), koord(24,6) );
		player_color_1[i].background = i*8+4;
		player_color_1[i].add_listener(this);
		add_komponente( player_color_1+i );
	}
	player_color_1[sp->get_player_color1()/8].pressed = true;
	// player color 2
	for(unsigned i=0;  i<28;  i++) {
		player_color_2[i].init( button_t::box_state, "", koord(155,i*6), koord(24,6) );
		player_color_2[i].background = i*8+4;
		player_color_2[i].add_listener(this);
		add_komponente( player_color_2+i );
	}
	player_color_2[sp->get_player_color2()/8].pressed = true;
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool farbengui_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	for(unsigned i=0;  i<28;  i++) {
		// new player 1 color ?
		if(komp==player_color_1+i) {
			for(unsigned j=0;  j<28;  j++) {
				player_color_1[j].pressed = false;
			}
			player_color_1[i].pressed = true;
			sp->set_player_color( i*8, sp->get_player_color2() );
			return true;
		}
		// new player color 2?
		if(komp==player_color_2+i) {
			for(unsigned j=0;  j<28;  j++) {
				player_color_2[j].pressed = false;
			}
			player_color_2[i].pressed = true;
			sp->set_player_color( sp->get_player_color1(), i*8 );
			return true;
		}
	}
	return false;
}
