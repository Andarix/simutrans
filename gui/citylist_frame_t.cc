/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "citylist_frame_t.h"
#include "citylist_stats_t.h"

#include "components/list_button.h"
#include "../dataobj/translator.h"
#include "../simcolor.h"
#include "../dataobj/umgebung.h"


/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool citylist_frame_t::sortreverse = false;

karte_t *citylist_frame_t::welt = NULL;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */
citylist::sort_mode_t citylist_frame_t::sortby = citylist::by_name;

const char *citylist_frame_t::sort_text[citylist::SORT_MODES] = {
    "Name",
    "citicens",
    "Growth"
};

const char citylist_frame_t::hist_type[karte_t::MAX_WORLD_COST][20] =
{
	"citicens",
	"Growth",
	"Towns",
	"Factories",
	"Convoys",
	"Verkehrsteilnehmer",
	"ratio_pax",
	"Passagiere",
	"sended",
	"Post",
	"Arrived",
	"Goods"
};

const uint8 citylist_frame_t::hist_type_color[karte_t::MAX_WORLD_COST] =
{
	COL_WHITE,
	COL_DARK_GREEN,
	COL_LIGHT_PURPLE,
	71 /*COL_GREEN*/,
	COL_TURQUOISE,
	COL_POWERLINES,
	COL_LIGHT_BLUE,
	COL_BLUE,
	COL_LIGHT_YELLOW,
	COL_YELLOW,
	COL_LIGHT_BROWN,
	COL_BROWN
};

const uint8 citylist_frame_t::hist_type_type[karte_t::MAX_WORLD_COST] =
{
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	MONEY,
	STANDARD,
	MONEY,
	STANDARD,
	MONEY,
	STANDARD
};

#define CHART_HEIGHT (170)
#define TOTAL_HEIGHT (240)


citylist_frame_t::citylist_frame_t(karte_t * welt) :
	gui_frame_t("City list"),
	sort_label(translator::translate("hl_txt_sort")),
	stats(welt,sortby,sortreverse),
	scrolly(&stats)
{
	this->welt = welt;

	sort_label.set_pos(koord(BUTTON1_X, 40-BUTTON_HEIGHT-LINESPACE));
	add_komponente(&sort_label);

	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, 40-BUTTON_HEIGHT), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, 40-BUTTON_HEIGHT), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	show_stats.init(button_t::roundbox_state, "Chart", koord(BUTTON4_X, 40-BUTTON_HEIGHT), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	show_stats.set_tooltip("Show/hide statistics");
	show_stats.add_listener(this);
	add_komponente(&show_stats);

	// name buttons
	sortedby.set_text(sort_text[get_sortierung()]);
	sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");

	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	year_month_tabs.set_pos(koord(0,42));
	year_month_tabs.set_groesse(koord(TOTAL_WIDTH, CHART_HEIGHT-BUTTON_HEIGHT*3-16));
//	year_month_tabs.add_listener(this);
	year_month_tabs.set_visible(false);
	add_komponente(&year_month_tabs);

	const sint16 yb = 42+CHART_HEIGHT-BUTTON_HEIGHT*3-8;
	chart.set_pos(koord(60,8+gui_tab_panel_t::HEADER_VSIZE));
	chart.set_groesse(koord(TOTAL_WIDTH-60-8,yb-16-42-10-gui_tab_panel_t::HEADER_VSIZE));
	chart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_YEARS);
	chart.set_visible(false);
	chart.set_background(MN_GREY1);
	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		chart.add_curve(hist_type_color[cost], welt->get_finance_history_year(), karte_t::MAX_WORLD_COST, cost, MAX_WORLD_HISTORY_YEARS, hist_type_type[cost], false, true, (cost==1) ? 1 : 0 );
	}

	mchart.set_pos(koord(60,8+gui_tab_panel_t::HEADER_VSIZE));
	mchart.set_groesse(koord(TOTAL_WIDTH-60-8,yb-16-42-10-gui_tab_panel_t::HEADER_VSIZE));
	mchart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_MONTHS);
	mchart.set_visible(false);
	mchart.set_background(MN_GREY1);
	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		mchart.add_curve(hist_type_color[cost], welt->get_finance_history_month(), karte_t::MAX_WORLD_COST, cost, MAX_WORLD_HISTORY_MONTHS, hist_type_type[cost], false, true, (cost==1) ? 1 : 0 );
	}

	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		filterButtons[cost].init(button_t::box_state, hist_type[cost], koord(BUTTON1_X+(BUTTON_WIDTH+BUTTON_SPACER)*(cost%4), yb+(BUTTON_HEIGHT+2)*(cost/4)), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background = hist_type_color[cost];
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		add_komponente(filterButtons + cost);
	}

	scrolly.set_show_scroll_x(false);
	scrolly.set_pos(koord(1,42));
	add_komponente(&scrolly);

	set_fenstergroesse(koord(TOTAL_WIDTH, TOTAL_HEIGHT));

	set_min_windowsize(koord(TOTAL_WIDTH, TOTAL_HEIGHT));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool citylist_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
    if(komp == &sortedby) {
		set_sortierung((citylist::sort_mode_t)((get_sortierung() + 1) % citylist::SORT_MODES));
		sortedby.set_text(sort_text[get_sortierung()]);
		stats.sort(get_sortierung(),get_reverse());
    }
    else if(komp == &sorteddir) {
		set_reverse(!get_reverse());
		sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		stats.sort(get_sortierung(),get_reverse());
    }
    else if(komp == &show_stats) {
		show_stats.pressed = !show_stats.pressed;
		chart.set_visible( show_stats.pressed );
		year_month_tabs.set_visible( show_stats.pressed );
		set_min_windowsize( koord(TOTAL_WIDTH, show_stats.pressed ? TOTAL_HEIGHT+CHART_HEIGHT : TOTAL_HEIGHT));
		for(  int i=0;  i<karte_t::MAX_WORLD_COST;  i++ ) {
			filterButtons[i].set_visible(show_stats.pressed);
		}
		resize( koord(0,0) );
	}
	else {
		for(  int i=0;  i<karte_t::MAX_WORLD_COST;  i++ ) {
			if(  komp == filterButtons+i  ) {
				filterButtons[i].pressed = !filterButtons[i].pressed;
				if (filterButtons[i].pressed) {
					chart.show_curve(i);
					mchart.show_curve(i);
				}
				else {
					chart.hide_curve(i);
					mchart.hide_curve(i);
				}
			}
		}
	}
    return true;
}

/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void citylist_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	koord groesse = get_fenstergroesse()-koord(0,58);	// fensterhoehe - 16(title) -42 (header)
	if(show_stats.pressed) {
		// addition space for statistics
		groesse += koord(0,-CHART_HEIGHT);
	}
	scrolly.set_pos( koord(0, 42+(show_stats.pressed*CHART_HEIGHT) ) );
	scrolly.set_groesse(groesse);
	set_dirty();
}



/**
 * Komponente neu zeichnen. Die �bergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void
citylist_frame_t::zeichnen(koord pos, koord gr)
{
	if(show_stats.pressed) {
		welt->update_history();
	}
	gui_frame_t::zeichnen(pos,gr);

	display_proportional(pos.x+2, pos.y+18, citylist_stats_t::total_bev_string, ALIGN_LEFT,COL_BLACK,true);
}
