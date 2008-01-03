/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 */

#include <stdio.h>

#include "../../vehicle/simpeople.h"
#include "../fussgaenger_besch.h"
#include "../obj_node_info.h"

#include "pedestrian_reader.h"

/*
 *  Autor:
 *      Volker Meyer
 */
void pedestrian_reader_t::register_obj(obj_besch_t *&data)
{
    fussgaenger_besch_t *besch = static_cast<fussgaenger_besch_t  *>(data);

    fussgaenger_t::register_besch(besch);

    obj_for_xref(get_type(), besch->gib_name(), data);
//    printf("...Fussgaenger %s geladen\n", besch->gib_name());
}



bool pedestrian_reader_t::successfully_loaded() const
{
    return fussgaenger_t::laden_erfolgreich();
}



/**
 * Read a goods info node. Does version check and
 * compatibility transformations.
 * @author Hj. Malthaner
 */
obj_besch_t * pedestrian_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

  fussgaenger_besch_t *besch = new fussgaenger_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

  // Hajo: Read data
  fread(besch_buf, node.size, 1, fp);
  char * p = besch_buf;

  // Hajo: old versions of PAK files have no version stamp.
  // But we know, the higher most bit was always cleared.

  const uint16 v = decode_uint16(p);
  const int version = v & 0x8000 ? v & 0x7FFF : 0;

  if(version == 0) {
    // old, nonversion node
    besch->gewichtung = v;
  }
  DBG_DEBUG("pedestrian_reader_t::read_node()","version=%i, gewichtung",version,besch->gewichtung);
  return besch;
}
