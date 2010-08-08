/*
 * Copyright (c) 2007 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../vehicle/simvehikel.h"
#include "../simworld.h"
#include "../simsound.h"

#include "translator.h"

#include "../besch/kreuzung_besch.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../tpl/slist_tpl.h"

#include "crossing_logic.h"
#include "../dings/crossing.h"


karte_t *crossing_logic_t::welt = NULL;



crossing_logic_t::crossing_logic_t( const kreuzung_besch_t *besch )
{
	zustand = CROSSING_INVALID;
	this->besch = besch;
	request_close = NULL;
}



/**
 * @return string; currently unused but useful for debugging
 * @author prissi
 */
void crossing_logic_t::info(cbuffer_t & buf) const
{
	static const char *state_str[4] = { "invalid", "open", "request closing", "closed" };
	assert(zustand<4);
	buf.append( translator::translate("\nway1 reserved by") );
	buf.append( on_way1.get_count() );
	buf.append( translator::translate("\nway2 reserved by") );
	buf.append( on_way2.get_count() );
	buf.append( translator::translate( "cars.\nstate") );
	buf.append( translator::translate(state_str[zustand]) );
}



// after merging two crossings ...
void crossing_logic_t::recalc_state()
{
	if(  !crossings.empty()  ) {
		on_way1.clear();
		on_way2.clear();
		for(  uint i=0;  i<crossings.get_count();  i++  ) {
			// add vehicles already there
			grund_t *gr = welt->lookup(crossings[i]->get_pos());
			if(gr) {
				for( uint8 i=3;  i<gr->get_top();  i++  ) {
					if (vehikel_basis_t const* const v = ding_cast<vehikel_basis_t>(gr->obj_bei(i))) {
						add_to_crossing( v );
					}
				}
			}
		}
	}
	request_close = NULL;
	if(zustand==CROSSING_INVALID) {
		// now just set the state, if needed
		if(on_way2.empty()) {
			set_state( CROSSING_OPEN );
		}
		else {
			set_state( CROSSING_CLOSED );
		}
	}
}



// request permission to pass crossing
bool crossing_logic_t::request_crossing( const vehikel_basis_t *v )
{
	if(v->get_waytype()==besch->get_waytype(0)) {
		if(on_way2.empty()  &&  zustand == CROSSING_OPEN) {
			// way2 is empty ...
			return true;
		}
		// passage denied, since there are vehicle on way2
		// which has priority
		// => ok only if I am already crossing
		return on_way1.is_contained(v);
	}
	else if(v->get_waytype()==besch->get_waytype(1)) {

		// vehikel from way2 arrives
		if(on_way1.get_count()) {
			// sorry, still things on the crossing, but we will prepare
			set_state( CROSSING_REQUEST_CLOSE );
			return false;
		}
		else {
			request_close = v;
			set_state( CROSSING_CLOSED );
			return true;
		}
	}
	// likely an airplane ...
	return true;
}



// request permission to pass crossing
void crossing_logic_t::add_to_crossing( const vehikel_basis_t *v )
{
	if(v->get_waytype()==besch->get_waytype(0)) {
		on_way1.append_unique(v);
	}
 	else if(v->get_waytype()==besch->get_waytype(1)) {
		// add it and close crossing
		on_way2.append_unique(v);
		if(  request_close==v  ) {
			request_close = NULL;
		}
		set_state( CROSSING_CLOSED );
	}
}



// called after passing of the last vehicle (in a convoi)
// or of a city car; releases the crossing which may switch state
void
crossing_logic_t::release_crossing( const vehikel_basis_t *v )
{
	if(v->get_waytype()==besch->get_waytype(0)) {
		on_way1.remove(v);
		if(zustand == CROSSING_REQUEST_CLOSE  &&  on_way1.empty()) {
			set_state( CROSSING_CLOSED );
		}
	}
	else {
		on_way2.remove(v);
		if(  request_close==v  ) {
			request_close = NULL;
		}
		if(on_way2.empty()  &&  request_close==NULL) {
			set_state( CROSSING_OPEN );
		}
	}
}



// change state; mark dirty and plays sound
void
crossing_logic_t::set_state( crossing_state_t new_state )
{
	// play sound (if there and closing)
	if(new_state==CROSSING_CLOSED  &&  besch->get_sound()>=0  &&  !welt->is_fast_forward()) {
		struct sound_info info;
		info.index = besch->get_sound();
		info.volume = 255;
		info.pri = 0;
		welt->play_sound_area_clipped(crossings[0]->get_pos().get_2d(), info);
	}

	if(new_state!=zustand) {
		zustand = new_state;
		for(  uint8 i=0;  i<crossings.get_count();  i++  ) {
			crossings[i]->state_changed();
		}
	}
}



/* static stuff from here on ... */


/**
 * nothing can cross airways, so waytype 0..7 is enough
 * only save this entries:
 * way0 way1
 *  0 .. 1 2 3 4 5 6 7 8
 *  1 ..   2 3 4 5 6 7 8
 *  2 ..     3 4 5 6 7 8
 * ..          ...
 */
