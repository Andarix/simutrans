/*
 * dialog for setting the climate border and other map related parameters
 *
 * prissi
 *
 * August 2006
 */

#include "climates.h"
#include "karte.h"
#include "welt.h"

#include "../besch/grund_besch.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../simwin.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

// just for their structure size ...
#include "../simgraph.h"

#include "../utils/simstring.h"
#include "components/list_button.h"

#define START_HEIGHT (28)

#define LEFT_ARROW (130)
#define RIGHT_ARROW (180)
#define TEXT_RIGHT (165) // 10 are offset in routine ..

#include <sys/stat.h>
#include <time.h>

/**
 * set the climate borders
 * @author prissi
 */

climate_gui_t::climate_gui_t(einstellungen_t* sets) :
	gui_frame_t("Climate Control")
{
DBG_MESSAGE("","sizeof(stat)=%d, sizeof(tm)=%d",sizeof(struct stat),sizeof(struct tm) );
	this->sets = sets;

	// select map stuff ..
	int intTopOfButton = 4;

	// mountian/water stuff
	water_level.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	water_level.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	water_level.set_limits( -10, 0 );
	water_level.set_value( sets->get_grundwasser() );
	water_level.wrap_mode( false );
	water_level.add_listener( this );
	add_komponente( &water_level );
	intTopOfButton += 12;

	mountain_height.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	mountain_height.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	mountain_height.set_limits( 0, 320 );
	mountain_height.set_increment_mode( 10 );
	mountain_height.set_value( (int)sets->get_max_mountain_height() );
	mountain_height.wrap_mode( false );
	mountain_height.add_listener( this );
	add_komponente( &mountain_height );
	intTopOfButton += 12;

	mountain_roughness.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	mountain_roughness.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
#ifndef DOUBLE_GROUNDS
	mountain_roughness.set_limits( 0, 7 );
#else
	mountain_roughness.set_limits( 0, 10 );
#endif
	mountain_roughness.set_value( (int)(sets->get_map_roughness()*20.0 + 0.5)-8 );
	mountain_roughness.wrap_mode( false );
	mountain_roughness.add_listener( this );
	add_komponente( &mountain_roughness );
	intTopOfButton += 12;

	// summer snowline alsway startig above highest climate
	intTopOfButton += 12+5;

	// artic starts at maximum end of climate
	snowline_winter.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	snowline_winter.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	snowline_winter.set_value( sets->get_winter_snowline() );
	snowline_winter.wrap_mode( false );
	snowline_winter.add_listener( this );
	add_komponente( &snowline_winter );
	intTopOfButton += 12+5;

	// other climate borders ...
	sint16 arctic = 0;
	for(  int i=desert_climate-1;  i<=rocky_climate-1;  i++  ) {

		climate_borders_ui[i].set_pos(koord(LEFT_ARROW,intTopOfButton) );
		climate_borders_ui[i].set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
		climate_borders_ui[i].set_limits( 0, 24 );
		climate_borders_ui[i].set_value( sets->get_climate_borders()[i+1] );
		climate_borders_ui[i].wrap_mode( false );
		climate_borders_ui[i].add_listener( this );
		add_komponente( climate_borders_ui+i );
		if(sets->get_climate_borders()[i]>arctic) {
			arctic = sets->get_climate_borders()[i];
		}
		intTopOfButton += 12;
	}
	snowline_winter.set_limits( 0, arctic );
	intTopOfButton += 5;

	no_tree.init( button_t::square, "no tree", koord(10,intTopOfButton) ); // right align
	no_tree.pressed = sets->get_no_trees();
	no_tree.add_listener( this );
	add_komponente( &no_tree );
	intTopOfButton += 12+4;

	river_n.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	river_n.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	river_n.add_listener(this);
	river_n.set_limits(0,1024);
	river_n.set_value( sets->get_river_number() );
	river_n.wrap_mode( false );
	add_komponente( &river_n );
	intTopOfButton += 12;

	river_min.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	river_min.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	river_min.set_limits(0,max(16,sets->get_max_river_length())-16);
	river_min.set_value( sets->get_min_river_length() );
	river_min.wrap_mode( false );
	river_min.add_listener(this);
	add_komponente( &river_min );
	intTopOfButton += 12;

	river_max.set_pos(koord(LEFT_ARROW,intTopOfButton) );
	river_max.set_groesse(koord(RIGHT_ARROW-LEFT_ARROW+10, 12));
	river_max.set_limits(sets->get_min_river_length()+16,1024);
	river_max.set_value( sets->get_max_river_length() );
	river_max.wrap_mode( false );
	river_max.add_listener(this);
	add_komponente( &river_max );
	intTopOfButton += 12;

	set_fenstergroesse( koord(RIGHT_ARROW+16, intTopOfButton+4+16) );
}





