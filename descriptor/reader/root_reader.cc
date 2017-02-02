#include "../obj_besch.h"
#include "root_reader.h"


void root_reader_t::register_obj(obj_desc_t *&data)
{
    delete data;
    data = NULL;
}


obj_desc_t* root_reader_t::read_node(FILE*, obj_node_info_t& info)
{
	return obj_reader_t::read_node<obj_desc_t>(info);
}
