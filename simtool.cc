/*
 * Tools for the players
 *
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "simdebug.h"
#include "simevent.h"
#include "simcity.h"
#include "simmesg.h"
#include "simconvoi.h"
#include "gui/simwin.h"
#include "display/viewport.h"

#include "bauer/fabrikbauer.h"
#include "bauer/vehikelbauer.h"

#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/schiene.h"
#include "boden/tunnelboden.h"
#include "boden/monorailboden.h"

#include "simdepot.h"
#include "simfab.h"
#include "display/simimg.h"
#include "simintr.h"
#include "simhalt.h"
#include "simskin.h"

#include "besch/grund_besch.h"
#include "besch/haus_besch.h"
#include "besch/roadsign_besch.h"
#include "besch/tunnel_besch.h"

#include "vehicle/simvehicle.h"
#include "vehicle/simroadtraffic.h"
#include "vehicle/simpeople.h"

#include "gui/line_management_gui.h"
#include "gui/tool_selector.h"
#include "gui/station_building_select.h"
#include "gui/karte.h"	// to update map after construction of new industry
#include "gui/depot_frame.h"
#include "gui/fahrplan_gui.h"
#include "gui/player_frame_t.h"
#include "gui/schedule_list.h"
#include "gui/signal_spacing.h"
#include "gui/city_info.h"
#include "gui/trafficlight_info.h"
#include "gui/privatesign_info.h"
#include "gui/messagebox.h"

#include "obj/zeiger.h"
#include "obj/bruecke.h"
#include "obj/tunnel.h"
#include "obj/groundobj.h"
#include "obj/signal.h"
#include "obj/crossing.h"
#include "obj/roadsign.h"
#include "obj/wayobj.h"
#include "obj/leitung2.h"
#include "obj/baum.h"
#include "obj/field.h"
#include "obj/label.h"

#include "dataobj/settings.h"
#include "dataobj/environment.h"
#include "dataobj/fahrplan.h"
#include "dataobj/route.h"
#include "dataobj/scenario.h"
#include "network/network_cmd_ingame.h" // for dragging raise / lower tools

#include "bauer/tunnelbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"

#include "besch/weg_besch.h"
#include "besch/roadsign_besch.h"

#include "tpl/vector_tpl.h"

#include "network/memory_rw.h"
#include "utils/simrandom.h"
#include "utils/simstring.h"

#include "simtool.h"
#include "player/finance.h"


#define is_scenario()  welt->get_scenario()->is_scripted()

#define CHECK_FUNDS() \
	/* do not allow, if out of money */ \
	if(  !welt->get_settings().is_freeplay()  &&  player->get_player_nr()!=1  &&  !player->has_money_or_assets() ) {\
		return "Out of funds";\
	}\


/****************************************** static helper functions **************************************/

/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char *tooltip_with_price(const char * tip, sint64 price)
{
	const int n = sprintf(tool_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(tool_t::toolstr+n, (double)price/-100.0);
	return tool_t::toolstr;
}



/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char *tooltip_with_price_maintenance(karte_t *welt, const char *tip, sint64 price, sint64 maintenance)
{
	int n = sprintf(tool_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(tool_t::toolstr+n, (double)price/-100.0);
	strcat( tool_t::toolstr, " (" );
	n = strlen(tool_t::toolstr);

	money_to_string(tool_t::toolstr+n, (double)(welt->scale_with_month_length(maintenance) ) / 100.0);
	strcat( tool_t::toolstr, ")" );
	return tool_t::toolstr;
}



/**
 * Creates a tooltip from tip text and money value
 */
static char const* tooltip_with_price_maintenance_capacity(karte_t* const welt, char const* const tip, sint64 const price, sint64 const maintenance, uint32 const capacity, uint8 const enables)
{
	int n = sprintf(tool_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(tool_t::toolstr+n, (double)price/-100.0);
	strcat( tool_t::toolstr, " (" );
	n = strlen(tool_t::toolstr);

	money_to_string(tool_t::toolstr+n, (double)(welt->scale_with_month_length(maintenance) ) / 100.0);
	strcat( tool_t::toolstr, ")" );
	n = strlen(tool_t::toolstr);

	if((enables&7)!=0) {
		n += sprintf( tool_t::toolstr+n, ", %d", capacity );
		if(enables&1) {
			n += sprintf( tool_t::toolstr+n, " %s", translator::translate("Passagiere") );
		}
		if(enables&2) {
			n += sprintf( tool_t::toolstr+n, " %s", translator::translate("Post") );
		}
		if(enables&4) {
			n += sprintf( tool_t::toolstr+n, " %s", translator::translate("Fracht") );
		}
	}
	else if (!welt->get_settings().is_separate_halt_capacities()) {
		n += sprintf( tool_t::toolstr+n, ", %s %d", translator::translate("Storage capacity"), capacity );
	}

	return tool_t::toolstr;
}



/**
 * sucht Haltestelle um Umkreis +1/-1 um (pos, b, h)
 * extended to search first in our direction
 * @author Hj. Malthaner, V.Meyer, prissi
 */
static halthandle_t suche_nahe_haltestelle(player_t *player, karte_t *welt, koord3d pos, sint16 b=1, sint16 h=1)
{
	koord k(pos.get_2d());

	// any other ground with a valid stop here?
	halthandle_t my_halt;
	if(  planquadrat_t* plan=welt->access(pos.get_2d())  ) {
		my_halt = plan->get_halt( player );
		if(  my_halt.is_bound()  ) {
			return my_halt;
		}
	}

	grund_t *bd = welt->lookup(pos);
	if(  bd==NULL  ) {
		bd = welt->lookup_kartenboden(k);
	}

	// first we try to connect to a stop straight in our direction; otherwise our station may break during construction
	if(  bd->hat_wege()  ) {
		ribi_t::ribi ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		for(  int i=0;  i<4;  i++ ) {
			if(  ribi_t::nsow[i] & ribi ) {
				if(  planquadrat_t* plan=welt->access(k+koord::nsow[i])  ) {
					my_halt = plan->get_halt( player );
					if(  my_halt.is_bound()  ) {
						return my_halt;
					}
				}
			}
		}
	}

	// now just search all neighbours
	for(  sint16 y=-1;  y<=h;  y++  ) {
		for(  sint16 x=-1;  x<=b;  (x==-1 && y>-1 && y<h) ? x=b:x++  ) {
			if(  planquadrat_t* plan=welt->access(k+koord(x,y))  ) {
				my_halt = plan->get_halt( player );
				if(  my_halt.is_bound()  ) {
					return my_halt;
				}
			}
		}
	}

#if AUTOJOIN_PUBLIC
	// now search everything for public stops
	for(  int i=0;  i<8;  i++ ) {
		if(  planquadrat_t* plan=welt->access(k+koord::neighbours[i])  ) {
			my_halt = plan->get_halt( welt->get_player(1) );
			if(  my_halt.is_bound()  ) {
				return my_halt;
			}
		}
	}
#endif

	// here we reach only with a non-found station, i.e. a non-bounded handle
	return halthandle_t();
}


// converts a 2d koord to a suitable ground pointer
static grund_t *tool_intern_koord_to_weg_grund(player_t *player, karte_t *welt, koord3d pos, waytype_t wt)
{
	// check for valid ground
	grund_t *gr=welt->lookup(pos);
	if (gr==NULL) {
		return NULL;
	}

	if(  wt==powerline_wt  &&  gr->get_leitung()  ) {
		// check for ownership
		if(gr->get_leitung()->is_deletable(player)!=NULL) {
			return NULL;
		}
		// ok
		else {
			return gr;
		}
	}

	// tram
	if(wt==tram_wt) {
		weg_t *way = gr->get_weg(track_wt);
		if (way && way->get_besch()->get_styp() == weg_t::type_tram &&  way->is_deletable(player)==NULL) {
			return gr;
		}
		else {
			return NULL;
		}
	}


	// has some rail or monorail?
	if(  !gr->hat_weg(wt)  ) {
		return NULL;
	}
	// check for ownership
	if(gr->get_weg(wt)->is_deletable(player)!=NULL){
		return NULL;
	}
	// ok, now we have a valid ground
	return gr;
}



/****************************************** now the actual tools **************************************/
const char *tool_query_t::work( player_t *player, koord3d pos )
{
	grund_t *gr = welt->lookup(pos);
	if(gr) {
		DBG_MESSAGE("tool_query_t()","checking map square %s", pos.get_str());

		if(  env_t::single_info  ) {

			int old_count = win_get_open_count();

			if(  is_ctrl_pressed()  ) {
				// reverse order
				for(int n=0;  n<gr->get_top();  n++  ) {
					obj_t *obj = gr->obj_bei(n);
					if(  obj && obj->get_typ()!=obj_t::wayobj && obj->get_typ()!=obj_t::pillar && obj->get_typ()!=obj_t::label  ) {
						DBG_MESSAGE("tool_query_t()", "index %d", n);
						obj->show_info();
						// did some new window open?
						if(old_count!=win_get_open_count()) {
							return NULL;
						}
					}
				}

				if(  gr->get_flag(grund_t::marked)  ) {
					label_t *lb = gr->find<label_t>();
					if(  lb  ) {
						lb->show_info();
						if(  old_count!=win_get_open_count()  ) {
							return NULL;
						}
					}
				}

				if(  gr->get_halt().is_bound()  ) {
					gr->get_halt()->zeige_info();
					if(  old_count!=win_get_open_count()  ) {
						return NULL;
					}
				}

			}
			else {

				// show halt and labels first ...
				if(  gr->get_halt().is_bound()  ) {
					gr->get_halt()->zeige_info();
					if(  old_count!=win_get_open_count()  ) {
						return NULL;
					}
				}
				if(  gr->get_flag(grund_t::marked)  ) {
					label_t *lb = gr->find<label_t>();
					if(  lb  ) {
						lb->show_info();
						if(  old_count!=win_get_open_count()  ) {
							return NULL;
						}
					}
				}

				for (size_t n = gr->get_top(); n-- != 0;) {
					obj_t *obj = gr->obj_bei(n);
					if(  obj && obj->get_typ()!=obj_t::wayobj && obj->get_typ()!=obj_t::pillar && obj->get_typ()!=obj_t::label  ) {
						DBG_MESSAGE("tool_query_t()", "index %u", (unsigned)n);
						obj->show_info();
						// did some new window open?
						if(old_count!=win_get_open_count()) {
							return NULL;
						}
					}
				}
			}

			// no window yet opened -> try ground info
			gr->zeige_info();
		}
		else {
			// lowest (less interesting) first
			gr->zeige_info();
			for(int n=0; n<gr->get_top();  n++  ) {
				obj_t *obj = gr->obj_bei(n);
				if(  obj && obj->get_typ()!=obj_t::wayobj && obj->get_typ()!=obj_t::pillar  ) {
					obj->show_info();
				}
			}
		}

		if(gr->get_depot()  &&  gr->get_depot()->get_owner()==player) {
			int old_count = win_get_open_count();
			gr->get_depot()->show_info();
			// did some new window open?
			if(env_t::single_info  &&  old_count!=win_get_open_count()) {
				return NULL;
			}
		}
	}
	return NULL;
}


/* delete things from a tile
 * citycars and pedestrian first and then go up to queue to more important objects
 */
bool tool_remover_t::tool_remover_intern(player_t *player, koord3d pos, sint8 type, const char *&msg)
{
DBG_MESSAGE("tool_remover_intern()","at (%s)", pos.get_str());
	// check if there is something to remove from here ...
	grund_t *gr = welt->lookup(pos);
	if (!gr  ||  gr->get_top()==0) {
		msg = "";
		return false;
	}

	// marker?
	if (type == obj_t::label  ||  type == obj_t::undefined) {
		if (label_t* l = gr->find<label_t>()) {
			msg = l->is_deletable(player);
			if(msg==NULL) {
				delete l;
				return true;
			}
			else if(  gr->get_top()==1  ||  type == obj_t::label  ) {
				// only complain if this is the last object on this tile ...
				return false;
			}
			msg = NULL;
			// not deletable: skip it
		}
	}

	// citycar? (we allow always)
	if (type == obj_t::road_vehicle  ||  type == obj_t::undefined) {
		if (private_car_t* citycar = gr->find<private_car_t>()) {
			delete citycar;
			return true;
		}
	}
	// pedestrians?
	if (type == obj_t::pedestrian  ||  type == obj_t::undefined) {
		if (pedestrian_t* pedestrian = gr->find<pedestrian_t>()) {
			delete pedestrian;
			return true;
		}
	}

	koord k(pos.get_2d());

	// prissi: check powerline (can cross ground of another player)
	leitung_t* lt = gr->get_leitung();
	// check whether powerline related stuff should be removed, and if there is any to remove
	if (  (type == obj_t::label  ||  type == obj_t::pumpe  ||  type == obj_t::senke  ||  type == obj_t::undefined)
	       &&  lt != NULL  &&  lt->is_deletable(player) == NULL) {
		if(  gr->ist_bruecke()  ) {
			bruecke_t* br = gr->find<bruecke_t>();
			if(  br == NULL  ) {
				// no bridge? most likely transformer on a former bridge tile...
				grund_t *gr_new = new boden_t(pos, gr->get_grund_hang());
				gr_new->take_obj_from( gr );
				welt->access(k)->kartenboden_setzen( gr_new );
				gr = gr_new;
			}
			else if(  br->get_besch()->get_waytype() == powerline_wt  ) {
				msg = brueckenbauer_t::remove(player, gr->get_pos(), powerline_wt );
				return msg == NULL;
			}
		}
		if(gr->ist_tunnel()  &&  gr->ist_karten_boden()) {
			if (gr->find<tunnel_t>()->get_besch()->get_waytype()==powerline_wt) {
				msg = tunnelbauer_t::remove(player, gr->get_pos(), powerline_wt, is_ctrl_pressed() );
				return msg == NULL;
			}
		}
		if(  gr->ist_im_tunnel()  ) {
			lt->cleanup(player);
			delete lt;
			// now everything gone?
			if(  gr->get_top() == 1  ) {
				// delete tunnel too
				tunnel_t *t = gr->find<tunnel_t>();
				t->cleanup(player);
				delete t;
			}
			// unmark kartenboden (is marked during underground mode deletion)
			welt->lookup_kartenboden(k)->clear_flag(grund_t::marked);
			// remove upper or lower ground
			welt->access(k)->boden_entfernen(gr);
			delete gr;
		}
		else {
			lt->cleanup(player);
			delete lt;
		}
		return true;
	}

	// check for signal
	roadsign_t* rs = gr->find<signal_t>();
	if (rs == NULL) rs = gr->find<roadsign_t>();
	if ( (type == obj_t::signal  ||  type == obj_t::roadsign  ||  type == obj_t::undefined)  &&  rs!=NULL) {
		msg = rs->is_deletable(player);
		if(msg) {
			return false;
		}
DBG_MESSAGE("tool_remover()",  "removing roadsign at (%s)", pos.get_str());
		weg_t *weg = gr->get_weg(rs->get_besch()->get_wtyp());
		if(  weg==NULL  &&  rs->get_besch()->get_wtyp()==tram_wt  ) {
			weg = gr->get_weg(track_wt);
		}
		rs->cleanup(player);
		delete rs;
		assert( weg );
		weg->count_sign();
		return true;
	}

	// check stations
	halthandle_t halt = gr->get_halt();
DBG_MESSAGE("tool_remover()", "bound=%i",halt.is_bound());
	if (gr->is_halt()  &&  halt.is_bound()  &&  fabrik_t::get_fab(k)==NULL  &&  type == obj_t::undefined) {
		// halt and not a factory (oil rig etc.)
		const player_t* owner = halt->get_owner();
		if(  player_t::check_owner( owner, player )  ) {
			return haltestelle_t::remove(player, gr->get_pos());
		}
	}

	// catenary or something like this
	wayobj_t* wo = gr->find<wayobj_t>();
	if(wo  &&  (type == obj_t::wayobj  ||  type == obj_t::undefined)) {
		msg = wo->is_deletable(player);
		if(msg) {
			return false;
		}
		wo->cleanup(player);
		delete wo;
		depot_t *dep = gr->get_depot();
		if( dep ) {
			dep->update_win();
		}
		return true;
	}

DBG_MESSAGE("tool_remover()", "check tunnel/bridge");

	// bridge?
	if(gr->ist_bruecke()  &&  (type == obj_t::bruecke  ||  type == obj_t::undefined)) {
DBG_MESSAGE("tool_remover()",  "removing bridge from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
		bruecke_t* br = gr->find<bruecke_t>();
		msg = brueckenbauer_t::remove(player, gr->get_pos(), br->get_besch()->get_waytype());
		return msg == NULL;
	}

	// beginning/end of tunnel
	if(gr->ist_tunnel()  &&  gr->ist_karten_boden()  &&  (type == obj_t::tunnel  ||  type == obj_t::undefined)) {
DBG_MESSAGE("tool_remover()",  "removing tunnel  from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
		waytype_t wegtyp =  gr->get_leitung() ? powerline_wt : gr->get_weg_nr(0)->get_waytype();
		msg = tunnelbauer_t::remove(player, gr->get_pos(), wegtyp, is_ctrl_pressed());
		return msg == NULL;
	}

	// fields
	field_t* f = gr->find<field_t>();
	if (f  &&  (type == obj_t::field  ||  type == obj_t::undefined)) {
		msg = f->is_deletable(player);
		if(msg==NULL) {
			f->cleanup(player);
			delete f;
			// fields have foundations ...
			sint8 dummy;
			welt->access(k)->boden_ersetzen( gr, new boden_t(gr->get_pos(), welt->recalc_natural_slope(k,dummy) ) );
			welt->lookup_kartenboden(k)->calc_bild();
			welt->lookup_kartenboden(k)->set_flag( grund_t::dirty );
		}
		return msg == NULL;
	}

	// depots
	depot_t* dep = gr->get_depot();
	if (dep  &&  (type == obj_t::bahndepot  ||  type == obj_t::undefined)) {
		// type == bahndepot to remove any type of depot
		msg = dep->is_deletable(player);
		if(msg) {
			return false;
		}
		dep->cleanup(player);
		delete dep;
		return true;
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if(gb != NULL  &&  (type == obj_t::gebaeude  ||  type == obj_t::undefined)) {
		msg = gb->is_deletable(player);
		if(msg) {
			return false;
		}
		if(!gb->get_tile()->get_besch()->can_rotate()  &&  welt->cannot_save()) {
			msg = "Not possible in this rotation!";
			return false;
		}
		DBG_MESSAGE("tool_remover()",  "removing building" );

		// remove town? (when removing townhall)
		if(gb->ist_rathaus()) {
			stadt_t *stadt = welt->suche_naechste_stadt(k);
			if(!welt->rem_stadt( stadt )) {
				msg = "Das Feld gehoert\neinem anderen Spieler\n";
				return false;
			}
		}
		else {
			// townhall is also removed during town removal
			hausbauer_t::remove( player, gb );
		}
		return true;
	}

	// if type is given, then leave here. Below other stuff and ways gets removed.
	if (type != obj_t::undefined) {
		msg = "Requested object not found.";
		return false;
	}

	// there is a powerline above this tile, but we do not own it
	// so we take it out and add it later again
	if(lt) {
DBG_MESSAGE("tool_remover()",  "took out powerline");
		gr->obj_remove(lt);
	}

	// do not delete crossing, so we remove it
	crossing_t *cr = gr->find<crossing_t>(2);
	if(cr) {
		gr->obj_remove(cr);
	}
	// do not delete pointers - they may come from players on other clients
	zeiger_t *zeiger = gr->find<zeiger_t>();
	if(zeiger) {
		gr->obj_remove(zeiger);
	}
	// do not delete other players label
	label_t *label = gr->find<label_t>();
	if(label) {
		gr->obj_remove(label);
	}

	// remove all other stuff (clouds, ...)
	bool return_ok = false;
	uint8 num_obj = gr->obj_count();
	if(num_obj>0) {
		msg = gr->kann_alle_obj_entfernen(player);
		return_ok = (msg==NULL  &&  !(gr->get_typ()==grund_t::brueckenboden  ||  gr->get_typ()==grund_t::tunnelboden)  &&  gr->obj_loesche_alle(player));
		DBG_MESSAGE("tool_remover()",  "removing everything from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
	}

	if(lt) {
		DBG_MESSAGE("tool_remover()",  "add again powerline");
		gr->obj_add(lt);
	}
	if(cr) {
		gr->obj_add(cr);
	}
	if(zeiger) {
		gr->obj_add(zeiger);
	}
	if(label) {
		gr->obj_add(label);
	}

	// could not delete everything
	if(msg) {
		return false;
	}
	if(return_ok) {
		// no sound
		msg = "";
		return true;
	}

	// ok, now we remove every object that should be removed - one by one.
	// the following objects will be removed together
DBG_MESSAGE("tool_remover()", "removing way");

	waytype_t wt = ignore_wt;
	if(gr->get_typ()!=grund_t::tunnelboden  ||  gr->has_two_ways()) {
		weg_t *w = gr->get_weg_nr(1);
		if(gr->get_typ()==grund_t::brueckenboden  &&  w==NULL) {
			// do not delete the middle of a bridge
			return false;
		}
		if(  w  &&  w->get_waytype()==water_wt  ) {
			// remove the other way first
			w = NULL;
		}
		if(w==NULL  ||  w->is_deletable(player)!=NULL) {
			w = gr->get_weg_nr(0);
			if(w==NULL) {
				// no way at all ...
				return true;
			}
			if(w->is_deletable(player)!=NULL){
				msg = w->is_deletable(player);
				return false;
			}
		}
		wt = w->get_besch()->get_finance_waytype();
		sint32 cost_sum = gr->weg_entfernen(w->get_waytype(), true);
		player_t::book_construction_costs(player, -cost_sum, k, wt);
	}
	else {
		// remove ways and tunnel
		if(  weg_t *weg = gr->get_weg_nr(0)  ) {
			gr->remove_everything_from_way(player, weg->get_waytype(), ribi_t::keine);
		}
		// tunnel without way: delete anything else
		if(  !gr->hat_wege()  ) {
			gr->obj_loesche_alle(player);
		}
	}

	// remove empty tile
	if(  !gr->ist_karten_boden()  &&  gr->get_top()==0  ) {
		// unmark kartenboden (is marked during underground mode deletion)
		welt->lookup_kartenboden(k)->clear_flag(grund_t::marked);
		// remove upper or lower ground
		welt->access(k)->boden_entfernen(gr);
		delete gr;
	}

	return true;
}



const char *tool_remover_t::work( player_t *player, koord3d pos )
{
	DBG_MESSAGE("tool_remover()","at %d,%d", pos.x, pos.y);

	obj_t::typ type = obj_t::undefined;

	if (default_param) {
		int t = atoi(default_param);
		if (t != 0  &&  -1 <= t  &&  t <= 200) {
			type = (obj_t::typ)t;
		}
	}

	const char *fail = NULL;
	if(!tool_remover_intern(player, pos, type, fail)) {
		return fail;
	}

	// must recalc neighbourhood for slopes etc.
	if(pos.x>1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::west)->calc_bild();
	}
	if(pos.y>1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::nord)->calc_bild();
	}

	if(pos.x<welt->get_size().x-1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::ost)->calc_bild();
	}
	if(pos.y<welt->get_size().y-1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::sued)->calc_bild();
	}

	return NULL;
}



const char *tool_raise_lower_base_t::move( player_t *player, uint16 buttonstate, koord3d pos )
{
	CHECK_FUNDS();

	const char *result = NULL;
	if(  buttonstate==1  ) {
		char buf[16];
		if(!is_dragging) {
			drag_height = get_drag_height(pos.get_2d());
		}
		is_dragging = true;
		sprintf( buf, "%i", drag_height );
		default_param = buf;
		if (env_t::networkmode) {
			// queue tool for network
			nwc_tool_t *nwc = new nwc_tool_t(player, this, pos, welt->get_steps(), welt->get_map_counter(), false);
			network_send_server(nwc);
		}
		else {
			result = work( player, pos );
		}
		default_param = NULL;
	}
	return result;
}


const char* tool_raise_lower_base_t::drag(player_t *player, koord k, sint16 height, int &n)
{
	if(  !welt->is_within_grid_limits(k)  ) {
		return "";
	}
	const char* err = NULL;

	// dragging may be going up or down!
	while(  welt->lookup_hgt(k) < height  &&  height <= welt->get_maximumheight()  ) {
		int diff = welt->grid_raise( player, k, err );
		if(  diff == 0  ) {
			break;
		}
		n += diff;
	}

	// when going down need to check here we will not be going below sea level
	// cannot rely on check within lower as water height can be recalculated
	while(  height >= welt->get_water_hgt(k)  &&  welt->lookup_hgt(k) > height  &&  height >= welt->get_minimumheight()  ) {
		int diff = welt->grid_lower( player, k, err );
		if(  diff == 0  ) {
			break;
		}
		n += diff;
	}

	return err; //height == welt->lookup_hgt(k);
}


bool tool_raise_lower_base_t::check_dragging()
{
	// reset dragging
	if(  is_dragging  &&  strempty(default_param)  ) {
		is_dragging = false;
		return false;
	}
	return true;
}


sint16 tool_raise_t::get_drag_height(koord k)
{
	const grund_t *gr = welt->lookup_kartenboden_gridcoords(k);

	return  gr->get_hoehe(welt->get_corner_to_operate(k)) + 1;
}


const char *tool_raise_t::check_pos(player_t *, koord3d pos )
{
	// check for underground mode
	if(  is_dragging  &&  drag_height-1 > grund_t::underground_level  ) {
		is_dragging = false;
		return "";
	}
	if(  !welt->is_within_grid_limits(pos.get_2d())  ) {
		return "";
	}
	sint8 h = (sint8) get_drag_height(pos.get_2d());
	if(  h > grund_t::underground_level  ) {
		return "Terraforming not possible\nhere in underground view";
	}
	return NULL;
}


const char *tool_raise_t::work(player_t* player, koord3d pos )
{
	if (!check_dragging()) {
		return NULL;
	}

	const char* err = NULL;
	koord k = pos.get_2d();

	CHECK_FUNDS();

	if(welt->is_within_grid_limits(k)) {

		const sint8 hgt = (sint8) get_drag_height(k);

		if(  hgt <= welt->get_maximumheight()  ) {

			int n = 0;	// tiles changed
			if(  !strempty(default_param)  ) {
				// called by dragging or by AI
				err = drag(player, k, atoi(default_param), n);
			}
			else {
				n = welt->grid_raise(player, k, err);
			}
			if(n>0) {
				player_t::book_construction_costs(player, welt->get_settings().cst_alter_land * n, k, ignore_wt);
			}
			return err == NULL ? (n ? NULL : "")
			                   : (*err == 0 ? "Tile not empty." : err);
		}
		else {
			// no mountains higher than welt->get_maximumheight() ...
			return "Maximum tile height difference reached.";
		}
	}
	return "Zu nah am Kartenrand";
}


sint16 tool_lower_t::get_drag_height(koord k)
{
	const grund_t *gr = welt->lookup_kartenboden_gridcoords(k);

	return  gr->get_hoehe(welt->get_corner_to_operate(k)) - 1;
}


const char *tool_lower_t::check_pos( player_t *, koord3d pos )
{
	// check for underground mode
	if (is_dragging  &&  drag_height+1 > grund_t::underground_level) {
		is_dragging = false;
		return "";
	}
	if (! welt->is_within_grid_limits(pos.get_2d())) {
		return "";
	}
	sint8 h = (sint8) get_drag_height(pos.get_2d());
	if (h > grund_t::underground_level) {
			return "Terraforming not possible\nhere in underground view";
	}
	return NULL;
}


const char *tool_lower_t::work( player_t *player, koord3d pos )
{
	if (!check_dragging()) {
		return NULL;
	}

	const char* err = NULL;
	koord k = pos.get_2d();

	CHECK_FUNDS();

	if(welt->is_within_grid_limits(k)) {
		const sint8 hgt = (sint8) get_drag_height(k);

		if(  hgt >= welt->get_water_hgt( k )  ) {
			int n = 0; // tiles changed
			if (!strempty(default_param)) {
				// called by dragging or by AI
				err = drag(player, k, atoi(default_param), n);
			}
			else {
				n = welt->grid_lower(player, k, err);
			}
			if(n>0) {
				player_t::book_construction_costs(player, welt->get_settings().cst_alter_land * n, k, ignore_wt);
			}
			return err == NULL ? (n ? NULL : "")
			                   : (*err == 0 ? "Tile not empty." : err);
		}
		else {
			// below water level
			return "";
		}
	}
	return "Zu nah am Kartenrand";
}


const char *tool_setslope_t::check_pos( player_t *, koord3d pos)
{
	grund_t *gr1 = welt->lookup(pos);
	if(gr1) {
		// check for underground mode
		if(  grund_t::underground_mode == grund_t::ugm_all  &&  !gr1->ist_tunnel()  ) {
			return "Terraforming not possible\nhere in underground view";
		}
	}
	else {
		return "";
	}
	return NULL;
}

const char *tool_restoreslope_t::check_pos( player_t *, koord3d pos)
{
	grund_t *gr1 = welt->lookup(pos);
	if(gr1) {
		// check for underground mode
		if(  grund_t::underground_mode == grund_t::ugm_all  &&  !gr1->ist_tunnel()  ) {
			return "Terraforming not possible\nhere in underground view";
		}
	}
	else {
		return "";
	}
	return NULL;
}

const char *tool_setslope_t::tool_set_slope_work( player_t *player, koord3d pos, int new_slope )
{
	if(  !grund_besch_t::double_grounds  ) {
		// translate old single slope parameter to new double slope
		if(  0 < new_slope  &&  new_slope < ALL_UP_SLOPE_SINGLE  ) {
			new_slope = scorner1(new_slope) + scorner2(new_slope) * 3 + scorner3(new_slope) * 9 + scorner4(new_slope) * 27;
		}
		else {
			switch(  new_slope  ) {
				case ALL_UP_SLOPE:
				case ALL_UP_SLOPE_SINGLE:   new_slope = ALL_UP_SLOPE;   break;
				case ALL_DOWN_SLOPE:
				case ALL_DOWN_SLOPE_SINGLE: new_slope = ALL_DOWN_SLOPE; break;
				case RESTORE_SLOPE:
				case RESTORE_SLOPE_SINGLE:  new_slope = RESTORE_SLOPE;  break;
				default:
					return ""; // invalid parameter
			}
		}
	}

	bool ok = false;

	grund_t *gr1 = welt->lookup(pos);
	if(  gr1  ) {
		koord k(pos.get_2d());

		sint8 water_hgt = welt->get_water_hgt( k );

		const uint8 max_hdiff = grund_besch_t::double_grounds ?  2 : 1;

		// at least a pixel away from the border?
		if(  pos.z < water_hgt  &&  !gr1->ist_tunnel()  ) {
			return "Maximum tile height difference reached.";
		}

		if(  new_slope==RESTORE_SLOPE  &&  !(gr1->get_typ()==grund_t::boden  ||  gr1->get_typ()==grund_t::wasser)  ) {
			return "No suitable ground!";
		}

		// finally: empty enough
		if(  gr1->get_grund_hang()!=gr1->get_weg_hang()  ||  gr1->get_halt().is_bound()  ||  gr1->kann_alle_obj_entfernen(player)  ||
				   gr1->find<gebaeude_t>()  ||  gr1->get_depot()  ||  (gr1->get_leitung() && gr1->hat_wege())  ||  gr1->get_weg(air_wt)  ||  gr1->find<label_t>()  ||  gr1->get_typ()==grund_t::brueckenboden) {
			return "Tile not empty.";
		}

		if(  !welt->is_within_limits(k+koord(1,1))  ||  !welt->is_within_limits(k+koord(-1,-1))) {
			return "Zu nah am Kartenrand";
		}

		// slopes may affect the position and the total height!
		koord3d new_pos = pos;

		if(  gr1->hat_wege() || gr1->get_leitung() ) {
			// check the resulting slope
			ribi_t::ribi ribis = 0;
			if( gr1->hat_wege()) {
				ribis |= gr1->get_weg_nr(0)->get_ribi_unmasked();
				if(  gr1->get_weg_nr(1)  ) {
					ribis |= gr1->get_weg_nr(1)->get_ribi_unmasked();
				}
			}
			if( gr1->get_leitung()) {
				ribis |= gr1->get_leitung()->get_ribi();
			}

			if(  new_slope==RESTORE_SLOPE  ||  !ribi_t::ist_einfach(ribis)  ||  (new_slope<hang_t::erhoben  &&  ribi_t::rueckwaerts(ribi_typ(new_slope))!=ribis)  ) {
				// has the wrong tilt
				return "Tile not empty.";
			}
			/* new things getting tricky:
			 * A single way on an all up or down slope will result in
			 * a slope with the way as hinge.
			 */
			if(  new_slope==ALL_UP_SLOPE  ) {
				if(  gr1->get_weg_hang()==hang_t::flach  ) {
					new_slope = hang_typ(ribis);
				}
				else if(  gr1->get_weg_hang() == hang_typ(ribis)  ) {
					// check that weg_besch supports such steep slopes
					if(  (gr1->get_weg_nr(0)  &&  !gr1->get_weg_nr(0)->get_besch()->has_double_slopes())
					  ||  (gr1->get_weg_nr(1)  &&  !gr1->get_weg_nr(1)->get_besch()->has_double_slopes())
					  ||  (gr1->get_leitung()  &&  !gr1->get_leitung()->get_besch()->has_double_slopes())  ) {
						return "Tile not empty.";
					}
					new_slope = hang_typ(ribis) * 2;
				}
				else if(  gr1->get_weg_hang() == hang_typ( ribi_t::rueckwaerts(ribis) ) * 2  ) {
					new_pos.z++;
					if(  welt->lookup(new_pos)  ) {
						return "Tile not empty.";
					}
					new_slope = hang_typ( ribi_t::rueckwaerts(ribis) );
				}
				else if(  gr1->get_weg_hang() != hang_typ( ribi_t::rueckwaerts(ribis) )  ) {
					return "Maximum tile height difference reached.";
				}
			}
			else if(  new_slope==ALL_DOWN_SLOPE  ) {
				if(  gr1->get_grund_hang()==hang_typ(ribis)  ) {
					// do not lower tiles to sea
					if(  pos.z == water_hgt  &&  !gr1->ist_tunnel()  ) {
						return "Tile not empty.";
					}
				}
				else if(  gr1->get_grund_hang() == hang_typ(ribis) * 2  ) {
					if(  pos.z == water_hgt  &&  !gr1->ist_tunnel()  ) {
						return "Tile not empty.";
					}
					new_slope = hang_typ(ribis);
				}
				else if(  gr1->get_grund_hang() == hang_t::flach  ) {
					new_slope = hang_typ( ribi_t::rueckwaerts(ribis) );
					new_pos.z--;
					if(  welt->lookup(new_pos)  ) {
						return "Tile not empty.";
					}
				}
				else if(  gr1->get_grund_hang() == hang_typ( ribi_t::rueckwaerts(ribis) )  ) {
					// check that weg_besch supports such steep slopes
					if(  (gr1->get_weg_nr(0)  &&  !gr1->get_weg_nr(0)->get_besch()->has_double_slopes())
					  ||  (gr1->get_weg_nr(1)  &&  !gr1->get_weg_nr(1)->get_besch()->has_double_slopes())
					  ||  (gr1->get_leitung()  &&  !gr1->get_leitung()->get_besch()->has_double_slopes())  ) {
						return "Tile not empty.";
					}
					new_slope = hang_typ( ribi_t::rueckwaerts(ribis) ) * 2;
					new_pos.z--;
					if(  welt->lookup(new_pos)  ) {
						return "Tile not empty.";
					}
				}
				else {
					return "Maximum tile height difference reached.";
				}
			}
		}

		if(  new_slope == ALL_DOWN_SLOPE  ||  new_slope == RESTORE_SLOPE  ) {
			if(  new_slope == RESTORE_SLOPE  ) {
				// prissi: special action: set to natural slope
				sint8 min_hgt;
				new_slope = welt->recalc_natural_slope( k, min_hgt );
				new_pos = koord3d( k, min_hgt );
				DBG_MESSAGE("natural_slope","%i",new_slope);
			}
			else {
				new_slope = hang_t::flach;
				// is more intuitive: if there is a slope, first downgrade it
				if(  gr1->get_grund_hang() == 0  ) {
					new_pos.z--;
				}
			}

			// now prevent being lowered below neighbouring water
			sint8 water_table = (water_hgt >= (gr1->get_hoehe() + (gr1->get_grund_hang() ? 1 : 0))) ? water_hgt : welt->get_grundwasser() - 4;
			sint8 min_neighbour_height = gr1->get_hoehe();

			for(  sint16 i = 0 ;  i < 8 ;  i++  ) {
				const koord neighbour = k + koord::neighbours[i];

				if(  welt->is_within_grid_limits( neighbour )  ) {
					grund_t *gr2 = welt->lookup_kartenboden( neighbour );
					const sint8 water_hgt_neighbour = welt->get_water_hgt( neighbour );
					if(  gr2  &&  (water_hgt_neighbour >= (gr2->get_hoehe() + (gr2->get_grund_hang() ? 1 : 0)))  ) {
						water_table = max( water_table, water_hgt_neighbour );
					}
					if(  gr2  &&  gr2->get_hoehe() < min_neighbour_height  ) {
						min_neighbour_height = gr2->get_hoehe();
					}
				}
			}

			if(  water_table>new_pos.z  ||  (water_table == new_pos.z  &&  min_neighbour_height < new_pos.z)  ) {
				// do not lower tiles when it will be below water level
				return "Tile not empty.";
			}
			welt->set_water_hgt( k, water_table );
			water_hgt = water_table;
		}
		else if(  new_slope == ALL_UP_SLOPE  ) {
			new_slope = hang_t::flach;
			new_pos.z++;
		}

		// already some ground here (tunnel, bridge, monorail?)
		if(  new_pos.z != pos.z  &&  welt->lookup(new_pos) != NULL  ) {
			return "Tile not empty.";
		}
		// check for grounds above / below
		if(  new_pos.z >= pos.z  ) {
			grund_t *gr2 = welt->lookup( new_pos + koord3d(0, 0, 1) );
			if(  !gr2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, 2) );
			}
			if(  !gr2  &&  welt->get_settings().get_way_height_clearance()==2  &&  (gr1->hat_wege()  ||  gr1->get_leitung())  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, 3) );
			}
			// slope may alter amount of clearance required
			if(  gr2  &&  gr2->get_pos().z - new_pos.z + hang_t::min_diff( gr2->get_weg_hang(), new_slope ) < welt->get_settings().get_way_height_clearance()  ) {
				return "Tile not empty.";
			}
		}
		if(  new_pos.z <= pos.z  ) {
			grund_t *gr2 = welt->lookup( new_pos + koord3d(0, 0, -1) );
			if(  !gr2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, -2) );
			}
			if(  !gr2  &&  welt->get_settings().get_way_height_clearance()==2  ) {
				gr2 = welt->lookup( new_pos + koord3d(0, 0, -3) );
			}
			// slope may alter amount of clearance required
			if(  gr2  &&  new_pos.z - gr2->get_pos().z + hang_t::min_diff( new_slope, gr2->get_weg_hang() ) < welt->get_settings().get_way_height_clearance()  ) {
				return "Tile not empty.";
			}
		}

		// check, if action is valid ...
		const sint16 hgt=new_pos.z;
		// maximum difference check with tiles to north, south east and west
		const sint8 test_hgt = hgt+(new_slope!=0);

		if(  gr1->get_typ()==grund_t::boden  ) {
			for(  sint16 i = 0 ;  i < 4 ;  i++  ) {
				const koord neighbour = k + koord::nsow[i];

				const grund_t *gr_neighbour=welt->lookup_kartenboden(neighbour);
				if(gr_neighbour) {
					const sint16 gr_neighbour_hgt=gr_neighbour->get_hoehe() + (new_slope==ALL_DOWN_SLOPE && gr_neighbour->get_grund_hang()? 1 : 0);
					const sint8 diff_from_ground = abs(gr_neighbour_hgt-test_hgt);
					if(  diff_from_ground > 2 * max_hdiff  ) {
						return "Maximum tile height difference reached.";
					}
				}
			}
		}

		// ok, now we set the slope ...
		ok = (new_pos!=pos);
		bool slope_changed = new_slope!=gr1->get_grund_hang();
		ok |= slope_changed;

		if(ok) {
			if(  gr1->kann_alle_obj_entfernen(player)  ) {
				// not empty ...
				return "Tile not empty.";
			}
			// check way ownership
			if(gr1->hat_wege()) {
				if(gr1->get_weg_nr(0)->is_deletable(player)!=NULL) {
					return "Tile not empty.";
				}
				if(gr1->has_two_ways()  &&  gr1->get_weg_nr(1)->is_deletable(player)!=NULL) {
					return "Tile not empty.";
				}
			}

			// ok, it was a success
			if(  !gr1->ist_wasser()  &&  new_slope == 0  &&  hgt == water_hgt  &&  gr1->get_typ() != grund_t::tunnelboden  ) {
				// now water
				gr1->obj_loesche_alle(player);
				welt->access(k)->kartenboden_setzen( new wasser_t(new_pos) );
				gr1 = welt->lookup_kartenboden(k);
			}
			else if(  gr1->ist_wasser()  &&  (new_pos.z > water_hgt  ||  new_slope != 0)  ) {
				// build underwater hill first
				if(  !welt->ebne_planquadrat( player, k, water_hgt, false, true )  ) {
					return "Tile not empty.";
				}
				gr1->obj_loesche_alle(player);
				welt->access(k)->kartenboden_setzen( new boden_t(new_pos,new_slope) );
				gr1 = welt->lookup_kartenboden(k);
				welt->set_water_hgt(k, welt->get_grundwasser()-4);
			}
			else {
				gr1->set_grund_hang(new_slope);
				gr1->set_pos(new_pos);
				gr1->clear_flag(grund_t::marked);
				gr1->set_flag(grund_t::dirty);
				// update new positions if changed
				if(  new_pos!=pos  ) {
					for(  int i=0;  i<gr1->get_top();  i++  ) {
						gr1->obj_bei(i)->set_pos( new_pos );
					}
				}
				// correct tree offsets if slope has changed
				if(  slope_changed  ) {
					for(  int i=0;  i<gr1->get_top();  i++  ) {
						baum_t *tree = obj_cast<baum_t>(gr1->obj_bei(i));
						if (tree) {
							tree->recalc_off();
						}
					}
				}
				if(  !gr1->ist_karten_boden()  ) {
					gr1->calc_bild();
				}
			}

			// if there is a powerline here we need to treat it as newly built as it may connect to neighbours
			leitung_t *lt = gr1->get_leitung();
			if(  lt  ) {
				// remove maintenance for existing powerline
				player_t::add_maintenance(lt->get_owner(), -lt->get_besch()->get_wartung(), powerline_wt);
				lt->finish_rd();
			}

			if(  gr1->ist_karten_boden()  ) {
				if(  new_slope!=hang_t::flach  ) {
					// no lakes on slopes ...
					groundobj_t *obj = gr1->find<groundobj_t>();
					if(  obj  &&  obj->get_besch()->get_phases()!=16  ) {
						obj->cleanup(player);
						delete obj;
					}
					// connect canals to sea
					if(  gr1->get_hoehe() == water_hgt  &&  gr1->hat_weg(water_wt)  ) {
						grund_t *sea = welt->lookup_kartenboden(k - koord( ribi_typ(new_slope ) ));
						if (sea  &&  sea->ist_wasser()) {
							gr1->weg_erweitern(water_wt, ribi_t::rueckwaerts(ribi_typ(new_slope)));
							sea->calc_bild();
						}
					}
				}
				// recalc slope walls on neighbours
				for(int y=-1; y<=1; y++) {
					for(int x=-1; x<=1; x++) {
						grund_t *gr = welt->lookup_kartenboden(k+koord(x,y));
						gr->calc_bild();
					}
				}
				// correct the grid height
				if(  gr1->ist_wasser()  ) {
					sint8 grid_hgt = min( water_hgt, welt->lookup_hgt( k ) );
					welt->set_grid_hgt(k, grid_hgt );
				}
				else {
					welt->set_grid_hgt(k, gr1->get_hoehe()+ corner4(gr1->get_grund_hang()) );
				}
				reliefkarte_t::get_karte()->calc_map_pixel(k);

				welt->calc_climate( k, true );
			}
			settings_t const& s = welt->get_settings();
			player_t::book_construction_costs(player, new_slope == RESTORE_SLOPE ? s.cst_alter_land : s.cst_set_slope, k, ignore_wt);
		}
		// update limits
		if(  welt->min_height > gr1->get_hoehe()  ) {
			welt->min_height = gr1->get_hoehe();
		}
		else if(  welt->max_height < gr1->get_hoehe()  ) {
			welt->max_height = gr1->get_hoehe();
		}
	}
	return ok ? NULL : "";
}



