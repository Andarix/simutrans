/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef obj_groundobj_h
#define obj_groundobj_h

#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../besch/groundobj_besch.h"
#include "../dataobj/environment.h"

/**
 * Decorative objects, like rocks, ponds etc.
 */
class groundobj_t : public obj_t
{
private:
	/// type of object, index into groundobj_typen
	uint16 groundobjtype;

	/// the image, cached
	image_id bild;

	/// table to lookup object based on name
	static stringhashtable_tpl<groundobj_besch_t *> besch_names;

	/// all such objects
	static vector_tpl<const groundobj_besch_t *> groundobj_typen;

public:
	static bool register_besch(groundobj_besch_t *besch);
	static bool alles_geladen();

	static const groundobj_besch_t *random_groundobj_for_climate(climate cl, hang_t::typ slope );

	groundobj_t(loadsave_t *file);
	groundobj_t(koord3d pos, const groundobj_besch_t *);

	void rdwr(loadsave_t *file);

	image_id get_image() const { return bild; }

	/// recalculates image depending on season and slope of ground
	void calc_image();

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool);

	const char *get_name() const {return "Groundobj";}
	typ get_typ() const { return groundobj; }

	void show_info();

	void info(cbuffer_t & buf) const;

	void cleanup(player_t *player);

	const groundobj_besch_t* get_besch() const { return groundobj_typen[groundobjtype]; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
