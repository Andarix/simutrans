/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_scenario_frame_h
#define gui_scenario_frame_h


#include "savegame_frame.h"
#include "../utils/cbuffer_t.h"



class ai_selector_t : public savegame_frame_t
{
private:
	cbuffer_t path;
	cbuffer_t title;

	uint8 plnr; /// select script for this player

protected:
	/**
	 * Action that's started by the press of a button.
	 */
	bool item_action(const char *fullpath) OVERRIDE;

	/**
	 * Action, started after X-Button pressing
	 */
	bool del_action(const char *f) OVERRIDE { return item_action(f); }

	// returns extra file info
	const char *get_info(const char *fname) OVERRIDE;

	// true, if valid
	bool check_file( const char *filename, const char *suffix ) OVERRIDE;
public:
	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char * get_help_filename() const OVERRIDE { return "ai_selector.txt"; }

	ai_selector_t(uint8 plnr_);
};

#endif
