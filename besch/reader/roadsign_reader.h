/*
 *
 *  roadsign_reader.h
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#ifndef __ROADSIGN_READER_H
#define __ROADSIGN_READER_H

/*
 *  includes
 */
#include "obj_reader.h"


/*
 *  class:
 *      citycar_reader_t()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 */
class roadsign_reader_t : public obj_reader_t {
    static roadsign_reader_t the_instance;

    roadsign_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

public:
    static roadsign_reader_t*instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_roadsign; }
    virtual const char *get_type_name() const { return "roadsign"; }
    virtual obj_besch_t *read_node(FILE *fp, obj_node_info_t &node);
};

#endif // __ROADSIGN_READER_H