// set marker
const char *tool_marker_t::work( player_t *player, koord3d pos )
{
	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
	if (gr) {
		if(!gr->get_text()) {
			const obj_t* thing = gr->obj_bei(0);
			if(thing == NULL  ||  thing->get_owner() == player  ||  (player_t::check_owner(thing->get_owner(), player)  &&  (thing->get_typ() != obj_t::gebaeude))) {
				gr->obj_add(new label_t(gr->get_pos(), player, default_param ? default_param : "\0"));
				if (can_use_gui()) {
					gr->find<label_t>()->show_info();
				}
				return NULL;
			}
		}
	}
	return "Das Feld gehoert\neinem anderen Spieler\n";
}



// show/repair blocks
bool tool_clear_reservation_t::init( player_t * )
{
	if (can_use_gui()) {
		schiene_t::show_reservations = true;
		welt->set_dirty();
	}
	return true;
}

bool tool_clear_reservation_t::exit( player_t * )
{
	if (can_use_gui()) {
		schiene_t::show_reservations = false;
		welt->set_dirty();
	}
	return true;
}

const char *tool_clear_reservation_t::work( player_t *, koord3d pos )
{
	grund_t *gr = welt->lookup(pos);
	if(gr) {
		for(unsigned wnr=0;  wnr<2;  wnr++  ) {

			schiene_t const* const w = obj_cast<schiene_t>(gr->get_weg_nr(wnr));
			// is this a reserved track?
			if(w!=NULL  &&  w->is_reserved()) {
				/* now we do a very crude procedure:
				 * - we search all ways for reservations of this convoi and remove them
				 * - we set the convoi state to ROUTING_1; it must reserve again its ways then
				 */
				const waytype_t waytype = w->get_waytype();
				const convoihandle_t cnv = w->get_reserved_convoi();
				if(cnv->get_state()==convoi_t::DRIVING) {
					// reset driving state
					cnv->suche_neue_route();
				}
				FOR(slist_tpl<weg_t*>, const w, weg_t::get_alle_wege()) {
					if (w->get_waytype() == waytype) {
						schiene_t* const sch = obj_cast<schiene_t>(w);
						if (sch->get_reserved_convoi() == cnv) {
							vehicle_t& v = *cnv->front();
							if (!gr->suche_obj(v.get_typ())) {
								// force free
								sch->unreserve(&v);
							}
						}
					}
				}
			}
		}
	}
	return NULL;
}


// transformer for electricity supply
const char* tool_transformer_t::get_tooltip(const player_t *) const
{
	settings_t const& s = welt->get_settings();
	sprintf(toolstr, "%s, %ld$ (%ld$)", translator::translate("Build drain"), (long)(s.cst_transformer / -100), (long)(welt->scale_with_month_length(s.cst_maintain_transformer)) / -100);
	return toolstr;
}

image_id tool_transformer_t::get_icon(player_t*) const
{
	return wegbauer_t::waytype_available( powerline_wt, welt->get_timeline_year_month() ) ? icon : IMG_LEER;
}

bool tool_transformer_t::init( player_t *)
{
	return wegbauer_t::waytype_available( powerline_wt, welt->get_timeline_year_month() );
}


const char *tool_transformer_t::check_pos( player_t *, koord3d pos )
{
	if(grund_t::underground_mode == grund_t::ugm_all  &&  env_t::networkmode) {
		// clients cannot guess at which height transformer should be build
		return "Cannot built this station/building\nin underground mode here.";
	}
	if(grund_t::underground_mode == grund_t::ugm_level) {
		// only above or directly under surface
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		return (gr->get_pos() == pos  ||  gr->get_hoehe() == grund_t::underground_level+1) ? NULL : "";
	}
	return NULL;
}


const char *tool_transformer_t::work( player_t *player, koord3d pos )
{
	DBG_MESSAGE("tool_transformer_t()","called on %d,%d", pos.x, pos.y);

	koord k(pos.get_2d());

	grund_t *gr = welt->lookup_kartenboden(k);
	if(  !welt->get_settings().get_allow_underground_transformers()  &&  pos.z!=gr->get_hoehe()  ) {
		// no underground transformers allowed
		return "Cannot built this station/building\nin underground mode here.";
	}

	bool underground = false;
	fabrik_t *fab = NULL;
	// full underground mode: coordinate is on ground, adjust it to one level below ground
	// not possible in network mode!
	if (!env_t::networkmode  &&  grund_t::underground_mode == grund_t::ugm_all) {
		pos = gr->get_pos() - koord3d( 0, 0, welt->get_settings().get_way_height_clearance() );
	}
	// search for factory
	// must be independent of network mode
	if (gr->get_pos().z <= pos.z) {
		fab = leitung_t::suche_fab_4(k);
	}
	else if(  gr->get_pos().z == pos.z+welt->get_settings().get_way_height_clearance()  ) {
		fab = fabrik_t::get_fab( k);
		underground = true;
	}

	if( !fab  ) {
		return "Transformer only next to factory!";
	}
	if(  fab->is_transformer_connected()  ) {
		return "Only one transformer per factory!";
	}

	// underground: first build tunnel tile	at coordinate pos
	if(underground) {
		if(gr->ist_wasser()) {
			return "Transformer only next to factory!";
		}

		if(welt->lookup(pos)) {
			return "Tile not empty.";
		}

		if(  welt->get_settings().get_way_height_clearance()==2  &&  welt->lookup(pos + koord3d( 0, 0, 1 ))  ) {
			return "Tile not empty.";
		}

		const tunnel_besch_t *tunnel_besch = tunnelbauer_t::find_tunnel(powerline_wt, 0, 0);
		if(  tunnel_besch==NULL  ) {
			return "Cannot built this station/building\nin underground mode here.";
		}

		tunnelboden_t* tunnel = new tunnelboden_t(pos, 0);
		welt->access(k)->boden_hinzufuegen(tunnel);
		tunnel->obj_add(new tunnel_t(pos, player, tunnel_besch));
		player_t::add_maintenance( player, tunnel_besch->get_wartung(), tunnel_besch->get_finance_waytype() );
		gr = tunnel;
	}
	else {
		// above ground: check for clear tile
		if(gr->get_grund_hang()!=0  ||  !gr->ist_natur()) {
			return "Transformer only on flat bare land!";
		}
		// remove everything on that spot
		if(const char *fail = gr->kann_alle_obj_entfernen(player)) {
			return fail;
		}
		gr->obj_loesche_alle(player);
	}
	// transformer will be build on tile pointed to by gr

	// build source or drain depending on factory type
	if(fab->get_besch()->is_electricity_producer()) {
		pumpe_t *p = new pumpe_t(gr->get_pos(), player);
		gr->obj_add( p );
		p->finish_rd();
	}
	else {
		senke_t *s = new senke_t(gr->get_pos(), player);
		gr->obj_add(s);
		s->finish_rd();
	}

	return NULL;	// ok
}



/**
 * found a new city
 * @author Hj. Malthaner
 */
const char *tool_add_city_t::work( player_t *player, koord3d pos )
{
	CHECK_FUNDS();

	koord k(pos.get_2d());

	grund_t *gr = welt->lookup_kartenboden(k);
	if(gr) {
		if(gr->ist_natur() &&
			!gr->ist_wasser() &&
			gr->get_grund_hang() == 0  &&
			hausbauer_t::get_special( 0, haus_besch_t::rathaus, welt->get_timeline_year_month(), 0, welt->get_climate( k ) ) != NULL  ) {

			gebaeude_t const* const gb = obj_cast<gebaeude_t>(gr->first_obj());
			if(gb && gb->ist_rathaus()) {
				dbg->warning("tool_add_city()", "Already a city here");
				return "Tile not empty.";
			}
			else {

				// Hajo: if city is owned by player and player removes special
				// buildings the game crashes. To avoid this problem cities
				// always belong to player 1

				int const citizens = (int)(welt->get_settings().get_mittlere_einwohnerzahl() * 0.9);
				//  stadt_t *stadt = new stadt_t(welt->get_player(1), pos,citizens/10+simrand(2*citizens+1));

				// always start with 1/10 citizens
				stadt_t* stadt = new stadt_t(welt->get_player(1), k, citizens / 10);
				if (stadt->get_buildings() == 0) {
					delete stadt;
					return "No suitable ground!";
				}

				welt->add_stadt(stadt);
				stadt->finish_rd();
				stadt->verbinde_fabriken();

				player_t::book_construction_costs(player, welt->get_settings().cst_found_city, k, ignore_wt);
				reliefkarte_t::get_karte()->calc_map();
				return NULL;
			}
		}
		else {
			return "No suitable ground!";
		}
	}
	return "";
}

// buy a house
const char *tool_buy_house_t::work( player_t *player, koord3d pos)
{
	if ( player == welt->get_player(1) ) {
		return "";
	}
	grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(!gr  ||  gr->hat_wege()  ||  gr->get_halt().is_bound()) {
		return "";
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if(  gb== NULL  ||  gb->get_haustyp()==gebaeude_t::unbekannt  ||  !player_t::check_owner(gb->get_owner(),player)  ) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	if(  gb->get_owner()==player  ) {
		// I bought this already ...
		return "";
	}

	player_t *old_owner = gb->get_owner();
	const haus_tile_besch_t *tile  = gb->get_tile();
	const haus_besch_t * hb = tile->get_besch();
	koord size = hb->get_groesse( tile->get_layout() );

	koord k;
	for(k.y = 0; k.y < size.y; k.y ++) {
		for(k.x = 0; k.x < size.x; k.x ++) {
			grund_t *gr = welt->lookup(koord3d(k,0)+pos);
			if(gr) {
				gebaeude_t *gb_part = gr->find<gebaeude_t>();
				// there may be buildings with holes
				if(  gb_part  &&  gb_part->get_tile()->get_besch()==hb  &&  player_t::check_owner(gb_part->get_owner(),player)  ) {
					sint32 const maint = welt->get_settings().maint_building * hb->get_level();
					player_t::add_maintenance(old_owner, -maint, gb->get_waytype());
					player_t::add_maintenance(player,        +maint, gb->get_waytype());
					gb->set_owner(player);
					player_t::book_construction_costs(player, -maint, k + pos.get_2d(), gb->get_waytype());
				}
			}
		}
	}
	return NULL;
}

/* change city size
 * @author prissi
 */
bool tool_change_city_size_t::init( player_t * )
{
	cursor = atoi(default_param)>0 ? tool_t::general_tool[TOOL_RAISE_LAND]->cursor : tool_t::general_tool[TOOL_LOWER_LAND]->cursor;
	return true;
}

const char *tool_change_city_size_t::work( player_t *, koord3d pos )
{
	stadt_t *city = welt->suche_naechste_stadt(pos.get_2d());
	if(city!=NULL) {
		city->change_size( atoi(default_param) );
		// Knightly : update the links from other cities to this city
		FOR(weighted_vector_tpl<stadt_t*>, const c, welt->get_staedte()) {
			c->remove_target_city(city);
			c->add_target_city(city);
		}
		return NULL;
	}
	return "";
}


/* change climate
 * @author kieron
 */
const char *tool_set_climate_t::get_tooltip(player_t const*) const
{
	char temp[1024];
	sprintf( temp, translator::translate( "Set tile climate" ), translator::translate( grund_besch_t::get_climate_name_from_bit((climate)atoi(default_param)) ) );
	return tooltip_with_price( temp,  welt->get_settings().cst_alter_climate );
}

uint8 tool_set_climate_t::is_valid_pos(player_t *player, const koord3d &, const char *& error, const koord3d &)
{
	error = NULL;
	// no dragging in networkmode but for admin
	return env_t::networkmode ? (player->get_player_nr()==1)+1 : 2;
}

void tool_set_climate_t::mark_tiles(player_t *, const koord3d &start, const koord3d &end)
{
	koord k1, k2;
	k1.x = start.x < end.x ? start.x : end.x;
	k1.y = start.y < end.y ? start.y : end.y;
	k2.x = start.x + end.x - k1.x;
	k2.y = start.y + end.y - k1.y;
	koord k;
	for(  k.x = k1.x;  k.x <= k2.x;  k.x++  ) {
		for(  k.y = k1.y;  k.y <= k2.y;  k.y++  ) {
			grund_t *gr = welt->lookup_kartenboden( k );

			zeiger_t *marker = new zeiger_t(gr->get_pos(), NULL );

			const uint8 grund_hang = gr->get_grund_hang();
			const uint8 weg_hang = gr->get_weg_hang();
			const uint8 hang = max( corner1(grund_hang), corner1(weg_hang) ) + 3 * max( corner2(grund_hang), corner2(weg_hang) ) + 9 * max( corner3(grund_hang), corner3(weg_hang) ) + 27 * max( corner4(grund_hang), corner4(weg_hang) );
			uint8 back_hang = (hang % 3) + 3 * ((uint8)(hang / 9)) + 27;
			marker->set_after_bild( grund_besch_t::marker->get_bild( grund_hang % 27 ) );
			marker->set_bild( grund_besch_t::marker->get_bild( back_hang ) );

			marker->mark_image_dirty( marker->get_image(), 0 );
			gr->obj_add( marker );
			marked.insert( marker );
		}
	}
}


const char *tool_set_climate_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	int n = 0;	// tiles altered
	climate cl = (climate) atoi(default_param);
	koord k1, k2;
	if(  end == koord3d::invalid  ) {
		k1.x = k2.x = start.x;
		k1.y = k2.y = start.y;
	}
	else {
		k1.x = start.x < end.x ? start.x : end.x;
		k1.y = start.y < end.y ? start.y : end.y;
		k2.x = start.x + end.x - k1.x;
		k2.y = start.y + end.y - k1.y;
	}
	koord k;
	for(  k.x = k1.x;  k.x <= k2.x;  k.x++  ) {
		for(  k.y = k1.y;  k.y <= k2.y;  k.y++  ) {
			if(  grund_t *gr=welt->lookup_kartenboden(k)  ) {
				if(  cl != water_climate  ) {
					bool ok = true;
					if(  gr->ist_wasser()  ) {
						const sint8 hgt = welt->lookup_hgt(k);
						ok = welt->get_water_hgt(k) == hgt  &&  welt->is_plan_height_changeable( k.x, k.y );
						// check s, se, e - these must not be deep water!
						for(  int i = 3 ;  i < 6 && ok ;  i++  ) {
							koord k_neighbour(k + koord::neighbours[i]);
							if(  welt->is_within_grid_limits(k_neighbour)  ) {
								ok = welt->lookup_hgt(k_neighbour) >= hgt;
							}
						}
						if(  ok  ) {
							gr->obj_loesche_alle( NULL );
							welt->set_water_hgt( k, hgt - 1 );
							welt->access(k)->correct_water();
						}
					}
					if(  ok  ) {
						welt->set_climate( k, cl, true );
						reliefkarte_t::get_karte()->calc_map_pixel( k );
						n ++;
					}
				}
				else if(  !gr->ist_wasser()  &&  gr->get_grund_hang() == hang_t::flach  &&  welt->is_plan_height_changeable( k.x, k.y )  ) {
					bool ok = true;
					for(  int i = 0 ;  i < 8;  i++  ) {
						grund_t *gr2 = welt->lookup_kartenboden( k + koord::neighbours[i] );
						if(  gr2  &&  ok  ) {
							ok = gr2->get_pos().z >= gr->get_pos().z;
						}
					}
					if(  ok  ) {
						gr->obj_loesche_alle( NULL );
						welt->set_water_hgt( k, gr->get_pos().z );
						welt->access(k)->correct_water();
						welt->set_climate( k, water_climate, true );
						reliefkarte_t::get_karte()->calc_map_pixel( k );
						n ++;
					}
				}

			}
		}
	}
	if(n>0) {
		player_t::book_construction_costs(player, welt->get_settings().cst_alter_climate * n, k, ignore_wt);
	}
	return NULL;
}


/* change water height
 * @author kieron
 */
bool tool_change_water_height_t::init( player_t *player )
{
	cursor = atoi(default_param) > 0 ? tool_t::general_tool[TOOL_RAISE_LAND]->cursor : tool_t::general_tool[TOOL_LOWER_LAND]->cursor;
	return !env_t::networkmode  ||  player->get_player_nr()==1;
}


