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
#include "../descriptor/groundobj_desc.h"
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
	image_id image;

	/// table to lookup object based on name
	static stringhashtable_tpl<groundobj_desc_t *> desc_table;

	/// all such objects
	static vector_tpl<const groundobj_desc_t *> groundobj_typen;

public:
	static bool register_desc(groundobj_desc_t *desc);
	static bool successfully_loaded();

	static const groundobj_desc_t *random_groundobj_for_climate(climate_bits cl, slope_t::type slope );

	groundobj_t(loadsave_t *file);
	groundobj_t(koord3d pos, const groundobj_desc_t *);

	void rdwr(loadsave_t *file) OVERRIDE;

	image_id get_image() const OVERRIDE { return image; }

	/// recalculates image depending on season and slope of ground
	void calc_image() OVERRIDE;

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool) OVERRIDE;

	const char *get_name() const OVERRIDE {return "Groundobj";}
	typ get_typ() const OVERRIDE { return groundobj; }

	void show_info() OVERRIDE;

	void info(cbuffer_t & buf) const OVERRIDE;

	void cleanup(player_t *player) OVERRIDE;

	const groundobj_desc_t* get_desc() const { return groundobj_typen[groundobjtype]; }

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
