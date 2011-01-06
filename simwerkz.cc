/*
 * Tools for the players
 *
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "simdebug.h"
#include "simworld.h"
#include "player/simplay.h"
#include "simsound.h"
#include "simevent.h"
#include "simskin.h"
#include "simcity.h"
#include "simtools.h"
#include "simmesg.h"

#include "bauer/fabrikbauer.h"
#include "bauer/vehikelbauer.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/strasse.h"
#include "boden/wege/schiene.h"
#include "boden/wege/kanal.h"
#include "boden/tunnelboden.h"
#include "boden/monorailboden.h"

#include "simdepot.h"
#include "simfab.h"
#include "simwin.h"
#include "simimg.h"
#include "simintr.h"
#include "simhalt.h"

#include "besch/grund_besch.h"
#include "besch/haus_besch.h"
#include "besch/way_obj_besch.h"
#include "besch/skin_besch.h"
#include "besch/roadsign_besch.h"
#include "besch/tunnel_besch.h"
#include "besch/groundobj_besch.h"
#include "besch/roadsign_besch.h"

#include "vehicle/simvehikel.h"
#include "vehicle/simverkehr.h"
#include "vehicle/simpeople.h"

#include "gui/line_management_gui.h"
#include "gui/werkzeug_waehler.h"
#include "gui/station_building_select.h"
#include "gui/karte.h"	// to update map after construction of new industry
#include "gui/depot_frame.h"
#include "gui/fahrplan_gui.h"
#include "gui/signal_spacing.h"
#include "gui/stadt_info.h"
#include "gui/trafficlight_info.h"

#include "dings/zeiger.h"
#include "dings/bruecke.h"
#include "dings/tunnel.h"
#include "dings/groundobj.h"
#include "dings/signal.h"
#include "dings/crossing.h"
#include "dings/roadsign.h"
#include "dings/wayobj.h"
#include "dings/leitung2.h"
#include "dings/baum.h"
#include "dings/field.h"
#include "dings/label.h"

#include "dataobj/tabfile.h"
#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/fahrplan.h"
#include "dataobj/route.h"

#include "bauer/tunnelbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"

#include "besch/weg_besch.h"
#include "besch/roadsign_besch.h"

#include "sucher/bauplatz_sucher.h"

#include "tpl/vector_tpl.h"

#include "utils/simstring.h"

#include "simwerkz.h"

/****************************************** static helper functions **************************************/

/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char *tooltip_with_price(const char * tip, sint64 price)
{
	const int n = sprintf(werkzeug_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(werkzeug_t::toolstr+n, (double)price/-100.0);
	return werkzeug_t::toolstr;
}



/**
 * Creates a tooltip from tip text and money value
 * @author Hj. Malthaner
 */
char *tooltip_with_price_maintenance(karte_t *welt, const char *tip, sint64 price, sint64 maintenance)
{
	int n = sprintf(werkzeug_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(werkzeug_t::toolstr+n, (double)price/-100.0);
	strcat( werkzeug_t::toolstr, " (" );
	n = strlen(werkzeug_t::toolstr);

	money_to_string(werkzeug_t::toolstr+n,
		welt->ticks_per_world_month_shift>=18 ?
		(double)(maintenance << (welt->ticks_per_world_month_shift - 18)) / 100.0 :
		(double)(maintenance >> (18 - welt->ticks_per_world_month_shift)) / 100.0
	);
	strcat( werkzeug_t::toolstr, ")" );
	return werkzeug_t::toolstr;
}



/**
 * Creates a tooltip from tip text and money value
 */
static char const* tooltip_with_price_maintenance_level(karte_t* const welt, char const* const tip, sint64 const price, sint64 const maintenance, uint32 const level, uint8 const enables)
{
	int n = sprintf(werkzeug_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(werkzeug_t::toolstr+n, (double)price/-100.0);
	strcat( werkzeug_t::toolstr, " (" );
	n = strlen(werkzeug_t::toolstr);

	money_to_string(werkzeug_t::toolstr+n,
		welt->ticks_per_world_month_shift>=18 ?
		(double)(maintenance << (welt->ticks_per_world_month_shift - 18)) / 100.0 :
		(double)(maintenance >> (18 - welt->ticks_per_world_month_shift)) / 100.0
			);
	strcat( werkzeug_t::toolstr, ")" );
	n = strlen(werkzeug_t::toolstr);

	if((enables&7)!=0) {
		n += sprintf( werkzeug_t::toolstr+n, ", %d", level*32 );
		if(enables&1) {
			n += sprintf( werkzeug_t::toolstr+n, " %s", translator::translate("Passagiere") );
		}
		if(enables&2) {
			n += sprintf( werkzeug_t::toolstr+n, " %s", translator::translate("Post") );
		}
		if(enables&4) {
			n += sprintf( werkzeug_t::toolstr+n, " %s", translator::translate("Fracht") );
		}
	}
	else if(  !welt->get_einstellungen()->is_seperate_halt_capacities()  ) {
		n += sprintf( werkzeug_t::toolstr+n, ", %s %d", translator::translate("Storage capacity"), level*32 );
	}

	return werkzeug_t::toolstr;
}



/**
 * sucht Haltestelle um Umkreis +1/-1 um (pos, b, h)
 * extended to search first in our direction
 * @author Hj. Malthaner, V.Meyer, prissi
 */
static halthandle_t suche_nahe_haltestelle(spieler_t *sp, karte_t *welt, koord3d pos, sint16 b=1, sint16 h=1)
{
	const planquadrat_t *plan = welt->lookup(pos.get_2d());
	if(plan  &&  plan->get_halt().is_bound()) {
		return plan->get_halt();
	}

	ribi_t::ribi ribi = ribi_t::keine;
	koord next_try_dir[4];  // will be updated each step: biggest distance try first ...
	int iAnzahl = 0;

	grund_t *bd = welt->lookup(pos);
	if(bd==NULL) {
		bd = welt->lookup_kartenboden(pos.get_2d());
	}

	// first we try to connect to a stop straight in our direction; otherwise our station may break during construction
	if(bd->hat_wege()) {
		ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
	}
	if(  ribi_t::nord & ribi ) {
		next_try_dir[iAnzahl++] = koord(0,-1);
	}
	if(  ribi_t::sued & ribi ) {
		next_try_dir[iAnzahl++] = koord(0,1);
	}
	if(  ribi_t::ost & ribi ) {
		next_try_dir[iAnzahl++] = koord(1,0);
	}
	if(  ribi_t::west & ribi ) {
		next_try_dir[iAnzahl++] = koord(-1,0);
	}

	// first try to connect to our own
	for(  int i=0;  i<iAnzahl;  i++ ) {
		const planquadrat_t *plan = welt->lookup(pos.get_2d()+next_try_dir[i]);
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(halt.is_bound()  &&  sp==halt->get_besitzer()) {
				return halt;
			}
		}
	}

	// now just search all neighbours
	for(  sint16 y=-1;  y<=h;  y++  ) {
		const planquadrat_t *plan = welt->lookup( pos.get_2d()+koord(-1,y) );
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(halt.is_bound()  &&  sp==halt->get_besitzer()) {
				return halt;
			}
		}
		plan = welt->lookup( pos.get_2d()+koord(b,y) );
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(halt.is_bound()  &&  sp==halt->get_besitzer()) {
				return halt;
			}
		}
	}
	for(  sint16 x=0;  x<b;  x++  ) {
		const planquadrat_t *plan = welt->lookup( pos.get_2d()+koord(x,-1) );
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(halt.is_bound()  &&  sp==halt->get_besitzer()) {
				return halt;
			}
		}
		plan = welt->lookup( pos.get_2d()+koord(x,h) );
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(halt.is_bound()  &&  sp==halt->get_besitzer()) {
				return halt;
			}
		}
	}

#if AUTOJOIN_PUBLIC
	// now search everything for public stops
	for(  int i=0;  i<8;  i++ ) {
		const planquadrat_t *plan = welt->lookup(pos.get_2d()+koord::neighbours[i]);
		if(plan) {
			halthandle_t halt = plan->get_halt();
			if(halt.is_bound()  &&  welt->get_spieler(1)==halt->get_besitzer()) {
				return halt;
			}
		}
	}
#endif

	// here we reach only with a non-found station, i.e. a non-bounded handle
	return halthandle_t();
}