const char *tool_change_water_height_t::work( player_t *, koord3d pos )
{
	if(  pos == koord3d::invalid  ) {
		return "Cannot alter water";
	}

	// calculate new height to use:
	bool raising = atoi(default_param) > 0;
	koord k = pos.get_2d();
	sint8 new_water_height;
	grund_t *gr = welt->lookup_kartenboden(k);

	if(  gr->ist_wasser()  ) {
		// lower + control removes shallow water only. If this tile is deep water this will fail
		if(  !raising  &&  is_ctrl_pressed()  &&  welt->min_hgt(k)!=gr->get_hoehe()  ) {
			return "Cannot alter water";
		}

		// if currently water, raise = +1, lower = -1
		new_water_height = gr->get_hoehe() + (raising ? 1 : -1);
	}
	// if not water then raise = set water height to ground height, lower = error
	else if(  raising  ) {
		hang_t::typ slope = gr->get_grund_hang();
		new_water_height = gr->get_hoehe() + max( max( corner1(slope), corner2(slope) ),max( corner3(slope), corner4(slope) ) );
	}
	else {
		return "Cannot alter water";
	}
	if(  new_water_height < welt->get_grundwasser() - 3  ) {
		return "Cannot alter water";
	}
	sint8 test_height = max( new_water_height, gr->get_hoehe() );

	// make a list of tiles to change
	// cannot use a recursive method as stack is not large enough!

	sint8 *from_dir = new sint8[welt->get_size().x * welt->get_size().y];
	sint8 *stage = new sint8[welt->get_size().x * welt->get_size().y];
	memset( from_dir, -1, sizeof(sint8) * welt->get_size().x * welt->get_size().y );
	memset( stage, -1, sizeof(sint8) * welt->get_size().x * welt->get_size().y );
#define array_koord(px,py) (px + py * welt->get_size().x)
	stage[array_koord(k.x,k.y)]=0;
	do {
		// firstly we must be able to change ground height
		bool ok = welt->is_plan_height_changeable( k.x, k.y )  &&  k.x > 0  &&  k.y > 0  &&  k.x < welt->get_size().x - 1  &&  k.y < welt->get_size().y - 1;
		const planquadrat_t *plan = welt->access(k);

		// next there cannot be any grounds directly above this tile
		sint8 h = plan->get_kartenboden()->get_hoehe() + 1;
		while(  ok  &&  h < new_water_height + welt->get_settings().get_way_height_clearance()  ) {
			if(  plan->get_boden_in_hoehe(h)  ) {
				ok = false;
			}
			h++;
		}

		if(  !ok  ) {
			delete [] from_dir;
			delete [] stage;
			return "Cannot alter water";
		}

		// get neighbour corner heights
		sint8 neighbour_heights[8][4];
		welt->get_neighbour_heights( k, neighbour_heights );

		for(  int i = stage[array_koord(k.x,k.y)];  i < 8;  i++  ) {
			koord k_neighbour = k + koord::neighbours[i];
			grund_t *gr2 = welt->lookup_kartenboden(k_neighbour);
			if(  gr2  ) {
				sint8 neighbour_height = gr2->get_hoehe();

				// move onto this tile if it hasn't been processed yet
				bool ok = stage[array_koord(k_neighbour.x, k_neighbour.y)] == -1;

				if(  raising  ) {
					// test whether points adjacent to current tile will be flooded
					// if control key modifier pressed, level ground will be left alone, but then need to check for spills

					// for neighbour i test corners adjacent to tile
					// nw (i = 0), test se (corner 1)
					// w (i = 1), test se (corner 1) and ne (corner 2)
					// sw (i = 2), test ne (corner 2)
					// s (i = 3), test ne (corner 2) and nw (corner 3)
					// se (i = 4), test nw (corner 3)
					// e (i = 5), test nw (corner 3) and sw (corner 0)
					// ne (i = 6), test sw (corner 0)
					// n (i = 7), test sw (corner 0) and se (corner 1)

					if(  is_ctrl_pressed()  ) {
						ok = ok  &&  ( (gr2->get_grund_hang()!=hang_t::flach  &&  welt->max_hgt(k_neighbour) <= test_height) ||
							neighbour_heights[i][((i >> 1) + 1) & 3] < test_height ||
							( (i & 1)  &&  neighbour_heights[i][((i >> 1) + 2) & 3] < test_height) );
					}
					else {
						ok = ok  &&  (neighbour_heights[i][((i >> 1) + 1) & 3] <= test_height ||
							( (i & 1)  &&  neighbour_heights[i][((i >> 1) + 2) & 3] <= test_height));
					}

					// move onto this tile unless it already has water at new level, or the land level is above new level
					ok = ok  &&  welt->get_water_hgt(k_neighbour) < new_water_height;
				}
				else {
					if(  is_ctrl_pressed()  ) {
						ok = ok  &&  welt->min_hgt(k_neighbour) == test_height;
					}
					else {
						ok = ok  &&  neighbour_height <= test_height;
					}

					// move onto this tile unless it already has water at new level, or the land level is above new level
					ok = ok  &&  welt->get_water_hgt(k_neighbour) > new_water_height;
				}

				if(  ok  ) {
					//move on to next tile
					from_dir[array_koord(k_neighbour.x,k_neighbour.y)] = i;
					stage[array_koord(k_neighbour.x,k_neighbour.y)] = 0;
					stage[array_koord(k.x,k.y)] = i;
					k = k_neighbour;
					break;
				}
			}
			//return back to previous tile
			if(  i==7  ) {
				stage[array_koord(k.x,k.y)] = 8;
				if(  from_dir[array_koord(k.x,k.y)] != -1  ) {
					k = k - koord::neighbours[from_dir[array_koord(k.x,k.y)]];
				}
			}
		}
	} while(  from_dir[array_koord(k.x,k.y)] != -1  ||  stage[array_koord(k.x,k.y)] < 7  );

	delete [] from_dir;

	// loop over map to find marked tiles
	for(  int y = 1;  y<welt->get_size().y - 1;  y++  ) {
		for(  int x = 1;  x<welt->get_size().x - 1;  x++  ) {
			if(  stage[array_koord(x,y)] > -1  ) {
				// calculate new height, slope and climate and set water height
				grund_t *gr2 =welt->lookup_kartenboden(x, y);

				// remove any objects on this tile
				gr2->obj_loesche_alle( NULL );

				const sint8 h0 = gr2->get_hoehe();
				const sint8 min_grid_hgt = welt->min_hgt( koord( x, y ) );

				sint8 h0_nw, h0_ne, h0_se, h0_sw;

				if(  gr2->ist_wasser()  ) {
					// water - maximum existing height can be is old water height no matter what surrounding grids are
					h0_nw = min(h0, welt->lookup_hgt(x, y));
					h0_ne = min(h0, welt->lookup_hgt(x+1, y));
					h0_se = min(h0, welt->lookup_hgt(x+1, y+1));
					h0_sw = min(h0, welt->lookup_hgt(x, y+1));
				}
				else if(  h0 > min_grid_hgt  ) {
					// if min grid height here is less than ground height it will be because we are partially water
					h0_nw = welt->lookup_hgt(x, y);
					h0_ne = welt->lookup_hgt(x+1, y);
					h0_se = welt->lookup_hgt(x+1, y+1);
					h0_sw = welt->lookup_hgt(x, y+1);
					if(  !gr2->ist_wasser()  ) {
						// while this appears to be a single height slope actually it is a double height slope half underwater
						const sint8 water_hgt = welt->get_water_hgt(x, y);
						h0_nw >= water_hgt ? h0_nw = h0 + corner4( gr2->get_grund_hang() ) : 0;
						h0_ne >= water_hgt ? h0_ne = h0 + corner3( gr2->get_grund_hang() ) : 0;
						h0_se >= water_hgt ? h0_se = h0 + corner2( gr2->get_grund_hang() ) : 0;
						h0_sw >= water_hgt ? h0_sw = h0 + corner1( gr2->get_grund_hang() ) : 0;
					}
				}
				else {
					// fully land
					h0_nw = h0 + corner4( gr2->get_grund_hang() );
					h0_ne = h0 + corner3( gr2->get_grund_hang() );
					h0_se = h0 + corner2( gr2->get_grund_hang() );
					h0_sw = h0 + corner1( gr2->get_grund_hang() );
				}


				const sint8 hneu_nw = max( new_water_height, h0_nw );
				const sint8 hneu_ne = max( new_water_height, h0_ne );
				const sint8 hneu_se = max( new_water_height, h0_se );
				const sint8 hneu_sw = max( new_water_height, h0_sw );
				const sint8 hneu = min( min( hneu_nw, hneu_ne ), min( hneu_se, hneu_sw ) );

				gr2->set_hoehe( hneu );

				const uint8 sneu = (hneu_sw - hneu > 2 ? 2 : hneu_sw - hneu) + ((hneu_se - hneu > 2 ? 2 : hneu_se-hneu) * 3) + ((hneu_ne - hneu > 2 ? 2 : hneu_ne - hneu) * 9) + ((hneu_nw - hneu > 2 ? 2 : hneu_nw - hneu) * 27);
				gr2->set_grund_hang( sneu );

				welt->set_water_hgt(x, y, new_water_height );
				welt->access(x, y)->correct_water();
				welt->calc_climate( koord( x, y ), true );
			}
		}
	}

	delete [] stage;

	return NULL;
}


char const* tool_plant_tree_t::move(player_t* const player, uint16 const b, koord3d const pos)
{
	if (b==0) {
		return NULL;
	}
	if (env_t::networkmode) {
		// queue tool for network
		nwc_tool_t *nwc = new nwc_tool_t(player, this, pos, welt->get_steps(), welt->get_map_counter(), false);
		network_send_server(nwc);
		return NULL;
	}
	else {
		return work( player, pos );
	}
}


const char *tool_plant_tree_t::work( player_t *player, koord3d pos )
{
	koord k(pos.get_2d());

	grund_t *gr = welt->lookup_kartenboden(k);
	if(gr) {
		const baum_besch_t *besch = NULL;
		bool check_climates = true;
		bool random_age = false;
		if(default_param==NULL  ||  strlen(default_param)==0) {
			besch = baum_t::random_tree_for_climate( welt->get_climate( k ) );
		}
		else {
			// parse default_param: bbbesch_nr b=1 ignore climate b=1 random age
			check_climates = default_param[0]=='0';
			random_age = default_param[1]=='1';
			besch = baum_t::find_tree(default_param+3);
		}
		if(besch  &&  baum_t::plant_tree_on_coordinate( k, besch, check_climates, random_age )  ) {
			player_t::book_construction_costs(player, welt->get_settings().cst_remove_tree, k, ignore_wt);
			return NULL;
		}
		return "";
	}
	return NULL;
}



/* the following routines add waypoints/halts to a schedule
 * because we do not like to stop at AIs stop, but we still want to force the truck to use AI roads
 * So if there is a halt, then it must be either public or ours!
 * @author prissi
 */
static const char *tool_fahrplan_insert_aux(karte_t *welt, player_t *player, koord3d pos, schedule_t *fpl, bool append)
{
	if(fpl == NULL) {
		dbg->warning("tool_fahrplan_insert_aux()","Schedule is (null), doing nothing");
		return 0;
	}
	grund_t *bd = welt->lookup(pos);
	if (bd) {
		// now just for error messages, we're assuming a valid ground
		// check for right way type
		if(!fpl->ist_halt_erlaubt(bd)) {
			return fpl->fehlermeldung();
		}
		// and check for ownership
		if(  !bd->is_halt()  ) {
			weg_t *w = bd->get_weg( fpl->get_waytype() );
			if(  w==NULL  &&  fpl->get_waytype()==tram_wt  ) {
				w = bd->get_weg( track_wt );
			}
			if(  w!=NULL  &&  w->get_owner()!=welt->get_player(1)  &&  !player_t::check_owner(w->get_owner(),player)  ) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
			if(  bd->get_depot()  &&  !player_t::check_owner( bd->get_depot()->get_owner(), player )  ) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
		}
		if(  bd->is_halt()  &&  !player_t::check_owner( player, bd->get_halt()->get_owner()) ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		// ok, now we have a valid ground
		if(append) {
			fpl->append(bd);
		}
		else {
			fpl->insert(bd);
		}
	}
	return NULL;
}

const char *tool_schedule_add_t::work( player_t *player, koord3d pos )
{
	return tool_fahrplan_insert_aux( welt, player, pos, (schedule_t*)const_cast<char *>(default_param), true );
}

const char *tool_schedule_ins_t::work( player_t *player, koord3d pos )
{
	return tool_fahrplan_insert_aux( welt, player, pos, (schedule_t*)const_cast<char *>(default_param), false );
}


/* way construction */
const weg_besch_t *tool_build_way_t::defaults[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const weg_besch_t *tool_build_way_t::get_besch( uint16 timeline_year_month, bool remember ) const
{
	const weg_besch_t *besch = default_param ? wegbauer_t::get_besch(default_param,0) :NULL;
	if(  besch==NULL  &&  default_param  ) {
		waytype_t wt = (waytype_t)atoi(default_param);
		besch = defaults[wt&63];
		if(besch==NULL  ||  !besch->is_available(timeline_year_month)) {
			// search fastest way.
			if(  wt == tram_wt  ||  wt == powerline_wt  ) {
				besch = wegbauer_t::weg_search(wt, 0xffffffff, timeline_year_month, weg_t::type_flat);
			}
			else {
				// this triggers an assertion if wt == powerline_wt
				weg_t *w = weg_t::alloc(wt);
				besch = w->get_besch();
				delete w;
			}
		}
	}
	if(  besch  &&  remember  ) {
		if(  besch->get_styp() == weg_t::type_tram  ) {
			defaults[ tram_wt ] = besch;
		}
		else {
			defaults[besch->get_wtyp()&63] = besch;
		}
	}
	return besch;
}

image_id tool_build_way_t::get_icon(player_t *) const
{
	const weg_besch_t *besch = wegbauer_t::get_besch(default_param,0);
	image_id bild = icon;
	bool is_tram = false;
	if(  besch  ) {
		is_tram = (besch->get_wtyp()==tram_wt) || (besch->get_styp() == weg_t::type_tram);
		if(  bild ==  IMG_LEER  ) {
			bild = besch->get_cursor()->get_bild_nr(1);
		}
		if(  !besch->is_available( world()->get_timeline_year_month() )  ) {
			return IMG_LEER;
		}
	}
	if(  grund_t::underground_mode==grund_t::ugm_all && !is_tram ) {
		return IMG_LEER;
	}
	return bild;
}

const char* tool_build_way_t::get_tooltip(const player_t *) const
{
	const weg_besch_t *besch = get_besch(welt->get_timeline_year_month(),false);
	if (besch == NULL) {
		return "";
	}
	tooltip_with_price_maintenance( welt, besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed() );
	return toolstr;
}

// default ways are not initialized synchronously for different clients
// always return the name of a way, never the string containing the waytype
const char* tool_build_way_t::get_default_param(player_t *player) const
{
	if (player==NULL) {
		return default_param;
	}
	if (besch) {
		return besch->get_name();
	}
	else {
		if (default_param == NULL) {
			// no chance to guess anything sensible
			return NULL;
		}
		const weg_besch_t* test_besch = get_besch(0, false);
		if (test_besch) {
			return test_besch->get_name();
		}
		else {
			return default_param;
		}
	}
}

bool tool_build_way_t::is_selected() const
{
	tool_t const* const tool = welt->get_tool(welt->get_active_player_nr());
	if (tool->get_id() != get_id()) {
		return false;
	}
	tool_build_way_t const* const selected = dynamic_cast<tool_build_way_t const*>(tool);
	return (selected  &&  selected->get_besch(welt->get_timeline_year_month(),false) == get_besch(welt->get_timeline_year_month(),false));
}

bool tool_build_way_t::init( player_t *player )
{
	two_click_tool_t::init( player );
	if( ok_sound == NO_SOUND ) {
		ok_sound = SFX_CASH;
	}

	// now get current besch
	besch = get_besch( welt->get_timeline_year_month(), can_use_gui() );
	if(  besch  &&  besch->get_cursor()->get_bild_nr(0) != IMG_LEER  ) {
		cursor = besch->get_cursor()->get_bild_nr(0);
	}
	if(  besch  &&  !besch->is_available(welt->get_timeline_year_month())  &&  player!=NULL  &&  player!=welt->get_player(1)  ) {
		// non available way => fail
		return false;
	}
	return besch!=NULL;
}

waytype_t tool_build_way_t::get_waytype() const
{
	const weg_besch_t *besch = get_besch( welt->get_timeline_year_month(), false );
	waytype_t wt = besch ? besch->get_wtyp() : invalid_wt;
	if (  wt==track_wt  &&  besch->get_styp()==7  ) {
		wt = tram_wt;
	}
	return wt;
}

uint8 tool_build_way_t::is_valid_pos(  player_t *player, const koord3d &pos, const char *&error, const koord3d & )
{
	error = NULL;
	grund_t *gr=welt->lookup(pos);
	if(  gr  &&  hang_t::ist_wegbar(gr->get_weg_hang())  ) {
		// ignore tunnel tiles (except road tunnel for tram track building ..)
		if(  gr->get_typ() == grund_t::tunnelboden  &&  !gr->ist_karten_boden()  && !(besch->get_wtyp()==track_wt  &&  besch->get_styp()==7  && gr->hat_weg(road_wt)) ) {
			return 0;
		}
		bool const elevated = besch->get_styp() == 1  &&  besch->get_wtyp() != air_wt;
		// ignore water
		if(  besch->get_wtyp() != water_wt  &&  gr->get_typ() == grund_t::wasser  ) {
			if(  !elevated  ||  welt->lookup_hgt( pos.get_2d() ) < welt->get_water_hgt( pos.get_2d() )  ) {
				return 0;
			}
			// here either channel or elevated way over not too deep water
		}
		// elevated ways have to check tile above
		if(  elevated  ) {
			gr = welt->lookup( pos + koord3d( 0, 0, welt->get_settings().get_way_height_clearance() ) );
			if(  gr == NULL  ) {
				return 2;
			}
			if(  gr->get_typ() != grund_t::monorailboden  ) {
				return 0;
			}
		}
		// test if way already exists on the way and if we are allowed to connect
		weg_t *way = gr->get_weg(besch->get_wtyp());
		if(  way  ) {
			// allow to connect to any road
			if(  besch->get_wtyp() == road_wt  ||  besch->get_wtyp() == water_wt  ) {
				return 2;
			}
			error = way->is_deletable(player);
			return error==NULL ? 2 : 0;
		}
		// check for ownership but ignore moving things
		if(player!=NULL) {
			for(uint8 i=0; i<gr->obj_count(); i++) {
				obj_t* dt = gr->obj_bei(i);
				if (!dt->is_moving()  &&  dt->is_deletable(player)!=NULL) {
					error =  dt->is_deletable(player); // "Das Feld gehoert\neinem anderen Spieler\n";
					return 0;
				}
			}
		}
	}
	else {
		return 0;
	}
	return 2;
}

void tool_build_way_t::calc_route( wegbauer_t &bauigel, const koord3d &start, const koord3d &end )
{
	// recalc type of construction
	wegbauer_t::bautyp_t bautyp = (wegbauer_t::bautyp_t)besch->get_wtyp();
	if(besch->get_wtyp()==track_wt  &&  besch->get_styp()==7) {
		bautyp = wegbauer_t::schiene_tram;
	}
	// elevated track?
	if(besch->get_styp()==1  &&  besch->get_wtyp()!=air_wt) {
		bautyp |= wegbauer_t::elevated_flag;
	}

	bauigel.route_fuer(bautyp, besch);
	if(  is_ctrl_pressed()  ) {
		bauigel.set_keep_existing_ways( false );
	}
	else {
		bauigel.set_keep_existing_faster_ways( true );
	}
	if(  is_ctrl_pressed()  ||  (env_t::straight_way_without_control  &&  !env_t::networkmode  &&  !is_scripted())  ) {
		DBG_MESSAGE("tool_build_way_t()", "try straight route");
		bauigel.calc_straight_route(start,end);
	}
	else {
		bauigel.calc_route(start,end);
	}
	DBG_MESSAGE("tool_build_way_t()", "builder found route with %d squares length.", bauigel.get_count());
}

const char *tool_build_way_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(player);
	calc_route( bauigel, start, end );
	if(  bauigel.get_route().get_count()>1  ) {
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);

		return NULL;
	}
	return "";
}

void tool_build_way_t::mark_tiles(  player_t *player, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(player);
	calc_route( bauigel, start, end );

	uint8 offset = (besch->get_styp() == 1  &&  besch->get_wtyp() != air_wt) ? welt->get_settings().get_way_height_clearance() : 0;

	if(  bauigel.get_count()>1  ) {
		// Set tooltip first (no dummygrounds, if bauigel.calc_casts() is called).
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", bauigel.calc_costs() ) );

		// make dummy route from bauigel
		for(  uint32 j=0;  j<bauigel.get_count();  j++   ) {
			koord3d pos = bauigel.get_route()[j] + koord3d(0,0,offset);
			grund_t *gr = welt->lookup( pos );
			if( !gr ) {
				gr = new monorailboden_t(pos, 0);
				// should only be here when elevated/monorail, therefore will be at height offset above ground
				gr->set_grund_hang( welt->lookup( pos - koord3d( 0, 0, offset ) )->get_grund_hang() );
				welt->access(pos.get_2d())->boden_hinzufuegen(gr);
			}
			if (gr->ist_wasser()) {
				continue;
			}
			ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(besch->get_wtyp()) | bauigel.get_route().get_ribi( j );

			zeiger_t *way = new zeiger_t(pos, player);
			if(gr->get_weg_hang()) {
				way->set_bild( besch->get_hang_bild_nr(gr->get_weg_hang(),0) );
			}
			else if(besch->get_wtyp()!=powerline_wt  &&  ribi_t::ist_kurve(zeige)  &&  besch->has_diagonal_bild()) {
				way->set_bild( besch->get_diagonal_bild_nr(zeige,0) );
			}
			else {
				way->set_bild( besch->get_bild_nr(zeige,0) );
			}
			gr->obj_add( way );
			way->set_yoff(-gr->get_weg_yoff() );
			marked.insert( way );
			way->mark_image_dirty( way->get_image(), 0 );
		}
	}
}


/* city road construction */
const weg_besch_t *tool_build_cityroad::get_besch(uint16,bool) const
{
	return welt->get_city_road();
}

const char *tool_build_cityroad::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(player);
	bauigel.set_build_sidewalk(true);
	calc_route( bauigel, start, end );
	if(  bauigel.get_route().get_count()>1  ) {
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);

		return NULL;
	}
	return "";
}


/* bridge construction */
const char* tool_build_bridge_t::get_tooltip(const player_t *) const
{
	const bruecke_besch_t * besch = brueckenbauer_t::get_besch(default_param);
	if (besch == NULL) {
		return "";
	}
	tooltip_with_price_maintenance( welt, besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	if(besch->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
	}
	if(besch->get_max_length()>0) {
		n += sprintf(toolstr+n, ", %dkm", besch->get_max_length());
	}
	return toolstr;
}

waytype_t tool_build_bridge_t::get_waytype() const
{
	const bruecke_besch_t * besch = brueckenbauer_t::get_besch(default_param);
	return besch ? besch->get_waytype() : invalid_wt;
}


bool tool_build_bridge_t::init( player_t *player )
{
	two_click_tool_t::init( player );
	// now get current besch
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	if(  besch  &&  !besch->is_available(welt->get_timeline_year_month())  &&  player!=NULL  &&  player!=welt->get_player(1)  ) {
		return false;
	}
	return besch!=NULL;
}


const char *tool_build_bridge_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	if (end==koord3d::invalid) {
		return brueckenbauer_t::baue( player, start, besch );
	}
	else {
		const koord zv(ribi_typ(end-start));
		sint8 bridge_height;
		const char *error;
		koord3d end2 = brueckenbauer_t::finde_ende(player, start, zv, besch, error, bridge_height, false, koord_distance(start, end), is_ctrl_pressed());
		assert(end2 == end); (void)end2;
		brueckenbauer_t::baue_bruecke( player, start, end, zv, bridge_height, besch, wegbauer_t::weg_search(besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat));
		return NULL; // all checks are performed before building.
	}
}

void tool_build_bridge_t::rdwr_custom_data(memory_rw_t *packet)
{
	two_click_tool_t::rdwr_custom_data(packet);
	uint8 i = ribi;
	packet->rdwr_byte(i);
	ribi = (ribi_t::ribi)i;
}

void tool_build_bridge_t::mark_tiles(  player_t *player, const koord3d &start, const koord3d &end )
{
	const ribi_t::ribi ribi_mark = ribi_typ(end-start);
	const koord zv(ribi_mark);
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	const char *error;
	sint8 bridge_height;
	koord3d end2 = brueckenbauer_t::finde_ende(player, start, zv, besch, error, bridge_height, false, koord_distance(start, end), is_ctrl_pressed());
	assert(end2 == end); (void)end2;

	sint64 costs = 0;
	// start
	grund_t *gr = welt->lookup(start);

	// get initial height of bridge from start tile
	// flat -> height is 1 if conversion factor 1, 2 if conversion factor 2
	// single height -> height is 1
	// double height -> height is 2
	const hang_t::typ slope = gr->get_weg_hang();
	uint8 max_height = slope ?  hang_t::max_diff(slope) : bridge_height;

	zeiger_t *way = new zeiger_t(start, player );
	const bruecke_besch_t::img_t img0 = besch->get_end( slope, slope, hang_typ(zv)*max_height );

	gr->obj_add( way );
	way->set_bild( besch->get_hintergrund( img0, 0 ) );
	way->set_after_bild( besch->get_vordergrund( img0, 0 ) );

	if(  gr->get_grund_hang() != 0  ) {
		way->set_yoff( -TILE_HEIGHT_STEP * max_height );
	}
	// eventually we have to remove trees on start tile
	if (besch->get_waytype() != powerline_wt) {
		for(  uint8 i=0;  i<gr->get_top();  i++  ) {
			obj_t *obj = gr->obj_bei(i);
			switch(obj->get_typ()) {
				case obj_t::baum:
					costs -= welt->get_settings().cst_remove_tree;
					break;
				case obj_t::groundobj:
					costs += ((groundobj_t *)obj)->get_besch()->get_preis();
					break;
				default: break;
			}
		}
	}
	marked.insert( way );
	way->mark_image_dirty( way->get_image(), 0 );
	// loop
	koord3d pos( start + zv + koord3d( 0, 0, max_height ) );
	while (pos.get_2d()!=end.get_2d()) {
		grund_t *gr = welt->lookup( pos );
		if( !gr ) {
			gr = new monorailboden_t(pos, 0);
			gr->set_grund_hang( 0 );
			welt->access(pos.get_2d())->boden_hinzufuegen(gr);
		}
		zeiger_t *way = new zeiger_t(pos, player );
		gr->obj_add( way );
		grund_t *kb = welt->lookup_kartenboden(pos.get_2d());
		sint16 height = pos.z - kb->get_pos().z;
		way->set_bild(besch->get_hintergrund(besch->get_simple(ribi_mark,height-hang_t::max_diff(kb->get_grund_hang())),0));
		way->set_after_bild(besch->get_vordergrund(besch->get_simple(ribi_mark,height-hang_t::max_diff(kb->get_grund_hang())), 0));
		marked.insert( way );
		way->mark_image_dirty( way->get_image(), 0 );
		pos = pos + zv;
	}
	costs += besch->get_preis() * koord_distance(start, pos);
	// end
	gr = welt->lookup(end);

	// get initial height of bridge from start tile
	// flat -> height is 1 if conversion factor 1, 2 if conversion factor 2
	// single height -> height is 1
	// double height -> height is 2
	const hang_t::typ end_slope = gr->get_weg_hang();
	const uint8 end_max_height = end_slope ? ((end_slope & 7) ? 1 : 2) : (pos.z-end.z);

	if(  gr->ist_karten_boden()  &&  end.z + end_max_height == start.z + max_height  ) {
		zeiger_t *way = new zeiger_t(end, player );
		const bruecke_besch_t::img_t img1 = besch->get_end( end_slope, end_slope, end_slope?0:(pos.z-end.z)*hang_typ(-zv) );
		gr->obj_add( way );
		way->set_bild(besch->get_hintergrund(img1, 0));
		way->set_after_bild(besch->get_vordergrund(img1, 0));
		if(  gr->get_grund_hang() != 0  ) {
			way->set_yoff( -TILE_HEIGHT_STEP * end_max_height );
		}
		marked.insert( way );
		way->mark_image_dirty( way->get_image(), 0 );
		costs += besch->get_preis();
	}
	else {
		if (besch->get_waytype() == powerline_wt  ? !gr->find<leitung_t>() : !gr->hat_weg(besch->get_waytype())) {
			const weg_besch_t *weg_besch = wegbauer_t::weg_search(besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat);
			costs += weg_besch->get_preis();
		}
	}
	// eventually we have to remove trees on end tile
	if (besch->get_waytype() != powerline_wt) {
		for(  uint8 i=0;  i<gr->get_top();  i++  ) {
			obj_t *obj = gr->obj_bei(i);
			switch(obj->get_typ()) {
				case obj_t::baum:
					costs -= welt->get_settings().cst_remove_tree;
					break;
				case obj_t::groundobj:
					costs += ((groundobj_t *)obj)->get_besch()->get_preis();
					break;
				default: break;
			}
		}
	}
	win_set_static_tooltip( tooltip_with_price("Building costs estimates", costs ) );
}

