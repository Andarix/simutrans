#ifndef XREF_WRITER_H
#define XREF_WRITER_H

#include <stdio.h>
#include "obj_writer.h"
#include "../objversion.h"


class obj_node_t;


class xref_writer_t : public obj_writer_t {
	private:
		static xref_writer_t the_instance;

		xref_writer_t() { register_writer(false); }

	public:
		static xref_writer_t* instance() { return &the_instance; }

		virtual obj_type get_type() const { return obj_xref; }
		virtual const char* get_type_name() const { return "xref"; }

		void write_obj(FILE* fp, obj_node_t& parent, obj_type type, const char* text, bool fatal);
		void dump_node(FILE* infp, const obj_node_info_t& node);
};

#endif
