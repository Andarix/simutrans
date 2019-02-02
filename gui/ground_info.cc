/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * An adapter class to display info windows for ground (floor) objects
 *
 * @author Hj. Malthaner
 * @date 20-Nov-2001
 */

#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simworld.h"
#include "../dataobj/translator.h"
#include "ground_info.h"



grund_info_t::grund_info_t(const grund_t* gr_) :
	base_infowin_t(translator::translate(gr_->get_name())),
	gr(gr_),
	view(gr_->get_pos(), scr_size( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) ))
{
	const obj_t *const d = gr->obj_bei(0);
	if (  d!=NULL  ) {
		set_owner( d->get_owner() );
	}
	buf.clear();
	gr->info(buf);
	set_embedded(&view);
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void grund_info_t::draw(scr_coord pos, scr_size size)
{
	// update for owner and name change
	set_dirty();
	const obj_t *const d = gr->obj_bei(0);
	if (  d!=NULL  ) {
		set_owner( d->get_owner() );
	}
	base_infowin_t::set_name( translator::translate(gr->get_name()) );

	const cbuffer_t old_buf(buf);
	buf.clear();
	gr->info(buf);
	if(  strcmp( buf, old_buf )  ) {
		recalc_size();
	}

	base_infowin_t::draw(pos, size);
}


koord3d grund_info_t::get_weltpos(bool)
{
	return gr->get_pos();
}


bool grund_info_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center( gr->get_pos() ) );
}


void grund_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}
