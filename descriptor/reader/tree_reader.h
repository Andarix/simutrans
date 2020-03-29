/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_TREE_READER_H
#define DESCRIPTOR_READER_TREE_READER_H


#include "obj_reader.h"


class tree_reader_t : public obj_reader_t {
	static tree_reader_t the_instance;

	tree_reader_t() { register_reader(); }
protected:
	bool successfully_loaded() const OVERRIDE;
	void register_obj(obj_desc_t*&) OVERRIDE;
public:
	static tree_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_tree; }
	char const* get_type_name() const OVERRIDE { return "tree"; }
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;
};

#endif
