/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __VEHIKEL_BESCH_H
#define __VEHIKEL_BESCH_H

#include "obj_besch_std_name.h"
#include "ware_besch.h"
#include "bildliste_besch.h"
#include "bildliste2d_besch.h"
#include "skin_besch.h"
#include "sound_besch.h"
#include "../dataobj/ribi.h"
#include "../simtypes.h"
#include "../simunits.h"


class checksum_t;

/**
 * Vehicle type description - all attributes of a vehicle type
 *
 *  child nodes:
 *	0   Name
 *	1   Copyright
 *	2   freight
 *	3   smoke
 *	4   empty 1d image list
 *	5   either 1d (freight_image_type==0) or 2d image list
 *	6   required leading vehicle 1
 *	7   required leading vehicle 2
 *	... ...
 *	n+5 required leading vehicle n
 *	n+6 allowed trailing vehicle 1
 *	n+7 allowed trailing vehicle 2
 *	... ...
 *	n+m+5 allowed trailing vehicle m
 *  n+m+6 freight for which special images are defined
 *
 * @author Volker Meyer, Hj. Malthaner, kierongreen
 */
class vehikel_besch_t : public obj_desc_transport_related_t {
    friend class vehicle_reader_t;
    friend class vehicle_builder_t;

public:
	/**
	 * Engine type
	 * @author Hj. Malthaner
	 */
	enum engine_t {
		 unknown=-1,
		steam=0,
		diesel,
		electric,
		bio,
		sail,
		fuel_cell,
		hydrogene,
		battery
	};


private:
	uint16  capacity;
	uint16  loading_time;	// time per full loading/unloading
	uint32  weight;
	uint32  power;
	uint16  running_cost;
	uint32  fixed_cost;

	uint16  gear;       // engine gear (power multiplier), 64=100

	uint8 len;			// length (=8 is half a tile, the old default)
	sint8 sound;

	uint8  leader_count;	// all defined leading vehicles
	uint8  trailer_count;	// all defined trailer

	uint8  engine_type; // diesel, steam, electric (requires electrified ways), fuel_cell, etc.

	sint8 freight_image_type;	// number of freight images (displayed for different goods)


public:
	// since we have a second constructor
	vehikel_besch_t() { }

	// default vehicle (used for way seach and similar tasks)
	// since it has no images and not even a name knot any calls to this will case a crash
	vehikel_besch_t(uint8 wtyp, uint16 speed, engine_t engine) {
		freight_image_type = cost = capacity = axle_load = running_cost = fixed_cost = intro_date = leader_count = trailer_count = 0;
		power = weight = 1;
		loading_time = 1000;
		gear = 64;
		len = 8;
		sound = -1;
		wt = wtyp;
		engine_type = (uint8)engine;
		topspeed = speed;
	}

	ware_besch_t const* get_ware() const { return get_child<ware_besch_t>(2); }

	skin_besch_t const* get_smoke() const { return get_child<skin_besch_t>(3); }

	image_id get_base_image() const { return get_image_id(ribi_t::dir_south, get_ware() ); }

	// returns the number of different directions
	uint8 get_dirs() const { return get_child<image_list_t>(4)->get_image(4) ? 8 : 4; }

