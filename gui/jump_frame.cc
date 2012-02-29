/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * jumps to a position
 */

#include <string.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "jump_frame.h"
#include "components/gui_button.h"

#include "../dataobj/translator.h"


jump_frame_t::jump_frame_t(karte_t *welt) :
	gui_frame_t( translator::translate("Jump to") )
{
	this->welt = welt;

	// Input box for new name
	sprintf(buf, "%i,%i", welt->get_world_position().x, welt->get_world_position().y );
	input.set_text(buf, 62);
	input.add_listener(this);
	input.set_pos(koord(10,4));
	input.set_groesse(koord(D_BUTTON_WIDTH, 14));
	add_komponente(&input);

	divider1.set_pos(koord(10,24));
	divider1.set_groesse(koord(D_BUTTON_WIDTH,0));
	add_komponente(&divider1);

	jumpbutton.init( button_t::roundbox, "Jump to", koord(10, 28), koord( D_BUTTON_WIDTH,D_BUTTON_HEIGHT ) );
	jumpbutton.add_listener(this);
	add_komponente(&jumpbutton);

	set_focus(&input);
	set_fenstergroesse(koord(D_BUTTON_WIDTH+20, 62));
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool jump_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &input || komp == &jumpbutton) {
		// OK- Button or Enter-Key pressed
		//---------------------------------------
		koord my_pos;
		sscanf(buf, "%hd,%hd", &my_pos.x, &my_pos.y);
		if(welt->ist_in_kartengrenzen(my_pos)) {
			welt->change_world_position(koord3d(my_pos,welt->min_hgt(my_pos)));
		}
	}
	return true;
}
