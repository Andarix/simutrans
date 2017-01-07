#include <stdio.h>

#include "../../obj/roadsign.h"
#include "../../simunits.h"	// for kmh to speed conversion
#include "../roadsign_besch.h"
#include "../intro_dates.h"

#include "roadsign_reader.h"
#include "../obj_node_info.h"

#include "../../simdebug.h"
#include "../../network/pakset_info.h"


void roadsign_reader_t::register_obj(obj_desc_t *&data)
{
    roadsign_besch_t *desc = static_cast<roadsign_besch_t *>(data);

    roadsign_t::register_desc(desc);

	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), chk);
}


bool roadsign_reader_t::successfully_loaded() const
{
    return roadsign_t::successfully_loaded();
}


obj_desc_t * roadsign_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	roadsign_besch_t *desc = new roadsign_besch_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version==4) {
		// Versioned node, version 3
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->cost = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = decode_sint8(p);
		desc->wt = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
	}
	else if(version==3) {
		// Versioned node, version 3
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->cost = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->wt = decode_uint8(p);
		desc->intro_date = decode_uint16(p);
		desc->obsolete_date = decode_uint16(p);
	}
	else if(version==2) {
		// Versioned node, version 2
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->cost = decode_uint32(p);
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->obsolete_date = DEFAULT_RETIRE_DATE*12;
		desc->wt = road_wt;
	}
	else if(version==1) {
		// Versioned node, version 1
		desc->min_speed = kmh_to_speed(decode_uint16(p));
		desc->cost = 50000;
		desc->flags = decode_uint8(p);
		desc->offset_left = 14;
		desc->intro_date = DEFAULT_INTRO_DATE*12;
		desc->obsolete_date = DEFAULT_RETIRE_DATE*12;
		desc->wt = road_wt;
	}
	else {
		dbg->fatal("roadsign_reader_t::read_node()","version 0 not supported. File corrupt?");
	}

	if(  version<=3  &&  (  desc->is_choose_sign() ||  desc->is_private_way()  )  &&  desc->get_waytype() == road_wt  ) {
		// do not shift these signs to the left for compatibility
		desc->offset_left = 0;
	}

	DBG_DEBUG("roadsign_reader_t::read_node()","min_speed=%i, cost=%i, flags=%x, wt=%i, intro=%i%i, retire=%i,%i",desc->min_speed,desc->cost/100,desc->flags,desc->wt,desc->intro_date%12+1,desc->intro_date/12,desc->obsolete_date%12+1,desc->obsolete_date/12 );
	return desc;
}
