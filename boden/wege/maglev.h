/*
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef boden_wege_maglev_h
#define boden_wege_maglev_h


#include "schiene.h"


/**
 * derived from schiene, because signals will behave similar
 */
class maglev_t : public schiene_t
{
public:
	static const way_desc_t *default_maglev;

	maglev_t() : schiene_t() { set_desc(default_maglev); }

	/**
	 * File loading constructor.
	 * @author prissi
	 */
	maglev_t(loadsave_t *file);

	waytype_t get_waytype() const OVERRIDE {return maglev_wt;}

	void rdwr(loadsave_t *file) OVERRIDE;
};

#endif
