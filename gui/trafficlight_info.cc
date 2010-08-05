/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "trafficlight_info.h"
#include "../dings/roadsign.h"

#include "../simmenu.h"

trafficlight_info_t::trafficlight_info_t(roadsign_t* s) :
	ding_infowin_t(s),
	ampel(s)
{
	ns.set_pos( koord(10,get_fenstergroesse().y-40) );
	ns.set_groesse( koord(60, 12) );
	ns.set_limits( 1, 255 );
	ns.set_value( s->get_ticks_ns() );
	ns.wrap_mode( false );
	ns.add_listener( this );
	add_komponente( &ns );

	ow.set_pos( koord(74,get_fenstergroesse().y-40) );
	ow.set_groesse( koord(60, 12) );
	ow.set_limits( 1, 255 );
	ow.set_value( s->get_ticks_ow() );
	ow.wrap_mode( false );
	ow.add_listener( this );
	add_komponente( &ow );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 *
 * Returns true, if action is done and no more
 * components should be triggered.
 * V.Meyer
   */
bool trafficlight_info_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	static char param[16];
	karte_t *welt = ampel->get_welt();
	if(komp == &ns) {
		sprintf( param, "%s,1,%i", ampel->get_pos().get_str(), (int)v.i );
		werkzeug_t::simple_tool[WKZ_TRAFFIC_LIGHT_TOOL]->set_default_param( param );
		welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_TRAFFIC_LIGHT_TOOL], welt->get_active_player() );
	}
	else if(komp == &ow) {
		sprintf( param, "%s,0,%i", ampel->get_pos().get_str(), (int)v.i );
		werkzeug_t::simple_tool[WKZ_TRAFFIC_LIGHT_TOOL]->set_default_param( param );
		welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_TRAFFIC_LIGHT_TOOL], welt->get_active_player() );
	}
	return true;
}


// notify for an external update
void trafficlight_info_t::update_data()
{
	ns.set_value( ampel->get_ticks_ns() );
	ow.set_value( ampel->get_ticks_ow() );
}
