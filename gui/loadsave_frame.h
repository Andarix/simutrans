/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_loadsave_frame_h
#define gui_loadsave_frame_h

#include <time.h>

#include "savegame_frame.h"
#include "../tpl/stringhashtable_tpl.h"
#include <string>

class loadsave_t;

class sve_info_t {
public:
	std::string pak;
	sint64 mod_time;
	sint32 file_size;
	bool file_exists;
	sve_info_t() : pak(""), mod_time(0), file_size(0), file_exists(false) {}
	sve_info_t(const char *pak_, time_t mod_, sint32 fs);
	bool operator== (const sve_info_t &) const;
	void rdwr(loadsave_t *file);
};

class loadsave_frame_t : public savegame_frame_t
{
private:
	bool do_load;

	button_t easy_server; // only active on loading savegames

	static stringhashtable_tpl<sve_info_t *> cached_info;

protected:
	/**
	 * Action that's started with a button click
	 * @author Hansj�rg Malthaner
	 */
	bool item_action (const char *filename) OVERRIDE;
	bool ok_action   (const char *fullpath) OVERRIDE;

	// returns extra file info
	const char *get_info(const char *fname) OVERRIDE;

	// sort with respect to info, which is date
	bool compare_items ( const dir_entry_t & entry, const char *info, const char *) OVERRIDE;

public:
	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char *get_help_filename() const OVERRIDE;

	loadsave_frame_t(bool do_load);

	/**
	 * save hashtable to xml file
	 */
	~loadsave_frame_t();
};

#endif
