#include <stdio.h>
#include <string.h>

#include "../../display/simgraph.h"
#include "../../simdebug.h"
#include "../../display/simimg.h"

#include "../bild_besch.h"
#include "image_reader.h"
#include "../obj_node_info.h"

#include <zlib.h>
#include "../../tpl/inthashtable_tpl.h"

// if without graphics backend, do not copy any pixel
#if COLOUR_DEPTH != 0
#define skip_reading_pixels_if_no_graphics
#else
#define skip_reading_pixels_if_no_graphics goto adjust_image
#endif

obj_desc_t *image_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);
	image_t* desc=NULL;

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf+6;

	// always zero in old version, since length was always less than 65535
	// because a node could not hold more data
	uint8 version = decode_uint8(p);
	p = besch_buf;

#if COLOUR_DEPTH != 0
	desc = new image_t();
#else
	// reserve space for one single pixel and initialize data
	desc = image_t::create_single_pixel();
#endif

	if(version==0) {
		desc->x = decode_uint8(p);
		desc->w = decode_uint8(p);
		desc->y = decode_uint8(p);
		desc->h = decode_uint8(p);
		desc->alloc(decode_uint32(p)); // len
		desc->imageid = IMG_EMPTY;
		p += 2;	// dummys
		desc->zoomable = decode_uint8(p);

		skip_reading_pixels_if_no_graphics;
		//DBG_DEBUG("image_t::read_node()","x,y=%d,%d  w,h=%d,%d, len=%i",desc->x,desc->y,desc->w,desc->h, desc->len);

		uint16* dest = desc->data;
		p = besch_buf+12;

		if (desc->h > 0) {
			for (uint i = 0; i < desc->len; i++) {
				uint16 data = decode_uint16(p);
				if(data>=0x8000u  &&  data<=0x800Fu) {
					// player color offset changed
					data ++;
				}
				*dest++ = data;
			}
		}
	}
	else if(version<=2) {
		desc->x = decode_sint16(p);
		desc->y = decode_sint16(p);
		desc->w = decode_uint8(p);
		desc->h = decode_uint8(p);
		p++; // skip version information
		desc->alloc(decode_uint16(p)); // len
		desc->zoomable = decode_uint8(p);
		desc->imageid = IMG_EMPTY;

		skip_reading_pixels_if_no_graphics;
		uint16* dest = desc->data;
		if (desc->h > 0) {
			for (uint i = 0; i < desc->len; i++) {
				*dest++ = decode_uint16(p);
			}
		}
	}
	else if(version==3) {
		desc->x = decode_sint16(p);
		desc->y = decode_sint16(p);
		desc->w = decode_sint16(p);
		p++; // skip version information
		desc->h = decode_sint16(p);
		desc->alloc((node.size-10)/2); // len
		desc->zoomable = decode_uint8(p);
		desc->imageid = IMG_EMPTY;

		skip_reading_pixels_if_no_graphics;
		uint16* dest = desc->data;
		if (desc->h > 0) {
			for (uint i = 0; i < desc->len; i++) {
				*dest++ = decode_uint16(p);
			}
		}
	}
	else {
		dbg->fatal("image_reader_t::read_node()","illegal versions %d", version );
	}

#if COLOUR_DEPTH == 0
adjust_image:
	// reset image parameters, but only for non-empty images
	if(  desc->h > 0  ) {
		desc->h = 1;
	}
	if(  desc->w > 0  ) {
		desc->w = 1;
	}
	if(  desc->len > 0  ) {
		desc->len = 4;
	}
	desc->x = 0;
	desc->y = 0;
#endif

	// check for left corner
	if(version<2  &&  desc->h>0) {
		// find left left border
		uint16 left = 255;
		uint16 *dest = desc->data;
		for( uint8 y=0;  y<desc->h;  y++  ) {
			if(*dest<left) {
				left = *dest;
			}
			// skip rest of the line
			do {
				dest++;
				dest += *dest + 1;
			} while (*dest);
			dest++;	// skip trailing zero
		}

		if(left<desc->x) {
			dbg->warning( "image_reader_t::read_node()","left(%i)<x(%i) (may be intended)", left, desc->x );
		}

		dest = desc->data;
		for( uint8 y=0;  y<desc->h;  y++  ) {
			*dest -= left;
			// skip rest of the line
			do {
				dest++;
				dest += *dest + 1;
			} while (*dest);
			dest++;	// skip trailing zero
		}
	}

	if (desc->len != 0) {
		// get the adler hash (since we have zlib on board anyway ... )
		bool do_register_image = true;
		uint32 adler = adler32(0L, NULL, 0 );
		// remember len is sizeof(uint16)!
		adler = adler32(adler, (const Bytef *)(desc->data), desc->len*2 );
		static inthashtable_tpl<uint32, image_t *> images_adlers;
		image_t *same = images_adlers.get(adler);
		if (same) {
			// same checksum => if same then skip!
			image_t const& a = *desc;
			image_t const& b = *same;
			do_register_image =
				a.x        != b.x        ||
				a.y        != b.y        ||
				a.w        != b.w        ||
				a.h        != b.h        ||
				a.zoomable != b.zoomable ||
				a.len      != b.len      ||
				memcmp(a.data, b.data, sizeof(*a.data) * a.len) != 0;
		}
		// unique image here
		if(  do_register_image  ) {
			if(!same) {
				images_adlers.put(adler,desc);	// still with imageid == IMG_EMPTY!
			}
			// register image adds this image to the internal array maintained by simgraph??.cc
			register_image(desc);
		}
		else {
			// no need to load doubles ...
			delete desc;
			desc = same;
		}
	}

	return desc;
}
