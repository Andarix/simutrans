/*
 * Werkzeuge f�r den Simutrans-Spieler
 * von Hj. Malthaner
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
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

#include "bauer/fabrikbauer.h"
#include "bauer/vehikelbauer.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/strasse.h"
#include "boden/wege/schiene.h"
#include "boden/wege/kanal.h"
#include "boden/tunnelboden.h"

#include "simdepot.h"
#include "simfab.h"
#include "simwin.h"
#include "simimg.h"
#include "simintr.h"
#include "simhalt.h"

#include "besch/haus_besch.h"
#include "besch/way_obj_besch.h"
#include "besch/skin_besch.h"

#include "vehicle/simvehikel.h"
#include "vehicle/simverkehr.h"
#include "vehicle/simpeople.h"

#include "gui/werkzeug_waehler.h"
#include "gui/station_building_select.h"
#include "gui/karte.h"	// to update map after construction of new industry

#include "dings/zeiger.h"
#include "dings/bruecke.h"
#include "dings/tunnel.h"
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
char *tooltip_with_price_maintenance(karte_t *welt, const char *tip, sint64 price, sint64 maitenance)
{
	int n = sprintf(werkzeug_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(werkzeug_t::toolstr+n, (double)price/-100.0);
	strcat( werkzeug_t::toolstr, " (" );
	n = strlen(werkzeug_t::toolstr);

	money_to_string(werkzeug_t::toolstr+n, (double)(maitenance<<(welt->ticks_bits_per_tag-18))/100.0 );
	strcat( werkzeug_t::toolstr, ")" );
	return werkzeug_t::toolstr;
}



/**
 * Creates a tooltip from tip text and money value
 */
char *tooltip_with_price_maintenance_level(karte_t *welt, const char *tip, sint64 price, sint64 maitenance, uint32 level, uint8 enables)
{
	int n = sprintf(werkzeug_t::toolstr, "%s, ", translator::translate(tip) );
	money_to_string(werkzeug_t::toolstr+n, (double)price/-100.0);
	strcat( werkzeug_t::toolstr, " (" );
	n = strlen(werkzeug_t::toolstr);

	money_to_string(werkzeug_t::toolstr+n, (double)(maitenance<<(welt->ticks_bits_per_tag-18))/100.0 );
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

	// now just search alll neighbours
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
static grund_t *wkz_intern_koord_to_weg_grund(spieler_t *sp, karte_t *welt, koord pos, waytype_t wt)
{
	const planquadrat_t *plan = welt->lookup(pos);

	// check for valid ground
	if(plan==NULL) {
		return NULL;
	}
	if(wt==tram_wt) {
		wt = track_wt;
	}

	const bool backwards=event_get_last_control_shift()==2;

	grund_t *gr=NULL;
	// search all grounds for match
	for( unsigned cnt=0;  cnt<plan->get_boden_count();  cnt++  ) {
		// with control backwards
		const unsigned i = (backwards) ? plan->get_boden_count()-1-cnt : cnt;
		gr = plan->get_boden_bei(i);
		// ignore tunnel
		if(gr->ist_im_tunnel()) {
			gr = NULL;
			continue;
		}
		if(  wt==powerline_wt  &&  gr->get_leitung()  ) {
			// check for ownership
			if(sp!=NULL  &&  !spieler_t::check_owner( sp, gr->get_leitung()->get_besitzer())){
				gr = NULL;
				continue;
			}
			// ok, found
			break;
		}

		// has some rail or monorail?
		if(  !gr->hat_weg(wt)  ) {
			gr = NULL;
			continue;
		}
		// check for ownership
		if(sp!=NULL  &&  !spieler_t::check_owner( sp, gr->get_weg(wt)->get_besitzer())){
			gr = NULL;
			continue;
		}
		// ok, now we have a valid ground
		break;
	}
	return gr;
}



/****************************************** now the actual tools **************************************/

// werkzeuge
const char *wkz_abfrage_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const planquadrat_t *plan = welt->lookup(pos.get_2d());
	if(plan) {
		DBG_MESSAGE("wkz_abfrage()","checking map square %d,%d", pos.x, pos.y);

		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->get_boden_count();  i++  ) {
			grund_t *gr=plan->get_boden_bei( backwards ? plan->get_boden_count()-1-i : i );

			if(gr) {

				int old_count = win_get_open_count();
				for(int n=0; n<gr->get_top(); n++) {
					ding_t *dt = gr->obj_bei(n);
					if(dt  &&  (dt->get_typ()!=ding_t::wayobj  ||  dt->get_typ()!=ding_t::pillar)) {
						DBG_MESSAGE("wkz_abfrage()", "index %d", n);
						dt->zeige_info();
						// did some new window open?
						if(umgebung_t::single_info  &&  old_count!=win_get_open_count()  &&  !gr->ist_wasser()) {
							return NULL;
						}
					}
				}

				if(gr->get_depot()  &&  gr->get_depot()->get_besitzer()==sp) {
					gr->get_depot()->zeige_info();
					return NULL;
				}

				gr->zeige_info();
			}
		}
	}
	return NULL;
}

/* delete things from a tile
 * citycars and pedestrian first and then go up to queue to more important objects
 */