uint8 tool_build_bridge_t::is_valid_pos(  player_t *player, const koord3d &pos, const char *&error, const koord3d &start )
{
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	const waytype_t wt = besch->get_waytype();

	error = NULL;
	grund_t *gr = welt->lookup(pos);
	if(  gr==NULL  ||  !hang_t::ist_wegbar(gr->get_grund_hang())  ||  !brueckenbauer_t::ist_ende_ok( player, gr, wt, (is_first_click() ? 0 : ribi_typ(pos-start)) )  ) {
		return 0;
	}

	if(  welt->lookup( pos + koord3d(0, 0, 1))  ||  (welt->get_settings().get_way_height_clearance()==2  &&  welt->lookup( pos + koord3d(0, 0, 2) ))  ) {
		return 0;
	}

	if(  is_first_click()  ) {
		if(  gr->ist_karten_boden()  ) {
			// first click
			ribi_t::ribi rw = ribi_t::keine;
			if (wt==powerline_wt) {
				if (gr->hat_wege()) {
					return 0;
				}
				if (gr->find<leitung_t>()) {
					rw |= gr->find<leitung_t>()->get_ribi();
				}
			}
			else {
				if (gr->find<leitung_t>()) {
					return 0;
				}
				if(wt!=road_wt) {
					// only road bridges can have other ways on it (ie trams)
					if(gr->has_two_ways()  ||  (gr->hat_wege() && gr->get_weg_nr(0)->get_waytype()!=wt) ) {
						return 0;
					}
					if(gr->hat_wege()){
						rw |= gr->get_weg_nr(0)->get_ribi_unmasked();
					}
				}
				else {
					// If road and tram, we have to check both ribis.
					for(int i=0;i<2;i++) {
						const weg_t *w = gr->get_weg_nr(i);
						if (w) {
							if (w->get_waytype()!=road_wt  &&  (w->get_waytype()!=track_wt  ||  w->get_besch()->get_styp()!=tram_wt)) {
								return 0;
							}
							rw |= w->get_ribi_unmasked();
						}
						else break;
					}
				}
			}
			// ribi from slope
			rw |= ribi_typ(gr->get_grund_hang());
			if(  rw!=ribi_t::keine && !ribi_t::ist_einfach(rw)  ) {
				return 0;
			}
			// determine possible directions
			ribi = ribi_t::rueckwaerts(rw);
			return (ribi!=ribi_t::keine ? 2 : 0) | (ribi_t::ist_einfach(ribi) ? 1 : 0);
		}
		else {
			if(  gr->get_weg_hang()  ) {
				return 0;
			}

			if(  gr->get_typ() != grund_t::monorailboden  &&
			     gr->get_typ() != grund_t::tunnelboden  ) {
				return 0;
			}

			if(!gr->get_weg_nr(0)) {
				return 0;
			}

			ribi = ~gr->get_weg_nr(0)->get_ribi_unmasked();

			return 2;
		}
	}
	else {
		// second click

		// dragging in the right direction?
		ribi_t::ribi test = ribi_typ(pos - start);
		if (!ribi_t::ist_einfach(test)  ||  ((test & (~ribi))!=0) ) {
			return 0;
		}

		// check whether we can build a bridge here
		const char *error = NULL;
		sint8 bridge_height;
 		koord3d end = brueckenbauer_t::finde_ende(player, start, koord(test), besch, error, bridge_height, false, koord_distance(start, pos), is_ctrl_pressed());
		if (end!=pos) {
			return 0;
		}
		return 2;
	}
}


/* more difficult, since this builds also underground ways */
const char* tool_build_tunnel_t::get_tooltip(const player_t *) const
{
	const tunnel_besch_t * besch = tunnelbauer_t::get_besch(default_param);
	tooltip_with_price_maintenance( welt, besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	if(besch->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
	}
	return toolstr;
}


waytype_t tool_build_tunnel_t::get_waytype() const
{
	const tunnel_besch_t * besch = tunnelbauer_t::get_besch(default_param);
	return besch ? besch->get_waytype() : invalid_wt;
}


bool tool_build_tunnel_t::init( player_t *player )
{
	two_click_tool_t::init( player );
	// now get current besch
	const tunnel_besch_t * besch = tunnelbauer_t::get_besch(default_param);
	if(  besch  &&  !besch->is_available(welt->get_timeline_year_month())  &&  player!=NULL  &&  player!=welt->get_player(1)  ) {
		return false;
	}
	return besch!=NULL;
}


const char *tool_build_tunnel_t::check_pos( player_t *player, koord3d pos)
{
	if (grund_t::underground_mode == grund_t::ugm_all) {
		return NULL;
	}
	else {
		return two_click_tool_t::check_pos(player, pos);
	}
}

void tool_build_tunnel_t::calc_route( wegbauer_t &bauigel, const koord3d &start, const koord3d &end)
{
	const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
	wegbauer_t::bautyp_t bt = (wegbauer_t::bautyp_t)(besch->get_waytype());

	const weg_besch_t *wb = besch->get_weg_besch();
	if(wb==NULL) {
		// ignore timeline to get consistent results
		wb = wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), 0, weg_t::type_flat );
	}

	bauigel.route_fuer(bt | wegbauer_t::tunnel_flag, wb, besch);
	bauigel.set_keep_existing_faster_ways( !is_ctrl_pressed() );
	// wegbauer (way builder) tries to find route to 3d coordinate if no ground at end exists or is not kartenboden (map ground)
	bauigel.calc_straight_route(start,end);
}

const char *tool_build_tunnel_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	if( end == koord3d::invalid ) {
		// Build tunnel mouths
		if (welt->lookup_kartenboden(start.get_2d())->get_hoehe() == start.z) {
			const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
			return tunnelbauer_t::baue( player, start.get_2d(), besch, !is_ctrl_pressed() );
		}
		else {
			return "";
		}
	}
	else {
		// Build tunnels
		wegbauer_t bauigel(player);
		calc_route( bauigel, start, end );
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);
		welt->lookup_kartenboden(end.get_2d())->clear_flag(grund_t::marked);
		return NULL;
	}
}

uint8 tool_build_tunnel_t::is_valid_pos(  player_t *player, const koord3d &pos, const char *&error, const koord3d & )
{
	if(  !is_first_click()  ) {
		error = NULL;
		// All pos are valid for the second click!
		return 2;
	}
	// search for ground
	// start needs valid tile!
	grund_t *gr = welt->lookup(pos);
	if(  gr  ) {
		if( gr->hat_wege() ) {
			const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
			// use the check_owner routine of wegbauer_t (not spieler_t!), needs an instance
			weg_t *w = gr->get_weg_nr(0);
			if(  w==NULL  ||  w->get_besch()->get_wtyp()!=besch->get_waytype()  ) {
				error = "No suitable ground!";
				return 0;
			}
			wegbauer_t bauigel(player);
			if(!bauigel.check_owner( w->get_owner(), player )) {
				error = "Das Feld gehoert\neinem anderen Spieler\n";
				return 0;
			}
		}
	}
	else {
		error = "No suitable ground!";
		return 0;
	}
	// if starting tile is tunnel .. build underground tracks
	error = NULL;
	if(gr->ist_tunnel()) {
		return 2;
	}
	// .. otherwise build tunnel mouths (and tunnel behind)
	else {
		return 1;
	}
}

void tool_build_tunnel_t::mark_tiles(  player_t *player, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(player);
	calc_route( bauigel, start, end );

	const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
	// now we search a matching way for the tunnels top speed
	const weg_besch_t *wb = besch->get_weg_besch();
	if(wb==NULL) {
		// ignore timeline to get consistent results
		wb = wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), 0, weg_t::type_flat );
	}

	welt->lookup_kartenboden(end.get_2d())->clear_flag(grund_t::marked);

	if(  bauigel.get_count()>1  ) {
		// Set tooltip first (no dummygrounds, if bauigel.calc_casts() is called).
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -bauigel.calc_costs() ) );

		// make dummy route from bauigel
		for(  uint32 j=0;  j<bauigel.get_count();  j++  ) {
			koord3d pos = bauigel.get_route()[j];
			grund_t *gr = welt->lookup(pos);
			if( !gr ) {
				// We need to create a dummy ground.
				gr = new tunnelboden_t(pos, 0);
				welt->access(pos.get_2d())->boden_hinzufuegen(gr);
			}
			ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(wb->get_wtyp()) | bauigel.get_route().get_ribi( j );

			zeiger_t *way = new zeiger_t(pos, player );
			if(gr->get_weg_hang()) {
				way->set_bild( wb->get_hang_bild_nr(gr->get_weg_hang(),0) );
			}
			else if(wb->get_wtyp()!=powerline_wt  &&  ribi_t::ist_kurve(zeige)  &&  wb->has_diagonal_bild()) {
				way->set_bild( wb->get_diagonal_bild_nr(zeige,0) );
			}
			else {
				way->set_bild( wb->get_bild_nr(zeige,0) );
			}
			gr->obj_add( way );
			marked.insert( way );
			way->mark_image_dirty( way->get_image(), 0 );
		}
		welt->lookup(end)->set_flag(grund_t::marked);
	}
}


/* removes a way like a driving car ... */
char const* tool_wayremover_t::get_tooltip(player_t const*) const
{
	switch(atoi(default_param)) {
		case road_wt: return translator::translate("remove roads");
		case tram_wt:
		case track_wt: return translator::translate("remove tracks");
		case maglev_wt: return translator::translate("remove maglev tracks");
		case narrowgauge_wt: return translator::translate("remove narrowgauge tracks");
		case monorail_wt: return translator::translate("remove monorails");
		case water_wt: return translator::translate("remove channels");
		case air_wt: return translator::translate("remove airstrips");
		case powerline_wt: return translator::translate("remove powerlines");
	}
	return NULL;
}

image_id tool_wayremover_t::get_icon(player_t *) const
{
	if(  default_param  &&  wegbauer_t::waytype_available( (waytype_t)atoi(default_param), welt->get_timeline_year_month() )  ) {
		return icon;
	}
	return IMG_LEER;
}

waytype_t tool_wayremover_t::get_waytype() const
{
	return default_param ? (waytype_t)atoi(default_param) : invalid_wt;
}

class electron_t : public test_driver_t {
	bool check_next_tile(const grund_t* gr) const { return gr->get_leitung()!=NULL; }
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_leitung()->get_ribi(); }
	virtual waytype_t get_waytype() const { return invalid_wt; }
	virtual int get_cost(const grund_t *, const sint32, koord) const { return 1; }
	virtual bool is_target(const grund_t *,const grund_t *) const { return false; }
};

class scenario_checker_t : public test_driver_t {
public:
	test_driver_t *other;
	scenario_t *scenario;
	uint16 id;
	player_t *player;
	~scenario_checker_t() { delete other; }

	/**
	 * checks for active scenario,
	 * @returns scenario_checker_t if scenario active, the supplied test_driver otherwise
	 */
	static test_driver_t* apply(test_driver_t *test_driver, player_t *player, tool_t *tool) {
		karte_t *welt = world();
		if (is_scenario()) {
			scenario_checker_t *td2 = new scenario_checker_t();
			td2->other = test_driver;
			td2->scenario = welt->get_scenario();
			td2->id = tool->get_id();
			td2->player = player;
			return td2;
		}
		return test_driver;
	}
private:
	bool check_next_tile(const grund_t* gr) const { return other->check_next_tile(gr)  &&  scenario->is_work_allowed_here(player, id, other->get_waytype(), gr->get_pos())==NULL;}
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return other->get_ribi(gr); }
	virtual waytype_t get_waytype() const { return other->get_waytype(); }
	virtual int get_cost(const grund_t *gr, const sint32 c, koord p) const { return other->get_cost(gr,c,p); }
	virtual bool is_target(const grund_t *gr,const grund_t *gr2) const { return other->is_target(gr,gr2); }
};

void tool_wayremover_t::mark_tiles( player_t *player, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, player, start, end );
	if( can_built ) {
		FOR(vector_tpl<koord3d>, const& pos, verbindung.get_route()) {
			zeiger_t *marker = new zeiger_t(pos, NULL );
			marker->set_bild( cursor );
			marker->mark_image_dirty( marker->get_image(), 0 );
			marked.insert( marker );
			welt->lookup(pos)->obj_add( marker );
		}
	}
}

uint8 tool_wayremover_t::is_valid_pos( player_t *player, const koord3d &pos, const char *&error, const koord3d & )
{
	// search for starting ground
	waytype_t wt = get_waytype();
	grund_t *gr=tool_intern_koord_to_weg_grund(player,welt,pos,wt);
	if(gr==NULL) {
		DBG_MESSAGE("tool_wayremover_t()", "no ground on %i,%i",pos.x, pos.y);
		// wrong ground or not this way here => exit
		return 0;
	}
	// do not remove ground from depot
	if(gr->get_depot()) {
		error = "No suitable ground!";
		return 0;
	}
	if(is_scenario()) {
		error = welt->get_scenario()->is_work_allowed_here(player, get_id(), wt, pos);
		if (error) {
			dbg->warning("tool_wayremover_t::is_within_limits()", error);
			return 0;
		}
	}
	error = NULL;
	return 2;
}

bool tool_wayremover_t::calc_route( route_t &verbindung, player_t *player, const koord3d &start, const koord3d &end )
{
	waytype_t wt = get_waytype();
	if (wt == tram_wt) {
		wt = track_wt;
	}

	if(  start == end  ) {
		verbindung.clear();
		grund_t *gr=welt->lookup(start);
		if(  gr  &&  (wt!=powerline_wt ? gr->get_weg(wt)!=NULL : gr->get_leitung()!=NULL) ) {
			verbindung.append( start );
		}
	}
	else {
		// get a default vehikel
		test_driver_t* test_driver;
		if(  wt!=powerline_wt  ) {
			vehikel_besch_t remover_besch(wt, 500, vehikel_besch_t::diesel );
			vehicle_t *driver = vehikelbauer_t::baue(start, player, NULL, &remover_besch);
			driver->set_flag( obj_t::not_on_map );
			test_driver = driver;
		}
		else {
			test_driver = new electron_t();
		}
		test_driver = scenario_checker_t::apply(test_driver, player, this);

		verbindung.calc_route(welt, start, end, test_driver, 0, 0);
		delete test_driver;
	}
	DBG_MESSAGE("tool_wayremover_t()","route with %d tile found",verbindung.get_count());

	calc_route_error = NULL;
	bool can_delete = start == end  ||  verbindung.get_count()>1;
	if(  can_delete  ) {
		// found a route => check if I can delete anything on it
		FOR(koord3d_vector_t, const& i, verbindung.get_route()) {
			if (!can_delete) break;
			grund_t const* const gr = welt->lookup(i);
			if(  wt!=powerline_wt  ) {
				// no way found
				if(  gr==NULL  ||  gr->get_weg(wt)==NULL  ) {
					can_delete = false;
					break;
				}
				// check all if we want to delete the first on a no-ground tile
				bool check_all = !gr->ist_karten_boden()  &&  gr->has_two_ways()  &&  gr->get_weg_nr(0)->get_waytype()==wt;
				// we have to do a fine check
				for( uint i=0;  i<gr->get_top()  &&  can_delete;  i++  ) {
					obj_t *obj = gr->obj_bei(i);
					const uint8 type = obj->get_typ();
					// ignore pillars, powerlines
					if (type == obj_t::pillar  ||  type==obj_t::leitung) {
						continue;
					}
					// ignore flying aircraft
					if (type == obj_t::air_vehicle  &&  !(static_cast<air_vehicle_t*>(obj)->is_on_ground())) {
						continue;
					}
					const waytype_t obj_wt = obj->get_waytype();
					// way-related things
					if (obj_wt != invalid_wt) {
						// check this thing if it has the same waytype or if we want to remove the whole bridge/tunnel tile
						// special case: stations - take care not to produce station without any way
						const bool lonely_station = type==obj_t::gebaeude  &&  !gr->has_two_ways();
						if (check_all ||  obj_wt == wt  ||  lonely_station) {
							can_delete = (calc_route_error = obj->is_deletable(player)) == NULL;
						}
					}
					// all other stuff
					else {
						can_delete = (calc_route_error = obj->is_deletable(player)) == NULL;
					}
				}
			}
			else {
				// for powerline: only a ground and a powerline to remove
				if(  gr==NULL  ||  gr->get_leitung()==NULL  ||  (calc_route_error = gr->get_leitung()->is_deletable(player))!=NULL  ) {
					can_delete = false;
					break;
				}
			}
		}
	}
	DBG_MESSAGE("tool_wayremover_t()", "route search returned %d", can_delete);

	return can_delete;
}

const char *tool_wayremover_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	waytype_t wt = get_waytype();
	route_t verbindung;
	if( !calc_route( verbindung, player, start, end )  ) {
		DBG_MESSAGE("tool_wayremover_t()","no route found");
		if (calc_route_error  &&  *calc_route_error) {
			return calc_route_error;
		}
		else {
			return "Ways not connected";
		}
	}
	bool can_delete = true;	// assume success

	// if successful => delete everything
	for( uint32 i=0;  i<verbindung.get_count();  i++  ) {

		grund_t *gr=welt->lookup(verbindung.position_bei(i));

		// ground can be missing after deleting a bridge ...
		if(gr  &&  !gr->ist_wasser()) {

			if(gr->ist_bruecke()) {
				if(gr->find<bruecke_t>()->get_besch()->get_waytype()==wt) {
					if(  brueckenbauer_t::is_start_of_bridge(gr)  ) {
						const char *err = NULL;
						err = brueckenbauer_t::remove(player,verbindung.position_bei(i),wt);
						if(err) {
							return err;
						}
						gr = welt->lookup(verbindung.position_bei(i));
						if(  !gr  ) {
							// happens with bridges without ramps
							continue;
						}
					}
					else {
						// do not remove asphalt from a bridge ...
						continue;
					}
				}
			}

			// now the tricky part: delete just part of a way (or everything, if possible)
			// calculate remaining directions
			ribi_t::ribi rem = 15 ^ ( verbindung.get_route().get_ribi(i) );
			// if start=end tile then delete every direction
			if(  verbindung.get_count() <= 1  ) {
				rem = 0;
			}

			if(  wt!=powerline_wt  ) {
				if(!gr->get_flag(grund_t::is_kartenboden)  &&  (gr->get_typ()==grund_t::tunnelboden  ||  gr->get_typ()==grund_t::monorailboden)  &&  gr->get_weg_nr(0)->get_waytype()==wt) {
					can_delete &= gr->remove_everything_from_way(player,wt,rem);
					if(can_delete  &&  gr->get_weg(wt)==NULL) {
						if(gr->get_weg_nr(0)!=0) {
							gr->remove_everything_from_way(player,gr->get_weg_nr(0)->get_waytype(),ribi_t::keine);
						}
						gr->obj_loesche_alle(player);
						gr->mark_image_dirty();
						if (gr->is_visible() && gr->get_typ()==grund_t::tunnelboden && i>0) { // visibility test does not influence execution
							grund_t *bd = welt->access(verbindung.position_bei(i-1).get_2d())->get_kartenboden();
							bd->calc_bild();
							bd->set_flag(grund_t::dirty);
						}
						// delete tunnel ground too, if empty
						welt->access(gr->get_pos().get_2d())->boden_entfernen(gr);
						delete gr;
					}
				}
				else {
					can_delete &= gr->remove_everything_from_way(player,wt,rem);
				}
			}
			else {
				leitung_t *lt = gr->get_leitung();
				if(  lt  &&  (rem&lt->get_ribi())==0  ) {
					// remove only single connections
					lt->cleanup(player);
					delete lt;
					// delete tunnel ground too, if empty
					if (gr->get_typ()==grund_t::tunnelboden) {
						gr->obj_loesche_alle(player);
						gr->mark_image_dirty();
						if (!gr->get_flag(grund_t::is_kartenboden)) {
							welt->access(gr->get_pos().get_2d())->boden_entfernen(gr);
							delete gr;
						}
						else {
							grund_t *gr_new = new boden_t(gr->get_pos(), gr->get_grund_hang());
							welt->access(gr->get_pos().get_2d())->boden_ersetzen(gr, gr_new);
							gr_new->calc_bild();
						}
					}
				}
				// otherwise it is a crossing ...
			}
		}
		// ok, now everything removed ...
	}

	// return success
	return can_delete ? NULL : "";
}



/* add catenary during construction */
const way_obj_besch_t *tool_build_wayobj_t::default_electric = NULL;

const char* tool_build_wayobj_t::get_tooltip(const player_t *) const
{
	if(  build  ) {
		const way_obj_besch_t *besch = get_besch();
		if(besch) {
			tooltip_with_price_maintenance( welt, besch->get_name(), -besch->get_preis(), besch->get_wartung() );
			size_t n= strlen(toolstr);
			if (besch->get_own_wtyp()==overheadlines_wt) {
				// only overheadlines impose topspeed
				sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
			}
			return toolstr;
		}
		return NULL;
	}
	else {
		waytype_t wt = (waytype_t)atoi( default_param );
		sprintf( toolstr, translator::translate("Remove wayobj %s"), translator::translate(weg_t::waytype_to_string(wt)) );
		return toolstr;
	}
}

const way_obj_besch_t *tool_build_wayobj_t::get_besch() const
{
	const way_obj_besch_t *besch = default_param ? wayobj_t::find_besch(default_param) : NULL;
	if(besch==NULL) {
		besch = default_electric;
		if(besch==NULL) {
			besch = wayobj_t::wayobj_search( track_wt, overheadlines_wt, welt->get_timeline_year_month() );
		}
	}
	return besch;
}

waytype_t tool_build_wayobj_t::get_waytype() const
{
	if(  build  ) {
		const way_obj_besch_t *besch = get_besch();
		return besch ? besch->get_wtyp() : invalid_wt;
	}
	else {
		return default_param ? (waytype_t)atoi( default_param ) : invalid_wt;
	}
}

bool tool_build_wayobj_t::is_selected() const
{
	const tool_build_wayobj_t *selected = dynamic_cast<const tool_build_wayobj_t *>(welt->get_tool(welt->get_active_player_nr()));
	return (selected  &&  selected->build==build  &&  selected->get_besch() == get_besch());
}

bool tool_build_wayobj_t::init( player_t *player )
{
	two_click_tool_t::init( player );

	if( build ) {
		besch = get_besch();
		if( besch ) {
			cursor = besch->get_cursor()->get_bild_nr(0);
			wt = besch->get_wtyp();
			default_electric = besch;
		}
		return besch!=NULL;
	}
	else {
		besch = NULL;
		wt = (waytype_t)atoi( default_param );
		return wt != 0;
	}
}

bool tool_build_wayobj_t::calc_route( route_t &verbindung, player_t *player, const koord3d& start, const koord3d& to )
{
	// get a default vehikel
	vehikel_besch_t remover_besch( wt, 500, vehikel_besch_t::diesel );
	vehicle_t* test_vehicle = vehikelbauer_t::baue(start, player, NULL, &remover_besch);
	test_vehicle->set_flag( obj_t::not_on_map );
	test_driver_t* test_driver = scenario_checker_t::apply(test_vehicle, player, this);

	bool can_built;
	if( start != to ) {
		can_built = verbindung.calc_route(welt, start, to, test_driver, 0, 0);
	}
	else {
		verbindung.clear();
		verbindung.append( start );
		can_built = true;
	}
	delete test_driver;
	return can_built;
}

uint8 tool_build_wayobj_t::is_valid_pos( player_t* player, const koord3d& pos, const char *&error, const koord3d & )
{
	// search for starting ground
	grund_t *gr=tool_intern_koord_to_weg_grund(player, welt, pos, wt );
	if(  gr == NULL  ) {
		DBG_MESSAGE("tool_build_wayobj_t::is_within_limits()", "no ground on %s",pos.get_str());
		// wrong ground or not this way here => exit
		return 0;
	}
	error = NULL;
	return 2;
}

void tool_build_wayobj_t::mark_tiles( player_t* player, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, player, start, end );
	if( can_built ) {
		sint32 cost_estimate = 0;

		for( uint32 j = 0; j < verbindung.get_count(); j++ ) {
			koord3d pos = verbindung.position_bei(j);
			grund_t *gr = welt->lookup(pos);

			ribi_t::ribi show = verbindung.get_route().get_ribi(j);
			// Search a matching catenary on gr.
			wayobj_t *wayobj = gr->get_wayobj( wt );
			if( build ) {
				cost_estimate += besch->get_preis();
				if( wayobj ) {
					show = show | wayobj->get_dir();
					// Already a catenary here -> costs only, if new catenary is faster
					if(  wayobj->get_besch()->get_topspeed() >= besch->get_topspeed()  ) {
						cost_estimate -= besch->get_preis();
					}
				}
			}
			else if( wayobj ) {
				cost_estimate += wayobj->get_besch()->get_preis();
			}

			zeiger_t *way_obj = NULL;
			if( build ) {
				way_obj = new zeiger_t(pos, player );
				if(  gr->get_weg_hang()  ) {
					way_obj->set_after_bild( besch->get_front_slope_image_id(gr->get_weg_hang()) );
					way_obj->set_bild( besch->get_back_slope_image_id(gr->get_weg_hang()) );
				}
				else if(  ribi_t::ist_kurve(show)  &&  besch->has_diagonal_bild()  ) {
					way_obj->set_after_bild( besch->get_front_diagonal_image_id(show) );
					way_obj->set_bild( besch->get_back_diagonal_image_id(show) );
				}
				else {
					way_obj->set_after_bild( besch->get_front_image_id(show) );
					way_obj->set_bild( besch->get_back_image_id(show) );
				}
			}
			else {
				if( gr->get_wayobj( wt ) ) {
					way_obj = new zeiger_t(pos, player );
					way_obj->set_bild( cursor ); //skinverwaltung_t::bauigelsymbol->get_bild_nr(0));
				}
			}
			if( way_obj ) {
				way_obj->mark_image_dirty( way_obj->get_image(), 0 );
				gr->obj_add( way_obj );
				marked.insert( way_obj );
			}
		}
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -cost_estimate ) );
	}
}

const char *tool_build_wayobj_t::do_work( player_t* player, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, player, start, end );
	DBG_MESSAGE("tool_build_wayobj_t::work()","route search returned %d",can_built);

	if(!can_built) {
		return "Ways not connected";
	}

	// built wayobj ...
	koord3d_vector_t const& r = verbindung.get_route();
	for(uint32 i=0;  i<verbindung.get_count();  i++  ) {
		if( build ) {
			wayobj_t::extend_wayobj_t(r[i], player, r.get_ribi(i), besch);
		}
		else {
			if (wayobj_t* const wo = welt->lookup(r[i])->find<wayobj_t>()) {
				const char *err = wo->is_deletable( player );
				if( !err ) {
					wo->cleanup( player );
					delete wo;
				}
			}
		}
	}

	// Update depots (new electric tab?). Depots can only be on first and last tile.
	for(  uint8 j = 0;  j < 2;  j++  ) {
		uint8 i = j==0 ? 0 : verbindung.get_count()-1;
		depot_t *dep = welt->lookup( verbindung.position_bei(i) )->get_depot();
		if( dep ) {
			dep->update_win();
		}
	}

	return NULL;
}


/* build all kind of station extension buildings */
const char *tool_build_station_t::tool_station_building_aux(player_t *player, bool extend_public_halt, koord3d pos, const haus_besch_t *besch, sint8 rotation )
{
	koord k = pos.get_2d();

	// need kartenboden (map ground)
	if (welt->lookup_kartenboden(k)->get_hoehe() != pos.z) {
		return "";
	}
DBG_MESSAGE("tool_station_building_aux()", "building mail office/station building on square %d,%d", k.x, k.y);

	// Player pays for the construction
	// but we try to extend stations of Player new_owner that may be the public player
	player_t *new_owner = extend_public_halt ? welt->get_player(1) : player;

	koord offsets;
	halthandle_t halt;
	const char *msg = "Tile not empty.";

	if(  rotation==-1  ) {
		//no predefined rotation

		int best_halt = 0;
		int any_halt = 0;

		// find valid rotations (since halt extensions are symmetric, we need to check only two)
		bool any_ok = false;
		for( int r=0;  r<2;  r++  ) {
			koord testsize = besch->get_groesse(r);
			for(  sint8 j=3;  j>=0;  j-- ) {
				bool ok = true;
				koord offset(((j&1)^1)*(testsize.x-1),((j>>1)&1)*(testsize.y-1));
				if(welt->square_is_free(k-offset, testsize.x, testsize.y, NULL, besch->get_allowed_climate_bits())) {
					// first we must check over/under halt
					halthandle_t last_halt;
					for(  sint16 x=0;  x<testsize.x;  x++  ) {
						for(  sint16 y=0;  y<testsize.y;  y++  ) {
							const planquadrat_t *pl = welt->access( k-offset+koord(x,y) );
							if (pl) {
								for(  uint8 i=0;  i < pl->get_boden_count();  i++  ) {
									halthandle_t test_halt = pl->get_boden_bei(i)->get_halt();
									if(test_halt.is_bound()) {
										if(!player_t::check_owner( new_owner, test_halt->get_owner())) {
											// there is another player's halt
											ok = false;
											msg = "Das Feld gehoert\neinem anderen Spieler\n";
										}
										else if(!last_halt.is_bound()) {
											last_halt = test_halt;
										}
										else if(last_halt != test_halt) {
											// there are several halts
											ok = false;
											msg = "Several halts found.";
										}
									}
								}
							}
						}
					}
					if(!ok) {
						continue;
					}
					// well, at least this is theoretical possible here
					any_ok = true;
					if(rotation==-1) {
						// we can build it. reserve this one
						// This needs to build a building at under/over a halt.
						rotation = r;
						offsets = offset;
					}
					koord test_start = k-offset;

					// find all surrounding tiles with a stop
					// for following section of code arrays are arranged such that
					// 0 - facing north
					// 1 - facing west
					// 2 - facing south
					// 3 - facing east
					int neighbour_halts[4] = { 0, 0, 0, 0 };
					int best_halts[4] = { 0, 0, 0, 0 };

					// test also diagonal corners (that is why from -1 to size!)
					for(  sint16 y=-1;  y<=testsize.y;  y++  ) {
						for(  sint16 x=-1;  x<=testsize.x;  (x==-1 && y>-1 && y<testsize.y)?x=testsize.x:x++  ) {
							const planquadrat_t *pl = welt->access( test_start+koord(x,y) );
							if(  pl  ) {
								for(  uint b=0;  b < pl->get_boden_count();  b++  ) {
									grund_t *gr = pl->get_boden_bei(b);
									if(  gr->is_halt()  &&  gr->get_halt().is_bound() &&  new_owner == gr->get_halt()->get_owner()  ) {
										halt = gr->get_halt();
										gebaeude_t *gb = gr->find<gebaeude_t>();
										bool best = gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra();

										// north
										if(  y==-1  ) {
											neighbour_halts[0] ++;
											if(  best  ) {
												best_halts[0] ++;
											}
										}

										// west
										if(  x==-1  ) {
											neighbour_halts[1] ++;
											if(  best  ) {
												best_halts[1] ++;
											}
										}

										// south
										if(  y==testsize.y  ) {
											neighbour_halts[2] ++;
											if(  best  ) {
												best_halts[2] ++;
											}
										}

										// east
										if(  x==testsize.x  ) {
											neighbour_halts[3] ++;
											if(  best  ) {
												best_halts[3] ++;
											}
										}
									}
								}
							}
						}
					}

					// now find out, if this offset/rotation is better ... (i.e. matches more fitting buildings)
					// for r=0 we check north and south, for r=1 we check east and west
					for(  int i=r;  i<4;  i+=2  ) {
						if(  best_halts[i]>best_halt  ||  (best_halt==0  &&  neighbour_halts[i]>any_halt)  ) {
							best_halt = best_halts[i];
							any_halt = neighbour_halts[i];
							rotation = i;
							offsets = offset;
						}
					}
				}
			}
		}

		// no suitable ground here ...
		if(  !any_ok  ) {
			return msg;
		}
		// check over/under halt again
		for(  sint16 x=0;  x<besch->get_b(rotation);  x++  ) {
			for(  sint16 y=0;  y<besch->get_h(rotation);  y++  ) {
				const planquadrat_t *pl = welt->access( k-offsets+koord(x,y) );
				for(  uint8 i=0;  i < pl->get_boden_count();  i++  ) {
					halthandle_t test_halt = pl->get_boden_bei(i)->get_halt();
					if( test_halt.is_bound()  &&  player_t::check_owner( new_owner, test_halt->get_owner()) ) {
						halt = test_halt;
						break;
					}
				}
			}
		}
		// is there no halt to connect?
		if(  !halt.is_bound()  ) {
			return "Post muss neben\nHaltestelle\nliegen!\n";
		}
	}
	else {
		// rotation was pre-selected; just search for stop now
		assert(  rotation < besch->get_all_layouts()  );
		koord testsize = besch->get_groesse(rotation);
		offsets = koord(0,0);

		if(  !welt->square_is_free(k, testsize.x, testsize.y, NULL, besch->get_allowed_climate_bits())  ) {
			return "Tile not empty.";
		}
		// check over/under halt
		for(  sint16 x=0;  x<testsize.x;  x++  ) {
			for(  sint16 y=0;  y<testsize.y;  y++  ) {
				const planquadrat_t *pl = welt->access(k+koord(x,y));
				for(  uint8 i=0;  i < pl->get_boden_count();  i++  ) {
					halthandle_t test_halt = pl->get_boden_bei(i)->get_halt();
					if(test_halt.is_bound()) {
						if(!player_t::check_owner( new_owner, test_halt->get_owner())) {
							return "Das Feld gehoert\neinem anderen Spieler\n";
						}
						else if(!halt.is_bound()) {
							halt = test_halt;
						}
						else if(halt != test_halt) {
							 return "Several halts found.";
						}
					}
				}
			}
		}
		if(!halt.is_bound()) {
			halt = suche_nahe_haltestelle(new_owner, welt, welt->lookup_kartenboden(k)->get_pos(), besch->get_b(rotation), besch->get_h(rotation) );
			// is there no halt to connect?
			if(  !halt.is_bound()  ) {
				return "Post muss neben\nHaltestelle\nliegen!\n";
			}
		}
	}

	if(  rotation>besch->get_all_layouts()  ) {
		rotation %= besch->get_all_layouts();
	}

	hausbauer_t::baue(halt->get_owner(), pos-offsets, rotation, besch, &halt);

	sint32     const  factor = besch->get_b() * besch->get_h();
	sint64     cost = -besch->get_price(welt) * factor;

	if(  player!=halt->get_owner()  &&  halt->get_owner()==welt->get_player(1)  ) {
		// public stops are expensive!
		cost -= (besch->get_maintenance(welt) * factor * 60);
	}
	// difficult to distinguish correctly most suitable waytype
	player_t::book_construction_costs(player,  cost, k, besch->get_finance_waytype());
	halt->recalc_station_type();

	return NULL;
}

