/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simworld.h"
#include "../simwin.h"
#include "../simimg.h"
#include "../simware.h"
#include "../simfab.h"
#include "../simhalt.h"
#include "../simline.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "schedule_list.h"

#include "halt_detail.h"


halt_detail_t::halt_detail_t(halthandle_t halt_) :
	gui_frame_t(halt_->get_name(), halt_->get_besitzer()),
	halt(halt_),
	scrolly(&cont),
	txt_info(" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
		" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
		" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
		" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
		" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"),
	cb_info_buffer(8192)
{
	cont.add_komponente(&txt_info);

	// fill buffer with halt detail
	halt_detail_info(cb_info_buffer);
	txt_info.set_text(cb_info_buffer);
	txt_info.set_pos(koord(10,10));

	// calc window size
	const koord size = txt_info.get_groesse();
	if (size.y < 400) {
		set_fenstergroesse(koord(300, size.y + 32));
	}
	else {
		set_fenstergroesse(koord(300, 400));
	}

	// add scrollbar
	scrolly.set_pos(koord(1, 1));
	scrolly.set_groesse(get_fenstergroesse() + koord(-1, -17));

	add_komponente(&scrolly);
	cached_active_player=NULL;
}



halt_detail_t::~halt_detail_t()
{
	while(!posbuttons.empty()) {
		button_t *b = posbuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!linelabels.empty()) {
		gui_label_t *l = linelabels.remove_first();
		cont.remove_komponente( l );
		delete l;
	}
	while(!linebuttons.empty()) {
		button_t *b = linebuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!convoylabels.empty()) {
		gui_label_t *l = convoylabels.remove_first();
		cont.remove_komponente( l );
		delete l;
	}
	while(!convoybuttons.empty()) {
		button_t *b = convoybuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!label_names.empty()) {
		free( (void *)(label_names.remove_first()) );
	}
}