minivec_tpl<const kreuzung_besch_t *> crossing_logic_t::can_cross_array[36];

void crossing_logic_t::register_besch(kreuzung_besch_t *besch)
{
	// mark if crossing possible
	const waytype_t way0 = (const waytype_t)min(besch->get_waytype(0), besch->get_waytype(1));
	const waytype_t way1 = (const waytype_t)max(besch->get_waytype(0), besch->get_waytype(1));
	if(way0<8  &&  way1<9  &&  way0<way1) {
		uint8 index = way0 * 9 + way1 - ((way0+2)*(way0+1))/2;
		// max index = 7*9 + 8 - 9*4 = 71-36 = 35
		// .. overwrite double entries
		minivec_tpl<const kreuzung_besch_t *> &vec = can_cross_array[index];
		for(uint8 i=0; i<vec.get_count(); i++) {
			if (strcmp(vec[i]->get_name(), besch->get_name())==0) {
				vec[i] = besch;
				dbg->warning( "crossing_logic_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
				return;
			}
		}
		vec.append(besch);
	}
DBG_DEBUG( "crossing_logic_t::register_besch()","%s", besch->get_name() );
}

const kreuzung_besch_t *crossing_logic_t::get_crossing(const waytype_t ns, const waytype_t ow, uint16 timeline_year_month)
{
	// mark if crossing possible
	const waytype_t way0 = ns <  ow ? ns : ow;
	const waytype_t way1 = ns >= ow ? ns : ow;
	if(way0<8  &&  way1<9  &&  way0!=way1) {
		uint8 index = way0 * 9 + way1 - ((way0+2)*(way0+1))/2;
		minivec_tpl<const kreuzung_besch_t *> &vec = can_cross_array[index];
		for(uint8 i=0; i<vec.get_count(); i++) {
			if (vec[i]->is_available(timeline_year_month)) {
				return vec[i];
			}
		}
	}
	return NULL;
}

// returns a new or an existing crossing_logic_t object
// new, of no matching crossings are next to it
void crossing_logic_t::add( karte_t *w, crossing_t *start_cr, crossing_state_t zustand )
{
	koord3d pos = start_cr->get_pos();
	const koord zv = start_cr->get_dir() ? koord::west : koord::nord;
	slist_tpl<crossing_t *>crossings;
	minivec_tpl<crossing_logic_t *>crossings_logics;
	welt = w;

	crossings.append( start_cr );
	// go nord/west
	while(1) {
		pos += zv;
		grund_t *gr = welt->lookup( pos );
		if(gr==NULL) {
			break;
		}
		crossing_t *found_cr = gr->find<crossing_t>();
		if(found_cr==NULL  ||  found_cr->get_besch()!=start_cr->get_besch()) {
			break;
		}
		crossings.append( found_cr );
		if(  found_cr->get_logic()!=NULL  ) {
			crossings_logics.append_unique( found_cr->get_logic() );
		}
	}
	// go east/south
	pos = start_cr->get_pos();
	while(1) {
		pos -= zv;
		grund_t *gr = welt->lookup( pos );
		if(gr==NULL) {
			break;
		}
		crossing_t *found_cr = gr->find<crossing_t>();
		if(found_cr==NULL  ||  found_cr->get_besch()!=start_cr->get_besch()) {
			break;
		}
		crossings.append( found_cr );
		if(  found_cr->get_logic()!=NULL  ) {
			crossings_logics.append_unique( found_cr->get_logic() );
		}
	}
	// remove all old crossing logics
	crossing_logic_t *found_logic = NULL;
	if(	crossings_logics.get_count()>=1  ) {
		// leave one logic to be used further
		while(  crossings_logics.get_count()>1  ) {
			crossing_logic_t *cl = crossings_logics[0];
			crossings_logics.remove_at(0);
			delete cl;
		}
		found_logic = crossings_logics[0];
	}
	// no old logic there create a new one
	if(  found_logic == NULL ) {
		found_logic = new crossing_logic_t( start_cr->get_besch() );
	}
	crossings.append(start_cr);

	// set new crossing logic to all
	slist_iterator_tpl<crossing_t *> iter(crossings);
	while( iter.next() ) {
		crossing_t *cr=iter.get_current();
		cr->set_logic( found_logic );
		found_logic->append_crossing( cr );
	}
	found_logic->recalc_state();
	if(  zustand!=CROSSING_INVALID  ) {
		found_logic->set_state( zustand );
	}
}



// removes a crossing logic, if all crossings are removed
void crossing_logic_t::remove( crossing_t *cr )
{
	crossings.remove( cr );
	if(  crossings.empty()  ) {
		delete this;
	}
	else {
		// because we do not know, which tile is going where
		zustand = CROSSING_INVALID;
		for(  uint i=0;  i<crossings.get_count();  i++  ) {
			add( welt, crossings[i], CROSSING_INVALID );
		}
		recalc_state();
	}
}
