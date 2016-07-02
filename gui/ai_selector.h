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
	bool do_load;
	cbuffer_t path;
	cbuffer_t title;

	uint8 plnr; /// select script for this player

protected:
	/**
	 * Action that's started by the press of a button.
	 */
	virtual bool item_action(const char *fullpath);

	/**
	 * Action, started after X-Button pressing
	 */
	virtual bool del_action(const char *f) { return item_action(f); }

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
	virtual const char * get_help_filename() const { return "ai_selector.txt"; }

	ai_selector_t(uint8 plnr_);
};

#endif
