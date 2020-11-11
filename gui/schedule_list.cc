/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "messagebox.h"
#include "schedule_list.h"
#include "line_management_gui.h"
#include "components/gui_convoiinfo.h"
#include "line_item.h"
#include "simwin.h"

#include "../simcolor.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simworld.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../simskin.h"
#include "../simconvoi.h"
#include "../vehicle/simvehicle.h"
#include "../simlinemgmt.h"
#include "../simmenu.h"
#include "../utils/simstring.h"
#include "../player/simplay.h"

#include "../bauer/vehikelbauer.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../boden/wege/kanal.h"
#include "../boden/wege/maglev.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/narrowgauge.h"
#include "../boden/wege/runway.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/strasse.h"

#include "../unicode.h"


#include "minimap.h"

static uint8 tabs_to_lineindex[9];
static uint8 max_idx=0;

static int current_sort_mode = 0;

/// selected tab per player
static uint8 selected_tab[MAX_PLAYER_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/// selected line per tab (static)
linehandle_t schedule_list_gui_t::selected_line[MAX_PLAYER_COUNT][simline_t::MAX_LINE_TYPE];

schedule_list_gui_t::schedule_list_gui_t(player_t *player_) :
	gui_frame_t( translator::translate("Line Management"), player_),
	player(player_),
	scl(gui_scrolled_list_t::listskin, line_scrollitem_t::compare)
{
	schedule_filter[0] = 0;
	old_schedule_filter[0] = 0;

	// add components
	// first column: scrolled list of all lines

	set_table_layout(1,0);

	add_table(2,0);
	{
		// below line list: line filter
		new_component<gui_label_t>( "Line Filter");

		inp_filter.set_text( schedule_filter, lengthof( schedule_filter ) );
		inp_filter.add_listener( this );
		add_component( &inp_filter );

		// sort by what
		sort_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Name"), SYSCOL_TEXT) ;
		sort_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Revenue"), SYSCOL_TEXT) ;
		sort_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Free Capacity"), SYSCOL_TEXT) ;
		sort_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Number of convois"), SYSCOL_TEXT) ;
		sort_type_c.set_selection(0);
		sort_type_c.set_focusable( true );
		sort_type_c.add_listener( this );
		add_component(&sort_type_c);

		// freight type filter
		viewable_freight_types.append(NULL);
		freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("All"), SYSCOL_TEXT) ;
		viewable_freight_types.append(goods_manager_t::passengers);
		freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Passagiere"), SYSCOL_TEXT) ;
		viewable_freight_types.append(goods_manager_t::mail);
		freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Post"), SYSCOL_TEXT) ;
		viewable_freight_types.append(goods_manager_t::none); // for all freight ...
		freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Fracht"), SYSCOL_TEXT) ;
		for(  int i = 0;  i < goods_manager_t::get_max_catg_index();  i++  ) {
			const goods_desc_t *freight_type = goods_manager_t::get_info_catg(i);
			const int index = freight_type->get_catg_index();
			if(  index == goods_manager_t::INDEX_NONE  ||  freight_type->get_catg()==0  ) {
				continue;
			}
			freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(freight_type->get_catg_name()), SYSCOL_TEXT);
			viewable_freight_types.append(freight_type);
		}
		for(  int i=0;  i < goods_manager_t::get_count();  i++  ) {
			const goods_desc_t *ware = goods_manager_t::get_info(i);
			if(  ware->get_catg() == 0  &&  ware->get_index() > 2  ) {
				// Special freight: Each good is special
				viewable_freight_types.append(ware);
				freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate(ware->get_name()), SYSCOL_TEXT) ;
			}
		}
		freight_type_c.set_selection(0);
		freight_type_c.set_focusable( true );
		freight_type_c.add_listener( this );
		add_component(&freight_type_c);

		bt_new_line.init( button_t::roundbox, "New Line" );
		bt_new_line.add_listener(this);
		add_component(&bt_new_line,2);
	}
	end_table();

	// init scrolled list
	scl.add_listener(this);

	// tab panel
	tabs.add_tab(&scl, translator::translate("All"));
	max_idx = 0;
	tabs_to_lineindex[max_idx++] = simline_t::line;

	// now add all specific tabs
	if(  maglev_t::default_maglev  ) {
		tabs.add_tab(&scl, translator::translate("Maglev"), skinverwaltung_t::maglevhaltsymbol, translator::translate("Maglev"));
		tabs_to_lineindex[max_idx++] = simline_t::maglevline;
	}
	if(  monorail_t::default_monorail  ) {
		tabs.add_tab(&scl, translator::translate("Monorail"), skinverwaltung_t::monorailhaltsymbol, translator::translate("Monorail"));
		tabs_to_lineindex[max_idx++] = simline_t::monorailline;
	}
	if(  schiene_t::default_schiene  ) {
		tabs.add_tab(&scl, translator::translate("Train"), skinverwaltung_t::zughaltsymbol, translator::translate("Train"));
		tabs_to_lineindex[max_idx++] = simline_t::trainline;
	}
	if(  narrowgauge_t::default_narrowgauge  ) {
		tabs.add_tab(&scl, translator::translate("Narrowgauge"), skinverwaltung_t::narrowgaugehaltsymbol, translator::translate("Narrowgauge"));
		tabs_to_lineindex[max_idx++] = simline_t::narrowgaugeline;
	}
	if(  !vehicle_builder_t::get_info(tram_wt).empty()  ) {
		tabs.add_tab(&scl, translator::translate("Tram"), skinverwaltung_t::tramhaltsymbol, translator::translate("Tram"));
		tabs_to_lineindex[max_idx++] = simline_t::tramline;
	}
	if(  strasse_t::default_strasse) {
		tabs.add_tab(&scl, translator::translate("Truck"), skinverwaltung_t::autohaltsymbol, translator::translate("Truck"));
		tabs_to_lineindex[max_idx++] = simline_t::truckline;
	}
	if(  !vehicle_builder_t::get_info(water_wt).empty()  ) {
		tabs.add_tab(&scl, translator::translate("Ship"), skinverwaltung_t::schiffshaltsymbol, translator::translate("Ship"));
		tabs_to_lineindex[max_idx++] = simline_t::shipline;
	}
	if(  runway_t::default_runway  ) {
		tabs.add_tab(&scl, translator::translate("Air"), skinverwaltung_t::airhaltsymbol, translator::translate("Air"));
		tabs_to_lineindex[max_idx++] = simline_t::airline;
	}
	tabs.add_listener(this);
	add_component(&tabs);


	// recover last selected line
	int index = 0;
	for(  uint i=0;  i<max_idx;  i++  ) {
		if(  tabs_to_lineindex[i] == selected_tab[player->get_player_nr()]  ) {
			selected_line[player->get_player_nr()][selected_tab[player->get_player_nr()]];
			index = i;
			break;
		}
	}
	selected_tab[player->get_player_nr()] = tabs_to_lineindex[index]; // reset if previous selected tab is not there anymore
	tabs.set_active_tab_index(index);

	build_line_list(index);

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


