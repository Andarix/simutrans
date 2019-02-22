/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_load_relief_frame_h
#define gui_load_relief_frame_h


#include "savegame_frame.h"


class settings_t;

class load_relief_frame_t : public savegame_frame_t
{
private:
	settings_t* sets;

	button_t new_format;

protected:
	bool item_action(const char *fullpath) OVERRIDE;
	const char *get_info(const char *fullpath) OVERRIDE;
	bool check_file(const char *fullpath, const char *suffix) OVERRIDE;

public:
	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char *get_help_filename() const OVERRIDE { return "load_relief.txt"; }

	load_relief_frame_t(settings_t*);
};

#endif
