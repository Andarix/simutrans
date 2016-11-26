/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __BILDLISTE_BESCH_H
#define __BILDLISTE_BESCH_H

#include "bild_besch.h"

/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      One-dimensional image list.
 *
 *  Child nodes:
 *	0   1st Bild
 *	1   2nd Bild
 *	... ...
 */
class image_list_t : public obj_besch_t {
    friend class imagelist_reader_t;

    uint16  count;

public:
	image_list_t() : count(0) {}

	uint16 get_count() const { return count; }

	image_t const* get_image(uint16 i) const { return i < count ? get_child<image_t>(i) : 0; }

	image_id get_image_id(uint16 i) const {
		const image_t *image = get_image(i);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}
};

#endif