// converts a 2d koord to a suitable ground pointer
static grund_t *wkz_intern_koord_to_weg_grund(spieler_t *sp, karte_t *welt, koord3d pos, waytype_t wt)
{
	// check for valid ground
	grund_t *gr=welt->lookup(pos);
	if (gr==NULL) {
		return NULL;
	}

	if(  wt==powerline_wt  &&  gr->get_leitung()  ) {
		// check for ownership
		if(sp!=NULL  &&  gr->get_leitung()->ist_entfernbar(sp)!=NULL) {
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
		if (way && way->get_besch()->get_styp() == weg_t::type_tram &&  way->ist_entfernbar(sp)==NULL) {
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
	if(sp!=NULL  &&  gr->get_weg(wt)->ist_entfernbar(sp)!=NULL){
		return NULL;
	}
	// ok, now we have a valid ground
	return gr;
}



/****************************************** now the actual tools **************************************/

// werkzeuge
const char *wkz_abfrage_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	grund_t *gr = welt->lookup(pos);
	if(gr) {
		DBG_MESSAGE("wkz_abfrage()","checking map square %s", pos.get_str());

		if(  umgebung_t::single_info  ) {

			int old_count = win_get_open_count();

			if(  is_ctrl_pressed()  ) {
				// reverse order
				for(int n=0;  n<gr->get_top();  n++  ) {
					ding_t *dt = gr->obj_bei(n);
					if(dt  &&  dt->get_typ()!=ding_t::wayobj  &&  dt->get_typ()!=ding_t::pillar  &&  dt->get_typ()!=ding_t::label) {
						DBG_MESSAGE("wkz_abfrage()", "index %d", n);
						dt->zeige_info();
						// did some new window open?
						if(old_count!=win_get_open_count()  &&  !gr->ist_wasser()) {
							return NULL;
						}
					}
				}

				if(  gr->get_flag(grund_t::marked)  ) {
					label_t *lb = gr->find<label_t>();
					if(  lb  ) {
						lb->zeige_info();
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
						lb->zeige_info();
						if(  old_count!=win_get_open_count()  ) {
							return NULL;
						}
					}
				}

				for(int n=gr->get_top()-1;  n>=0;  n--  ) {
					ding_t *dt = gr->obj_bei(n);
					if(dt  &&  dt->get_typ()!=ding_t::wayobj  &&  dt->get_typ()!=ding_t::pillar  &&  dt->get_typ()!=ding_t::label) {
						DBG_MESSAGE("wkz_abfrage()", "index %d", n);
						dt->zeige_info();
						// did some new window open?
						if(old_count!=win_get_open_count()  &&  !gr->ist_wasser()) {
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
				ding_t *dt = gr->obj_bei(n);
				if(dt  &&  dt->get_typ()!=ding_t::wayobj  &&  dt->get_typ()!=ding_t::pillar) {
					dt->zeige_info();
				}
			}
		}

		if(gr->get_depot()  &&  gr->get_depot()->get_besitzer()==sp) {
			int old_count = win_get_open_count();
			gr->get_depot()->zeige_info();
			// did some new window open?
			if(umgebung_t::single_info  &&  old_count!=win_get_open_count()) {
				return NULL;
			}
		}
	}
	return NULL;
}


/* delete things from a tile
 * citycars and pedestrian first and then go up to queue to more important objects
 */
bool wkz_remover_t::wkz_remover_intern(spieler_t *sp, karte_t *welt, koord3d pos, const char *&msg)
{
DBG_MESSAGE("wkz_remover_intern()","at (%s)", pos.get_str());

	grund_t *gr = welt->lookup(pos);
	if (!gr) {
		msg = "";
		return false;
	}

	// check if there is something to remove from here ...
	if(gr->get_top()==0  ) {
		msg = "";
		return false;
	}

	// marker?
	label_t* l = gr->find<label_t>();
	if (l) {
		msg = l->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
		delete l;
		return true;
	}

	// citycar? (we allow always)
	stadtauto_t* citycar = gr->find<stadtauto_t>();
	if(citycar) {
		delete citycar;
		return true;
	}

	// pedestrians?
	fussgaenger_t* fussgaenger = gr->find<fussgaenger_t>();
	if (fussgaenger) {
		delete fussgaenger;
		return true;
	}

	// prissi: check powerline (can cross ground of another player)
	leitung_t* lt = gr->get_leitung();
	if(lt!=NULL  &&  lt->ist_entfernbar(sp)==NULL) {
		bool is_leitungsbruecke = false;
		if(gr->ist_bruecke()  &&  gr->ist_karten_boden()) {
			bruecke_t* br = gr->find<bruecke_t>();
			if (br == NULL) {
				// no bridge? most likely transformer on a former bridge tile...
				grund_t *gr_new = new boden_t(welt, pos, gr->get_grund_hang());
				gr_new->take_obj_from( gr );
				welt->access(pos.get_2d())->kartenboden_setzen( gr_new );
				gr = gr_new;
			}
			else {
				is_leitungsbruecke = br->get_besch()->get_waytype()==powerline_wt;
			}
		}
		if(is_leitungsbruecke) {
			msg = brueckenbauer_t::remove(welt, sp, gr->get_pos(), powerline_wt );
			return msg == NULL;
		}
		else if(  !gr->ist_bruecke()  ) {
			lt->entferne(sp);
			delete lt;
			return true;
		}
	}

	// check for signal
	roadsign_t* rs = gr->find<signal_t>();
	if (rs == NULL) rs = gr->find<roadsign_t>();
	if(rs!=NULL) {
		msg = rs->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
DBG_MESSAGE("wkz_remover()",  "removing roadsign at (%s)", pos.get_str());
		weg_t *weg = gr->get_weg(rs->get_besch()->get_wtyp());
		if(  weg==NULL  &&  rs->get_besch()->get_wtyp()==tram_wt  ) {
			weg = gr->get_weg(track_wt);
		}
		rs->entferne(sp);
		delete rs;
		assert( weg );
		weg->count_sign();
		return true;
	}

	// check stations
	halthandle_t halt = gr->get_halt();
DBG_MESSAGE("wkz_remover()", "bound=%i",halt.is_bound());
	if (gr->is_halt()  &&  halt.is_bound()  &&  fabrik_t::get_fab(welt,pos.get_2d())==NULL) {
		// halt and not a factory (oil rig etc.)
		const spieler_t* owner = halt->get_besitzer();
		if(  spieler_t::check_owner( owner, sp )  ) {
			return haltestelle_t::remove(welt, sp, gr->get_pos(), msg);
		}
	}

	// catenary or something like this
	wayobj_t* wo = gr->find<wayobj_t>();
	if(wo) {
		msg = wo->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
		wo->entferne(sp);
		delete wo;
		depot_t *dep = gr->get_depot();
		if( dep ) {
			dep->update_win();
		}
		return true;
	}

DBG_MESSAGE("wkz_remover()", "check tunnel/bridge");

	// beginning/end of bridge?
	if(gr->ist_bruecke()  &&  gr->ist_karten_boden()) {
DBG_MESSAGE("wkz_remover()",  "removing bridge from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
		bruecke_t* br = gr->find<bruecke_t>();
		msg = brueckenbauer_t::remove(welt, sp, gr->get_pos(), br->get_besch()->get_waytype());
		return msg == NULL;
	}

	// beginning/end of tunnel
	if(gr->ist_tunnel()  &&  gr->ist_karten_boden()) {
DBG_MESSAGE("wkz_remover()",  "removing tunnel  from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
		msg = tunnelbauer_t::remove(welt, sp, gr->get_pos(), gr->get_weg_nr(0)->get_waytype());
		return msg == NULL;
	}

	// fields
	field_t* f = gr->find<field_t>();
	if (f) {
		msg = f->ist_entfernbar(sp);
		if(msg==NULL) {
			f->entferne(sp);
			delete f;
			// fields have foundations ...
			koord pos = gr->get_pos().get_2d();
			sint8 dummy;
			welt->access(pos)->boden_ersetzen( gr, new boden_t(welt, gr->get_pos(), welt->recalc_natural_slope(pos,dummy) ) );
			welt->lookup_kartenboden(pos)->calc_bild();
			welt->lookup_kartenboden(pos)->set_flag( grund_t::dirty );
		}
		return msg == NULL;
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if(gb != NULL) {
		msg = gb->ist_entfernbar(sp);
		if(msg) {
			return false;
		}
		if(!gb->get_tile()->get_besch()->can_rotate()  &&  welt->cannot_save()) {
			msg = "Not possible in this rotation!";
			return false;
		}
		DBG_MESSAGE("wkz_remover()",  "removing building" );
		const haus_tile_besch_t *tile  = gb->get_tile();
		koord size = tile->get_besch()->get_groesse( tile->get_layout() );

		// get startpos
		koord k=tile->get_offset();
		if(k != koord(0,0)) {
			return wkz_remover_intern(sp, welt, pos-k, msg);
		}
		else {
			// remove town? (when removing townhall)
			if(gb->ist_rathaus()) {
				stadt_t *stadt = welt->suche_naechste_stadt(pos.get_2d());
				if(!welt->rem_stadt( stadt )) {
					msg = "Das Feld gehoert\neinem anderen Spieler\n";
					return false;
				}
			}
			else {
				// townhall is also removed during town removal
				hausbauer_t::remove( welt, sp, gb );
			}
		}
		return true;
	}

	// there is a powerline above this tile, but we do not own it
	// so we take it out and add it later again
	if(lt) {
DBG_MESSAGE("wkz_remover()",  "took out powerline");
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

	// remove all other stuff (clouds, ...)
	bool return_ok = false;
	uint8 num_obj = gr->obj_count();
	if(num_obj>0) {
		msg = gr->kann_alle_obj_entfernen(sp);
		return_ok = (msg==NULL  &&  !(gr->get_typ()==grund_t::brueckenboden  ||  gr->get_typ()==grund_t::tunnelboden)  &&  gr->obj_loesche_alle(sp));
		DBG_MESSAGE("wkz_remover()",  "removing everything from %d,%d,%d",gr->get_pos().x, gr->get_pos().y, gr->get_pos().z);
	}

	if(lt) {
		DBG_MESSAGE("wkz_remover()",  "add again powerline");
		gr->obj_add(lt);
	}
	if(cr) {
		gr->obj_add(cr);
	}
	if(zeiger) {
		gr->obj_add(zeiger);
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
DBG_MESSAGE("wkz_remover()", "removing way");

	/*
	* @TODO Eigentlich muessen wir hier noch verhindern, dass ein Bahnhofsgebaeude oder eine
	* Bushaltestelle vereinzelt wird!
	* Sonst laesst sich danach die Richtung der Haltestelle verdrehen und die Bilder
	* gehen kaputt.
	*/
	long cost_sum = 0;
	if(gr->get_typ()!=grund_t::tunnelboden  ||  gr->has_two_ways()) {
		weg_t *w=gr->get_weg_nr(1);
		if(gr->get_typ()==grund_t::brueckenboden  &&  w==NULL) {
			// do not delete the middle of a bridge
			return false;
		}
		if(w==NULL  ||  w->ist_entfernbar(sp)!=NULL) {
			w = gr->get_weg_nr(0);
			if(w==NULL) {
				// no way at all ...
				return true;
			}
			if(w->ist_entfernbar(sp)!=NULL){
				msg = w->ist_entfernbar(sp);
				return false;
			}
		}
		cost_sum = gr->weg_entfernen(w->get_waytype(), true);
	}
	else {
		// remove upper ways ...
		if(gr->get_weg_nr(1)) {
			cost_sum = gr->weg_entfernen(gr->get_weg_nr(1)->get_waytype(), true);
		}
		else {
			// delete tunnel here ...
			const tunnel_besch_t* besch = gr->find<tunnel_t>()->get_besch();
			gr->obj_loesche_alle(sp);
			cost_sum += gr->weg_entfernen(besch->get_waytype(), true);
			cost_sum += besch->get_preis();
		}
	}

	if(cost_sum > 0) {
		sp->buche(-cost_sum, pos.get_2d(), COST_CONSTRUCTION);
		if(gr->hat_wege()) {
			return true;
		}
	}
DBG_MESSAGE("wkz_remover()", "check ground");

	if(!gr->ist_karten_boden()  &&  gr->get_top()==0) {
DBG_MESSAGE("wkz_remover()", "removing ground");
		// unmark kartenboden (is marked during underground mode deletion)
		welt->lookup_kartenboden(pos.get_2d())->clear_flag(grund_t::marked);
		// remove upper or lower ground
		welt->access(pos.get_2d())->boden_entfernen(gr);
		delete gr;
	}

	return true;
}



const char *wkz_remover_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	DBG_MESSAGE("wkz_remover()","at %d,%d", pos.x, pos.y);
	const char *fail = NULL;
	if(!wkz_remover_intern(sp, welt, pos, fail)) {
		return fail;
	}

	// must recalc neighbourhood for slopes etc.
	if(pos.x>1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::west)->calc_bild();
	}
	if(pos.y>1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::nord)->calc_bild();
	}

	if(pos.x<welt->get_groesse_x()-1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::ost)->calc_bild();
	}
	if(pos.y<welt->get_groesse_y()-1) {
		welt->lookup_kartenboden(pos.get_2d()+koord::sued)->calc_bild();
	}

	return NULL;
}



const char *wkz_raise_t::move( karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	if(  buttonstate==1  ) {
		char buf[16];
		if(!is_dragging) {
			grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
			drag_height = gr->get_hoehe() + corner4(gr->get_grund_hang()) +1;
		}
		is_dragging = true;
		sprintf( buf, "%i", drag_height );
		default_param = buf;
		const char *result = work( welt, sp, pos );
		default_param = NULL;
		return result;
	}
	return NULL;
}


const char *wkz_raise_t::check( karte_t *welt, spieler_t *, koord3d k )
{
	// check for underground mode
	if (is_dragging  &&  drag_height-1 > grund_t::underground_level) {
		is_dragging = false;
		return "";
	}
	grund_t *gr = welt->lookup_kartenboden(k.get_2d());
	if (gr==NULL) {
		return "";
	}
	sint8 h = gr->get_hoehe() + corner4(gr->get_grund_hang());
	if (h > grund_t::underground_level) {
			return "Terraforming not possible\nhere in underground view";
	}
	return NULL;
}

const char *wkz_raise_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
//DBG_MESSAGE("wkz_raise()","raising square (%d,%d) to %d",pos.x, pos.y, welt->lookup_hgt(pos)+Z_TILE_STEP);
	bool ok = false;
	koord pos = k.get_2d();

	if(welt->ist_in_kartengrenzen(pos)  &&  pos.x>0  &&  pos.y>0) {
		grund_t *gr = welt->lookup_kartenboden(pos);
		const sint8 hgt = gr->get_hoehe() + corner4(gr->get_grund_hang());

		if(hgt < 14*Z_TILE_STEP) {

			int n = 0;	// tiles changed
			if(default_param  &&  strlen(default_param)>0) {
				ok = true;
				// called by dragging or by AI
				sint16 height = atoi(default_param);
				// dragging may be going up or down!
				while(welt->lookup_hgt(pos)<height) {
					int diff = welt->raise(pos);
					if(diff==0) break;
					n += diff;
				}
				while(welt->lookup_hgt(pos)>height) {
					int diff = welt->lower(pos);
					if(diff==0) break;
					n += diff;
				}
				ok = height==welt->lookup_hgt(pos);
			}
			else {
				if(  is_dragging  ) {
					is_dragging = false;
					return NULL;
				}
				n = welt->raise(pos);
				ok = (n!=0);
			}
			if(n>0) {
				spieler_t::accounting(sp, welt->get_einstellungen()->cst_alter_land*n, pos, COST_CONSTRUCTION);
			}
			return !ok ? "Tile not empty." : (n ? NULL : "");
		}
		else {
			// no mountains higher than 14 ...
			return "Maximum tile height difference reached.";
		}
	}
	return "Zu nah am Kartenrand";
}



const char *wkz_lower_t::move( karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	if(  buttonstate==1  ) {
		char buf[16];
		if(!is_dragging) {
			drag_height = welt->lookup_hgt(pos.get_2d())-Z_TILE_STEP;
		}
		is_dragging = true;
		sprintf( buf, "%i", drag_height );
		default_param = buf;
		const char *result = work( welt, sp, pos );
		default_param = NULL;
		return result;
	}
	return NULL;
}


const char *wkz_lower_t::check( karte_t *welt, spieler_t *, koord3d k )
{
	// check for underground mode
	if (is_dragging  &&  drag_height+1 > grund_t::underground_level) {
		is_dragging = false;
		return "";
	}
	grund_t *gr = welt->lookup_kartenboden(k.get_2d());
	if (gr==NULL) {
		return "";
	}
	sint8 h = gr->get_hoehe() + corner4(gr->get_grund_hang()) - 1;
	if (h > grund_t::underground_level) {
			return "Terraforming not possible\nhere in underground view";
	}
	return NULL;
}

const char *wkz_lower_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
// DBG_MESSAGE("wkz_lower()","lowering square %d,%d to %d", pos.x, pos.y, welt->lookup_hgt(pos)-Z_TILE_STEP);
	bool ok = false;
	koord pos = k.get_2d();

	if(welt->ist_in_kartengrenzen(pos)  &&  pos.x>0  &&  pos.y>0) {
		grund_t *gr = welt->lookup_kartenboden(pos);
		const sint8 hgt = gr->get_hoehe() + corner4(gr->get_grund_hang());

		if(hgt > welt->get_grundwasser()) {

			int n = 0;	// tiles changed
			if(default_param  &&  strlen(default_param)>0) {
				// called by dragging or by AI
				sint16 height = atoi(default_param);
				// dragging may be going up or down!
				while(welt->lookup_hgt(pos)<height) {
					int diff = welt->raise(pos);
					if(diff==0) break;
					n += diff;
				}
				while(welt->lookup_hgt(pos)>height) {
					int diff = welt->lower(pos);
					if(diff==0) break;
					n += diff;
				}
				ok = height==welt->lookup_hgt(pos);
			}
			else {
				if(  is_dragging  ) {
					is_dragging = false;
					return NULL;
				}
				n = welt->lower(pos);
				ok = (n!=0);
			}
			if(n>0) {
				spieler_t::accounting(sp, welt->get_einstellungen()->cst_alter_land*n, pos, COST_CONSTRUCTION);
			}
			return !ok ? "Tile not empty." : (n ? NULL : "");
		}
		else {
			// below water level
			return "";
		}
	}
	return "Zu nah am Kartenrand";
}


const char *wkz_setslope_t::check( karte_t *welt, spieler_t *, koord3d pos)
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

const char *wkz_restoreslope_t::check( karte_t *welt, spieler_t *, koord3d pos)
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

/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
const char *wkz_setslope_t::wkz_set_slope_work( karte_t *welt, spieler_t *sp, koord3d pos, int new_slope )
{
	bool ok = false;

	grund_t *gr1 = welt->lookup(pos);
	if(gr1) {


		// at least a pixel away from the border?
		if(  pos.z<welt->get_grundwasser() &&  !gr1->ist_tunnel() ) {
			return "Maximum tile height difference reached.";
		}

		if(  new_slope==RESTORE_SLOPE  &&  !(gr1->get_typ()==grund_t::boden  ||  gr1->get_typ()==grund_t::wasser)  ) {
			return "No suitable ground!";
		}

		// finally: empty enough
		if(  gr1->get_grund_hang()!=gr1->get_weg_hang()  ||  gr1->get_halt().is_bound()  ||  gr1->kann_alle_obj_entfernen(sp)  ||
				   gr1->find<gebaeude_t>()  ||  gr1->get_depot()  ||  gr1->get_leitung()  ||  gr1->get_weg(air_wt)  ||  gr1->find<label_t>()  ||  gr1->get_typ()==grund_t::brueckenboden) {
			return "Tile not empty.";
		}

		if(  !welt->ist_in_kartengrenzen(pos.get_2d()+koord(1,1))  ||  !welt->ist_in_kartengrenzen(pos.get_2d()+koord(-1,-1))) {
			return "Zu nah am Kartenrand";
		}

		// slopes may affect the position and the total height!
		koord3d new_pos = pos;

		ribi_t::ribi ribis = new_slope<hang_t::erhoben ? ribi_t::rueckwaerts(ribi_typ(new_slope)) : (ribi_t::ribi)ribi_t::alle;
		if(  gr1->hat_wege()  ) {
			// check the resulting slope
			ribis = gr1->get_weg_nr(0)->get_ribi_unmasked();
			if(  gr1->get_weg_nr(1)  ) {
				ribis |= gr1->get_weg_nr(1)->get_ribi_unmasked();
			}
			if(  new_slope==RESTORE_SLOPE  ||  !ribi_t::ist_einfach(ribis)  ||  (new_slope<hang_t::erhoben  &&  ribi_t::rueckwaerts(ribi_typ(new_slope))!=ribis)  ) {
				// has the wrong tilt
				return "Tile not empty.";
			}
			/* new things getting tricky:
			 * A single way on an allup or down slope will result in
			 * a slope with the way as hinge.
			 */
			if(  new_slope==ALL_UP_SLOPE  ) {
				if(  gr1->get_weg_hang()==hang_t::flach  ) {
					new_slope = hang_typ(ribis);
				}
				else if(  gr1->get_weg_hang()!=hang_typ(ribi_t::rueckwaerts(ribis))  ) {
					return "Maximum tile height difference reached.";
				}
			}
			else if(  new_slope==ALL_DOWN_SLOPE  ) {
				if(  gr1->get_grund_hang()==hang_typ(ribis)  ) {
					// do not lower tiles to sea
					if(  pos.z == welt->get_grundwasser()  &&  !gr1->ist_tunnel()  ) {
						return "Tile not empty.";
					}
				}
				else if(  gr1->get_grund_hang()==hang_t::flach  ) {
					new_slope = hang_typ(ribi_t::rueckwaerts(ribis));
					new_pos.z -= Z_TILE_STEP;
					if(  welt->lookup(new_pos)  ) {
						return "Tile not empty.";
					}
				}
				else {
					return "Maximum tile height difference reached.";
				}
			}
		}


		if(new_slope == RESTORE_SLOPE) {
			// prissi: special action: set to natural slope
			sint8 min_hgt;
			new_slope = welt->recalc_natural_slope(pos.get_2d(),min_hgt);
			new_pos = koord3d(pos.get_2d(), min_hgt);
			DBG_MESSAGE("natural_slope","%i",new_slope);
		}
		else if(new_slope == ALL_DOWN_SLOPE) {
			new_slope = hang_t::flach;
			// is more intuitive: if there is a slope, first downgrade it
			if (gr1->get_grund_hang()==0  ) {
				new_pos.z -= Z_TILE_STEP;
			}
		}
		else if(new_slope == ALL_UP_SLOPE) {
			new_slope = hang_t::flach;
			new_pos.z += Z_TILE_STEP;
		}

		// already some ground here (tunnel, bridge, monorail?)
		if(new_pos.z!=pos.z  &&  welt->lookup(new_pos)!=NULL) {
			return "Tile not empty.";
		}
		// check for grounds above / below
		if (new_pos.z >= pos.z) {
			grund_t *gr2 = welt->lookup(new_pos+koord3d(0,0,Z_TILE_STEP));
			// only raise corners that are raised above
			if(  gr2  &&  (new_slope & (~gr2->get_weg_hang())) ) {
				return "Tile not empty.";
			}
		}
		if (new_pos.z <= pos.z) {
			grund_t *gr2 = welt->lookup(new_pos+koord3d(0,0,-Z_TILE_STEP));
			// only lower corners that are not raised below
			if(  gr2  &&  (gr2->get_weg_hang() & (~new_slope)) ) {
				return "Tile not empty.";
			}
		}

		// check, if action is valid ...
		const sint16 hgt=new_pos.z/Z_TILE_STEP;
		// maximum difference
		const sint8 test_hgt = hgt+(new_slope!=0);

		if(  gr1->get_typ()==grund_t::boden  ) {
			// first left side
			const grund_t *grleft=welt->lookup_kartenboden(pos.get_2d()+koord(-1,0));
			if(grleft) {
				const sint16 left_hgt=grleft->get_hoehe()/Z_TILE_STEP + (new_slope==ALL_DOWN_SLOPE && grleft->get_grund_hang()? 1 : 0);
				const sint8 diff_from_ground = abs(left_hgt-test_hgt);
				if(diff_from_ground>2) {
					return "Maximum tile height difference reached.";
				}
			}

			// right side
			const grund_t *grright=welt->lookup_kartenboden(pos.get_2d()+koord(1,0));
			if(grright) {
				const sint16 right_hgt=grright->get_hoehe()/Z_TILE_STEP  + (new_slope==ALL_DOWN_SLOPE && grright->get_grund_hang()? 1 : 0);
				const sint8 diff_from_ground = abs(right_hgt-test_hgt);
				if(diff_from_ground>2) {
					return "Maximum tile height difference reached.";
				}
			}

			const grund_t *grback=welt->lookup_kartenboden(pos.get_2d()+koord(0,-1));
			if(grback) {
				const sint16 back_hgt=grback->get_hoehe()/Z_TILE_STEP  + (new_slope==ALL_DOWN_SLOPE && grback->get_grund_hang()? 1 : 0);
				const sint8 diff_from_ground = abs(back_hgt-test_hgt);
				if(diff_from_ground>2) {
					return "Maximum tile height difference reached.";
				}
			}

			const grund_t *grfront=welt->lookup_kartenboden(pos.get_2d()+koord(0,1));
			if(grfront) {
				const sint16 front_hgt=grfront->get_hoehe()/Z_TILE_STEP  + (new_slope==ALL_DOWN_SLOPE && grfront->get_grund_hang()? 1 : 0);
				const sint8 diff_from_ground = abs(front_hgt-test_hgt);
				if(diff_from_ground>2) {
					return "Maximum tile height difference reached.";
				}
			}
		}

		// ok, now we set the slope ...
		ok = (new_pos!=pos);
		ok |= new_slope!=gr1->get_grund_hang();

		if(ok) {

			if(  gr1->kann_alle_obj_entfernen(sp)  ) {
				// not empty ...
				return "Tile not empty.";
			}

			// ok, was sucess
			if(!gr1->ist_wasser()  &&  new_slope==0  &&  new_pos.z==welt->get_grundwasser()  &&  gr1->get_typ()!=grund_t::tunnelboden  ) {
				// now water
				gr1->obj_loesche_alle(sp);
				welt->access(pos.get_2d())->kartenboden_setzen( new wasser_t(welt,new_pos) );
				gr1 = welt->lookup_kartenboden(new_pos.get_2d());
			}
			else if(gr1->ist_wasser()  &&  (new_pos.z>welt->get_grundwasser()  ||  new_slope!=0)) {
				gr1->obj_loesche_alle(sp);
				welt->access(pos.get_2d())->kartenboden_setzen( new boden_t(welt,new_pos,new_slope) );
				gr1 = welt->lookup_kartenboden(new_pos.get_2d());
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
				if(  !gr1->ist_karten_boden()  ) {
					gr1->calc_bild();
				}
			}

			if(  gr1->ist_karten_boden()  ) {
				if(  new_slope!=hang_t::flach  ) {
					// no lakes on slopes ...
					groundobj_t *d = gr1->find<groundobj_t>();
					if(  d  &&  d->get_besch()->get_phases()!=16  ) {
						d->entferne(sp);
						delete d;
					}
					// connect canals to sea
					if (gr1->get_hoehe()==welt->get_grundwasser()  &&  gr1->hat_weg(water_wt)) {
						grund_t *sea = welt->lookup_kartenboden(new_pos.get_2d() - koord( ribi_typ(new_slope ) ));
						if (sea  &&  sea->ist_wasser()) {
							gr1->weg_erweitern(water_wt, ribi_t::rueckwaerts(ribi_typ(new_slope)));
							sea->calc_bild();
						}
					}
				}
				// recalc slope walls on neightbours
				for(int y=-1; y<=1; y++) {
					for(int x=-1; x<=1; x++) {
						grund_t *gr = welt->lookup_kartenboden(pos.get_2d()+koord(x,y));
						gr->calc_bild();
					}
				}
				// corect the grid height
				if(  gr1->ist_wasser()  ) {
					sint8 grid_hgt = min( welt->get_grundwasser(), welt->lookup_hgt(pos.get_2d()) );
					welt->set_grid_hgt(pos.get_2d(), grid_hgt );
				}
				else {
					welt->set_grid_hgt(pos.get_2d(), gr1->get_hoehe()+ corner4(gr1->get_grund_hang()) );
				}
				reliefkarte_t::get_karte()->calc_map_pixel(pos.get_2d());
			}
			spieler_t::accounting(sp, new_slope==RESTORE_SLOPE?welt->get_einstellungen()->cst_alter_land:welt->get_einstellungen()->cst_set_slope, pos.get_2d(), COST_CONSTRUCTION);
		}

	}
	return ok ? NULL : "";
}



// set marker
const char *wkz_marker_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(welt->ist_in_kartengrenzen(pos.get_2d())) {
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		if (gr) {
			if(!gr->get_text()) {
				const ding_t* thing = gr->obj_bei(0);
				if(thing == NULL  ||  thing->get_besitzer() == sp  ||  (spieler_t::check_owner(thing->get_besitzer(), sp)  &&  (thing->get_typ() != ding_t::gebaeude))) {
					gr->obj_add(new label_t(welt, gr->get_pos(), sp, "\0"));
					if (is_local_execution()) {
						gr->find<label_t>()->zeige_info();
					}
					return "";
				}
			}
		}
	}
	return "Das Feld gehoert\neinem anderen Spieler\n";
}



// show/repair blocks
bool wkz_clear_reservation_t::init( karte_t *welt, spieler_t * )
{
	if (is_local_execution()) {
		schiene_t::show_reservations = true;
		welt->set_dirty();
	}
	return true;
}

bool wkz_clear_reservation_t::exit( karte_t *welt, spieler_t * )
{
	if (is_local_execution()) {
		schiene_t::show_reservations = false;
		welt->set_dirty();
	}
	return true;
}

const char *wkz_clear_reservation_t::work( karte_t *welt, spieler_t *, koord3d k )
{
	grund_t *gr = welt->lookup(k);
	if(gr) {
		for(unsigned wnr=0;  wnr<2;  wnr++  ) {

			schiene_t const* const w = ding_cast<schiene_t>(gr->get_weg_nr(wnr));
			// is this a reserved track?
			if(w!=NULL  &&  w->is_reserved()) {
				/* now we do a very crude procedure:
				 * - we search all ways for reservations of this convoi and remove them
				 * - we set the convoi state to ROUTING_1; it must rereserve its ways then
				 */
				const waytype_t waytype = w->get_waytype();
				const convoihandle_t cnv = w->get_reserved_convoi();
				if(cnv->get_state()==convoi_t::DRIVING) {
					// reset driving state
					cnv->suche_neue_route();
				}
				slist_iterator_tpl<weg_t *>iter(weg_t::get_alle_wege());
				while(iter.next()) {
					if(iter.get_current()->get_waytype()==waytype) {
						schiene_t* const sch = ding_cast<schiene_t>(iter.access_current());
						if (sch->get_reserved_convoi() == cnv) {
							vehikel_t& v = *cnv->front();
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
const char *wkz_transformer_t::get_tooltip( spieler_t *sp )
{
	sprintf(toolstr, "%s, %ld$ (%ld$)", translator::translate("Build drain"), (long)(sp->get_welt()->get_einstellungen()->cst_transformer/-100l), (long)(sp->get_welt()->get_einstellungen()->cst_maintain_transformer<<(sp->get_welt()->ticks_per_world_month_shift-18))/-100l );
	return toolstr;
}

const char *wkz_transformer_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
DBG_MESSAGE("wkz_senke()","called on %d,%d", k.x, k.y);
	grund_t *gr=welt->lookup_kartenboden(k.get_2d());
	if(gr  &&  gr->get_grund_hang()==0  &&  !gr->ist_wasser()  &&  gr->ist_natur()  &&  gr->kann_alle_obj_entfernen(sp)==NULL) {
		fabrik_t *fab=leitung_t::suche_fab_4(k.get_2d());
		if(fab==NULL) {
			return "Transformer only next to factory!";
		}
		if(  fab->is_transformer_connected()  ) {
			return "Only one transformer per factory!";
		}

		// remove everything on that spot
		const char *fail = gr->kann_alle_obj_entfernen(sp);
		if(fail) {
			return fail;
		}
		gr->obj_loesche_alle(sp);
		// now decide from the string whether a source or drain is built
		if(fab->get_besch()->is_electricity_producer()) {
			pumpe_t *p = new pumpe_t(welt, gr->get_pos(), sp);
			gr->obj_add( p );
			p->laden_abschliessen();
		}
		else {
			senke_t *s = new senke_t(welt, gr->get_pos(), sp);
			gr->obj_add(s);
			s->laden_abschliessen();
		}
		fab->set_transformer_connected( true );
		return NULL;	// ok
	}
	return "Transformer only next to factory!";
}



/**
 * found a new city
 * @author Hj. Malthaner
 */
const char *wkz_add_city_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr) {
		if(gr->ist_natur() &&
			!gr->ist_wasser() &&
			gr->get_grund_hang() == 0  &&
			hausbauer_t::get_special(0,haus_besch_t::rathaus,welt->get_timeline_year_month(),0,welt->get_climate(gr->get_hoehe()))!=NULL  ) {

			gebaeude_t const* const gb = ding_cast<gebaeude_t>(gr->first_obj());
			if(gb && gb->ist_rathaus()) {
				dbg->warning("wkz_add_city()", "Already a city here");
				return "Tile not empty.";
			}
			else {

				// Hajo: if city is owned by player and player removes special
				// buildings the game crashes. To avoid this problem cities
				// always belong to player 1

				int citizens=(int)(welt->get_einstellungen()->get_mittlere_einwohnerzahl()*0.9);
				//  stadt_t *stadt = new stadt_t(welt, welt->get_spieler(1), pos,citizens/10+simrand(2*citizens+1));

				// always start with 1/10 citicens
				stadt_t* stadt = new stadt_t(welt->get_spieler(1), pos.get_2d(), citizens / 10);

				welt->add_stadt(stadt);
				stadt->laden_abschliessen();
				stadt->verbinde_fabriken();

				spieler_t::accounting(sp, welt->get_einstellungen()->cst_found_city, pos.get_2d(), COST_CONSTRUCTION);
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
const char *wkz_buy_house_t::work( karte_t *welt, spieler_t *sp, koord3d pos)
{
	if ( sp == welt->get_spieler(1) ) {
		return "";
	}
	grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(!gr  ||  gr->hat_wege()  ||  gr->get_halt().is_bound()) {
		return "";
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if(  gb== NULL  ||  gb->get_haustyp()==gebaeude_t::unbekannt  ||  !spieler_t::check_owner(gb->get_besitzer(),sp)  ) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	if(  gb->get_besitzer()==sp  ) {
		// I bought this already ...
		return "";
	}

	spieler_t *old_owner = gb->get_besitzer();
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
				if(  gb_part  &&  gb_part->get_tile()->get_besch()==hb  &&  spieler_t::check_owner(gb_part->get_besitzer(),sp)  ) {
					spieler_t::add_maintenance( old_owner, -welt->get_einstellungen()->maint_building*hb->get_level() );
					spieler_t::add_maintenance( sp, +welt->get_einstellungen()->maint_building*hb->get_level() );
					gb->set_besitzer(sp);
					sp->buche( -welt->get_einstellungen()->maint_building*hb->get_level(), k+pos.get_2d(), COST_CONSTRUCTION);
				}
			}
		}
	}
	return NULL;
}

/* change city size
 * @author prissi
 */
bool wkz_change_city_size_t::init( karte_t *, spieler_t * )
{
	cursor = atoi(default_param)>0 ? werkzeug_t::general_tool[WKZ_RAISE_LAND]->cursor : werkzeug_t::general_tool[WKZ_LOWER_LAND]->cursor;
	return true;
}

const char *wkz_change_city_size_t::work( karte_t *welt, spieler_t *, koord3d pos )
{
	stadt_t *city = welt->suche_naechste_stadt(pos.get_2d());
	if(city!=NULL) {
		city->change_size( atoi(default_param) );
		return NULL;
	}
	return "";
}



const char *wkz_plant_tree_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr) {
		const baum_besch_t *besch = NULL;
		bool check_climates = true;
		bool random_age = false;
		if(default_param==NULL  ||  strlen(default_param)==0) {
			besch = baum_t::random_tree_for_climate( welt->get_climate(pos.z) );
		}
		else {
			// parse default_param: bbbesch_nr b=1 ignore climate b=1 random age
			check_climates = default_param[0]=='0';
			random_age = default_param[1]=='1';
			besch = baum_t::find_tree(default_param+3);
		}
		if(besch  &&  baum_t::plant_tree_on_coordinate( welt, pos.get_2d(), besch, check_climates, random_age )  ) {
			spieler_t::accounting( sp, welt->get_einstellungen()->cst_remove_tree, pos.get_2d(), COST_CONSTRUCTION );
			return NULL;
		}
		return "";
	}
	return NULL;
}



/* the following three routines add waypoints/halts to a schedule
 * because we do not like to stop at AIs stop, but we still want to force the truck to use AI roads
 * So if there is a halt, then it must be either public or ours!
 * @author prissi
 */
static const char *wkz_fahrplan_insert_aux(karte_t *welt, spieler_t *sp, koord3d pos, schedule_t *fpl, bool append){
	if(fpl == NULL) {
		dbg->warning("wkz_fahrplan_insert_aux()","Schedule is (null), doing nothing");
		return false;
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
			if(  w!=NULL  &&  w->get_besitzer()!=welt->get_spieler(1)  &&  !spieler_t::check_owner(w->get_besitzer(),sp)  ) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
			if(  bd->get_depot()  &&  !spieler_t::check_owner( bd->get_depot()->get_besitzer(), sp )  ) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
		}
		if(  bd->is_halt()  &&  !spieler_t::check_owner( sp, bd->get_halt()->get_besitzer()) ) {
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

const char *wkz_fahrplan_add_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	return wkz_fahrplan_insert_aux( welt, sp, k, (schedule_t *)default_param, true );
}

const char *wkz_fahrplan_ins_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	return wkz_fahrplan_insert_aux( welt, sp, k, (schedule_t *)default_param, false );
}



/* way construction */
const weg_besch_t *wkz_wegebau_t::defaults[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
karte_t *wkz_wegebau_t::welt = NULL;	// for default city road

const weg_besch_t *wkz_wegebau_t::get_besch( uint16 timeline_year_month, bool remember ) const
{
	const weg_besch_t *besch = default_param ? wegbauer_t::get_besch(default_param,0) :NULL;
	if(besch==NULL) {
		waytype_t wt = (waytype_t)atoi(default_param);
		besch = defaults[wt&63];
		if(besch==NULL) {
			// search fastest way.
			besch = wegbauer_t::weg_search(wt, 0xffffffff, timeline_year_month, weg_t::type_flat);
		}
	}
	assert(besch);
	if(remember) {
		if(  besch->get_styp() == weg_t::type_tram  ) {
			defaults[ tram_wt ] = besch;
		}
		else {
			defaults[besch->get_wtyp()&63] = besch;
		}
	}
	return besch;
}

image_id wkz_wegebau_t::get_icon(spieler_t *) const
{
	const weg_besch_t *besch = wegbauer_t::get_besch(default_param,0);
	const bool is_tram = besch ? (besch->get_wtyp()==tram_wt) || (besch->get_styp() == weg_t::type_tram) : false;
	return (grund_t::underground_mode==grund_t::ugm_all && !is_tram ) ? IMG_LEER : icon;
}

const char *wkz_wegebau_t::get_tooltip(spieler_t *sp)
{
	const weg_besch_t *besch = get_besch(sp->get_welt()->get_timeline_year_month(),false);
	tooltip_with_price_maintenance( sp->get_welt(), besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed() );
	return toolstr;
}

// default ways are not intialized synchronously for different clients
// always return the name of a way, never the string containing the waytype
const char* wkz_wegebau_t::get_default_param(spieler_t *sp) const
{
	if (sp==NULL) {
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

bool wkz_wegebau_t::is_selected( karte_t *welt ) const
{
	if (welt->get_werkzeug(welt->get_active_player_nr())->get_id()!=id) {
		return false;
	}
	const wkz_wegebau_t *selected = dynamic_cast<const wkz_wegebau_t *>(welt->get_werkzeug(welt->get_active_player_nr()));
	return (selected  &&  selected->get_besch(welt->get_timeline_year_month(),false) == get_besch(welt->get_timeline_year_month(),false));
}

bool wkz_wegebau_t::init( karte_t *welt, spieler_t *sp )
{
	this->welt = welt;
	two_click_werkzeug_t::init( welt, sp );

	// now get current besch
	besch = get_besch(welt->get_timeline_year_month(), is_local_execution());
	if(besch  &&  besch->get_cursor()->get_bild_nr(0) != IMG_LEER) {
		cursor = besch->get_cursor()->get_bild_nr(0);
	}
	return besch!=NULL;
}

uint8 wkz_wegebau_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d & )
{
	error = NULL;
	grund_t *gr=welt->lookup(pos);
	if(gr  &&  hang_t::ist_wegbar(gr->get_weg_hang())) {
		// ignore tunnel tiles (except road tunnel for tram track building ..)
		if(  gr->get_typ() == grund_t::tunnelboden  &&  !gr->ist_karten_boden()  && !(besch->get_wtyp()==track_wt  &&  besch->get_styp()==7  && gr->hat_weg(road_wt)) ) {
			return 0;
		}
		// ignore water
		if( besch->get_wtyp() != water_wt  &&  gr->get_typ() == grund_t::wasser ) {
			return 0;
		}
		// test if way already exists on the way and if we are allowed to connect
		weg_t *way = gr->get_weg(besch->get_wtyp());
		if(way) {
			// allow to connect to any road
			if (besch->get_wtyp() == road_wt) {
				return 2;
			}
			error = way->ist_entfernbar(sp);
			return error==NULL ? 2 : 0;
		}
		// check for ownership but ignore moving things
		if(sp!=NULL) {
			for(uint8 i=0; i<gr->obj_count(); i++) {
				ding_t* dt = gr->obj_bei(i);
				if (!dt->is_moving()  &&  dt->ist_entfernbar(sp)!=NULL) {
					error =  dt->ist_entfernbar(sp); // "Das Feld gehoert\neinem anderen Spieler\n";
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

void wkz_wegebau_t::calc_route( wegbauer_t &bauigel, const koord3d &start, const koord3d &end )
{
	// recalc type of construction
	wegbauer_t::bautyp_t bautyp = (wegbauer_t::bautyp_t)besch->get_wtyp();
	if(besch->get_wtyp()==track_wt  &&  besch->get_styp()==7) {
		bautyp = wegbauer_t::schiene_tram;
	}
	// elevated track?
	if(besch->get_styp()==1  &&  besch->get_wtyp()!=air_wt) {
		bautyp = (wegbauer_t::bautyp_t)((int)bautyp|(int)wegbauer_t::elevated_flag);
	}

	bauigel.route_fuer(bautyp, besch);
	if(is_ctrl_pressed()) {
		DBG_MESSAGE("wkz_wegebau()", "try straight route");
		bauigel.set_keep_existing_ways(false);
		bauigel.calc_straight_route(start,end);
	}
	else {
		bauigel.set_keep_existing_faster_ways(true);
		bauigel.calc_route(start,end);
	}
	DBG_MESSAGE("wkz_wegebau()", "builder found route with %d sqaures length.", bauigel.get_count());
}

const char *wkz_wegebau_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(welt, sp);
	calc_route( bauigel, start, end );
	if(  bauigel.get_route().get_count()>1  ) {
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);

		struct sound_info info;
		info.index = SFX_CASH;
		info.volume = 255;
		info.pri = 0;
		sound_play(info);

		return NULL;
	}
	return "";
}

void wkz_wegebau_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(welt, sp);
	calc_route( bauigel, start, end );

	uint8 offset = (besch->get_styp()==1  &&  besch->get_wtyp()!=air_wt) ? 1 : 0;
	if(  bauigel.get_count()>1  ) {
		// Set tooltip first (no dummygrounds, if bauigel.calc_casts() is called).
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -bauigel.calc_costs() ) );

		// make dummy route from bauigel
		for(  uint32 j=0;  j<bauigel.get_count();  j++   ) {
			koord3d pos = bauigel.get_route()[j] + koord3d(0,0,offset);
			grund_t *gr = welt->lookup( pos );
			if( !gr ) {
				gr = new monorailboden_t(welt, pos, 0);
				gr->set_grund_hang( welt->lookup( pos - koord3d(0,0,1) )->get_grund_hang());
				welt->access(pos.get_2d())->boden_hinzufuegen(gr);
			}
			ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(besch->get_wtyp()) | bauigel.get_route().get_ribi( j );

			zeiger_t *way = new zeiger_t( welt, pos, NULL );
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
			marked[sp->get_player_nr()].insert( way );
			way->mark_image_dirty( way->get_bild(), 0 );
		}
	}
}

/* city road construction */
const weg_besch_t *wkz_build_cityroad::get_besch(uint16,bool) const
{
	return welt->get_city_road();
}

const char *wkz_build_cityroad::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(welt, sp);
	bauigel.set_build_sidewalk(true);
	calc_route( bauigel, start, end );
	if(  bauigel.get_route().get_count()>1  ) {
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);

		struct sound_info info;
		info.index = SFX_CASH;
		info.volume = 255;
		info.pri = 0;
		sound_play(info);

		return NULL;
	}
	return "";
}

/* bridge construction */
const char *wkz_brueckenbau_t::get_tooltip(spieler_t *sp)
{
	const bruecke_besch_t * besch = brueckenbauer_t::get_besch(default_param);
	tooltip_with_price_maintenance( sp->get_welt(), besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	if(besch->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
	}
	if(besch->get_max_length()>0) {
		n += sprintf(toolstr+n, ", %dkm", besch->get_max_length());
	}
	return toolstr;
}


const char *wkz_brueckenbau_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	if (end==koord3d::invalid) {
		return brueckenbauer_t::baue( welt, sp, start.get_2d(), besch );
	}
	else {
		const koord zv(ribi_typ(end-start));
		brueckenbauer_t::baue_bruecke( welt, sp, start, end, zv, besch, wegbauer_t::weg_search(besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat));
		return NULL; // all checks are performed before building.
	}
}
void wkz_brueckenbau_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	const ribi_t::ribi ribi_mark = ribi_typ(end-start);
	const koord zv(ribi_mark);
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	sint64 costs = 0;
	// start
	grund_t *gr = welt->lookup(start);
	zeiger_t *way = new zeiger_t( welt, start, sp );
	const bruecke_besch_t::img_t img0 = gr->get_grund_hang()==0 ? besch->get_rampe(ribi_mark) : besch->get_start(ribi_mark);
	gr->obj_add( way );
	way->set_bild(besch->get_hintergrund(img0, 0));
	way->set_after_bild(besch->get_vordergrund(img0, 0));
	if (gr->get_grund_hang()!=0) {
		way->set_yoff(-TILE_HEIGHT_STEP);
	}
	// eventually we have to remove trees on start tile
	if (besch->get_waytype() != powerline_wt) {
		for(  uint8 i=0;  i<gr->get_top();  i++  ) {
			ding_t *dt = gr->obj_bei(i);
			switch(dt->get_typ()) {
				case ding_t::baum:
					costs -= welt->get_einstellungen()->cst_remove_tree;
					break;
				case ding_t::groundobj:
					costs += ((groundobj_t *)dt)->get_besch()->get_preis();
					break;
				default: break;
			}
		}
	}
	marked[sp->get_player_nr()].insert( way );
	way->mark_image_dirty( way->get_bild(), 0 );
	// loop
	koord3d pos(start + zv + koord3d(0,0,1));
	while (pos.get_2d()!=end.get_2d()) {
		grund_t *gr = welt->lookup( pos );
		if( !gr ) {
			gr = new monorailboden_t(welt, pos, 0);
			gr->set_grund_hang( 0 );
			welt->access(pos.get_2d())->boden_hinzufuegen(gr);
		}
		zeiger_t *way = new zeiger_t( welt, pos, sp );
		gr->obj_add( way );
		way->set_bild(besch->get_hintergrund(besch->get_simple(ribi_mark),0));
		way->set_after_bild(besch->get_vordergrund(besch->get_simple(ribi_mark), 0));
		marked[sp->get_player_nr()].insert( way );
		way->mark_image_dirty( way->get_bild(), 0 );
		pos = pos + zv;
	}
	costs += besch->get_preis() * koord_distance(start, pos);
	// end
	gr = welt->lookup(end);
	if (gr->ist_karten_boden() && end.z==start.z) {
		zeiger_t *way = new zeiger_t( welt, end, sp );
		const bruecke_besch_t::img_t img1 = gr->get_grund_hang()==0 ? besch->get_rampe(ribi_t::rueckwaerts(ribi_mark)) : besch->get_start(ribi_t::rueckwaerts(ribi_mark));
		gr->obj_add( way );
		way->set_bild(besch->get_hintergrund(img1, 0));
		way->set_after_bild(besch->get_vordergrund(img1, 0));
		if (gr->get_grund_hang()!=0) {
			way->set_yoff(-TILE_HEIGHT_STEP);
		}
		marked[sp->get_player_nr()].insert( way );
		way->mark_image_dirty( way->get_bild(), 0 );
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
			ding_t *dt = gr->obj_bei(i);
			switch(dt->get_typ()) {
				case ding_t::baum:
					costs -= welt->get_einstellungen()->cst_remove_tree;
					break;
				case ding_t::groundobj:
					costs += ((groundobj_t *)dt)->get_besch()->get_preis();
					break;
				default: break;
			}
		}
	}
	win_set_static_tooltip( tooltip_with_price("Building costs estimates", costs ) );
}
uint8 wkz_brueckenbau_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d &start )
{
	const bruecke_besch_t *besch = brueckenbauer_t::get_besch(default_param);
	const waytype_t wt = besch->get_waytype();

	error = NULL;
	grund_t *gr = welt->lookup(pos);
	if (gr==NULL  || !brueckenbauer_t::ist_ende_ok(sp,gr) || !hang_t::ist_wegbar(gr->get_grund_hang()) ) {
		return 0;
	}
	if (welt->lookup(pos + koord3d(0,0,1))) {
		return 0;
	}
	if (is_first_click(sp)) {
		// first click
		if (!gr->ist_karten_boden()) {
			return 0;
		}
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
		if (rw!=ribi_t::keine && !ribi_t::ist_einfach(rw)) {
			return 0;
		}
		// determine possible directions
		ribi = ribi_t::rueckwaerts(rw);
		return (ribi!=ribi_t::keine ? 2 : 0) | (ribi_t::ist_einfach(ribi) ? 1 : 0);
	}
	else {
		// second click
		if (start.z > pos.z || start.z+1 < pos.z ) {
			return 0;
		}
		// dragging in the right direction?
		ribi_t::ribi test = ribi_typ(pos - start);
		if (!ribi_t::ist_einfach(test)  ||  ((test & (~ribi))!=0) ) {
			return 0;
		}
		// check whether we can build a bridge here
		const char *error = NULL;
		koord3d end = brueckenbauer_t::finde_ende(welt, start, koord(test), besch, error, false, koord_distance(start, pos));
		if (end!=pos) {
			koord3d end = brueckenbauer_t::finde_ende(welt, start, koord(test), besch, error, false, koord_distance(start, pos));
			return 0;
		}
		return 2;
	}
}


/* more difficult, since this builds also underground ways */
const char *wkz_tunnelbau_t::get_tooltip(spieler_t *sp)
{
	const tunnel_besch_t * besch = tunnelbauer_t::get_besch(default_param);
	tooltip_with_price_maintenance( sp->get_welt(), besch->get_name(), -besch->get_preis(), besch->get_wartung() );
	size_t n= strlen(toolstr);
	if(besch->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
	}
	return toolstr;
}

const char *wkz_tunnelbau_t::check( karte_t *welt, spieler_t *sp, koord3d pos)
{
	if (grund_t::underground_mode == grund_t::ugm_all) {
		return NULL;
	}
	else {
		return two_click_werkzeug_t::check(welt, sp, pos);
	}
}

void wkz_tunnelbau_t::calc_route( wegbauer_t &bauigel, const koord3d &start, const koord3d &end, karte_t *welt )
{
	const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
	int bt = besch->get_waytype()|wegbauer_t::tunnel_flag;
	const weg_besch_t *wb = wegbauer_t::weg_search( besch->get_waytype(), besch->get_topspeed(), welt->get_timeline_year_month(), weg_t::type_flat );
	bauigel.route_fuer((wegbauer_t::bautyp_t)bt, wb, besch);
	bauigel.set_keep_existing_faster_ways( !is_ctrl_pressed() );
	// wegbauer tries to find route to 3d coordinate if no ground at end exists or is not kartenboden
	bauigel.calc_straight_route(start,end);
}

const char *wkz_tunnelbau_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	if( end == koord3d::invalid ) {
		// Build tunnel mouths
		if (welt->lookup_kartenboden(start.get_2d())->get_hoehe() == start.z) {
			const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
			return tunnelbauer_t::baue( welt, sp, start.get_2d(), besch, !is_ctrl_pressed() );
		}
		else {
			return "";
		}
	}
	else {
		// Build tunnels
		wegbauer_t bauigel(welt, sp);
		calc_route( bauigel, start, end, welt );
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);
		welt->lookup_kartenboden(end.get_2d())->clear_flag(grund_t::marked);
		return NULL;
	}
}

uint8 wkz_tunnelbau_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d & )
{
	if(  !is_first_click(sp)  ) {
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
			wegbauer_t bauigel(welt, sp);
			if(!bauigel.check_owner( w->get_besitzer(), sp )) {
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

void wkz_tunnelbau_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	wegbauer_t bauigel(welt, sp);
	calc_route( bauigel, start, end, welt );

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
				gr = new tunnelboden_t(welt, pos, 0);
				welt->access(pos.get_2d())->boden_hinzufuegen(gr);
			}
			ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(wb->get_wtyp()) | bauigel.get_route().get_ribi( j );

			zeiger_t *way = new zeiger_t( welt, pos, sp );
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
			marked[sp->get_player_nr()].insert( way );
			way->mark_image_dirty( way->get_bild(), 0 );
		}
		welt->lookup(end)->set_flag(grund_t::marked);
	}
}