bool schedule_list_gui_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	if(  comp == &bt_new_line  ) {
		// create typed line
		assert(  tabs.get_active_tab_index() > 0  &&  tabs.get_active_tab_index()<max_idx  );
		// update line schedule via tool!
		tool_t *tmp_tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
		cbuffer_t buf;
		int type = tabs_to_lineindex[tabs.get_active_tab_index()];
		buf.printf( "c,0,%i,0,0|%i|", type, type );
		tmp_tool->set_default_param(buf);
		welt->set_tool( tmp_tool, player );
		// since init always returns false, it is safe to delete immediately
		delete tmp_tool;
		depot_t::update_all_win();
	}
	else if(  comp == &tabs  ) {
		int const tab = tabs.get_active_tab_index();
		uint8 old_selected_tab = selected_tab[player->get_player_nr()];
		selected_tab[player->get_player_nr()] = tabs_to_lineindex[tab];
		if(  old_selected_tab == simline_t::line  &&  selected_line[player->get_player_nr()][0].is_bound()  &&
			selected_line[player->get_player_nr()][0]->get_linetype() == selected_tab[player->get_player_nr()]  ) {
				// switching from general to same waytype tab while line is seletced => use current line instead
				selected_line[player->get_player_nr()][selected_tab[player->get_player_nr()]] = selected_line[player->get_player_nr()][0];
		}
		build_line_list(tab);
		if(  tab>0  ) {
			bt_new_line.enable();
		}
		else {
			bt_new_line.disable();
		}
	}
	else if(  comp == &scl  ) {
		if(  line_scrollitem_t *li=(line_scrollitem_t *)scl.get_element(v.i)  ) {
			line = li->get_line();
		}
		player->simlinemgmt.show_lineinfo( player, line );
		selected_line[player->get_player_nr()][selected_tab[player->get_player_nr()]] = line;
		selected_line[player->get_player_nr()][0] = line; // keep these the same in overview
	}
	else if(  comp == &inp_filter  ) {
		if(  strcmp(old_schedule_filter,schedule_filter)  ) {
			build_line_list(tabs.get_active_tab_index());
			strcpy(old_schedule_filter,schedule_filter);
		}
	}
	else if(  comp == &freight_type_c  ) {
		build_line_list(tabs.get_active_tab_index());
	}
	else if(  comp == &sort_type_c  ) {
		build_line_list(tabs.get_active_tab_index());
	}

	return true;
}


