/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <string>
#include <stdio.h>
#include "../simdebug.h"

#include "../dataobj/tabfile.h"
#include "../dataobj/umgebung.h"

#include "../macros.h"
#include "../sound/sound.h"

#include "../utils/simstring.h"

#include "../tpl/stringhashtable_tpl.h"

#include "spezial_obj_tpl.h"
#include "sound_besch.h"
#include "grund_besch.h"

/* sound of the program *
 * @author prissi
 */

class sound_ids {
public:
	std::string filename;
	sint16 id;
	sound_ids() { id=NO_SOUND; }
	sound_ids(sint16 i) { id=i;  }
	sound_ids(sint16 i, const char* fn) : filename(fn), id(i) {}
};


static stringhashtable_tpl<sound_ids *> name_sound;
static bool sound_on=false;
static std::string sound_path;

sint16 sound_besch_t::compatible_sound_id[MAX_OLD_SOUNDS]=
{
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND,
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND,
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND,
	NO_SOUND, NO_SOUND, NO_SOUND, NO_SOUND
};

// sound with the names of climates and "beach" and "forest" are reserved for ambient noises
sint16 sound_besch_t::beach_sound;
sint16 sound_besch_t::forest_sound;
sint16 sound_besch_t::climate_sounds[MAX_CLIMATES];


/* init sounds */
/* standard sounds and old sounds are found in the file pak/sound/sound.tab */
void sound_besch_t::init()
{
	// ok, now init
	sound_on = true;
	sound_path = umgebung_t::program_dir;
	sound_path= sound_path + umgebung_t::objfilename + "sound/";
	// process sound.tab
	tabfile_t soundconf;
	if (soundconf.open((sound_path + "sound.tab").c_str())) {
DBG_MESSAGE("sound_besch_t::init()","successfully opened sound/sound.tab"  );
		tabfileobj_t contents;
		soundconf.read(contents);
		// max. 16 old sounds ...
		for( unsigned i=0;  i<MAX_OLD_SOUNDS;  i++ ) {
			char buf[1024];
			sprintf(buf, "sound[%i]", i);
			const char *fn=ltrim(contents.get(buf));
			if(fn[0]>0) {
DBG_MESSAGE("sound_besch_t::init()","reading sound %s", fn  );
				compatible_sound_id[i] = get_sound_id( fn );
DBG_MESSAGE("sound_besch_t::init()","assigned system sound %d to sound %s (id=%i)", i, (const char *)fn, compatible_sound_id[i] );
			}
		}
		// now assign special sounds for climates, beaches and forest
		beach_sound = get_sound_id( "beaches.wav" );
		forest_sound = get_sound_id( "forest.wav" );
		for(  int i=0;  i<MAX_CLIMATES;  i++  ) {
			char name[64];
			sprintf( name, "%s.wav", grund_besch_t::get_climate_name_from_bit((climate)i) );
			climate_sounds[i] = get_sound_id( name );
		}
	}
}




/* return sound id from index */
sint16 sound_besch_t::get_sound_id(const char *name)
{
	if(!sound_on) {
		return NO_SOUND;
	}
	sound_ids *s = name_sound.get(name);
	if((s==NULL  ||  s->id==NO_SOUND)  &&  *name!=0) {
		// ty to load it ...
		sint16 id  = dr_load_sample((sound_path + name).c_str());
		if(id!=NO_SOUND) {
			s = new sound_ids(id,name);
			name_sound.put(s->filename.c_str(), s );
DBG_MESSAGE("sound_besch_t::get_sound_id()","successfully loaded sound %s internal id %i", s->filename.c_str(), s->id );
			return s->id;
		}
		dbg->warning("sound_besch_t::get_sound_id()","sound \"%s\" not found", name );
		return NO_SOUND;
	}
DBG_MESSAGE("sound_besch_t::get_sound_id()","successfully retrieved sound %s internal id %i", s->filename.c_str(), s->id );
	return s->id;
}



/*
 * if there is already such a sound => fail, else success and get an internal sound id
 */
bool
sound_besch_t::register_besch(sound_besch_t *besch)
{
	if(!sound_on) {
		delete besch;
		return false;
	}
	// register, if not there (all done by this one here)
	besch->sound_id = get_sound_id( besch->get_name() );
	if(besch->sound_id!=NO_SOUND) {
		if(besch->nr>=0  &&  besch->nr<=8) {
			compatible_sound_id[besch->nr] = besch->sound_id;
DBG_MESSAGE("sound_besch_t::get_sound_id()","successfully registered sound %s internal id %i as compatible sound %i", besch->get_name(), besch->sound_id, besch->nr );
			delete besch;
			return true;
		}
	}
	dbg->warning("sound_besch_t::get_sound_id()","failed to register sound %s internal id %i", besch->get_name() );
	delete besch;
	return false;
}


bool sound_besch_t::alles_geladen()
{
	DBG_MESSAGE("sound_besch_t::alles_geladen()","sounds");
	return true;	// no mandatory objects here
}