/* removes a way like a driving car ... */
const char *wkz_wayremover_t::get_tooltip(spieler_t *)
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

class electron_t : public fahrer_t {
	bool ist_befahrbar(const grund_t* gr) const { return gr->get_leitung()!=NULL; }
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_leitung()->get_ribi(); }
	virtual waytype_t get_waytype() const { return invalid_wt; }
	virtual int get_kosten(const grund_t *, const sint32, koord) const { return 1; }
	virtual bool ist_ziel(const grund_t *,const grund_t *) const { return false; }
};

void wkz_wayremover_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, sp, start, end );
	if( can_built ) {
		for( uint32 j = 0; j < verbindung.get_count(); j++ ) {
			koord3d pos = verbindung.position_bei(j);
			zeiger_t *marker = new zeiger_t( welt, pos, NULL );
			marker->set_bild( cursor );
			marker->mark_image_dirty( marker->get_bild(), 0 );
			marked[sp->get_player_nr()].insert( marker );
			welt->lookup(pos)->obj_add( marker );
		}
	}
}

uint8 wkz_wayremover_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d & )
{
	// search for starting ground
	waytype_t wt = (waytype_t)atoi(default_param);
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos,wt);
	if(gr==NULL) {
		DBG_MESSAGE("wkz_wayremover()", "no ground on %i,%i",pos.x, pos.y);
		// wrong ground or not this way here => exit
		return 0;
	}
	// do not remove ground from depot
	if(gr->get_depot()) {
		error = "No suitable ground!";
		return 0;
	}
	error = NULL;
	return 2;
}


