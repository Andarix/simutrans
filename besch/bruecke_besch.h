/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 *
 *  BEWARE: non-standard node structure!
 *	0   Foreground-images
 *	1   Background-images
 *	2   Cursor/Icon (Hajo: 14-Feb-02: now also icon image)
 *	3   Foreground-images - snow
 *	4   Background-images - snow
 */

#ifndef __BRUECKE_BESCH_H
#define __BRUECKE_BESCH_H


#include "skin_besch.h"
#include "bildliste_besch.h"
#include "text_besch.h"
#include "../simtypes.h"
#include "../display/simimg.h"

#include "../dataobj/ribi.h"

class tool_t;
class checksum_t;


class bruecke_besch_t : public obj_desc_transport_infrastructure_t {
    friend class bridge_reader_t;

private:
	uint8 pillars_every;	// =0 off
	bool pillars_asymmetric;	// =0 off else leave one off for north/west slopes
	uint offset;	// flag, because old bridges had their name/copyright at the wrong position

	uint8 max_length;	// =0 off, else maximum length
	uint8 max_height;	// =0 off, else maximum length

	/* number of seasons (0 = none, 1 = no snow/snow
	*/

	sint8 number_seasons;

public:
	/*
	 * Nummerierung all der verschiedenen Schienst�cke
	 */
	enum img_t {
		NS_Segment, OW_Segment, N_Start, S_Start, O_Start, W_Start, N_Rampe, S_Rampe, O_Rampe, W_Rampe, NS_Pillar, OW_Pillar,
		NS_Segment2, OW_Segment2, N_Start2, S_Start2, O_Start2, W_Start2, N_Rampe2, S_Rampe2, O_Rampe2, W_Rampe2, NS_Pillar2, OW_Pillar2
	};

	/*
	 * Name und Copyright sind beim Cursor gespeichert!
	 */
	const char *get_name() const { return get_cursor()->get_name(); }
	const char *get_copyright() const { return get_cursor()->get_copyright(); }

	skin_besch_t const* get_cursor() const { return get_child<skin_besch_t>(2 + offset); }

	image_id get_background(img_t img, uint8 season) const 	{
		const image_t *image = NULL;
		if(season && number_seasons == 1) {
			image = get_child<image_list_t>(3 + offset)->get_image(img);
		}
		if(image == NULL) {
			image = get_child<image_list_t>(0 + offset)->get_image(img);
		}
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	image_id get_foreground(img_t img, uint8 season) const {
		const image_t *image = NULL;
		if(season && number_seasons == 1) {
			image = get_child<image_list_t>(4 + offset)->get_image(img);
		}
		if(image == NULL) {
			image = get_child<image_list_t>(1 + offset)->get_image(img);
		}
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}

	img_t get_simple(ribi_t::ribi ribi, uint8 height) const;
	img_t get_start(slope_t::type slope) const;
	img_t get_rampe(slope_t::type slope) const;
	static img_t get_pillar(ribi_t::ribi ribi);

	/**
	 * @return true if this bridge can raise two level from flat terrain
	 */
	bool has_double_ramp() const;

	/**
	 * @return true if this bridge can start or end on a double slope
	 */
	bool has_double_start() const;

	img_t get_end(slope_t::type test_slope, slope_t::type ground_slope, slope_t::type way_slope) const;

	/**
	 * There is no way to distinguish between train bridge and tram bridge.
	 * However there are no real tram bridges possible in the game.
	 */
	waytype_t get_finance_waytype() const { return get_waytype(); }

	/**
	 * Distance of pillars (=0 for no pillars)
	 * @author prissi
	 */
	uint8  get_pillar() const { return pillars_every; }

	/**
	 * skips lowest pillar on south/west slopes?
	 * @author prissi
	 */
	bool  has_pillar_asymmetric() const { return pillars_asymmetric; }

	/**
	 * maximum bridge span (=0 for infinite)
	 * @author prissi
	 */
	uint8  get_max_length() const { return max_length; }

	/**
	 * maximum bridge height (=0 for infinite)
	 * @author prissi
	 */
	uint8  get_max_height() const { return max_height; }

	void calc_checksum(checksum_t *chk) const;
};

#endif
