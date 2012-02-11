/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef halt_list_stats_h
#define halt_list_stats_h

#include "../dataobj/koord.h"
#include "components/gui_komponente.h"
#include "../halthandle_t.h"


/**
 * @autor Hj. Malthaner
 */
class halt_list_stats_t : public gui_komponente_t
{
private:
	halthandle_t halt;

public:
	halt_list_stats_t() : halt() {}
	halt_list_stats_t(halthandle_t halt_) : halt(halt_) { groesse.y = 28; }
	const halthandle_t get_halt() const { return halt; }

	bool infowin_event(event_t const*) OVERRIDE;

	void zeichnen(koord offset);
};

#endif
