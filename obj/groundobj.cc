/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../simtools.h"
#include "../simtypes.h"

#include "../boden/grund.h"
#include "../besch/groundobj_besch.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/freelist.h"


#include "groundobj.h"

/******************************** static routines for besch management ****************************************************************/

vector_tpl<const groundobj_besch_t *> groundobj_t::groundobj_typen(0);

stringhashtable_tpl<groundobj_besch_t *> groundobj_t::besch_names;


bool compare_groundobj_besch(const groundobj_besch_t* a, const groundobj_besch_t* b)
{
	return strcmp(a->get_name(), b->get_name())<0;
}


bool groundobj_t::alles_geladen()
{
	groundobj_typen.resize(besch_names.get_count());
	FOR(stringhashtable_tpl<groundobj_besch_t*>, const& i, besch_names) {
		groundobj_typen.insert_ordered(i.value, compare_groundobj_besch);
	}
	// iterate again to assign the index
	FOR(stringhashtable_tpl<groundobj_besch_t*>, const& i, besch_names) {
		i.value->index = groundobj_typen.index_of(i.value);
	}

	if(besch_names.empty()) {
		groundobj_typen.append( NULL );
		DBG_MESSAGE("groundobj_t", "No groundobj found - feature disabled");
	}
	return true;
}


bool groundobj_t::register_besch(groundobj_besch_t *besch)
{
	assert(besch->get_speed()==0);
	// remove duplicates
	if(  besch_names.remove( besch->get_name() )  ) {
		dbg->warning( "groundobj_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	besch_names.put(besch->get_name(), besch );
	return true;
}


/* also checks for distribution values
 * @author prissi
 */
const groundobj_besch_t *groundobj_t::random_groundobj_for_climate(climate cl, hang_t::typ slope  )
{
	// none there
	if(  besch_names.empty()  ) {
		return NULL;
	}

	int weight = 0;
	FOR(  vector_tpl<groundobj_besch_t const*>,  const i,  groundobj_typen  ) {
		if(  i->is_allowed_climate(cl)  &&  (slope == hang_t::flach  ||  i->get_phases() == 16)  ) {
			weight += i->get_distribution_weight();
		}
	}

	// now weight their distribution
	if(  weight > 0  ) {
		const int w=simrand(weight);
		weight = 0;
		FOR(vector_tpl<groundobj_besch_t const*>, const i, groundobj_typen) {
			if (i->is_allowed_climate(cl) && (slope == hang_t::flach || i->get_phases() == 16)) {
				weight += i->get_distribution_weight();
				if(weight>=w) {
					return i;
				}
			}
		}
	}
	return NULL;
}



/******************************* end of static ******************************************/



// recalculates only the seasonal image
void groundobj_t::calc_bild()
{
	const groundobj_besch_t *besch=get_besch();
	const sint16 seasons = besch->get_seasons()-1;
	uint8 season=0;

	// two possibilities
	switch(seasons) {
				// summer only
		case 0: season = 0;
				break;
				// summer, snow
		case 1: season = welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate;
				break;
				// summer, winter, snow
		case 2: season = (welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate) ? 2 : welt->get_season() == 1;
				break;
		default: if(  welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate  ) {
					season = seasons;
				}
				else {
					// resolution 1/8th month (0..95)
					const uint32 yearsteps = (welt->get_current_month()%12)*8 + ((welt->get_zeit_ms()>>(welt->ticks_per_world_month_shift-3))&7) + 1;
					season = (seasons*yearsteps-1)/96;
				}
				break;
	}
	// check for slopes?
	uint16 phase = 0;
	if(besch->get_phases()==16) {
		phase = welt->lookup(get_pos())->get_grund_hang();
	}
	const bild_besch_t *bild_ptr = get_besch()->get_bild( season, phase );
	bild = bild_ptr ? bild_ptr->get_nummer() : IMG_LEER;
}


groundobj_t::groundobj_t(loadsave_t *file) : obj_t()
{
	rdwr(file);
}


groundobj_t::groundobj_t(koord3d pos, const groundobj_besch_t *b ) : obj_t(pos)
{
	groundobjtype = groundobj_typen.index_of(b);
	calc_bild();
}


bool groundobj_t::check_season(const bool)
{
	const image_id old_image = get_bild();
	calc_bild();

	if(  get_bild() != old_image  ) {
		mark_image_dirty( get_bild(), 0 );
	}
	return true;
}


void groundobj_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "groundobj_t" );

	obj_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = get_besch()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		groundobj_besch_t *besch = besch_names.get(bname);
		if(  besch_names.empty()  ||  besch==NULL  ) {
			groundobjtype = simrand(groundobj_typen.get_count());
		}
		else {
			groundobjtype = besch->get_index();
		}
		// if not there, besch will be zero
	}
}


/**
 * �ffnet ein neues Beobachtungsfenster f�r das Objekt.
 * @author Hj. Malthaner
 */
void groundobj_t::zeige_info()
{
	if(env_t::tree_info) {
		obj_t::zeige_info();
	}
}


/**
 * @return Einen Beschreibungsstring f�r das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void groundobj_t::info(cbuffer_t & buf) const
{
	obj_t::info(buf);

	buf.append(translator::translate(get_besch()->get_name()));
	if (char const* const maker = get_besch()->get_copyright()) {
		buf.append("\n");
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
	buf.append("\n");
	buf.append(translator::translate("cost for removal"));
	char buffer[128];
	money_to_string( buffer, get_besch()->get_preis()/100.0 );
	buf.append( buffer );
}


void groundobj_t::entferne(spieler_t *sp)
{
	spieler_t::book_construction_costs(sp, -get_besch()->get_preis(), get_pos().get_2d(), ignore_wt);
	mark_image_dirty( get_bild(), 0 );
}


void *groundobj_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(groundobj_t));
}


void groundobj_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(groundobj_t),p);
}
