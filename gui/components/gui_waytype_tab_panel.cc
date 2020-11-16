/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


#include "gui_waytype_tab_panel.h"

#include "../../simskin.h"

#include "../../bauer/vehikelbauer.h"

#include "../../boden/wege/kanal.h"
#include "../../boden/wege/maglev.h"
#include "../../boden/wege/monorail.h"
#include "../../boden/wege/narrowgauge.h"
#include "../../boden/wege/runway.h"
#include "../../boden/wege/schiene.h"
#include "../../boden/wege/strasse.h"

#include "../../dataobj/translator.h"

#include "../../descriptor/skin_desc.h"


void gui_waytype_tab_panel_t::init_tabs(gui_component_t* c)
{
	add_tab(c, translator::translate("All"));
	uint8 max_idx = 0;
	tabs_to_waytype[max_idx++] = ignore_wt;

	// now add all specific tabs
	if (maglev_t::default_maglev) {
		add_tab(c, translator::translate("Maglev"), skinverwaltung_t::maglevhaltsymbol, translator::translate("Maglev"));
		tabs_to_waytype[max_idx++] = maglev_wt;
	}
	if (monorail_t::default_monorail) {
		add_tab(c, translator::translate("Monorail"), skinverwaltung_t::monorailhaltsymbol, translator::translate("Monorail"));
		tabs_to_waytype[max_idx++] = monorail_wt;
	}
	if (schiene_t::default_schiene) {
		add_tab(c, translator::translate("Train"), skinverwaltung_t::zughaltsymbol, translator::translate("Train"));
		tabs_to_waytype[max_idx++] = track_wt;
	}
	if (narrowgauge_t::default_narrowgauge) {
		add_tab(c, translator::translate("Narrowgauge"), skinverwaltung_t::narrowgaugehaltsymbol, translator::translate("Narrowgauge"));
		tabs_to_waytype[max_idx++] = narrowgauge_wt;
	}
	if (!vehicle_builder_t::get_info(tram_wt).empty()) {
		add_tab(c, translator::translate("Tram"), skinverwaltung_t::tramhaltsymbol, translator::translate("Tram"));
		tabs_to_waytype[max_idx++] = tram_wt;
	}
	if (strasse_t::default_strasse) {
		add_tab(c, translator::translate("Truck"), skinverwaltung_t::autohaltsymbol, translator::translate("Truck"));
		tabs_to_waytype[max_idx++] = road_wt;
	}
	if (!vehicle_builder_t::get_info(water_wt).empty()) {
		add_tab(c, translator::translate("Ship"), skinverwaltung_t::schiffshaltsymbol, translator::translate("Ship"));
		tabs_to_waytype[max_idx++] = water_wt;
	}
	if (runway_t::default_runway) {
		add_tab(c, translator::translate("Air"), skinverwaltung_t::airhaltsymbol, translator::translate("Air"));
		tabs_to_waytype[max_idx++] = air_wt;
	}
}
