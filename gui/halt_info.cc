/*
 * halt_info.cc
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>

#include "halt_info.h"

#include "../simworld.h"
#include "../simhalt.h"
#include "../simplay.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simskin.h"

#include "../dataobj/translator.h"
#include "components/gui_chart.h"
#include "../besch/skin_besch.h"

static const char *sort_text[4] = {
  "Zielort",
  "via",
  "via Menge",
  "Menge"
};

const char cost_type[MAX_HALT_COST][64] =
{
  "Arrived", "Departed", "Waiting", "Happy",
  "Unhappy", "No Route", "Vehicles"
};

const int cost_type_color[MAX_HALT_COST] =
{
  7, 11, 15, 132, 23, 27, 31
};

halt_info_t::halt_info_t(karte_t *welt, halthandle_t halt)
 : gui_frame_t(halt->access_name(), halt->gib_besitzer()->kennfarbe),
  scrolly(&text),
  text("                                                                                     "
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
      ),
   view(welt, halt->gib_basis_pos3d()),
   freight_info(16384)
{
    this->halt = halt;
    this->welt = welt;

    input.setze_pos(koord(11,4));
    input.setze_text(halt->access_name(), 48);

    button.setze_groesse(koord(64, 13));
    button.text = translator::translate("Details");
    button.setze_typ(button_t::roundbox);

    toggler.setze_groesse(koord(84, 13));
    toggler.setze_pos(koord(11, 88));
    toggler.text = translator::translate("Chart");
    toggler.setze_typ(button_t::roundbox);
    toggler.set_tooltip(translator::translate("Show/hide statistics"));
    toggler.add_listener(this);
    add_komponente(&toggler);
    btoggled = false;

    // hsiegeln: added sort_button
    sort_button.setze_groesse(koord(110, 13));
    sort_button.setze_pos(koord(105, 88));
    sort_button.text = translator::translate("Zielort");
    sort_button.setze_typ(button_t::roundbox);
    sort_button.set_tooltip(translator::translate("Sort waiting list by"));

    scrolly.setze_pos(koord(1, 90));

    add_komponente(&sort_button);
    add_komponente(&button);

    add_komponente(&input);

    setze_opaque(true);
    setze_fenstergroesse(koord(304, 264));

    set_min_windowsize(koord(304, 194));
    set_resizemode(diagonal_resize);     // 31-May-02	markus weber	added
    resize(koord(0,0));

    button.add_listener(this);
    sort_button.add_listener(this);

    // chart
    chart = new gui_chart_t;
    chart->setze_pos(koord(50,95));
    chart->setze_groesse(koord(280, 100));
    chart->set_dimension(12, 10000);
    chart->set_visible(false);
    chart->set_background(MN_GREY1);

    for (int i = 0; i<MAX_HALT_COST; i++)
    {
    	chart->add_curve(cost_type_color[i], halt->get_finance_history(), MAX_HALT_COST, i, MAX_MONTHS, 0, false, true);
    }
    add_komponente(chart);

    for (int cost=0;cost<MAX_HALT_COST;cost++)
    {
      filterButtons[cost].init(button_t::box,
			       cost_type[cost],
			       koord(11+107*(cost%3),
				     205+15*((int)cost/3+1)),
			       koord(106, 14));
      filterButtons[cost].add_listener(this);
      filterButtons[cost].background = cost_type_color[cost];
      filterButtons[cost].set_visible(false);
      bFilterIsActive[cost] = false;
      add_komponente(filterButtons + cost);
    }

    add_komponente(&view);
    add_komponente(&scrolly);
}


/**
 * Komponente neu zeichnen. Die �bergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void
halt_info_t::zeichnen(koord pos, koord gr)
{
  koord viewpos = view.gib_pos(); // 31-May-02  markus weber   added

  if(halt.is_bound()) {
    halt->set_sortby (halt->get_sortby());

    freight_info.clear();
    halt->get_freight_info(freight_info);
    text.setze_text(freight_info);

    for (int i = 0;i<MAX_HALT_COST;i++) {
      filterButtons[i].pressed = bFilterIsActive[i];
    }
    toggler.pressed = btoggled;

    gui_frame_t::zeichnen(pos, gr);

	// now what do we accept here?
	int left = 11;
	if (halt->get_pax_enabled()) {
	    display_color_img(skinverwaltung_t::passagiere->gib_bild_nr(0), pos.x+left, pos.y+40, 0, false, false);
	    left += 10;
	}
	if (halt->get_post_enabled()) {
	    display_color_img(skinverwaltung_t::post->gib_bild_nr(0), pos.x+left, pos.y+40, 0, false, false);
	    left += 10;
	}
	if (halt->get_ware_enabled()) {
	    display_color_img(skinverwaltung_t::waren->gib_bild_nr(0), pos.x+left, pos.y+40, 0, false, false);
	}

    display_ddd_box(pos.x+viewpos.x, pos.y+viewpos.y+16, 66, 57, MN_GREY0, MN_GREY4);

	// status box
	unsigned indikatorfarbe = halt->gib_status_farbe();
	display_ddd_box_clip(pos.x + view.pos.x, pos.y + view.pos.y + 75, 66, 8, MN_GREY0, MN_GREY4);
	display_fillbox_wh_clip(pos.x + view.pos.x+1, pos.y + view.pos.y + 76, 64, 6, indikatorfarbe, true);

    // Hajo: Reuse of freight_info buffer to get and display
    // information about the convoi itself
    freight_info.clear();
    halt->info(freight_info);

    display_multiline_text(pos.x+11, pos.y+56, freight_info, SCHWARZ);

    const int sortby = MIN(MAX(halt->get_sortby(), 0), 3);

    sort_button.text = translator::translate(sort_text[sortby]);
  }
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool halt_info_t::action_triggered(gui_komponente_t *comp)
{
	if (comp == &button) { 			// details button pressed
		halt->open_detail_window();
	} else if (comp == &sort_button) { 	// @author hsiegeln sort button pressed
		int sortby = halt->get_sortby();

		if(sortby < 0) {
			// Hajo: error case, correct silently
			sortby = 0;
		} else if (sortby < 3){
			sortby++;
		} else {
			sortby = 0;
		}

		halt->set_sortby((haltestelle_t::sort_mode_t) sortby);
		sort_button.text = translator::translate(sort_text[sortby]);
	} else  if (comp == &toggler) {
		btoggled = !btoggled;
		const koord offset = btoggled ? koord(40, 178) : koord(-40, -178);
		set_min_windowsize(btoggled ? koord(344, 372) : koord(304, 194));
		scrolly.pos.y += offset.y;
		// toggle visibility of components
		chart->set_visible(btoggled);
		setze_fenstergroesse(gib_fenstergroesse() + offset);
		resize(koord(0,0));
		for (int i=0;i<MAX_HALT_COST;i++) {
			filterButtons[i].set_visible(btoggled);
		}
	} else {
		for ( int i = 0; i<MAX_HALT_COST; i++) {

			if (comp == &filterButtons[i]) {

				bFilterIsActive[i] = !bFilterIsActive[i];
				if(bFilterIsActive[i]) {
					chart->show_curve(i);
				} else {
					chart->hide_curve(i);
				}
				break;
			}
		}
	}

	return true;
}


/**
 * Resize the contents of the window
 * @author Markus Weber
 */
void halt_info_t::resize(const koord delta)
{
  gui_frame_t::resize(delta);

  input.setze_groesse(koord(get_client_windowsize().x-23, 13));
  button.setze_pos(koord(get_client_windowsize().x - 90-64-86,22));
  toggler.setze_pos(koord(get_client_windowsize().x - 106-64,22));
  sort_button.setze_pos(koord(get_client_windowsize().x - 108-16,btoggled?268:90));
  view.setze_pos(koord(get_client_windowsize().x - 64 - 16 , 21));
  scrolly.setze_groesse(get_client_windowsize()-scrolly.gib_pos());
}
