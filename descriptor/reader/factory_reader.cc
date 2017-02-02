#include <stdio.h>
#include "../../simfab.h"
#include "../../bauer/fabrikbauer.h"
#include "../../simdebug.h"
#include "../obj_node_info.h"
#include "../factory_desc.h"
#include "../xref_desc.h"
#include "../../network/pakset_info.h"

#include "factory_reader.h"


// Knightly : determine the combined probability of 256 rounds of chances
uint16 rescale_probability(const uint16 p)
{
	if(  p  ) {
		sint64 pp = ( (sint64)p << 30 ) / 10000LL;
		sint64 qq = ( 1LL << 30 ) - pp;
		uint16 ss = 256u;
		while(  (ss >>= 1)  ) {
			pp += (pp * qq) >> 30;
			qq = (qq * qq) >> 30;
		}
		return (uint16)(((pp * 10000LL) + (1LL << 29)) >> 30);
	}
	return p;
}


obj_desc_t *factory_field_class_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	field_class_desc_t *desc = new field_class_desc_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	uint16 v = decode_uint16(p);
	if(  v==0x8001  ) {
		// Knightly : field class specific data
		desc->snow_image = decode_uint8(p);
		desc->production_per_field = decode_uint16(p);
		desc->storage_capacity = decode_uint16(p);
		desc->spawn_weight = decode_uint16(p);

		DBG_DEBUG("factory_field_class_reader_t::read_node()", "has_snow %i, production %i, capacity %i, spawn_weight %i", desc->snow_image, desc->production_per_field, desc->storage_capacity, desc->spawn_weight);
	}
	else {
		dbg->fatal("factory_field_class_reader_t::read_node()","unknown version %i", v&0x00ff );
	}

	return desc;
}


obj_desc_t *factory_field_group_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	field_group_desc_t *desc = new field_group_desc_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	uint16 v = decode_uint16(p);
	if(  v==0x8003  ) {
		desc->probability = rescale_probability( decode_uint16(p) );
		desc->max_fields = decode_uint16(p);
		desc->min_fields = decode_uint16(p);
		desc->start_fields = decode_uint16(p);
		desc->field_classes = decode_uint16(p);

		DBG_DEBUG("factory_field_group_reader_t::read_node()", "probability %i, fields: max %i, min %i, field classes: %i", desc->probability, desc->max_fields, desc->min_fields, desc->field_classes);
	}
	else if(  v==0x8002  ) {
		// Knightly : this version only store shared, common data
		desc->probability = rescale_probability( decode_uint16(p) );
		desc->max_fields = decode_uint16(p);
		desc->min_fields = decode_uint16(p);
		desc->field_classes = decode_uint16(p);

		DBG_DEBUG("factory_field_group_reader_t::read_node()", "probability %i, fields: max %i, min %i, start %i, field classes: %i", desc->probability, desc->max_fields, desc->min_fields, desc->start_fields, desc->field_classes);
	}
	else if(  v==0x8001  ) {
		/* Knightly :
		 *   leave shared, common data in field besch
		 *   field class specific data goes to field class besch
		 */
		field_class_desc_t *const field_class_desc = new field_class_desc_t();

		field_class_desc->snow_image = decode_uint8(p);
		desc->probability = rescale_probability( decode_uint16(p) );
		field_class_desc->production_per_field = decode_uint16(p);
		desc->max_fields = decode_uint16(p);
		desc->min_fields = decode_uint16(p);
		desc->field_classes = 1;
		field_class_desc->storage_capacity = 0;
		field_class_desc->spawn_weight = 1000;

		/* Knightly :
		 *   store it in a static variable for further processing
		 *   later in factory_field_reader_t::register_obj()
		 */
		incomplete_field_class_desc = field_class_desc;

		DBG_DEBUG("factory_field_group_reader_t::read_node()", "has_snow %i, probability %i, fields: max %i, min %i, production %i", field_class_desc->snow_image, desc->probability, desc->max_fields, desc->min_fields, field_class_desc->production_per_field);
	}
	else {
		dbg->fatal("factory_field_group_reader_t::read_node()","unknown version %i", v&0x00ff );
	}

	return desc;
}


