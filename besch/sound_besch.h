/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __sound_besch_t
#define __sound_besch_t

#include "obj_besch_std_name.h"
#include "../simtypes.h"

#define NO_SOUND (sint16)(0xFFFFu)
#define LOAD_SOUND (sint8)(0xFFFEu)

#define AMBIENT_SOUND_INTERVALL (13000)

/*
 *  Author:
 *      prissi
 *
 *  Description:
 *      Sounds in the game; name is the file name
 *      ingame, sounds are referred to by their number
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 */

#define SFX_CASH sound_besch_t::get_compatible_sound_id(15)
#define SFX_REMOVER sound_besch_t::get_compatible_sound_id(14)
#define SFX_DOCK sound_besch_t::get_compatible_sound_id(13)
#define SFX_GAVEL sound_besch_t::get_compatible_sound_id(12)
#define SFX_JACKHAMMER sound_besch_t::get_compatible_sound_id(11)
#define SFX_FAILURE sound_besch_t::get_compatible_sound_id(10)
#define SFX_SELECT sound_besch_t::get_compatible_sound_id(9)

#define MAX_OLD_SOUNDS (16)


class sound_besch_t : public obj_named_desc_t {
    friend class sound_reader_t;

private:
	static sint16 compatible_sound_id[MAX_OLD_SOUNDS];

	sint16 sound_id;
	sint16 nr;	// for old sounds/system sounds etc.

public:
	// sounds for ambient
	static sint16 beach_sound;
	static sint16 forest_sound;
	static sint16 climate_sounds[MAX_CLIMATES];

	static sint16 get_sound_id(const char *name);

	static bool register_desc(sound_besch_t *desc);

	static void init();

	/* return old sound id from index */
	static sint16 get_compatible_sound_id(const sint8 nr) { return compatible_sound_id[nr&(15)]; }
};


#endif
