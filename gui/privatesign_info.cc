/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "privatesign_info.h"
#include "../obj/roadsign.h"
#include "../player/simplay.h"


#include "../simmenu.h"
#include "../simworld.h"

privatesign_info_t::privatesign_info_t(roadsign_t* s) :
	obj_infowin_t(s),
	sign(s)
{
	for(  int i=0;  i<PLAYER_UNOWNED;  i++  ) {
		if(  welt->get_player(i)  ) {
			players[i].init( button_t::square_state, welt->get_player(i)->get_name());
			players[i].add_listener( this );
		}
		else {
			players[i].init( button_t::square_state, "");
			players[i].disable();
		}
		players[i].pressed = (i>=8? sign->get_ticks_ow() & (1<<(i-8)) : sign->get_ticks_ns() & (1<<i) )!=0;
		add_component( &players[i] );
	}
	recalc_size();
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
   */
bool privatesign_info_t::action_triggered( gui_action_creator_t *comp, value_t /* */)
{
	if(  welt->get_active_player() ==  sign->get_owner()  ) {
		char param[256];
		for(  int i=0;  i<PLAYER_UNOWNED;  i++  ) {
			if(comp == &players[i]) {
				uint16 mask = sign->get_player_mask();
				mask ^= 1 << i;
				// change active player mask for this private sign
				if(  i<8  ) {
					sprintf( param, "%s,1,%i", sign->get_pos().get_str(), mask & 0x00FF );
				}
				else {
					sprintf( param, "%s,0,%i", sign->get_pos().get_str(), mask >> 8 );
				}
				tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT]->set_default_param( param );
				welt->set_tool( tool_t::simple_tool[TOOL_CHANGE_TRAFFIC_LIGHT], welt->get_active_player() );
				players[i].pressed = (mask >> i)&1;
			}
		}
	}
	return true;
}


// notify for an external update
void privatesign_info_t::update_data()
{
	for(  int i=0;  i<PLAYER_UNOWNED;  i++  ) {
		players[i].pressed = (i>=8? sign->get_ticks_ow() & (1<<(i-8)) : sign->get_ticks_ns() & (1<<i) )!=0;
	}
}