void halt_detail_t::halt_detail_info(cbuffer_t & buf)
{
	if (!halt.is_bound()) {
		return;
	}

	while(!posbuttons.empty()) {
		button_t *b = posbuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!linelabels.empty()) {
		gui_label_t *l = linelabels.remove_first();
		cont.remove_komponente( l );
		delete l;
	}
	while(!linebuttons.empty()) {
		button_t *b = linebuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!convoylabels.empty()) {
		gui_label_t *l = convoylabels.remove_first();
		cont.remove_komponente( l );
		delete l;
	}
	while(!convoybuttons.empty()) {
		button_t *b = convoybuttons.remove_first();
		cont.remove_komponente( b );
		delete b;
	}
	while(!label_names.empty()) {
		free( (void *)(label_names.remove_first()) );
	}
	buf.clear();

	const slist_tpl<fabrik_t *> & fab_list = halt->get_fab_list();
	slist_tpl<const ware_besch_t *> nimmt_an;

	sint16 offset_y = 22;
	buf.append(translator::translate("Fabrikanschluss"));
	buf.append("\n");
	offset_y += LINESPACE;

	if (!fab_list.empty()) {

		slist_iterator_tpl<fabrik_t *> fab_iter(fab_list);

		while(fab_iter.next()) {
			const fabrik_t * fab = fab_iter.get_current();
			const koord pos = fab->get_pos().get_2d();

			// target button ...
			button_t *pb = new button_t();
			pb->init( button_t::posbutton, NULL, koord(10, offset_y) );
			pb->set_targetpos( pos );
			pb->add_listener( this );
			posbuttons.append( pb );
			cont.add_komponente( pb );

			buf.append("   ");
			buf.append(translator::translate(fab->get_name()));
			buf.append(" (");
			buf.append(pos.x);
			buf.append(", ");
			buf.append(pos.y);
			buf.append(")\n");
			offset_y += LINESPACE;

			const array_tpl<ware_production_t>& eingang = fab->get_eingang();
			for (uint32 i = 0; i < eingang.get_count(); i++) {
				const ware_besch_t* ware = eingang[i].get_typ();

				if(!nimmt_an.is_contained(ware)) {
					nimmt_an.append(ware);
				}
			}
		}
	}
	else {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
		offset_y += LINESPACE;
	}

	buf.append("\n");
	offset_y += LINESPACE;

	buf.append(translator::translate("Angenommene Waren"));
	buf.append("\n");
	offset_y += LINESPACE;

	if (!nimmt_an.empty()  &&  halt->get_ware_enabled()) {
		for(uint32 i=0; i<warenbauer_t::get_waren_anzahl(); i++) {
			const ware_besch_t *ware = warenbauer_t::get_info(i);
			if(nimmt_an.is_contained(ware)) {

				buf.append(" - ");
				buf.append(translator::translate(ware->get_name()));
				buf.append("\n");
				offset_y += LINESPACE;
			}
		}
	}
	else {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
		offset_y += LINESPACE;
	}

	// add lines that serve this stop
	buf.append("\n");
	offset_y += LINESPACE;

	buf.append(translator::translate("Lines serving this stop"));
	buf.append("\n");
	offset_y += LINESPACE;

	if(  !halt->registered_lines.empty()  ) {
		for (unsigned int i = 0; i<halt->registered_lines.get_count(); i++) {
			// Line buttons only if owner ...
			if (halt->get_welt()->get_active_player()==halt->registered_lines[i]->get_besitzer()) {
				button_t *b = new button_t();
				b->init( button_t::posbutton, NULL, koord(10, offset_y) );
				b->set_targetpos( koord(-1,i) );
				b->add_listener( this );
				linebuttons.append( b );
				cont.add_komponente( b );
			}

			// Line labels with color of player
			label_names.append( strdup(halt->registered_lines[i]->get_name()) );
			gui_label_t *l = new gui_label_t( label_names.back(), PLAYER_FLAG|(halt->registered_lines[i]->get_besitzer()->get_player_color1()+0) );
			l->set_pos( koord(26, offset_y) );
			linelabels.append( l );
			cont.add_komponente( l );
			buf.append("\n");
			offset_y += LINESPACE;
		}
	}
	else {
		buf.append(" ");
		buf.append( translator::translate("keine") );
		buf.append("\n");
		offset_y += LINESPACE;
	}

	// Knightly : add lineless convoys which serve this stop
	buf.append("\n");
	offset_y += LINESPACE;

	buf.append( translator::translate("Lineless convoys serving this stop") );
	buf.append("\n");
	offset_y += LINESPACE;

	if(  !halt->registered_convoys.empty()  ) {
		for(  uint32 i=0;  i<halt->registered_convoys.get_count();  ++i  ) {
			// Convoy buttons
			button_t *b = new button_t();
			b->init( button_t::posbutton, NULL, koord(10, offset_y) );
			b->set_targetpos( koord(-2, i) );
			b->add_listener( this );
			convoybuttons.append( b );
			cont.add_komponente( b );

			// Line labels with color of player
			label_names.append( strdup(halt->registered_convoys[i]->get_name()) );
			gui_label_t *l = new gui_label_t( label_names.back(), PLAYER_FLAG|(halt->registered_convoys[i]->get_besitzer()->get_player_color1()+0) );
			l->set_pos( koord(26, offset_y) );
			convoylabels.append( l );
			cont.add_komponente( l );
			buf.append("\n");
			offset_y += LINESPACE;
		}
	}
	else {
		buf.append(" ");
		buf.append( translator::translate("keine") );
		buf.append("\n");
		offset_y += LINESPACE;
	}

	buf.append("\n");
	offset_y += LINESPACE;

	buf.append(translator::translate("Direkt erreichbare Haltestellen"));
	buf.append("\n");
	offset_y += LINESPACE;

	bool has_stops = false;

	for (uint i=0; i<warenbauer_t::get_max_catg_index(); i++){
		const vector_tpl<halthandle_t> *ziele = halt->get_warenziele(i);
		if(!ziele->empty()) {
			buf.append("\n");
			offset_y += LINESPACE;

			buf.append(" �");
			const ware_besch_t* info = warenbauer_t::get_info_catg_index(i);
			// If it is a special freight, we display the name of the good, otherwise the name of the category.
			buf.append(translator::translate(info->get_catg()==0?info->get_name():info->get_catg_name()));
			buf.append(":\n");
			offset_y += LINESPACE;

			for(  uint32 idx=0;  idx < ziele->get_count();  idx++  ) {
				halthandle_t a_halt = (*ziele)[idx];
				if(a_halt.is_bound()) {

					has_stops = true;

					buf.append("   ");
					buf.append(a_halt->get_name());

					// target button ...
					button_t *pb = new button_t();
					pb->init( button_t::posbutton, NULL, koord(10, offset_y) );
					pb->set_targetpos( a_halt->get_basis_pos() );
					pb->add_listener( this );
					posbuttons.append( pb );
					cont.add_komponente( pb );
				}

				buf.append("\n");
				offset_y += LINESPACE;
			}
		}
	}

	if (!has_stops) {
		buf.append(" ");
		buf.append(translator::translate("keine"));
		buf.append("\n");
	}
	buf.append("\n\n");

	txt_info.set_text(buf);
	txt_info.recalc_size();
	cont.set_groesse( txt_info.get_groesse() );

	// ok, we have now this counter for pending updates
	destination_counter = halt->get_rebuild_destination_counter();
	cached_line_count = halt->registered_lines.get_count();
	cached_convoy_count = halt->registered_convoys.get_count();
}