/* build a dock either small or large */
const char *tool_build_station_t::tool_station_dock_aux(player_t *player, koord3d pos, const haus_besch_t *besch)
{
	// the cursor cannot be outside the map from here on
	koord k = pos.get_2d();
	grund_t *gr = welt->lookup_kartenboden(k);
	if (gr->get_hoehe()!= pos.z) {
		return "";
	}
	hang_t::typ hang = gr->get_grund_hang();
	// first get the size
	int len = besch->get_groesse().y-1;
	koord dx = koord((hang_t::typ)hang);
	koord last_k = k - dx*len;
	halthandle_t halt;

	// check, if we can build here ...
	if(!hang_t::ist_einfach(hang)) {
		return "Dock must be built on single slope!";
	}
	else {
		for(int i=0;  i<=len;  i++  ) {
			if(!welt->is_within_limits(k-dx*i)) {
				// need at least a single tile to navigate ...
				return "Zu nah am Kartenrand";
			}
			// search for nearby stops
			const planquadrat_t* pl = welt->access(k-dx*i);
			for(  uint8 j=0;  j < pl->get_boden_count();  j++  ) {
				halthandle_t test_halt = pl->get_boden_bei(j)->get_halt();
				if(test_halt.is_bound()) {
					if(!player_t::check_owner( player, test_halt->get_owner())) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					else if(!halt.is_bound()) {
						halt = test_halt;
					}
					else if(halt != test_halt) {
						return "Several halts found.";
					}
				}
			}

			// check whether we can build something
			const grund_t *gr=welt->lookup_kartenboden(k-dx*i);
			if (gr->get_hoehe() != pos.z) {
				return "No suitable ground!";
				break;
			}
			if (const char *msg = gr->kann_alle_obj_entfernen(player)) {
				return msg;
			}

			if (i==0) {
				// start tile on slope near water
				if(gr->hat_wege()  ||  gr->get_typ()!=grund_t::boden  ||  gr->is_halt()) {
					return "Tile not empty.";
				}
			}
			else {
				// all other tiles in water
				if (!gr->ist_wasser()  ||  gr->find<gebaeude_t>()  ||  gr->get_depot()  ||  gr->is_halt()) {
					return "Tile not empty.";
				}
			}
		}
	}

	// remove everything from tile
	gr->obj_loesche_alle(player);

	int layout = 0;
	koord3d bau_pos = welt->lookup_kartenboden(k)->get_pos();
	koord dx2;
	switch(hang) {
		case hang_t::sued:
		case hang_t::sued*2:
			layout = 0;
			dx2 = koord::west;
			break;
		case hang_t::ost:
		case hang_t::ost*2:
			layout = 1;
			dx2 = koord::nord;
			break;
		case hang_t::nord:
		case hang_t::nord*2:
			layout = 2;
			dx2 = koord::west;
			bau_pos = welt->lookup_kartenboden(last_k)->get_pos();
			break;
		case hang_t::west:
		case hang_t::west*2:
			layout = 3;
			dx2 = koord::nord;
			bau_pos = welt->lookup_kartenboden(last_k)->get_pos();
			break;
	}

	// handle 16 layouts
	bool change_layout = false;
	if(besch->get_all_layouts()==16) {
		if(  layout<2  ) {
			layout = 15-layout;
		}
		else {
			layout = 9-layout;
		}
		change_layout = true;
	}

	// oriented buildings here - get neighbouring layouts
	gr = welt->lookup_kartenboden(k+dx2);

	// find out if middle end or start tile
	if(gr  &&  gr->is_halt()  &&  player_t::check_owner( player, gr->get_halt()->get_owner() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  (gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::dock  ||  gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::flat_dock)  ) {
			if(change_layout) {
				layout -= 4;
			}
			if(gb->get_tile()->get_besch()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x02) {
					gb->set_tile( gb->get_tile()->get_besch()->get_tile(ly&0xFD,0,0), false );
				}
			}
		}
	}

	gr = welt->lookup_kartenboden(k-dx2);
	if(gr  &&  gr->is_halt()  &&  player_t::check_owner( player, gr->get_halt()->get_owner() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  (gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::dock  ||  gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::flat_dock)  ) {
			if(change_layout) {
				layout -= 2;
			}
			if(gb->get_tile()->get_besch()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x04) {
					gb->set_tile( gb->get_tile()->get_besch()->get_tile(ly&0xFB,0,0), false );
				}
			}
		}
	}

	if(!halt.is_bound()) {
		halt = suche_nahe_haltestelle(player, welt, welt->lookup_kartenboden(k)->get_pos() );
	}
	bool neu = !halt.is_bound();

	if(neu) {
		if( gr && gr->get_halt().is_bound()  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		// ok, really new stop on this tile then
		halt = haltestelle_t::create(k, player);
	}
	hausbauer_t::baue(halt->get_owner(), bau_pos, layout, besch, &halt);
	sint64 costs = -besch->get_price(welt);
	if(  player!=halt->get_owner() && player != welt->get_player(1)  ) {
		// public stops are expensive!
		// (Except for the public player itself)
		costs -= (besch->get_maintenance(welt) * 60);
	}
	for(  int i=0;  i<=len;  i++  ) {
		koord p=k-dx*i;
		player_t::book_construction_costs(player,  costs, p, water_wt);
	}

	halt->recalc_station_type();
	if(  env_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==k  ) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	if(neu) {
		char* const name = halt->create_name(k, "Dock");
		halt->set_name( name );
		free(name);
	}
	return NULL;
}

/* build a dock either small or large */
const char *tool_build_station_t::tool_station_flat_dock_aux(player_t *player, koord3d pos, const haus_besch_t *besch, sint8 rotation)
{
	// the cursor cannot be outside the map from here on
	koord k = pos.get_2d();
	grund_t *gr = welt->lookup_kartenboden(k);
	if (gr->get_hoehe()!= pos.z) {
		return "";
	}
	// first get the size
	int len = besch->get_groesse().y-1;

	// check, if we can build here ...
	if(  !gr->ist_natur()  ||  gr->get_grund_hang() != hang_t::flach  ) {
		return "No suitable ground!";
	}

	// now find the direction
	// first: find the next water
	ribi_t::ribi water_dir = 0;
	uint8        total_dir = 0;
	for(  uint8 i=0;  i<4;  i++  ) {
		if(  grund_t *gr = welt->lookup_kartenboden(k+koord::nsow[i])  ) {
			if(  gr->ist_wasser()  &&  gr->get_hoehe() == pos.z) {
				water_dir |= ribi_t::nsow[i];
				total_dir ++;
			}
		}
	}

	// not surrounded by water => fail
	if(  total_dir == 0  ) {
		return "No suitable ground!";
	}

	// prefer layouts that reach an existing halt
	ribi_t::ribi halt_dir = 0;
	halthandle_t test_halt[4];

	for(  uint8 ii=0;  ii<4;  ii++  ) {

		if(  (water_dir & ribi_t::nsow[ii]) == 0  ) {
			continue;
		}
		const koord dx = koord::nsow[ii];
		const char *last_error = NULL;

		for(int i=0;  i<=len;  i++  ) {

			// check whether we can build something
			const grund_t *gr = welt->lookup_kartenboden(k+dx*i);
			if( !gr ) {
				// need at least a single tile to navigate ...
				last_error = "Zu nah am Kartenrand";
				break;
			}

			if (gr->get_hoehe() != pos.z) {
				return "No suitable ground!";
				break;
			}

			// search for nearby stops
			const planquadrat_t* pl = welt->access(k+dx*i);
			for(  uint8 j=0;  j < pl->get_boden_count()  &&  !test_halt[ii].is_bound();  j++  ) {
				halthandle_t halt = pl->get_boden_bei(j)->get_halt();
				if (halt.is_bound()  &&  player_t::check_owner( player, halt->get_owner()) ) {
					test_halt[ii] = halt;
					halt_dir |= ribi_t::nsow[ii];
				}
			}

			if(  (last_error = gr->kann_alle_obj_entfernen(player))  ) {
				break;
			}

			if (i>0) {
				// all other tiles in water
				if (!gr->ist_wasser()  ||  gr->find<gebaeude_t>()  ||  gr->get_depot()  ||  gr->is_halt()) {
					last_error = "Tile not empty.";
				}
			}
		}

		// error: then remove this direction
		if(  last_error  ) {
			water_dir &= ~ribi_t::nsow[ii];
			if(  --total_dir == 0  ) {
				// no duitable directions found
				return last_error;
			}
		}
	}

	// now we may have more than one dir left
	if (rotation == -1  &&  total_dir > 1  &&  !ribi_t::ist_einfach(water_dir & halt_dir) ) {
		return "More than one possibility to build this dock found.";
	}

	// remove everything from tile
	gr->obj_loesche_alle(player);

	koord3d bau_pos = welt->lookup_kartenboden(k)->get_pos();
	koord dx = koord::invalid;
	koord last_k;
	uint8 layout = 0; // building orientation
	halthandle_t halt;

	for(  uint8 i=0;  i<4;  i++  ) {
		if(  water_dir & ribi_t::nsow[i]  ) {
			dx = koord::nsow[i];
			halt = test_halt[i];
			koord last_k = k + dx*len;
			// layout: north 2, west 3, south 0, east 1
			static const uint8 nsow_to_layout[4] = { 2, 0, 1, 3 };
			layout = nsow_to_layout[i];
			if(  layout>=2  ) {
				// reverse construction in these directions
				bau_pos = welt->lookup_kartenboden(last_k)->get_pos();
			}
			if (rotation == layout) {
				// desired rotation works
				break;
			}
			if (rotation != -1) {
				// desired rotation not possible, try others
				if(  --total_dir == 0  ) {
					return "No suitable ground!";
				}
			}
		}
	}

	// handle 16 layouts
	bool change_layout = false;
	if(besch->get_all_layouts()==16) {
		if(  layout<2  ) {
			layout = 15-layout;
		}
		else {
			layout = 9-layout;
		}
		change_layout = true;
	}

	// oriented buildings here - get neighbouring layouts
	koord dx2 = dx;
	dx2.rotate90(0);
	gr = welt->lookup_kartenboden(k+dx2);

	// find out if middle end or start tile
	if(gr  &&  gr->is_halt()  &&  player_t::check_owner( player, gr->get_halt()->get_owner() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  (gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::dock  ||  gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::flat_dock)  ) {
			if(change_layout) {
				layout -= 4;
			}
			if(gb->get_tile()->get_besch()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x02) {
					gb->set_tile( gb->get_tile()->get_besch()->get_tile(ly&0xFD,0,0), false );
				}
			}
		}
	}

	gr = welt->lookup_kartenboden(k-dx2);
	if(gr  &&  gr->is_halt()  &&  player_t::check_owner( player, gr->get_halt()->get_owner() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  (gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::dock  ||  gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::flat_dock)  ) {
			if(change_layout) {
				layout -= 2;
			}
			if(gb->get_tile()->get_besch()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x04) {
					gb->set_tile( gb->get_tile()->get_besch()->get_tile(ly&0xFB,0,0), false );
				}
			}
		}
	}

	DBG_MESSAGE("tool_station_flat_dock_aux()","building dock from square (%d,%d) to (%d,%d) layout=%i", k.x, k.y, last_k.x, last_k.y, layout );

	if(!halt.is_bound()) {
		halt = suche_nahe_haltestelle(player, welt, welt->lookup_kartenboden(k)->get_pos() );
	}
	bool neu = !halt.is_bound();

	if(neu) {
		if(  gr  &&  gr->get_halt().is_bound()  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		// ok, really new stop on this tile then
		halt = haltestelle_t::create(k, player);
	}

	hausbauer_t::baue(halt->get_owner(), bau_pos, layout, besch, &halt);
	sint64 costs = -besch->get_price(welt);
	if(  player!=halt->get_owner() && player != welt->get_player(1)  ) {
		// public stops are expensive!
		// (Except for the public player itself)
		costs -= (besch->get_maintenance(welt) * 60);
	}
	for(  int i=0;  i<=len;  i++  ) {
		koord p=k-dx*i;
		player_t::book_construction_costs(player,  costs, p, water_wt);
	}

	halt->recalc_station_type();
	if(  env_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==k  ) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	if(neu) {
		char* const name = halt->create_name(k, "Dock");
		halt->set_name( name );
		free(name);
	}
	return NULL;
}

// build all types of stops but sea harbours
const char *tool_build_station_t::tool_station_aux(player_t *player, koord3d pos, const haus_besch_t *besch, waytype_t wegtype, const char *type_name )
{
	koord k = pos.get_2d();
DBG_MESSAGE("tool_station_aux()", "building %s on square %d,%d for waytype %x", besch->get_name(), k.x, k.y, wegtype);
	const char *p_error=(besch->get_all_layouts()==4) ? "No terminal station here!" : "No through station here!";

	// underground is checked in work(); if underground only simple stations are allowed
	// get valid ground
	grund_t *bd = tool_intern_koord_to_weg_grund(player, welt, pos, wegtype);

	if(  !bd  ||  bd->get_weg_hang()!=hang_t::flach  ) {
		// only flat tiles, only one stop per map square
		return "No suitable way on the ground!";
	}

	if(  bd->ist_tunnel()  &&  bd->ist_karten_boden()  ) {
		// do not build on tunnel entries
		return "No suitable way on the ground!";
	}

	if(  bd->get_depot()  ) {
		// not on depots
		return "No suitable ground!";
	}

	if(  bd->hat_weg(air_wt)  &&  bd->get_weg(air_wt)->get_besch()->get_styp()!=0  ) {
		return "Flugzeughalt muss auf\nRunway liegen!\n";
	}

	// find out orientation ...
	uint32 layout = 0;
	ribi_t::ribi ribi=ribi_t::keine;
	if(  besch->get_all_layouts()==2  ||  besch->get_all_layouts()==8  ||  besch->get_all_layouts()==16  ) {
		// through station
		if(  bd->has_two_ways()  ) {
			// a crossing or maybe just a tram track on a road ...
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked()  |  bd->get_weg_nr(1)->get_ribi_unmasked();
		}
		else if(  bd->hat_wege()  ) {
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		}
		// not straight: sorry cannot build here ...
		if(  !ribi_t::ist_gerade(ribi)  ) {
			return p_error;
		}
		layout = (ribi & ribi_t::nordsued)?0 :1;
	}
	else if(  besch->get_all_layouts()==4  ) {
		// terminal station
		if(  bd->hat_wege()  ) {
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		}
		// sorry cannot build here ... (not a terminal tile)
		if(  !ribi_t::ist_einfach(ribi)  ) {
			return p_error;
		}

		switch(ribi) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}
	}
	else {
		// something wrong with station number of layouts
		dbg->fatal( "tool_station_t::tool_station_aux", "%s has wrong number of layouts (must be 2,4,8,16!)", besch->get_name() );
		return p_error;
	}

	if(  besch->get_all_layouts() == 8  ||  besch->get_all_layouts() == 16  ) {
		// through station - complex layout
		// bits
		// 1 = north south/east west (as simple layout)
		// 2 = use far end image  \ can be combined
		// 3 = use near end image / to use both end image
		// 4 = platform face - 0 = far, 1 = near

		// bit 1 has already been set

//		ribi_t::ribi next_halt = ribi_t::keine;
		ribi_t::ribi next_own = ribi_t::keine;

		sint8 offset = bd->get_hoehe()+bd->get_weg_yoff()/TILE_HEIGHT_STEP;

		grund_t *gr;
		sint32 neighbour_layout[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
		for(  unsigned i=0;  i<4;  i++  ) {
			// oriented buildings here - get neighbouring layouts
			gr = welt->lookup(koord3d(k+koord::nsow[i],offset));
			if(!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup(koord3d(k+koord::nsow[i],offset-1));
				if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
				else {
					grund_t * gr_tmp = welt->lookup(koord3d(k+koord::nsow[i],offset-2));
					if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 2) {
						gr = gr_tmp;
					}
				}
			}
			if(  gr && gr->get_halt().is_bound()  ) {
				// check, if there is an oriented stop
				const gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->get_tile()->get_besch()->get_all_layouts()>4  &&  (gb->get_tile()->get_besch()->get_utyp()>haus_besch_t::dock  ||  gb->get_tile()->get_besch()->get_utyp()>haus_besch_t::flat_dock)  ) {
					next_own |= ribi_t::nsow[i];
					neighbour_layout[ribi_t::nsow[i]] = gb->get_tile()->get_layout();
				}
			}
		}

		// now for the details
		ribi_t::ribi senkrecht = ~ribi_t::doppelt(ribi);
		ribi_t::ribi waagerecht = ribi_t::doppelt(ribi);
		if(next_own!=ribi_t::keine) {
			// oriented buildings here
			if(ribi_t::ist_einfach(ribi & next_own)) {
				// only a single next neighbour on the same track
				layout |= neighbour_layout[ribi & next_own] & 8;
			}
			else if(ribi_t::ist_gerade(ribi & next_own)) {
				// two neighbours on the same track, use the north/west one
				layout |= neighbour_layout[ribi & next_own & ribi_t::nordwest] & 8;
			}
			else if(ribi_t::ist_einfach((~ribi) & waagerecht & next_own)) {
				// neighbour across break in track
				layout |= neighbour_layout[(~ribi) & waagerecht & next_own] & 8;
			}
			else {
				// no buildings left and right
				// oriented buildings left and right
				if(neighbour_layout[senkrecht & next_own & ribi_t::nordwest] != -1) {
					// just rotate layout
					layout |= 8-(neighbour_layout[senkrecht & next_own & ribi_t::nordwest]&8);
				}
				else {
					if(neighbour_layout[senkrecht & next_own & ribi_t::suedost] != -1) {
						layout |= 8-(neighbour_layout[senkrecht & next_own & ribi_t::suedost]&8);
					}
				}
			}
		}
		// avoid orientation on 8 tiled buildings
		layout &= (besch->get_all_layouts()-1);
	}

	halthandle_t old_halt = bd->get_halt();
	sint64 old_cost = 0;
	bool recalc_schedule = false;

	halthandle_t halt;

	if(  old_halt.is_bound()  ) {
		gebaeude_t* gb = bd->find<gebaeude_t>();
		const haus_besch_t *old_besch = gb->get_tile()->get_besch();
		if(  old_besch == besch  ) {
			// already has the same station
			return NULL;
		}
		if(  old_besch->get_capacity() >= besch->get_capacity()  &&  !is_ctrl_pressed()  ) {
			return "Upgrade must have\na higher level";
		}
		old_cost = old_besch->get_price(welt)*old_besch->get_b()*old_besch->get_h();
		gb->cleanup( NULL );
		delete gb;
		halt = old_halt;
		if(  old_besch->get_enabled() != besch->get_enabled()  ) {
			recalc_schedule = true;
		}
	}
	else {
		halt = suche_nahe_haltestelle(player,welt,bd->get_pos());
	}

	// seems everything ok, lets build
	bool neu = !halt.is_bound();

	if(neu) {
		if(  bd && bd->get_halt().is_bound()  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		halt = haltestelle_t::create(k, player);
	}
	hausbauer_t::neues_gebaeude(halt->get_owner(), bd->get_pos(), layout, besch, &halt);
	halt->recalc_station_type();

	if(neu) {
		char* const name = halt->create_name(k, type_name);
		halt->set_name(name);
		free(name);
	}

	// cost to build new station
	sint64 cost = -besch->get_price(welt)*besch->get_b()*besch->get_h();
	// discount for existing station
	cost += old_cost/2;
	if(  player!=halt->get_owner() && player != welt->get_player(1)  ) {
		// public stops are expensive!
		// (Except for the public player itself)

		cost -= (besch->get_maintenance(welt) * besch->get_b() * besch->get_h() * 60);
	}
	player_t::book_construction_costs(player,  cost, k, wegtype);
	if(  env_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==k  ) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	// the new station (after upgrading) might accept different goods => needs new schedule
	if(  recalc_schedule  ) {
		welt->set_schedule_counter();
	}

	return NULL;
}

// gives the description and sets the rotation value
const haus_besch_t *tool_build_station_t::get_besch( sint8 &rotation ) const
{
	char *haus = strdup( default_param );
	const haus_tile_besch_t *ht = NULL;
	if(  haus  ) {
		char *p = strrchr( haus, ',' );
		if(  p  ) {
			*p++ = 0;
			rotation = atoi( p );
		}
		else {
			rotation = -1;
		}
		ht = hausbauer_t::find_tile(haus,0);
		free( haus );
	}
	if(  ht==NULL  ) {
		return NULL;
	}
	return ht->get_besch();
}

bool tool_build_station_t::init( player_t * )
{
	sint8 rotation = -1;
	const haus_besch_t *hb = get_besch( rotation );
	if(  hb==NULL  ) {
		return false;
	}
	cursor = hb->get_cursor()->get_bild_nr(0);
	if(  !can_use_gui()  ) {
		// do not change cursor
		return true;
	}
	if(  (hb->get_utyp()==haus_besch_t::generic_extension  ||  hb->get_utyp()==haus_besch_t::flat_dock)  &&  hb->get_all_layouts()>1  ) {
		if(  is_ctrl_pressed()  &&  rotation==-1  ) {
			// call station dialog instead
			destroy_win( magic_station_building_select );
			create_win( new station_building_select_t(hb), w_info, magic_station_building_select);
			// we do not activate building yet; else uncomment the return statement
			return false;
		}
		else if(  rotation>=0  ) {
			// rotation is already fixed
			cursor_area = koord( hb->get_b(rotation), hb->get_h(rotation) );
			cursor_centered = false;
			cursor_offset = koord(0,0);
			if (hb->get_utyp()==haus_besch_t::flat_dock  &&  rotation >= 2 ) {
				cursor_offset = cursor_area - koord(1,1);
			}
		}
		else {
			goto set_area_cov;
		}
	}
	else {
set_area_cov:
		uint16 const cov = welt->get_settings().get_station_coverage() * 2 + 1;
		cursor_area = koord(cov, cov);
		cursor_centered = true;
		cursor_offset = koord(0,0);
	}
	return true;
}


image_id tool_build_station_t::get_icon( player_t * ) const
{
	sint8 dummy;
	const haus_besch_t *besch=get_besch(dummy);
	if (besch == NULL) {
		return IMG_LEER;
	}
	if(  grund_t::underground_mode==grund_t::ugm_all  ) {
		// in underground mode, buildings will be done invisible above ground => disallow such confusion
		if(  besch->get_utyp()!=haus_besch_t::generic_stop  ||  besch->get_extra()==air_wt) {
			return IMG_LEER;
		}
		if(  besch->get_utyp()==haus_besch_t::generic_stop  &&  !besch->can_be_built_underground()) {
			return IMG_LEER;
		}
	}
	if(  grund_t::underground_mode==grund_t::ugm_none  ) {
		if(  besch->get_utyp()==haus_besch_t::generic_stop  &&  !besch->can_be_built_aboveground()) {
			return IMG_LEER;
		}
	}
	return icon;
}


const char* tool_build_station_t::get_tooltip(const player_t *) const
{
	sint8               dummy;
	haus_besch_t const* besch    = get_besch(dummy);
	if (besch == NULL) {
		return "";
	}

	sint64 price = 0;
	sint64 maint = 0;
	uint32 cap = besch->get_capacity(); // This is always correct in the besch object.

	maint = besch->get_maintenance(welt);

	if(  besch->get_utyp()==haus_besch_t::generic_stop || besch->get_utyp()==haus_besch_t::generic_extension || besch->get_utyp()==haus_besch_t::dock || besch->get_utyp()==haus_besch_t::flat_dock  ) {
		price = -besch->get_price(welt);
	}
	else {
		return "Illegal description";
	}

	if(besch->get_utyp()==haus_besch_t::generic_extension || besch->get_utyp()==haus_besch_t::dock || besch->get_utyp()==haus_besch_t::flat_dock) {
		const sint16 size_multiplier = besch->get_groesse().x * besch->get_groesse().y;
		price *= size_multiplier;
		cap *= size_multiplier;
		maint *= size_multiplier;
	}

	return tooltip_with_price_maintenance_capacity(welt, besch->get_name(), price, maint, cap, besch->get_enabled());
}

waytype_t tool_build_station_t::get_waytype() const
{
	sint8 dummy;
	haus_besch_t const* besch = get_besch(dummy);
	switch (besch ? besch->get_utyp() : haus_besch_t::generic_extension) {
		case haus_besch_t::generic_stop:
			return (waytype_t)besch->get_extra();
		case haus_besch_t::dock:
		case haus_besch_t::flat_dock:
			return water_wt;
		case haus_besch_t::generic_extension:
		default:
			return invalid_wt;
	}
}


const char *tool_build_station_t::check_pos( player_t*,  koord3d pos )
{
	if(  grund_t *gr = welt->lookup( pos )  ) {
		sint8 rotation;
		const haus_besch_t *besch = get_besch(rotation);
		if(  grund_t *bd = welt->lookup_kartenboden( pos.get_2d() )  ) {
			const bool underground = bd->get_hoehe()>gr->get_hoehe();
			if(  underground  ) {
				// in underground mode, buildings will be done invisible above ground => disallow such confusion
				if(  besch->get_utyp()!=haus_besch_t::generic_stop  ||  besch->get_extra()==air_wt) {
					return "Cannot built this station/building\nin underground mode here.";
				}
				if(  besch->get_utyp()==haus_besch_t::generic_stop  &&  !besch->can_be_built_underground()) {
					return "Cannot built this station/building\nin underground mode here.";
				}
			}
			else if(  besch->get_utyp()==haus_besch_t::generic_stop  &&  !besch->can_be_built_aboveground()) {
				return "This station/building\ncan only be built underground.";
			}
			return NULL;
		}
	}
	// no ground here???
	return "Missing ground (fatal!)";
}


const char *tool_build_station_t::move( player_t *player, uint16 buttonstate, koord3d pos )
{
	CHECK_FUNDS();

	const char *result = NULL;
	if(  buttonstate==1  ) {
		const grund_t *gr = welt->lookup(pos);
		if(!gr) {
			return "";
		}

		// ownership allowed?
		halthandle_t halt = gr->get_halt();
		if(halt.is_bound()  &&  !player_t::check_owner( player, halt->get_owner())) {
			return "";
		}

		if(  env_t::networkmode  ) {
			// queue tool for network
			nwc_tool_t *nwc = new nwc_tool_t(player, this, pos, welt->get_steps(), welt->get_map_counter(), false);
			network_send_server(nwc);
		}
		else {
			result = work( player, pos );
		}
	}
	return result;
}


