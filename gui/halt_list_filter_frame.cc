/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 * written by Volker Meyer
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "gui_container.h"
#include "components/gui_scrollpane.h"
#include "gui_convoiinfo.h"
#include "halt_list_filter_frame.h"
#include "../simconvoi.h"
#include "../simcolor.h"

#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"
#include "../dataobj/translator.h"

koord halt_list_filter_frame_t::filter_buttons_pos[FILTER_BUTTONS] = {
    koord(5, 4),
    koord(5, 43),
    koord(21, 58),
    koord(21, 74),
    koord(21, 90),
    koord(21, 122),
    koord(21, 138),
    koord(5, 207),
    koord(21, 223),
    koord(21, 239),
    koord(125, 4),
    koord(265, 4),
    koord(21, 154),
    koord(21, 170),
    koord(21, 186),
    koord(21, 106)
};

const char *halt_list_filter_frame_t::filter_buttons_text[FILTER_BUTTONS] = {
    "hlf_chk_name_filter",
    "hlf_chk_type_filter",
    "hlf_chk_frachthof",
    "hlf_chk_bushalt",
    "hlf_chk_bahnhof",
    "hlf_chk_anleger",
    "hlf_chk_airport",
    "hlf_chk_spezial_filter",
    "hlf_chk_overflow",
    "hlf_chk_keine_verb",
    "hlf_chk_waren_annahme",
    "hlf_chk_waren_abgabe",
    "hlf_chk_monorailstop",
    "hlf_chk_maglevstop",
    "hlf_chk_narrowgaugestop",
    "hlf_chk_tramstop"
};

halt_list_frame_t::filter_flag_t halt_list_filter_frame_t::filter_buttons_types[FILTER_BUTTONS] = {
    halt_list_frame_t::name_filter,
    halt_list_frame_t::typ_filter,
    halt_list_frame_t::frachthof_filter,
    halt_list_frame_t::bushalt_filter,
    halt_list_frame_t::bahnhof_filter,
    halt_list_frame_t::dock_filter,
    halt_list_frame_t::airport_filter,
    halt_list_frame_t::spezial_filter,
    halt_list_frame_t::ueberfuellt_filter,
    halt_list_frame_t::ohneverb_filter,
    halt_list_frame_t::ware_an_filter,
    halt_list_frame_t::ware_ab_filter,
    halt_list_frame_t::monorailstop_filter,
    halt_list_frame_t::maglevstop_filter,
    halt_list_frame_t::narrowgaugestop_filter,
    halt_list_frame_t::tramstop_filter
};