/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool climate_gui_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	welt_gui_t *welt_gui = dynamic_cast<welt_gui_t *>(win_get_magic( magic_welt_gui_t ));
	if(komp==&no_tree) {
		no_tree.pressed ^= 1;
		sets->set_no_trees(no_tree.pressed);
	}
	else if(komp==&water_level) {
		sets->grundwasser = (sint16)v.i;
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(komp==&mountain_height) {
		sets->max_mountain_height = v.i;
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(komp==&mountain_roughness) {
		sets->map_roughness = (double)(v.i+8)/20.0;
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(komp==&river_n) {
		sets->river_number = (sint16)v.i;
	}
	else if(komp==&river_min) {
		sets->min_river_length = (sint16)v.i;
		river_max.set_limits(v.i+16,1024);
	}
	else if(komp==&river_max) {
		sets->max_river_length = (sint16)v.i;
		river_min.set_limits(0,max(16,v.i)-16);
	}
	else if(komp==&snowline_winter) {
		sets->winter_snowline = (sint16)v.i;
	}
	else {
		// all climate borders from here on

		// artic starts at maximum end of climate
		sint16 arctic = 0;
		for(  int i=desert_climate;  i<=rocky_climate;  i++  ) {
			if(  komp==climate_borders_ui+i-1  ) {
				sets->climate_borders[i] = v.i;
			}
			if(sets->climate_borders[i]>arctic) {
				arctic = sets->climate_borders[i];
			}
		}
		sets->climate_borders[arctic_climate] = arctic;

		// correct summer snowline too
		if(arctic<sets->get_winter_snowline()) {
			sets->winter_snowline = arctic;
		}
		snowline_winter.set_limits( 0, arctic );
	}
	return true;
}




void climate_gui_t::zeichnen(koord pos, koord gr)
{
	no_tree.pressed = sets->get_no_trees();

	no_tree.set_text( "no tree" );

	gui_frame_t::zeichnen(pos, gr);

	const int x = pos.x+10;
	int y = pos.y+16+4;

	// water level       18-Nov-01       Markus W. Added
	display_proportional_clip(x, y, translator::translate("Water level"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Mountain height"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("Map roughness"), ALIGN_LEFT, COL_BLACK, true);
	y += 12+5;

	// season stuff
	display_proportional_clip(x, y, translator::translate("Summer snowline"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+TEXT_RIGHT, y, ntos( sets->get_climate_borders()[arctic_climate], 0) , ALIGN_RIGHT, COL_WHITE, true);     // x = round(roughness * 10)-4  // 0.6 * 10 - 4 = 2    //29-Nov-01     Markus W. Added
	y += 12;
	display_proportional_clip(x, y, translator::translate("Winter snowline"), ALIGN_LEFT, COL_BLACK, true);
	y += 12+5;

	// climate borders
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(desert_climate)), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(tropic_climate)), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(mediterran_climate)), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(temperate_climate)), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(tundra_climate)), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate(grund_besch_t::get_climate_name_from_bit(rocky_climate)), ALIGN_LEFT, COL_BLACK, true);
	y += 12;

	y += 12+5+5;	// no tree

	display_proportional_clip(x, y, translator::translate("Number of rivers"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("minimum length of rivers"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
	display_proportional_clip(x, y, translator::translate("maximum length of rivers"), ALIGN_LEFT, COL_BLACK, true);
	y += 12;
}
