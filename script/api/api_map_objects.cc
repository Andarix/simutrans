#include "api.h"

/** @file api_map_objects.cc exports all map-objects. */

#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "../api_class.h"
#include "../api_function.h"

#include "../../simobj.h"
#include "../../simdepot.h"
#include "../../simworld.h"
#include "../../boden/grund.h"
#include "../../dataobj/scenario.h"
#include "../../obj/baum.h"
#include "../../obj/gebaeude.h"
#include "../../obj/label.h"
#include "../../obj/roadsign.h"
#include "../../obj/signal.h"
#include "../../player/simplay.h"

// for depot tools
#include "../../simconvoi.h"
#include "../../simmenu.h"
#include "../../descriptor/vehicle_desc.h"


using namespace script_api;

// use pointers to obj_t_tag[ <type> ] to tag the obj_t classes
static uint8 obj_t_tag[256];

/*
 * template struct to bind obj_t::typ to obj_t classes
 */
template<class D> struct bind_code;

/*
 * Class to templatify access of obj_t objects
 */
template<class D> struct access_objs {

	/*
	 * Access object: check whether object is still on this tile.
	 */
	static D* get_by_pos(HSQUIRRELVM vm, SQInteger index)
	{
		SQUserPointer tag = obj_t_tag + bind_code<D>::objtype;
		SQUserPointer p = NULL;
		if (SQ_SUCCEEDED(sq_getinstanceup(vm, index, &p, tag))  &&  p) {
			D *obj = static_cast<D*>(p);
			koord3d pos = param<koord3d>::get(vm, index);
			grund_t *gr = welt->lookup(pos);
			if (gr  &&  gr->obj_ist_da(obj)) {
				return obj;
			}
			else {
				// object or tile disappeared: clear userpointer
				sq_setinstanceup(vm, index, NULL);
			}
		}
		return NULL;
	}

	/*
	 * Create instance: call constructor with coordinates.
	 */
	static SQInteger push_with_pos(HSQUIRRELVM vm, D* const& obj)
	{
		if (obj == NULL) {
			sq_pushnull(vm);
			return 1;
		}
		koord pos = obj->get_pos().get_2d();
		coordinate_transform_t::koord_w2sq(pos);
		sint16 x = pos.x;
		sint16 y = pos.y;
		sint8  z = obj->get_pos().z;
		if (bind_code<D>::objtype == obj_t::obj) {
			// generic object, object type as fourth parameter
			if (!SQ_SUCCEEDED(push_instance(vm, script_api::param<D*>::squirrel_type(), x, y, z, obj->get_typ()))) {
				return SQ_ERROR;
			}
		}
		else{
			assert(bind_code<D>::objtype == obj->get_typ());
			// specific object with its own constructor, type already preset as default parameter
			if (!SQ_SUCCEEDED(push_instance(vm, script_api::param<D*>::squirrel_type(), x, y, z))) {
				return SQ_ERROR;
			}
		}
		sq_setinstanceup(vm, -1, obj);
		return 1;
	}

};


SQInteger exp_obj_pos_constructor(HSQUIRRELVM vm) // parameters: sint16 x, sint16 y, sint8 z, obj_t::typ type
{
	// get coordinates
	sint16 x = param<sint16>::get(vm, 2);
	sint16 y = param<sint16>::get(vm, 3);
	sint8  z = param<sint16>::get(vm, 4);
	// set coordinates
	set_slot(vm, "x", x, 1);
	set_slot(vm, "y", y, 1);
	set_slot(vm, "z", z, 1);
	koord pos(x,y);
	coordinate_transform_t::koord_sq2w(pos);
	// find object and set instance up
	if (grund_t *gr = welt->lookup(koord3d(pos, z))) {
		obj_t::typ type = (obj_t::typ)param<uint8>::get(vm, 5);
		obj_t *obj = NULL;
		if (type == obj_t::roadsign  ||  type == obj_t::signal) {
			// special treatment of signals/roadsigns
			obj = gr->suche_obj(obj_t::roadsign);
			if (obj == NULL) {
				obj = gr->suche_obj(obj_t::signal);
			}
		}
		else if (type != obj_t::old_airdepot) { // special treatment of depots
			obj = gr->suche_obj(type);
		}
		else {
			obj = gr->get_depot();
		}
		if (obj) {
			sq_setinstanceup(vm, 1, obj);
			return SQ_OK;
		}
	}
	return sq_raise_error(vm, "No object of requested type on tile (or no tile at this position)");
}