bool wkz_wayremover_t::calc_route( route_t &verbindung, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	waytype_t wt = (waytype_t)atoi(default_param);
	if (wt == tram_wt) {
		wt = track_wt;
	}
	karte_t *welt = sp->get_welt();

	if(  start == end  ) {
		verbindung.clear();
		grund_t *gr=welt->lookup(start);
		if(  gr  &&  (wt!=powerline_wt ? gr->get_weg(wt)!=NULL : gr->get_leitung()!=NULL) ) {
			verbindung.append( start );
		}
	}
	else {
		// get a default vehikel
		fahrer_t* test_driver;
		if(  wt!=powerline_wt  ) {
			vehikel_besch_t remover_besch(wt, 500, vehikel_besch_t::diesel );
			vehikel_t *driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
			driver->set_flag( ding_t::not_on_map );
			test_driver = driver;
		}
		else {
			test_driver = new electron_t();
		}
		verbindung.calc_route(sp->get_welt(), start, end, test_driver, 0);
		delete test_driver;
	}
	DBG_MESSAGE("wkz_wayremover()","route with %d tile found",verbindung.get_count());

	calc_route_error = NULL;
	bool can_delete = start == end  ||  verbindung.get_count()>1;
	if(  can_delete  ) {
		// found a route => check if I can delete anything on it
		for(  uint32 i=0;  can_delete  &&  i<verbindung.get_count();  i++  ) {
			grund_t *gr=welt->lookup(verbindung.position_bei(i));
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
					ding_t *d = gr->obj_bei(i);
					const uint8 type = d->get_typ();
					// ignore pillars, powerlines
					if (type == ding_t::pillar  ||  type==ding_t::leitung) {
						continue;
					}
					// ignore flying aircraft
					if (type == ding_t::aircraft  &&  !(static_cast<aircraft_t*>(d)->is_on_ground())) {
						continue;
					}
					const waytype_t ding_wt = d->get_waytype();
					// way-related things
					if (ding_wt != invalid_wt) {
						// check this thing if it has the same waytype or if we want to remove the whole bridge/tunnel tile
						// special case: stations - take care not to produce station without any way
						const bool lonely_station = type==ding_t::gebaeude  &&  !gr->has_two_ways();
						if (check_all ||  ding_wt == wt  ||  lonely_station) {
							can_delete = (calc_route_error = d->ist_entfernbar(sp)) == NULL;
						}
					}
					// all other stuff
					else {
						can_delete = (calc_route_error = d->ist_entfernbar(sp)) == NULL;
					}
				}
			}
			else {
				// for powerline: only a ground and a powerline to remove
				if(  gr==NULL  ||  gr->get_leitung()==NULL  ||  (calc_route_error = gr->get_leitung()->ist_entfernbar(sp))!=NULL  ) {
					can_delete = false;
					break;
				}
			}
		}
	}
	DBG_MESSAGE("wkz_wayremover()", "route search returned %d", can_delete);

	return can_delete;
}

const char *wkz_wayremover_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	waytype_t wt = (waytype_t)atoi(default_param);
	route_t verbindung;
	if( !calc_route( verbindung, sp, start, end )  ) {
		DBG_MESSAGE("wkz_wayremover()","no route found");
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
					if(gr->ist_karten_boden()) {
						const char *err = NULL;
						err = brueckenbauer_t::remove(welt,sp,verbindung.position_bei(i),wt);
						if(err) {
							return err;
						}
						gr = welt->lookup(verbindung.position_bei(i));
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
					can_delete &= gr->remove_everything_from_way(sp,wt,rem);
					if(can_delete  &&  gr->get_weg(wt)==NULL) {
						if(gr->get_weg_nr(0)!=0) {
							gr->remove_everything_from_way(sp,gr->get_weg_nr(0)->get_waytype(),ribi_t::keine);
						}
						gr->obj_loesche_alle(sp);
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
					can_delete &= gr->remove_everything_from_way(sp,wt,rem);
				}
			}
			else {
				leitung_t *lt = gr->get_leitung();
				if(  lt  &&  (rem&lt->get_ribi())==0  ) {
					// remove only single connections
					lt->entferne(sp);
					delete lt;
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
const way_obj_besch_t *wkz_wayobj_t::default_electric = NULL;

const char *wkz_wayobj_t::get_tooltip(spieler_t *sp)
{
	if(  build  ) {
		const way_obj_besch_t *besch = get_besch(sp->get_welt());
		if(besch) {
			tooltip_with_price_maintenance( sp->get_welt(), besch->get_name(), -besch->get_preis(), besch->get_wartung() );
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
		wt = (waytype_t)atoi( default_param );
		sprintf( toolstr, translator::translate("Remove wayobj %s"), translator::translate(weg_t::waytype_to_string(wt)) );
		return toolstr;
	}
}

const way_obj_besch_t *wkz_wayobj_t::get_besch( const karte_t* welt ) const
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

bool wkz_wayobj_t::is_selected( karte_t *welt ) const
{
	const wkz_wayobj_t *selected = dynamic_cast<const wkz_wayobj_t *>(welt->get_werkzeug(welt->get_active_player_nr()));
	return (selected  &&  selected->build==build  &&  selected->get_besch(welt) == get_besch(welt));
}

bool wkz_wayobj_t::init( karte_t *welt, spieler_t *sp )
{
	two_click_werkzeug_t::init( welt, sp );

	if( build ) {
		besch = get_besch(welt);
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

bool wkz_wayobj_t::calc_route( route_t &verbindung, spieler_t *sp, const koord3d& start, const koord3d& to )
{
	// get a default vehikel
	vehikel_besch_t remover_besch( wt, 500, vehikel_besch_t::diesel );
	vehikel_t* test_driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
	test_driver->set_flag( ding_t::not_on_map );
	bool can_built;
	if( start != to ) {
		can_built = verbindung.calc_route(sp->get_welt(), start, to, test_driver, 0);
	}
	else {
		verbindung.clear();
		verbindung.append( start );
		can_built = true;
	}
	delete test_driver;
	return can_built;
}

uint8 wkz_wayobj_t::is_valid_pos( karte_t * welt, spieler_t * sp, const koord3d& pos, const char *&error, const koord3d & )
{
	// search for starting ground
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp, welt, pos, wt );
	if(  gr == NULL  ) {
		DBG_MESSAGE("wkz_wayobj_t::is_valid_pos()", "no ground on %s",pos.get_str());
		// wrong ground or not this way here => exit
		return 0;
	}
	error = NULL;
	return 2;
}

void wkz_wayobj_t::mark_tiles( karte_t * welt, spieler_t * sp, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, sp, start, end );
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
				way_obj = new zeiger_t( welt, pos, NULL );
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
					way_obj = new zeiger_t( welt, pos, NULL );
					way_obj->set_bild( cursor ); //skinverwaltung_t::bauigelsymbol->get_bild_nr(0));
				}
			}
			if( way_obj ) {
				way_obj->mark_image_dirty( way_obj->get_bild(), 0 );
				gr->obj_add( way_obj );
				marked[sp->get_player_nr()].insert( way_obj );
			}
		}
		win_set_static_tooltip( tooltip_with_price("Building costs estimates", -cost_estimate ) );
	}
}