	// return a matching image
	// beware, there are three class of vehicles
	// vehicles with and without freight images, and vehicles with different freight images
	// they can have 4 or 8 directions ...
	image_id get_image_id(ribi_t::dir dir, const ware_besch_t *ware) const
	{
		const image_t *image=0;
		const image_list_t *liste=0;

		if(freight_image_type>0  &&  ware!=NULL) {
			// more freight images and a freight: find the right one

			sint8 ware_index=0; // freight images: if not found use first freight

			for( sint8 i=0;  i<freight_image_type;  i++  ) {
				if (ware == get_child<ware_besch_t>(6 + trailer_count + leader_count + i)) {
					ware_index = i;
					break;
				}
			}

			// vehicle has freight images and we want to use - get appropriate one (if no list then fallback to empty image)
			image_array_t const* const liste2d = get_child<image_array_t>(5);
			image=liste2d->get_image(dir, ware_index);
			if(!image) {
				if(dir>3) {
					image = liste2d->get_image(dir - 4, ware_index);
				}
			}
			if (image != NULL) return image->get_id();
		}

		// only try 1d freight image list for old style vehicles
		if(freight_image_type==0  &&  ware!=NULL) {
			liste = get_child<image_list_t>(5);
		}

		if(!liste) {
			liste = get_child<image_list_t>(4);
			if(!liste) {
				return IMG_EMPTY;
			}
		}

		image = liste->get_image(dir);
		if(!image) {
			if(dir>3) {
				image = liste->get_image(dir - 4);
			}
			if(!image) {
				return IMG_EMPTY;
			}
		}
		return image->get_id();
	}

	// Liefert die erlaubten Vorgaenger.
	// liefert get_leader(0) == NULL, so bedeutet das entweder alle
	// Vorg�nger sind erlaubt oder keine. Um das zu unterscheiden, sollte man
	// vorher hat_vorgaenger() befragen
	const vehikel_besch_t *get_leader(uint8 i) const
	{
		if(  i >= leader_count  ) {
			return 0;
		}
		return get_child<vehikel_besch_t>(6 + i);
	}

	uint8 get_leader_count() const { return leader_count; }

	// Liefert die erlaubten Nachfolger.
	// liefert get_trailer(0) == NULL, so bedeutet das entweder alle
	// Nachfolger sind erlaubt oder keine. Um das zu unterscheiden, sollte
	// man vorher hat_nachfolger() befragen
	const vehikel_besch_t *get_trailer(uint8 i) const
	{
		if(  i >= trailer_count  ) {
			return 0;
		}
		return get_child<vehikel_besch_t>(6 + leader_count + i);
	}

	uint8 get_trailer_count() const { return trailer_count; }

	/* returns true, if this veh can be before the next_veh
	 * uses NULL to indicate end of convoi
	 */
	bool can_lead(const vehikel_besch_t *next_veh) const
	{
		if(  trailer_count==0  ) {
			return true;
		}
		for( uint8 i=0;  i<trailer_count;  i++  ) {
			vehikel_besch_t const* const veh = get_child<vehikel_besch_t>(6 + leader_count + i);
			if(veh==next_veh) {
				return true;
			}
		}
		// only here if not allowed
		return false;
	}

	/* returns true, if this veh can be after the prev_veh
	 * uses NULL to indicate front of convoi
	 */
	bool can_follow(const vehikel_besch_t *prev_veh) const
	{
		if(  leader_count==0  ) {
			return true;
		}
		for( uint8 i=0;  i<leader_count;  i++  ) {
			vehikel_besch_t const* const veh = get_child<vehikel_besch_t>(6 + i);
			if(veh==prev_veh) {
				return true;
			}
		}
		// only here if not allowed
		return false;
	}

	bool can_follow_any() const { return trailer_count==0; }

	uint16 get_capacity() const { return capacity; }
	uint16 get_loading_time() const { return loading_time; } // ms per full loading/unloading
	uint32 get_weight() const { return weight; }
	uint32 get_power() const { return power; }
	uint32 get_running_cost() const { return running_cost; }
	uint16 get_maintenance() const { return fixed_cost; }
	sint8 get_sound() const { return sound; }

	/**
	* 64 = 1.00
	* @return gear value
	* @author Hj. Malthaner
	*/
	uint16 get_gear() const { return gear; }

	/**
	* @return engine type
	* eletric engines require an electrified way to run
	* @author Hj. Malthaner
	*/
	uint8 get_engine_type() const { return engine_type; }

	/* @return the vehicles length in 1/8 of the normal len
	* @author prissi
	*/
	uint8 get_length() const { return len; }
	uint32 get_length_in_steps() const { return get_length() * VEHICLE_STEPS_PER_CARUNIT; }

	void calc_checksum(checksum_t *chk) const;
};

#endif
