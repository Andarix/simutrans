/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_loadsave_frame_h
#define gui_loadsave_frame_h


#include "savegame_frame.h"
#include "../tpl/stringhashtable_tpl.h"
#include <string>

class karte_t;
class loadsave_t;

class sve_info_t {
public:
	std::string pak;
	sint64 mod_time;
	sint32 file_size;
	bool file_exists;
	sve_info_t() : pak(""), mod_time(0), file_size(0), file_exists(false) {}
	sve_info_t(const char *pak_, time_t mod_, long fs);
	bool operator== (const sve_info_t &) const;
	void rdwr(loadsave_t *file);
};

class loadsave_frame_t : public savegame_frame_t
{
private:
	karte_t *welt;
	bool do_load;

	static stringhashtable_tpl<sve_info_t *> cached_info;
protected:
	/**
	 * Aktion, die nach Knopfdruck gestartet wird.
	 * @author Hansj�rg Malthaner
	 */
	virtual void action(const char *filename);

	/**
	 * Aktion, die nach X-Knopfdruck gestartet wird.
	 * @author V. Meyer
	 */
	virtual bool del_action(const char *filename);

	// returns extra file info
	virtual const char *get_info(const char *fname);

public:
	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen f�r die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	virtual const char * get_hilfe_datei() const;

	loadsave_frame_t(karte_t *welt, bool do_load);

	/**
	 * save hashtable to xml file
	 */
	~loadsave_frame_t();
};

#endif
