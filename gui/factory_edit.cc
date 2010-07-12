/*
 * Copyright (c) 1997 - 2004 Hansj�rg Malthaner
 *
 * Line management
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simcolor.h"
#include "../simtools.h"
#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simskin.h"
#include "../simwerkz.h"
#include "../simwin.h"

#include "components/list_button.h"

#include "../bauer/fabrikbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/intro_dates.h"

#include "../dataobj/translator.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "factory_edit.h"


#define LINE_NAME_COLUMN_WIDTH (int)((BUTTON_WIDTH*2.25)+11)
#define SCL_HEIGHT (170)
#define N_BUTTON_WIDTH  (int)(BUTTON_WIDTH*1.5)


wkz_build_industries_land_t factory_edit_frame_t::land_chain_tool = wkz_build_industries_land_t();
wkz_build_industries_city_t factory_edit_frame_t::city_chain_tool = wkz_build_industries_city_t();
wkz_build_factory_t factory_edit_frame_t::fab_tool = wkz_build_factory_t();
char factory_edit_frame_t::param_str[256];

static bool compare_fabrik_besch(const fabrik_besch_t* a, const fabrik_besch_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	return diff < 0;
}



factory_edit_frame_t::factory_edit_frame_t(spieler_t* sp_, karte_t* welt) :
	extend_edit_gui_t(translator::translate("factorybuilder"), sp_, welt),
	fablist(16),
	lb_rotation( rot_str, COL_WHITE, gui_label_t::right ),
	lb_rotation_info( translator::translate("Rotation"), COL_BLACK, gui_label_t::left ),
	lb_production_info( translator::translate("Produktion"), COL_BLACK, gui_label_t::left )
{
	rot_str[0] = 0;
	prod_str[0] = 0;
	land_chain_tool.set_default_param(param_str);
	city_chain_tool.set_default_param(param_str);
	fab_tool.set_default_param(param_str);
	land_chain_tool.cursor = city_chain_tool.cursor = fab_tool.cursor = werkzeug_t::general_tool[WKZ_BUILD_FACTORY]->cursor;

	fab_besch = NULL;

	bt_city_chain.init( button_t::square_state, "Only city chains", koord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_city_chain.add_listener(this);
	add_komponente(&bt_city_chain);
	offset_of_comp += BUTTON_HEIGHT;

	bt_land_chain.init( button_t::square_state, "Only land chains", koord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_land_chain.add_listener(this);
	add_komponente(&bt_land_chain);
	offset_of_comp += BUTTON_HEIGHT;

	lb_rotation_info.set_pos( koord( get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	add_komponente(&lb_rotation_info);

	bt_left_rotate.init( button_t::repeatarrowleft, NULL, koord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2-16,	offset_of_comp-4 ) );
	bt_left_rotate.add_listener(this);
	add_komponente(&bt_left_rotate);

	bt_right_rotate.init( button_t::repeatarrowright, NULL, koord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2+50, offset_of_comp-4 ) );
	bt_right_rotate.add_listener(this);
	add_komponente(&bt_right_rotate);

	lb_rotation.set_pos( koord( get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2+44, offset_of_comp-4 ) );
	add_komponente(&lb_rotation);
	offset_of_comp += BUTTON_HEIGHT;

	lb_production_info.set_pos( koord( get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	add_komponente(&lb_production_info);

	inp_production.set_pos(koord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2-16,	offset_of_comp-4 ));
	inp_production.set_groesse(koord( 76, 12 ));
	inp_production.set_limits(0,9999);
	inp_production.add_listener( this );
	add_komponente(&inp_production);

	offset_of_comp += BUTTON_HEIGHT;

	fill_list( is_show_trans_name );

	resize(koord(0,0));
}



// fill the current fablist
void factory_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const bool city_chain = bt_city_chain.pressed;
	const bool land_chain = bt_land_chain.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	fablist.clear();

	// timeline will be obeyed; however, we may show obsolete ones ...
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter(fabrikbauer_t::get_fabesch());
	while(iter.next()) {

		const fabrik_besch_t *besch = iter.get_current_value();
		if(besch->get_gewichtung()>0) {
			// DistributionWeight=0 is obsoluted item, only for backward compatibility

			if(!use_timeline  ||  (!besch->get_haus()->is_future(month_now)  &&  (!besch->get_haus()->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this

				if(city_chain) {
					if(besch->get_platzierung()==fabrik_besch_t::Stadt  &&  besch->get_produkt(0)==NULL) {
						fablist.insert_ordered( besch, compare_fabrik_besch );
					}
				}
				if(land_chain) {
					if(besch->get_platzierung()==fabrik_besch_t::Land  &&  besch->get_produkt(0)==NULL) {
						fablist.insert_ordered( besch, compare_fabrik_besch );
					}
				}
				if(!city_chain  &&  !land_chain) {
					fablist.insert_ordered( besch, compare_fabrik_besch );
				}
			}
		}
	}

	// now buil scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	for(  uint i=0;  i<fablist.get_count();  i++  ) {
		// color code for objects: BLACK: normal, YELLOW: consumer only, GREEN: source only
		COLOR_VAL color=COL_BLACK;
		if(fablist[i]->get_produkt(0)==NULL) {
			color = COL_BLUE;
		}
		else if(fablist[i]->get_lieferant(0)==NULL) {
			color = COL_DARK_GREEN;
		}
		scl.append_element( new gui_scrolled_list_t::const_text_scrollitem_t(
			translate ? translator::translate( fablist[i]->get_name() ):fablist[i]->get_name(),
			color )
		);
		if(fablist[i]==fab_besch) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );
}



bool factory_edit_frame_t::action_triggered( gui_action_creator_t *komp,value_t e)
{
	// only one chain can be shown
	if(  komp==&bt_city_chain  ) {
		bt_city_chain.pressed ^= 1;
		if(bt_city_chain.pressed) {
			bt_land_chain.pressed = 0;
		}
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_land_chain  ) {
		bt_land_chain.pressed ^= 1;
		if(bt_land_chain.pressed) {
			bt_city_chain.pressed = 0;
		}
		fill_list( is_show_trans_name );
	}
	else if(fab_besch) {
		if (komp==&inp_production) {
			production = inp_production.get_value();
		}
		else if(  komp==&bt_left_rotate  &&  rotation!=255) {
			if(rotation==0) {
				rotation = 255;
			}
			else {
				rotation --;
			}
		}
		else if(  komp==&bt_right_rotate  &&  rotation!=fab_besch->get_haus()->get_all_layouts()-1) {
			rotation ++;
		}
		// update info ...
		change_item_info( scl.get_selection() );
	}
	return extend_edit_gui_t::action_triggered(komp,e);
}



void factory_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)fablist.get_count()) {

		const fabrik_besch_t *new_fab_besch = fablist[entry];
		if(new_fab_besch!=fab_besch) {

			fab_besch = new_fab_besch;
			production = fab_besch->get_produktivitaet() + sim_async_rand( fab_besch->get_bereich() );
			// Knightly : should also consider the effects of the minimum number of fields
			const field_besch_t *const field_besch = fab_besch->get_field();
			if(  field_besch  &&  field_besch->get_field_class_count()>0  ) {
				const weighted_vector_tpl<uint16> &field_class_indices = field_besch->get_field_class_indices();
				sint32 min_fields = field_besch->get_min_fields();
				while(  min_fields-- > 0  ) {
					const uint16 field_class_index = field_class_indices.at_weight( sim_async_rand( field_class_indices.get_sum_weight() ) );
					production += field_besch->get_field_class(field_class_index)->get_field_production();
				}
			}
			production <<= (welt->ticks_per_world_month_shift-18);
			inp_production.set_value( production);
			// show produced goods
			buf.clear();
			if(fab_besch->get_produkte()>0) {
				buf.append( translator::translate("Produktion") );
				buf.append("\n");
				for (uint i = 0; i < fab_besch->get_produkte(); i++) {
					buf.append(" - ");
					buf.append( translator::translate(fab_besch->get_produkt(i)->get_ware()->get_name()) );
					buf.append( " (" );
					buf.append( translator::translate(fab_besch->get_produkt(i)->get_ware()->get_catg_name()) );
					buf.append( ")\n" );
				}
				buf.append("\n");
			}

			// show consumed goods
			if(fab_besch->get_lieferanten()>0) {
				buf.append( translator::translate("Verbrauch") );
				buf.append("\n");
				for(  int i=0;  i<fab_besch->get_lieferanten();  i++  ) {
					buf.append(" - ");
					buf.append( translator::translate(fab_besch->get_lieferant(i)->get_ware()->get_name()) );
					buf.append( " (" );
					buf.append( translator::translate(fab_besch->get_lieferant(i)->get_ware()->get_catg_name()) );
					buf.append( ")\n" );
				}
				buf.append("\n");
			}

			if(fab_besch->is_electricity_producer()) {
				buf.append( translator::translate( "Electricity producer\n\n" ) );
			}

			// now the house stuff
			const haus_besch_t *besch = fab_besch->get_haus();

			// climates
			buf.append( translator::translate("allowed climates:\n") );
			uint16 cl = besch->get_allowed_climate_bits();
			if(cl==0) {
				buf.append( translator::translate("none") );
				buf.append("\n");
			}
			else {
				for(uint16 i=0;  i<=arctic_climate;  i++  ) {
					if(cl &  (1<<i)) {
						buf.append(" - ");
						buf.append(translator::translate(grund_besch_t::get_climate_name_from_bit((climate)i)));
						buf.append("\n");
					}
				}
			}
			buf.append("\n");

			buf.append(translator::translate("Passagierrate"));
			buf.append(": ");
			buf.append(fablist[entry]->get_pax_level());
			buf.append("\n");

			buf.append(translator::translate("Postrate"));
			buf.append(": ");
			buf.append(fablist[entry]->get_pax_level());
			buf.append("\n");

			buf.append(translator::translate("\nBauzeit von"));
			buf.append(besch->get_intro_year_month()/12);
			if(besch->get_retire_year_month()!=DEFAULT_RETIRE_DATE*12) {
				buf.append(translator::translate("\nBauzeit bis"));
				buf.append(besch->get_retire_year_month()/12);
			}

			const char *maker=besch->get_copyright();
			if(maker!=NULL  && maker[0]!=0) {
				buf.append("\n");
				buf.printf(translator::translate("Constructed by %s"), maker);
			}
			buf.append("\n");
			info_text.recalc_size();
			cont.set_groesse( info_text.get_groesse() + koord(0, 20) );

			// orientation (255=random)
			if(besch->get_all_layouts()>1) {
				rotation = 255; // no definition yet
			}
			else {
				rotation = 0;
			}

			// now for the tool
			fab_besch = fablist[entry];
		}

		// change lable numbers
		if(rotation == 255) {
			tstrncpy(rot_str, translator::translate("random"), lengthof(rot_str));
		}
		else {
			sprintf( rot_str, "%i", rotation );
		}

		// now the images (maximum is 2x2 size)
		// since these may be affected by rotation, we do this every time ...
		for(int i=0;  i<4;  i++  ) {
			img[i].set_image( IMG_LEER );
		}

		const haus_besch_t *besch = fab_besch->get_haus();
		uint8 rot = (rotation==255) ? 0 : rotation;
		if(besch->get_b(rot)==1) {
			if(besch->get_h(rot)==1) {
				img[3].set_image( besch->get_tile(rot,0,0)->get_hintergrund(0,0,0) );
			}
			else {
				img[2].set_image( besch->get_tile(rot,0,0)->get_hintergrund(0,0,0) );
				img[3].set_image( besch->get_tile(rot,0,1)->get_hintergrund(0,0,0) );
			}
		}
		else {
			if(besch->get_h(rot)==1) {
				img[1].set_image( besch->get_tile(rot,0,0)->get_hintergrund(0,0,0) );
				img[3].set_image( besch->get_tile(rot,1,0)->get_hintergrund(0,0,0) );
			}
			else {
				// maximum 2x2 image
				for(int i=0;  i<4;  i++  ) {
					img[i].set_image( besch->get_tile(rot,i/2,i&1)->get_hintergrund(0,0,0) );
				}
			}
		}

		// the tools will be always updated, even though the data up there might be still current
		sprintf( param_str, "%i%c%i,%s", bt_climates.pressed, rotation==255 ? '#' : '0'+rotation, production, fab_besch->get_name() );
		if(bt_land_chain.pressed) {
			welt->set_werkzeug( &land_chain_tool, sp );
		}
		else if(bt_city_chain.pressed) {
			welt->set_werkzeug( &city_chain_tool, sp );
		}
		else {
			welt->set_werkzeug( &fab_tool, sp );
		}
	}
	else if(fab_besch!=NULL) {
		for(int i=0;  i<4;  i++  ) {
			img[i].set_image( IMG_LEER );
		}
		buf.clear();
		prod_str[0] = 0;
		tstrncpy(rot_str, translator::translate("random"), lengthof(rot_str));
		fab_besch = NULL;
		welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], sp );
	}
}
