#include <stdio.h>

#include "../../simobj.h"
#include "../../simdebug.h"
#include "../../obj/baum.h"

#include "../tree_desc.h"
#include "../obj_node_info.h"
#include "tree_reader.h"
#include "../../network/pakset_info.h"


void tree_reader_t::register_obj(obj_desc_t *&data)
{
    tree_desc_t *desc = static_cast<tree_desc_t *>(data);

    baum_t::register_desc(desc);
//    printf("...Tree %s loaded\n", desc->get_name());
	checksum_t *chk = new checksum_t();
	desc->calc_checksum(chk);
	pakset_info_t::append(desc->get_name(), get_type(), chk);
}


bool tree_reader_t::successfully_loaded() const
{
    return baum_t::successfully_loaded();
}


obj_desc_t * tree_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, desc_buf, node.size);

	tree_desc_t *desc = new tree_desc_t();

	// Hajo: Read data
	fread(desc_buf, node.size, 1, fp);

	char * p = desc_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the highest bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;
	if(version==2) {
		// Versioned node, version 2
		desc->allowed_climates = (climate_bits)decode_uint16(p);
		desc->distribution_weight = (uint8)decode_uint8(p);
		desc->number_of_seasons = (uint8)decode_uint8(p);
	} else if(version == 1) {
		// Versioned node, version 1
		desc->allowed_climates = all_but_arctic_climate;
		desc->number_of_seasons = 0;
		decode_uint8(p);	// ignore hoehenlage
		desc->distribution_weight = (uint8)decode_uint8(p);
	} else {
		// old node, version 0
		desc->number_of_seasons = 0;
		desc->allowed_climates = all_but_arctic_climate;
		desc->distribution_weight = 3;
	}
	DBG_DEBUG("tree_reader_t::read_node()",
		"version=%i, climates=$%X, seasons=%i, chance=%i (node.size=%i)",
		version,
		desc->allowed_climates,
		desc->number_of_seasons,
		desc->distribution_weight,
		node.size);

	return desc;
}
