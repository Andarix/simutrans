#include <stdio.h>

#include "../../simdings.h"
#include "../../simdebug.h"
#include "../../dings/groundobj.h"

#include "../groundobj_besch.h"
#include "../obj_node_info.h"
#include "groundobj_reader.h"


void groundobj_reader_t::register_obj(obj_besch_t *&data)
{
	groundobj_besch_t *besch = static_cast<groundobj_besch_t *>(data);
	groundobj_t::register_besch(besch);
}


bool groundobj_reader_t::successfully_loaded() const
{
	return groundobj_t::alles_geladen();
}


obj_besch_t * groundobj_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	groundobj_besch_t *besch = new groundobj_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;
	if(version == 1) {
		besch->allowed_climates = (climate_bits)decode_uint16(p);
		besch->distribution_weight = decode_uint16(p);
		besch->number_of_seasons = decode_uint8(p);
		besch->moving = (bool)decode_uint8(p);
		besch->waytype = (waytype_t)decode_uint16(p);
		besch->cost_removal = decode_sint32(p);
	} else {
		// old node, version 0, never existed
		dbg->fatal( "groundobj_reader_t::read_node()", "version %i not supported!", version );
	}
	DBG_DEBUG("groundobj_reader_t::read_node()", "climates=$%X, seasons %i, weight=%i, moves=%i, ways=%i, cost=%f",besch->allowed_climates,besch->number_of_seasons,besch->distribution_weight, besch->moving, besch->waytype, besch->cost_removal );

	return besch;
}
