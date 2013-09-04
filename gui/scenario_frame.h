/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_scenario_frame_h
#define gui_scenario_frame_h


#include "savegame_frame.h"
#include "../utils/cbuffer_t.h"

class karte_t;


class scenario_frame_t : public savegame_frame_t
{
private:
	karte_t *welt;
	bool do_load;
	cbuffer_t path;

protected:
	/**
	 * Action that's started by the press of a button.
	 * @author Hansj�rg Malthaner
	 */
	virtual void action(const char *fullpath);

	/**
	 * Action, started after X-Button pressing
	 * @author V. Meyer
	 */
	virtual bool del_action(const char *f) { action(f); return true; }

	// returns extra file info
	virtual const char *get_info(const char *fname);

	// true, if valid
	virtual bool check_file( const char *filename, const char *suffix );
public:
	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	virtual const char * get_hilfe_datei() const { return "scenario.txt"; }

	scenario_frame_t(karte_t *welt);
};

#endif
