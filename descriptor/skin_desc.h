/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __SKIN_DESC_H
#define __SKIN_DESC_H

#include "../display/simimg.h"
#include "obj_base_desc.h"
#include "image_array.h"


/*
 *  Author:
 *      Volker Meyer
 *
 *  Description:
 *      An image list, with name and author attributes. Mostly used for gui purposes.
 *
 *  Child nodes:
 *	0   Name
 *	1   Copyright
 *	2   Image list
 */
class skin_desc_t : public obj_named_desc_t {
public:
	image_t const* get_image(uint16 i) const { return get_child<image_list_t>(2)->get_image(i); }

	uint16 get_count() const { return get_child<image_list_t>(2)->get_count(); }

	image_id get_image_id(uint16 i) const
	{
		const image_t *image = get_image(i);
		return image != NULL ? image->get_id() : IMG_EMPTY;
	}
};

#endif
