/*
 * Copyright (c) 1997 - 2004 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * The citybuilding editor (urban buildings builder)
 */

#include <algorithm>
#include <stdio.h>

#include "../simworld.h"
#include "../simtool.h"

#include "../bauer/hausbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/intro_dates.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simrandom.h"
#include "../utils/simstring.h"

#include "citybuilding_edit.h"


// new tool definition
tool_build_house_t citybuilding_edit_frame_t::haus_tool=tool_build_house_t();
char citybuilding_edit_frame_t::param_str[256];



static bool compare_haus_besch(const haus_besch_t* a, const haus_besch_t* b)
{
	int diff = a->get_level()-b->get_level();
	if(  diff==0  ) {
		diff = a->get_typ()-b->get_typ();
	}
	if(  diff==0  ) {
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}



static bool compare_haus_besch_trans(const haus_besch_t* a, const haus_besch_t* b)
{
	int diff = strcmp( translator::translate(a->get_name()), translator::translate(b->get_name()) );
	if(  diff==0  ) {
		diff = a->get_level()-b->get_level();
	}
	if(  diff==0  ) {
		diff = a->get_typ()-b->get_typ();
	}
	return diff < 0;
}



citybuilding_edit_frame_t::citybuilding_edit_frame_t(player_t* player_) :
	extend_edit_gui_t(translator::translate("citybuilding builder"), player_),
	hauslist(16),
	lb_rotation( rot_str, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right ),
	lb_rotation_info( translator::translate("Rotation"), SYSCOL_TEXT, gui_label_t::left )
{
	rot_str[0] = 0;
	besch = NULL;
	haus_tool.set_default_param(NULL);
	haus_tool.cursor = tool_t::general_tool[TOOL_BUILD_HOUSE]->cursor;

	bt_res.init( button_t::square_state, "residential house", scr_coord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_res.add_listener(this);
	bt_res.pressed = true;
	add_component(&bt_res);
	offset_of_comp += D_BUTTON_HEIGHT;

	bt_com.init( button_t::square_state, "shops and stores", scr_coord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_com.add_listener(this);
	bt_com.pressed = true;
	add_component(&bt_com);
	offset_of_comp += D_BUTTON_HEIGHT;

	bt_ind.init( button_t::square_state, "industrial building", scr_coord(get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	bt_ind.add_listener(this);
	add_component(&bt_ind);
	bt_com.pressed = true;
	offset_of_comp += D_BUTTON_HEIGHT;

	lb_rotation_info.set_pos( scr_coord( get_tab_panel_width()+2*MARGIN, offset_of_comp-4 ) );
	add_component(&lb_rotation_info);

	bt_left_rotate.init( button_t::repeatarrowleft, NULL, scr_coord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2-16,	offset_of_comp-4 ) );
	bt_left_rotate.add_listener(this);
	add_component(&bt_left_rotate);

	bt_right_rotate.init( button_t::repeatarrowright, NULL, scr_coord(get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2+50, offset_of_comp-4 ) );
	bt_right_rotate.add_listener(this);
	add_component(&bt_right_rotate);

	//lb_rotation.set_pos( scr_coord( get_tab_panel_width()+2*MARGIN+COLUMN_WIDTH/2+44, offset_of_comp-4 ) );
	lb_rotation.set_width( bt_right_rotate.get_pos().x - bt_left_rotate.get_pos().x - bt_left_rotate.get_size().w );
	lb_rotation.align_to(&bt_left_rotate, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V);
	add_component(&lb_rotation);
	offset_of_comp += D_BUTTON_HEIGHT;

	fill_list( is_show_trans_name );

	resize(scr_coord(0,0));
}



// fill the current hauslist
void citybuilding_edit_frame_t::fill_list( bool translate )
{
	const bool allow_obsolete = bt_obsolete.pressed;
	const bool use_timeline = bt_timeline.pressed;
	const sint32 month_now = bt_timeline.pressed ? welt->get_current_month() : 0;

	hauslist.clear();

	if(bt_res.pressed) {
		FOR(vector_tpl<haus_besch_t const*>, const besch, *hausbauer_t::get_citybuilding_list(gebaeude_t::wohnung)) {
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.insert_ordered(besch, translate?compare_haus_besch_trans:compare_haus_besch );
			}
		}
	}

	if(bt_com.pressed) {
		FOR(vector_tpl<haus_besch_t const*>, const besch, *hausbauer_t::get_citybuilding_list(gebaeude_t::gewerbe)) {
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.insert_ordered(besch, translate?compare_haus_besch_trans:compare_haus_besch );
			}
		}
	}

	if(bt_ind.pressed) {
		FOR(vector_tpl<haus_besch_t const*>, const besch, *hausbauer_t::get_citybuilding_list(gebaeude_t::industrie)) {
			if(!use_timeline  ||  (!besch->is_future(month_now)  &&  (!besch->is_retired(month_now)  ||  allow_obsolete))  ) {
				// timeline allows for this
				hauslist.insert_ordered(besch, translate?compare_haus_besch_trans:compare_haus_besch );
			}
		}
	}

	// now build scrolled list
	scl.clear_elements();
	scl.set_selection(-1);
	FOR(vector_tpl<haus_besch_t const*>, const i, hauslist) {
		// color code for objects: BLACK: normal, YELLOW: consumer only, GREEN: source only
		COLOR_VAL color;
		switch (i->get_typ()) {
			case gebaeude_t::wohnung: color = COL_BLUE;       break;
			case gebaeude_t::gewerbe: color = COL_DARK_GREEN; break;
			default:                  color = SYSCOL_TEXT;      break;
		}
		char const* const name = translate ? translator::translate(i->get_name()) : i->get_name();
		scl.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(name, color));
		if (i == besch) {
			scl.set_selection(scl.get_count()-1);
		}
	}
	// always update current selection (since the tool may depend on it)
	change_item_info( scl.get_selection() );
}



bool citybuilding_edit_frame_t::action_triggered( gui_action_creator_t *komp,value_t e)
{
	// only one chain can be shown
	if(  komp==&bt_res  ) {
		bt_res.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_com  ) {
		bt_com.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(  komp==&bt_ind  ) {
		bt_ind.pressed ^= 1;
		fill_list( is_show_trans_name );
	}
	else if(besch) {
		if(  komp==&bt_left_rotate  &&  rotation!=254) {
			if(rotation==0) {
				rotation = 255;
			}
			else {
				rotation --;
			}
		}
		else if(  komp==&bt_right_rotate  &&  rotation!=besch->get_all_layouts()-1) {
			rotation ++;
		}
		// update info ...
		change_item_info( scl.get_selection() );
	}
	return extend_edit_gui_t::action_triggered(komp,e);
}



void citybuilding_edit_frame_t::change_item_info(sint32 entry)
{
	if(entry>=0  &&  entry<(sint32)hauslist.get_count()) {

		const haus_besch_t *new_besch = hauslist[entry];
		if(new_besch!=besch) {

			buf.clear();
			besch = new_besch;
			if(besch->get_typ()==gebaeude_t::wohnung) {
				buf.append( translator::translate( "residential house" ) );
			}
			else if(besch->get_typ()==gebaeude_t::gewerbe) {
				buf.append( translator::translate( "shops and stores" ) );
			}
			else if(besch->get_typ()==gebaeude_t::industrie) {
				buf.append( translator::translate( "industrial building" ) );
			}
			buf.append("\n\n");
			buf.append( translator::translate( besch->get_name() ) );

			buf.printf("\n\n%s: %i\n",translator::translate("Passagierrate"),besch->get_level());
			buf.printf("%s: %i\n",translator::translate("Postrate"),besch->get_post_level());

			buf.printf("%s%u", translator::translate("\nBauzeit von"), besch->get_intro_year_month() / 12);
			if(besch->get_retire_year_month()!=DEFAULT_RETIRE_DATE*12) {
				buf.printf("%s%u", translator::translate("\nBauzeit bis"), besch->get_retire_year_month() / 12);
			}
			buf.append("\n");

			if (char const* const maker = besch->get_copyright()) {
				buf.append("\n");
				buf.printf(translator::translate("Constructed by %s"), maker);
				buf.append("\n");
			}

			info_text.recalc_size();
			cont.set_size( info_text.get_size() + scr_size(0, 20) );

			// orientation (254=auto, 255=random)
			if(besch->get_all_layouts()>1) {
				rotation = 255; // no definition yet
			}
			else {
				rotation = 0;
			}
		}

		// change label numbers
		if(rotation == 255) {
			tstrncpy(rot_str, translator::translate("random"), lengthof(rot_str));
		}
		else if(rotation == 254) {
			tstrncpy(rot_str, translator::translate("auto"), lengthof(rot_str));
		}
		else {
			sprintf( rot_str, "%i", rotation );
		}

		// now the images (maximum is 2x2 size)
		// since these may be affected by rotation, we do this every time ...
		for(int i=0;  i<3;  i++  ) {
			img[i].set_image( IMG_EMPTY );
		}

		uint8 rot = (rotation>253) ? 0 : rotation;
		img[3].set_image( besch->get_tile(rot,0,0)->get_background(0,0,0) );

		// the tools will be always updated, even though the data up there might be still current
		sprintf( param_str, "%i%c%s", bt_climates.pressed, rotation>253 ? (rotation==254 ? 'A' : '#') : '0'+rotation, besch->get_name() );
		haus_tool.set_default_param(param_str);
		welt->set_tool( &haus_tool, player );
	}
	else if(welt->get_tool(player->get_player_nr())==&haus_tool) {
		besch = NULL;
		welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
	}
}