const char *wkz_wayobj_t::do_work( karte_t * welt, spieler_t * sp, const koord3d &start, const koord3d &end )
{
	route_t verbindung;
	bool can_built = calc_route( verbindung, sp, start, end );
	DBG_MESSAGE("wkz_wayobj_t::work()","route search returned %d",can_built);

	if(!can_built) {
		return "Ways not connected";
	}

	// built wayobj ...
	koord3d_vector_t const& r = verbindung.get_route();
	for(uint32 i=0;  i<verbindung.get_count();  i++  ) {
		if( build ) {
			wayobj_t::extend_wayobj_t(welt, r[i], sp, r.get_ribi(i), besch);
		}
		else {
			if (wayobj_t* const wo = welt->lookup(r[i])->find<wayobj_t>()) {
				const char *err = wo->ist_entfernbar( sp );
				if( !err ) {
					wo->entferne( sp );
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
const char *wkz_station_t::wkz_station_building_aux(karte_t *welt, spieler_t *sp, bool extend_public_halt, koord3d k, const haus_besch_t *besch, sint8 rotation )
{
	// need kartenboden
	if (welt->lookup_kartenboden(k.get_2d())->get_hoehe() != k.z) {
		return "";
	}
	koord pos = k.get_2d();
DBG_MESSAGE("wkz_station_building_aux()", "building mail office/station building on square %d,%d", pos.x, pos.y);

	// Player sp pays for the construction
	// but we try to extend stations of Player new_owner that may be the public player
	spieler_t *new_owner = extend_public_halt ? welt->get_spieler(1) : sp;

	koord size = besch->get_groesse();
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
				if(welt->ist_platz_frei(pos-offset, testsize.x, testsize.y, NULL, besch->get_allowed_climate_bits())) {
					// first we must check over/under halt
					halthandle_t last_halt = halthandle_t();
					for(  sint16 x=0;  x<testsize.x;  x++  ) {
						for(  sint16 y=0;  y<testsize.y;  y++  ) {
							const planquadrat_t *pl = welt->lookup( pos-offset+koord(x,y) );
							halthandle_t test_halt = pl->get_halt();
							if(test_halt.is_bound()) {
								if(!spieler_t::check_owner( new_owner, test_halt->get_besitzer())) {
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
					koord test_start = pos-offset;
					// find all surrounding tiles with a stop
					int neighbour_halt_n = 0, neighbour_halt_s = 0, neighbour_halt_e = 0, neighbour_halt_w = 0;
					int best_halt_n = 0, best_halt_s = 0, best_halt_e = 0, best_halt_w = 0;
					// test also diagonal corners (that is why from -1 to size!)
					for(  sint16 y=-1;  y<=testsize.y;  y++  ) {
						// left (for all tiles, even bridges)
						const planquadrat_t *pl = welt->lookup( test_start+koord(-1,y) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  new_owner==pl->get_halt()->get_besitzer()  ) {
							halt = pl->get_halt();
							for(  uint b=0;  b<pl->get_boden_count();  b++  ) {
								grund_t *gr = pl->get_boden_bei(b);
								if(  gr->is_halt()  ) {
									neighbour_halt_w ++;
									gebaeude_t *gb = gr->find<gebaeude_t>();
									if(  gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra()  ) {
										best_halt_w ++;
									}
								}
							}
						}
						pl = welt->lookup( test_start+koord(testsize.x,y) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  new_owner==pl->get_halt()->get_besitzer()  ) {
							halt = pl->get_halt();
							for(  uint b=0;  b<pl->get_boden_count();  b++  ) {
								grund_t *gr = pl->get_boden_bei(b);
								if(  gr->is_halt()  ) {
									neighbour_halt_e ++;
									gebaeude_t *gb = gr->find<gebaeude_t>();
									if(  gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra()  ) {
										best_halt_e ++;
									}
								}
							}
						}
					}
					// corners were already checked, but to get correct numbers, we must check them again here
					for(  sint16 x=-1;  x<=testsize.x;  x++  ) {
						// upper and lower
						const planquadrat_t *pl = welt->lookup( test_start+koord(x,-1) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  new_owner==pl->get_halt()->get_besitzer()  ) {
							halt = pl->get_halt();
							for(  uint b=0;  b<pl->get_boden_count();  b++  ) {
								grund_t *gr = pl->get_boden_bei(b);
								if(  gr->is_halt()  ) {
									neighbour_halt_n ++;
									gebaeude_t *gb = gr->find<gebaeude_t>();
									if(  gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra()  ) {
										best_halt_n ++;
									}
								}
							}
						}
						pl = welt->lookup( test_start+koord(x,testsize.y) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  new_owner==pl->get_halt()->get_besitzer()  ) {
							halt = pl->get_halt();
							for(  uint b=0;  b<pl->get_boden_count();  b++  ) {
								grund_t *gr = pl->get_boden_bei(b);
								if(  gr->is_halt()  ) {
									neighbour_halt_s ++;
									gebaeude_t *gb = gr->find<gebaeude_t>();
									if(  gr->hat_wege()  &&  gb  &&  gb->get_tile()->get_besch()->get_extra()==besch->get_extra()  ) {
										best_halt_s ++;
									}
								}
							}
						}
					}
					// now find out, if this offset/rotation is better ... (i.e. matches more fitting buildings)
					if(  r==0  ) {
						// r=0 is either facing south or north
						if(  best_halt_n>best_halt  ||  (best_halt==0  &&  neighbour_halt_n>any_halt)  ) {
							best_halt = best_halt_n;
							any_halt = neighbour_halt_n;
							rotation = 0;
							offsets = offset;
						}
						if(  best_halt_s>best_halt  ||  (best_halt==0  &&  neighbour_halt_s>any_halt)  ) {
							best_halt = best_halt_s;
							any_halt = neighbour_halt_s;
							rotation = 2;
							offsets = offset;
						}
					}
					else {
						// r=1 is either facing east or west
						if(  best_halt_w>best_halt  ||  (best_halt==0  &&  neighbour_halt_w>any_halt)  ) {
							best_halt = best_halt_w;
							any_halt = neighbour_halt_w;
							rotation = 1;
							offsets = offset;
						}
						if(  best_halt_e>best_halt  ||  (best_halt==0  &&  neighbour_halt_e>any_halt)  ) {
							best_halt = best_halt_e;
							any_halt = neighbour_halt_e;
							rotation = 3;
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
				const planquadrat_t *pl = welt->lookup( pos-offsets+koord(x,y) );
				halthandle_t test_halt = pl->get_halt();
				if( test_halt.is_bound()  &&  spieler_t::check_owner( new_owner, test_halt->get_besitzer()) ) {
					halt = test_halt;
					break;
				}
			}
		}
		// is there no halt to connect?
		if(  !halt.is_bound()  ) {
			return "Post muss neben\nHaltestelle\nliegen!\n";
		}
	}
	else {
		// rotation was pre-slected; just search for stop now
		assert(  rotation < besch->get_all_layouts()  );
		koord testsize = besch->get_groesse(rotation);
		offsets = koord(0,0);

		if(  !welt->ist_platz_frei(pos, testsize.x, testsize.y, NULL, besch->get_allowed_climate_bits())  ) {
			return "Tile not empty.";
		}
		// check over/under halt
		for(  sint16 x=0;  x<testsize.x;  x++  ) {
			for(  sint16 y=0;  y<testsize.y;  y++  ) {
				const planquadrat_t *pl = welt->lookup(pos+koord(x,y));
				halthandle_t test_halt = pl->get_halt();
				if(test_halt.is_bound()) {
					if(!spieler_t::check_owner( new_owner, test_halt->get_besitzer())) {
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
		if(!halt.is_bound()) {
			halt = suche_nahe_haltestelle(new_owner, welt, welt->lookup_kartenboden(pos)->get_pos(), besch->get_b(rotation), besch->get_h(rotation) );
			// is there no halt to connect?
			if(  !halt.is_bound()  ) {
				return "Post muss neben\nHaltestelle\nliegen!\n";
			}
		}
	}

	if(  rotation>besch->get_all_layouts()  ) {
		rotation %= besch->get_all_layouts();
	}

	hausbauer_t::baue(welt, halt->get_besitzer(), k-offsets, rotation, besch, &halt);

	sint64 cost = welt->get_einstellungen()->cst_multiply_post*besch->get_level()*besch->get_b()*besch->get_h();
	if(sp!=halt->get_besitzer()  &&  halt->get_besitzer()==welt->get_spieler(1)) {
		// public stops are expensive!
		cost -= ((welt->get_einstellungen()->maint_building*besch->get_level()*besch->get_b()*besch->get_h()*60)<<(welt->ticks_per_world_month_shift-18));
	}
	sp->buche( cost, pos, COST_CONSTRUCTION);
	halt->recalc_station_type();

	return NULL;
}

/* build a dock either small or large */
const char *wkz_station_t::wkz_station_dock_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch)
{
	// the cursor cannot be outside the map from here on
	koord pos = k.get_2d();
	grund_t *gr = welt->lookup_kartenboden(pos);
	if (gr->get_hoehe()!= k.z) {
		return "";
	}
	hang_t::typ hang = gr->get_grund_hang();
	// first get the size
	int len = besch->get_groesse().y-1;
	koord dx = koord((hang_t::typ)hang);
	koord last_pos = pos - dx*len;
	halthandle_t halt = halthandle_t();

	// check, if we can build here ...
	if(!hang_t::ist_einfach(hang)) {
		return "Dock must be built on single slope!";
	}
	else {
		for(int i=0;  i<=len;  i++  ) {
			if(!welt->ist_in_kartengrenzen(pos-dx*i)) {
				// need at least a single tile to navigate ...
				return "Zu nah am Kartenrand";
			}
			else {
				halthandle_t test_halt = welt->lookup(pos-dx*i)->get_halt();
				if(test_halt.is_bound()) {
					if(!spieler_t::check_owner( sp, test_halt->get_besitzer())) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					else if(!halt.is_bound()) {
						halt = test_halt;
					}
					else if(halt != test_halt) {
						 return "Several halts found.";
					}
				}
				else {
					const grund_t *gr=welt->lookup_kartenboden(pos-dx*i);
					const char *msg = gr->kann_alle_obj_entfernen(sp);
					if(msg) {
						return msg;
					}
					else if((i==0  &&  (gr->ist_wasser()  ||  gr->hat_wege()  ||  gr->get_typ()!=grund_t::boden )) ||  gr->kann_alle_obj_entfernen(sp)!=NULL  ||  gr->is_halt()) {
						return "Tile not empty.";
					}
					else if (i!=0  &&  (!gr->ist_wasser() || gr->find<gebaeude_t>() || gr->get_depot() || gr->is_halt())) {
						return "Tile not empty.";
					}
				}
			}
		}
	}

	// remove everything from tile
	gr->obj_loesche_alle(sp);

DBG_MESSAGE("wkz_dockbau()","building dock from square (%d,%d) to (%d,%d)", pos.x, pos.y, last_pos.x, last_pos.y);
	int layout = 0;
	koord3d bau_pos = welt->lookup_kartenboden(pos)->get_pos();
	koord dx2;
	switch(hang) {
		case hang_t::sued:
			layout = 0;
			dx2 = koord::west;
			break;
		case hang_t::ost:
			layout = 1;
			dx2 = koord::nord;
			break;
		case hang_t::nord:
			layout = 2;
			dx2 = koord::west;
			bau_pos = welt->lookup_kartenboden(last_pos)->get_pos();
			break;
		case hang_t::west:
			layout = 3;
			dx2 = koord::nord;
			bau_pos = welt->lookup_kartenboden(last_pos)->get_pos();
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
	gr = welt->lookup_kartenboden(pos+dx2);

	// find out if middle end or start tile
	if(gr  &&  gr->is_halt()  &&  spieler_t::check_owner( sp, gr->get_halt()->get_besitzer() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::hafen) {
			if(change_layout) {
				layout -= 4;
			}
			if(gb->get_tile()->get_besch()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x02) {
					gb->set_tile( gb->get_tile()->get_besch()->get_tile(ly&0xFD,0,0) );
				}
			}
		}
	}

	gr = welt->lookup_kartenboden(pos-dx2);
	if(gr  &&  gr->is_halt()  &&  spieler_t::check_owner( sp, gr->get_halt()->get_besitzer() )) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb  &&  gb->get_tile()->get_besch()->get_utyp()==haus_besch_t::hafen) {
			if(change_layout) {
				layout -= 2;
			}
			if(gb->get_tile()->get_besch()->get_all_layouts()==16) {
				sint8 ly = gb->get_tile()->get_layout();
				if(ly&0x04) {
					gb->set_tile( gb->get_tile()->get_besch()->get_tile(ly&0xFB,0,0) );
				}
			}
		}
	}

//DBG_MESSAGE("wkz_dockbau()","search for stop");
	if(!halt.is_bound()) {
		halt = suche_nahe_haltestelle(sp, welt, welt->lookup_kartenboden(pos)->get_pos() );
	}
	bool neu = !halt.is_bound();

	if(neu) { // neues dock
		halt = sp->halt_add(pos);
	}
	hausbauer_t::baue(welt, halt->get_besitzer(), bau_pos, layout, besch, &halt);
	sint64 costs = welt->get_einstellungen()->cst_multiply_dock*besch->get_level();
	if(sp!=halt->get_besitzer()) {
		// public stops are expensive!
		costs -= ((welt->get_einstellungen()->maint_building*besch->get_level()*60)<<(welt->ticks_per_world_month_shift-18));
	}
	for(int i=0;  i<=len;  i++ ) {
		koord p=pos-dx*i;
		sp->buche( costs, p, COST_CONSTRUCTION);
	}

	halt->recalc_station_type();
	if(umgebung_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==pos) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}

	if(neu) {
		char* name = halt->create_name(pos, "Dock", welt->get_einstellungen()->get_name_language_id() );
		halt->set_name( name );
		free(name);
	}
	return NULL;
}

// build all types of stops but sea harbours
const char *wkz_station_t::wkz_station_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch, waytype_t wegtype, sint64 cost, const char *type_name )
{
	koord pos = k.get_2d();
DBG_MESSAGE("wkz_halt_aux()", "building %s on square %d,%d for waytype %x", besch->get_name(), pos.x, pos.y, wegtype);
	const char *p_error=(besch->get_all_layouts()==4) ? "No terminal station here!" : "No through station here!";

	// underground is checked in work(); if underground only simple stations are allowed
	// get valid ground
	grund_t *bd = wkz_intern_koord_to_weg_grund( sp==welt->get_spieler(1)?NULL:sp,welt,k,wegtype);

	if(!bd  ||  bd->get_weg_hang()!=hang_t::flach) {
		// only flat tiles, only one stop per map square
		return "No suitable ground!";
	}

	if(bd->ist_tunnel()  &&  bd->ist_karten_boden()) {		// do not build on tunnel entries
		return "No suitable ground!";
	}

	if(bd->get_depot()) {
		return "No suitable ground!";
	}

	if(bd->hat_weg(air_wt)  &&  bd->get_weg(air_wt)->get_besch()->get_styp()!=0) {
		return "No suitable ground!";
	}

	// find out orientation ...
	uint32 layout = 0;
	ribi_t::ribi  ribi=ribi_t::dir_invalid;
	if(besch->get_all_layouts()==2 || besch->get_all_layouts()==8 || besch->get_all_layouts()==16) {
		// through station
		if(bd->has_two_ways()) {
			// a crossing or maybe just a tram track on a road ...
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked()  |  bd->get_weg_nr(1)->get_ribi_unmasked();
		}
		else if (bd->hat_wege()) {
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		}
		// not straight: sorry cannot build here ...
		if(!ribi_t::ist_gerade(ribi)) {
			return p_error;
		}
		layout = (ribi & ribi_t::nordsued)?0 :1;
	}
	else if(besch->get_all_layouts()==4) {
		// terminal station
		if (bd->hat_wege()) {
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked();
		}
		// sorry cannot build here ... (not a terminal tile)
		if(!ribi_t::ist_einfach(ribi)) {
			return p_error;
		}

		switch(ribi) {
			//case ribi_t::sued:layout = 0;  break;
			case ribi_t::ost:   layout = 1;    break;
			case ribi_t::nord:  layout = 2;    break;
			case ribi_t::west:  layout = 3;    break;
		}
	} else {
		// something wrong with station number of layouts
		dbg->fatal( "wkz_station_t::wkz_station_aux", "%s has wrong number of layouts (must be 2,4,8,16!)", besch->get_name() );
		return p_error;
	}

	if(besch->get_all_layouts()==8 || besch->get_all_layouts()==16) {
		// through station - complex layout
		// bits
		// 1 = north south/east west (as simple layout)
		// 2 = use far end image  \ can be combined
		// 3 = use near end image / to use both end image
		// 4 = platform face - 0 = far, 1 = near

		// bit 1 has already been set

		ribi_t::ribi next_halt = ribi_t::keine;
		ribi_t::ribi next_own = ribi_t::keine;

		sint8 offset = bd->get_hoehe()+bd->get_weg_yoff()/TILE_HEIGHT_STEP;

		grund_t *gr;
		sint32 neighbour_layout[] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
		for(unsigned i=0;  i<4;  i++) {
			// oriented buildings here - get neighbouring layouts
			const planquadrat_t *plan = welt->lookup(pos+koord::nsow[i]);
			if(plan  &&  plan->get_halt().is_bound()) {
				// ok, here is a halt at least
				next_halt |= ribi_t::nsow[i];
				gr = welt->lookup(koord3d(pos+koord::nsow[i],offset));
				if(!gr) {
					// check whether bridge end tile
					grund_t * gr_tmp = welt->lookup(koord3d(pos+koord::nsow[i],offset-1));
					if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
						gr = gr_tmp;
					}
				}
				if(gr) {
					// check, if there is an oriented stop
					const gebaeude_t* gb = gr->find<gebaeude_t>();
					if(gb  &&  gb->get_tile()->get_besch()->get_all_layouts()>4  &&  gb->get_tile()->get_besch()->get_utyp()>haus_besch_t::hafen) {
						next_own |= ribi_t::nsow[i];
						neighbour_layout[ribi_t::nsow[i]] = gb->get_tile()->get_layout();
					}
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
	uint16 old_level = 0;

	halthandle_t halt;

	if( old_halt.is_bound() ) {
		gebaeude_t* gb = bd->find<gebaeude_t>();
		const haus_besch_t *old_besch = gb->get_tile()->get_besch();
		old_level = old_besch->get_level();
		if( old_besch->get_level() >= besch->get_level() ) {
			return "Upgrade must have\na higher level";
		}
		gb->entferne( NULL );
		delete gb;
		halt = old_halt;
	}
	else {
		halt = suche_nahe_haltestelle(sp,welt,bd->get_pos());
	}

	// seems everything ok, lets build
	bool neu = !halt.is_bound();

	if(neu) {
		halt = sp->halt_add(pos);
	}
	hausbauer_t::neues_gebaeude( welt, halt->get_besitzer(), bd->get_pos(), layout, besch, &halt);
	halt->recalc_station_type();

	if(neu) {
		char* name = halt->create_name(pos, type_name, welt->get_einstellungen()->get_name_language_id() );
		halt->set_name( name );
		free(name);
	}

	sint64 old_cost = old_level * cost;
	cost *= besch->get_level()*besch->get_b()*besch->get_h();
	cost -= old_cost/2;
	if(sp!=halt->get_besitzer()) {
		// public stops are expensive!
		cost -= ((welt->get_einstellungen()->maint_building*besch->get_level()*besch->get_b()*besch->get_h()*60)<<(welt->ticks_per_world_month_shift-18));
	}
	sp->buche( cost, pos, COST_CONSTRUCTION);
	if(umgebung_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==pos) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}
	return NULL;
}

// gives the description and sets the rotation value
const haus_besch_t *wkz_station_t::get_besch( sint8 &rotation ) const
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

bool wkz_station_t::init( karte_t *welt, spieler_t * )
{
	sint8 rotation = -1;
	const haus_besch_t *hb = get_besch( rotation );
	if(  hb==NULL  ) {
		return false;
	}
	cursor = hb->get_cursor()->get_bild_nr(0);
	if(  !is_local_execution()  ) {
		// do not change cursor
		return false;
	}
	if(  hb->get_utyp()==haus_besch_t::generic_extension  &&  hb->get_all_layouts()>1  ) {
		if(  is_ctrl_pressed()  &&  rotation==-1  ) {
			// call station dialog instead
			destroy_win( magic_station_building_select );
			create_win( new station_building_select_t(welt, hb), w_info, magic_station_building_select);
			// we do not activate building yet; else uncomment the return statement
//			welt->get_zeiger()->set_area( koord( welt->get_einstellungen()->get_station_coverage()*2+1, welt->get_einstellungen()->get_station_coverage()*2+1 ), true );
			return false;
		}
		else if(  rotation>=0  ) {
			// rotation is already fixed
			welt->get_zeiger()->set_area( koord( hb->get_b(rotation), hb->get_h(rotation) ), false );
		}
		else {
			welt->get_zeiger()->set_area( koord( welt->get_einstellungen()->get_station_coverage()*2+1, welt->get_einstellungen()->get_station_coverage()*2+1 ), true );
		}
	}
	else {
		welt->get_zeiger()->set_area( koord( welt->get_einstellungen()->get_station_coverage()*2+1, welt->get_einstellungen()->get_station_coverage()*2+1 ), true );
	}
	return true;
}


image_id wkz_station_t::get_icon( spieler_t * ) const
{
	if(  grund_t::underground_mode==grund_t::ugm_all  ) {
		// in underground mode, buildings will be done invisible above ground => disallow such confusion
		sint8 dummy;
		const haus_besch_t *besch=get_besch(dummy);
		if(  besch->get_utyp()!=haus_besch_t::generic_stop  ||  besch->get_extra()==air_wt) {
			return IMG_LEER;
		}
	}
	return icon;
}



char const* wkz_station_t::get_tooltip(spieler_t* const sp)
{
	sint8                  dummy;
	karte_t&               welt     = *sp->get_welt();
	einstellungen_t const& settings = *welt.get_einstellungen();
	haus_besch_t    const& besch    = *get_besch(dummy);
	uint32                 level    = besch.get_level();
	sint64                 price;
	switch (besch.get_utyp()) {
		case haus_besch_t::generic_stop:
			switch (besch.get_extra()) {
				case track_wt:
				case monorail_wt:
				case maglev_wt:
				case tram_wt:
				case narrowgauge_wt: price = settings.cst_multiply_station;     break;
				case road_wt:        price = settings.cst_multiply_roadstop;    break;
				case water_wt:       price = settings.cst_multiply_dock;        break;
				case air_wt:         price = settings.cst_multiply_airterminal; break;
				case ignore_wt:      price = settings.cst_multiply_post;        break;
				default:             goto invalid;
			}
			break;

		case haus_besch_t::generic_extension: price = settings.cst_multiply_post; goto scale_by_size;
		case haus_besch_t::hafen:             price = settings.cst_multiply_dock; goto scale_by_size;
scale_by_size:
			level = level * besch.get_groesse().x * besch.get_groesse().y;
			break;

		default:
invalid:
			return "Illegal description";
	}
	return tooltip_with_price_maintenance_level(&welt, besch.get_name(), price * level, settings.maint_building * level, level, besch.get_enabled());
}


const char *wkz_station_t::check( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const char *msg = werkzeug_t::check(welt,sp, pos);
	if (msg==NULL) {
		sint8 rotation;
		const haus_besch_t *besch=get_besch(rotation);
		if(  grund_t::underground_mode==grund_t::ugm_all || (grund_t::underground_mode==grund_t::ugm_level && welt->lookup_kartenboden(pos.get_2d())->get_hoehe() > grund_t::underground_level) ) {
			// in underground mode, buildings will be done invisible above ground => disallow such confusion
			if(  besch->get_utyp()!=haus_besch_t::generic_stop  ||  besch->get_extra()==air_wt) {
				msg = "Cannot built this station/building\nin underground mode here.";
			}
		}
	}
	return msg;
}

const char *wkz_station_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const planquadrat_t *plan = welt->lookup(pos.get_2d());
	if(!plan) {
		return "";
	}

	// ownership allowed?
	halthandle_t halt = plan->get_halt();
	if(halt.is_bound()  &&  !spieler_t::check_owner( sp, halt->get_besitzer())) {
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	sint8 rotation;
	const haus_besch_t *besch=get_besch(rotation);
	const char *msg = NULL;
	switch (besch->get_utyp()) {
		case haus_besch_t::hafen:
			msg = wkz_station_t::wkz_station_dock_aux(welt, sp, pos, besch );
			break;
		case haus_besch_t::hafen_geb:
		case haus_besch_t::generic_extension:
			msg = wkz_station_t::wkz_station_building_aux(welt, sp, false, pos, besch, rotation );
			if (msg) {
				// try to build near a public halt
				msg = wkz_station_t::wkz_station_building_aux(welt, sp, true, pos, besch, rotation );
			}
			break;
		case haus_besch_t::generic_stop:
			switch(besch->get_extra()) {
				case road_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, road_wt, welt->get_einstellungen()->cst_multiply_roadstop, "H");
					break;
				case track_wt:
				case monorail_wt:
				case maglev_wt:
				case narrowgauge_wt:
				case tram_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, (waytype_t)besch->get_extra(), welt->get_einstellungen()->cst_multiply_station, "BF");
					break;
				case water_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, water_wt, welt->get_einstellungen()->cst_multiply_dock, "Dock");
					break;
				case air_wt:
					msg = wkz_station_t::wkz_station_aux(welt, sp, pos, besch, air_wt, welt->get_einstellungen()->cst_multiply_airterminal, "Airport");
					break;
			}

			break;
		default:
			dbg->fatal("wkz_station_t::work()","tool called for illegal besch \"%\"", default_param );
	}
	return msg;
}


// builds roadsigns and signals
wkz_roadsign_t::wkz_roadsign_t() : two_click_werkzeug_t()
{
	id = WKZ_ROADSIGN | GENERAL_TOOL;
	for (uint8 i=0; i<MAX_PLAYER_COUNT; i++) {
		signal_spacing[i] = 2;
		remove_intermediate_signals[i] = true;
		replace_other_signals[i] = true;
	}
	besch = NULL;
}

const char *wkz_roadsign_t::get_tooltip(spieler_t *)
{
	const roadsign_besch_t * besch = roadsign_t::find_besch(default_param);
	if(besch) {
		return tooltip_with_price( besch->get_name(), -besch->get_preis() );
	}
	return NULL;
}

void wkz_roadsign_t::draw_after( karte_t *welt, koord pos ) const
{
	if(  icon!=IMG_LEER  &&  is_selected(welt)  ) {
		display_img_blend( icon, pos.x, pos.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, true );
		char level_str[16];
		sprintf( level_str, "%i", signal_spacing[welt->get_active_player_nr()] );
		display_proportional( pos.x+4, pos.y+4, level_str, ALIGN_LEFT, COL_YELLOW, true );
	}
}

const char* wkz_roadsign_t::check_pos_intern(karte_t *welt, spieler_t *sp, koord3d pos)
{
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	if (besch==NULL) {
		// read data from string
		read_default_param(sp);
	}
	if (besch==NULL) {
		return error;
	}
	// search for starting ground
	grund_t *gr = wkz_intern_koord_to_weg_grund(sp, welt, pos, besch->get_wtyp());
	if(gr) {
		// get the sign direction
		weg_t *weg = gr->get_weg( besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
		signal_t *s = gr->find<signal_t>();
		if(s  &&  s->get_besch()!=besch) {
			// only one sign per tile
			return error;
		}
		if(besch->is_signal()  &&  gr->find<roadsign_t>())  {
			// only one sign per tile
			return error;
		}

		ribi_t::ribi dir = weg->get_ribi_unmasked();

		// no signals on switches
		if(  ribi_t::is_threeway(dir)  &&  besch->is_signal_type()  ) {
			return error;
		}

		const bool two_way = besch->is_single_way()  ||  besch->is_signal() ||  besch->is_pre_signal();

		if(!(besch->is_traffic_light() || two_way)  ||  (two_way  &&  ribi_t::is_twoway(dir))  ||  (besch->is_traffic_light()  &&  ribi_t::is_threeway(dir))) {
			roadsign_t* rs;
			if (besch->is_signal_type()) {
				// if there is already a signal, we might need to inverse the direction
				rs = gr->find<signal_t>();
				if (rs) {
					if(  !spieler_t::check_owner( rs->get_besitzer(), sp )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
				}
			} else {
				// if there is already a sign, we might need to inverse the direction
				rs = gr->find<roadsign_t>();
				if (rs) {
					if(  !spieler_t::check_owner( rs->get_besitzer(), sp )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
				}
			}
			error = NULL;
		}
	}
	return error;
}

char wkz_roadsign_t::toolstring[256];

const char* wkz_roadsign_t::get_default_param(spieler_t *sp) const
{
	if (besch  &&  sp) {
		sprintf(toolstring, "%s,%d,%d,%d", besch->get_name(), signal_spacing[sp->get_player_nr()], remove_intermediate_signals[sp->get_player_nr()], replace_other_signals[sp->get_player_nr()]);
		return toolstring;
	}
	else {
		return default_param;
	}
}

// read variables from default_param if cmd comes from network
// default_param: sign_name,signal_spacing,remove,replace
// if the static variable toolstring is the default_param then reset default_param to name of signal
void wkz_roadsign_t::read_default_param(spieler_t * sp)
{
	char name[256]="";
	uint32 i;
	for(i=0; default_param[i]!=0  &&  default_param[i]!=','; i++) {
		name[i]=default_param[i];
	}
	name[i]=0;
	besch = roadsign_t::find_besch(name);

	if (default_param[i]) {
		int i_signal_spacing = signal_spacing[sp->get_player_nr()];
		int i_remove_intermediate_signals = remove_intermediate_signals[sp->get_player_nr()];
		int i_replace_other_signals = replace_other_signals[sp->get_player_nr()];
		sscanf(default_param+i, ",%d,%d,%d", &i_signal_spacing, &i_remove_intermediate_signals, &i_replace_other_signals);
		signal_spacing[sp->get_player_nr()] = (uint8)i_signal_spacing;
		remove_intermediate_signals[sp->get_player_nr()] = i_remove_intermediate_signals!=0;
		replace_other_signals[sp->get_player_nr()] = i_replace_other_signals!=0;
	}
	if (default_param==toolstring) {
		default_param = besch->get_name();
	}
}

bool wkz_roadsign_t::init( karte_t *welt, spieler_t * sp)
{
	// read data from string
	read_default_param(sp);

	if (is_ctrl_pressed()  &&  is_local_execution()) {
		create_win(new signal_spacing_frame_t(sp, this), w_info, (long)this);
	}
	return two_click_werkzeug_t::init(welt, sp);
}

bool wkz_roadsign_t::exit( karte_t *welt, spieler_t *sp )
{
	destroy_win((long)this);
	return two_click_werkzeug_t::exit(welt,sp);
}

uint8 wkz_roadsign_t::is_valid_pos( karte_t *welt, spieler_t *sp, const koord3d &pos, const char *&error, const koord3d &start)
{
	// first click
	if (start==koord3d::invalid) {
		error = check_pos_intern(welt, sp, pos);
		return (error==NULL ? 3 : 0);
	}
	// second click
	else {
		error = NULL;
		return 2;
	}
}


bool wkz_roadsign_t::calc_route( route_t &verbindung, spieler_t *sp, const koord3d& start, const koord3d& to )
{
	// get a default vehikel
	vehikel_besch_t rs_besch( besch->get_wtyp(), 500, vehikel_besch_t::diesel );
	vehikel_t* test_driver = vehikelbauer_t::baue(start, sp, NULL, &rs_besch);
	bool can_built;
	if( start != to ) {
		can_built = verbindung.calc_route(sp->get_welt(), start, to, test_driver, 0);
		// prevent building of many signals if start and to are adjacent
		// but the step start->to is now allowed
		if (can_built  &&  koord_distance(start, to)==1  &&  verbindung.get_count()>2) {
			grund_t *gr, *grto = sp->get_welt()->lookup(to);
			if (sp->get_welt()->lookup(start)->get_neighbour(gr, besch->get_wtyp(), to.get_2d()-start.get_2d())  &&  gr==grto) {
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

void wkz_roadsign_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &ziel )
{
	route_t route;
	if (!calc_route(route, sp, start, ziel)) {
		return;
	}
	const uint8 signal_density = 2*signal_spacing[sp->get_player_nr()]; // measured in half tiles (straight track count as 2, diagonal as 1, since sqrt(1/2) = 1/2 ;)
	uint8 next_signal = signal_density+1; // to place a sign asap
	sint32 cost = 0;
	// dummy roadsign to get images for preview
	roadsign_t *dummy_rs;
	if (besch->is_signal_type()) {
		dummy_rs = new signal_t(welt, sp, koord3d::invalid, ribi_t::keine, besch);
	}
	else {
		dummy_rs = new roadsign_t(welt, sp, koord3d::invalid, ribi_t::keine, besch);
	}
	bool single_ribi = besch->is_signal_type() || besch->is_single_way() || besch->is_choose_sign();
	for(  uint16 i = 0;  i < route.get_count();  i++  ) {
		grund_t* gr = welt->lookup( route.position_bei(i) );

		weg_t *weg = gr->get_weg(besch->get_wtyp());
		ribi_t::ribi ribi=weg->get_ribi_unmasked(); // set full ribi when signal is on a crossing.
		if(  single_ribi  &&  ribi_t::is_twoway(weg->get_ribi_unmasked()) ) {
			if(i < route.get_count()-1  ) {
				ribi -= ribi_typ(route.position_bei(i), route.position_bei(i+1));
			}
			else if(i>0) {
				ribi -= ribi_typ(route.position_bei(i-1), route.position_bei(i));
			}
		}

		roadsign_t *rs = gr->find<signal_t>();
		if (rs==NULL) {
			rs = gr->find<roadsign_t>();
		}
		// check owner .. other signals...
		next_signal += ribi_t::ist_gerade(ribi)? 2 : 1;
		if(  next_signal >= signal_density  /*&&  !ribi_t::ist_einfach(ribi)*/  ) {
			// can we place signal here?
			if (check_pos_intern(welt, sp, route.position_bei(i))==NULL  ||
				(replace_other_signals[sp->get_player_nr()]  &&  rs != NULL  &&  rs->ist_entfernbar(sp) == NULL) ) {
				zeiger_t* zeiger = new zeiger_t(welt, gr->get_pos(), sp );
				marked[sp->get_player_nr()].append(zeiger);
				zeiger->set_bild( skinverwaltung_t::bauigelsymbol->get_bild_nr(0) );
				gr->obj_add( zeiger );
				zeiger->set_richtung(ribi /* !=0 -> place sign*/);
				next_signal = 0;
				dummy_rs->set_pos(gr->get_pos());
				dummy_rs->set_dir(ribi); // calls calc_bild()
				zeiger->set_after_bild(dummy_rs->get_after_bild());
				zeiger->set_bild(dummy_rs->get_bild());
				dummy_rs->set_dir(ribi_t::keine);
				cost += rs ? (rs->get_besch()==besch ? 0  : besch->get_preis()+rs->get_besch()->get_preis()) : besch->get_preis();
			}
		}
		else if (remove_intermediate_signals[sp->get_player_nr()]  &&  rs  &&  rs->ist_entfernbar(sp)==NULL) {
				zeiger_t* zeiger = new zeiger_t(welt, gr->get_pos(), sp );
				marked[sp->get_player_nr()].append(zeiger);
				zeiger->set_bild( werkzeug_t::general_tool[WKZ_REMOVER]->cursor );
				gr->obj_add( zeiger );
				zeiger->set_richtung(ribi_t::keine /*remove sign*/);
				cost += rs->get_besch()->get_preis();
		}
	}
	delete dummy_rs;
	win_set_static_tooltip( tooltip_with_price("Building costs estimates", cost ) );
}

const char *wkz_roadsign_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end)
{
	// read data from string
	read_default_param(sp);
	// single click ->place signal
	if( end == koord3d::invalid  ||  start == end ) {
		grund_t *gr = welt->lookup(start);
		return place_sign_intern( welt, sp, gr );
	}
	// mark tiles to calculate positions of signals
	mark_tiles(welt, sp, start, end);
	// only search the marked tiles
	for(  slist_tpl<zeiger_t*>::const_iterator i=marked[sp->get_player_nr()].begin(); i!=marked[sp->get_player_nr()].end();  ++i  ) {
		koord3d pos = (*i)->get_pos();
		grund_t *gr = welt->lookup(pos);
		weg_t *weg = gr->get_weg(besch->get_wtyp());
		if( (*i)->get_richtung()) {
			// try to place signal
			const char* error_text =  place_sign_intern( welt, sp, gr );
			if(  error_text  ) {
				if(  replace_other_signals[sp->get_player_nr()]  ) {
					roadsign_t* rs = gr->find<signal_t>();
					if(rs == NULL) rs = gr->find<roadsign_t>();
					if(  rs != NULL  &&  rs->ist_entfernbar(sp) == NULL  ) {
						rs->entferne(sp);
						delete rs;
						error_text =  place_sign_intern( welt, sp, gr );
					}
				}
			}
			if(  error_text  ) {
				return error_text;
			}
			roadsign_t* rs = gr->find<signal_t>();
			if(rs == NULL) rs = gr->find<roadsign_t>();
			assert(rs);
			rs->set_dir( (*i)->get_richtung() );
		}
		else {
			// Place no signal -> remove existing signal
			roadsign_t* rs = gr->find<signal_t>();
			if(rs == NULL) rs = gr->find<roadsign_t>();
			if(  rs != NULL  &&  rs->ist_entfernbar(sp) == NULL  ) {
				rs->entferne(sp);
				delete rs;
			};
		}
		weg->count_sign();
		gr->calc_bild();
	}
	cleanup(sp, true);
	return NULL;
}

/*
 * Called by the GUI (gui/signal_spacing.*)
 */
void wkz_roadsign_t::set_values( spieler_t *sp, uint8 spacing, bool remove, bool replace )
{
	signal_spacing[sp->get_player_nr()] = spacing;
	remove_intermediate_signals[sp->get_player_nr()] = remove;
	replace_other_signals[sp->get_player_nr()] = replace;
}
void wkz_roadsign_t::get_values( spieler_t *sp, uint8 &spacing, bool &remove, bool &replace )
{
	spacing = signal_spacing[sp->get_player_nr()];
	remove = remove_intermediate_signals[sp->get_player_nr()];
	replace = replace_other_signals[sp->get_player_nr()];
}

const char *wkz_roadsign_t::place_sign_intern( karte_t *welt, spieler_t *sp, grund_t* gr, const roadsign_besch_t*)
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
					if(  !spieler_t::check_owner( rs->get_besitzer(), sp )  ) {
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
				} else {
					// add a new signal at position zero!
					rs = new signal_t(welt, sp, gr->get_pos(), dir, besch);
					DBG_MESSAGE("wkz_roadsign()", "new signal, dir is %i", dir);
					goto built_sign;
				}
			} else {
				// if there is already a sign, we might need to inverse the direction
				rs = gr->find<roadsign_t>();
				if (rs) {
					if(  !spieler_t::check_owner( rs->get_besitzer(), sp )  ) {
						return "Das Feld gehoert\neinem anderen Spieler\n";
					}
					// reverse only if single way sign
					if (besch->is_single_way() || besch->is_choose_sign()) {
						dir = ~rs->get_dir() & weg->get_ribi_unmasked();
						rs->set_dir(dir);
						DBG_MESSAGE("wkz_roadsign()", "reverse ribi %i", dir);
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
					DBG_MESSAGE("wkz_roadsign()", "new roadsign, dir is %i", dir);
					rs = new roadsign_t(welt, sp, gr->get_pos(), dir, besch);
built_sign:
					gr->obj_add(rs);
					rs->laden_abschliessen();	// to make them visible
					weg->count_sign();
					spieler_t::accounting(sp, -besch->get_preis(), gr->get_pos().get_2d(), COST_CONSTRUCTION);
				}
			}
			error = NULL;
		}
	}
	return error;
}



// built all types of depots
const char *wkz_depot_t::wkz_depot_aux(karte_t *welt, spieler_t *sp, koord3d pos, const haus_besch_t *besch, waytype_t wegtype, sint64 cost)
{
	if(welt->ist_in_kartengrenzen(pos.get_2d())) {
		grund_t *bd=NULL;
		// special for the seven seas ...
		if(wegtype==water_wt) {
			bd = welt->lookup_kartenboden(pos.get_2d());
			if(!bd->ist_wasser()) {
				bd = NULL;
			}
		}
		if(bd==NULL) {
			bd = wkz_intern_koord_to_weg_grund(sp,welt,pos,wegtype);
		}
		if(!bd  ||  bd->has_two_ways()) {
			return "Cannot built depot here!";
		}

		// no depots on runways!
		if(besch->get_extra()==air_wt  &&  bd->get_weg(air_wt)->get_besch()->get_styp()!=0) {
			return "Cannot built depot here!";
		}

		const char *p=bd->kann_alle_obj_entfernen(sp);
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
			hausbauer_t::neues_gebaeude( welt, sp, bd->get_pos(), layout, besch );
			sp->buche(cost, pos.get_2d(), COST_CONSTRUCTION);
			if(sp == welt->get_active_player()) {
				welt->set_werkzeug( general_tool[WKZ_ABFRAGE], sp );
			}

			struct sound_info info;
			info.index = ok_sound;
			info.volume = 255;
			info.pri = 0;
			sound_play(info);

			return NULL;
		}
		return "Cannot built depot here!";
	}
	return "";
}

bool wkz_depot_t::init( karte_t *welt, spieler_t *sp )
{
	// no depots for player 1
	if(sp!=welt->get_spieler(1)) {
		cursor = hausbauer_t::find_tile(default_param,0)->get_besch()->get_cursor()->get_bild_nr(0);
		return true;
	}
	return false;
}


char const* wkz_depot_t::get_tooltip(spieler_t* const sp)
{
	karte_t&               welt     = *sp->get_welt();
	einstellungen_t const& settings = *welt.get_einstellungen();
	haus_besch_t    const& besch    = *hausbauer_t::find_tile(default_param, 0)->get_besch();
	char            const* tip;
	sint64                 price;
	switch (besch.get_extra()) {
		case road_wt:        tip = "Build road depot";        price = settings.cst_depot_road; break;
		case track_wt:       tip = "Build train depot";       price = settings.cst_depot_rail; break;
		case monorail_wt:    tip = "Build monorail depot";    price = settings.cst_depot_rail; break;
		case maglev_wt:      tip = "Build maglev depot";      price = settings.cst_depot_rail; break;
		case narrowgauge_wt: tip = "Build narrowgauge depot"; price = settings.cst_depot_rail; break;
		case tram_wt:        tip = "Build tram depot";        price = settings.cst_depot_rail; break;
		case water_wt:       tip = "Build ship depot";        price = settings.cst_depot_ship; break;
		case air_wt:         tip = "Build air depot";         price = settings.cst_depot_air;  break;
		default:             return 0;
	}
	return tooltip_with_price_maintenance(&welt, tip, price, settings.maint_building * besch.get_level());
}


const char *wkz_depot_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if(sp==welt->get_spieler(1)) {
		// no depots for player 1
		return false;
	}

	const haus_besch_t *besch = hausbauer_t::find_tile(default_param,0)->get_besch();
	switch(besch->get_extra()) {
		case road_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k, besch, road_wt, welt->get_einstellungen()->cst_depot_road );
		case track_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k, besch, track_wt, welt->get_einstellungen()->cst_depot_rail );
		case monorail_wt:
			{
				// since it needs also a foundation, this is slightly more complex ...
				const char *err = wkz_depot_t::wkz_depot_aux( welt, sp, k, besch, monorail_wt, welt->get_einstellungen()->cst_depot_rail );
				if(err==NULL) {
					grund_t *bd = welt->lookup_kartenboden(k.get_2d());
					if(hausbauer_t::elevated_foundation_besch  &&  k.z-bd->get_pos().z==1  &&  bd->ist_natur()) {
						hausbauer_t::baue( welt, sp, bd->get_pos(), 0, hausbauer_t::elevated_foundation_besch );
					}
				}
				return err;
			}
		case tram_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k, besch, track_wt, welt->get_einstellungen()->cst_depot_rail );
		case water_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k, besch, water_wt, welt->get_einstellungen()->cst_depot_ship );
		case air_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k, besch, air_wt, welt->get_einstellungen()->cst_depot_air );
		case maglev_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k, besch, maglev_wt, welt->get_einstellungen()->cst_depot_rail );
		case narrowgauge_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k, besch, narrowgauge_wt, welt->get_einstellungen()->cst_depot_rail );
		default:
			dbg->fatal("wkz_depot()","called with unknown besch %s",besch->get_name() );
			return "";
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
bool wkz_build_haus_t::init( karte_t *welt, spieler_t * )
{
	if(is_local_execution()  &&  default_param  &&  strlen(default_param)>0) {
		const char *c = default_param+2;
		const haus_tile_besch_t *tile = hausbauer_t::find_tile(c,0);
		if(tile!=NULL) {
			int rotation = (default_param[1]-'0') % tile->get_besch()->get_all_layouts();
			welt->get_zeiger()->set_area( tile->get_besch()->get_groesse(rotation), false );
		}
	}
	return true;
}

const char *wkz_build_haus_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(gr==NULL) {
		return "";
	}

	// Parsing parameter (if there)
	const haus_besch_t *besch = NULL;
	if(default_param  &&  strlen(default_param)>0) {
		const char *c = default_param+2;
		const haus_tile_besch_t *tile = hausbauer_t::find_tile(c,0);
		if(tile) {
			besch = tile->get_besch();
		}
	}
	else {
		besch = hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month(),false,welt->get_climate(pos.z));
	}

	if(besch==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % besch->get_all_layouts() : simrand(besch->get_all_layouts());
	koord size = besch->get_groesse(rotation);

	// process ignore climates switch
	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : besch->get_allowed_climate_bits();

	bool hat_platz = welt->ist_platz_frei( pos.get_2d(), besch->get_b(rotation), besch->get_h(rotation), NULL, cl );
	if(!hat_platz  &&  size.y!=size.x  &&  besch->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
		// try other rotation too ...
		rotation = (rotation+1) % besch->get_all_layouts();
		hat_platz = welt->ist_platz_frei( pos.get_2d(), besch->get_b(rotation), besch->get_h(rotation), NULL, cl );
	}

	// Platz gefunden ...
	if(hat_platz) {
		spieler_t *gb_sp = besch->get_typ()!=gebaeude_t::unbekannt ? NULL : welt->get_spieler(1);
		gebaeude_t *gb = hausbauer_t::baue(welt, gb_sp, gr->get_pos(), rotation, besch);
		if(gb) {
			// building successfull
			if(  besch->get_utyp()!=haus_besch_t::attraction_land  &&  besch->get_utyp()!=haus_besch_t::attraction_city  ) {
				stadt_t *city = welt->suche_naechste_stadt( pos.get_2d() );
				if(city) {
					city->add_gebaeude_to_stadt(gb);
				}
			}
			spieler_t::accounting(sp, welt->get_einstellungen()->cst_multiply_remove_haus * besch->get_level() * size.x * size.y, pos.get_2d(), COST_CONSTRUCTION);
			return NULL;
		}
	}
	return "No suitable ground!";
}



// show industry size in cursor (in known)
bool wkz_build_industries_land_t::init( karte_t *welt, spieler_t * )
{
	if(is_local_execution()  &&  default_param  &&  strlen(default_param)>0) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		welt->get_zeiger()->set_area( fab->get_haus()->get_groesse(rotation), false );
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
const char *wkz_build_industries_land_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if(default_param  &&  strlen(default_param)>0) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1<<welt->get_climate(gr->get_hoehe())), welt->get_timeline_year_month() );
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
		hat_platz = welt->ist_wasser( k.get_2d(), fab->get_haus()->get_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->ist_wasser( k.get_2d(), fab->get_haus()->get_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->ist_platz_frei( k.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->ist_platz_frei( k.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		koord3d k = gr->get_pos();
		int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, rotation, &k, welt->get_spieler(1), 10000 );

		if(anzahl>0) {
			// at least one factory has been built
			welt->change_world_position( k.get_2d(), 0, 0 );
			spieler_t::accounting(sp, anzahl*welt->get_einstellungen()->cst_multiply_found_industry, k.get_2d(), COST_CONSTRUCTION );

			// eventually adjust production
			if(default_param  &&  strlen(default_param)>0) {
				fabrik_t::get_fab(welt,k.get_2d())->set_base_production( atol(default_param+2)>>(welt->ticks_per_world_month_shift-18) );
			}

			// crossconnect all?
			if(welt->get_einstellungen()->is_crossconnect_factories()) {
				const slist_tpl<fabrik_t *> & list = welt->get_fab_list();
				slist_iterator_tpl <fabrik_t *> iter (list);
				while( iter.next() ) {
					iter.get_current()->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return "No suitable ground!";
}


// show industry size in cursor (in known)
bool wkz_build_industries_city_t::init( karte_t *welt, spieler_t * )
{
	if(is_local_execution()  &&  default_param  &&  strlen(default_param)>0) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		welt->get_zeiger()->set_area( fab->get_haus()->get_groesse(rotation), false );
	}
	return true;
}

/* builds a industry chain in the next town
 * defaukt_param see previous function
 */
const char *wkz_build_industries_city_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if(default_param  &&  strlen(default_param)>0) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1<<welt->get_climate(gr->get_hoehe())), welt->get_timeline_year_month() );
	}

	if(fab==NULL) {
		return "";
	}
	int rotation = (default_param  &&  default_param[1]!='#') ? (default_param[1]-'0') % fab->get_haus()->get_all_layouts() : simrand(fab->get_haus()->get_all_layouts()-1);
	koord size = fab->get_haus()->get_groesse(rotation);

// process ignore climates switch (not possible for chains!)
//	climate_bits cl = (default_param  &&  default_param[0]=='1') ? ALL_CLIMATES : fab->get_haus()->get_allowed_climate_bits();

	k = gr->get_pos();
	int anzahl = fabrikbauer_t::baue_hierarchie(NULL, fab, false, &k, welt->get_spieler(1), 10000 );
	if(anzahl>0) {
		// at least one factory has been built
		welt->change_world_position( k.get_2d(), 0, 0 );

		// eventually adjust production
		if(default_param  &&  strlen(default_param)>0) {
			fabrik_t::get_fab(welt,k.get_2d())->set_base_production( atol(default_param+2)>>(welt->ticks_per_world_month_shift-18) );
		}

		// crossconnect all?
		if(welt->get_einstellungen()->is_crossconnect_factories()) {
			const slist_tpl<fabrik_t *> & list = welt->get_fab_list();
			slist_iterator_tpl <fabrik_t *> iter (list);
			while( iter.next() ) {
				iter.get_current()->add_all_suppliers();
			}
		}
		// ain't going to be cheap
		spieler_t::accounting(sp, anzahl*welt->get_einstellungen()->cst_multiply_found_industry, k.get_2d(), COST_CONSTRUCTION );
		return NULL;
	}
	return "No suitable ground!";
}



