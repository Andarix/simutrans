#ifndef BRIDGE_WRITER_H
#define BRIDGE_WRITER_H

#include <string>
#include "obj_writer.h"
#include "../objversion.h"


class bridge_writer_t : public obj_writer_t {
	private:
		static bridge_writer_t the_instance;

		bridge_writer_t() { register_writer(true); }

	protected:
		virtual std::string get_node_name(FILE* fp) const { return name_from_next_node(fp); }

	public:
		/**
		 * Writes bridge node data to file
		 * @author Hj. Malthaner
		 */
		virtual void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj);

		virtual obj_type get_type() const { return obj_bridge; }
		virtual const char* get_type_name() const { return "bridge"; }
};

#endif
