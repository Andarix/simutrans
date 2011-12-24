#ifndef __SOUND_READER_H
#define __SOUND_READER_H

#include "obj_reader.h"


class sound_reader_t : public obj_reader_t {
	static sound_reader_t the_instance;

	sound_reader_t() { register_reader(); }
protected:
	void register_obj(obj_besch_t*&) OVERRIDE;

public:
	static sound_reader_t*instance() { return &the_instance; }

	obj_besch_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_sound; }
	char const* get_type_name() const OVERRIDE { return "sound"; }
};

#endif
