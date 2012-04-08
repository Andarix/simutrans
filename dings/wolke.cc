/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simtools.h"
#include "wolke.h"

#include "../dataobj/loadsave.h"

#include "../tpl/vector_tpl.h"


vector_tpl<const skin_besch_t *>wolke_t::all_clouds(0);

bool wolke_t::register_besch(const skin_besch_t* besch)
{
	// avoid duplicates with same name
	FOR(vector_tpl<skin_besch_t const*>, & i, all_clouds) {
		if (strcmp(i->get_name(), besch->get_name()) == 0) {
			i = besch;
			return true;
		}
	}
	return all_clouds.append_unique( besch );
}



wolke_t::wolke_t(karte_t *welt, koord3d pos, sint8 x_off, sint8 y_off, const skin_besch_t* besch, bool vehicle ) :
	ding_no_info_t(welt, pos)
{
	cloud_nr = all_clouds.index_of(besch);
	base_y_off = clamp( (((sint16)y_off-8)*OBJECT_OFFSET_STEPS)/16, -128, 127 );
	set_xoff( (x_off*OBJECT_OFFSET_STEPS)/16 );
	set_yoff( base_y_off );
	insta_zeit = 0;
	vehicle_smoke = vehicle;
}



wolke_t::~wolke_t()
{
	mark_image_dirty( get_bild(), 0 );
	if(  insta_zeit != 2499  ) {
		if(  vehicle_smoke  ) {
			welt->sync_way_eyecandy_remove( this );
		}
		else if(  welt->sync_eyecandy_remove( this )  ) {
		}
		else {
			welt->sync_way_eyecandy_remove( this );
			dbg->error( "wolke_t::~wolke_t()", "wolke not in bthe correct sync list" );
		}
	}
}



wolke_t::wolke_t(karte_t* const welt, loadsave_t* const file) : ding_no_info_t(welt)
{
	rdwr(file);
}


image_id wolke_t::get_bild() const
{
	const skin_besch_t *besch = all_clouds[cloud_nr];
	return besch->get_bild_nr( (insta_zeit*besch->get_bild_anzahl())/2500 );
}



void wolke_t::rdwr(loadsave_t *file)
{
	// not saving cloads! (and loading only for compatibility)
	assert(file->is_loading());

	ding_t::rdwr( file );

	cloud_nr = 0;
	insta_zeit = 0;

	uint32 ldummy = 0;
	file->rdwr_long(ldummy);

	uint16 dummy = 0;
	file->rdwr_short(dummy);
	file->rdwr_short(dummy);

	// do not remove from this position, since there will be nothing
	ding_t::set_flag(ding_t::not_on_map);
}



bool wolke_t::sync_step(long delta_t)
{
	insta_zeit += (uint16)delta_t;
	if(insta_zeit>=2499) {
		// delete wolke ...
		insta_zeit = 2499;
		return false;
	}
	// move cloud up
	sint8 ymove = ((insta_zeit*OBJECT_OFFSET_STEPS) >> 12);
	if(  base_y_off-ymove!=get_yoff()  ) {
		// move/change cloud ... (happens much more often than image change => image change will be always done when drawing)
		if(!get_flag(ding_t::dirty)) {
			mark_image_dirty(get_bild(),0);
		}
		set_yoff(  base_y_off - ymove  );
		set_flag(ding_t::dirty);
	}
	return true;
}


// called during map rotation
void wolke_t::rotate90()
{
	set_yoff( 0 );
	ding_t::rotate90();
	set_yoff( ((insta_zeit*OBJECT_OFFSET_STEPS) >> 12) + base_y_off );
}

/***************************** just for compatibility, the old raucher and smoke clouds *********************************/

raucher_t::raucher_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	assert(file->is_loading());
	ding_t::rdwr( file );
	const char *s = NULL;
	file->rdwr_str(s);
	free(const_cast<char *>(s));

	// do not remove from this position, since there will be nothing
	ding_t::set_flag(ding_t::not_on_map);
}


async_wolke_t::async_wolke_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	// not saving clouds!
	assert(file->is_loading());

	ding_t::rdwr( file );

	uint32 dummy;
	file->rdwr_long(dummy);

	// do not remove from this position, since there will be nothing
	ding_t::set_flag(ding_t::not_on_map);
}