// show industry size in cursor (must be known!)
bool wkz_build_factory_t::init( karte_t *welt, spieler_t * )
{
	if(is_local_execution()  &&  default_param  &&  strlen(default_param)>0) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		const fabrik_besch_t *fab = fabrikbauer_t::get_fabesch(c);
		if(fab==NULL) {
			// wrong tool!
			return false;
		}
		int rotation = (default_param[1]-'0') % fab->get_haus()->get_all_layouts();
		welt->get_zeiger()->set_area( fab->get_haus()->get_groesse(rotation), false );
		return true;
	}
	return false;
}

/* builds an industry next to the cursor (default_param see above) */
const char *wkz_build_factory_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if(default_param  &&  strlen(default_param)>0) {
		const char *c = default_param+2;
		while(*c  &&  *c++!=',') { /* do nothing */ }
		fab = fabrikbauer_t::get_fabesch(c);
	}
	else {
		fab = fabrikbauer_t::get_random_consumer( false, (climate_bits)(1<<welt->get_climate(gr->get_hoehe())), welt->get_timeline_year_month() );
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
		hat_platz = welt->ist_wasser( k.get_2d(), fab->get_haus()->get_groesse(rotation) );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->ist_wasser( k.get_2d(), fab->get_haus()->get_groesse(rotation) );
		}
	}
	else {
		// and on solid ground
		hat_platz = welt->ist_platz_frei( k.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );

		if(!hat_platz  &&  size.y!=size.x  &&  fab->get_haus()->get_all_layouts()>1  &&  (default_param==NULL  ||  default_param[1]=='#')) {
			// try other rotation too ...
			rotation = (rotation+1) % fab->get_haus()->get_all_layouts();
			hat_platz = welt->ist_platz_frei( k.get_2d(), fab->get_haus()->get_b(rotation), fab->get_haus()->get_h(rotation), NULL, cl );
		}
	}

	if(hat_platz) {
		fabrik_t *f = fabrikbauer_t::baue_fabrik(welt, NULL, fab, rotation, gr->get_pos(), welt->get_spieler(1));
		if(f) {
			// at least one factory has been built
			welt->change_world_position( k.get_2d(), 0, 0 );
			spieler_t::accounting(sp, welt->get_einstellungen()->cst_multiply_found_industry, k.get_2d(), COST_CONSTRUCTION );

			// eventually adjust production
			if(default_param  &&  strlen(default_param)>0) {
				f->set_base_production( max(1,atol(default_param+2)>>(welt->ticks_per_world_month_shift-18)) );
			}

			// crossconnect all?
			if(welt->get_einstellungen()->is_crossconnect_factories()) {
				const slist_tpl<fabrik_t *> & list = welt->get_fab_list();
				slist_iterator_tpl <fabrik_t *> iter (list);
				while( iter.next() ) {
					iter.get_current()->add_all_suppliers();
				}
			}
			return NULL;
		}
	}
	return "No suitable ground!";
}