void factory_field_group_reader_t::register_obj(obj_desc_t *&data)
{
	field_group_desc_t *const desc = static_cast<field_group_desc_t *>(data);

	// Knightly : check if we need to continue with the construction of field class besch
	if (field_class_desc_t *const field_class_desc = incomplete_field_class_desc) {
		// we *must* transfer the obj_desc_t array and not just the desc object itself
		// as xref reader has already logged the address of the array element for xref resolution
		field_class_desc->children  = desc->children;
		desc->children              = new obj_desc_t*[1];
		desc->children[0]           = field_class_desc;
		incomplete_field_class_desc = NULL;
	}
}



obj_desc_t *factory_smoke_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	ALLOCA(char, besch_buf, node.size);

	smoke_desc_t *desc = new smoke_desc_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	sint16 x = decode_sint16(p);
	sint16 y = decode_sint16(p);
	desc->pos_off = koord( x, y );

	x = decode_sint16(p);
	y = decode_sint16(p);

	desc->xy_off = koord( x, y );
	/*smoke speed*/ decode_sint16(p);

	DBG_DEBUG("factory_smoke_reader_t::read_node()","(size %i)",node.size);

	return desc;
}


obj_desc_t *factory_supplier_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_product_reader_t::read_node()", "called");
	ALLOCA(char, besch_buf, node.size);

	factory_supplier_desc_t *desc = new factory_supplier_desc_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);
	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 1) {
		// Versioned node, version 1

		// not there yet ...
	}
	else {
		// old node, version 0
		desc->capacity = v;
		desc->supplier_count = decode_uint16(p);
		desc->consumption = decode_uint16(p);
	}
	DBG_DEBUG("factory_product_reader_t::read_node()",  "capacity=%d count=%d, consumption=%d", version, desc->capacity, desc->supplier_count,desc->consumption);

	return desc;
}


obj_desc_t *factory_product_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_product_reader_t::read_node()", "called");
	ALLOCA(char, besch_buf, node.size);

	factory_product_desc_t *desc = new factory_product_desc_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.
	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	if(version == 1) {
		// Versioned node, version 1
		desc->capacity = decode_uint16(p);
		desc->factor = decode_uint16(p);
	}
	else {
		// old node, version 0
		decode_uint16(p);
		desc->capacity = v;
		desc->factor = 256;
	}

	DBG_DEBUG("factory_product_reader_t::read_node()", "version=%d capacity=%d factor=%x", version, desc->capacity, desc->factor);
	return desc;
}


