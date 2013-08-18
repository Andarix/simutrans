/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */
#include "../simdebug.h"

#include "../simworld.h"

#include "haus_besch.h"
#include "../network/checksum.h"



/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Rechnet aus dem Index das Layout aus, zu dem diese Tile geh�rt.
 */
uint8 haus_tile_besch_t::get_layout() const
{
	koord groesse = get_besch()->get_groesse();
	return index / (groesse.x * groesse.y);
}



/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Bestimmt die Relativ-Position des Einzelbildes im Gesamtbild des
 *	Geb�udes.
 */
koord haus_tile_besch_t::get_offset() const
{
	const haus_besch_t *besch = get_besch();
	koord groesse = besch->get_groesse(get_layout());	// ggf. gedreht
	return koord( index % groesse.x, (index / groesse.x) % groesse.y );
}



waytype_t haus_besch_t::get_finance_waytype() const
{
	switch( get_utyp() )
	{
		case haus_besch_t::bahnhof:      return track_wt;
		case haus_besch_t::bushalt:      return road_wt;
		case haus_besch_t::hafen:        return water_wt;
		case haus_besch_t::binnenhafen:  return water_wt;
		case haus_besch_t::airport:      return air_wt;
		case haus_besch_t::monorailstop: return monorail_wt;
		case haus_besch_t::bahnhof_geb:  return track_wt;
		case haus_besch_t::bushalt_geb:  return road_wt;
		case haus_besch_t::hafen_geb:    return water_wt;
		case haus_besch_t::binnenhafen_geb: return water_wt;
		case haus_besch_t::airport_geb:  return air_wt;
		case haus_besch_t::monorail_geb: return monorail_wt;
		case haus_besch_t::depot:
		case haus_besch_t::generic_stop:
		case haus_besch_t::generic_extension:
			return (waytype_t) get_extra();
		default: return ignore_wt;
	}
}



/**
 * Mail generation level
 * @author Hj. Malthaner
 */
uint16 haus_besch_t::get_post_level() const
{
	switch (gtyp) {
		default:
		case gebaeude_t::wohnung:   return level;
		case gebaeude_t::gewerbe:   return level * 2;
		case gebaeude_t::industrie: return level / 2;
	}
}



/**
 * true, if this building needs a connection with a town
 * @author prissi
 */
bool haus_besch_t::is_connected_with_town() const
{
	switch(get_utyp()) {
		case haus_besch_t::unbekannt:	// normal town buildings (RES, COM, IND)
		case haus_besch_t::denkmal:	// monuments
		case haus_besch_t::rathaus:	// townhalls
		case haus_besch_t::firmensitz:	// headquarter
			return true;
		default:
			return false;
	}
}



/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Abh�ngig von Position und Layout ein tile zur�ckliefern
 */
const haus_tile_besch_t *haus_besch_t::get_tile(int layout, int x, int y) const
{
	layout = layout_anpassen(layout);
	koord dims = get_groesse(layout);

	if(layout < 0  ||  x < 0  ||  y < 0  ||  layout >= layouts  ||  x >= get_b(layout)  ||  y >= get_h(layout)) {
	dbg->fatal("haus_tile_besch_t::get_tile()",
			   "invalid request for l=%d, x=%d, y=%d on building %s (l=%d, x=%d, y=%d)",
		   layout, x, y, get_name(), layouts, groesse.x, groesse.y);
	}
	return get_tile(layout * dims.x * dims.y + y * dims.x + x);
}



/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Layout normalisieren.
 */
int haus_besch_t::layout_anpassen(int layout) const
{
	if(layout >= 4 && layouts <= 4) {
		layout -= 4;
	}
	if(layout >= 2 && layouts <= 2) {
		// Sind Layout C und D nicht definiert, nehemen wir ersatzweise A und B
		layout -= 2;
	}
	if(layout > 0 && layouts <= 1) {
		// Ist Layout B nicht definiert und das Teil quadratisch, nehmen wir ersatzweise A
		if(groesse.x == groesse.y) {
			layout--;
		}
	}
	return layout;
}


void haus_besch_t::calc_checksum(checksum_t *chk) const
{
	chk->input((uint8)gtyp);
	chk->input((uint8)utype);
	chk->input(animation_time);
	chk->input(extra_data);
	chk->input(groesse.x);
	chk->input(groesse.y);
	chk->input((uint8)flags);
	chk->input(level);
	chk->input(layouts);
	chk->input(enables);
	chk->input(chance);
	chk->input((uint8)allowed_climates);
	chk->input(intro_date);
	chk->input(obsolete_date);
	chk->input(maintenance);
	chk->input(price);
	chk->input(capacity);
	chk->input(allow_underground);
	// now check the layout
	for(uint8 i=0; i<layouts; i++) {
		sint16 b=get_b(i);
		for(sint16 x=0; x<b; x++) {
			sint16 h=get_h(i);
			for(sint16 y=0; y<h; y++) {
				if (get_tile(i,x,y)  &&  get_tile(i,x,y)->has_image()) {
					chk->input((sint16)(x+y+i));
				}
			}
		}
	}
}


/**
* @station get functions - see haus_besch.h for variable information
* @author jamespetts
*/

sint32 haus_besch_t::get_maintenance(karte_t *welt) const
{
	if(  maintenance == COST_MAGIC  ) {
		return welt->get_settings().maint_building*get_level();
	}
	else {
		return maintenance;
	}
}


sint32 haus_besch_t::get_price(karte_t *welt) const
{
	if(  price == COST_MAGIC  ) {
		settings_t const& s = welt->get_settings();
		switch (get_utyp()) {
			case haus_besch_t::hafen:
				return -s.cst_multiply_dock * get_level();
			break;
			case haus_besch_t::generic_extension:
				return -s.cst_multiply_post * get_level();
			break;
			case haus_besch_t::generic_stop: {
				switch(get_extra()) {
					case road_wt:
						return -s.cst_multiply_roadstop * get_level();
					break;
					case track_wt:
					case monorail_wt:
					case maglev_wt:
					case narrowgauge_wt:
					case tram_wt:
						return -s.cst_multiply_station * get_level();
					break;
					case water_wt:
						return -s.cst_multiply_dock * get_level();
					break;
					case air_wt:
						return -s.cst_multiply_airterminal * get_level();
					break;
					case 0:
						return -s.cst_multiply_post * get_level();
					break;
				}
			}
			break;
			default:
				return 0;
			break;
		}
	}
	else {
		return price;
	}
	return 0;
}


uint16 haus_besch_t::get_capacity() const
{
	return capacity;
}