/**	link tool: links products of factory one with factory two (if possible)
 */
bool wkz_link_factory_t::init( karte_t *, spieler_t * )
{
	last_fab = NULL;
	if(wkz_linkzeiger!=NULL) {
		wkz_linkzeiger->mark_image_dirty( cursor, 0 );
		delete wkz_linkzeiger;
		wkz_linkzeiger = NULL;
	}
	return true;
}

const char *wkz_link_factory_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	fabrik_t *fab = fabrik_t::get_fab( welt, pos.get_2d() );
	if(fab!=NULL  &&  last_fab!=fab) {
		// It's a factory
		if(last_fab==NULL) {
			// first click
			if (is_local_execution()) {
				grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
				wkz_linkzeiger = new zeiger_t(welt, gr->get_pos(), NULL);
				wkz_linkzeiger->set_bild( cursor );
				gr->obj_add(wkz_linkzeiger);
			}
			last_fab = fab;
			return NULL;
		}
		else {
			// second click
			if(fab->add_supplier(last_fab) || last_fab->add_supplier(fab)) {
				//ok! they are connected => remove marker
				init( welt, sp );
				return NULL;
			}
		}
	}
	return "";
}




/* builds company headquarter
 * @author prissi
 */
const haus_besch_t *wkz_headquarter_t::next_level( spieler_t *sp )
{
	return hausbauer_t::get_headquarter(sp->get_headquarter_level(), sp->get_welt()->get_timeline_year_month());
}

const char *wkz_headquarter_t::get_tooltip( spieler_t *sp )
{
	const haus_besch_t* besch = next_level(sp);
	if(besch) {
		return tooltip_with_price_maintenance( sp->get_welt(), sp->get_headquarter_level()==0 ? "build HQ" : "upgrade HQ", sp->get_welt()->get_einstellungen()->cst_multiply_headquarter*besch->get_level()*besch->get_b()*besch->get_h(), sp->get_welt()->get_einstellungen()->maint_building*besch->get_level()*besch->get_groesse().x*besch->get_groesse().y );
	}
	return NULL;
}

bool wkz_headquarter_t::init( karte_t *welt, spieler_t *sp )
{
	// do no use this, if there is no next level to build ...
	const haus_besch_t *besch = next_level(sp);
	if (is_local_execution()  &&  besch) {
		const int rotation = 0;
		welt->get_zeiger()->set_area( besch->get_groesse(rotation), false );
		return true;
	}
	return false;
}


const char *wkz_headquarter_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	bool ok=false;
DBG_MESSAGE("wkz_headquarter()", "building headquarter at (%d,%d)", pos.x, pos.y);

	const haus_besch_t* besch = next_level(sp);
	if(besch==NULL) {
		// no further headquarter level
		dbg->message( "wkz_headquarter()", "Already at maximum level!" );
		return "";
	}

	koord size = besch->get_groesse();
	const sint64 cost = welt->get_einstellungen()->cst_multiply_headquarter * besch->get_level() * size.x * size.y;
	if(  -cost > sp->get_finance_history_month(0,COST_CASH)  ) {
		return "Not enough money!";
	}

	if(welt->ist_in_kartengrenzen(pos.get_2d())) {
		// check for underground ..
		grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
		if (!gr) {
			return "";
		}
		// remove previous one
		koord previous = sp->get_headquarter_pos();
		if(previous!=koord::invalid) {
			grund_t *gr = welt->lookup_kartenboden(previous);
			gebaeude_t *prev_hq = gr->find<gebaeude_t>();
			sp->add_headquarter( prev_hq->get_tile()->get_besch()->get_extra(), koord::invalid );
			hausbauer_t::remove( welt, sp, prev_hq );
			// resize cursor
			init(welt, sp);
		}

		int rotate = 0;

		if(welt->ist_platz_frei(pos.get_2d(), size.x, size.y, NULL, besch->get_allowed_climate_bits())) {
			ok = true;
		}
		if(!ok  &&  besch->get_all_layouts()>1  &&  size.y != size.x  &&  welt->ist_platz_frei(pos.get_2d(), size.y, size.x, NULL, besch->get_allowed_climate_bits())) {
			rotate = 1;
			ok = true;
		}

		if(ok) {
			// then built is
			gebaeude_t *hq = hausbauer_t::baue(welt, sp, welt->lookup_kartenboden(pos.get_2d())->get_pos(), rotate, besch, NULL);
			stadt_t *city = welt->suche_naechste_stadt( pos.get_2d() );
			if(city) {
				city->add_gebaeude_to_stadt( hq );
			}
			// sometimes those are not correct after rotation ...
			sp->add_headquarter( besch->get_extra()+1, hq->get_pos().get_2d()-hq->get_tile()->get_offset() );
			sp->buche( cost, pos.get_2d(), COST_CONSTRUCTION);
			// tell the world of it ...
			cbuffer_t buf(256);
			buf.printf( translator::translate("%s s\nheadquarter now\nat (%i,%i)."), sp->get_name(), pos.x, pos.y );
			welt->get_message()->add_message( buf, pos.get_2d(), message_t::ai, PLAYER_FLAG|sp->get_player_nr(), hq->get_tile()->get_hintergrund(0,0,0) );
			// reset to query tool, since costly relocations should be avoided
			if(is_local_execution()  &&  sp == welt->get_active_player()) {
				welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], sp );
			}
			return NULL;
		}
		else {
			return "No suitable ground!";
		}
	}
	return "";
}

const char *wkz_lock_game_t::work( karte_t *welt, spieler_t *, koord3d )
{
	if(  welt->get_spieler(1)->is_locked()  ||  !welt->get_einstellungen()->get_allow_player_change()  ) {
		return "Only public player can lock games!";
	}
	welt->clear_player_password_hashes();
	if(  !welt->get_spieler(1)->is_locked() ) {
		return "In order to lock the game, you have to protect the public player by password!";
	}
	welt->access_einstellungen()->set_allow_player_change( false );
	destroy_all_win( true );
	welt->switch_active_player( 0, true );
	welt->set_werkzeug( general_tool[WKZ_ABFRAGE], welt->get_spieler(0) );
	return NULL;
}

const char *wkz_add_citycar_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if( stadtauto_t::list_empty() ) {
		// No citycar
		return "";
	}
	grund_t *gr = wkz_intern_koord_to_weg_grund( sp, welt, k, road_wt );

	if(  gr != NULL  &&  ribi_t::is_twoway(gr->get_weg_ribi_unmasked(road_wt))  &&  gr->find<stadtauto_t>() == NULL) {
		// add citycar
		stadtauto_t* vt = new stadtauto_t(welt, gr->get_pos(), koord::invalid);
		gr->obj_add(vt);
		welt->sync_add(vt);
		return NULL;
	}
	return "";
}

uint8 wkz_forest_t::is_valid_pos( karte_t *, spieler_t *, const koord3d &, const char *&, const koord3d & )
{
	// do really nothing ...
	return 2;
}

void wkz_forest_t::mark_tiles( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	koord min, max;
	min.x = start.x < end.x ? start.x : end.x;
	min.y = start.y < end.y ? start.y : end.y;
	max.x = start.x + end.x - min.x;
	max.y = start.y + end.y - min.y;
	koord pos;
	for( pos.x = min.x; pos.x <= max.x; pos.x++ ) {
		for( pos.y = min.y; pos.y <= max.y; pos.y++ ) {
			grund_t *gr = welt->lookup_kartenboden( pos );

			uint8 hang = gr->get_grund_hang() | gr->get_weg_hang();
			uint8 back_hang = (hang&1) + ((hang>>1)&6)+8;

			zeiger_t *marker = new zeiger_t( welt, gr->get_pos(), NULL );
			marker->set_after_bild( grund_besch_t::marker->get_bild( gr->get_grund_hang()&7 ) );
			marker->set_bild( grund_besch_t::marker->get_bild( back_hang ) );
			marker->mark_image_dirty( marker->get_bild(), 0 );
			gr->obj_add( marker );
			marked[sp->get_player_nr()].insert( marker );
		}
	}
}

const char *wkz_forest_t::do_work( karte_t *welt, spieler_t *sp, const koord3d &start, const koord3d &end )
{
	koord wh, nw;
	wh.x = abs(end.x-start.x)+1;
	wh.y = abs(end.y-start.y)+1;
	nw.x = min(start.x, end.x)+(wh.x/2);
	nw.y = min(start.y, end.y)+(wh.y/2);

	sint64 costs = baum_t::create_forest( welt, nw, wh );
	spieler_t::accounting( sp, costs*welt->get_einstellungen()->cst_remove_tree, end.get_2d(), COST_CONSTRUCTION );

	return NULL;
}



bool wkz_stop_moving_t::init( karte_t *, spieler_t * )
{
	last_pos = koord3d::invalid;
	last_halt = halthandle_t();
	waytype[0] = invalid_wt;
	waytype[1] = invalid_wt;
	if(wkz_linkzeiger!=NULL) {
		wkz_linkzeiger->mark_image_dirty( cursor, 0 );
		delete wkz_linkzeiger;
		wkz_linkzeiger = NULL;
	}
	return true;
}


