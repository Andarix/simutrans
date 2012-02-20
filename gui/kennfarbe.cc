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
	gui_frame_t( translator::translate("Meldung"), sp ),
	txt(&buf),
	c1( "Your primary color:" ),
	c2( "Your secondary color:" ),
	bild( skinverwaltung_t::color_options->get_bild_nr(0), sp->get_player_nr() )
{
	this->sp = sp;
	buf.clear();
	buf.append(translator::translate("COLOR_CHOOSE\n"));

	txt.set_pos( koord(DIALOG_LEFT,DIALOG_TOP-LINESPACE) );
	txt.recalc_size();
	add_komponente( &txt );

	bild.set_pos( koord( (DIALOG_LEFT + 14*BUTTON_HEIGHT + 13*BUTTON_SPACER)-bild.get_groesse().x, DIALOG_TOP ) );
	add_komponente( &bild );

	KOORD_VAL y = DIALOG_TOP+DIALOG_SPACER + max( txt.get_groesse().y, bild.get_groesse().y )-LINESPACE;

	// player color 1
	c1.set_pos( koord(DIALOG_LEFT,y) );
	add_komponente( &c1 );
	y += LINESPACE+DIALOG_SPACER;
	// color buttons
	for(unsigned i=0;  i<28;  i++) {
		player_color_1[i].init( button_t::box_state, "", koord( DIALOG_LEFT+(i%14)*(BUTTON_HEIGHT+BUTTON_SPACER), y+(i/14)*(BUTTON_HEIGHT+BUTTON_SPACER) ), koord(BUTTON_HEIGHT,BUTTON_HEIGHT) );
		player_color_1[i].background = i*8+4;
		player_color_1[i].add_listener(this);
		add_komponente( player_color_1+i );
	}
	player_color_1[sp->get_player_color1()/8].pressed = true;
	y += 2*BUTTON_HEIGHT+BUTTON_SPACER+DIALOG_SPACER+LINESPACE;

	// player color 2
	c2.set_pos( koord(DIALOG_LEFT,y) );
	add_komponente( &c2 );
	y += LINESPACE+DIALOG_SPACER;
	// second color buttons
	for(unsigned i=0;  i<28;  i++) {
		player_color_2[i].init( button_t::box_state, "", koord( DIALOG_LEFT+(i%14)*(BUTTON_HEIGHT+BUTTON_SPACER), y+(i/14)*(BUTTON_HEIGHT+BUTTON_SPACER) ), koord(BUTTON_HEIGHT,BUTTON_HEIGHT) );
		player_color_2[i].background = i*8+4;
		player_color_2[i].add_listener(this);
		add_komponente( player_color_2+i );
	}
	player_color_2[sp->get_player_color2()/8].pressed = true;
	y += 2*BUTTON_HEIGHT+BUTTON_SPACER+DIALOG_BOTTOM;

	set_fenstergroesse( koord( DIALOG_LEFT + 14*BUTTON_HEIGHT + 13*BUTTON_SPACER + DIALOG_RIGHT, y+TITLEBAR_HEIGHT ) );
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
