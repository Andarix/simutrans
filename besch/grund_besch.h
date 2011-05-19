/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __GRUND_BESCH_H
#define __GRUND_BESCH_H

#include "obj_besch_std_name.h"
#include "bildliste2d_besch.h"
#include "../simtypes.h"
#include "../dataobj/ribi.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Verschiedene Untergr�nde - viellcht bald weisse Berge?
 *
 *  Kindknoten:
 *	0   Name
 *	1   Copyright
 *	2   Bildliste2D
 */

class karte_t;

class grund_besch_t : public obj_besch_std_name_t {
private:
	static karte_t *welt;

	static image_id image_offset;

public:
	static int water_animation_stages;

	// only these textures need external access
	static const grund_besch_t *fundament;
	static const grund_besch_t *slopes;
	static const grund_besch_t *fences;
	static const grund_besch_t *marker;
	static const grund_besch_t *borders;
	static const grund_besch_t *sea;	// different water depth
	static const grund_besch_t *border;
	static const grund_besch_t *ausserhalb;

	static char const* get_climate_name_from_bit(climate n);

#ifdef DOUBLE_GROUNDS
    static const uint8 slopetable[80];
    // returns the correct hang number for this slope
    static inline int get_double_hang(hang_t::typ typ) {
		return slopetable[typ];
    }
#endif

	// returns the pointer to an image structure
	const bild_besch_t *get_bild_ptr(int typ, int stage=0) const
	{
		bildliste2d_besch_t const* const bl2   = get_child<bildliste2d_besch_t>(2);
		bildliste_besch_t   const* const liste = bl2->get_liste(typ);
		if(liste && liste->get_anzahl() > 0) {
			bild_besch_t const* const bild = bl2->get_bild(typ, stage);
			return bild;
		}
		return NULL;
	}

	// image for all non-climate stuff like foundations ...
	image_id get_bild(int typ, int stage=0) const
	{
		bild_besch_t const* const bild = get_bild_ptr(typ, stage);
		return bild ? bild->get_nummer() : IMG_LEER;
	}

	// image for all ground tiles
	static image_id get_ground_tile(hang_t::typ slope, sint16 height );

	static bool register_besch(const grund_besch_t *besch);

	static bool alles_geladen();

	/* this routine is called during the creation of a new map
	 * it will recalculate all transitions according the given water level
	 * and put the result in height_to_climate
	 */
	static void calc_water_level(karte_t *welt, uint8 *height_to_climate);

};

#endif