const char *wkz_stop_moving_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	// now we can start
	grund_t *bd = welt->lookup(pos);
	if (bd==NULL) {
		return "";
	}

	// check halts vs waypoints
	halthandle_t h = haltestelle_t::get_halt(welt,pos,sp);
	if(last_pos!=koord3d::invalid  &&  (h.is_bound() ^ last_halt.is_bound())) {
		init(welt,sp);
		return "Can only move from halt to halt or waypoint to waypoint.";
	}
	if(  h.is_bound()  &&  !spieler_t::check_owner( sp, h->get_besitzer() )  ) {
		init(welt,sp);
		return "Das Feld gehoert\neinem anderen Spieler\n";
	}

	// ok, now we have old_stop
	if(  h.is_bound()  &&  !(bd->is_halt()  ||  (h->get_station_type()&haltestelle_t::dock  &&  bd->ist_wasser())  )  ) {
		// not this halt ...
		return "No suitable ground!";
	}
	// check waytypes
	if(  waytype[0] == invalid_wt  &&  (bd->ist_wasser()  ||  bd->hat_wege())  ) {
		// ok;
	}
	else {
		if(  (waytype[0] == water_wt  &&  bd->ist_wasser())  ||  bd->hat_weg(waytype[0])  ||  bd->hat_weg(waytype[1])  ) {
		// ok;
		}
		else
			return "No suitable ground!";
	}


	if(  last_pos == koord3d::invalid  ) {
		// put cursor
		last_pos = bd->get_pos();
		last_halt = h;
		if(bd->ist_wasser()) {
			waytype[0] = water_wt;
		}
		else {
			waytype[0] = bd->get_weg_nr(0)->get_waytype();
			if(bd->get_weg_nr(1)) {
				waytype[1] = bd->get_weg_nr(1)->get_waytype();
			}
		}
		if (is_local_execution()) {
			wkz_linkzeiger = new zeiger_t(welt, last_pos, NULL);
			wkz_linkzeiger->set_bild( cursor );
			bd->obj_add(wkz_linkzeiger);
		}
	}
	else {
		// second click
		pos = bd->get_pos();
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
			for (vector_tpl<convoihandle_t>::const_iterator i = welt->convois_begin(), end = welt->convois_end(); i != end; ++i) {
				convoihandle_t cnv = *i;
				// check line and owner
				if(!cnv->get_line().is_bound()  &&  cnv->get_besitzer()==sp) {
					schedule_t *fpl = cnv->get_schedule();
					// check waytype
					if(fpl  &&  fpl->ist_halt_erlaubt(bd)) {
						bool updated = false;
						for(  int k=0;  k<fpl->get_count();  k++  ) {
							if(  (catch_all_halt  &&  haltestelle_t::get_halt(welt,fpl->eintrag[k].pos,cnv->get_besitzer())==last_halt)  ||  old_platform.is_contained(fpl->eintrag[k].pos)  ) {
								fpl->eintrag[k].pos = pos;
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
			sp->simlinemgmt.get_lines(simline_t::line,&lines);
			for (vector_tpl<linehandle_t>::const_iterator i = lines.begin(), end = lines.end(); i != end; ++i) {
				linehandle_t line = (*i);
				schedule_t *fpl = line->get_schedule();
				// check waytype
				if(fpl->ist_halt_erlaubt(bd)) {
					bool updated = false;
					for(  int k=0;  k<fpl->get_count();  k++  ) {
						// ok!
						if(  (catch_all_halt  &&  haltestelle_t::get_halt(welt,fpl->eintrag[k].pos,line->get_besitzer())==last_halt)  ||  old_platform.is_contained(fpl->eintrag[k].pos)  ) {
							fpl->eintrag[k].pos = pos;
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
						sp->simlinemgmt.update_line(line);
					}
				}
			}
		}
		// since factory connections may have changed
		welt->set_schedule_counter();
		//ok! they are connected => remove marker
		init( welt, sp );
		return NULL;
	}
	return "";

}



const char *wkz_daynight_level_t::get_tooltip(spieler_t *) {
	if(default_param  &&  strlen(default_param)>0) {
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

bool wkz_daynight_level_t::init( karte_t *, spieler_t * ) {
	if(grund_t::underground_mode==grund_t::ugm_all  ||  umgebung_t::night_shift) {
		return false;
	}
	if(default_param  &&  strlen(default_param)>0) {
		if(default_param[0]=='+'  &&  umgebung_t::daynight_level > 0) {
			// '+': fade in one level
			umgebung_t::daynight_level = umgebung_t::daynight_level-1;
		}
		else if (default_param[0]=='-') {
			// '-': fade out one level
			umgebung_t::daynight_level = umgebung_t::daynight_level+1;
		}
		else {
			// number: toggle number/0. 4 or 5 is good for night
			const sint8 level = atoi(default_param);
			umgebung_t::daynight_level = (umgebung_t::daynight_level==0) ? level : 0;
		}
	}
	return false;
}



/* make all tiles of this player a public stop
 * if this player is public, make all connected tiles a public stop */
bool wkz_make_stop_public_t::init( karte_t *, spieler_t * )
{
	win_set_static_tooltip( NULL );
	return true;
}

const char *wkz_make_stop_public_t::get_tooltip(spieler_t *sp) {
	sprintf(toolstr, translator::translate("make stop public (or join with public stop next) costs %i per tile and level"), ((sp->get_welt()->get_einstellungen()->maint_building*60)<<(sp->get_welt()->ticks_per_world_month_shift-18))/100 );
	return toolstr;
}

const char *wkz_make_stop_public_t::move( karte_t *welt, spieler_t *sp, uint16, koord3d p )
{
	win_set_static_tooltip( NULL );
	const planquadrat_t *pl = welt->lookup(p.get_2d());
	if(pl!=NULL) {
		halthandle_t halt = pl->get_halt();
		if(  halt.is_bound()  &&  spieler_t::check_owner(halt->get_besitzer(),sp)  &&  halt->get_besitzer()!=welt->get_spieler(1) ) {
			sint64 costs = halt->calc_maintenance();
			// set only tooltip if it costs (us)
			if(costs>0) {
				win_set_static_tooltip( tooltip_with_price("Building costs estimates", -((costs*60)<<(welt->ticks_per_world_month_shift-18)) ) );
			}
		}
	}
	return NULL;
}

const char *wkz_make_stop_public_t::work( karte_t *welt, spieler_t *sp, koord3d p )
{
	const planquadrat_t *pl = welt->lookup(p.get_2d());
	if(!pl  ||  !pl->get_halt().is_bound()) {
		return "No stop here!";
	}
	else {
		halthandle_t halt = pl->get_halt();
		if(  !(spieler_t::check_owner(halt->get_besitzer(),sp)  ||  halt->get_besitzer()==welt->get_spieler(1))  ) {
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}
		else {
			halt->make_public_and_join(sp);
		}
	}
	return NULL;
}



sint8 wkz_show_underground_t::save_underground_level = -128;

bool wkz_show_underground_t::init( karte_t *welt, spieler_t * )
{
	koord3d zpos = welt->get_zeiger()->get_pos();
	// move zeiger to invalid position -> unmark tiles
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
				if(  grund_t::underground_level>welt->get_grundwasser()-5  ) {
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
				if(  grund_t::underground_level<20  ) {
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
			dbg->error( "wkz_show_underground_t::init()", "Unknown command string \"%s\"", default_param );

	}

	// move zeiger back
	welt->get_zeiger()->change_pos( zpos);

	if (ok) {
		save_underground_level = old_underground_level;

		// renew toolbar
		werkzeug_t::update_toolbars(welt);

		// recalc all images on map
		welt->update_map();
	}
	return needs_click;
}

const char *wkz_show_underground_t::work( karte_t *welt, spieler_t *sp, koord3d pos)
{
	koord3d zpos = welt->get_zeiger()->get_pos();
	// move zeiger to invalid position -> unmark tiles
	welt->get_zeiger()->change_pos( koord3d::invalid);

	save_underground_level = grund_t::underground_level;
	grund_t::set_underground_mode( grund_t::ugm_level, pos.z);

	// move zeiger back
	welt->get_zeiger()->change_pos( zpos);

	// renew toolbar
	werkzeug_t::update_toolbars(welt);

	// recalc all images on map
	welt->update_map();

	if(sp == welt->get_active_player()) {
		welt->set_werkzeug( general_tool[WKZ_ABFRAGE], sp );
	}

	return NULL;
}

const char *wkz_show_underground_t::get_tooltip(spieler_t *)
{
	// default default-param = U for backward compatibility
	if(  default_param == NULL  ) {
		default_param = strdup("U");
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

bool wkz_show_underground_t::is_selected(karte_t *) const
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

void wkz_show_underground_t::draw_after( karte_t *welt, koord pos ) const
{
	if(  icon!=IMG_LEER  &&  is_selected(welt)  ) {
		display_img_blend( icon, pos.x, pos.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, true );
		// additionall show level in sliced mode
		if(  default_param!=NULL  &&  grund_t::underground_mode==grund_t::ugm_level  ) {
			char level_str[16];
			sprintf( level_str, "%i", grund_t::underground_level );
			display_proportional( pos.x+4, pos.y+4, level_str, ALIGN_LEFT, COL_YELLOW, true );
		}
	}
}



/************************* internal tools, only need for networking ***************/

/* Handles all action of convois in depots. Needs a default param:
 * [function],[convoi_id],addition stuff
 * following simple command exists:
 * 'x' : self destruct
 * 'f' : open the schedule window
 * 'g' : apply a schedule
 * 'n' : toggle 'no load'
 * 'w' : toggle withdraw
 * 'd' : dissassemble convoi and store vehicle in this depot
 * 'l' : apply new line [number]
 */
bool wkz_change_convoi_t::init( karte_t *welt, spieler_t *sp )
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
		if (is_local_execution()) {
			create_win( new news_img("Convoy already deleted!"), w_time_delete, magic_none);
		}
#endif
		dbg->error("wkz_change_convoi_t::init", "no convoy with id=%d found", convoi_id);
		return false;
	}

	// first letter is now the actual command
	switch(  tool  ) {
		case 'x': // self destruction ...
			if(cnv.is_bound()) {
				cnv->self_destruct();
			}
			return false;

		case 'f': // open schedule
			{
				// we open the window only when executed on the same client that triggered the tool
				// but the all clients must call the function anyway
				cnv->open_schedule_window( is_local_execution() );
			}
			break;

		case 'g': // change schedule
			{
				schedule_t *fpl = cnv->create_schedule()->copy();
				fpl->eingabe_abschliessen();
				fpl->sscanf_schedule( p );
				cnv->set_schedule( fpl );
			}
			break;

		case 'l': // change line
			{
				// read out id and new aktuell index
				uint16 id=0, aktuell=0;
				int count=sscanf( p, "%hi,%hi", &id, &aktuell );
				linehandle_t l;
				l.set_id( id );
				if(  l.is_bound()  ) {
					if(  count==1 ) {
						// aktuell was not supplied -> take it from line schedule
						aktuell = l->get_schedule()->get_aktuell();
					}
					cnv->set_line( l );
					cnv->get_schedule()->set_aktuell(aktuell);
					cnv->get_schedule()->eingabe_abschliessen();
				}
			}
			break;

		case 'n': // change no_load
			if(  sp!=welt->get_active_player()  &&  !umgebung_t::networkmode  ) {
				// pop up error message here!
				return false;
			}
			cnv->set_no_load( !cnv->get_no_load() );
			if(  !cnv->get_no_load()  ) {
				cnv->set_withdraw( false );
			}
			break;

		case 'w': // change withdraw
			if(  sp!=welt->get_active_player()  &&  !umgebung_t::networkmode  ) {
				// pop up error message here!
				return false;
			}
			cnv->set_withdraw( !cnv->get_withdraw() );
			cnv->set_no_load( cnv->get_withdraw() );
			break;
	}

	return false;	// no related work tool ...
}



/* Handles all action of lines. Needs a default param:
 * [function],[line_id],addition stuff
 * following simple command exists:
 * 'g' : apply new schedule to line [schedule follows]
 */
bool wkz_change_line_t::init( karte_t *, spieler_t *sp )
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
		dbg->error( "wkz_change_line_t::init()", "too short command \"%s\"", default_param );
	}

	line_id = atoi(p);
	while(  *p  &&  *p++!=','  ) {
	}

	linehandle_t line;
	line.set_id( line_id );

	// first letter is now the actual command
	switch(  tool  ) {
		case 'c': // create line, next paraemter line type and magic of schedule window (only right window gets updated)
			{
				line = sp->simlinemgmt.create_line( atoi(p), sp );
				while(  *p  &&  *p++!=','  ) {
				}
				long t;
				sscanf( p, "%ld", &t );
				while(  *p  &&  *p++!=','  ) {
				}
				line->get_schedule()->sscanf_schedule( p );
				if (is_local_execution()) {
					fahrplan_gui_t *fg = dynamic_cast<fahrplan_gui_t *>(win_get_magic(t));
					if(  fg  ) {
						fg->init_line_selector();
					}
					schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic((long)&(sp->simlinemgmt)));
					if(  sl  ) {
						sl->show_lineinfo( line );
					}
					// no schedule window open => then open one
					if(  fg==NULL  ) {
						create_win( new line_management_gui_t(line, sp), w_info, (long)line.get_rep() );
					}
				}
			}
			break;

		case 'd':	// delete line
			{
				if (line.is_bound()  &&  line->count_convoys()==0) {
					// close a schedule window, if still active
					gui_frame_t *w = win_get_magic( (long)line.get_rep() );
					if(w) {
						destroy_win( w );
					}
					sp->simlinemgmt.delete_line(line);
				}
			}
			break;

		case 'g': // change schedule
			{
				if (line.is_bound()) {
					line->get_schedule()->eingabe_abschliessen();
					schedule_t *fpl = line->get_schedule()->copy();
					fpl->sscanf_schedule( p );
					line->set_schedule( fpl );
					line->get_besitzer()->simlinemgmt.update_line(line);
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
 * 'l' : creates a new line (convoi_id might be invalid)
 * 'b' : starts the convoi
 * 'c' : copies this convoi
 * 'd' : dissassembles convoi
 * 's' : sells convoi
 * 'a' : appends a vehicle (+vehikel_name) uses the oldest
 * 'i' : inserts a vehicle in front (+vehikel_name) uses the oldest
 * 's' : sells a vehikel (+vehikel_name) uses the newest
 * 'r' : removes a vehikel (+number in convoi)
 */
bool wkz_change_depot_t::init( karte_t *welt, spieler_t *sp )
{
	char tool=0;
	koord3d pos = koord3d::invalid;
	sint16	z;
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
		dbg->warning("wkz_change_depot_t::init", "no depot found at (%s)", pos.get_str());
		return false;
	}
	if(  !spieler_t::check_owner( depot->get_besitzer(), sp)  ) {
		dbg->warning("wkz_change_depot_t::init", "depot at (%s) belongs to another player", pos.get_str());
		return false;
	}

	convoihandle_t cnv;
	cnv.set_id( convoi_id );

	// ok now do our stuff
	switch(  tool  ) {
		case 'l':
			// create line schedule window
			{
				linehandle_t selected_line = depot->get_besitzer()->simlinemgmt.create_line(depot->get_line_type(),depot->get_besitzer());
				if (is_local_execution()) {
					depot->set_selected_line(selected_line);
					depot_frame_t *depot_frame = dynamic_cast<depot_frame_t *>(win_get_magic( (long)depot ));
					if(  welt->get_active_player()==sp  &&  depot_frame  ) {
						create_win(new line_management_gui_t(selected_line, depot->get_besitzer()), w_info, (long)selected_line.get_rep() );
					}
				}
				DBG_MESSAGE("depot_frame_t::new_line()","id=%d",selected_line.get_id() );
			}
			break;

		case 'b':
			// start a convoi from the depot
			if(  cnv.is_bound()  ) {
				depot->start_convoi(cnv, is_local_execution());
			}
			break;

		case 'd':
		case 'v':
			// disassemble/sell convoi
			depot->disassemble_convoi( cnv, tool=='v' );
			break;

		case 'c':
			// copy this convoi
			if(  cnv.is_bound()  ) {
				depot->copy_convoi(cnv);
			}
			break;

		case 'a':	// append a vehicle
		case 'i':	// insert a vehicle in front
		case 's':	// sells a vehicle
		case 'r': 	// removes a vehicle (assumes a valid depot)
			if(  tool=='r'  ) {
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
					if(cnv->get_vehikel_anzahl()==nr-start_nr) {
						depot->disassemble_convoi(cnv, false);
					}
					else {
						for( int i=start_nr;  i<nr;  i++  ) {
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
						while (info->get_vorgaenger_count() == 1 && info->get_vorgaenger(0) != NULL  &&  !new_vehicle_info.is_contained(info)) {
							info = info->get_vorgaenger(0);
							new_vehicle_info.insert(info);
						}
						info = start_info;
					}
					while(info) {
						new_vehicle_info.append( info );
						if(info->get_nachfolger_count()!=1  ||  (tool=='i'  &&  info==start_info)  ||  new_vehicle_info.is_contained(info->get_nachfolger(0))) {
							break;
						}
						info = info->get_nachfolger(0);
					}
					// now we have a valid composition together
					if(  tool=='s'  ) {
						while(new_vehicle_info.get_count()) {
							// We sell the newest vehicle - gives most money back.
							vehikel_t* veh = depot->find_oldest_newest(new_vehicle_info.remove_first(), false);
							if(veh != NULL) {
								depot->sell_vehicle(veh);
							}
						}
					}
					else {
						// append/insert into convoi; create one if needed
						if(!cnv.is_bound()) {
							if(  convoihandle_t::is_exhausted()  ) {
								if (is_local_execution()) {
									create_win( new news_img("Convoi handles exhausted!"), w_time_delete, magic_none);
								}
								return false;
							}
							// create a new convoi
							cnv = depot->add_convoi();
							cnv->set_name(new_vehicle_info.front()->get_name());
						}

						// now we have a valid cnv
						if(cnv->get_vehikel_anzahl()+new_vehicle_info.get_count() <= depot->get_max_convoi_length()) {

							for(  unsigned i=0;  i<new_vehicle_info.get_count();  i++  ) {
								// insert/append needs reverse order
								unsigned nr = (tool=='i') ? new_vehicle_info.get_count()-i-1 : i;
								// We add the oldest vehicle - newer stay for selling
								const vehikel_besch_t* vb = new_vehicle_info.at(nr);
								vehikel_t* veh = depot->find_oldest_newest(vb, true);
								if (veh == NULL) {
									// nothing there => we buy it
									veh = depot->buy_vehicle(vb);
								}
								depot->append_vehicle(cnv, veh, tool=='i');
							}
						}
					}
				}
			}
			depot->update_win();
			break;
	}
	return false;
}


// sets the password (hash) for a given player
bool wkz_change_password_hash_t::init( karte_t *welt, spieler_t *sp)
{
	if(  default_param==NULL  ) {
		return false;
	}
	pwd_hash_t new_hash;
	const char *ptr = default_param;
	for(  int i=0; i<40;  i++  ) {
		uint8 nibble;
		switch(*ptr) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': nibble = *ptr-'0';
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F': nibble = *ptr-'A'+10;
				break;
			default:
				dbg->error( "wkz_change_password_hash_t::init()", "Password hash too short!" );
				return false;
		}
		ptr ++;
		if(  i&1 ) {
			new_hash[i/2] |= nibble;
		}
		else {
			new_hash[i/2] = (nibble<<4);
		}
	}
	sp->get_password_hash() = new_hash;
	sp->set_unlock( welt->get_player_password_hash(sp->get_player_nr()) );
	return false;
}


/* Handles all player stuff default_param:
 * [function],[player_id],[state]
 * following command exists:
 * 'a' : activate/deactivate player (depends on state)
 * 'n' : create player at id of type state
 * 'f' : activates/deactivates freeplay
 */
bool wkz_change_player_t::init( karte_t *welt, spieler_t *sp)
{
	if(  default_param==NULL  ) {
		dbg->error( "wkz_change_player_t::init()", "nothing to do!" );
		return false;
	}

	char tool=0;
	int id=0;
	int state=0;

	// skip the rest of the command
	const char *p = default_param;
	while(  *p  &&  *p<=' '  ) {
		p++;
	}
	sscanf( p, "%c,%i,%i", &tool, &id, &state );

	// ok now do our stuff
	switch(  tool  ) {
		case 'n': // new player with type state
			if(  state==spieler_t::HUMAN  ||  sp==welt->get_spieler(1)  ||  !welt->get_spieler(1)->is_locked()  ) {
				const char *msg = welt->new_spieler( id, state );
				if(  msg  ) {
					dbg->error( "wkz_change_player_t::init()", msg );
				}
			}
			else {
				dbg->error( "wkz_change_player_t::init()", "Only public player can enable AIs!" );
			}
			break;
		case 'a': // activate/deactivate AI
			if(welt->get_spieler(id)  &&  welt->get_spieler(id)->get_ai_id()!=spieler_t::HUMAN) {
				welt->get_spieler(id)->set_active(state);
				welt->access_einstellungen()->set_player_active( id, welt->get_spieler(id)->is_active() );
			}
			break;
		case 'f': // activate/deactivate freeplay
			if(  welt->get_spieler(1)->is_locked()  ||  !welt->get_einstellungen()->get_allow_player_change()  ) {
				dbg->error( "wkz_change_player_t::init()", "Only public player can enable freeplay!" );
			}
			else {
				welt->access_einstellungen()->set_freeplay( !welt->get_einstellungen()->is_freeplay() );
			}
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
bool wkz_change_traffic_light_t::init( karte_t *welt, spieler_t *sp )
{
	koord3d pos;
	sint16 z, ns, ticks;
	if(  5!=sscanf( default_param, "%hi,%hi,%hi,%hi,%hi", &pos.x, &pos.y, &z, &ns, &ticks )  ) {
		return false;
	}
	pos.z = (sint8)z;
	if(  grund_t *gr = welt->lookup(pos)  ) {
		if( roadsign_t *rs = gr->find<roadsign_t>()  ) {
			if(  rs->get_besch()->is_traffic_light()  &&  spieler_t::check_owner(rs->get_besitzer(),sp)  ) {
				if(  ns  ) {
					rs->set_ticks_ns( ticks );
				}
				else {
					rs->set_ticks_ow( ticks );
				}
				// update the window
				trafficlight_info_t* trafficlight_win = (trafficlight_info_t*)win_get_magic((long)rs);
				if (trafficlight_win) {
					trafficlight_win->update_data();
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
bool wkz_change_city_t::init( karte_t *welt, spieler_t * )
{
	koord pos;
	sint16 allow_growth;
	if(  3!=sscanf( default_param, "g%hi,%hi,%hi", &pos.x, &pos.y, &allow_growth )  ) {
		return false;
	}
	grund_t *gr = welt->lookup_kartenboden(pos);
	if (gr) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if (gb) {
			stadt_t *st = gb->get_stadt();
			if (st) {
				st->set_citygrowth_yesno(allow_growth);
				stadt_info_t *stinfo = dynamic_cast<stadt_info_t*>(win_get_magic((long)st));
				if (stinfo) {
					stinfo->update_data();
				}
			}
		}
	}
	return false;
}



/* Handles all action of lines. Needs a default param:
 * [object='c|h|l|m|t'][id|pos],[name]
 * c=convoi, h=halt, l=line,  m=marker, t=town, p=player
 * in case of marker, id is a pos3d string
 */
bool wkz_rename_t::init(karte_t* const welt, spieler_t *sp)
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
			if(  3!=sscanf( p, "%hi,%hi,%hi", &pos.x, &pos.y, &id )  ) {
				dbg->error( "wkz_rename_t::init", "no position given for marker! (%s)", default_param );
				return false;
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			while(  *p>0  &&  *p++!=','  ) {
			}
			pos.z = id;
			id = 0;
			break;
		default:
			dbg->error( "wkz_rename_t::init", "illegal request! (%s)", default_param );
			return false;
	}

	// now for action ...
	switch(  what  ) {
		case 'h':
		{
			halthandle_t halt;
			halt.set_id( id );
			if(  halt.is_bound()  ) {
				halt->set_name( p );
				return false;
			}
			break;
		}

		case 'l':
		{
			linehandle_t line;
			line.set_id( id );
			if(  line.is_bound()  ) {
				line->set_name( p );

				schedule_list_gui_t *sl = dynamic_cast<schedule_list_gui_t *>(win_get_magic((long)&(sp->simlinemgmt)));
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
			if(  cnv.is_bound()  ) {
				//  set name without ID
				cnv->set_name( p, false );
				return false;
			}
			break;
		}

		case 't':
		{
			if(  id<welt->get_staedte().get_count()  ) {
				welt->get_staedte()[id]->set_name( p );
				return false;
			}
			break;
		}

		case 'm':
			if(  grund_t *gr = welt->lookup(pos)  ) {
				gr->set_text(p);
				return false;
			}
			break;

		case 'p':
			if(  welt->get_spieler(id)  ) {
				welt->get_spieler(id)->set_name(p);
			}
	}
	// we are only getting here, if we could not process this request
	dbg->error( "wkz_rename_t::init", "could not perform (%s)", default_param );
	return false;
}


/* send message to the message queue
 */
bool wkz_add_message_t::init( karte_t *welt, spieler_t *sp )
{
	if(  *default_param  ) {
		if(  sp  ) {
			if(  umgebung_t::add_player_name_to_message  ) {
				cbuffer_t buffer(1024);
				buffer.printf("[%s]\n%s", sp->get_name(), default_param);
				welt->get_message()->add_message( buffer, koord::invalid, message_t::chat, PLAYER_FLAG|sp->get_player_nr(), IMG_LEER );
			}
			else {
				welt->get_message()->add_message( default_param, koord::invalid, message_t::chat, PLAYER_FLAG|sp->get_player_nr(), IMG_LEER );
			}
		}
		else {
			// system message (will not be save on server and will not appear on new clients)
			welt->get_message()->add_message( default_param, koord::invalid, message_t::general | message_t::local_flag, COL_BLACK, IMG_LEER );
		}
	}
	return false;
}