bool wkz_remover_t::wkz_remover_intern(spieler_t *sp, karte_t *welt, koord pos, const char *&msg)
{
DBG_MESSAGE("wkz_remover_intern()","at (%d,%d)", pos.x, pos.y);
	planquadrat_t *plan = welt->access(pos);
	if(!plan) {
		return false;
	}

	grund_t *gr=0;
	const bool backwards = (event_get_last_control_shift()==2);
	// remove lower ground first with CNTRL
	for(unsigned i=0;  i<plan->get_boden_count();  i++  ) {
		gr=plan->get_boden_bei( backwards ? plan->get_boden_count()-1-i : i );
		if(gr->ist_im_tunnel()) {
			// do not remove tunnel ground
			gr = 0;
			continue;
		}
		// ok, something to remove from here ...
		if(gr->get_top()>0  &&  spieler_t::check_owner( sp, gr->obj_bei(0)->get_besitzer()) ) {
			break;
		}
	}
	// everything removed, nothing left ...
	if(gr==NULL) {
		return true;
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
	if (citycar) {
		delete citycar;
		return true;
	}

	// pedestrians?
	fussgaenger_t* fussgaenger = gr->find<fussgaenger_t>();
	if (fussgaenger) {
		delete fussgaenger;
		return true;
	}

	// prissi: Leitung pr�fen (can cross ground of another player)
	leitung_t* lt = gr->get_leitung();
	if(lt!=NULL  &&  lt->get_besitzer()==sp) {
		bool is_leitungsbruecke = false;
		if(gr->ist_bruecke()  &&  gr->ist_karten_boden()) {
			bruecke_t* br = gr->find<bruecke_t>();
			is_leitungsbruecke = br->get_besch()->get_waytype()==powerline_wt;
		}
		if(is_leitungsbruecke) {
			msg = brueckenbauer_t::remove(welt, sp, gr->get_pos(), powerline_wt );
			return msg == NULL;
		}
		else {
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
DBG_MESSAGE("wkz_remover()",  "removing roadsign %d,%d",  pos.x, pos.y);
		weg_t *weg = gr->get_weg(rs->get_besch()->get_wtyp());
		rs->entferne(sp);
		delete rs;
		weg->count_sign();
		return true;
	}

	// Haltestelle pr�fen
	halthandle_t halt = plan->get_halt();
DBG_MESSAGE("wkz_remover()", "bound=%i",halt.is_bound());
	if (gr->is_halt()  &&  halt.is_bound()  &&  fabrik_t::get_fab(welt,pos)==NULL) {
		// halt and not a factory (oil rig etc.)
		const spieler_t* owner = halt->get_besitzer();
		if (owner == sp || owner == welt->get_spieler(1)) {
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
			welt->access(pos)->boden_ersetzen( gr, new boden_t(welt, gr->get_pos(), welt->calc_natural_slope(pos) ) );
			welt->lookup_kartenboden(pos)->calc_bild();
			welt->lookup_kartenboden(pos)->set_flag( grund_t::dirty );
		}
		return msg == NULL;
	}

	// since buildings can have more than one tile, we must handle them together
	gebaeude_t* gb = gr->find<gebaeude_t>();
	if (gb != NULL) {
		const spieler_t* owner = gb->get_besitzer();
		if (owner == sp || owner == NULL  ||  sp==welt->get_spieler(1)) {
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
					stadt_t *stadt = welt->suche_naechste_stadt(pos);
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

	// remove all other stuff (clouds ... )
	bool return_ok = false;
	if(gr->obj_count()>0) {
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

	// could not delete everything
	if(msg) {
		return false;
	}
	if(return_ok) {
		return true;
	}

	// ok, now we remove every object, that should be removed one by one.
	// the following objects will be removed together
DBG_MESSAGE("wkz_remover()", "removing way");

	/*
	* Eigentlich m�ssen wir hier noch verhindern, da� ein Bahnhofsgeb�ude oder eine
	* Bushaltestelle vereinzelt wird!
	* Sonst l�sst sich danach die Richtung der Haltestelle verdrehen und die Bilder
	* gehen kaputt.
	*/
	long cost_sum = 0;
	if(gr->get_typ()!=grund_t::tunnelboden  ||  gr->has_two_ways()) {
		weg_t *w=gr->get_weg_nr(1);
		if(gr->get_typ()==grund_t::brueckenboden  &&  w==NULL) {
			// do not delete the middle of a bridge
			return false;
		}
		if(w==NULL  ||  !spieler_t::check_owner( sp, w->get_besitzer())) {
			w = gr->get_weg_nr(0);
			if(w==NULL) {
				// no way at all ...
				return true;
			}
			if(!spieler_t::check_owner( sp, w->get_besitzer())) {
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
		sp->buche(-cost_sum, pos, COST_CONSTRUCTION);
		if(gr->hat_wege()) {
			return true;
		}
	}
DBG_MESSAGE("wkz_remover()", "check ground");

	if(gr!=plan->get_kartenboden()  &&  gr->get_top()==0) {
DBG_MESSAGE("wkz_remover()", "removing ground");
		// unmark kartenboden (is marked during underground mode deletion)
		plan->get_kartenboden()->clear_flag(grund_t::marked);
		// remove upper or lower ground
		plan->boden_entfernen(gr);
		delete gr;
	}

	return true;
}



const char *wkz_remover_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	DBG_MESSAGE("wkz_remover()","at %d,%d", pos.x, pos.y);
	const char *fail = NULL;
	if(!wkz_remover_intern(sp, welt, pos.get_2d(), fail)) {
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
			drag_height = welt->lookup_hgt(pos.get_2d())+Z_TILE_STEP;
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



const char *wkz_raise_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
//DBG_MESSAGE("wkz_raise()","raising square (%d,%d) to %d",pos.x, pos.y, welt->lookup_hgt(pos)+Z_TILE_STEP);
	bool ok = false;
	koord pos = k.get_2d();

	if(welt->ist_in_gittergrenzen(pos)  &&  pos.x>0  &&  pos.y>0) {
		const int hgt = welt->lookup_hgt(pos);

		if(hgt < 14*Z_TILE_STEP) {

			int n = 0;	// tiles changed
			if(default_param) {
				ok = true;
				// called by dragging or by AI
				sint16 height = atoi(default_param);
				// dragging may be goind up or down!
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
				ok = true;
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
				// update image
				for(int j=-n; j<=n; j++) {
					for(int i=-n; i<=n; i++) {
						const planquadrat_t* p = welt->lookup(pos + koord(i, j));
						if (p)  {
							grund_t* g = p->get_kartenboden();
							if (g) g->calc_bild();
						}
					}
				}
			}
			return !ok ? "Tile not empty." : NULL;
		}
		else {
			// no mountains heigher than 14 ...
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



const char *wkz_lower_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
// DBG_MESSAGE("wkz_lower()","lowering square %d,%d to %d", pos.x, pos.y, welt->lookup_hgt(pos)-Z_TILE_STEP);
	bool ok = false;
	koord pos = k.get_2d();

	if(welt->ist_in_gittergrenzen(pos)  &&  pos.x>0  &&  pos.y>0) {

		const int hgt = welt->lookup_hgt(pos);

		if(hgt > welt->get_grundwasser()) {

			int n = 0;	// tiles changed
			if(default_param) {
				// called by dragging or by AI
				sint16 height = atoi(default_param);
				// dragging may be goind up or down!
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
				ok = welt->lookup_hgt(pos);
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
				// update image
				for(int j=-n; j<=n; j++) {
					for(int i=-n; i<=n; i++) {
						const planquadrat_t* p = welt->lookup(pos + koord(i, j));
						if (p)  {
							grund_t* g = p->get_kartenboden();
							if (g) g->calc_bild();
						}
					}
				}
			}
			return !ok ? "Tile not empty." : NULL;
		}
		else {
			// below water level
			return "";
		}
	}
	return "Zu nah am Kartenrand";
}



/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
const char *wkz_setslope_t::wkz_set_slope_work( karte_t *welt, spieler_t *sp, koord pos, int new_slope )
{
	bool ok = false;

	grund_t * gr1 = welt->lookup_kartenboden(pos);
	if(gr1) {
		// at least a pixel away from the border?
		if(welt->min_hgt(pos)<welt->get_grundwasser()  ) {
			return "Maximum tile height difference reached.";
		}

		// finally: empty
		if (gr1->find<gebaeude_t>() || gr1->hat_wege() || gr1->kann_alle_obj_entfernen(sp)) {
			return "Tile not empty.";
		}

		if(  !welt->ist_in_kartengrenzen(pos+koord(1,1))  ||  !welt->ist_in_kartengrenzen(pos+koord(-1,-1))) {
			return "Zu nah am Kartenrand";
		}

		// slopes may affect the position and the total height!
		koord3d new_pos;
		sint8 slope_this;

		if(new_slope == RESTORE_SLOPE) {
			// prissi: special action: set to natural slope
			new_pos=koord3d(pos,welt->min_hgt(pos));
			slope_this = welt->calc_natural_slope(pos);
			DBG_MESSAGE("natural_slope","%i",slope_this);
		}
		else {
			// now check offsets before changing the slope ...
			sint8 change_to_slope=new_slope;
			if(new_slope==ALL_DOWN_SLOPE  &&  gr1->get_grund_hang()>0) {
				// is more intiutive: if there is a slope, first downgrade it
				change_to_slope = 0;
			}
			slope_this = (change_to_slope>=ALL_UP_SLOPE) ? 0 : change_to_slope;
			new_pos = gr1->get_pos() + koord3d(0,0,(change_to_slope==ALL_UP_SLOPE?Z_TILE_STEP:(change_to_slope==ALL_DOWN_SLOPE?-Z_TILE_STEP:0)));
#ifdef DOUBLE_GROUNDS
			// if already the same, double the slope
			if(slope_this==gr1->get_grund_hang()) {
				slope_this *= 2;
			}
#endif
		}

		// check, if action is valid ...
		const sint16 hgt=new_pos.z/Z_TILE_STEP;
		// maximum difference
		const sint8 test_hgt = hgt+(slope_this!=0);

		// first left side
		const grund_t *grleft=welt->lookup(pos+koord(-1,0))->get_kartenboden();
		if(grleft) {
			const sint16 left_hgt=grleft->get_hoehe()/Z_TILE_STEP;
			const sint8 diff_from_ground = abs(left_hgt-test_hgt);
			if(diff_from_ground>2) {
				return "Maximum tile height difference reached.";
			}
		}

		// right side
		const grund_t *grright=welt->lookup(pos+koord(1,0))->get_kartenboden();
		if(grright) {
			const sint16 right_hgt=grright->get_hoehe()/Z_TILE_STEP;
			const sint8 diff_from_ground = abs(right_hgt-test_hgt);
			if(diff_from_ground>2) {
				return "Maximum tile height difference reached.";
			}
		}

		const grund_t *grback=welt->lookup(pos+koord(0,-1))->get_kartenboden();
		if(grback) {
			const sint16 back_hgt=grback->get_hoehe()/Z_TILE_STEP;
			const sint8 diff_from_ground = abs(back_hgt-test_hgt);
			if(diff_from_ground>2) {
				return "Maximum tile height difference reached.";
			}
		}

		const grund_t *grfront=welt->lookup(pos+koord(0,1))->get_kartenboden();
		if(grfront) {
			const sint16 front_hgt=grfront->get_hoehe()/Z_TILE_STEP;
			const sint8 diff_from_ground = abs(front_hgt-test_hgt);
			if(diff_from_ground>2) {
				return "Maximum tile height difference reached.";
			}
		}

		// ok, now we set the slope ...
		ok = (new_pos!=gr1->get_pos());
		ok |= slope_this!=gr1->get_grund_hang();

		if(ok) {

			if(  gr1->kann_alle_obj_entfernen(sp)  ) {
				// not empty ...
				return "Tile not empty.";
			}

			// already some ground here (tunnel, bridge, monorail?)
			if(new_pos!=gr1->get_pos()  &&  welt->lookup(new_pos)!=NULL) {
				return "Tile not empty.";
			}

			gr1->obj_loesche_alle(sp);

			// ok, was sucess
			if(!gr1->ist_wasser()  &&  slope_this==0  &&  new_pos.z==welt->get_grundwasser()) {
				// now water
				welt->access(pos)->kartenboden_setzen( new wasser_t(welt,new_pos) );
			}
			else if(gr1->ist_wasser()  &&  (new_pos.z>welt->get_grundwasser()  ||  slope_this!=0)) {
				welt->access(pos)->kartenboden_setzen( new boden_t(welt,new_pos,slope_this) );
			}
			else {
				gr1->obj_loesche_alle(sp);
				gr1->set_grund_hang(slope_this);
				gr1->set_pos(new_pos);
				gr1->clear_flag(grund_t::marked);
				gr1->set_flag(grund_t::dirty);
			}
			// recalc slope walls on neightbours
			for(int y=-1; y<=1; y++) {
				for(int x=-1; x<=1; x++) {
					grund_t *gr = welt->lookup(pos+koord(x,y))->get_kartenboden();
					gr->calc_bild();
				}
			}
			spieler_t::accounting(sp, new_slope==RESTORE_SLOPE?welt->get_einstellungen()->cst_alter_land:welt->get_einstellungen()->cst_set_slope, pos, COST_CONSTRUCTION);
		}

	}
	return ok ? NULL : "";
}



// set marker
const char *wkz_marker_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(welt->ist_in_kartengrenzen(pos.get_2d())) {
		grund_t *gr = welt->lookup(pos.get_2d())->get_kartenboden();
		if(gr && !gr->get_text()) {
			const ding_t* thing = gr->obj_bei(0);
			const label_t* l = gr->find<label_t>();

			if(thing == NULL  ||  thing->get_besitzer() == sp  ||  (spieler_t::check_owner(thing->get_besitzer(), sp)  &&  (thing->get_typ() != ding_t::gebaeude))) {
				gr->obj_add(new label_t(welt, gr->get_pos(), sp, "\0"));
				gr->find<label_t>()->zeige_info();
				return "";
			}
		}
	}
	return "Das Feld gehoert\neinem anderen Spieler\n";
}



// show/repair blocks
bool wkz_clear_reservation_t::init( karte_t *welt, spieler_t * )
{
	schiene_t::show_reservations = true;
	welt->set_dirty();
	return true;
}

bool wkz_clear_reservation_t::exit( karte_t *welt, spieler_t * )
{
	schiene_t::show_reservations = false;
	welt->set_dirty();
	return true;
}

const char *wkz_clear_reservation_t::work( karte_t *welt, spieler_t *, koord3d k )
{
	const planquadrat_t *plan = welt->lookup(k.get_2d());
	if(plan) {
		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->get_boden_count();  i++  ) {
			grund_t *gr=plan->get_boden_bei( backwards ? plan->get_boden_count()-1-i : i );

			if(gr) {

				for(unsigned wnr=0;  wnr<2;  wnr++  ) {

					schiene_t *w = dynamic_cast<schiene_t *>(gr->get_weg_nr(wnr));
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
								schiene_t *sch = dynamic_cast<schiene_t *>(iter.access_current());
								if(sch->get_reserved_convoi()==cnv  &&  !gr->suche_obj(cnv->get_vehikel(0)->get_typ())) {
									// force free
									sch->unreserve( cnv->get_vehikel(0) );
								}
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
	sprintf(toolstr, "%s, %ld$ (%ld$)", translator::translate("Build drain"), (long)(sp->get_welt()->get_einstellungen()->cst_transformer/-100l), (long)(sp->get_welt()->get_einstellungen()->cst_maintain_transformer<<(sp->get_welt()->ticks_bits_per_tag-18))/-100l );
	return toolstr;
}

const char *wkz_transformer_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
DBG_MESSAGE("wkz_senke()","called on %d,%d", k.x, k.y);
	grund_t *gr=welt->lookup_kartenboden(k.get_2d());
	if(gr  &&  gr->get_grund_hang()==0  &&  !gr->ist_wasser()  &&  !gr->hat_wege()  &&  gr->kann_alle_obj_entfernen(sp)==NULL) {

		fabrik_t *fab=leitung_t::suche_fab_4(k.get_2d());
		if(fab==NULL) {
			return "Transformer only next to factory!";
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
	}
	return NULL;
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
			hausbauer_t::get_special(0,haus_besch_t::rathaus,0,0,welt->get_climate(gr->get_hoehe()))!=NULL  ) {

			ding_t *d = gr->first_obj();
			gebaeude_t *gb = dynamic_cast<gebaeude_t *>(d);

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
	if(welt->ist_in_kartengrenzen(pos.get_2d())) {
		const baum_besch_t *besch = NULL;
		bool check_climates = true;
		bool random_age = false;
		if(default_param==NULL) {
			besch = baum_t::random_tree_for_climate( welt->get_climate(pos.z) );
		}
		else {
			// parse default_param: bbbesch_nr b=1 ignore climate b=1 randome age
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
 * because we do not like to stop at AIs stop, but we want still force the truck to use AI roads
 * So if there is a halt, then it must be either public or our!
 * @autor prissi
 */
static const char *wkz_fahrplan_insert_aux(karte_t *welt, spieler_t *sp, koord pos, schedule_t *fpl, bool append)
{
	if(fpl == NULL) {
dbg->warning("wkz_fahrplan_insert_aux()","Schedule is (null), doing nothing");
		return false;
	}

	// now we can start
	if(welt->ist_in_kartengrenzen(pos)) {
		bool wrong_owner = false;
		const planquadrat_t *pl = welt->lookup(pos);
		const bool backwards=event_get_last_control_shift()==2;
		const grund_t *bd=0;
		// search all grounds for match
		for(  unsigned cnt=0;  cnt<pl->get_boden_count();  cnt++  ) {
			// with control backwards
			const unsigned i = (backwards) ? pl->get_boden_count()-1-cnt : cnt;
			bd = pl->get_boden_bei(i);
			// ignore tunnel (can be set with Underground mode)
			if(bd->ist_im_tunnel()) {
				bd = 0;
				continue;
			}
			// now just for error messages, we assuming a valid ground
			// and check for ownership
			if(!bd->is_halt()  &&  bd->obj_count()!=0  &&  !spieler_t::check_owner( sp, bd->obj_bei(0)->get_besitzer())) {
				bd = 0;
				continue;
			}
			if(bd->is_halt()  &&  !spieler_t::check_owner( sp, bd->get_halt()->get_besitzer()) ) {
				bd = 0;
				continue;
			}
			// check for rail
			if(!fpl->ist_halt_erlaubt(bd)) {
				bd = 0;
				continue;
			}
			// ok, now we have a valid ground
			break;
		}

		if(bd) {
			// no halt; ownership not checked here, so we checked before!
			if(append) {
				fpl->append(bd);
			}
			else {
				fpl->insert(bd);
			}
		}
		else {
			// here we failed
			if(wrong_owner) {
				return "Das Feld gehoert\neinem anderen Spieler\n";
			}
			else {
				return fpl->fehlermeldung();
			}
		}
	}
	return NULL;
}

const char *wkz_fahrplan_add_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	return wkz_fahrplan_insert_aux( welt, sp, k.get_2d(), (schedule_t *)default_param, true );
}

const char *wkz_fahrplan_ins_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	return wkz_fahrplan_insert_aux( welt, sp, k.get_2d(), (schedule_t *)default_param, false );
}



/* way construction */
const weg_besch_t *wkz_wegebau_t::defaults[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const weg_besch_t * wkz_wegebau_t::get_besch(bool remember)
{
	const weg_besch_t *besch = wegbauer_t::get_besch(default_param,0);
	if(besch==NULL) {
		waytype_t wt = (waytype_t)atoi(default_param);
		besch = defaults[wt&63];
		if(besch==NULL) {
			if(wt<=air_wt) {
				weg_t *w = weg_t::alloc(wt);
				besch = w->get_besch();
				delete w;
			}
			else {
				besch = wegbauer_t::leitung_besch;
			}
		}
	}
	assert(besch);
	if(remember) {
		defaults[besch->get_wtyp()&63] = besch;
	}
	return besch;
}

const char *wkz_wegebau_t::get_tooltip(spieler_t *sp)
{
	const weg_besch_t *besch = get_besch(false);
	sprintf(toolstr, "%s, %ld$ (%.2lf$), %dkm/h",
		translator::translate(besch->get_name()),
		besch->get_preis()/100l,
		(double)(besch->get_wartung()<<(sp->get_welt()->ticks_bits_per_tag-18))/100.0,
		besch->get_topspeed());
	return toolstr;
}

bool wkz_wegebau_t::init( karte_t *welt, spieler_t * )
{
	welt->show_distance = start = koord3d::invalid;
	if(wkz_wegebau_bauer != NULL) {
		wkz_wegebau_bauer->mark_image_dirty( wkz_wegebau_bauer->get_bild(), 0 );
		delete wkz_wegebau_bauer;
		wkz_wegebau_bauer = NULL;
	}
	// delete old route
	while(!marked.empty()) {
		zeiger_t *z = marked.remove_first();
		z->mark_image_dirty( z->get_bild(), 0 );
		delete z;
	}
	// now get current besch
	besch = get_besch(true);
	if(besch  &&  besch->get_cursor()->get_bild_nr(0) != IMG_LEER) {
		cursor = besch->get_cursor()->get_bild_nr(0);
	}
	win_set_static_tooltip( NULL );
	return besch!=NULL;
}

const char *wkz_wegebau_t::move(karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	// on map?
	const planquadrat_t *plan = welt->lookup(pos.get_2d());
	if(plan == NULL) {
		return "";
	}

	// ignore start==pos
	if(start==pos  &&  buttonstate==0) {
		init(welt,sp);
		return "";
	}

	// recalc type of construction
	wegbauer_t::bautyp_t bautyp = (wegbauer_t::bautyp_t)besch->get_wtyp();
	if(besch->get_wtyp()==track_wt  &&  besch->get_styp()==7) {
		bautyp = wegbauer_t::schiene_tram;
	}
	// elevated track?
	if(besch->get_styp()==1  &&  besch->get_wtyp()!=air_wt) {
		bautyp = (wegbauer_t::bautyp_t)((int)bautyp|(int)wegbauer_t::elevated_flag);
	}

	win_set_static_tooltip( NULL );
	if(buttonstate==1) {
		// delete old route
		while(!marked.empty()) {
			zeiger_t *z = marked.remove_first();
			z->mark_image_dirty( z->get_bild(), 0 );
			delete z;
		}
		// check for suitable ground
		grund_t *gr=NULL;
		if(grund_t::underground_mode) {
			// search all grounds for match
			for( unsigned cnt=0;  cnt<plan->get_boden_count();  cnt++  ) {
				// with control backwards
				gr = plan->get_boden_bei(cnt);
				// ignore tunnel
				if(gr->get_typ()!=grund_t::tunnelboden) {
					gr = NULL;
					continue;
				}
				// check for ownership
				if(sp!=NULL  &&  (gr->obj_count()==0  ||  !spieler_t::check_owner( sp, gr->obj_bei(0)->get_besitzer()))){
					gr = NULL;
					continue;
				}
			}
		}
		else {
			// normal ground; just check for ownership
			gr = plan->get_kartenboden();
			if(gr->kann_alle_obj_entfernen(sp)!=NULL  &&  gr->get_weg((waytype_t)besch->get_wtyp())==NULL) {
				gr = NULL;
			}
		}
		// calc new route (if there)
		if(gr) {
			if(start==koord3d::invalid) {
				welt->show_distance = start = gr->get_pos();
				wkz_wegebau_bauer = new zeiger_t(welt, start, sp);
				wkz_wegebau_bauer->set_bild( skinverwaltung_t::bauigelsymbol->get_bild_nr(0) );
				gr->obj_add(wkz_wegebau_bauer);
			}
			else {
				// calculate route
				wegbauer_t bauigel(welt, sp);
				koord3d ziel = gr->get_pos();
				display_show_load_pointer(true);
				bauigel.route_fuer(bautyp, besch);
				if(event_get_last_control_shift()==2  ||  grund_t::underground_mode) {
					bauigel.set_keep_existing_ways(false);
					bauigel.calc_straight_route(start,ziel);
				}
				else {
					bauigel.set_keep_existing_faster_ways(true);
					bauigel.calc_route(start,ziel);
				}
				if(bauigel.max_n>0) {
					// make dummy route from bauigel
					for( int j=0;  j<=bauigel.max_n;  j++  ) {
						koord3d pos = bauigel.get_route_bei(j);
						grund_t *gr = welt->lookup(pos);
						ribi_t::ribi zeige = gr->get_weg_ribi_unmasked(besch->get_wtyp());
						if(j>0) {
							zeige |= ribi_typ( bauigel.get_route_bei(j-1).get_2d()-pos.get_2d() );
						}
						if(j<bauigel.max_n) {
							zeige |= ribi_typ( bauigel.get_route_bei(j+1).get_2d()-pos.get_2d() );
						}
						zeiger_t *way = new zeiger_t( welt, pos, sp );
						if(gr->get_weg_hang()) {
							way->set_bild( besch->get_hang_bild_nr(gr->get_weg_hang(),0) );
						}
						else if(besch->get_wtyp()!=powerline_wt  &&  ribi_t::ist_kurve(zeige)  &&  besch->has_diagonal_bild()) {
							way->set_bild( besch->get_diagonal_bild_nr(zeige,0) );
						}
						else {
							way->set_bild( besch->get_bild_nr(zeige,0) );
						}
						welt->lookup(pos)->obj_add( way );
						marked.insert( way );
						way->mark_image_dirty( way->get_bild(), 0 );
					}
					win_set_static_tooltip( tooltip_with_price("Building costs estimates", -bauigel.calc_costs() ) );
				}
				display_show_load_pointer(false);
			}
		}
	}
	return NULL;
}

const char *wkz_wegebau_t::work(karte_t *welt, spieler_t *sp, koord3d pos )
{
	const planquadrat_t *plan = welt->lookup(pos.get_2d());
	if(plan == NULL) {
		return false;
	}

	grund_t *gr=NULL;
	if(grund_t::underground_mode) {
		// search all grounds for match
		for( unsigned cnt=0;  cnt<plan->get_boden_count();  cnt++  ) {
			// with control backwards
			gr = plan->get_boden_bei(cnt);
			// ignore tunnel
			if(gr->get_typ()!=grund_t::tunnelboden) {
				gr = NULL;
				continue;
			}
			// check for ownership
			if(sp!=NULL  &&  (gr->obj_count()==0  ||  !spieler_t::check_owner( sp, gr->obj_bei(0)->get_besitzer()))){
				gr = NULL;
				continue;
			}
		}
	}
	else {
		// normal ground; just check for ownership
		gr = plan->get_kartenboden();
		if(gr->kann_alle_obj_entfernen(sp)!=NULL  &&  gr->get_weg((waytype_t)besch->get_wtyp())==NULL) {
			gr = NULL;
		}
	}

	if(gr==NULL) {
		return "";
	}

	if(start==koord3d::invalid) {
		welt->show_distance = start = gr->get_pos();

		DBG_MESSAGE("wkz_wegebau()", "Setting start to %d,%d,%d",start.x, start.y, start.z);

		// symbol f�r strassenanfang setzen
		wkz_wegebau_bauer = new zeiger_t(welt, start, sp);
		wkz_wegebau_bauer->set_bild( skinverwaltung_t::bauigelsymbol->get_bild_nr(0) );
		gr->obj_add(wkz_wegebau_bauer);
	}
	else {
		const koord3d baustart = start;
		wegbauer_t bauigel(welt, sp);
		koord3d ziel = gr->get_pos();
		DBG_MESSAGE("wkz_wegebau()", "Setting end to %d,%d,%d",ziel.x, ziel.y, ziel.z);

		// remove old pointers
		init(welt,sp);
		display_show_load_pointer(true);

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
		if(event_get_last_control_shift()==2  ||  grund_t::underground_mode) {
DBG_MESSAGE("wkz_wegebau()", "try straight route");
			bauigel.set_keep_existing_ways(false);
			bauigel.calc_straight_route(baustart,ziel);
		}
		else {
			bauigel.set_keep_existing_faster_ways(true);
			bauigel.calc_route(baustart,ziel);
		}
		welt->show_distance = start = koord3d::invalid;

		DBG_MESSAGE("wkz_wegebau()", "builder found route with %d sqaures length.", bauigel.max_n);

		long cost = bauigel.calc_costs();
		welt->mute_sound(true);
		bauigel.baue();
		welt->mute_sound(false);
		if(cost>10000) {
			struct sound_info info;
			info.index = SFX_CASH;
			info.volume = 255;
			info.pri = 0;
			sound_play(info);
		}
		display_show_load_pointer(false);
	}
	return NULL;
}



/* bridge construction */
const char *wkz_brueckenbau_t::get_tooltip(spieler_t *sp)
{
	const bruecke_besch_t * besch = brueckenbauer_t::get_besch(default_param);
	int n = sprintf(toolstr, "%s, %d$ (%d$)",
		  translator::translate(besch->get_name()),
		  besch->get_preis()/100,
		  (besch->get_wartung()<<(sp->get_welt()->ticks_bits_per_tag-18))/100);

	if(besch->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
	}
	if(besch->get_max_length()>0) {
		n += sprintf(toolstr+n, ", %dkm", besch->get_max_length());
	}
	return toolstr;
}



/* just call the bruckenbauer */
const char *wkz_brueckenbau_t::work(karte_t *welt, spieler_t *sp, koord3d pos )
{
	return brueckenbauer_t::baue( welt, sp, pos.get_2d(), brueckenbauer_t::get_besch(default_param) );
}



/* more difficult, since this builts also underground ways */
const char *wkz_tunnelbau_t::get_tooltip(spieler_t *sp)
{
	const tunnel_besch_t * besch = tunnelbauer_t::get_besch(default_param);
	int n = sprintf(toolstr, "%s, %d$ (%d$)",
		  translator::translate(besch->get_name()),
		  besch->get_preis()/100,
		  (besch->get_wartung()<<(sp->get_welt()->ticks_bits_per_tag-18))/100);

	if(besch->get_waytype()!=powerline_wt) {
		n += sprintf(toolstr+n, ", %dkm/h", besch->get_topspeed());
	}
	return toolstr;
}

bool wkz_tunnelbau_t::init( karte_t *welt, spieler_t * )
{
	welt->show_distance = start = koord3d::invalid;
	if(wkz_tunnelbau_bauer != NULL) {
		wkz_tunnelbau_bauer->mark_image_dirty( wkz_tunnelbau_bauer->get_bild(), 0 );
		delete wkz_tunnelbau_bauer;
		wkz_tunnelbau_bauer = NULL;
	}
	return true;
}

const char *wkz_tunnelbau_t::work(karte_t *welt, spieler_t *sp, koord3d pos )
{
	if(!welt->ist_in_kartengrenzen(pos.get_2d())) {
		return "";
	}

	if(  !grund_t::underground_mode  ) {
		init(welt,sp);
	}

	const tunnel_besch_t *besch = tunnelbauer_t::get_besch(default_param);
	DBG_MESSAGE("tunnelbauer_t::baue()", "called on %d,%d", pos.x, pos.y);

	// in underground mode, new tunnel can be made ...
	if(grund_t::underground_mode) {
		// search for ground
		// start needs valid tile!
		if(start==koord3d::invalid) {
			const planquadrat_t *plan=welt->lookup(pos.get_2d());
			grund_t *gr=NULL;
			for (uint i = 0; i < plan->get_boden_count(); i++) {
				if(plan->get_boden_bei(i)->get_typ()==grund_t::tunnelboden) {
					if(spieler_t::check_owner( sp, plan->get_boden_bei(i)->obj_bei(0)->get_besitzer())) {
						gr = plan->get_boden_bei(i);
						break;
					}
				}
			}
			if(gr==NULL) {
				return "No suitable ground!";
			}
			welt->show_distance = start = gr->get_pos();
			// move bulldozer to start ...
			wkz_tunnelbau_bauer = new zeiger_t(welt, start, sp);
			wkz_tunnelbau_bauer->set_bild( skinverwaltung_t::bauigelsymbol->get_bild_nr(0));
			gr->obj_add(wkz_tunnelbau_bauer);
			return NULL;
		}
		else {
			// we have a start, now just try to built it ...
			wkz_tunnelbau_bauer->mark_image_dirty( wkz_tunnelbau_bauer->get_bild(), 0 );
			delete wkz_tunnelbau_bauer;
			wkz_tunnelbau_bauer = NULL;

			int bt = besch->get_waytype()|wegbauer_t::tunnel_flag;
			weg_t *w=weg_t::alloc(besch->get_waytype());
			const weg_besch_t *wb=w->get_besch();
			delete w;
			// now try construction
			wegbauer_t bauigel(welt, sp);
			bauigel.route_fuer((wegbauer_t::bautyp_t)bt, wb, besch);
			bauigel.set_keep_existing_ways(false);
			bauigel.calc_straight_route(start,koord3d(pos.get_2d(),start.z));
			welt->mute_sound(true);
			bauigel.baue();
			welt->mute_sound(false);
			welt->show_distance = start = koord3d::invalid;
			return NULL;
		}
	}
	else {
		return tunnelbauer_t::baue( welt, sp, pos.get_2d(), besch );
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

bool wkz_wayremover_t::init( karte_t *welt, spieler_t * )
{
	erster = true;
	welt->show_distance = start = koord3d::invalid;
	if(wkz_wayremover_bauer != NULL) {
		wkz_wayremover_bauer->mark_image_dirty( wkz_wayremover_bauer->get_bild(), 0 );
		delete wkz_wayremover_bauer;
		wkz_wayremover_bauer = NULL;
	}
	return true;
}

class electron_t : fahrer_t {
	bool ist_befahrbar(const grund_t* gr) const { return gr->get_leitung()!=NULL; }
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_leitung()->get_ribi(); }
	virtual waytype_t get_waytype() const { return invalid_wt; }
	virtual int get_kosten(const grund_t *,const uint32) const { return 1; }
	virtual bool ist_ziel(const grund_t *cur,const grund_t *) const { return false; }
};

const char *wkz_wayremover_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	// search for starting ground
	waytype_t wt = (waytype_t)atoi(default_param);
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos.get_2d(),wt);
	if(gr==NULL) {
		DBG_MESSAGE("wkz_wayremover()", "no ground on %i,%i",pos.x, pos.y);
		// wrong ground or not this way here => exit
		return "";
	}
	// do not remove ground from depot
	if(gr->get_depot()) {
		return "No suitable ground!";
	}

	if( erster ) {
		// set start position
		erster = false;
		welt->show_distance = start = gr->get_pos();
		wkz_wayremover_bauer = new zeiger_t(welt, start, sp);
		wkz_wayremover_bauer->set_bild( cursor );
		gr->obj_add(wkz_wayremover_bauer);
DBG_MESSAGE("wkz_wayremover()", "Setting start to %d,%d,%d",start.x, start.y,start.z);
	}
	else {
DBG_MESSAGE("wkz_wayremover()", "Setting end to %d,%d,%d",gr->get_pos().x, gr->get_pos().y,gr->get_pos().z);

		// remove marker
		wkz_wayremover_bauer->mark_image_dirty( wkz_wayremover_bauer->get_bild(), 0 );
		delete wkz_wayremover_bauer;
		wkz_wayremover_bauer = NULL;
		erster = true;

		// get a default vehikel
		fahrer_t* test_driver;
		if(  wt!=powerline_wt  ) {
			vehikel_besch_t remover_besch(wt, 500, vehikel_besch_t::diesel );
			test_driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
		}
		else {
			test_driver = (fahrer_t * )new electron_t();
		}
		route_t verbindung;
		bool can_delete = verbindung.calc_route(welt, start, gr->get_pos(), test_driver, 0);
		delete test_driver;
		welt->show_distance = start = koord3d::invalid;
		DBG_MESSAGE("wkz_wayremover()", "route search returned %d", can_delete);

		if(!can_delete) {
			DBG_MESSAGE("wkz_wayremover()","no route found");
			return "Ways not connected";
		}

DBG_MESSAGE("wkz_wayremover()","route with %d tile found",verbindung.get_max_n());

		// found a route => check if I can delete anything on it
		for(  uint32 i=0;  can_delete  &&  i<=verbindung.get_max_n();  i++  ) {
			grund_t *gr=welt->lookup(verbindung.position_bei(i));
			if(gr) {
				if(  wt!=powerline_wt  ) {
					if(gr->get_weg(wt)==NULL  ||  !spieler_t::check_owner( sp, gr->get_weg(wt)->get_besitzer())) {
						can_delete = false;
					}
					else {
						if(gr->kann_alle_obj_entfernen(sp)!=NULL) {
							// we have to do a fine check
							for( uint i=1;  i<gr->get_top();  i++  ) {
								uint8 type = gr->obj_bei(i)->get_typ();
								if(type>=ding_t::automobil  &&  type!=ding_t::aircraft) {
									can_delete = false;
									break;
								}
							}
						}
					}
				}
				else {
					leitung_t *lt = gr->get_leitung();
					can_delete = lt  &&  spieler_t::check_owner( sp, lt->get_besitzer() );
				}
			}
			else {
				can_delete = false;
			}

		}

		// if successful => delete everything
		for( uint32 i=0;  can_delete  &&  i<=verbindung.get_max_n();  i++  ) {

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
						}
						// do not remove asphalt from a bridge ...
						continue;
					}
				}

				// now the tricky part: delete just part of a way (or everything, if possible)
				// calculated removing directions
				ribi_t::ribi rem = (i>0) ? ribi_typ( verbindung.position_bei(i).get_2d(), verbindung.position_bei(i-1).get_2d() ) : 0;
				ribi_t::ribi rem2 = (i<verbindung.get_max_n()) ? ribi_typ(verbindung.position_bei(i).get_2d(),verbindung.position_bei(i+1).get_2d()) : 0;
				rem = ~(rem|rem2);

				if(  wt!=powerline_wt  ) {
					if(!gr->get_flag(grund_t::is_kartenboden)  &&  (gr->get_typ()==grund_t::tunnelboden  ||  gr->get_typ()==grund_t::monorailboden)  &&  gr->get_weg_nr(0)->get_waytype()==wt) {
						can_delete &= gr->remove_everything_from_way(sp,wt,rem);
						if(can_delete  &&  gr->get_weg(wt)==NULL) {
							if(gr->get_weg_nr(0)!=0) {
								gr->remove_everything_from_way(sp,gr->get_weg_nr(0)->get_waytype(),ribi_t::alle);
							}
							gr->obj_loesche_alle(sp);
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
					if(  (rem&lt->get_ribi())==0  ) {
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
	return NULL;
}



/* add catenary during construction */
const way_obj_besch_t *wkz_wayobj_t::default_electric = NULL;

const char *wkz_wayobj_t::get_tooltip(spieler_t *sp)
{
	const way_obj_besch_t *besch = wayobj_t::find_besch(default_param);
	if(besch==NULL) {
		besch = default_electric;
		if(besch==NULL) {
			besch = default_electric = wayobj_t::wayobj_search(track_wt,overheadlines_wt,sp->get_welt()->get_timeline_year_month());
		}
	}
	if(besch) {
		sprintf(toolstr, "%s, %ld$ (%ld$), %dkm/h",
			translator::translate(besch->get_name()),
			besch->get_preis()/100l,
			(besch->get_wartung()<<(sp->get_welt()->ticks_bits_per_tag-18l))/100l,
			besch->get_topspeed());
		return toolstr;
	}
	return NULL;
}

bool wkz_wayobj_t::init( karte_t *welt, spieler_t *sp )
{
	const way_obj_besch_t *besch = default_param ? wayobj_t::find_besch(default_param) : NULL;
	if(besch==NULL) {
		besch = default_electric;
		if(besch==NULL) {
			besch = default_electric = wayobj_t::wayobj_search(track_wt,overheadlines_wt,sp->get_welt()->get_timeline_year_month());
		}
	}
	else {
		default_electric = besch;
	}
	erster = true;
	welt->show_distance = start = koord3d::invalid;
	if(wkz_wayobj_bauer != NULL) {
		wkz_wayobj_bauer->mark_image_dirty( skinverwaltung_t::bauigelsymbol->get_bild_nr(0), 0 );
		delete wkz_wayobj_bauer;
		wkz_wayobj_bauer = NULL;
	}
	return besch!=NULL;
}

const char *wkz_wayobj_t::work( karte_t *welt, spieler_t *sp, koord3d pos )
{
	const way_obj_besch_t *besch = wayobj_t::find_besch(default_param);
	if(besch==NULL) {
		besch = default_electric;
	}
	waytype_t wt=besch->get_wtyp();
	koord3d end;

	// search for starting ground
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,pos.get_2d(),wt);
	if(gr==NULL) {
		DBG_MESSAGE("wkz_wayremover()", "no ground on %i,%i",pos.x, pos.y);
		// wrong ground or not this way here => exit
		return "";
	}

	if( erster ) {
		// set start position
		erster = false;
		welt->show_distance = start = gr->get_pos();
		wkz_wayobj_bauer = new zeiger_t(welt, start, sp);
		wkz_wayobj_bauer->set_bild( besch->get_cursor()->get_bild_nr(0) );
		gr->obj_add(wkz_wayobj_bauer);
		DBG_MESSAGE("wkz_wayremover()", "Setting start to %d,%d,%d",start.x, start.y,start.z);
	}
	else {
		DBG_MESSAGE("wkz_wayremover()", "Setting end to %d,%d,%d",gr->get_pos().x, gr->get_pos().y,gr->get_pos().z);

		// remove marker
		wkz_wayobj_bauer->mark_image_dirty( skinverwaltung_t::bauigelsymbol->get_bild_nr(0), 0 );
		delete wkz_wayobj_bauer ;
		wkz_wayobj_bauer = NULL;
		erster = true;

		// get a default vehikel
		vehikel_besch_t remover_besch(wt, 500, vehikel_besch_t::diesel );
		vehikel_t* test_driver = vehikelbauer_t::baue(start, sp, NULL, &remover_besch);
		route_t verbindung;
		bool can_built = verbindung.calc_route(welt, start, gr->get_pos(), test_driver, 0);
		delete test_driver;
		welt->show_distance = start = koord3d::invalid;
		DBG_MESSAGE("wkz_wayremover()","route search returned %d",can_built);

		if(!can_built) {
			DBG_MESSAGE("wkz_wayremover()","no route found");
			return "Ways not connected";
		}

		// built wayobj ...
		koord3d last_pos, pos=verbindung.position_bei(0);
		uint8 dir = ribi_t::alle;
		for(uint32 i=1;  i<=verbindung.get_max_n();  i++  ) {
			last_pos = pos;
			pos = verbindung.position_bei(i);
			uint8 last_dir = ribi_t::rueckwaerts(dir);
			dir = ribi_typ((pos-last_pos).get_2d());
			koord3d pos=verbindung.position_bei(i);
			wayobj_t::extend_wayobj_t(welt,last_pos,sp,dir|last_dir,besch);
		}
		// last point
		wayobj_t::extend_wayobj_t(welt,pos,sp,ribi_t::rueckwaerts(dir),besch);
		return NULL;
	}
	return "";
}


/* build all kind of station extension buildings */
const char *wkz_station_t::wkz_station_building_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch, sint8 rotation )
{
	koord pos = k.get_2d();
DBG_MESSAGE("wkz_station_building_aux()", "building mail office/station building on square %d,%d", pos.x, pos.y);

	koord size = besch->get_groesse();
	koord offsets;
	halthandle_t halt;


	if(  rotation==-1  ) {
		//no predefined rotation

		int best_halt = 0;
		int any_halt = 0;

		// find valid rotations (since halt extensions are symmetric, we need to check only two)
		bool any_ok = false;
		for( int r=0;  r<2;  r++  ) {
			koord testsize = besch->get_groesse(r);
			for(  sint8 j=3;  j>=0;  j-- ) {
				koord offset(((j&1)^1)*(testsize.x-1),((j>>1)&1)*(testsize.y-1));
				if(welt->ist_platz_frei(pos-offset, testsize.x, testsize.y, NULL, besch->get_allowed_climate_bits())) {
					// well, at least this is theoretical possible here
					any_ok = true;
					koord test_start = pos-offset;
					// find all surrounding tiles with a stop
					int neighbour_halt_n = 0, neighbour_halt_s = 0, neighbour_halt_e = 0, neighbour_halt_w = 0;
					int best_halt_n = 0, best_halt_s = 0, best_halt_e = 0, best_halt_w = 0;
					// test also diagonal corners (that is why from -1 to size!)
					for(  sint16 y=-1;  y<=testsize.y;  y++  ) {
						// left (for all tiles, even bridges)
						const planquadrat_t *pl = welt->lookup( test_start+koord(-1,y) );
						if(  pl  &&  pl->get_halt().is_bound()  &&  sp==pl->get_halt()->get_besitzer()  ) {
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
						if(  pl  &&  pl->get_halt().is_bound()  &&  sp==pl->get_halt()->get_besitzer()  ) {
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
						if(  pl  &&  pl->get_halt().is_bound()  &&  sp==pl->get_halt()->get_besitzer()  ) {
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
						if(  pl  &&  pl->get_halt().is_bound()  &&  sp==pl->get_halt()->get_besitzer()  ) {
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
			return "Tile not empty.";
		}
		// is there no halt to connect?
		if(  !halt.is_bound()  ||  any_halt==0  ) {
			return "Post muss neben\nHaltestelle\nliegen!\n";
		}
	}
	else {
		// rotation was pre-slected; just search for stop now
		assert(  rotation < besch->get_all_layouts()  );
		koord testsize = besch->get_groesse(rotation);
		if(  !welt->ist_platz_frei(pos-offset, testsize.x, testsize.y, NULL, besch->get_allowed_climate_bits())  ) {
			return "Tile not empty.";
		}
		halt = suche_nahe_haltestelle(sp, welt, welt->lookup_kartenboden(pos)->get_pos(), besch->get_b(rotation), besch->get_h(rotation) );
		// is there no halt to connect?
		if(  !halt.is_bound()  ) {
			return "Post muss neben\nHaltestelle\nliegen!\n";
		}
	}

	if(  rotation>besch->get_all_layouts()  ) {
		rotation %= besch->get_all_layouts();
	}

	hausbauer_t::baue(welt, halt->get_besitzer(), k-offsets, rotation, besch, &halt);
	sp->buche(welt->get_einstellungen()->cst_multiply_post*besch->get_level()*besch->get_b()*besch->get_h(), pos, COST_CONSTRUCTION);
	halt->recalc_station_type();

	return NULL;
}

/* build a dock either small or large */
const char *wkz_station_t::wkz_station_dock_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch)
{
	// the cursor cannot be outside the map from here on
	koord pos = k.get_2d();
	grund_t *gr = welt->lookup_kartenboden(pos);
	hang_t::typ hang = gr->get_grund_hang();
	// first get the size
	int len = besch->get_groesse().y-1;
	koord dx = koord((hang_t::typ)hang);
	koord last_pos = pos - dx*len;

	// check, if we can built here ...
	if(!hang_t::ist_einfach(hang)) {
		return "Dock must be built on single slope!";
	}
	else {
		for(int i=0;  i<=len;  i++  ) {
			if(!welt->ist_in_kartengrenzen(pos-dx*i)) {
				// need at least a signle tile to navigate ...
				return "Zu nah am Kartenrand";
			}
			else {
				halthandle_t halt = welt->lookup(pos-dx*i)->get_halt();
				if(halt.is_bound()  &&  !spieler_t::check_owner( sp, halt->get_besitzer())) {
					return "Das Feld gehoert\neinem anderen Spieler\n";
				}
				else {
					const grund_t *gr=welt->lookup(pos-dx*i)->get_kartenboden();
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
	halthandle_t halt = suche_nahe_haltestelle(sp, welt, welt->lookup_kartenboden(pos)->get_pos() );
	bool neu = !halt.is_bound();

	if(neu) { // neues dock
		halt = sp->halt_add(pos);
	}
	hausbauer_t::baue(welt, halt->get_besitzer(), bau_pos, layout, besch, &halt);
	sint64 costs = welt->get_einstellungen()->cst_multiply_dock*besch->get_level();
	if(sp!=halt->get_besitzer()) {
		// public stops are expensive!
		costs -= ((welt->get_einstellungen()->maint_building*besch->get_level()*60)<<(welt->ticks_bits_per_tag-18));
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
		char* name = halt->create_name(pos, "Dock");
		halt->set_name( name );
		free(name);
	}
	return NULL;
}

// built all types of stops but sea harbours
const char *wkz_station_t::wkz_station_aux(karte_t *welt, spieler_t *sp, koord3d k, const haus_besch_t *besch, waytype_t wegtype, sint64 cost, const char *type_name )
{
	koord pos = k.get_2d();
DBG_MESSAGE("wkz_halt_aux()", "building %s on square %d,%d for waytype %x", besch->get_name(), pos.x, pos.y, wegtype);
	const char *p_error=(besch->get_all_layouts()==4) ? "No terminal station here!" : "No through station here!";

	grund_t *bd = wkz_intern_koord_to_weg_grund( sp==welt->get_spieler(1)?NULL:sp,welt,k.get_2d(),wegtype);
	if(!bd  &&  track_wt) {
		bd = wkz_intern_koord_to_weg_grund(sp==welt->get_spieler(1)?NULL:sp,welt,k.get_2d(),monorail_wt);
	}

	if(!bd  ||  bd->get_weg_hang()!=hang_t::flach  ||  bd->is_halt()) {
		// only flat tiles, only one stop per map square
		return p_error;
	}

	if(bd->get_typ()==grund_t::tunnelboden  &&  bd->ist_karten_boden()) {
		// do not build on tunnel entries
		return false;
	}

	if(bd->get_depot()) {
		return "No suitable ground!";
	}

	if(bd->hat_weg(air_wt)  &&  bd->get_weg(air_wt)->get_besch()->get_styp()!=0) {
		return "No suitable ground!";
	}

	// find out orientation ...
	uint32 layout = 0;
	ribi_t::ribi  ribi;
	if(besch->get_all_layouts()==2 || besch->get_all_layouts()==8 || besch->get_all_layouts()==16) {
		// through station
		if(bd->has_two_ways()) {
			// a crossing or maybe just a tram track on a road ...
			ribi = bd->get_weg_nr(0)->get_ribi_unmasked()  |  bd->get_weg_nr(1)->get_ribi_unmasked();
		}
		else {
			ribi = bd->get_weg_ribi_unmasked(wegtype);
		}
		// not straight: sorry cannot built here ...
		if(!ribi_t::ist_gerade(ribi)) {
			return p_error;
		}
		layout = (ribi & ribi_t::nordsued)?0 :1;
	}
	else if(besch->get_all_layouts()==4) {
		// terminal station
		ribi = bd->get_weg_ribi_unmasked(wegtype);
		// sorry cannot built here ... (not a terminal tile)
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

		sint16 offset = bd->get_hoehe()+bd->get_weg_yoff()/TILE_HEIGHT_STEP;

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

	// seems everything ok, lets build
	halthandle_t halt = suche_nahe_haltestelle(sp,welt,bd->get_pos());
	bool neu = !halt.is_bound();

	if(neu) {
		halt = sp->halt_add(pos);
	}
	hausbauer_t::neues_gebaeude( welt, halt->get_besitzer(), bd->get_pos(), layout, besch, &halt);
	halt->recalc_station_type();

	if(neu) {
		char* name = halt->create_name(pos, type_name);
		halt->set_name( name );
		free(name);
	}
	cost *= besch->get_level()*besch->get_b()*besch->get_h();
	if(sp!=halt->get_besitzer()) {
		// public stops are expensive!
		cost += ((welt->get_einstellungen()->maint_building*besch->get_level()*besch->get_b()*besch->get_h()*60)<<(welt->ticks_bits_per_tag-18));
	}
	sp->buche( cost, pos, COST_CONSTRUCTION);
	if(umgebung_t::station_coverage_show  &&  welt->get_zeiger()->get_pos().get_2d()==pos) {
		// since we are larger now ...
		halt->mark_unmark_coverage( true );
	}
	return NULL;
}

// gives the description and sets the rotation value
const haus_besch_t *wkz_station_t::get_besch( sint8 &rotation )
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
	if(  hb->get_utyp()==haus_besch_t::generic_extension  &&  hb->get_all_layouts()>1  ) {
		if(  event_get_last_control_shift()==2  &&  rotation==-1  ) {
			// call station dialoge instead
			destroy_win( magic_station_building_select );
			create_win( new station_building_select_t(welt, hb), w_info, magic_station_building_select);
			// we do not activate building yet; else uncomment the return statement
//			welt->get_zeiger()->set_area( koord( welt->get_einstellungen()->get_station_coverage()*2+1, welt->get_einstellungen()->get_station_coverage()*2+1 ), true );
			return false;
		}
		else if(  rotation>=0  ) {
			// rotation si already fixed
			welt->get_zeiger()->set_area( koord( hb->get_b(rotation), hb->get_h(rotation) ), false );
		}
		else {
			welt->get_zeiger()->set_area( koord( welt->get_einstellungen()->get_station_coverage()*2+1, welt->get_einstellungen()->get_station_coverage()*2+1 ), true );
		}
	}
	else {
		rotation = -1;
		welt->get_zeiger()->set_area( koord( welt->get_einstellungen()->get_station_coverage()*2+1, welt->get_einstellungen()->get_station_coverage()*2+1 ), true );
	}
	return true;
}


image_id wkz_station_t::get_icon( spieler_t * )
{
	if(  grund_t::underground_mode  ) {
		// in underground mode, buildings will be done invisible above ground => disallow such confusion
		sint8 dummy;
		const haus_besch_t *besch=get_besch(dummy);
		if(  besch->get_utyp()!=haus_besch_t::generic_stop  ||  besch->get_extra()==air_wt) {
			return IMG_LEER;
		}
	}
	return icon;
}



const char *wkz_station_t::get_tooltip(spieler_t *sp)
{
	sint8 dummy;
	const haus_besch_t *besch=get_besch(dummy);
	if(  besch->get_utyp()==haus_besch_t::generic_stop  ) {
		switch (besch->get_extra()) {
			case track_wt:
			case monorail_wt:
			case maglev_wt:
			case tram_wt:
			case narrowgauge_wt:
				return tooltip_with_price_maintenance_level( sp->get_welt(), besch->get_name(), sp->get_welt()->get_einstellungen()->cst_multiply_station*besch->get_level(), sp->get_welt()->get_einstellungen()->maint_building*besch->get_level(), besch->get_level(), besch->get_enabled() );
			case road_wt:
				return tooltip_with_price_maintenance_level( sp->get_welt(), besch->get_name(), sp->get_welt()->get_einstellungen()->cst_multiply_roadstop*besch->get_level(), sp->get_welt()->get_einstellungen()->maint_building*besch->get_level(), besch->get_level(), besch->get_enabled() );
			case water_wt:
				return tooltip_with_price_maintenance_level( sp->get_welt(), besch->get_name(), sp->get_welt()->get_einstellungen()->cst_multiply_dock*besch->get_level(), sp->get_welt()->get_einstellungen()->maint_building*besch->get_level(), besch->get_level(), besch->get_enabled() );
			case air_wt:
				return tooltip_with_price_maintenance_level( sp->get_welt(), besch->get_name(), sp->get_welt()->get_einstellungen()->cst_multiply_airterminal*besch->get_level(), sp->get_welt()->get_einstellungen()->maint_building*besch->get_level(), besch->get_level(), besch->get_enabled() );
			case 0:
				return tooltip_with_price_maintenance_level( sp->get_welt(), besch->get_name(), sp->get_welt()->get_einstellungen()->cst_multiply_post*besch->get_level(), sp->get_welt()->get_einstellungen()->maint_building*besch->get_level(), besch->get_level(), besch->get_enabled() );
		}
	}
	else if(  besch->get_utyp()==haus_besch_t::generic_extension  ) {
		return tooltip_with_price_maintenance_level( sp->get_welt(), besch->get_name(), sp->get_welt()->get_einstellungen()->cst_multiply_post*besch->get_level()*besch->get_groesse().x*besch->get_groesse().y, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level()*besch->get_groesse().x*besch->get_groesse().y, besch->get_level()*besch->get_groesse().x*besch->get_groesse().y, besch->get_enabled() );
	}
	else if(  besch->get_utyp()==haus_besch_t::hafen  ) {
		return tooltip_with_price_maintenance_level( sp->get_welt(), besch->get_name(), sp->get_welt()->get_einstellungen()->cst_multiply_dock*besch->get_level()*besch->get_groesse().x*besch->get_groesse().y, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level()*besch->get_groesse().x*besch->get_groesse().y, besch->get_level()*besch->get_groesse().x*besch->get_groesse().y, besch->get_enabled() );
	}
	return "Illegal description";
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
			msg = wkz_station_t::wkz_station_building_aux(welt, sp, pos, besch, rotation );
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

	if(msg==NULL) {
		// no error? => recalc all station connections
		welt->set_schedule_counter();
	}
	return msg;
}


// builds roadsings and signals
const char *wkz_roadsign_t::get_tooltip(spieler_t *)
{
	const roadsign_besch_t * besch = roadsign_t::find_besch(default_param);
	if(besch) {
		return tooltip_with_price( besch->get_name(), besch->get_preis() );
	}
	return NULL;
}

const char *wkz_roadsign_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	DBG_MESSAGE("wkz_roadsign()","called on %d,%d", k.x, k.y);
	const roadsign_besch_t * besch = roadsign_t::find_besch(default_param);
	if(besch==NULL) {
		dbg->fatal("wkz_roadsign_t::work()","No roadsign \"%s\"", default_param );
	}
	const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";
	// search for starting ground
	grund_t *gr=wkz_intern_koord_to_weg_grund(sp,welt,k.get_2d(),besch->get_wtyp());
	if(gr) {
		// get the sign dirction
		weg_t *weg = gr->get_weg(besch->get_wtyp());
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
					if (besch->is_single_way() || besch->is_free_route()) {
						dir = ~rs->get_dir() & weg->get_ribi_unmasked();
						rs->set_dir(dir);
						DBG_MESSAGE("wkz_roadsign()", "reverse ribi %i", dir);
					}
				}
				else {
					// add a new roadsign at position zero!
					// if single way, we need to reduce the allowed ribi to one
					if (besch->is_single_way() || besch->is_free_route()) {
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
					spieler_t::accounting(sp, -besch->get_preis(), k.get_2d(), COST_CONSTRUCTION);
				}
			}
			error = NULL;
		}
	}
	return error;
}



// built all types of depots
const char *wkz_depot_t::wkz_depot_aux(karte_t *welt, spieler_t *sp, koord pos, const haus_besch_t *besch, waytype_t wegtype, sint64 cost)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *bd=NULL;
		// special for the seven seas ...
		if(wegtype==water_wt) {
			bd = welt->lookup_kartenboden(pos);
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
			sp->buche(cost, pos, COST_CONSTRUCTION);
			if(sp == welt->get_active_player()) {
				welt->set_werkzeug( general_tool[WKZ_ABFRAGE] );
			}
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

const char *wkz_depot_t::get_tooltip(spieler_t *sp)
{
	const haus_besch_t *besch = hausbauer_t::find_tile(default_param,0)->get_besch();
	switch(besch->get_extra()) {
		case road_wt: return tooltip_with_price_maintenance( sp->get_welt(), "Build road depot", sp->get_welt()->get_einstellungen()->cst_depot_road, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level() );
		case track_wt: return tooltip_with_price_maintenance( sp->get_welt(), "Build train depot", sp->get_welt()->get_einstellungen()->cst_depot_rail, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level() );
		case monorail_wt: return tooltip_with_price_maintenance( sp->get_welt(), "Build monorail depot", sp->get_welt()->get_einstellungen()->cst_depot_rail, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level() );
		case maglev_wt: return tooltip_with_price_maintenance( sp->get_welt(), "Build maglev depot", sp->get_welt()->get_einstellungen()->cst_depot_rail, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level() );
		case narrowgauge_wt: return tooltip_with_price_maintenance( sp->get_welt(), "Build narrowgauge depot", sp->get_welt()->get_einstellungen()->cst_depot_rail, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level() );
		case tram_wt: return tooltip_with_price_maintenance( sp->get_welt(), "Build tram depot", sp->get_welt()->get_einstellungen()->cst_depot_rail, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level() );
		case water_wt: return tooltip_with_price_maintenance( sp->get_welt(), "Build ship depot", sp->get_welt()->get_einstellungen()->cst_depot_ship, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level() );
		case air_wt: return tooltip_with_price_maintenance( sp->get_welt(), "Build air depot", sp->get_welt()->get_einstellungen()->cst_depot_air, sp->get_welt()->get_einstellungen()->maint_building*besch->get_level() );
	}
	return NULL;
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
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.get_2d(), besch, road_wt, welt->get_einstellungen()->cst_depot_road );
		case track_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.get_2d(), besch, track_wt, welt->get_einstellungen()->cst_depot_rail );
		case monorail_wt:
			{
				// since it need also a foundations, this is slightly more complex ...
				const char *err = wkz_depot_t::wkz_depot_aux( welt, sp, k.get_2d(), besch, monorail_wt, welt->get_einstellungen()->cst_depot_rail );
				if(err==NULL) {
					grund_t *bd = welt->lookup_kartenboden(k.get_2d());
					if(bd->ist_natur()) {
						hausbauer_t::baue( welt, sp, bd->get_pos(), 0, hausbauer_t::elevated_foundation_besch );
					}
				}
				return err;
			}
		case tram_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.get_2d(), besch, track_wt, welt->get_einstellungen()->cst_depot_rail );
		case water_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.get_2d(), besch, water_wt, welt->get_einstellungen()->cst_depot_ship );
		case air_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.get_2d(), besch, air_wt, welt->get_einstellungen()->cst_depot_air );
		case maglev_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.get_2d(), besch, maglev_wt, welt->get_einstellungen()->cst_depot_rail );
		case narrowgauge_wt:
			return wkz_depot_t::wkz_depot_aux( welt, sp, k.get_2d(), besch, narrowgauge_wt, welt->get_einstellungen()->cst_depot_rail );
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
	if(default_param) {
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
	if(default_param) {
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
	if(default_param) {
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

/* builts a (if param=NULL random) industry chain starting here *
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
	if(default_param) {
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
			// least one factory has been built
			welt->change_world_position( k.get_2d(), 0, 0 );
			spieler_t::accounting(sp, anzahl*welt->get_einstellungen()->cst_multiply_found_industry, k.get_2d(), COST_CONSTRUCTION );

			// eventually adjust production
			if(default_param) {
				fabrik_t::get_fab(welt,k.get_2d())->set_base_production( atol(default_param+2)>>(welt->ticks_bits_per_tag-18) );
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
	if(default_param) {
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

/* builts a industry chain in the next town
 * defaukt_param see previous function
 */
const char *wkz_build_industries_city_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if(default_param) {
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
		// least one factory has been built
		welt->change_world_position( k.get_2d(), 0, 0 );

		// eventually adjust production
		if(default_param) {
			fabrik_t::get_fab(welt,k.get_2d())->set_base_production( atol(default_param+2)>>(welt->ticks_bits_per_tag-18) );
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
	if(default_param) {
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

/* builts an industry next to the cursor (default_param see above) */
const char *wkz_build_factory_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	const grund_t* gr = welt->lookup_kartenboden(k.get_2d());
	if(gr==NULL) {
		return "";
	}

	const fabrik_besch_t *fab = NULL;
	if(default_param) {
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
			// least one factory has been built
			welt->change_world_position( k.get_2d(), 0, 0 );
			spieler_t::accounting(sp, welt->get_einstellungen()->cst_multiply_found_industry, k.get_2d(), COST_CONSTRUCTION );

			// eventually adjust production
			if(default_param) {
				f->set_base_production( atol(default_param+2)>>(welt->ticks_bits_per_tag-18) );
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
			grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
			wkz_linkzeiger = new zeiger_t(welt, gr->get_pos(), sp);
			wkz_linkzeiger->set_bild( cursor );
			gr->obj_add(wkz_linkzeiger);
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
	// assume no further headquarter level
	const sint16 level = sp->get_headquarter_level();
	for (vector_tpl<const haus_besch_t*>::const_iterator iter = hausbauer_t::headquarter.begin(), end = hausbauer_t::headquarter.end(); iter != end; ++iter) {
		if ((*iter)->get_extra() == level) {
			return *iter;
		}
	}
	return NULL;
}

const char *wkz_headquarter_t::get_tooltip( spieler_t *sp )
{
	const haus_besch_t* besch = next_level(sp);
	if(besch) {
		return tooltip_with_price_maintenance( sp->get_welt(), sp->get_headquarter_level()==0 ? "build HQ" : "upgrade HQ", sp->get_welt()->get_einstellungen()->cst_multiply_headquarter*besch->get_level()*besch->get_b()*besch->get_h(), sp->get_welt()->get_einstellungen()->maint_building*besch->get_level()*besch->get_groesse().x*besch->get_groesse().y );
	}
	return NULL;
}

bool wkz_headquarter_t::init( karte_t *, spieler_t *sp )
{
	// do no use this, if there is no next level to built ...
	return next_level(sp)!=NULL;
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

	if(welt->ist_in_kartengrenzen(pos.get_2d())) {

		// remove previous one
		koord previous = sp->get_headquarter_pos();
		if(previous!=koord::invalid) {
			grund_t *gr = welt->lookup_kartenboden(previous);
			gebaeude_t *prev_hq = gr->find<gebaeude_t>();
			sp->add_headquarter( prev_hq->get_tile()->get_besch()->get_extra(), koord::invalid );
			hausbauer_t::remove( welt, sp, prev_hq );
		}

		koord size = besch->get_groesse();
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
			sp->add_headquarter( besch->get_extra()+1, pos.get_2d() );
			sp->buche(welt->get_einstellungen()->cst_multiply_headquarter * besch->get_level() * size.x * size.y, pos.get_2d(), COST_CONSTRUCTION);
			if(sp == welt->get_active_player()) {
				welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE] );
			}
			return NULL;
		}
		else {
			return "No suitable ground!";
		}
	}
	return "";
}

const char *wkz_add_citycar_t::work( karte_t *welt, spieler_t *sp, koord3d k )
{
	if( stadtauto_t::list_empty() ) {
		// No citycar
		return "";
	}
	grund_t *gr = wkz_intern_koord_to_weg_grund( sp, welt, k.get_2d(), road_wt );

	if(  gr != NULL  &&  ribi_t::is_twoway(gr->get_weg_ribi_unmasked(road_wt))  &&  gr->find<stadtauto_t>() == NULL) {
		// add citycar
		stadtauto_t* vt = new stadtauto_t(welt, gr->get_pos(), koord::invalid);
		gr->obj_add(vt);
		welt->sync_add(vt);
		return NULL;
	}
	return "";
}


bool wkz_forest_t::init( karte_t *welt, spieler_t * )
{
	welt->show_distance = start = koord3d::invalid;

	if(marked!=NULL) {
		marked->mark_image_dirty( cursor, 0 );
		delete marked;
		marked = NULL;
	}
	return true;
}

const char *wkz_forest_t::move(karte_t *welt, spieler_t *sp, uint16 buttonstate, koord3d pos )
{
	// on map?
	const planquadrat_t *plan = welt->lookup(pos.get_2d());

	if(plan == NULL) {
		return "";
	}
	grund_t* gr = welt->lookup_kartenboden(pos.get_2d());

	if(buttonstate==1) {
		// delete old area
		if(marked!=NULL) {
			welt->mark_area(nw, wh, false);
		}

		if(start==koord3d::invalid) {
			welt->show_distance = start = gr->get_pos();
			marked = new zeiger_t(welt, start, sp);
			marked->set_bild( cursor );
			gr->obj_add(marked);
		}
		else {
			nw = gr->get_pos();

			wh.x = abs(nw.x-start.x)+1;
			wh.y = abs(nw.y-start.y)+1;
			nw.x = min(start.x, nw.x);
			nw.y = min(start.y, nw.y);

			welt->mark_area(nw, wh, true);
		}
	}
	return NULL;
}

const char *wkz_forest_t::work(karte_t *welt, spieler_t *sp, koord3d pos )
{
	grund_t* gr = welt->lookup_kartenboden(pos.get_2d());
	if(!gr) {
		return "";
	}

	if(start==koord3d::invalid) {
		welt->show_distance = start = gr->get_pos();

		marked = new zeiger_t(welt, start, sp);
		marked->set_bild( cursor );
		gr->obj_add(marked);
	}
	else {
		if(marked!=NULL) {
			welt->mark_area(nw, wh, false);
		}

		nw = gr->get_pos();

		wh.x = abs(nw.x-start.x)+1;
		wh.y = abs(nw.y-start.y)+1;
		nw.x = min(start.x, nw.x)+(wh.x/2);
		nw.y = min(start.y, nw.y)+(wh.y/2);

		// remove old pointers
		init(welt,sp);

		sint64 costs = baum_t::create_forest( welt, nw.get_2d(), wh );
		spieler_t::accounting( sp, costs*welt->get_einstellungen()->cst_remove_tree, pos.get_2d(), COST_CONSTRUCTION );

		// then init
		init( welt, sp );
	}
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
	const planquadrat_t *pl = welt->lookup(pos.get_2d());
	if(pl) {
		const bool backwards=event_get_last_control_shift()==2;
		const grund_t *bd=0;
		// search all grounds for match
		halthandle_t h = pl->get_halt();
		if(last_pos!=koord3d::invalid  &&  (h.is_bound() ^ last_halt.is_bound())) {
			init(welt,sp);
			return "Can only move from halt to halt or waypoint to waypoint.";
		}
		if(h.is_bound()  &&  !spieler_t::check_owner( sp, pl->get_haltlist()[0]->get_besitzer())) {
			init(welt,sp);
			return "Das Feld gehoert\neinem anderen Spieler\n";
		}

		for(  unsigned cnt=0;  cnt<pl->get_boden_count();  cnt++  ) {
			// with control backwards
			const unsigned i = (backwards) ? pl->get_boden_count()-1-cnt : cnt;
			bd = pl->get_boden_bei(i);
			// ignore tunnel (can be set with Underground mode)
			if(bd->ist_im_tunnel()) {
				bd = 0;
				continue;
			}
			// must be on a way or in the sea?
			if(!bd->ist_wasser()) {
				weg_t *w1 = bd->get_weg_nr(0);
				if(  w1==NULL  ||  !spieler_t::check_owner( w1->get_besitzer(), sp )  ) {
					// fails, if no way
					bd = 0;
					continue;
				}
				weg_t *w2 = bd->get_weg_nr(1);
				if(  w2  &&  !spieler_t::check_owner( w2->get_besitzer(), sp )  ) {
					// this only fails, if wrong owner
					bd = 0;
					continue;
				}
			}
			// ok, now we have old_stop
			break;
		}
		if(bd==NULL) {
			// here we failed
			return "";
		}

		if(last_pos == koord3d::invalid) {
			// put cursor
			last_pos = bd->get_pos();
			last_halt = bd->ist_wasser() ?  haltestelle_t::get_halt(welt,last_pos) : bd->get_halt();
			if(bd->ist_wasser()) {
				waytype[0] = water_wt;
			}
			else {
				waytype[0] = bd->get_weg_nr(0)->get_waytype();
				if(bd->get_weg_nr(1)) {
					waytype[1] = bd->get_weg_nr(1)->get_waytype();
				}
			}
			grund_t *gr = welt->lookup_kartenboden(last_pos.get_2d());
			wkz_linkzeiger = new zeiger_t(welt, last_pos, sp);
			wkz_linkzeiger->set_bild( cursor );
			gr->obj_add(wkz_linkzeiger);
		}
		else {
			// second click
			pos = bd->get_pos();
			const halthandle_t new_halt = haltestelle_t::get_halt(welt,pos);
			// depending on the waytype we simply built replacements lists
			// in the wort case we have to iterate over all tiles twice ...
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
								if(  (catch_all_halt  &&  haltestelle_t::get_halt(welt,fpl->eintrag[k].pos)==last_halt)  ||  old_platform.is_contained(fpl->eintrag[k].pos)  ) {
									fpl->eintrag[k].pos = pos;
									updated = true;
								}
							}
							if(updated) {
								fpl->cleanup();
								// set this schedule
								cnv->set_schedule(fpl);
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
							if(  (catch_all_halt  &&  haltestelle_t::get_halt(welt,fpl->eintrag[k].pos)==last_halt)  ||  old_platform.is_contained(fpl->eintrag[k].pos)  ) {
								fpl->eintrag[k].pos = pos;
								updated = true;
							}
						}
						// update line
						if(updated) {
							fpl->cleanup();
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
	}
	return "";

}



const char *wkz_daynight_level_t::get_tooltip(spieler_t *) {
	if(default_param) {
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
	if(grund_t::underground_mode  ||  umgebung_t::night_shift) {
		return false;
	}
	if(default_param) {
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
	sprintf(toolstr, translator::translate("make stop public (or join with public stop next) costs %i per tile and level"), ((sp->get_welt()->get_einstellungen()->maint_building*60)<<(sp->get_welt()->ticks_bits_per_tag-18))/100 );
	return toolstr;
}

const char *wkz_make_stop_public_t::move( karte_t *welt, spieler_t *sp, uint16, koord3d p )
{
	win_set_static_tooltip( NULL );
	const planquadrat_t *pl = welt->lookup(p.get_2d());
	if(pl!=NULL) {
		halthandle_t halt = pl->get_halt();
		if(  halt.is_bound()  &&  (spieler_t::check_owner(halt->get_besitzer(),sp)  ||  halt->get_besitzer()==welt->get_spieler(1))  ) {
			sint64 costs = halt->calc_maintenance();
			// set only tooltip if it costs (us)
			if(costs>0) {
				win_set_static_tooltip( tooltip_with_price("Building costs estimates", -((costs*60)<<(welt->ticks_bits_per_tag-18)) ) );
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

