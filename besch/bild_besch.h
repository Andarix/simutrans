/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __BILD_BESCH_H
#define __BILD_BESCH_H

#include "../simgraph.h"
#include "../simimg.h"
#include "obj_besch.h"


// number of special colors
#define SPECIAL (31)

//#define TRANSPARENT 0x808088
#define SPECIAL_TRANSPARENT (0xE7FFFF)


struct bild_t {
	sint16 x;
	sint16 y;
	sint16 w;
	sint16 h;
	uint8 zoomable; // some image may not be zoomed i.e. icons
	uint32 len;	// since the maximum size of a node is uint16!
	image_id bild_nr;	// Speichern wir erstmal als Dummy mit, wird von register_image() ersetzt
	uint16 data[];
};

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Beschreibung eines Bildes.
 *
 *  Kindknoten:
 *	(keine)
 */
class bild_besch_t : public obj_besch_t
{
public:
	static const uint32 rgbtab[SPECIAL];

	const bild_t* get_pic() const { return &pic; }

	uint16 const* get_daten() const { return pic.data; }
	uint16*       get_daten()       { return pic.data; }

	image_id get_nummer() const { return pic.bild_nr; }

	/* rotate_image_data - produces a (rotated) bild_besch
	 * only rotates by 90 degrees or multiples thereof, and assumes a square image
	 * Otherwise it will only succeed for angle=0;
	 */
	bild_besch_t* copy_rotate(const sint16 angle) const;

	void register_image() { ::register_image(&pic); }

	using obj_besch_t::operator new;

	// decodes this image into a 32 bit bitmap with width target_width
	void decode_img( sint16 xoff, sint16 yoff, uint32 *target, uint32 target_width, uint32 target_height ) const;

private:
	bild_t pic;

	friend class image_reader_t;
};

#endif
