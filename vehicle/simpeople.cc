/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../display/simimg.h"

#include "../utils/simrandom.h"
#include "../boden/grund.h"
#include "../dataobj/loadsave.h"

#include "simpeople.h"
#include "../descriptor/pedestrian_desc.h"

static uint32 const strecke[] = { 6000, 11000, 15000, 20000, 25000, 30000, 35000, 40000 };

static weighted_vector_tpl<const pedestrian_desc_t*> list;
stringhashtable_tpl<const pedestrian_desc_t *> pedestrian_t::table;


static bool compare_fussgaenger_desc(const pedestrian_desc_t* a, const pedestrian_desc_t* b)
{
	// sort pedestrian objects descriptors by their name
	return strcmp(a->get_name(), b->get_name())<0;
}


bool pedestrian_t::register_desc(const pedestrian_desc_t *desc)
{
	if(  table.remove(desc->get_name())  ) {
		dbg->doubled( "pedestrian", desc->get_name() );
	}
	table.put(desc->get_name(), desc);
	return true;
}


bool pedestrian_t::successfully_loaded()
{
	list.resize(table.get_count());
	if (table.empty()) {
		DBG_MESSAGE("pedestrian_t", "No pedestrians found - feature disabled");
	}
	else {
		vector_tpl<const pedestrian_desc_t*> temp_liste(0);
		FOR(stringhashtable_tpl<pedestrian_desc_t const*>, const& i, table) {
			// just entered them sorted
			temp_liste.insert_ordered(i.value, compare_fussgaenger_desc);
		}
		FOR(vector_tpl<pedestrian_desc_t const*>, const i, temp_liste) {
			list.append(i, i->get_distribution_weight());
		}
	}
	return true;
}


pedestrian_t::pedestrian_t(loadsave_t *file)
 : road_user_t()
{
	animation_steps = 0;
	on_left = false;
	steps_offset = 0;
	rdwr(file);
	if(desc) {
		welt->sync.add(this);
		ped_offset = desc->get_offset();
	}
	calc_disp_lane();
}


pedestrian_t::pedestrian_t(grund_t *gr) :
	road_user_t(gr, simrand(65535)),
	desc(pick_any_weighted(list))
{
	animation_steps = 0;
	on_left = simrand(2) > 0;
	steps_offset = 0;
	time_to_life = pick_any(strecke);
	ped_offset = desc->get_offset();
	calc_image();
	calc_disp_lane();
}


pedestrian_t::~pedestrian_t()
{
	if(  time_to_life>0  ) {
		welt->sync.remove( this );
	}
}


void pedestrian_t::calc_image()
{
	set_image(desc->get_image_id(ribi_t::get_dir(get_direction())));
}


image_id pedestrian_t::get_image() const
{
	if (desc->get_steps_per_frame() > 0) {
		uint16 frame = ((animation_steps + steps) / desc->get_steps_per_frame()) % desc->get_animation_count(ribi_t::get_dir(direction));
		return desc->get_image_id(ribi_t::get_dir(get_direction()), frame);
	}
	else {
		return image;
	}
}


void pedestrian_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fussgaenger_t" );

	road_user_t::rdwr(file);

	if(!file->is_loading()) {
		const char *s = desc->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		desc = table.get(s);
		// unknown pedestrian => create random new one
		if(desc == NULL  &&  !list.empty()  ) {
			desc = pick_any_weighted(list);
		}
	}

	if(file->get_version()<89004) {
		time_to_life = pick_any(strecke);
	}

	if (file->get_version() > 120005) {
		file->rdwr_short(steps_offset);
		file->rdwr_bool(on_left);
	}

	if (file->is_loading()) {
		calc_disp_lane();
	}
}

void pedestrian_t::calc_disp_lane()
{
	// walking in the back or the front
	ribi_t::ribi test_dir = on_left ? ribi_t::northeast : ribi_t::southwest;
	disp_lane = direction & test_dir ? 0 : 4;
}


void pedestrian_t::rotate90()
{
	road_user_t::rotate90();
	calc_disp_lane();
}

// create a number (count) of pedestrians (if possible)
void pedestrian_t::generate_pedestrians_at(const koord3d k, int &count)
{
	if (list.empty()) {
		return;
	}

	grund_t* bd = welt->lookup(k);
	if (bd) {
		const weg_t* weg = bd->get_weg(road_wt);

		// we do not start on crossings (not overrunning pedestrians please
		if (weg && ribi_t::is_twoway(weg->get_ribi_unmasked())) {
			// we create maximal 4 pedestrians here for performance reasons
			for (int i = 0; i < 4 && count > 0; i++) {
				pedestrian_t* fg = new pedestrian_t(bd);
				bool ok = bd->obj_add(fg) != 0;	// 256 limit reached
				if (ok) {
					fg->calc_height(bd);
					if (i > 0) {
						// walk a little
						fg->sync_step( (i & 3) * 64 * 24);
					}
					welt->sync.add(fg);
					count--;
				}
				else {
					// delete it, if we could not put it on the map
					fg->set_flag(obj_t::not_on_map);
					// do not try to delete it from sync-list
					fg->time_to_life = 0;
					delete fg;
					return; // it is pointless to try again
				}
			}
		}
	}
}


