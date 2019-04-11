/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string>
#include "../simcity.h"
#include "../simsys.h"
#include "simwin.h"

#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/tabfile.h"
#include "settings_frame.h"


#include "components/action_listener.h"

using std::string;

settings_frame_t::settings_frame_t(settings_t* const s) :
	gui_frame_t( translator::translate("Setting") ),
	sets(s),
	scrolly_general(&general, true, true),
	scrolly_display(&display, true, true),
	scrolly_economy(&economy, true, true),
	scrolly_routing(&routing, true, true),
	scrolly_costs(&costs, true, true),
	scrolly_climates(&climates, true, true)
{
	set_table_layout(1,0);
	add_table(2,1);
	{
		revert_to_default.init( button_t::roundbox, "Simuconf.tab");
		revert_to_default.add_listener( this );
		add_component( &revert_to_default );

		revert_to_last_save.init( button_t::roundbox, "Default.sve");
		revert_to_last_save.add_listener( this );
		add_component( &revert_to_last_save );
	}
	end_table();

	general.init( sets );
	display.init( sets );
	economy.init( sets );
	routing.init( sets );
	costs.init( sets );
	climates.init( sets );

	scrolly_general.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_economy.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_routing.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_costs.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_climates.set_scroll_amount_y(D_BUTTON_HEIGHT/2);

	tabs.add_tab(&scrolly_general, translator::translate("General"));
	tabs.add_tab(&scrolly_display, translator::translate("Helligk."));
	tabs.add_tab(&scrolly_economy, translator::translate("Economy"));
	tabs.add_tab(&scrolly_routing, translator::translate("Routing"));
	tabs.add_tab(&scrolly_costs, translator::translate("Costs"));
	tabs.add_tab(&scrolly_climates, translator::translate("Climate Control"));
	add_component(&tabs);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize() + general.get_min_size());
	set_resizemode(diagonal_resize);
}


 /* triggered, when button clicked; only single button registered, so the action is clear ... */
bool settings_frame_t::action_triggered( gui_action_creator_t *comp, value_t )
{
	if(  comp==&revert_to_default  ) {
		// reread from simucon.tab(s) the settings and apply them
		tabfile_t simuconf;
		env_t::init();
		*sets = settings_t();
		dr_chdir( env_t::program_dir );
		if(simuconf.open("config/simuconf.tab")) {
			sint16 dummy16;
			string dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
			sets->parse_colours( simuconf );
		}
		stadt_t::cityrules_init(env_t::objfilename);
		dr_chdir( env_t::program_dir );
		dr_chdir( env_t::objfilename.c_str() );
		if(simuconf.open("config/simuconf.tab")) {
			sint16 dummy16;
			string dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
			sets->parse_colours( simuconf );
		}
		dr_chdir(  env_t::user_dir  );
		if(simuconf.open("simuconf.tab")) {
			sint16 dummy16;
			string dummy_str;
			sets->parse_simuconf( simuconf, dummy16, dummy16, dummy16, dummy_str );
			sets->parse_colours( simuconf );
		}
		simuconf.close();

		// and update ...
		general.init( sets );
		display.init( sets );
		economy.init( sets );
		routing.init( sets );
		costs.init( sets );
		climates.init( sets );
		set_windowsize(get_windowsize());
	}
	else if(  comp==&revert_to_last_save  ) {
		// load settings of last generated map
		loadsave_t file;
		dr_chdir( env_t::user_dir  );
		if(  file.rd_open("default.sve")  ) {
			sets->rdwr(&file);
			file.close();
		}

		// and update ...
		general.init( sets );
		display.init( sets );
		economy.init( sets );
		routing.init( sets );
		costs.init( sets );
		climates.init( sets );
		set_windowsize(get_windowsize());
	}
	return true;
}



bool settings_frame_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
		general.read( sets );
		display.read( sets );
		routing.read( sets );
		economy.read( sets );
		costs.read( sets );
		climates.read( sets );

		// only the rgb colours have been changed, the colours in system format must be updated
		env_t_rgb_to_system_colors();
	}
	return gui_frame_t::infowin_event(ev);
}