obj_desc_t *factory_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	// DBG_DEBUG("factory_reader_t::read_node()", "called");
	ALLOCA(char, besch_buf, node.size);

	factory_desc_t *desc = new factory_desc_t();

	// Hajo: Read data
	fread(besch_buf, node.size, 1, fp);

	char * p = besch_buf;

	// Hajo: old versions of PAK files have no version stamp.
	// But we know, the higher most bit was always cleared.

	const uint16 v = decode_uint16(p);
	const int version = v & 0x8000 ? v & 0x7FFF : 0;

	typedef factory_desc_t::site_t site_t;
	if(version == 3) {
		// Versioned node, version 3
		desc->placement = (site_t)decode_uint16(p);
		desc->productivity = decode_uint16(p);
		desc->range = decode_uint16(p);
		desc->chance = decode_uint16(p);
		desc->color = decode_uint8(p);
		desc->fields = decode_uint8(p);
		desc->supplier_count = decode_uint16(p);
		desc->product_count = decode_uint16(p);
		desc->pax_level = decode_uint16(p);
		desc->expand_probability = rescale_probability( decode_uint16(p) );
		desc->expand_minimum = decode_uint16(p);
		desc->expand_range = decode_uint16(p);
		desc->expand_times = decode_uint16(p);
		desc->electric_boost = decode_uint16(p);
		desc->pax_boost = decode_uint16(p);
		desc->mail_boost = decode_uint16(p);
		desc->electric_amount = decode_uint16(p);
		desc->pax_demand = decode_uint16(p);
		desc->mail_demand = decode_uint16(p);
		DBG_DEBUG("factory_reader_t::read_node()","version=3, platz=%i, supplier_count=%i, pax=%i", desc->placement, desc->supplier_count, desc->pax_level );
	}
	else if(version == 2) {
		// Versioned node, version 2
		desc->placement = (site_t)decode_uint16(p);
		desc->productivity = decode_uint16(p);
		desc->range = decode_uint16(p);
		desc->chance = decode_uint16(p);
		desc->color = decode_uint8(p);
		desc->fields = decode_uint8(p);
		desc->supplier_count = decode_uint16(p);
		desc->product_count = decode_uint16(p);
		desc->pax_level = decode_uint16(p);
		desc->expand_probability = 0;
		desc->expand_minimum = 0;
		desc->expand_range = 0;
		desc->expand_times = 0;
		desc->electric_boost = 256;
		desc->pax_boost = 0;
		desc->mail_boost = 0;
		desc->electric_amount = 65535;
		desc->pax_demand = 65535;
		desc->mail_demand = 65535;
		DBG_DEBUG("factory_reader_t::read_node()","version=2, platz=%i, supplier_count=%i, pax=%i", desc->placement, desc->supplier_count, desc->pax_level );
	}
	else if(version == 1) {
		// Versioned node, version 1
		desc->placement = (site_t)decode_uint16(p);
		desc->productivity = decode_uint16(p);
		desc->range = decode_uint16(p);
		desc->chance = decode_uint16(p);
		desc->color = (uint8)decode_uint16(p);
		desc->supplier_count = decode_uint16(p);
		desc->product_count = decode_uint16(p);
		desc->pax_level = decode_uint16(p);
		desc->fields = 0;
		desc->expand_probability = 0;
		desc->expand_minimum = 0;
		desc->expand_range = 0;
		desc->expand_times = 0;
		desc->electric_boost = 256;
		desc->pax_boost = 0;
		desc->mail_boost = 0;
		desc->electric_amount = 65535;
		desc->pax_demand = 65535;
		desc->mail_demand = 65535;
		DBG_DEBUG("factory_reader_t::read_node()","version=1, platz=%i, supplier_count=%i, pax=%i", desc->placement, desc->supplier_count, desc->pax_level);
	}
	else {
		// old node, version 0, without pax_level
		DBG_DEBUG("factory_reader_t::read_node()","version=0");
		desc->placement = (site_t)v;
		decode_uint16(p);	// alsways zero
		desc->productivity = decode_uint16(p)|0x8000;
		desc->range = decode_uint16(p);
		desc->chance = decode_uint16(p);
		desc->color = (uint8)decode_uint16(p);
		desc->supplier_count = decode_uint16(p);
		desc->product_count = decode_uint16(p);
		desc->pax_level = 12;
		desc->fields = 0;
		desc->expand_probability = 0;
		desc->expand_minimum = 0;
		desc->expand_range = 0;
		desc->expand_times = 0;
		desc->electric_boost = 256;
		desc->pax_boost = 0;
		desc->mail_boost = 0;
		desc->electric_amount = 65535;
		desc->pax_demand = 65535;
		desc->mail_demand = 65535;
	}

	return desc;
}


void factory_reader_t::register_obj(obj_desc_t *&data)
{
	factory_desc_t* desc = static_cast<factory_desc_t*>(data);
	size_t fab_name_len = strlen( desc->get_name() );
	desc->electricity_producer = ( fab_name_len>11   &&  (strcmp(desc->get_name()+fab_name_len-9, "kraftwerk")==0  ||  strcmp(desc->get_name()+fab_name_len-11, "Power Plant")==0) );
	factory_builder_t::register_desc(desc);
}


bool factory_reader_t::successfully_loaded() const
{
	return factory_builder_t::successfully_loaded();
}