sync_result pedestrian_t::sync_step(uint32 delta_t)
{
	time_to_life -= delta_t;

	if (time_to_life>0) {
		weg_next += 128*delta_t;
		weg_next -= do_drive( weg_next );
		return time_to_life>0 ? SYNC_OK : SYNC_DELETE;
	}
	return SYNC_DELETE;
}


grund_t* pedestrian_t::hop_check()
{
	grund_t *from = welt->lookup(pos_next);
	if(!from) {
		time_to_life = 0;
		return NULL;
	}

	// find the allowed directions
	const weg_t *weg = from->get_weg(road_wt);
	if(weg==NULL) {
		// no road anymore: destroy it
		time_to_life = 0;
		return NULL;
	}
	return from;
}


void pedestrian_t::hop(grund_t *gr)
{
	koord3d from = get_pos();

	// hop
	leave_tile();
	set_pos(gr->get_pos());
	// no need to call enter_tile();
	gr->obj_add(this);

	// determine pos_next
	const weg_t *weg = gr->get_weg(road_wt);
	// new target
	grund_t *to = NULL;
	// current single direction
	ribi_t::ribi current_direction = get_direction();
	if (!ribi_t::is_single(current_direction)) {
		current_direction = ribi_type(from, get_pos());
	}
	// ribi opposite to current direction
	ribi_t::ribi reverse_direction = ribi_t::reverse_single( current_direction );
	// all possible directions
	ribi_t::ribi ribi = weg->get_ribi_unmasked() & (~reverse_direction);
	// randomized offset
	const uint8 offset = (ribi > 0  &&  ribi_t::is_single(ribi)) ? 0 : simrand(4);

	ribi_t::ribi new_direction;
	for(uint r = 0; r < 4; r++) {
		new_direction = ribi_t::nsew[ (r+offset) & 3];

		if(  (ribi & new_direction)!=0  &&  gr->get_neighbour(to, road_wt, new_direction) ) {
			// this is our next target
			break;
		}
	}
	steps_offset = 0;

	if (to) {
		pos_next = to->get_pos();

		if (new_direction == current_direction) {
			// going straight
			direction = calc_set_direction(get_pos(), pos_next);
		}
		else {
			ribi_t::ribi turn_ribi = on_left ? ribi_t::rotate90l(current_direction) : ribi_t::rotate90(current_direction);

			if (turn_ribi == new_direction) {
				// short diagonal (turn but do not cross street)
				direction = calc_set_direction(from, pos_next);
				steps_next = (ped_offset*181) / 128; // * sqrt(2)
				steps_offset = 0;
			}
			else {
				// do not cross street diagonally, change side
				on_left = !on_left;
				direction = calc_set_direction(get_pos(), pos_next);
			}
		}
	}
	else {
		// dead end, turn
		pos_next = from;
		direction = calc_set_direction(get_pos(), pos_next);
		steps_offset = VEHICLE_STEPS_PER_TILE - ped_offset;
		steps_next   = ped_offset;
		on_left = !on_left;
	}

	calc_disp_lane();
	// carry over remainder to next tile for continuous animation during straight movement
	uint16 steps_per_animation = desc->get_steps_per_frame() * desc->get_animation_count(ribi_t::get_dir(direction));
	if (steps_per_animation > 0) {
		animation_steps = (animation_steps + steps_next + 1) % steps_per_animation;
	}
	else {
		animation_steps = 0;
	}

	calc_image();
}

void pedestrian_t::get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const
{
	// vehicles needs finer steps to appear smoother
	sint32 display_steps = (uint32)(steps + steps_offset)*(uint16)raster_width;
	if(dx*dy) {
		display_steps &= 0xFFFFFC00;
	}
	else {
		display_steps = (display_steps*diagonal_multiplier)>>10;
	}
	xoff += (display_steps*dx) >> 10;
	yoff += ((display_steps*dy) >> 10) + (get_hoff(raster_width))/(4*16);

	if (on_left) {
		sint32 left_off_steps = ( (VEHICLE_STEPS_PER_TILE - 2*ped_offset)*(uint16)raster_width ) & 0xFFFFFC00;

		if (dx*dy==0) {
			// diagonal
			left_off_steps /= 2;
		}
		// turn left (dx,dy) increments
		xoff += (left_off_steps*2*dy) >> 10;
		yoff -= (left_off_steps*dx) >> (10+1);
	}
}