template<> struct bind_code<obj_t> { static const uint8 objtype = obj_t::obj; };

// macro to implement get and push for obj_t's with position
#define getpush_obj_pos(D, type) \
	D* script_api::param<D*>::get(HSQUIRRELVM vm, SQInteger index) \
	{ \
		return access_objs<D>::get_by_pos(vm, index); \
	} \
	SQInteger script_api::param<D*>::push(HSQUIRRELVM vm, D* const& obj) \
	{ \
		return access_objs<D>::push_with_pos(vm, obj); \
	} \
	template<> struct bind_code<D> { static const uint8 objtype = type; };

// implementation of get and push by macros
getpush_obj_pos(baum_t, obj_t::baum);
getpush_obj_pos(gebaeude_t, obj_t::gebaeude);
getpush_obj_pos(label_t, obj_t::label);
getpush_obj_pos(weg_t, obj_t::way);

namespace script_api {
	// each depot has its own class
	declare_specialized_param(depot_t*, "t|x|y", "depot_x");
	declare_specialized_param(airdepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(narrowgaugedepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(bahndepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(strassendepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(schiffdepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(monoraildepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(tramdepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(maglevdepot_t*, "t|x|y", "depot_x");

	// map roadsign_t and signal_t to the same class
	declare_specialized_param(roadsign_t*, "t|x|y", "sign_x");
	declare_specialized_param(signal_t*, "t|x|y", "sign_x");

};
// base depot class, use old_airdepot as identifier here
getpush_obj_pos(depot_t, obj_t::old_airdepot);
// now all the derived classes
getpush_obj_pos(airdepot_t, obj_t::airdepot);
getpush_obj_pos(narrowgaugedepot_t, obj_t::narrowgaugedepot);
getpush_obj_pos(bahndepot_t, obj_t::bahndepot);
getpush_obj_pos(strassendepot_t, obj_t::strassendepot);
getpush_obj_pos(schiffdepot_t, obj_t::schiffdepot);
getpush_obj_pos(monoraildepot_t, obj_t::monoraildepot);
getpush_obj_pos(tramdepot_t, obj_t::tramdepot);
getpush_obj_pos(maglevdepot_t, obj_t::maglevdepot);
// roadsigns/signals
getpush_obj_pos(roadsign_t, obj_t::roadsign);
getpush_obj_pos(signal_t, obj_t::signal);

#define case_resolve_obj(D) \
	case bind_code<D>::objtype: \
		return script_api::param<D*>::push(vm, (D*)obj);

// we have to resolve instances of derived classes here...
SQInteger script_api::param<obj_t*>::push(HSQUIRRELVM vm, obj_t* const& obj)
{
	if (obj == NULL) {
		sq_pushnull(vm);
		return 1;
	}
	obj_t::typ type = obj->get_typ();
	switch(type) {
		case_resolve_obj(baum_t);
		case_resolve_obj(gebaeude_t);
		case_resolve_obj(label_t);
		case_resolve_obj(weg_t);
		case_resolve_obj(roadsign_t);
		case_resolve_obj(signal_t);

		case_resolve_obj(airdepot_t);
		case_resolve_obj(narrowgaugedepot_t);
		case_resolve_obj(bahndepot_t);
		case_resolve_obj(strassendepot_t);
		case_resolve_obj(schiffdepot_t);
		case_resolve_obj(monoraildepot_t);
		case_resolve_obj(tramdepot_t);
		case_resolve_obj(maglevdepot_t);

		default:
			return access_objs<obj_t>::push_with_pos(vm, obj);
	}
}

obj_t* script_api::param<obj_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	return access_objs<obj_t>::get_by_pos(vm, index);
}

// return way ribis, have to implement a wrapper, to correctly rotate ribi
static SQInteger get_way_ribi(HSQUIRRELVM vm)
{
	weg_t *w = param<weg_t*>::get(vm, 1);
	bool masked = param<bool>::get(vm, 2);

	ribi_t::ribi ribi = w ? (masked ? w->get_ribi() : w->get_ribi_unmasked() ) : 0;

	return param<my_ribi_t>::push(vm, ribi);
}

// create class
template<class D>
void begin_obj_class(HSQUIRRELVM vm, const char* name, const char* base = NULL)
{
	SQInteger res = create_class(vm, name, base);
	if(  !SQ_SUCCEEDED(res)  ) {
		// base class not found: maybe script_base.nut is not up-to-date
		dbg->error( "begin_obj_class()", "Create class failed for %s. Base class %s missing. Please update simutrans (or just script/script_base.nut)!", name, base );
		sq_raise_error(vm, "Create class failed for %s. Base class %s missing. Please update simutrans (or just script/script_base.nut)!", name, base);
	}
	uint8 objtype = bind_code<D>::objtype;
	// store typetag to identify pointers
	sq_settypetag(vm, -1, obj_t_tag + objtype);
	// export constructor
	register_function_fv(vm, exp_obj_pos_constructor, "constructor", 4, "xiiii", freevariable<uint8>(objtype));
	// now functions can be registered
}

// mark objects
void_t mark_object(obj_t* obj)
{
	obj->set_flag(obj_t::highlight);
	obj->set_flag(obj_t::dirty);
	return void_t();
}
void_t unmark_object(obj_t* obj)
{
	obj->clear_flag(obj_t::highlight);
	obj->set_flag(obj_t::dirty);
	return void_t();
}
bool object_is_marked(obj_t* obj)
{
	return obj->get_flag(obj_t::highlight);
}

// markers / labels
call_tool_work create_marker(koord pos, player_t* player, const char* text)
{
	if (text == NULL) {
		return "Cannot create label with text == null";
	}
	return call_tool_work(TOOL_MARKER | GENERAL_TOOL, text, 0, player, koord3d(pos, 0));
}

call_tool_init label_set_text(label_t *l, const char* text)
{
	return command_rename(l->get_owner(), 'm', l->get_pos(), text);
}

const char* label_get_text(label_t* l)
{
	if (l) {
		if (grund_t *gr = welt->lookup(l->get_pos())) {
			return gr->get_text();
		}
	}
	return NULL;
}

// roadsign
bool roadsign_can_pass(const roadsign_t* rs, player_t* player)
{
	return player  &&  rs->get_desc()->is_private_way()  ?  (rs->get_player_mask() & (1<<player->get_player_nr()))!=0 : true;
}

// depot
call_tool_init depot_append_vehicle(depot_t *depot, player_t *player, convoihandle_t cnv, const vehicle_desc_t *desc)
{
	if (desc == NULL) {
		return "Invalid vehicle_desc_x provided";
	}
	// see depot_frame_t::image_from_storage_list: tool = 'a'
	// see depot_t::call_depot_tool for command string composition
	cbuffer_t buf;
	buf.printf( "%c,%s,%hu,%s", 'a', depot->get_pos().get_str(), cnv.get_id(), desc->get_name());

	return call_tool_init(TOOL_CHANGE_DEPOT | SIMPLE_TOOL, buf, 0, player);
}

call_tool_init depot_start_convoy(depot_t *depot, player_t *player, convoihandle_t cnv)
{
	// see depot_frame_t::action_triggered: tool = 'b'
	// see depot_t::call_depot_tool for command string composition
	cbuffer_t buf;
	if (cnv.is_bound()) {
		buf.printf( "%c,%s,%hu", 'b', depot->get_pos().get_str(), cnv->self.get_id());
	}
	else {
		buf.printf( "%c,%s,%hu", 'B', depot->get_pos().get_str(), 0);
	}

	return call_tool_init(TOOL_CHANGE_DEPOT | SIMPLE_TOOL, buf, 0, player);
}

vector_tpl<convoihandle_t> const& depot_get_convoy_list(depot_t *depot)
{
	static vector_tpl<convoihandle_t> list;
	list.clear();
	if (depot==NULL) {
		return list;
	}
	// fill list
	slist_tpl<convoihandle_t> const& slist = depot->get_convoy_list();

	for(slist_tpl<convoihandle_t>::const_iterator i = slist.begin(), end = slist.end(); i!=end; ++i) {
		list.append(*i);
	}
	return list;
}


void export_map_objects(HSQUIRRELVM vm)
{
	/**
	 * Class to access objects on the map
	 * These classes cannot modify anything.
	 */
	begin_class(vm, "map_object_x", "extend_get,coord3d,ingame_object");
	uint8 objtype = bind_code<obj_t>::objtype;
	sq_settypetag(vm, -1, obj_t_tag + objtype);
	/**
	 * Constructor. Implemented by derived classes.
	 * Fails if no object of precisely the requested type is on the tile.
	 * @param x
	 * @param y
	 * @param z
	 * @param type of the map object
	 * @typemask void(integer,integer,integer,map_objects)
	 */
	register_function(vm, exp_obj_pos_constructor, "constructor", 5, "xiiii");
	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<obj_t*>(vm); //register_function("is_valid")
	/**
	 * @returns owner of the object.
	 */
	register_method(vm, &obj_t::get_owner, "get_owner");
	/**
	 * @returns raw name.
	 */
	register_method(vm, &obj_t::get_name, "get_name");
	/**
	 * @returns way type, can be @ref wt_invalid.
	 */
	register_method(vm, &obj_t::get_waytype, "get_waytype");
	/**
	 * @returns position.
	 */
	register_method(vm, &obj_t::get_pos, "get_pos");
	/**
	 * Checks whether player can remove this object.
	 * @returns error message or null if object can be removed.
	 */
	register_method(vm, &obj_t::is_deletable, "is_removable");
	/**
	 * @returns type of object.
	 */
	register_method(vm, &obj_t::get_typ, "get_type");
	/**
	 * Marks the object for highlighting. Use with care.
	 */
	register_method(vm, &mark_object, "mark", true);
	/**
	 * Unmarks the object for highlighting.
	 */
	register_method(vm, &unmark_object, "unmark", true);
	/**
	 * @returns whether object is highlighted.
	 */
	register_method(vm, &object_is_marked, "is_marked", true);
	end_class(vm);


	/**
	 * Trees on the map.
	 */
	begin_obj_class<baum_t>(vm, "tree_x", "map_object_x");
	/**
	 * @returns age of tree in months.
	 */
	register_method(vm, &baum_t::get_age, "get_age");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &baum_t::get_desc, "get_desc");

	end_class(vm);

	/**
	 * Buildings.
	 */
	begin_obj_class<gebaeude_t>(vm, "building_x", "map_object_x");
	/**
	 * @returns factory if building belongs to one, otherwise null
	 */
	register_method(vm, &gebaeude_t::get_fabrik, "get_factory");
	/**
	 * @returns city if building belongs to one, otherwise null
	 */
	register_method(vm, &gebaeude_t::get_stadt, "get_city");
	/**
	 * @returns whether building is townhall
	 */
	register_method(vm, &gebaeude_t::is_townhall, "is_townhall");
	/**
	 * @returns whether building is headquarters
	 */
	register_method(vm, &gebaeude_t::is_headquarter, "is_headquarter");
	/**
	 * @returns whether building is a monument
	 */
	register_method(vm, &gebaeude_t::is_monument, "is_monument");
	/**
	 * Passenger level controls how many passengers will be generated by this building.
	 * @returns passenger level
	 */
	register_method(vm, &gebaeude_t::get_passagier_level, "get_passenger_level");
	/**
	 * Mail level controls how many mail will be generated by this building.
	 * @returns mail level
	 */
	register_method(vm, &gebaeude_t::get_mail_level, "get_mail_level");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &gebaeude_t::get_tile, "get_desc");
	/**
	 * @returns true if both building tiles are part of one (multi-tile) building
	 */
	register_method(vm, &gebaeude_t::is_same_building, "is_same_building");

	end_class(vm);

	/**
	 * Class to access depots.
	 * Main purpose is the manipulation of convoys in depots.
	 */
	begin_obj_class<depot_t>(vm, "depot_x", "building_x");
	/**
	 * Append a car to the convoy in this depot. If the convoy does not exist, a new one is created first.
	 * @param pl player owns the convoy
	 * @param cnv the convoy
	 * @param desc decriptor of the vehicle
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_append_vehicle, "append_vehicle", true);
	/**
	 * Start the convoy in this depot.
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_start_convoy, "start_convoy", true);
	/**
	 * Start all convoys in this depot.
	 * @ingroup game_cmd
	 */
	register_method_fv(vm, depot_start_convoy, "start_all_convoys", freevariable<convoihandle_t>(convoihandle_t()), true);
	/**
	 * @returns list of convoys sitting in this depot
	 */
	register_method(vm, &depot_get_convoy_list, "get_convoy_list", true);
	end_class(vm);

	/**
	 * Ways.
	 */
	begin_obj_class<weg_t>(vm, "way_x", "map_object_x");
	/**
	 * @return if this way has sidewalk - only meaningful for roads
	 */
	register_method(vm, &weg_t::hat_gehweg, "has_sidewalk");
	/**
	 * @return whether way is electrified by some way-object
	 */
	register_method(vm, &weg_t::is_electrified, "is_electrified");
	/**
	 * @return whether there is a road-sign associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_sign, "has_sign");
	/**
	 * @return whether there is a signal associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_signal, "has_signal");
	/**
	 * @return whether there is a way-object associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_wayobj, "has_wayobj");
	/**
	 * @return whether there is a crossing associated to the way on the tile
	 */
	register_method(vm, &weg_t::is_crossing, "is_crossing");
	/**
	 * Return directions of this way. One-way signs are ignored here.
	 * @returns direction
	 * @typemask dir()
	 */
	register_function_fv(vm, &get_way_ribi, "get_dirs", 1, "x", freevariable<bool>(false) );
	/**
	 * Return directions of this way. Some signs restrict available directions.
	 * @returns direction
	 * @typemask dir()
	 */
	register_function_fv(vm, &get_way_ribi, "get_dirs_masked", 1, "x", freevariable<bool>(true) );
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &weg_t::get_desc, "get_desc");
	end_class(vm);


	/**
	 * Labels.
	 */
	begin_obj_class<label_t>(vm, "label_x", "map_object_x");
	/**
	 * Creates a new marker.
	 * @param pos  position
	 * @param pl   owner
	 * @param text text
	 * @ingroup game_cmd
	 */
	STATIC register_method(vm, &create_marker, "create", false, true);
	/**
	 * Set text of marker.
	 * @param text text
	 * @ingroup rename_func
	 */
	register_method(vm, &label_set_text, "set_text", true);
	/**
	 * Get text of marker.
	 * @see tile_x::get_text
	 */
	register_method(vm, &label_get_text, "get_text", true);

	end_class(vm);

	/**
	 * Roadsigns and railway signals.
	 */
	begin_obj_class<roadsign_t>(vm, "sign_x", "map_object_x");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &roadsign_t::get_desc, "get_desc");

	/**
	 * Test whether @p player 's vehicles can pass private-way sign.
	 * Returns true if this is not a private-way sign.
	 * @param player
	 */
	register_method(vm, &roadsign_can_pass, "can_pass", true);
	end_class(vm);
}