const char *tool_build_station_t::work( player_t *player, koord3d pos )
{
	const grund_t *gr = welt->lookup(pos);
	if(!gr) {
		return "";
	}

	// ownership allowed?
	halthandle_t halt = gr->get_halt();
	if(halt.is_bound()  &&  !player_t::check_owner( player, halt->get_owner())) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	// check underground / above ground
	if (const char* msg = check_pos(player, pos)) {
		return msg;
	}

	sint8 rotation = 0;
	const haus_besch_t *besch=get_besch(rotation);
	const char *msg = NULL;
	switch (besch->get_utyp()) {
		case haus_besch_t::dock:
			msg = tool_build_station_t::tool_station_dock_aux(player, pos, besch );
			break;
		case haus_besch_t::flat_dock:
			msg = tool_build_station_t::tool_station_flat_dock_aux(player, pos, besch, rotation );
			break;
		case haus_besch_t::hafen_geb:
		case haus_besch_t::generic_extension:
			msg = tool_build_station_t::tool_station_building_aux(player, false, pos, besch, rotation );
			if (msg) {
				// try to build near a public halt
				msg = tool_build_station_t::tool_station_building_aux(player, true, pos, besch, rotation );
			}
			break;
		case haus_besch_t::generic_stop: {
			switch(besch->get_extra()) {
				case road_wt:
					msg = tool_build_station_t::tool_station_aux(player, pos, besch, road_wt, "H");
					break;
				case track_wt:
				case monorail_wt:
				case maglev_wt:
				case narrowgauge_wt:
				case tram_wt:
					msg = tool_build_station_t::tool_station_aux(player, pos, besch, (waytype_t)besch->get_extra(), "BF");
					break;
				case water_wt:
					msg = tool_build_station_t::tool_station_aux(player, pos, besch, water_wt, "Dock");
					break;
				case air_wt:
					msg = tool_build_station_t::tool_station_aux(player, pos, besch, air_wt, "Airport");
					break;
			}
			break;
		}

		default:
			dbg->warning("tool_station_t::work()","tool called for illegal besch \"%\"", default_param );
			msg = "Illegal station tool";
	}
	return msg;
}


char const* tool_build_roadsign_t::get_tooltip(player_t const*) const
{
	const roadsign_besch_t * besch = roadsign_t::find_besch(default_param);
	if(besch) {
		return tooltip_with_price( besch->get_name(), -besch->get_preis() );
	}
	return NULL;
}

void tool_build_roadsign_t::draw_after(scr_coord k, bool dirty) const
{
	if(  icon!=IMG_LEER  &&  is_selected()  ) {
		display_img_blend( icon, k.x, k.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, dirty );
		char level_str[16];
		sprintf(level_str, "%i", signal[welt->get_active_player_nr()].spacing);
		display_proportional( k.x+4, k.y+4, level_str, ALIGN_LEFT, COL_YELLOW, true );
	}
}

