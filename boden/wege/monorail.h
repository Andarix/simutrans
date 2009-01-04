/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_monorail_h
#define boden_wege_monorail_h


#include "schiene.h"


/**
 * Klasse f�r Schienen in Simutrans.
 * Auf den Schienen koennen Z�ge fahren.
 * Jede Schiene geh�rt zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 */
class monorail_t : public schiene_t
{
public:
	static const weg_besch_t *default_monorail;

	monorail_t(karte_t *welt) : schiene_t(welt) { set_besch(default_monorail); }

	/**
	 * File loading constructor.
	 * @author prissi
	 */
	monorail_t(karte_t *welt, loadsave_t *file);

	virtual waytype_t get_waytype() const {return monorail_wt;}

	void rdwr(loadsave_t *file);
};

#endif
