#include <stdio.h>
#include "../../simdebug.h"

#include "../../bauer/brueckenbauer.h"
#include "../bruecke_besch.h"
#include "../intro_dates.h"

#include "bridge_reader.h"
#include "../obj_node_info.h"


void bridge_reader_t::register_obj(obj_besch_t *&data)
{
	bruecke_besch_t *besch = static_cast<bruecke_besch_t *>(data);

	brueckenbauer_t::register_besch(besch);
}


bool bridge_reader_t::successfully_loaded() const
{
	return brueckenbauer_t::laden_erfolgreich();
}


obj_besch_t * bridge_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("bridge_reader_t::read_node()", "called");

	ALLOCA(char, besch_buf, node.size);

	bruecke_besch_t *besch = new bruecke_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	// some defaults
	besch->maintenance = 800;
	besch->pillars_every = 0;
	besch->max_length = 0;
	besch->intro_date = DEFAULT_INTRO_DATE*12;
	besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
	besch->number_seasons = 0;

	if(version == 1) {
		// Versioned node, version 1

		besch->wegtyp = (uint8)decode_uint16(p);
		besch->topspeed = decode_uint16(p);
		besch->preis = decode_uint32(p);

	} else if (version == 2) {

		// Versioned node, version 2

		besch->topspeed = decode_uint16(p);
		besch->preis = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wegtyp = decode_uint8(p);

	} else if (version == 3) {

		// Versioned node, version 3
		// pillars added

		besch->topspeed = decode_uint16(p);
		besch->preis = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wegtyp = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = 0;

	} else if (version == 4) {

		// Versioned node, version 3
		// pillars added

		besch->topspeed = decode_uint16(p);
		besch->preis = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wegtyp = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = decode_uint8(p);

	} else if (version == 5) {

		// Versioned node, version 5
		// timeline

		besch->topspeed = decode_uint16(p);
		besch->preis = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wegtyp = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = decode_uint8(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);

	} else if (version == 6) {

		// Versioned node, version 6
		// snow

		besch->topspeed = decode_uint16(p);
		besch->preis = decode_uint32(p);
		besch->maintenance = decode_uint32(p);
		besch->wegtyp = decode_uint8(p);
		besch->pillars_every = decode_uint8(p);
		besch->max_length = decode_uint8(p);
		besch->intro_date = decode_uint16(p);
		besch->obsolete_date = decode_uint16(p);
		besch->number_seasons = decode_uint8(p);

	} else {
		// old node, version 0

		besch->wegtyp = (uint8)v;
		decode_uint16(p);                    // Menupos, no more used
		besch->preis = decode_uint32(p);
		besch->topspeed = 999;               // Safe default ...
	}

  DBG_DEBUG("bridge_reader_t::read_node()",
	"version=%d waytype=%d price=%d topspeed=%d,pillars=%i,max_length=%i",
	version, besch->wegtyp, besch->preis, besch->topspeed,besch->pillars_every,besch->max_length);

  return besch;
}