bool halt_detail_t::action_triggered( gui_action_creator_t *, value_t extra)
{
	if(extra.i&~1) {
		koord k = *(const koord *)extra.p;
		if(  k.x>=0  ) {
			// goto button pressed
			halt->get_welt()->change_world_position( koord3d(k,halt->get_welt()->max_hgt(k)) );
		}
		else if(  k.x==-1  ) {
			// Line button pressed.
			uint16 j=k.y;
			if(  j < halt->registered_lines.get_count()  ) {
				linehandle_t line=halt->registered_lines[j];
				spieler_t *sp=halt->get_welt()->get_active_player();
				if(  sp==line->get_besitzer()  ) {
					//TODO:
					// Change player => change marked lines
					sp->simlinemgmt.show_lineinfo(sp,line);
					halt->get_welt()->set_dirty();
				}
			}
		}
		else if(  k.x==-2  ) {
			// Knightly : lineless convoy button pressed
			uint16 j = k.y;
			if(  j<halt->registered_convoys.get_count()  ) {
				convoihandle_t convoy = halt->registered_convoys[j];
				convoy->zeige_info();
			}
		}
	}
	return true;
}



void halt_detail_t::zeichnen(koord pos, koord gr)
{
	if(halt.is_bound()) {
		if(  halt->get_rebuild_destination_counter()!=destination_counter  ||  cached_active_player!=halt->get_welt()->get_active_player()
				||  halt->registered_lines.get_count()!=cached_line_count  ||  halt->registered_convoys.get_count()!=cached_convoy_count  ) {
			// fill buffer with halt detail
			halt_detail_info(cb_info_buffer);
			txt_info.set_text(cb_info_buffer);
			cached_active_player=halt->get_welt()->get_active_player();
		}
	}
	gui_frame_t::zeichnen( pos, gr );
}


halt_detail_t::halt_detail_t(karte_t *):
	gui_frame_t("", NULL),
	scrolly(&cont),
	txt_info(""),
	cb_info_buffer(0)
{
	// just a dummy
}


void halt_detail_t::rdwr(loadsave_t *file)
{
	koord3d halt_pos;
	koord gr = get_fenstergroesse();
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();
	if(  file->is_saving()  ) {
		halt_pos = halt->get_basis_pos3d();
	}
	halt_pos.rdwr( file );
	gr.rdwr( file );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );
	if(  file->is_loading()  ) {
		halt = haltestelle_t::get_welt()->lookup( halt_pos )->get_halt();
		// now we can open the window ...
		KOORD_VAL xpos = win_get_posx( this );
		KOORD_VAL ypos = win_get_posy( this );
		halt_detail_t *w = new halt_detail_t(halt);
		create_win( xpos, ypos, w, w_info, magic_halt_detail+halt.get_id() );
		w->set_fenstergroesse( gr );
		w->scrolly.set_scroll_position( xoff, yoff );
		destroy_win( this );
	}
}