const char* tool_build_roadsign_t::check_pos_intern(player_t *player, koord3d pos)
{
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	if (besch==NULL) {
		// read data from string
		read_default_param(player);
	}
	if (besch==NULL) {
		return error;
	}
	// search for starting ground
	grund_t *gr = tool_intern_koord_to_weg_grund(player, welt, pos, besch->get_wtyp());
	if(gr) {

		signal_t *s = gr->find<signal_t>();
		if(s  &&  s->get_besch()!=besch) {
			// only one sign per tile
			return error;
		}

		if(besch->is_signal()  &&  gr->find<roadsign_t>())  {
			// only one sign per tile
			return error;
		}

		// get the sign direction
		weg_t *weg = gr->get_weg( besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		ribi_t::ribi dir = weg->get_ribi_unmasked();

		// no signs on runways
		if(  weg->get_waytype() == air_wt  &&  weg->get_besch()->get_styp() == weg_besch_t::runway  ) {
			return error;
		}

		// no signals on switches
		if(  ribi_t::is_threeway(dir)  &&  besch->is_signal_type()  ) {
			return error;
		}

		if(  besch->is_private_way()  &&  !ribi_t::ist_gerade(dir)  ) {
			// only on straight tiles ...
			return error;
		}

		const bool two_way = besch->is_single_way()  ||  besch->is_signal() ||  besch->is_pre_signal();

		if(!(besch->is_traffic_light() || two_way)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (besch->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
			roadsign_t* rs;
			if (besch->is_signal_type()) {
				// if there is already a signal, we might need to inverse the direction
				rs = gr->find<signal_t>();
				if (rs) {
					if(  !player_t::check_owner( rs->get_owner(), player )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
				}
			}
			else {
				// if there is already a sign, we might need to inverse the direction
				rs = gr->find<roadsign_t>();
				if (rs) {
					if(  !player_t::check_owner( rs->get_owner(), player )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
				}
			}
			error = NULL;
		}
	}
	return error;
}

char tool_build_roadsign_t::toolstring[256];

const char* tool_build_roadsign_t::get_default_param(player_t *player) const
{
	if (besch  &&  player) {
		signal_info const& s = signal[player->get_player_nr()];
		sprintf(toolstring, "%s,%d,%d,%d", besch->get_name(), s.spacing, s.remove_intermediate, s.replace_other);
		return toolstring;
	}
	else {
		return default_param;
	}
}

waytype_t tool_build_roadsign_t::get_waytype() const
{
	return besch ? besch->get_wtyp() : invalid_wt;
}

// read variables from default_param if cmd comes from network
// default_param: sign_name,signal_spacing,remove,replace
// if the static variable toolstring is the default_param then reset default_param to name of signal
void tool_build_roadsign_t::read_default_param(player_t * player)
{
	char name[256]="";
	uint32 i;
	for(i=0; default_param[i]!=0  &&  default_param[i]!=','; i++) {
		name[i]=default_param[i];
	}
	name[i]=0;
	besch = roadsign_t::find_besch(name);

	if (default_param[i]) {
		signal_info& s = signal[player->get_player_nr()];
		int i_signal_spacing              = s.spacing;
		int i_remove_intermediate_signals = s.remove_intermediate;
		int i_replace_other_signals       = s.replace_other;
		sscanf(default_param+i, ",%d,%d,%d", &i_signal_spacing, &i_remove_intermediate_signals, &i_replace_other_signals);
		s.spacing             = (uint8)i_signal_spacing;
		s.remove_intermediate = i_remove_intermediate_signals != 0;
		s.replace_other       = i_replace_other_signals       != 0;
	}
	if (default_param==toolstring) {
		default_param = besch->get_name();
	}
}

bool tool_build_roadsign_t::init( player_t * player)
{
	// read data from string
	read_default_param(player);

	if (is_ctrl_pressed()  &&  can_use_gui()) {
		create_win(new signal_spacing_frame_t(player, this), w_info, (ptrdiff_t)this);
	}
	return two_click_tool_t::init(player)  &&  (besch!=NULL);
}

bool tool_build_roadsign_t::exit( player_t *player )
{
	destroy_win((ptrdiff_t)this);
	return two_click_tool_t::exit(player);
}

uint8 tool_build_roadsign_t::is_valid_pos( player_t *player, const koord3d &pos, const char *&error, const koord3d &start)
{
	// first click
	if (start==koord3d::invalid) {
		error = check_pos_intern(player, pos);
		return (error==NULL ? 3 : 0);
	}
	// second click
	else {
		error = NULL;
		return 2;
	}
}


bool tool_build_roadsign_t::calc_route( route_t &verbindung, player_t *player, const koord3d& start, const koord3d& to )
{
	// get a default vehikel
	vehikel_besch_t rs_besch( besch->get_wtyp(), 500, vehikel_besch_t::diesel );
	vehicle_t* test_vehicle = vehikelbauer_t::baue(start, player, NULL, &rs_besch);
	test_vehicle->set_flag(obj_t::not_on_map);
	test_driver_t* test_driver = scenario_checker_t::apply(test_vehicle, player, this);

	bool can_built;
	if( start != to ) {
		can_built = verbindung.calc_route(welt, start, to, test_driver, 0, 0);
		// prevent building of many signals if start and to are adjacent
		// but the step start->to is now allowed
		if (can_built  &&  koord_distance(start, to)==1  &&  verbindung.get_count()>2) {
			grund_t *gr, *grto = welt->lookup(to);
			if(  welt->lookup(start)->get_neighbour(gr, besch->get_wtyp(), ribi_typ(to-start) )  &&  gr==grto) {
				can_built = false;
			}
		}
	}
	else {
		verbindung.clear();
		verbindung.append( start );
		can_built = true;
	}
	delete test_driver;
	return can_built;
}

void tool_build_roadsign_t::mark_tiles( player_t *player, const koord3d &start, const koord3d &ziel )
{
	route_t route;
	if (!calc_route(route, player, start, ziel)) {
		return;
	}
	signal_info const& s              = signal[player->get_player_nr()];
	uint8       const  signal_density = 2 * s.spacing;      // measured in half tiles (straight track count as 2, diagonal as 1, since sqrt(1/2) = 1/2 ;)
	uint8              next_signal    = signal_density + 1; // to place a sign asap
	sint32             cost           = 0;
	directions.clear();
	// dummy roadsign to get images for preview
	roadsign_t *dummy_rs;
	if (besch->is_signal_type()) {
		dummy_rs = new signal_t(player, koord3d::invalid, ribi_t::keine, besch, true);
	}
	else {
		dummy_rs = new roadsign_t(player, koord3d::invalid, ribi_t::keine, besch, true);
	}
	dummy_rs->set_flag(obj_t::not_on_map);

	bool single_ribi = besch->is_signal_type() || besch->is_single_way() || besch->is_choose_sign();
	for(  uint16 i = 0;  i < route.get_count();  i++  ) {
		grund_t* gr = welt->lookup( route.position_bei(i) );

		weg_t *weg = gr->get_weg(besch->get_wtyp());
		ribi_t::ribi ribi=weg->get_ribi_unmasked(); // set full ribi when signal is on a crossing.
		if(  single_ribi  ) {
			if(i>0) {
				// take backward direction
				ribi = ribi_typ(route.position_bei(i), route.position_bei(i-1));
			}
			else {
				// clear one direction bit to get single direction for signal
				ribi &= ~ribi_typ(route.position_bei(i), route.position_bei(i+1));
			}
		}

		roadsign_t *rs = gr->find<signal_t>();
		if (rs==NULL) {
			rs = gr->find<roadsign_t>();
		}

		if (rs  &&  rs->get_waytype() != besch->get_waytype()) {
			// do not delete signs from other ways
			continue;
		}

		// check owner .. other signals...
		bool straight = (i == 0)  ||  (i == route.get_count()-1)  ||  ribi_t::ist_gerade(ribi_typ(route.position_bei(i-1), route.position_bei(i+1)));
		next_signal += straight ? 2 : 1;
		if(  next_signal >= signal_density  ) {
			// can we place signal here?
			if (check_pos_intern(player, route.position_bei(i))==NULL  ||
					(s.replace_other && rs && !rs->is_deletable(player))) {
				zeiger_t* zeiger = new zeiger_t(gr->get_pos(), player );
				marked.append(zeiger);
				zeiger->set_bild( skinverwaltung_t::bauigelsymbol->get_bild_nr(0) );
				gr->obj_add( zeiger );
				directions.append(ribi /* !=0 -> place sign*/);
				next_signal = 0;
				dummy_rs->set_pos(gr->get_pos());
				dummy_rs->set_dir(ribi); // calls calc_bild()
				zeiger->set_after_bild(dummy_rs->get_front_image());
				zeiger->set_bild(dummy_rs->get_image());
				cost += rs ? (rs->get_besch()==besch ? 0  : besch->get_preis()+rs->get_besch()->get_preis()) : besch->get_preis();
			}
		}
		else if (s.remove_intermediate && rs && !rs->is_deletable(player)) {
				zeiger_t* zeiger = new zeiger_t(gr->get_pos(), player );
				marked.append(zeiger);
				zeiger->set_bild( tool_t::general_tool[TOOL_REMOVER]->cursor );
				gr->obj_add( zeiger );
				directions.append(ribi_t::keine /*remove sign*/);
				cost += rs->get_besch()->get_preis();
		}
	}
	delete dummy_rs;
	win_set_static_tooltip( tooltip_with_price("Building costs estimates", cost ) );
}

const char *tool_build_roadsign_t::do_work( player_t *player, const koord3d &start, const koord3d &end)
{
	// read data from string
	read_default_param(player);
	// single click ->place signal
	if( end == koord3d::invalid  ||  start == end ) {
		grund_t *gr = welt->lookup(start);
		return place_sign_intern( player, gr );
	}
	// mark tiles to calculate positions of signals
	mark_tiles(player, start, end);
	// only search the marked tiles
	uint32 j=0;
	FOR(slist_tpl<zeiger_t*>, const i, marked) {
		grund_t* const gr = welt->lookup(i->get_pos());
		weg_t *weg = gr->get_weg(besch->get_wtyp());
		ribi_t::ribi dir = directions[j++];
		if (dir) {
			// try to place signal
			const char* error_text =  place_sign_intern( player, gr );
			if(  error_text  ) {
				if (signal[player->get_player_nr()].replace_other) {
					roadsign_t* rs = gr->find<signal_t>();
					if(rs == NULL) rs = gr->find<roadsign_t>();
					if(  rs != NULL  &&  rs->is_deletable(player) == NULL  ) {
						rs->cleanup(player);
						delete rs;
						error_text =  place_sign_intern( player, gr );
					}
				}
			}
			if(  error_text  ) {
				return error_text;
			}
			roadsign_t* rs = gr->find<signal_t>();
			if(rs == NULL) rs = gr->find<roadsign_t>();
			assert(rs);
			rs->set_dir(dir);
		}
		else {
			// Place no signal -> remove existing signal
			roadsign_t* rs = gr->find<signal_t>();
			if(rs == NULL) rs = gr->find<roadsign_t>();
			if(  rs != NULL  &&  rs->is_deletable(player) == NULL  ) {
				rs->cleanup(player);
				delete rs;
			};
		}
		weg->count_sign();
		gr->calc_bild();
	}
	cleanup();
	directions.clear();
	return NULL;
}

/*
 * Called by the GUI (gui/signal_spacing.*)
 */
void tool_build_roadsign_t::set_values( player_t *player, uint8 spacing, bool remove, bool replace )
{
	signal_info& s = signal[player->get_player_nr()];
	s.spacing             = spacing;
	s.remove_intermediate = remove;
	s.replace_other       = replace;
}


void tool_build_roadsign_t::get_values( player_t *player, uint8 &spacing, bool &remove, bool &replace )
{
	signal_info const& s = signal[player->get_player_nr()];
	spacing = s.spacing;
	remove  = s.remove_intermediate;
	replace = s.replace_other;
}


const char *tool_build_roadsign_t::place_sign_intern( player_t *player, grund_t* gr, const roadsign_besch_t*)
{
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	// search for starting ground
	if(gr) {
		// get the sign direction
		weg_t *weg = gr->get_weg( besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		roadsign_t *s = gr->find<signal_t>();
		if(s==NULL) {
			s = gr->find<roadsign_t>();
		}
		if(s  &&  s->get_besch()!=besch) {
			// only one sign per tile
			return error;
		}
		ribi_t::ribi dir = weg->get_ribi_unmasked();

		const bool two_way = besch->is_single_way()  ||  besch->is_signal() ||  besch->is_pre_signal();

		if(!(besch->is_traffic_light() || two_way)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (besch->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
			roadsign_t* rs;
			if (besch->is_signal_type()) {
				// if there is already a signal, we might need to inverse the direction
				rs = gr->find<signal_t>();
				if (rs) {
					if(  !player_t::check_owner( rs->get_owner(), player )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					// signals have three options
					ribi_t::ribi sig_dir = rs->get_dir();
					uint8 i = 0;
					if (!ribi_t::is_twoway(sig_dir)) {
						// inverse first dir
						for (; i < 4; i++) {
							if ((dir & ribi_t::nsow[i]) == sig_dir) {
								i++;
								break;
							}
						}
					}
					// find the second dir ...
					for (; i < 4; i++) {
						if ((dir & ribi_t::nsow[i]) != 0) {
							dir = ribi_t::nsow[i];
						}
					}
					// if nothing found, we have two ways again ...
					rs->set_dir(dir);
				}
				else {
					// add a new signal at position zero!
					rs = new signal_t(player, gr->get_pos(), dir, besch);
					DBG_MESSAGE("tool_roadsign()", "new signal, dir is %i", dir);
					goto built_sign;
				}
			}
			else {
				// if there is already a sign, we might need to inverse the direction
				rs = gr->find<roadsign_t>();
				if (rs) {
					if(  !player_t::check_owner( rs->get_owner(), player )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					// reverse only if single way sign
					if (besch->is_single_way() || besch->is_choose_sign()) {
						dir = ~rs->get_dir() & weg->get_ribi_unmasked();
						rs->set_dir(dir);
						DBG_MESSAGE("tool_roadsign()", "reverse ribi %i", dir);
					}
				}
				else {
					// add a new roadsign at position zero!
					// if single way, we need to reduce the allowed ribi to one
					if (besch->is_single_way() || besch->is_choose_sign()) {
						for(  int i=0;  i<4;  i++  ) {
							if ((dir & ribi_t::nsow[i]) != 0) {
								dir = ribi_t::nsow[i];
								break;
							}
						}
					}
					DBG_MESSAGE("tool_roadsign()", "new roadsign, dir is %i", dir);
					rs = new roadsign_t(player, gr->get_pos(), dir, besch);
built_sign:
					gr->obj_add(rs);
					rs->finish_rd();	// to make them visible
					weg->count_sign();
					player_t::book_construction_costs(player, -besch->get_preis(), gr->get_pos().get_2d(), weg->get_waytype());
				}
			}
			error = NULL;
		}
	}
	return error;
}



// built all types of depots
const char *tool_build_depot_t::tool_depot_aux(player_t *player, koord3d pos, const haus_besch_t *besch, waytype_t wegtype)
{
	if(welt->is_within_limits(pos.get_2d())) {
		grund_t *bd=NULL;
		// special for the seven seas ...
		if(wegtype==water_wt) {
			bd = welt->lookup_kartenboden(pos.get_2d());
			if(!bd->ist_wasser()) {
				bd = NULL;
			}
		}
		if(bd==NULL) {
			bd = tool_intern_koord_to_weg_grund(player,welt,pos,wegtype);
		}
		if(!bd  ||  bd->has_two_ways()) {
			return "Cannot built depot here!";
		}

		// no depots on runways!
		if(besch->get_extra()==air_wt  &&  bd->get_weg(air_wt)->get_besch()->get_styp()!=0) {
			return "Cannot built depot here!";
		}

		const char *p=bd->kann_alle_obj_entfernen(player);
		if(p) {
			return p;
		}

		// avoid building over a stop
		if(bd->is_halt()  ||  bd->get_depot()!=NULL) {
			return "Cannot built depot here!";
		}

		ribi_t::ribi ribi;
		if(bd->ist_wasser()) {
			// assume one orientation with water
			ribi = ribi_t::sued;
		}
		else {
			ribi = bd->get_weg_ribi_unmasked(wegtype);
		}

		if(ribi_t::ist_einfach(ribi)  &&  bd->get_weg_hang()==0) {

			int layout = 0;
			switch(ribi) {
				//case ribi_t::sued:layout = 0;  break;
				case ribi_t::ost:   layout = 1;    break;
				case ribi_t::nord:  layout = 2;    break;
				case ribi_t::west:  layout = 3;    break;
			}
			hausbauer_t::neues_gebaeude(player, bd->get_pos(), layout, besch );
			player_t::book_construction_costs(player, -besch->get_price(welt), pos.get_2d(), besch->get_finance_waytype());
			if(can_use_gui()  &&  player == welt->get_active_player()) {
				welt->set_tool( general_tool[TOOL_QUERY], player );
			}

			return NULL;
		}
		return "Cannot built depot here!";
	}
	return "";
}

image_id tool_build_depot_t::get_icon(player_t *player) const
{
	if(  player  &&  player->get_player_nr()!=1  ) {
		const haus_besch_t *besch = hausbauer_t::find_tile(default_param,0)->get_besch();
		const uint16 time = welt->get_timeline_year_month();
		if(  besch  &&  besch->is_available(time)  ) {
			return besch->get_cursor()->get_bild_nr(1);
		}
	}
	return IMG_LEER;
}

bool tool_build_depot_t::init( player_t *player )
{
	haus_besch_t const* besch = hausbauer_t::find_tile(default_param, 0)->get_besch();
	if (besch == NULL) {
		return false;
	}
	// no depots for player 1
	if(player!=welt->get_player(1)) {
		cursor = besch->get_cursor()->get_bild_nr(0);
		return true;
	}
	return false;
}

const char* tool_build_depot_t::get_tooltip(const player_t *) const
{
	settings_t   const& settings = welt->get_settings();
	haus_besch_t const* besch    = hausbauer_t::find_tile(default_param, 0)->get_besch();
	if (besch == NULL) {
		return NULL;
	}

	char         const* tip;
	switch (besch->get_extra()) {
		case road_wt:        tip = "Build road depot";        break;
		case track_wt:       tip = "Build train depot";       break;
		case monorail_wt:    tip = "Build monorail depot";    break;
		case maglev_wt:      tip = "Build maglev depot";      break;
		case narrowgauge_wt: tip = "Build narrowgauge depot"; break;
		case tram_wt:        tip = "Build tram depot";        break;
		case water_wt:       tip = "Build ship depot";        break;
		case air_wt:         tip = "Build air depot";         break;
		default:             return 0;
	}
	return tooltip_with_price_maintenance(welt, tip, -besch->get_price(welt), settings.maint_building * besch->get_level());
}

waytype_t tool_build_depot_t::get_waytype() const
{
	const haus_besch_t *besch = hausbauer_t::find_tile(default_param,0)->get_besch();
	return besch ? (waytype_t)besch->get_extra() : invalid_wt;
}

const char *tool_build_depot_t::work( player_t *player, koord3d pos )
{
	if(player==welt->get_player(1)) {
		// no depots for player 1
		return 0;
	}

	haus_besch_t const* const besch = hausbauer_t::find_tile(default_param,0)->get_besch();
	switch(besch->get_extra()) {
		case road_wt:
		case track_wt:
		case water_wt:
		case air_wt:
		case maglev_wt:
		case narrowgauge_wt:
			return tool_build_depot_t::tool_depot_aux(player, pos, besch, (waytype_t)besch->get_extra());

		case monorail_wt:
			{
				// since it needs also a foundation, this is slightly more complex ...
				char const* const err = tool_build_depot_t::tool_depot_aux(player, pos, besch, monorail_wt);
				if(err==NULL) {
					grund_t *bd = welt->lookup_kartenboden(pos.get_2d());
					if(hausbauer_t::elevated_foundation_besch  &&  pos.z-bd->get_pos().z==1  &&  bd->ist_natur()) {
						hausbauer_t::baue(player, bd->get_pos(), 0, hausbauer_t::elevated_foundation_besch );
					}
				}
				return err;
			}
		case tram_wt:
			return tool_build_depot_t::tool_depot_aux(player, pos, besch, track_wt);
		default:
			dbg->warning("tool_depot()","called with unknown besch %s",besch->get_name() );
			return "Unknown depot object";
	}
	return NULL;
}



/* builds (random) tourist attraction and maybe adds it to the next city
 * the parameter string is a follow:
 * 1#theater
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * finally building name
 * @author prissi
 */
bool tool_build_house_t::init( player_t * )
{
	if (can_use_gui() && !strempty(default_param)) {
		const char *c = default_param+2;
		const haus_tile_besch_t *tile = hausbauer_t::find_tile(c,0);
		if(tile!=NULL) {
			int rotation = (default_param[1]-'0') % tile->get_besch()->get_all_layouts();
			cursor_area = tile->get_besch()->get_groesse(rotation);
		}
	}
	return true;
}

// TODO: merge this into building_layout defined in simcity.cc
static int const building_layout[] = { 0, 0, 1, 4, 2, 0, 5, 1, 3, 7, 1, 0, 6, 3, 2, 0 };

const char *tool_build_house_t::work( player_t *player, koord3d pos )
{
	koord k(pos.get_2d());

	const grund_t* gr = welt->lookup_kartenboden(k);
	if(gr==NULL) {
		return "";
	}

	// Parsing parameter (if there)
	const haus_besch_t *besch = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		const haus_tile_besch_t *tile = hausbauer_t::find_tile(c,0);
		if(tile) {
			besch = tile->get_besch();
		}
	}
	else {
		besch = hausbauer_t::waehle_sehenswuerdigkeit( welt->get_timeline_year_month(), false, welt->get_climate( k ) );
	}

	if(besch==NULL) {
		return "";
	}
	int rotation;
	if(  !default_param || default_param[1]=='#'  ) {
		rotation = simrand(besch->get_all_layouts());
	}
	else if(  default_param[1]=='A'  ) {
		if(  besch->get_utyp()!=haus_besch_t::attraction_land  &&  besch->get_utyp()!=haus_besch_t::attraction_city  ) {
			// auto rotation only valid for city buildings
			int streetdir = 0;
			for(  int i = 1;  i < 8;  i+=2  ) {
				grund_t *gr2 = welt->lookup_kartenboden(k + koord::neighbours[i]);
				if(  gr2  &&  gr2->get_weg_hang() == gr2->get_grund_hang()  &&  gr2->get_weg(road_wt) != NULL  ) {
					// update directions - note this is SENW, conversion from neighbours to SENW is
					// neighbours SENW
					// 3          0
					// 5          1
					// 7          2
					// 1          3
					streetdir += (1 << (((i-3)/2)&3));
				}
			}
			rotation = building_layout[streetdir];
		}
		else {
			rotation = simrand(besch->get_all_layouts());
		}
	}
	else {
		rotation = (default_param[1]-'0') % besch->get_all_layouts();
	}

	koord size = besch->get_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : besch->get_allowed_climate_bits();

	bool hat_platz = welt->square_is_free( k, besch->get_b(rotation), besch->get_h(rotation), NULL, cl );
	if(!hat_platz  &&  size.y!=size.x  &&  besch->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#'  ||  default_param[1]=='A')) {
		// try other rotation too ...
		rotation = (rotation+1) % besch->get_all_layouts();
		hat_platz = welt->square_is_free( k, besch->get_b(rotation), besch->get_h(rotation), NULL, cl );
	}

	// Place found...
	if(hat_platz) {
		player_t *gb_player = besch->get_typ()!=gebaeude_t::unbekannt ? NULL : welt->get_player(1);
		gebaeude_t *gb = hausbauer_t::baue(gb_player, gr->get_pos(), rotation, besch);
		if(gb) {
			// building successful
			if(  besch->get_utyp()!=haus_besch_t::attraction_land  &&  besch->get_utyp()!=haus_besch_t::attraction_city  ) {
				stadt_t *city = welt->suche_naechste_stadt( k );
				if(city) {
					city->add_gebaeude_to_stadt(gb);
				}
			}
			player_t::book_construction_costs(player, -besch->get_price(welt) * size.x * size.y, k, gb->get_waytype());
			return NULL;
		}
	}
	return "No suitable ground!";
}



// show industry size in cursor (in known)
bool tool_build_land_chain_t::init( player_t * )
{
	if (can_use_gui() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		cursor_area = fab->get_haus()->get_groesse(rotation);
	}
	return true;
}

/* builds a (if param=NULL random) industry chain starting here *
 * the parameter string is a follow:
 * 1#34,oelfeld
 * first letter: ignore climates
 * second letter: rotation (0,1,2,3,#=random)
 * next number is production value
 * finally industry name
 */
const char *tool_build_land_chain_t::work( player_t *player, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( pos.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->get_haus()->get_all_layouts() : simrand(fab->get_haus()->get_all_layouts()-1);
	koord size = fab->get_haus()->get_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : fab->get_haus()->get_allowed_climate_bits();

	bool hat_platz = false;
	if(fab->get_platzierung()==fabrik_besch_t::Wasser) {
		// at sea
		hat_platz = welt->ist_wasser( pos.get_2d(), fab->get_haus()->get_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->ist_wasser( pos.get_2d(), fab->get_haus()->get_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->square_is_free( pos.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->square_is_free( pos.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		// eventually adjust production
		sint32 initial_prod = -1;
		if (!strempty(default_param)) {
			initial_prod = welt->inverse_scale_with_month_length( atol(default_param+2) );
		}

		koord3d build_pos = gr->get_pos();
		int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, initial_prod, rotation, &build_pos, welt->get_player(1), 10000 );

		if(anzahl>0) {
			// at least one factory has been built
			welt->get_viewport()->change_world_position( build_pos );
			player_t::book_construction_costs(player, anzahl * welt->get_settings().cst_multiply_found_industry, build_pos.get_2d(), ignore_wt);

			// crossconnect all?
			if (welt->get_settings().is_crossconnect_factories()) {
				FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
					f->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return "No suitable ground!";
}


// show industry size in cursor (in known)
bool tool_city_chain_t::init( player_t * )
{
	if (can_use_gui() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		cursor_area = fab->get_haus()->get_groesse(rotation);
	}
	return true;
}

/* builds a industry chain in the next town
 * defaukt_param see previous function
 */
const char *tool_city_chain_t::work( player_t *player, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( pos.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}

	// eventually adjust production
	sint32 initial_prod = -1;
	if (!strempty(default_param)) {
		initial_prod = welt->inverse_scale_with_month_length( atol(default_param+2) );
	}

	pos = gr->get_pos();
	int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, initial_prod, 0, &pos, welt->get_player(1), 10000 );
	if(anzahl>0) {
		// at least one factory has been built
		welt->get_viewport()->change_world_position( pos );

		// crossconnect all?
		if (welt->get_settings().is_crossconnect_factories()) {
			FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
				f->add_all_suppliers();
			}
		}
		// ain't going to be cheap
		player_t::book_construction_costs(player, anzahl * welt->get_settings().cst_multiply_found_industry, pos.get_2d(), ignore_wt);
		return NULL;
	}
	return "No suitable ground!";
}



// show industry size in cursor (must be known!)
bool tool_build_factory_t::init( player_t * )
{
	if (can_use_gui() && !strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		cursor_area = fab->get_haus()->get_groesse(rotation);
		return true;
	}
	return true;
}

/* builds an industry next to the cursor (default_param see above) */
const char *tool_build_factory_t::work( player_t *player, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if (!strempty(default_param)) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1 << welt->get_climate( pos.get_2d() )), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->get_haus()->get_all_layouts() : simrand(fab->get_haus()->get_all_layouts());
	koord size = fab->get_haus()->get_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : fab->get_haus()->get_allowed_climate_bits();

	bool hat_platz = false;
	if(fab->get_platzierung()==fabrik_besch_t::Wasser) {
		// at sea
		hat_platz = welt->ist_wasser( pos.get_2d(), fab->get_haus()->get_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->ist_wasser( pos.get_2d(), fab->get_haus()->get_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->square_is_free( pos.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->square_is_free( pos.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		// eventually adjust production
		sint32 initial_prod = -1;
		if (!strempty(default_param)) {
			initial_prod = welt->inverse_scale_with_month_length( atol(default_param+2) );
		}

		fabrik_t *f = fabrikbauer_t::baue_fabrik(NULL, fab, initial_prod, rotation, gr->get_pos(), welt->get_player(1));
		if(f) {
			// at least one factory has been built
			welt->get_viewport()->change_world_position( pos );
			player_t::book_construction_costs(player, welt->get_settings().cst_multiply_found_industry, pos.get_2d(), ignore_wt);

			// crossconnect all?
			if (welt->get_settings().is_crossconnect_factories()) {
				FOR(slist_tpl<fabrik_t*>, const f, welt->get_fab_list()) {
					f->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return "No suitable ground!";
}



/**	link tool: links products of factory one with factory two (if possible)
 */
image_id tool_link_factory_t::get_marker_image()
{
	return cursor;
}


uint8 tool_link_factory_t::is_valid_pos( player_t *, const koord3d &pos, const char *&error, const koord3d & )
{
	fabrik_t *fab = fabrik_t::get_fab( pos.get_2d() );
	if (fab == NULL) {
		error = "";
		return 0;
	}
	return 2;
}


const char *tool_link_factory_t::do_work( player_t *, const koord3d &start, const koord3d &pos )
{
	fabrik_t *last_fab = fabrik_t::get_fab( start.get_2d() );
	fabrik_t *fab = fabrik_t::get_fab( pos.get_2d() );

	if(fab!=NULL  &&  last_fab!=NULL  &&  last_fab!=fab) {
		// It's a factory
		if(!is_ctrl_pressed()) {
			if(fab->add_supplier(last_fab) || last_fab->add_supplier(fab)) {
				//ok! they are connected
				return NULL;
			}
		}
		else {
			// remove connections
			fab->rem_supplier(last_fab->get_pos().get_2d());
			fab->rem_lieferziel(last_fab->get_pos().get_2d());
			last_fab->rem_supplier(fab->get_pos().get_2d());
			last_fab->rem_lieferziel(fab->get_pos().get_2d());
			return NULL;
		}
	}
	return "";
}


/* builds company headquarters
 * @author prissi
 */
const haus_besch_t *tool_headquarter_t::next_level( const player_t *player ) const
{
	return hausbauer_t::get_headquarter(player->get_headquarter_level(), welt->get_timeline_year_month());
}

const char* tool_headquarter_t::get_tooltip(const player_t *player) const
{
	if (haus_besch_t const* const besch = next_level(player)) {
		settings_t  const& s      = welt->get_settings();
		char const* const  tip    = player->get_headquarter_level() == 0 ? "build HQ" : "upgrade HQ";
		sint64      const  factor = besch->get_b() * besch->get_h();
		return tooltip_with_price_maintenance(welt, tip, -factor * besch->get_price(welt), factor * besch->get_level() * s.maint_building);
	}
	return NULL;
}

bool tool_headquarter_t::init( player_t *player )
{
	// do no use this, if there is no next level to build ...
	const haus_besch_t *besch = next_level(player);
	if (besch) {
		if (can_use_gui()) {
			const int rotation = 0;
			cursor_area = besch->get_groesse(rotation);
		}
		return true;
	}
	return false;
}


const char *tool_headquarter_t::work( player_t *player, koord3d pos )
{
	bool ok=false;
	bool built = false;
DBG_MESSAGE("tool_headquarter()", "building headquarters at (%d,%d)", pos.x, pos.y);

	const haus_besch_t* besch = next_level(player);
	if(besch==NULL) {
		// no further headquarters level
		dbg->message( "tool_headquarter()", "Already at maximum level!" );
		return "";
	}

	koord size = besch->get_groesse();
	sint64 const cost = -besch->get_price(welt) * size.x * size.y;
	if(  -cost > player->get_finance()->get_account_balance()  ) {
		return "Not enough money!";
	}


	koord k(pos.get_2d());

	grund_t *gr = welt->lookup_kartenboden(k);

	if(gr) {
		gebaeude_t *hq = NULL;
		// check for current head quarter
		koord previous = player->get_headquarter_pos();
		if(previous!=koord::invalid) {
			grund_t *gr_hq = welt->lookup_kartenboden(previous);
			gebaeude_t *prev_hq = gr_hq->find<gebaeude_t>();
			// check if upgrade should be built at same place as current one
			gebaeude_t *gb = gr->find<gebaeude_t>();
			if (gb  &&  gb->get_owner()==player  &&  prev_hq->get_tile()->get_besch()==gb->get_tile()->get_besch()) {
				const haus_besch_t* prev_besch = prev_hq->get_tile()->get_besch();
				// check if sizes fit
				uint8 prev_layout = prev_hq->get_tile()->get_layout();
				uint8 layout =  prev_layout % besch->get_all_layouts();
				koord size = besch->get_groesse(layout);
				if (prev_besch->get_groesse(prev_layout) == size) {
					// check for same tile structure
					ok = true;
					for (sint16 x=0; x<size.x  &&  ok; x++) {
						for (sint16 y=0; y<size.y  &&  ok; y++) {
							ok = (prev_besch->get_tile(prev_layout, x, y)==NULL)==(besch->get_tile(layout, x, y)==NULL);
						}
					}
					hq = gb;
					if(  ok  ) {
						// upgrade the tiles
						koord k_hq = k - gb->get_tile()->get_offset();
						for(  sint16 x = 0;  x < size.x;  x++  ) {
							for(  sint16 y = 0;  y < size.y;  y++  ) {
								if(  const haus_tile_besch_t *tile = besch->get_tile(layout, x, y)  ) {
									if(  grund_t *gr2 = welt->lookup_kartenboden(k_hq + koord(x, y))  ) {
										if(  gebaeude_t *gb = gr2->find<gebaeude_t>()  ) {
											if(  gb  &&  gb->get_owner() == player  &&  prev_besch == gb->get_tile()->get_besch()  ) {
												player_t::add_maintenance( player, -prev_besch->get_maintenance(welt), prev_besch->get_finance_waytype() );
												gb->set_tile( tile, true );
												player_t::add_maintenance( player, besch->get_maintenance(welt), besch->get_finance_waytype() );
											}
										}
									}
								}
							}
						}
						built = true;
					}
				}
			}
			// did not upgrade old one, need to remove it
			if(  !built  ) {
				// remove previous one
				hausbauer_t::remove( player, prev_hq );
				// resize cursor
				init(player);
			}
		}


		// build new one
		if (!built) {
			int rotate = 0;

			if(welt->square_is_free(k, size.x, size.y, NULL, besch->get_allowed_climate_bits())) {
				ok = true;
			}
			if(!ok  &&  besch->get_all_layouts()>1  &&  size.y != size.x  &&  welt->square_is_free(k, size.y, size.x, NULL, besch->get_allowed_climate_bits())) {
				rotate = 1;
				ok = true;
			}

			if(ok) {
				// then built it
				hq = hausbauer_t::baue(player, gr->get_pos(), rotate, besch, NULL);
				stadt_t *city = welt->suche_naechste_stadt( k );
				if(city) {
					city->add_gebaeude_to_stadt( hq );
				}
				built = true;
			}
			else {
				return "No suitable ground!";
			}
		}


		if(  built  ) {
			// sometimes those are not correct after rotation ...
			player->add_headquarter( besch->get_extra() + 1, hq->get_pos().get_2d() - hq->get_tile()->get_offset() );
			player_t::book_construction_costs(player,  cost, k, ignore_wt);
			// tell the world of it ...
			cbuffer_t buf;
			buf.printf( translator::translate("%s s\nheadquarter now\nat (%i,%i)."), player->get_name(), pos.x, pos.y );
			welt->get_message()->add_message( buf, k, message_t::ai, PLAYER_FLAG|player->get_player_nr(), hq->get_tile()->get_hintergrund(0,0,0) );
			// reset to query tool, since costly relocations should be avoided
			if(can_use_gui()  &&  player == welt->get_active_player()) {
				welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
			}
			return NULL;
		}
	}
	return "";
}

const char *tool_lock_game_t::work( player_t *, koord3d )
{
	// tool can never be executed in network mode
	if (env_t::networkmode) {
		return "";
	}
	// as the result depends on the local locked state of public player
	if (welt->get_player(1)->is_locked() || !welt->get_settings().get_allow_player_change()) {
		return "Only public player can lock games!";
	}
	welt->clear_player_password_hashes();
	if(  !welt->get_player(1)->is_locked() ) {
		return "In order to lock the game, you have to protect the public player by password!";
	}
	welt->get_settings().set_allow_player_change(false);
	destroy_all_win( true );
	welt->switch_active_player( 0, true );
	welt->set_tool( general_tool[TOOL_QUERY], welt->get_player(0) );
	return NULL;
}


const char *tool_add_citycar_t::work( player_t *player, koord3d pos )
{
	if( private_car_t::list_empty() ) {
		// No citycar
		return "";
	}
	grund_t *gr = tool_intern_koord_to_weg_grund( player, welt, pos, road_wt );

	if(  gr != NULL  &&  ribi_t::is_twoway(gr->get_weg_ribi_unmasked(road_wt))  &&  gr->find<private_car_t>() == NULL) {
		// add citycar
		private_car_t* vt = new private_car_t(gr, koord::invalid);
		gr->obj_add(vt);
		welt->sync.add(vt);
		return NULL;
	}
	return "";
}


uint8 tool_forest_t::is_valid_pos( player_t *, const koord3d &, const char *&, const koord3d & )
{
	// do really nothing ...
	return 2;
}


void tool_forest_t::mark_tiles(  player_t *, const koord3d &start, const koord3d &end )
{
	koord k1, k2;
	k1.x = start.x < end.x ? start.x : end.x;
	k1.y = start.y < end.y ? start.y : end.y;
	k2.x = start.x + end.x - k1.x;
	k2.y = start.y + end.y - k1.y;
	koord k;
	for(  k.x = k1.x;  k.x <= k2.x;  k.x++  ) {
		for(  k.y = k1.y;  k.y <= k2.y;  k.y++  ) {
			grund_t *gr = welt->lookup_kartenboden( k );

			zeiger_t *marker = new zeiger_t(gr->get_pos(), NULL );

			const uint8 grund_hang = gr->get_grund_hang();
			const uint8 weg_hang = gr->get_weg_hang();
			const uint8 hang = max( corner1(grund_hang), corner1(weg_hang)) +
					3 * max( corner2(grund_hang), corner2(weg_hang)) +
					9 * max( corner3(grund_hang), corner3(weg_hang)) +
					27 * max( corner4(grund_hang), corner4(weg_hang));
			uint8 back_hang = (hang % 3) + 3 * ((uint8)(hang / 9)) + 27;
			marker->set_after_bild( grund_besch_t::marker->get_bild( grund_hang % 27 ) );
			marker->set_bild( grund_besch_t::marker->get_bild( back_hang ) );

			marker->mark_image_dirty( marker->get_image(), 0 );
			gr->obj_add( marker );
			marked.insert( marker );
		}
	}
}


const char *tool_forest_t::do_work( player_t *player, const koord3d &start, const koord3d &end )
{
	koord wh, nw;
	wh.x = abs(end.x-start.x)+1;
	wh.y = abs(end.y-start.y)+1;
	nw.x = min(start.x, end.x)+(wh.x/2);
	nw.y = min(start.y, end.y)+(wh.y/2);

	sint64 costs = baum_t::create_forest( nw, wh );
	player_t::book_construction_costs(player, costs * welt->get_settings().cst_remove_tree, end.get_2d(), ignore_wt);

	return NULL;
}


image_id tool_stop_mover_t::get_marker_image()
{
	return cursor;
}


void tool_stop_mover_t::read_start_position(player_t *player, const koord3d &pos)
{
	waytype[0] = invalid_wt;
	waytype[1] = invalid_wt;
	last_halt = halthandle_t();

	grund_t *bd = welt->lookup(pos);
	if (bd==NULL) {
		return;
	}
	// now assign waytypes
	if(bd->ist_wasser()) {
		waytype[0] = water_wt;
	}
	else {
		waytype[0] = bd->get_weg_nr(0)->get_waytype();
		if(bd->get_weg_nr(1)) {
			waytype[1] = bd->get_weg_nr(1)->get_waytype();
		}
	}
	// .. and halt
	last_halt = haltestelle_t::get_halt(pos,player);
}


uint8 tool_stop_mover_t::is_valid_pos(  player_t *player, const koord3d &pos, const char *&error, const koord3d &start)
{
	grund_t *bd = welt->lookup(pos);
	if (bd==NULL) {
		error = "";
		return 0;
	}
	// check halt ownership
	halthandle_t h = haltestelle_t::get_halt(pos,player);
	if(  h.is_bound()  &&  !player_t::check_owner( player, h->get_owner() )  ) {
		error = "Das Feld gehoert\neinem anderen Spieler\n";
		return 0;
	}
	// check for halt on the tile
	if(  h.is_bound()  &&  !(bd->is_halt()  ||  (h->get_station_type()&haltestelle_t::dock  &&  bd->ist_wasser())  )  ) {
		error = "No suitable ground!";
		return 0;
	}

	if (start==koord3d::invalid) {
		// check for existing ways
		if (bd->ist_wasser()  ||  bd->hat_wege()) {
			return 2;
		}
		else {
			error = "No suitable ground!";
			return 0;
		}
	}
	else {
		// read conditions at start point
		read_start_position(player, start);
		// check halts vs waypoints
		if(h.is_bound() ^ last_halt.is_bound()) {
			error = "Can only move from halt to halt or waypoint to waypoint.";
			return 0;
		}
		// check waytypes
		if(  (waytype[0] == water_wt  &&  bd->ist_wasser())  ||  bd->hat_weg(waytype[0])  ||  bd->hat_weg(waytype[1])  ) {
			// ok
			return 2;
		}
		else {
			error = "No suitable ground!";
			return 0;
		}
	}
}

const char *tool_stop_mover_t::do_work( player_t *player, const koord3d &last_pos, const koord3d &pos)
{
	// read conditions at start point
	read_start_position(player, last_pos);

	// second click
	grund_t *bd = welt->lookup(pos);
	halthandle_t h = haltestelle_t::get_halt(pos,player);

	if (bd) {
		const halthandle_t new_halt = h;
		// depending on the waytype we simply build replacements lists
		// in the worst case we have to iterate over all tiles twice ...
		for(  uint i=0;  i<2;  i++  ) {
			const waytype_t wt = waytype[i];
			slist_tpl <koord3d>old_platform;

			if(bd->ist_wasser()) {
				if(wt!=water_wt) {
					break;
				}
			}
			else if(!bd->hat_weg(wt)) {
				continue;
			}
			// platform, stop or just tile moving?
			const bool catch_all_halt = (wt==water_wt  ||  wt==air_wt)  &&  last_halt.is_bound();
			if(!last_halt.is_bound()) {
				old_platform.append(last_pos);
			}
			else if(!catch_all_halt) {
				// builds a coordinate list
				if(wt==road_wt) {
					old_platform.append(last_pos);
				}
				else {
					// all connected tiles for start pos
					uint8 ribi = welt->lookup(last_pos)->get_weg_ribi_unmasked(wt);
					koord delta = ribi_t::ist_gerade_ns(ribi) ? koord(0,1) : koord(1,0);
					koord3d start_pos=last_pos;
					while(ribi&12) {
						koord3d test_pos = start_pos+delta;
						grund_t *gr = welt->lookup(test_pos);
						if(!gr  ||  !gr->is_halt()  ||  (ribi=gr->get_weg_ribi_unmasked(wt))==0) {
							break;
						}
						start_pos = test_pos;
					}
					// now add all of them
					while(ribi&3) {
						koord3d test_pos = start_pos-delta;
						grund_t *gr = welt->lookup(test_pos);
						old_platform.append(start_pos);
						if(!gr  ||  !gr->is_halt()  ||  (ribi=gr->get_weg_ribi_unmasked(wt))==0) {
							break;
						}
						start_pos = test_pos;
					}
				}
			}

			// first, check convoi without line
			FOR(vector_tpl<convoihandle_t>, const cnv, welt->convoys()) {
				// check line and owner
				if(!cnv->get_line().is_bound()  &&  cnv->get_owner()==player) {
					schedule_t *fpl = cnv->get_schedule();
					// check waytype
					if(fpl  &&  fpl->ist_halt_erlaubt(bd)) {
						bool updated = false;
						FOR(minivec_tpl<linieneintrag_t>, & k, fpl->eintrag) {
							if ((catch_all_halt && haltestelle_t::get_halt( k.pos, cnv->get_owner()) == last_halt) ||
									old_platform.is_contained(k.pos)) {
								k.pos   = pos;
								updated = true;
							}
						}
						if(updated) {
							fpl->cleanup();
							// Knightly : remove lineless convoy from old stop
							if(  last_halt.is_bound()  ) {
								last_halt->remove_convoy(cnv);
							}
							// Knightly : register lineless convoy at new stop
							if(  new_halt.is_bound()  ) {
								new_halt->add_convoy(cnv);
							}
							if(  !fpl->ist_abgeschlossen()  ) {
								// schedule is not owned by schedule window ...
								// ... thus we can set this schedule
								cnv->set_schedule(fpl);
								// otherwise the schedule window will reset it
							}
						}
					}
				}
			}
			// next, check lines serving old_halt (no owner check needed for own lines ...
			vector_tpl<linehandle_t>lines;
			player->simlinemgmt.get_lines(simline_t::line,&lines);
			FOR(vector_tpl<linehandle_t>, const line, lines) {
				schedule_t *fpl = line->get_schedule();
				// check waytype
				if(fpl->ist_halt_erlaubt(bd)) {
					bool updated = false;
					FOR(minivec_tpl<linieneintrag_t>, & k, fpl->eintrag) {
						// ok!
						if ((catch_all_halt && haltestelle_t::get_halt( k.pos, line->get_owner()) == last_halt) ||
								old_platform.is_contained(k.pos)) {
							k.pos   = pos;
							updated = true;
						}
					}
					// update line
					if(updated) {
						fpl->cleanup();
						// remove line from old stop is needed at here
						if(last_halt.is_bound()) {
							last_halt->remove_line(line);
						}
						player->simlinemgmt.update_line(line);
					}
				}
			}
		}
		// since factory connections may have changed
		welt->set_schedule_counter();
	}
	return NULL;
}


char const* tool_daynight_level_t::get_tooltip(player_t const*) const
{
	if (!strempty(default_param)) {
		if(default_param[0]=='+'  ||  default_param[0]=='-') {
			sprintf(toolstr, "%s %s",
			translator::translate("1LIGHT_CHOOSE"),
			&default_param[0]);
			return toolstr;
		}
		else {
			return translator::translate("Toggle day/night view");
		}
	}
	else {
		return "";
	}
}

bool tool_daynight_level_t::init( player_t * ) {
	if(grund_t::underground_mode==grund_t::ugm_all  ||  env_t::night_shift) {
		return false;
	}
	if (!strempty(default_param)) {
		if(default_param[0]=='+'  &&  env_t::daynight_level > 0) {
			// '+': fade in one level
			env_t::daynight_level = env_t::daynight_level-1;
		}
		else if (default_param[0]=='-') {
			// '-': fade out one level
			env_t::daynight_level = env_t::daynight_level+1;
		}
		else {
			// number: toggle number/0. 4 or 5 is good for night
			const sint8 level = atoi(default_param);
			env_t::daynight_level = (env_t::daynight_level==0) ? level : 0;
		}
	}
	return false;
}



/* make all tiles of this player a public stop
 * if this player is public, make all connected tiles a public stop */
bool tool_make_stop_public_t::init( player_t * )
{
	win_set_static_tooltip( NULL );
	return true;
}

const char* tool_make_stop_public_t::get_tooltip(const player_t *) const
{
	sint32 const cost = (welt->get_settings().maint_building * 60);
	sprintf(toolstr, translator::translate("make stop public (or join with public stop next) costs %i per tile and level"), cost);
	return toolstr;
}

const char *tool_make_stop_public_t::move( player_t *player, uint16, koord3d p )
{
	win_set_static_tooltip( NULL );
	const grund_t *gr = welt->lookup(p);
	if(gr!=NULL) {
		halthandle_t halt = gr->get_halt();
		if(  halt.is_bound()  &&  player_t::check_owner(halt->get_owner(),player)  &&  halt->get_owner()!=welt->get_player(1) ) {
			sint64 costs = halt->calc_maintenance();
			// set only tooltip if it costs (us)
			if(costs>0) {
				win_set_static_tooltip( tooltip_with_price("Building costs estimates", costs*60 ) );
			}
		}
		else {
			if(  gr->get_typ()==grund_t::brueckenboden  ||  gr->get_grund_hang()!=hang_t::flach  ) {
				// not making ways public on bridges or slopes
				return "No suitable ground!";
			}
			weg_t *w = gr->get_weg_nr(0);
			// no need for action if already player(1) => XOR ...
			if(  !(w  &&  (  (w->get_owner()==player)  ^  (player==welt->get_player(1))  ))  ) {
				w = gr->get_weg_nr(1);
				if(  !(w  &&  (  (w->get_owner()==player)  ^  (player==welt->get_player(1))  ))  ) {
					w = NULL;
				}
			}
			if(  w  ) {
				// no public way with signs
				if(  w->has_sign()  ) {
					return "No suitable ground!";
				}
				sint64 costs = w->get_besch()->get_wartung();
				if(  gr->ist_im_tunnel()  ) {
					tunnel_t *t = gr->find<tunnel_t>();
					costs = t->get_besch()->get_wartung();
				}
				// set only tooltip if it costs (us)
				if(costs>0) {
					win_set_static_tooltip( tooltip_with_price("Building costs estimates", -costs*60 ) );
				}

			}
		}
	}
	return NULL;
}

const char *tool_make_stop_public_t::work( player_t *player, koord3d p )
{
	const grund_t *gr = welt->lookup(p);
	if(  !gr  ||  !gr->get_halt().is_bound()  ||  gr->get_halt()->get_owner()==welt->get_player(1)  ) {
		weg_t *w = NULL;
		//convert a way here, if there is no halt or already public halt
		{
			if(  gr->get_typ()==grund_t::brueckenboden  ||  gr->get_grund_hang()!=hang_t::flach  ) {
				// not making ways public on bridges or slopes
				return "No suitable ground!";
			}
			w = gr->get_weg_nr(0);
			// no need for action if already player(1) => XOR ...
			if(  !(w  &&  (  (w->get_owner()==player)  ^  (player==welt->get_player(1))  ))  ) {
				w = gr->get_weg_nr(1);
				if(  !(w  &&  (  (w->get_owner()==player)  ^  (player==welt->get_player(1))  ))  ) {
					w = NULL;
				}
			}
			if(  w  ) {
				// no public way with signs
				if(  w->has_sign()  ) {
					return "No suitable ground!";
				}
				// change maintenance and ownership
				sint32 costs = w->get_besch()->get_wartung();
				if(  gr->ist_im_tunnel()  ) {
					tunnel_t *t = gr->find<tunnel_t>();
					costs = t->get_besch()->get_wartung();
					t->set_owner( welt->get_player(1) );
				}
				player_t::add_maintenance( w->get_owner(), -costs, w->get_besch()->get_finance_waytype() );
				player_t::book_construction_costs(   w->get_owner(), -costs*60, gr->get_pos().get_2d(), w->get_besch()->get_finance_waytype());
				player_t::book_construction_costs( welt->get_player(1), costs*60, koord::invalid, w->get_besch()->get_finance_waytype());
				w->set_owner( welt->get_player(1) );
				w->set_flag(obj_t::dirty);
				player_t::add_maintenance( welt->get_player(1), costs, w->get_besch()->get_finance_waytype() );
				// now search for wayobjects
				for(  uint8 i=1;  i<gr->get_top();  i++  ) {
					if(  wayobj_t *wo = obj_cast<wayobj_t>(gr->obj_bei(i))  ) {
						costs = wo->get_besch()->get_wartung();
						player_t::add_maintenance( wo->get_owner(), -costs, w->get_besch()->get_finance_waytype() );
						player_t::book_construction_costs(wo->get_owner(), -costs*60, gr->get_pos().get_2d(), w->get_waytype());
						wo->set_owner( welt->get_player(1) );
						wo->set_flag(obj_t::dirty);
						player_t::add_maintenance( welt->get_player(1), costs, w->get_besch()->get_finance_waytype() );
						player_t::book_construction_costs( welt->get_player(1), costs*60, koord::invalid, w->get_waytype());
					}
				}
				// and add message
				if(  player->get_player_nr()!=1  &&  env_t::networkmode  ) {
					cbuffer_t buf;
					buf.printf( translator::translate("(%s) now public way."), w->get_pos().get_str() );
					welt->get_message()->add_message( buf, w->get_pos().get_2d(), message_t::ai, PLAYER_FLAG|player->get_player_nr(), IMG_LEER );
				}
			}
		}
		if(  w==NULL  ) {
			return "No stop here!";
		}
	}
	else {
		halthandle_t halt = gr->get_halt();
		if(  !(player_t::check_owner(halt->get_owner(),player)  ||  halt->get_owner()==welt->get_player(1))  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		else {
			halt->make_public_and_join(player);
		}
	}
	return NULL;
}



bool tool_show_trees_t::init( player_t * )
{
	env_t::hide_trees = !env_t::hide_trees;
	baum_t::recalc_outline_color();
	welt->set_dirty();
	return false;
}


sint8 tool_show_underground_t::save_underground_level = -128;

bool tool_show_underground_t::init( player_t * )
{
	koord3d zpos = welt->get_zeiger()->get_pos();
	// move zeiger (pointer) to invalid position -> unmark tiles
	welt->get_zeiger()->change_pos( koord3d::invalid);

	sint8 old_underground_level = grund_t::underground_level;

	// map needs update?
	bool ok = true;
	// need an extra click?
	bool needs_click = false;

	// default default-param = U for backward compatibility
	if (default_param == NULL) {
		default_param = strdup("U");
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				grund_t::set_underground_mode( grund_t::ugm_none, 0);
			}
			else if(grund_t::underground_mode==grund_t::ugm_none) {
				needs_click = true;
				ok = false;
			}
			else {
				ok = false;
			}
			break;
		// decrease slice level
		case 'D':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				if(  grund_t::underground_level > welt->min_height  ) {
					grund_t::underground_level --;
				}
			}
			else {
				ok = false;
			}
			break;
		// increase slice level
		case 'I':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				if(  grund_t::underground_level < welt->max_height  ) {
					grund_t::underground_level ++;
				}
			}
			else {
				ok = false;
			}
			break;

		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			if(grund_t::underground_mode==grund_t::ugm_level) {
				// switch to normal or full-underground
				grund_t::set_underground_mode( grund_t::ugm_none, 0);
			}
			else if(grund_t::underground_mode==grund_t::ugm_none) {
				grund_t::set_underground_mode( grund_t::ugm_level, zpos.z);
			}
			else {
				ok = false;
			}
			break;

		//  switch between full underground or normal/sliced view
		case 'U':
			if (grund_t::underground_mode==grund_t::ugm_all) {
				// check if the old level is valid then switch back to sliced view
				if (-128<save_underground_level && save_underground_level<127) {
					grund_t::set_underground_mode(grund_t::ugm_level, save_underground_level);
				}
				else {
					grund_t::set_underground_mode(grund_t::ugm_none, 0);
				}
			}
			else {
				grund_t::set_underground_mode( grund_t::ugm_all, 0);
			}
			break;

		default:
			ok = false;
			dbg->error( "tool_show_underground_t::init()", "Unknown command string \"%s\"", default_param );

	}

	// move zeiger (pointer) back
	welt->get_zeiger()->change_pos( zpos);

	if (ok) {
		save_underground_level = old_underground_level;

		// renew toolbar
		tool_t::update_toolbars();

		// recalc all images on map
		welt->update_underground();
	}
	return needs_click;
}

const char *tool_show_underground_t::work( player_t *player, koord3d pos)
{
	koord3d zpos = welt->get_zeiger()->get_pos();
	// move zeiger (pointer) to invalid position -> unmark tiles
	welt->get_zeiger()->change_pos( koord3d::invalid);

	save_underground_level = grund_t::underground_level;
	grund_t::set_underground_mode( grund_t::ugm_level, pos.z);

	// move zeiger (pointer) back
	welt->get_zeiger()->change_pos( zpos);

	// renew toolbar
	tool_t::update_toolbars();

	// recalc all images on map
	welt->update_underground();

	if(player == welt->get_active_player()) {
		welt->set_tool( general_tool[TOOL_QUERY], player );
	}

	return NULL;
}


char const* tool_show_underground_t::get_tooltip(player_t const*) const
{
	// no default-param == U for backward compatibility
	if(  default_param == NULL  ) {
		return translator::translate("underground mode");
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			return translator::translate("sliced underground mode");
		// decrease slice level
		case 'D':
			return translator::translate("decrease underground view level");
		// increase slice level
		case 'I':
			return translator::translate("increase underground view level");
		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			return translator::translate("sliced underground mode");
		//  switch between full underground or normal/sliced view
		case 'U':
		default:
			return translator::translate("underground mode");
	}
}

bool tool_show_underground_t::is_selected() const
{
	// default default-param = U for backward compatibility
	if(  default_param == NULL  ) {
		return grund_t::underground_mode==grund_t::ugm_all;
	}
	// now check the default parameter
	switch(default_param[0]) {
		// toggle sliced view by toolbar - height taken from extra mouse click
		case 'C':
			return grund_t::underground_mode==grund_t::ugm_level;
		// decrease slice level
		case 'D':
			return false;
		// increase slice level
		case 'I':
			return false;
		// toggle sliced view by keyboard - height taken from cursor
		case 'K':
			return grund_t::underground_mode==grund_t::ugm_level;
		//  switch between full underground or normal/sliced view
		case 'U':
			return grund_t::underground_mode==grund_t::ugm_all;
	}
	return false;
}

void tool_show_underground_t::draw_after(scr_coord k, bool dirty) const
{
	if(  icon!=IMG_LEER  &&  is_selected()  ) {
		display_img_blend( icon, k.x, k.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, dirty );
		// additionally show level in sliced mode
		if(  default_param!=NULL  &&  grund_t::underground_mode==grund_t::ugm_level  ) {
			char level_str[16];
			sprintf( level_str, "%i", grund_t::underground_level );
			display_proportional( k.x+4, k.y+4, level_str, ALIGN_LEFT, COL_YELLOW, true );
		}
	}
}


void tool_rotate90_t::draw_after(scr_coord pos, bool dirty) const
{
	if(  !env_t::networkmode  ) {
		if(  skinverwaltung_t::compass_map  ) {
			display_img_aligned( skinverwaltung_t::compass_map->get_bild_nr( welt->get_settings().get_rotation()+4 ), scr_rect(pos,env_t::iconsize), ALIGN_CENTER_V|ALIGN_CENTER_H, false );
		}
		tool_t::draw_after( pos, dirty );
	}
}

bool tool_rotate90_t::init( player_t * )
{
	if(  !env_t::networkmode  ) {
		welt->rotate90();
		welt->update_map();
	}
	return false;
}


bool tool_quit_t::init( player_t * )
{
	destroy_all_win( true );
	welt->stop( true );
	return false;
}


bool tool_screenshot_t::init( player_t * )
{
	if(  is_ctrl_pressed()  ) {
		if(  const gui_frame_t * topwin = win_get_top()  ) {
			const scr_coord k = win_get_pos(topwin);
			const scr_size size = topwin->get_windowsize();
			display_snapshot( k.x, k.y, size.w, size.h );
		}
		else {
			display_snapshot( 0, 0, display_get_width(), display_get_height() );
		}
	}
	else {
		display_snapshot( 0, 0, display_get_width(), display_get_height() );
	}
	create_win( new news_img("Screenshot\ngespeichert.\n"), w_time_delete, magic_none);
	return false;
}


bool tool_undo_t::init( player_t *player )
{
	if(!player->undo()  &&  can_use_gui()) {
		create_win( new news_img("UNDO failed!"), w_time_delete, magic_none);
	}
	return false;
}


bool tool_increase_industry_t::init( player_t * )
{
	fabrikbauer_t::increase_industry_density( false );
	return false;
}


bool tool_zoom_in_t::init( player_t * )
{
	win_change_zoom_factor(true);
	welt->set_dirty();
	return false;
}


bool tool_zoom_out_t::init( player_t * )
{
	win_change_zoom_factor(false);
	welt->set_dirty();
	return false;
}

/************************* internal tools, only need for networking ***************/

static bool scenario_check_schedule(karte_t *welt, player_t *player, schedule_t *schedule, bool local)
{
	if (!is_scenario()) {
		return true;
	}
	const char* err = welt->get_scenario()->is_schedule_allowed(player, schedule);
	if (err) {
		if (*err  &&  local) {
			create_win( new news_img(err), w_time_delete, magic_none);
		}
		return false;
	}
	return true;
}


/* Handles all action of convois in depots. Needs a default param:
 * [function],[convoi_id],addition stuff
 * following simple command exists:
 * 'x' : self destruct
 * 'f' : open the schedule window
 * 'g' : apply a schedule
 * 'n' : toggle 'no load'
 * 'w' : toggle withdraw
 * 's' : change state to [number] (and maybe set open schedule flag)
 * 'l' : apply new line [number]
 * 'd' : go to nearest depot
 */
bool tool_change_convoi_t::init( player_t *player )
{
	char tool=0;
	uint16 convoi_id = 0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%hi", &tool, &convoi_id );

	// skip to the commands ...
	for(  int z = 2;  *p  &&  z>0;  p++  ) {
		if(  *p==','  ) {
			z--;
		}
	}

	convoihandle_t cnv;
	cnv.set_id( convoi_id );
	// double click on remove button will send two such commands
	// the first will delete the convoi, the second should not trigger an assertion
	// catch such commands here
	if( !cnv.is_bound()) {
#if DEBUG>=4
		if (can_use_gui()) {
			create_win( new news_img("Convoy already deleted!"), w_time_delete, magic_none);
		}
#endif
		dbg->warning("tool_change_convoi_t::init", "no convoy with id=%d found", convoi_id);
		return false;
	}
	// ownership check for network games
	if (cnv.is_bound()  &&  env_t::networkmode  &&  !player_t::check_owner(cnv->get_owner(), player)) {
		return false;
	}

	// first letter is now the actual command
	switch(  tool  ) {
		case 'x': // self destruction ...
			if(cnv.is_bound()) {
				if (cnv->get_state()==convoi_t::INITIAL) {
					// delete cnv in depot
					if (grund_t *gr = welt->lookup(cnv->get_pos())) {
						if (depot_t *dep = gr->get_depot()) {
							dep->disassemble_convoi(cnv, true);
							return false;
						}
					}
				}
				cnv->self_destruct();
			}
			return false;

		case 'f': // open schedule
			{
				// we open the window only when executed on the same client that triggered the tool
				// but the all clients must call the function anyway
				cnv->open_schedule_window( can_use_gui() );
			}
			break;

		case 'g': // change schedule
			{
				schedule_t *fpl = cnv->create_schedule()->copy();
				fpl->eingabe_abschliessen();
				if (fpl->sscanf_schedule( p )  &&  scenario_check_schedule(welt, player, fpl, can_use_gui())) {
					cnv->set_schedule( fpl );
				}
				else {
					// could not read schedule, do not assign
					delete fpl;
				}
			}
			break;

		case 'l': // change line
			{
				// read out id and new aktuell (actual) index
				uint16 id=0, aktuell=0;
				int count=sscanf( p, "%hi,%hi", &id, &aktuell );
				linehandle_t l;
				l.set_id( id );
				if(  l.is_bound()  ) {
					// sanity check for right line-type (compare schedule types ..)
					schedule_t *fpl = cnv->create_schedule();
					if(  fpl  &&  l->get_schedule()  &&  fpl->get_type()!=l->get_schedule()->get_type()  ) {
						dbg->warning("tool_change_convoi_t::init", "types of convoi and line do not match");
						return false;
					}
					if(  count==1 ) {
						// aktuell was not supplied -> take it from line schedule
						aktuell = l->get_schedule()->get_aktuell();
					}
					cnv->set_line( l );
					cnv->get_schedule()->set_aktuell((uint8)aktuell);
					cnv->get_schedule()->eingabe_abschliessen();
				}
			}
			break;

		case 'n': // change no_load
			cnv->set_no_load( !cnv->get_no_load() );
			if(  !cnv->get_no_load()  ) {
				cnv->set_withdraw( false );
			}
			break;

		case 's': // change state
			{
				int new_state = atoi(p);
				if(  new_state>0  ) {
					cnv->set_state( new_state );
					if(  new_state==convoi_t::FAHRPLANEINGABE  ) {
						cnv->get_schedule()->eingabe_beginnen();
					}
				}
			}
			break;

		case 'w': // change withdraw
			cnv->set_withdraw( !cnv->get_withdraw() );
			cnv->set_no_load( cnv->get_withdraw() );
			break;

		case 'd': // goto depot
		{
			const char* msg = cnv->send_to_depot(is_local_execution());

			if (is_local_execution()) {
				create_win( new news_img(msg), w_time_delete, magic_none);
			}
		}
	}

	if(  cnv->in_depot()  &&  (tool=='g'  ||  tool=='l')  ) {
		const grund_t *const ground = welt->lookup( cnv->get_home_depot() );
		if(  ground  ) {
			const depot_t *const depot = ground->get_depot();
			if(  depot  ) {
				depot_frame_t *const frame = dynamic_cast<depot_frame_t *>( win_get_magic( (ptrdiff_t)depot ) );
				if(  frame  ) {
					frame->update_data();
				}
			}
		}
	}


	return false;	// no related work tool ...
}



/* Handles all action of lines. Needs a default param:
 * [function],[line_id],addition stuff
 * following simple command exists:
 * 'g' : apply new schedule to line [schedule follows]
 */
bool tool_change_line_t::init( player_t *player )
{
	uint16 line_id = 0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}

	char tool=*p++;
	while(  *p  &&  *p++!=','  ) {
	}
	if(  *p==0  ) {
		dbg->warning( "tool_change_line_t::init()", "too short command \"%s\"", default_param );
		return false;
	}

	line_id = atoi(p);
	while(  *p  &&  *p++!=','  ) {
	}

	linehandle_t line;
	line.set_id( line_id );

	// ownership check for network games
	if (line.is_bound()  &&  env_t::networkmode  && !player_t::check_owner(line->get_owner(), player)) {
		return false;
	}

	// first letter is now the actual command
	switch(  tool  ) {
		case 'c': // create line, next parameter line type and magic of schedule window (only right window gets updated)
			{
				line = player->simlinemgmt.create_line( atoi(p), player );
				while(  *p  &&  *p++!=','  ) {
				}
				long t;
				sscanf( p, "%ld", &t );
				while(  *p  &&  *p++!=','  ) {
				}

				// no need to check schedule for scenario conditions, as schedule is only copied
				line->get_schedule()->sscanf_schedule( p );
				line->get_schedule()->eingabe_abschliessen();	// just in case ...
				if(  can_use_gui()  ) {
					fahrplan_gui_t *fg = dynamic_cast<fahrplan_gui_t *>(win_get_magic((ptrdiff_t)t));
					if(  fg  ) {
						fg->init_line_selector();
					}
					schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic(magic_line_management_t+player->get_player_nr()));
					if(  sl  ) {
						sl->show_lineinfo( line );
					}
					// no schedule window open => then open one
					if(  fg==NULL  ) {
						create_win( new line_management_gui_t(line, player), w_info, (ptrdiff_t)line.get_rep() );
					}
				}
			}
			break;

		case 'd':	// delete line
			{
				if (line.is_bound()  &&  line->count_convoys()==0) {
					// close a schedule window, if still active
					gui_frame_t *w = win_get_magic( (ptrdiff_t)line.get_rep() );
					if(w) {
						destroy_win( w );
					}
					player->simlinemgmt.delete_line(line);
				}
			}
			break;

		case 'g': // change schedule
			{
				if (line.is_bound()) {
					schedule_t *fpl = line->get_schedule()->copy();
					if (fpl->sscanf_schedule( p )  &&  scenario_check_schedule(welt, player, fpl, can_use_gui()) ) {
						fpl->eingabe_abschliessen();
						line->set_schedule( fpl );
						line->get_owner()->simlinemgmt.update_line(line);
					}
					else {
						// could not read schedule, do not assign
						delete fpl;
					}
				}
			}
			break;

		case 't':	// trims away convois on all lines of linetype with this default parameter
			{
				vector_tpl<linehandle_t> const& lines = player->simlinemgmt.get_line_list();
				// what kind of lines to trim
				const simline_t::linetype linetype = (simline_t::linetype)atoi(p);
				while(  *p  &&  *p++!=','  ) {
				}
				// how much as target
				sint32 percentage = *p ? atoi(p) : 10;
				if(  percentage == 0  ) {
					break;
				}

				FOR(vector_tpl<linehandle_t>,line,lines) {
					if(  line->get_linetype() == linetype  &&  line->get_convoys().get_count() > 2  ) {
						// correct waytpe and more than one,n now some up usage for the last six months
						sint64 transported = 0, capacity = 0;
						for(  int i=0;  i<6;  i++  ) {
							capacity += line->get_finance_history( i , LINE_CAPACITY );
							transported += line->get_finance_history( i , LINE_TRANSPORTED_GOODS );
						}
						// sanity check for non-moving lines
						if(  capacity == 0  ) {
							continue;
						}

						if(  (transported*100) / capacity < percentage  ) {
							// less than 33 % usage => remove concois
							vector_tpl<convoihandle_t> const& cnvs = line->get_convoys();
							sint64 old_sum_capacity = 0;
							FOR(vector_tpl<convoihandle_t>,cnv,cnvs) {
								for(  int i=0;  i<cnv->get_vehikel_anzahl();  i++  ) {
									old_sum_capacity += cnv->get_vehikel(i)->get_besch()->get_zuladung();
								}
							}
							/* now we have the total capacity. We will now remove convois until this capacity
							 * is reduced by the ration suggested from transported = 1/3 of total capacity
							 * x is the percentage of used capacity
							 * Then is the new target sum_capacity = x*old_sum_capacity
							 */
							sint64 new_sum_capacity = (transported * 1000 * old_sum_capacity) / (capacity * percentage * 10);

							// first we remove the totally empty convois (nowbody will miss them)
							int destroyed = 0;
							const int initial = line->get_convoys().get_count();
							const int max_left = (initial+2) / 2;

							for(  int j=initial-1;  j >= 0  &&  initial-destroyed > max_left  &&  new_sum_capacity < old_sum_capacity;  j--  ) {
								convoihandle_t cnv = line->get_convoy(j);
								if(  cnv->get_state() == convoi_t::INITIAL  ||  cnv->get_state() >= convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH  ) {
									for(  int i=0;  i<cnv->get_vehikel_anzahl();  i++  ) {
										old_sum_capacity -= cnv->get_vehikel(i)->get_besch()->get_zuladung();
									}
									cnv->self_destruct();
									destroyed ++;
								}
							}

							// not enough? Then remove from the end ...
							for(  uint j=0;  j < line->get_convoys().get_count()  &&  initial-destroyed > max_left  &&  new_sum_capacity < old_sum_capacity;  j++  ) {
								convoihandle_t cnv = line->get_convoy(j);
								if(  cnv->get_state() != convoi_t::SELF_DESTRUCT  ) {
									for(  int i=0;  i<cnv->get_vehikel_anzahl();  i++  ) {
										old_sum_capacity -= cnv->get_vehikel(i)->get_besch()->get_zuladung();
									}
									cnv->self_destruct();
									destroyed ++;
								}
							}
							// done
							if(  destroyed  ) {
								dbg->message( "tool_change_line_t::init", "trim line %s: Reduced from %d to %d convois", line->get_name(), initial, initial-destroyed );
							}
						}
					}
				}
			}
			break;

		case 'u':	// unite all lineless convois with similar schedules
			{
				array_tpl<vector_tpl<convoihandle_t> > cnvs(welt->convoys().get_count());
				uint32 max_cnvs=0;
				FOR(vector_tpl<convoihandle_t>, cnv, welt->convoys()) {
					// only check lineless convoys
					if(  !cnv->get_line().is_bound()  ) {
						bool found = false;
						// check, if already matches existing convois schedule
						for(  uint32 i=0;  i<max_cnvs  &&  !found;  i++  ) {
							FOR(vector_tpl<convoihandle_t>, cnvcomp, cnvs[i] ) {
								if(  cnvcomp->get_schedule()->matches( welt, cnv->get_schedule() )  ) {
									found = true;
									cnvs[i].append( cnv );
									break;
								}
							}
						}
						// not added: then may be new line for this?
						if(  !found  ) {
							cnvs[max_cnvs++].append( cnv );
						}
					}
				}
				// now we have an array of one or more lineless convois
				for(  uint32 i=0;  i<max_cnvs;  i++  ) {
					// if there is more than one convois => new line
					if(  cnvs[i].get_count()>1  ) {
						line = player->simlinemgmt.create_line( cnvs[i][0]->get_schedule()->get_type(), player, cnvs[i][0]->get_schedule() );
						FOR(vector_tpl<convoihandle_t>, cnv, cnvs[i] ) {
							line->add_convoy( cnv );
							cnv->set_line( line );
						}
					}
				}
			}
			break;

		case 'w': // change widthdraw
			{
				if (line.is_bound()) {
					line->set_withdraw( atoi(p) );
				}
			}
			break;
	}
	return false;
}



/* Handles all action of convois in depots. Needs a default param:
 * [function],[depot_pos_3d],[convoi_id],addition stuff
 * following simple command exists:
 * 'l' : creates a new line (convoi_id might be invalid) (+printf'd initial schedule)
 * 'b' : starts the convoi
 * 'B' : starts all convoys
 * 'c' : copies this convoi
 * 'd' : disassembles convoi
 * 's' : sells convoi
 * 'a' : appends a vehicle (+vehikel_name) uses the oldest
 * 'i' : inserts a vehicle in front (+vehikel_name) uses the oldest
 * 's' : sells a vehikel (+vehikel_name) uses the newest
 * 'r' : removes a vehikel (+number in convoi)
 * 'R' : removes all vehikels including (+number in convoi) to end
 */
bool tool_change_depot_t::init( player_t *player )
{
	char tool=0;
	koord3d pos = koord3d::invalid;
	sint16 z;
	uint16 convoi_id;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%hi,%hi,%hi,%hi", &tool, &pos.x, &pos.y, &z, &convoi_id );
	pos.z = (sint8)z;

	// skip to the commands ...
	z = 5;
	while(  *p  &&  z>0  ) {
		if(  *p==','  ) {
			z--;
		}
		p++;
	}

	grund_t *gr = welt->lookup(pos);
	depot_t *depot = gr ? gr->get_depot() : NULL;
	if(  depot==NULL  ){
		dbg->warning("tool_change_depot_t::init", "no depot found at (%s)", pos.get_str());
		return false;
	}
	if(  !player_t::check_owner( depot->get_owner(), player)  ) {
		dbg->warning("tool_change_depot_t::init", "depot at (%s) belongs to another player", pos.get_str());
		return false;
	}

	convoihandle_t cnv;
	cnv.set_id( convoi_id );

	// ok now do our stuff
	switch(  tool  ) {
		case 'l': { // create line schedule window
			linehandle_t selected_line = depot->get_owner()->simlinemgmt.create_line(depot->get_line_type(),depot->get_owner());
			// no need to check schedule for scenario conditions, as schedule is only copied
			selected_line->get_schedule()->sscanf_schedule( p );

			depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (ptrdiff_t)depot ));
			if(  can_use_gui()  ) {
				if(  welt->get_active_player()==player  &&  depot_frame  ) {
					create_win( new line_management_gui_t( selected_line, depot->get_owner() ), w_info, (ptrdiff_t)selected_line.get_rep() );
				}
			}

			if(  depot_frame  ) {
				if(  can_use_gui()  ) {
					depot_frame->set_selected_line( selected_line );
					depot_frame->apply_line();
				}
				depot_frame->update_data();
			}

			schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic( magic_line_management_t + player->get_player_nr() ));
			if(  sl  ) {
				sl->update_data( selected_line );
			}
			DBG_MESSAGE("depot_frame_t::new_line()","id=%d",selected_line.get_id() );
			break;
		}
		case 'b': { // start a convoi from the depot
			if(  cnv.is_bound()  ) {
				depot->start_convoi(cnv, can_use_gui());
			}
			break;
		}
		case 'B': { // start all convoys
			depot->start_all_convoys();
			break;
		}
		case 'd':   // disassemble convoi
		case 'v': { // sell convoi
			depot->disassemble_convoi( cnv, tool=='v' );
			break;
		}
		case 'c': { // copy this convoi
			if(  cnv.is_bound()  ) {
				if(  convoihandle_t::is_exhausted()  ) {
					if(  can_use_gui()  ) {
						create_win( new news_img("Convoi handles exhausted!"), w_time_delete, magic_none );
					}
					return false;
				}
				depot->copy_convoi( cnv, can_use_gui() );
			}
			break;
		}
		case 'a':   // append a vehicle
		case 'i':   // insert a vehicle in front
		case 's':   // sells a vehicle
		case 'r':   // removes a vehicle (assumes a valid depot)
		case 'R': { // removes all vehicles to end (assumes a valid depot)
			if(  tool=='r'  ||  tool=='R'  ) {
				// test may fail after double-click on the button:
				// two remove cmds are sent, only the first will remove, the second should not trigger assertion failure
				if ( cnv.is_bound() ) {
					int start_nr = atoi(p);
					int nr = start_nr;

					// find end
					while(nr<cnv->get_vehikel_anzahl()) {
						const vehikel_besch_t *info = cnv->get_vehikel(nr)->get_besch();
						nr ++;
						if(info->get_nachfolger_count()!=1) {
							break;
						}
					}
					// now remove the vehicles
					if(  cnv->get_vehikel_anzahl()==nr-start_nr  ||  (tool=='R'  &&  start_nr==0)  ) {
						depot->disassemble_convoi(cnv, false);
					}
					else if(  tool=='R'  ) {
						depot->remove_vehicles_to_end( cnv, start_nr );
					}
					else {
						for(  int i=start_nr;  i<nr;  i++  ) {
							depot->remove_vehicle(cnv, start_nr);
						}
					}
				}
			}
			else {
				// create and append it
				const vehikel_besch_t *info = vehikelbauer_t::get_info( p );
				// we have a valid vehicle there => now check for details
				if(  info  ) {
					// we buy/sell all vehicles together!
					slist_tpl<const vehikel_besch_t *>new_vehicle_info;
					const vehikel_besch_t *start_info = info;

					if(tool!='a') {
						// start of composition
						while(  info->get_vorgaenger_count() == 1  &&  info->get_vorgaenger(0) != NULL  &&  !new_vehicle_info.is_contained(info)  ) {
							info = info->get_vorgaenger(0);
							new_vehicle_info.insert(info);
						}
						info = start_info;
					}
					while(info) {
						new_vehicle_info.append( info );
						if(info->get_nachfolger_count() != 1  ||  (tool == 'i'  &&  info == start_info)  ||  new_vehicle_info.is_contained(info->get_nachfolger(0))  ) {
							break;
						}
						info = info->get_nachfolger(0);
					}
					// now we have a valid composition together
					if(  tool=='s'  ) {
						while(  new_vehicle_info.get_count()  ) {
							// We sell the newest vehicle - gives most money back.
							vehicle_t* veh = depot->find_oldest_newest(new_vehicle_info.remove_first(), false);
							if(veh != NULL) {
								depot->sell_vehicle(veh);
							}
						}
					}
					else {
						// now check if we are allowed to buy this (we test only leading vehicle, so one can still buy hidden stuff)
						info = new_vehicle_info.front();
						if(  !info->is_available(welt->get_timeline_year_month())  &&  !welt->get_settings().get_allow_buying_obsolete_vehicles()  ) {
							// only allow append/insert, if in depot do not create new obsolete vehicles
							if(  !depot->find_oldest_newest(info, true)  ) {
								// just fail silent
								return false;
							}
						}

						// append/insert into convoi; create one if needed
						depot->clear_command_pending();
						if(  !cnv.is_bound()  ) {
							if(  convoihandle_t::is_exhausted()  ) {
								if(  can_use_gui()  ) {
									create_win( new news_img("Convoi handles exhausted!"), w_time_delete, magic_none);
								}
								return false;
							}
							// create a new convoi
							cnv = depot->add_convoi( can_use_gui() );
							cnv->set_name( new_vehicle_info.front()->get_name() );
						}

						// now we have a valid cnv
						if(  cnv->get_vehikel_anzahl()+new_vehicle_info.get_count() <= depot->get_max_convoi_length()  ) {

							for(  unsigned i=0;  i<new_vehicle_info.get_count();  i++  ) {
								// insert/append needs reverse order
								unsigned nr = (tool=='i') ? new_vehicle_info.get_count()-i-1 : i;
								// We add the oldest vehicle - newer stay for selling
								const vehikel_besch_t* vb = new_vehicle_info.at(nr);
								vehicle_t* veh = depot->find_oldest_newest(vb, true);
								if(  veh == NULL  ) {
									// nothing there => we buy it
									veh = depot->buy_vehicle(vb);
								}
								depot->append_vehicle( cnv, veh, tool=='i', can_use_gui() );
							}
						}
					}
				}
			}
			depot->update_win();
			break;
		}
	}
	return false;
}


/* Handles all player stuff default_param:
 * [function],[player_id],[state]
 * following command exists:
 * 'a' : activate/deactivate player (depends on state)
 * 'c' : change player color
 * '$' : change bank account
 */
bool tool_change_player_t::init( player_t *player_in)
{
	if(  default_param==NULL  ) {
		dbg->warning( "tool_change_player_t::init()", "nothing to do!" );
		return false;
	}

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}

	char tool = 0;
	int id = 0;
	if (sscanf( p, "%c,%i", &tool, &id ) < 2) {
		return false; // invalid command
	}

	player_t *player = welt->get_player(id);

	// ok now do our stuff
	switch(  tool  ) {
		case 'a': // activate/deactivate AI
			if(  player  &&  player->get_ai_id()!=player_t::HUMAN  &&  (player_in==welt->get_player(1)  ||  !env_t::networkmode)  ) {
				int state = 0;
				if (sscanf( p, "%c,%i,%i", &tool, &id, &state ) == 3) {
					player->set_active(state);
					welt->get_settings().set_player_active(id, player->is_active());
				}
			}
			break;
		case 'c': // change player color
			if(  player  &&  player==player_in  ) {
				int c1, c2, dummy;
				sscanf( p, "%c,%i,%i,%i", &tool, &dummy, &c1, &c2 );
				player->set_player_color( c1, c2 );
			}
			break;

		case '$': // change bank account
			if(  player  &&  player_in==welt->get_player(1) ) {
				int delta;
				if (sscanf(p, "%c,%i,%i", &tool, &id, &delta) == 3) {
					player->get_finance()->book_account(delta);
				}
			}
			break;

		case 'n': // WAS: new player with type state
		case 'f': // WAS: activate/deactivate freeplay
			dbg->error( "tool_change_player_t::init()", "deprecated command called" );
			break;
	}

	// update the window
	ki_kontroll_t* playerwin = (ki_kontroll_t*)win_get_magic(magic_ki_kontroll_t);
	if (playerwin) {
		playerwin->update_data();
	}

	return false;
}


/* Sets traffic light phases via default_param:
 * [pos],[ns_flag],[ticks]
 */
bool tool_change_traffic_light_t::init( player_t *player )
{
	koord3d pos;
	sint16 z, ns, ticks;
	if(  5!=sscanf( default_param, "%hi,%hi,%hi,%hi,%hi", &pos.x, &pos.y, &z, &ns, &ticks )  ) {
		return false;
	}
	pos.z = (sint8)z;
	if(  grund_t *gr = welt->lookup(pos)  ) {
		if( roadsign_t *rs = gr->find<roadsign_t>()  ) {
			if(  (  rs->get_besch()->is_traffic_light()  ||  rs->get_besch()->is_private_way()  )  &&  player_t::check_owner(rs->get_owner(),player)  ) {
				if(  ns == 1  ) {
					rs->set_ticks_ns( (uint8)ticks );
				}
				else if(  ns == 0  ) {
					rs->set_ticks_ow( (uint8)ticks );
				}
				else if(  ns == 2  ) {
					rs->set_ticks_offset( (uint8)ticks );
				}
				// update the window
				if(  rs->get_besch()->is_traffic_light()  ) {
					trafficlight_info_t* trafficlight_win = (trafficlight_info_t*)win_get_magic((ptrdiff_t)rs);
					if (trafficlight_win) {
						trafficlight_win->update_data();
					}
				}
				else {
					privatesign_info_t* trafficlight_win = (privatesign_info_t*)win_get_magic((ptrdiff_t)rs);
					if (trafficlight_win) {
						trafficlight_win->update_data();
					}
				}
			}
		}
	}
	return false;
}


/**
 * change city:
 * g[x],[y],[allow_city_growth]
 */
bool tool_change_city_t::init( player_t *player )
{
	if (player != welt->get_player(1)) {
		return false;
	}
	koord k;
	sint16 allow_growth;
	if(  3!=sscanf( default_param, "g%hi,%hi,%hi", &k.x, &k.y, &allow_growth )  ) {
		return false;
	}
	grund_t *gr = welt->lookup_kartenboden(k);
	if (gr) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if (gb) {
			stadt_t *st = gb->get_stadt();
			if (st) {
				st->set_citygrowth_yesno(allow_growth);
				city_info_t *stinfo = dynamic_cast<city_info_t*>(win_get_magic((ptrdiff_t)st));
				if (stinfo) {
					stinfo->update_data();
				}
			}
		}
	}
	return false;
}



/* Handles renaming of ingame entities. Needs a default param:
 * [object='c|h|l|m|t|p|f'][id|pos],[name]
 * c=convoi, h=halt, l=line,  m=marker, t=town, p=player, f=factory
 * in case of marker / factory, id is a pos3d string
 */
bool tool_rename_t::init(player_t *player)
{
	uint16 id = 0;
	koord3d pos = koord3d::invalid;

	// skip the rest of the command
	const char *p = default_param;
	const char what = *p++;
	switch(  what  ) {
		case 'h':
		case 'l':
		case 'c':
		case 't':
		case 'p':
			id = atoi(p);
			while(  *p>0  &&  *p++!=','  ) {
			}
			break;
		case 'm':
		case 'f':
			if(  3!=sscanf( p, "%hi,%hi,%hi", &pos.x, &pos.y, &id )  ) {
				dbg->error( "tool_rename_t::init", "no position given for marker/factory! (%s)", default_param );
				return false;
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			pos.z = (sint8)id;
			id = 0;
			break;
		default:
			dbg->error( "tool_rename_t::init", "illegal request! (%s)", default_param );
			return false;
	}

	// now for action ...
	switch(  what  ) {
		case 'h':
		{
			halthandle_t halt;
			halt.set_id( id );
			if(  halt.is_bound()  &&  (!env_t::networkmode  ||  player_t::check_owner(halt->get_owner(), player))  ) {
				halt->set_name( p );
				return false;
			}
			break;
		}

		case 'l':
		{
			linehandle_t line;
			line.set_id( id );
			if(  line.is_bound()  &&  (!env_t::networkmode  ||  player_t::check_owner(line->get_owner(), player))  ) {
				line->set_name( p );

				schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic(magic_line_management_t+player->get_player_nr()));
				if(  sl  ) {
					sl->update_data( line );
				}
				return false;
			}
			break;
		}

		case 'c':
		{
			convoihandle_t cnv;
			cnv.set_id( id );
			if(  cnv.is_bound()  &&  (!env_t::networkmode  ||  player_t::check_owner(cnv->get_owner(), player))  ) {
				//  set name without ID
				cnv->set_name( p, false );
				return false;
			}
			break;
		}

		case 't':
		{
			if(  player == welt->get_player(1)  &&   id<welt->get_staedte().get_count()  ) {
				welt->get_staedte()[id]->set_name( p );
				return false;
			}
			break;
		}

		case 'm':
			if(  grund_t *gr = welt->lookup(pos)  ) {
				label_t *label = gr->find<label_t>();
				if (label  &&  (!env_t::networkmode  ||  player_t::check_owner(label->get_owner(), player))  ) {
					gr->set_text(p);
				}
				return false;
			}
			break;

		case 'p': {
			player_t *other = welt->get_player((uint8)id);
			if(  other  &&  other == player  ) {
				other->set_name(p);
				return false;
			}
		}

		case 'f':
		{
			if(  player == welt->get_player(1)) {
				if(  grund_t *gr = welt->lookup(pos)  ) {
					if(  gebaeude_t* gb = gr->find<gebaeude_t>()  ) {
						if (  fabrik_t *fab = gb->get_fabrik()  ) {
							fab->set_name(p);
							return false;
						}
					}
				}
			}
		}
	}
	// we are only getting here, if we could not process this request
	dbg->warning( "tool_rename_t::init", "could not perform (%s)", default_param );
	return false;
}


/*
 * Add a message to the message queue
 */
bool tool_add_message_t::init(player_t*)
{
	if (  *default_param  ) {
		// Local message, not stored by server
		welt->get_message()->add_message( default_param, koord::invalid, message_t::general | message_t::local_flag, COL_BLACK, IMG_LEER );
	}
	return false;
}
