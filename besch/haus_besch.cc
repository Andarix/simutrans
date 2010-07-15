/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */
#include "../simdebug.h"

#include "haus_besch.h"



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