void schedule_list_gui_t::draw(scr_coord pos, scr_size size)
{
	// deativate buttons, if not curretn player
	const bool activate = player != welt->get_active_player()  ||   welt->get_active_player()==welt->get_player( 1 );
	bt_new_line.enable( activate  &&  tabs.get_active_tab_index() > 0);

	// if search string changed, update line selection
	if(  old_line_count != player->simlinemgmt.get_line_count()  ||  strcmp( old_schedule_filter, schedule_filter )  ) {
		old_line_count = player->simlinemgmt.get_line_count();
		build_line_list(tabs.get_active_tab_index());
		strcpy( old_schedule_filter, schedule_filter );
	}

	gui_frame_t::draw(pos, size);
}


void schedule_list_gui_t::build_line_list(int filter)
{
	vector_tpl<linehandle_t>lines;

	sint32 sel = -1;
	scl.clear_elements();
	player->simlinemgmt.get_lines(tabs_to_lineindex[filter], &lines);

	FOR(vector_tpl<linehandle_t>, const l, lines) {
		// search name
		if(  utf8caseutf8(l->get_name(), schedule_filter)  ) {
			// match good category
			if(  is_matching_freight_catg( l->get_goods_catg_index() )  ) {
				scl.new_component<line_scrollitem_t>(l);
			}
			if(  line == l  ) {
				sel = scl.get_count() - 1;
			}
		}
	}

	scl.set_selection( sel );
	line_scrollitem_t::sort_mode = (line_scrollitem_t::sort_modes_t)current_sort_mode;
	scl.sort( 0 );
	scl.set_size(scl.get_size());

	old_line_count = player->simlinemgmt.get_line_count();
}


uint32 schedule_list_gui_t::get_rdwr_id()
{
	return magic_line_management_t+player->get_player_nr();
}


void schedule_list_gui_t::rdwr( loadsave_t *file )
{
	scr_size size;
	sint32 cont_xoff, cont_yoff, halt_xoff, halt_yoff;
	if(  file->is_saving()  ) {
		size = get_windowsize();
		cont_xoff = scl.get_scroll_x();
		cont_yoff = scl.get_scroll_y();
	}
	size.rdwr( file );
	tabs.rdwr( file );
	simline_t::rdwr_linehandle_t(file, line);

	file->rdwr_long( cont_xoff );
	file->rdwr_long( cont_yoff );
	// open dialogue
	if(  file->is_loading()  ) {
		set_windowsize( size );
		build_line_list(tabs.get_active_tab_index());
		resize( scr_coord(0,0) );
		scl.set_scroll_position( cont_xoff, cont_yoff );
	}
}


// borrowed code from minimap
bool schedule_list_gui_t::is_matching_freight_catg(const minivec_tpl<uint8> &goods_catg_index)
{
	const goods_desc_t *line_freight_type_group_index = viewable_freight_types[ freight_type_c.get_selection() ];
	// does this line/convoi has a matching freight
	if(  line_freight_type_group_index == goods_manager_t::passengers  ) {
		return goods_catg_index.is_contained(goods_manager_t::INDEX_PAS);
	}
	else if(  line_freight_type_group_index == goods_manager_t::mail  ) {
		return goods_catg_index.is_contained(goods_manager_t::INDEX_MAIL);
	}
	else if(  line_freight_type_group_index == goods_manager_t::none  ) {
		// all freights but not pax or mail
		for(  uint8 i = 0;  i < goods_catg_index.get_count();  i++  ) {
			if(  goods_catg_index[i] > goods_manager_t::INDEX_NONE  ) {
				return true;
			}
		}
		return false;
	}
	else if(  line_freight_type_group_index != NULL  ) {
		for(  uint8 i = 0;  i < goods_catg_index.get_count();  i++  ) {
			if(  goods_catg_index[i] == line_freight_type_group_index->get_catg_index()  ) {
				return true;
			}
		}
		return false;
	}
	// all true
	return true;
}
