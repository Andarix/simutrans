#include "api_obj_desc_base.h"

#include "../../bauer/brueckenbauer.h"
#include "../../bauer/hausbauer.h"
#include "../../bauer/tunnelbauer.h"
#include "../../bauer/vehikelbauer.h"
#include "../../bauer/warenbauer.h"
#include "../../bauer/wegbauer.h"
#include "../../besch/bruecke_besch.h"
#include "../../besch/tunnel_besch.h"
#include "../../besch/vehikel_besch.h"
#include "../../besch/ware_besch.h"
#include "../../besch/weg_besch.h"
#include "../../obj/baum.h"
#include "../../squirrel/sq_extensions.h"

using namespace script_api;

static const weg_besch_t *my_get_desc(const char *name)
{
	return wegbauer_t::get_desc(name);
}

implement_besch_param(baum_besch_t, "tree_desc_x", &baum_t::find_tree);
implement_besch_param(haus_besch_t, "building_desc_x", &hausbauer_t::get_desc);
implement_besch_param(ware_besch_t, "good_desc_x", (const ware_besch_t* (*)(const char*))(&warenbauer_t::get_info) );
implement_besch_param(weg_besch_t, "way_desc_x", &my_get_desc);
implement_besch_param(vehikel_besch_t, "vehicle_desc_x", &vehikelbauer_t::get_info);
implement_besch_param(tunnel_besch_t, "tunnel_desc_x", &tunnelbauer_t::get_desc);
implement_besch_param(bruecke_besch_t, "bridge_desc_x", &brueckenbauer_t::get_desc);

/**
 * Macro to get the implementation of get method based on unique tag.
 */
#define implement_class_with_tag(class) \
	/* tag: points to itself */ \
	static void *class ## _tag = &class ## _tag; \
	\
	const class* param<const class*>::get(HSQUIRRELVM vm, SQInteger index) \
	{ \
		return (const class*)get_instanceup(vm, index, tag(), squirrel_type()); \
	} \
	\
	void* param<const class*>::tag() \
	{ \
		return class ## _tag; \
	}

// use the macro to obtain the interface of some abstract classes
implement_class_with_tag(obj_besch_std_name_t);
implement_class_with_tag(obj_besch_timelined_t);
implement_class_with_tag(obj_besch_transport_related_t);

SQInteger param<const haus_tile_besch_t*>::push(HSQUIRRELVM vm, const haus_tile_besch_t* b)
{
	return param<const haus_besch_t*>::push(vm, b ? b->get_desc() : NULL);
}