halt_list_filter_frame_t::halt_list_filter_frame_t(spieler_t *sp, halt_list_frame_t *main_frame) :
    gui_frame_t( translator::translate("hlf_title"), sp),
    ware_scrolly_ab(&ware_cont_ab),
    ware_scrolly_an(&ware_cont_an)
{
	unsigned i;
	unsigned n;

	this->main_frame = main_frame;

	for(i = 0; i < FILTER_BUTTONS; i++) {
		filter_buttons[i].init(button_t::square, filter_buttons_text[i], filter_buttons_pos[i]);
		filter_buttons[i].add_listener(this);
		add_komponente(filter_buttons + i);
		if(filter_buttons_types[i] < halt_list_frame_t::sub_filter) {
			filter_buttons[i].foreground = COL_WHITE;
		}
	}
	name_filter_input.set_text(main_frame->access_name_filter(), 30);
	name_filter_input.set_groesse(koord(100, 14));
	name_filter_input.set_pos(koord(5, 17));
	name_filter_input.add_listener(this);
	add_komponente(&name_filter_input);

	ware_alle_an.init(button_t::roundbox, "hlf_btn_alle", koord(125, 17), koord(41, 14));
	ware_alle_an.add_listener(this);
	add_komponente(&ware_alle_an);
	ware_keine_an.init(button_t::roundbox, "hlf_btn_keine", koord(167, 17), koord(41, 14));
	ware_keine_an.add_listener(this);
	add_komponente(&ware_keine_an);
	ware_invers_an.init(button_t::roundbox, "hlf_btn_invers", koord(209, 17), koord(41, 14));
	ware_invers_an.add_listener(this);
	add_komponente(&ware_invers_an);

	ware_scrolly_an.set_pos(koord(125, 33));
	ware_scrolly_an.set_scroll_amount_y(16);
	ware_scrolly_an.set_size_corner(false);
	add_komponente(&ware_scrolly_an);

	for(i=n=0; i<warenbauer_t::get_waren_anzahl(); i++) {
		const ware_besch_t *ware = warenbauer_t::get_info(i);
		if(ware != warenbauer_t::nichts) {
			ware_item_t *item = new ware_item_t(this, NULL, ware);
			item->init(button_t::square, translator::translate(ware->get_name()), koord(16, 16*n++ + 4));
			ware_cont_an.add_komponente(item);
		}
	}
	ware_cont_an.set_groesse(koord(100, 16*n + 4));
	ware_scrolly_an.set_groesse(koord(125, 13*16));

	ware_alle_ab.init(button_t::roundbox, "hlf_btn_alle", koord(265, 17), koord(41, 14));
	ware_alle_ab.add_listener(this);
	add_komponente(&ware_alle_ab);
	ware_keine_ab.init(button_t::roundbox, "hlf_btn_keine", koord(307, 17), koord(41, 14));
	ware_keine_ab.add_listener(this);
	add_komponente(&ware_keine_ab);
	ware_invers_ab.init(button_t::roundbox, "hlf_btn_invers", koord(349, 17), koord(41, 14));
	ware_invers_ab.add_listener(this);
	add_komponente(&ware_invers_ab);

	ware_scrolly_ab.set_pos(koord(265, 33));
	ware_scrolly_ab.set_scroll_amount_y(16);
	ware_scrolly_ab.set_size_corner(false);
	add_komponente(&ware_scrolly_ab);

	for(i=n=0; i<warenbauer_t::get_waren_anzahl(); i++) {
		const ware_besch_t *ware = warenbauer_t::get_info(i);
		if(ware != warenbauer_t::nichts) {
		ware_item_t *item = new ware_item_t(this, ware, NULL);
		item->init(button_t::square, translator::translate(ware->get_name()), koord(16, 16*n++ + 4));
			ware_cont_ab.add_komponente(item);
		}
	}
	ware_cont_ab.set_groesse(koord(100, 16*n + 4));
	ware_scrolly_ab.set_groesse(koord(125, 13*16));

	set_fenstergroesse(koord(395, 275));
}



halt_list_filter_frame_t::~halt_list_filter_frame_t()
{
	main_frame->filter_frame_closed();
}



/**
 * This method is called if an action is triggered
 * @author V. Meyer
 */
bool halt_list_filter_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	int i;

	for (i = 0; i < FILTER_BUTTONS; i++) {
		if (komp == filter_buttons + i) {
			main_frame->set_filter(filter_buttons_types[i], !main_frame->get_filter(filter_buttons_types[i]));
			main_frame->display_list();
			return true;
		}
	}
	if (komp == &ware_alle_ab) {
		main_frame->set_alle_ware_filter_ab(1);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_keine_ab) {
		main_frame->set_alle_ware_filter_ab(0);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_invers_ab) {
		main_frame->set_alle_ware_filter_ab(-1);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_alle_an) {
		main_frame->set_alle_ware_filter_an(1);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_keine_an) {
		main_frame->set_alle_ware_filter_an(0);
		main_frame->display_list();
		return true;
	}
	if (komp == &ware_invers_an) {
		main_frame->set_alle_ware_filter_an(-1);
		main_frame->display_list();
		return true;
	}
	if (komp == &name_filter_input) {
		main_frame->display_list();
		return true;
	}
	return false;
}


void halt_list_filter_frame_t::ware_item_triggered(const ware_besch_t *ware_ab, const ware_besch_t *ware_an)
{
	if (ware_ab) {
		main_frame->set_ware_filter_ab(ware_ab, -1);
	}
	if (ware_an) {
		main_frame->set_ware_filter_an(ware_an, -1);
	}
	main_frame->display_list();
}



void halt_list_filter_frame_t::zeichnen(koord pos, koord gr)
{
	for(int i = 0; i < FILTER_BUTTONS; i++) {
		filter_buttons[i].pressed = main_frame->get_filter(filter_buttons_types[i]);
	}
	gui_frame_t::zeichnen(pos, gr);
}
