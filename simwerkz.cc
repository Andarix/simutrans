/*
 * simwerkz.cc
 *
 * Werkzeuge f�r den Simutrans-Spieler
 * von Hj. Malthaner
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <string.h>

#include "simdebug.h"
#include "simworld.h"
#include "simplay.h"
#include "simtools.h"
#include "simevent.h"
#include "simskin.h"
#include "simcity.h"
#include "simpeople.h"

#include "bauer/fabrikbauer.h"

#include "boden/boden.h"
#include "boden/grund.h"
#include "boden/wasser.h"
#include "boden/wege/strasse.h"
#include "boden/wege/schiene.h"
#include "boden/wege/kanal.h"
#include "boden/tunnelboden.h"

#include "simvehikel.h"
#include "simverkehr.h"
#include "simdepot.h"
#include "simfab.h"
#include "simwin.h"
#include "simimg.h"
#include "simintr.h"
#include "blockmanager.h"
#include "simhalt.h"

#include "simwerkz.h"

#include "besch/haus_besch.h"
#include "besch/skin_besch.h"

#include "gui/messagebox.h"
#include "gui/label_frame.h"
#include "gui/halt_list_filter_frame.h"
#include "gui/convoi_filter_frame.h"
#include "gui/citylist_frame_t.h"
#include "gui/goods_frame_t.h"
#include "gui/factorylist_frame_t.h"
#include "gui/curiositylist_frame_t.h"

#include "dings/zeiger.h"
#include "dings/bruecke.h"
#include "dings/tunnel.h"
#include "dings/signal.h"
#include "dings/roadsign.h"
#include "dings/leitung2.h"
#include "dings/baum.h"
#ifdef LAGER_NOT_IN_USE
#include "dings/lagerhaus.h"
#endif

#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/fahrplan.h"

#include "bauer/tunnelbauer.h"
#include "bauer/brueckenbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"

#include "besch/weg_besch.h"
#include "besch/roadsign_besch.h"

#include "gui/karte.h"	// to update map after construction of new industry
#include "sucher/bauplatz_sucher.h"

// hilfsfunktionen


/**
 * sucht Haltestelle um Umkreis +1/-1 um (pos, b, h)
 * extended to search first in our direction
 * @author Hj. Malthaner, V.Meyer, prissi
 */
static halthandle_t suche_nahe_haltestelle(spieler_t *sp, karte_t *welt, koord3d pos, int b = 1, int h = 1)
{
	// here below?
	if(sp->is_my_halt(pos.gib_2d())) {
		return welt->lookup(pos.gib_2d())->gib_halt();
	}

	ribi_t::ribi ribi = ribi_t::keine;
	koord next_try_dir[4];  // will be updated each step: biggest distance try first ...
	int iAnzahl = 0;

	grund_t *bd = welt->lookup(pos);
	if(bd==NULL) {
		bd = welt->lookup(pos.gib_2d())->gib_kartenboden();
	}

	// first we try to connect to a stop straight in our direction; otherwise our station may break during construction
	if(bd->gib_weg(weg_t::schiene) != NULL) {
		ribi = bd->gib_weg_ribi_unmasked(weg_t::schiene);
	}
	if(bd->gib_weg(weg_t::strasse) != NULL) {
		ribi = bd->gib_weg_ribi_unmasked(weg_t::strasse);
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

	halthandle_t halt=halthandle_t();

	// first try to connect to our own
	for(  int i=0;  i<iAnzahl;  i++ ) {
		const koord try_pos = pos.gib_2d()+next_try_dir[i];
		if(sp->is_my_halt(try_pos)) {
			return welt->lookup(try_pos)->gib_halt();
		}
	}

	// now just search everything
	koord k=pos.gib_2d();
	for(k.x=pos.x-1; k.x<=pos.x+b; k.x++) {
		for(k.y=pos.y-1; k.y<=pos.y+h; k.y++) {
			if(sp->is_my_halt(k)) {
				return welt->lookup(k)->gib_halt();
			}
		}
	}

	// here we reach only with a non-found station, i.e. a non-bounded handle
	return halt;
}


// werkzeuge

int
wkz_abfrage(spieler_t *, karte_t *welt, koord pos)
{
	DBG_MESSAGE("wkz_abfrage()","checking map square %d,%d", pos.x, pos.y);
	bool ok = false;

	if(welt->ist_in_kartengrenzen(pos)) {

		const planquadrat_t *plan = welt->lookup(pos);

		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			grund_t *gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );

			if(gr) {
				// remember top window
				const gui_fenster_t *old_top = win_get_top();

				for(int n=0; n<gr->gib_top(); n++) {
					ding_t *dt = gr->obj_bei(n);
					if(dt  &&  (dt->gib_typ()!=ding_t::oberleitung  ||  dt->gib_typ()!=ding_t::pillar)) {
						DBG_MESSAGE("wkz_abfrage()", "index %d", n);
						gr->obj_bei(n)->zeige_info();
						// did some new window open?
						if(umgebung_t::single_info  &&  old_top!=win_get_top()  &&  !gr->ist_wasser()) {
							return true;
						}
					}
				}

				if(gr->gib_depot()) {
					gr->gib_depot()->zeige_info();
					return true;
				}

				gr->zeige_info();
				ok = true;
			}
		}
	}
	return ok;
}

int
wkz_raise(spieler_t *sp, karte_t *welt, koord pos)
{
//DBG_MESSAGE("wkz_raise()","raising square (%d,%d) to %d",pos.x, pos.y, welt->lookup_hgt(pos)+16);

	bool ok = false;

	if(welt->ist_in_gittergrenzen(pos)) {
		const int hgt = welt->lookup_hgt(pos);

		if(hgt < 224) {

			int n = welt->raise(pos);
			ok = (n!=0);
			if(!ok) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
				return false;
			}
			else {
				sp->buche(umgebung_t::cst_alter_land*n, pos, COST_CONSTRUCTION);
				// update image
				for(int j=-n; j<=n; j++) {
					for(int i=-n; i<=n; i++) {
						if(welt->ist_in_kartengrenzen(pos.x+i,pos.y+j)  &&  welt->lookup(pos+koord(i,j))->gib_kartenboden()) {
							welt->lookup(pos+koord(i,j))->gib_kartenboden()->calc_bild();
						}
					}
				}
			}
//DBG_MESSAGE("wkz_raise()", "%d squares changed", n);
		} else {
//DBG_MESSAGE("wkz_raise()", "Maximum height reached");
		}
	}
	return ok;
}

int
wkz_lower(spieler_t *sp, karte_t *welt, koord pos)
{
// DBG_MESSAGE("wkz_lower()","lowering square %d,%d to %d", pos.x, pos.y, welt->lookup_hgt(pos)-16);

	bool ok = false;

	if(welt->ist_in_gittergrenzen(pos)) {

		const int hgt = welt->lookup_hgt(pos);

		if(hgt > welt->gib_grundwasser()) {

			int n = welt->lower(pos);
			ok = (n!=0);
			if(!ok) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
			}
			else {
				sp->buche(umgebung_t::cst_alter_land*n, pos, COST_CONSTRUCTION);
//      DBG_MESSAGE("wkz_lower()", "%d squares changed", n);

				// update image
				for(int j=-n; j<=n; j++) {
					for(int i=-n; i<=n; i++) {
						if(welt->ist_in_kartengrenzen(pos.x+i,pos.y+j)  &&  welt->lookup(pos+koord(i,j))->gib_kartenboden()) {
							welt->lookup(pos+koord(i,j))->gib_kartenboden()->calc_bild();
						}
					}
				}
			}

		} else {
//      DBG_MESSAGE("wkz_lower()", "Minimum height reached");
		}
	}

	return ok;
}


/*
 * Vorbedingungen:
 *  - f�r Docks nok
 * welt != NULL
 * sp != NULL
 * halt != NULL
 * pos in Kartengrenzen
 */

static bool
entferne_haltestelle(karte_t *welt, spieler_t *sp, halthandle_t halt, koord3d pos, const char *&msg)
{
DBG_MESSAGE("entferne_haltestelle()","removing segment from %d,%d,%d", pos.x, pos.y, pos.z);
	msg = NULL;
	grund_t *bd = welt->lookup(pos);

	// remove station building?
	gebaeude_t *gb=dynamic_cast<gebaeude_t *>(bd->suche_obj(ding_t::gebaeude));
	if(gb) {
		DBG_MESSAGE("entferne_haltestelle()",  "removing building" );
		const haus_tile_besch_t *tile  = gb->gib_tile();
		koord size = tile->gib_besch()->gib_groesse( tile->gib_layout() );
		const sint32 costs = tile->gib_besch()->gib_level()*umgebung_t::cst_multiply_post;

		// get startpos
		koord k=tile->gib_offset();
		if(k != koord(0,0)) {
			return entferne_haltestelle(welt, sp, halt, pos-koord3d(k,0),msg);
		}
		else {
			for(k.y = 0; k.y < size.y; k.y ++) {
				for(k.x = 0; k.x < size.x; k.x ++) {
					grund_t *gr = welt->lookup(koord3d(k,0)+pos);
					if(welt->max_hgt(k+pos.gib_2d())<=welt->gib_grundwasser()  ||  gr->gib_typ()==grund_t::fundament) {
						msg=gr->kann_alle_obj_entfernen(sp);
						if(msg) {
							return false;
						}
					}
				}
			}
			// remove all gound
DBG_MESSAGE("entferne_haltestelle()", "removing building: cleanup");
			for(k.y = 0; k.y < size.y; k.y ++) {
				for(k.x = 0; k.x < size.x; k.x ++) {
					grund_t *gr = welt->lookup(koord3d(k,0)+pos);
					sp->buche(costs, k, COST_CONSTRUCTION);
					if(gr) {
						halt->rem_grund(gr);
						gebaeude_t *gb=static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
						gr->obj_remove(gb,sp);	// remove building
						delete gb;
						gr->setze_text(NULL);
						if(gr->gib_typ()==grund_t::fundament) {
							welt->access(k+pos.gib_2d())->kartenboden_setzen(new boden_t(welt, koord3d(k+pos.gib_2d(),welt->min_hgt(k+pos.gib_2d()))), false);
						}
						else if(welt->max_hgt(k+pos.gib_2d())<=welt->gib_grundwasser()) {
							welt->access(k+pos.gib_2d())->kartenboden_setzen(new wasser_t(welt, k+pos.gib_2d()), false);
						}
						else {
							weg_t *weg = bd->gib_weg(weg_t::strasse);
							if(weg && static_cast<strasse_t *>(weg)->hat_gehweg()) {
								// Stadtstrassen sollten entfernbar sein, deshalb
								// d�rfen sie keinen Besitzer haben.
								bd->setze_besitzer( NULL );
							}
						}
						sp->buche(costs, pos.gib_2d()+k, COST_CONSTRUCTION);
					}
				}
			}
			bd = NULL;	// no need to do things twice ...
		}
	}
	else {
		halt->rem_grund(bd);
	}

	if(!halt->existiert_in_welt()) {
DBG_DEBUG("entferne_haltestelle()","remove last");
		// all deleted?
		halt->gib_besitzer()->halt_remove( halt );
DBG_DEBUG("entferne_haltestelle()","destroy");
		haltestelle_t::destroy( halt );
	}
	else {
DBG_DEBUG("entferne_haltestelle()","not last");
		// may have been changed ... (due to post office/dock/railways station deletion)
		halt->recalc_station_type();
	}

	// if building was removed this is false!
	if(bd) {
		bd->calc_bild();

DBG_DEBUG("entferne_haltestelle()","reset city way owner");
		weg_t *weg = bd->gib_weg(weg_t::strasse);
		if(weg && static_cast<strasse_t *>(weg)->hat_gehweg()) {
			// Stadtstrassen sollten entfernbar sein, deshalb
			// d�rfen sie keinen Besitzer haben.
			bd->setze_besitzer( NULL );
		}
		bd->setze_text( NULL );
	}
	return true;
}


static int
wkz_remover_intern(spieler_t *sp, karte_t *welt, koord pos, const char *&msg)
{
DBG_MESSAGE("wkz_remover_intern()","at (%d,%d)", pos.x, pos.y);

	planquadrat_t *plan = welt->access(pos);

	if(!plan) {
		return false;
	}

	grund_t *gr=0;
      const bool backwards = (event_get_last_control_shift()==2);
    	// remove lower ground first with CNTRL
     	for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
		gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
     		if(gr->ist_im_tunnel()) {
     			// do not remove tunnel ground
     			gr = 0;
     			continue;
     		}
		// ok, something to remove from here ...
		if(gr->obj_count()>0  ||  (gr->hat_wege()  &&  (gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL))) {
			break;
		}
	}
	// everything removed, nothing left ...
	if(gr==NULL) {
		return true;
	}

	// prissi: Leitung pr�fen (can cross ground of another player)
	leitung_t *lt=dynamic_cast<leitung_t *>(gr->suche_obj(ding_t::leitung));
	if(lt!=NULL  &&  lt->gib_besitzer()==sp) {
		gr->obj_remove(lt,sp);
		delete lt;
		return true;
	}

	// check for signal
	if(gr->suche_obj(ding_t::signal)!=NULL  ||  gr->suche_obj(ding_t::presignal)!=NULL) {
DBG_MESSAGE("wkz_remover()",  "removing signal %d,%d",  pos.x, pos.y);
		if(gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL) {
			blockmanager *bm = blockmanager::gib_manager();
			const bool ok = bm->entferne_signal(welt, gr->gib_pos());
			if(!ok) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Ambiguous signal combination.\nTry removing by clicking the\nother side of the signal.\n"), w_autodelete);
			}
			return ok;
		}
		else {
			msg = "Das Feld gehoert\neinem anderen Spieler\n";
			return false;
		}
	}

	if(gr->suche_obj(ding_t::roadsign)!=NULL) {
DBG_MESSAGE("wkz_remover()",  "removing roadsign %d,%d",  pos.x, pos.y);
		roadsign_t *rs = dynamic_cast<roadsign_t *>(gr->suche_obj(ding_t::roadsign));
		if(rs->gib_besitzer()==sp  ||  rs->gib_besitzer()==NULL) {
			rs->entferne(sp);
			delete rs;
			return true;
		}
		else {
			msg = "Das Feld gehoert\neinem anderen Spieler\n";
			return false;
		}
	}

	// Haltestelle pr�fen
	halthandle_t halt = gr->gib_halt();
DBG_MESSAGE("wkz_remover()", "bound=%i",halt.is_bound());
	if( halt.is_bound()  &&  (halt->gib_besitzer()==sp  ||  halt->gib_besitzer()==welt->gib_spieler(1))) {
		return entferne_haltestelle(welt, sp, halt, gr->gib_pos(), msg);
	}

	// citycar? (we allow always)
	if(gr->suche_obj(ding_t::verkehr)) {
		stadtauto_t *citycar = dynamic_cast<stadtauto_t *>(gr->suche_obj(ding_t::verkehr));
		delete citycar;
		return true;
	}

	// pedestrians?
	if(gr->suche_obj(ding_t::fussgaenger)) {
		fussgaenger_t *fussgaenger = dynamic_cast<fussgaenger_t *>(gr->suche_obj(ding_t::fussgaenger));
		delete fussgaenger;
		return true;
	}

DBG_MESSAGE("wkz_remover()", "check tunnel/bridge");

	// beginning/end of bridge?
	if(gr->ist_bruecke()) {
		if(gr==plan->gib_kartenboden()) {
DBG_MESSAGE("wkz_remover()",  "removing bridge from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
			bruecke_t *br = dynamic_cast<bruecke_t *>(gr->suche_obj(ding_t::bruecke));
			if(br) {
				msg = brueckenbauer_t::remove(welt, sp, gr->gib_pos(), br->gib_besch()->gib_wegtyp());
			}
		}
		return msg == NULL;
	}

	// beginning/end of tunnel
	if(gr->ist_tunnel()) {
		if(gr==plan->gib_kartenboden()) {
DBG_MESSAGE("wkz_remover()",  "removing tunnel  from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
			weg_t *weg = gr->gib_weg(weg_t::schiene);

			if(!weg) {
				weg = gr->gib_weg(weg_t::strasse);
			}
			msg = tunnelbauer_t::remove(welt, sp, gr->gib_pos(), weg->gib_typ());
		}
		return msg == NULL;
	}

	msg = gr->kann_alle_obj_entfernen(sp);
	if( msg ) {
		return false;
	}

#if 1
	// since buildings can have more than one tile, we must handle them together
	gebaeude_t *gb = static_cast<gebaeude_t *>(gr->suche_obj(ding_t::gebaeude));
	if(gb) {
		DBG_MESSAGE("wkz_remover()",  "removing building" );
		const haus_tile_besch_t *tile  = gb->gib_tile();
		koord size = tile->gib_besch()->gib_groesse( tile->gib_layout() );

		// get startpos
		koord k=tile->gib_offset();
		if(k != koord(0,0)) {
			return wkz_remover_intern(sp, welt, pos-k, msg);
		}
		else {
			// remove factory?
			if(gb->fabrik()!=NULL) {

				// remove connections ...
				fabrik_t *fab=gb->fabrik();
DBG_MESSAGE("wkz_remover()", "removing factory:  %p");

				if(welt->access(fab->gib_pos().gib_2d())->get_haltlist().get_count()!=0) {
					// remove from all stops
					msg = "Das Feld gehoert\neinem anderen Spieler\n";
					return false;
				}


				// delete it from map
DBG_MESSAGE("wkz_remover()", "removing factory from world");
				gb->setze_fab(NULL);
				welt->rem_fab(fab);

DBG_MESSAGE("wkz_remover()", "removing factory:  supplier info change for %i factories",welt->gib_fab_list().count() );
				// remove all links from factories
				slist_iterator_tpl<fabrik_t *> iter (welt->gib_fab_list());
				while(iter.next()) {
					fabrik_t * fab = iter.get_current();
					fab->rem_lieferziel(pos);
					fab->rem_supplier(pos);
DBG_MESSAGE("wkz_remover()", "reconnecting factories");
				}
		welt->rem_fab((fabrik_t*)6);

				// remove from all cities
DBG_MESSAGE("wkz_remover()", "removing factory:  reconnecting towns");
				const weighted_vector_tpl<stadt_t *> *stadt = welt->gib_staedte();
				for( unsigned i=0;  i<stadt->get_count();  i++ ) {
					stadt->at(i)->verbinde_fabriken();
				}
welt->rem_fab((fabrik_t*)1);

				// finally delete it
				fab->~fabrik_t();
			}

			// remove town? (when removing townhall)
			if(gb->ist_rathaus()) {
				stadt_t *stadt = welt->suche_naechste_stadt(pos);
				if(!welt->rem_stadt( stadt )) {
					msg = "Das Feld gehoert\neinem anderen Spieler\n";
					return false;
				}
			}

DBG_MESSAGE("wkz_remover()", "removing building: cleanup");
			for(k.y = 0; k.y < size.y; k.y ++) {
				for(k.x = 0; k.x < size.x; k.x ++) {
					grund_t *gr = welt->lookup(k+pos)->gib_kartenboden();
					gr->obj_loesche_alle(sp);
					if(gr->gib_typ()==grund_t::fundament) {
						welt->access(k+pos)->kartenboden_setzen(new boden_t(welt, koord3d(k+pos,welt->min_hgt(k+pos))), false);
					}
				}
			}

		}
		welt->rem_fab((fabrik_t*)1);
		return true;
	}
#endif

	// remove everything else ...
	if(gr->obj_count()>0  &&  !gr->ist_bruecke()) {
DBG_MESSAGE("wkz_remover()",  "removing everything from %d,%d,%d",gr->gib_pos().x, gr->gib_pos().y, gr->gib_pos().z);
		gr->obj_loesche_alle(sp);
		return true;
	}

	// pr�fen, ob boden entfernbar
	if(gr->gib_besitzer() != NULL && gr->gib_besitzer() != sp) {
		msg = "Das Feld gehoert\neinem anderen Spieler\n";
		return false;
	}

	// ok, now we removed every object, that should be removed one by one.
	// the following objects will be removed together
	if( gr->gib_weg(weg_t::schiene) != NULL ) {
		DBG_MESSAGE("wkz_remover()",  "removing block" );
		if(!blockmanager::gib_manager()->entferne_schiene(welt, gr->gib_pos())) {
			return false;
		}
	}

DBG_MESSAGE("wkz_remover()", "removing way");
	/*
	* Eigentlich m�ssen wir hier noch verhindern, da� ein Bahnhofsgeb�ude oder eine
	* Bushaltestelle vereinzelt wird!
	* Sonst l�sst sich danach die Richtung der Haltestelle verdrehen und die Bilder
	* gehen kaputt.
	*/
	int cost_sum = gr->weg_entfernen(weg_t::schiene, true);
	if(cost_sum>0  &&  gr->gib_weg(weg_t::strasse)) {
		// remove only railway track
		return true;
	}
	cost_sum += gr->weg_entfernen(weg_t::schiene_monorail, true);
	if(cost_sum>0  &&  gr->gib_weg(weg_t::strasse)) {
		// remove only railway track
		return true;
	}
	cost_sum += gr->weg_entfernen(weg_t::strasse, true);
	cost_sum += gr->weg_entfernen(weg_t::wasser, true);
	cost_sum += gr->weg_entfernen(weg_t::luft, true);

	if(cost_sum > 0) {
		sp->buche(-cost_sum, pos, COST_CONSTRUCTION);
	}
DBG_MESSAGE("wkz_remover()", "check ground");

	// now labels on water
	// replacements of ground will give a grass tile
	// => 	don't do this on water ...
	if(!gr->ist_wasser()  &&  plan->gib_kartenboden()==gr) {
		bool label = gr->gib_besitzer() && welt->gib_label_list().contains(pos);
		plan->kartenboden_setzen(new boden_t(welt, gr->gib_pos()), false);
		if(label) {
			plan->gib_kartenboden()->setze_besitzer(sp);
		}
	}

	if(gr!=plan->gib_kartenboden()) {
DBG_MESSAGE("wkz_remover()", "removing ground");
		// remove upper or lower ground
		plan->boden_entfernen(gr);
//		delete gr;
	}

	return true;
}


int
wkz_remover(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_remover()","at %d,%d", pos.x, pos.y);
    const char *fail = NULL;

    if(!wkz_remover_intern(sp, welt, pos, fail)) {
  if(fail) {
      create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, fail), w_autodelete);
  }
  return false;
    }
    // Nachbarschaft (Bilder) neu berechnen

    if(pos.x>1)
  welt->lookup(pos+koord(-1, 0))->gib_kartenboden()->calc_bild();
    if(pos.y>1)
  welt->lookup(pos+koord::nord)->gib_kartenboden()->calc_bild();

    if(pos.x<welt->gib_groesse_x()-1)
  welt->lookup(pos+koord::ost)->gib_kartenboden()->calc_bild();
    if(pos.y<welt->gib_groesse_y()-1)
  welt->lookup(pos+koord::sued)->gib_kartenboden()->calc_bild();

    return true;
}



/*
 * aufruf mit i == -1, j == -1 initialisiert
 *
 * erster aufruf setzt start
 * zweiter aufruf setzt ende und baut
 */
const weg_besch_t *default_track=NULL;
const weg_besch_t *default_road=NULL;

koord3d wkz_wegebau_start=koord3d::invalid;

int
wkz_wegebau(spieler_t *sp, karte_t *welt,  koord pos, value_t lParam)
{
	static zeiger_t *wkz_wegebau_bauer = NULL;
	static bool erster = true;
	static koord start, ziel;

	const weg_besch_t * besch = (const weg_besch_t *) (lParam.p);
	enum wegbauer_t::bautyp bautyp = wegbauer_t::strasse;

	if(besch->gib_wtyp() == weg_t::schiene) {
		if(besch->gib_styp() == 7){ // Dario: Tramway
			bautyp = wegbauer_t::schiene_tram;
		}
		else {
			bautyp = wegbauer_t::schiene;
		}
		default_track = besch;
	}
	else if(besch->gib_wtyp() == weg_t::schiene_monorail) {
		bautyp = besch->gib_styp()==1 ? wegbauer_t::schiene_elevated_monorail : wegbauer_t::schiene_monorail;
	} else if(besch->gib_wtyp() == weg_t::powerline) {
		bautyp = wegbauer_t::leitung;
	} else if(besch->gib_wtyp() == weg_t::strasse) {
		default_road = besch;
	}
	if(besch->gib_wtyp() == weg_t::wasser) {
		bautyp = wegbauer_t::wasser;
	}
	if(besch->gib_wtyp() == weg_t::luft) {
		bautyp = wegbauer_t::luft;
	}

	if(pos==INIT || pos == EXIT) {  // init strassenbau
		erster = true;

		if(wkz_wegebau_bauer != NULL) {
			delete wkz_wegebau_bauer;
			wkz_wegebau_bauer = NULL;
		}
		wkz_wegebau_start=koord3d::invalid;
	}
	else {
		const planquadrat_t *plan = welt->lookup(pos);

		// Hajo: check if it's safe to build here
		if(plan == NULL) {
			return false;
		}
		else {
			grund_t *gr = welt->lookup(pos)->gib_kartenboden();
			if(gr == NULL ||  gr->ist_wasser() ||  gr->kann_alle_obj_entfernen(sp)) {
				return false;
			}
		}


		if( erster ) {
			DBG_MESSAGE("wkz_wegebau()", "Setting start to %d,%d",pos.x, pos.y);

			start = pos;
			erster = false;

			// symbol f�r strassenanfang setzen
			grund_t *gr = welt->lookup(pos)->gib_kartenboden();
			wkz_wegebau_bauer = new zeiger_t(welt, gr->gib_pos(), sp);
			wkz_wegebau_bauer->setze_bild(0, skinverwaltung_t::bauigelsymbol->gib_bild_nr(0));
			gr->obj_pri_add(wkz_wegebau_bauer, PRI_NIEDRIG);
			wkz_wegebau_start = gr->gib_pos();
		}
		else {
			// Hajo: symbol f�r strassenanfang entfernen
			delete( wkz_wegebau_bauer );
			wkz_wegebau_bauer = NULL;
			wkz_wegebau_start = koord3d::invalid;

			wegbauer_t bauigel(welt, sp);

			// Hajo: to test building as disguised AI player
			// wegbauer_t bauigel(welt, welt->gib_spieler(2));
			erster = true;

			DBG_MESSAGE("wkz_wegebau()", "Setting end to %d,%d",pos.x, pos.y);
			ziel = pos;

			// Hajo: to test baubaer mode (only for AI)
			// bauigel.baubaer = true;

			bauigel.route_fuer(bautyp, besch);
			if(event_get_last_control_shift()==2) {
DBG_MESSAGE("wkz_wegebau()", "try straight route");
				bauigel.set_keep_existing_ways(false);
				bauigel.calc_straight_route(start,ziel);
			}
			else {
				bauigel.set_keep_existing_faster_ways(true);
				bauigel.calc_route(start,ziel);
			}

			// sp->platz1 = start;
			// sp->platz2 = ziel;
			// sp->create_complex_rail_transport();
			// sp->create_complex_road_transport();

			DBG_MESSAGE("wkz_wegebau()", "builder found route with %d sqaures length.", bauigel.max_n);

			bauigel.calc_costs();
			bauigel.baue();
		}
	}
	return true;
}


#if 0
/* build a ramp for monorail to change between levels ...
 * @author prissi
 */
int
wkz_monorail_rampe(spieler_t *sp, karte_t *welt, koord pos, value_t lParam)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		const bruecke_besch_t * besch = (const bruecke_besch_t *) (lParam.p);
		const weg_besch_t * weg_besch=NULL
		koord zv=koord::invalid;
		ribi_t::ribi ribi=ribi_typ(gr->gib_grund_hang());

		grund_t *gr=welt->lookup(pos)->gib_kartenboden();
		const char *p=gr2->kann_alle_obj_enternene(sp);
		if(p) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p), w_autodelete);
			return false;
		}
		if(gr->suche_obj(ding_t::signal)!=NULL  ||  gr->suche_obj(ding_t::presignal)!=NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
			return false;
		}
		weg_t *w=gr->gib_weg(weg_t::monorail);
		if(w) {
			ribi = gr->gib_weg_ribi_unmasked(weg_t::monorail);
			if(!ribi_t::ist_einach(ribi)) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "A bridge must start on a way!"), w_autodelete);
				return false;
			}
			weg_besch = w->gib_besch();
		}

		// check for top floor ...
		grund_t *gr2=welt->lookup(koord3d(pos,gr->gib_hoehe()+16));
		if(gr2) {
			if(gr2->gib_typ()!=grund_t::monorailboden) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
				return false;
			}
			// a monorailground should have a monorail on it, or something was very wrong ...
			ribi_t::ribi ribi2 = gib_weg_ribi(weg_t::monorail);
			if(!ribi_t::ist_gerade(ribi|ribi2)  ||  !ribi_t::ist_einfach(ribi2)) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "A bridge must start on a way!"), w_autodelete);
				return false;
			}
			// something else
			const char *p=gr2->kann_alle_obj_enternene(sp);
			if(p) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p), w_autodelete);
				return false;
			}
			// now remove ground
			gr2->obj_loesche_alle(sp);
			if(!weg_besch) {
				// monorail ground => must have a monorail on it!
				weg_besch = gr2->gib_weg(weg_t::schiene_monorail)->gib_besch();
			}
			gr2->weg_entfernen(weg_t::schiene_monorail,false);
		}

		// still not a monorail way here?
		if(weg_besch==NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "A bridge must start on a way!"), w_autodelete);
			return false;
		}

		// ok, now we have empty ground on top and a direction (hopefully)
		brueckenbauer_t::baue_auffahrt( welt, sp, gr->gib_pos(), koord(ribi_t::rueckwaerts(ribi)), bruecke_besch, weg_besch );
		return true;
	}
}
#endif


/* build all kind of station buildings
 * @author V. Meyer
 */
static int
wkz_station_building_aux(spieler_t *sp, karte_t *welt, koord pos, const haus_besch_t * besch)
{
DBG_MESSAGE("wkz_station_building_aux()", "building mail office/station building on square %d,%d", pos.x, pos.y);
	const bool is_post=(besch->get_enabled()&2)!=0;

	if(welt->ist_in_kartengrenzen(pos)) {
		koord size = besch->gib_groesse();
		koord3d k=welt->lookup(pos)->gib_kartenboden()->gib_pos();
		int rotate=-1, built_rotate=-1;
		static koord rotate_koords[4]={koord(0,-1),koord(1,0),koord(0,1),koord(-1,0)};
		bool slope_check = size.x+size.y>2;
		halthandle_t halt;

		// get initial rotation
		if(besch->gib_all_layouts()>1) {
DBG_MESSAGE("wkz_station_building_aux()","building %s; layouts %i",besch->gib_name(),besch->gib_all_layouts());
			uint8 best=0;
			for( int i=0;  i<4;  i++  ) {
				if(welt->ist_in_kartengrenzen(pos+rotate_koords[i])) {
					grund_t *gr=welt->lookup(pos+rotate_koords[i])->gib_kartenboden();
					if(gr->gib_halt().is_bound()) {
//DBG_MESSAGE("wkz_station_building_aux()","test layouts %i",i);
						// get best alignment judging all
						uint8 current=1;
						if(gr->gib_weg(weg_t::schiene)) {
							current += 2;
						} else if(gr->gib_weg(weg_t::strasse)) {
							current += 1;
						} else if(gr->gib_weg(weg_t::wasser)) {
							current += 1;
						} else if(gr->gib_weg(weg_t::luft)) {
							current += 3;
						} else if(gr->gib_typ()==grund_t::fundament) {
							// always samle layout as next station building
							built_rotate = static_cast<gebaeude_t *>(gr->obj_bei(0))->gib_tile()->gib_layout()%besch->gib_all_layouts();
//DBG_MESSAGE("wkz_station_building_aux()","find building with layout %i",built_rotate);
							continue;
						}
						if(current>best) {
							best = current;
							rotate = i;
//DBG_MESSAGE("wkz_station_building_aux()","propose layout %i",i);
						}
					}
				}
			}
		}

		// if a station building is found and nothing else, then mimic the station building alingment
		if(rotate!=-1) {
			built_rotate = rotate;
		}
		else if(built_rotate==-1) {
			// nothing found ... should never happen!!!
dbg->warning("wkz_station_building_aux()","no near building for a station extension found");
			built_rotate = 0;
		}

		// now try to built it (best layout first)
		for( int i=0;  i<besch->gib_all_layouts()  &&  !halt.is_bound();  i++  ) {
			if((i+built_rotate)&1!=0) {
				if(welt->ist_platz_frei(pos, size.y, size.x, NULL, slope_check)) {
					halt = suche_nahe_haltestelle(sp, welt, k, size.y, size.x);
				}
			}
			else {
				if(welt->ist_platz_frei(pos, size.x, size.y, NULL, slope_check)) {
					halt = suche_nahe_haltestelle(sp, welt, k, size.y, size.x);
				}
			}
			rotate = (built_rotate+i)%besch->gib_all_layouts();
		}

		// is there already a halt to connect?
		if(halt.is_bound()) {
			if(is_post  &&  halt->get_post_enabled()) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Station already\nhas a post office!\n"), w_autodelete);
			}
			hausbauer_t::baue(welt, halt->gib_besitzer(), k, rotate, besch, true, &halt);
			sp->buche(umgebung_t::cst_multiply_post*besch->gib_level()*besch->gib_b()*besch->gib_h(), pos, COST_CONSTRUCTION);
			halt->recalc_station_type();
		}
		else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Post muss neben\nHaltestelle\nliegen!\n", besch->gib_cursor()->gib_bild_nr(0) ), w_autodelete);
		}
		return true;
	}
	return false;
}



/* any station extension building here: Alignment automatically! */
int wkz_station_building(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	wkz_station_building_aux(sp, welt, pos, (const haus_besch_t *)value.p);
	return true;
}


#ifdef LAGER_NOT_IN_USE
int
wkz_lagerhaus(spieler_t *sp, karte_t *welt, koord pos)
{
    DBG_MESSAGE("wkz_lagerhaus()", "building storage shed on square %d,%d", pos.x, pos.y);

    if(welt->ist_in_kartengrenzen(pos)) {
  bool can_build = (welt->lookup(pos)->gib_boden()->kann_alle_obj_entfernen(sp) == NULL);

  if( can_build ) {
      halthandle_t halt = suche_nahe_haltestelle(sp, welt, pos);

      if(halt.is_bound()) {
    grund_t *gr = welt->lookup(pos)->gib_boden();
    lagerhaus_t *lager = new lagerhaus_t( welt, gr->gib_pos(), sp );
    lager->setze_name( halt->gib_name() );
    gr->baue_gebaeude( IMG_LAGERHAUS, lager, true );
                gr = NULL;

    halt->add_grund(welt->lookup(pos)->gib_boden());
    halt->setze_lager( lager );

      } else {
    create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Lager muss neben\nHaltestelle\nliegen!\n"), w_autodelete);
      }
  }
  return true;
    } else {
  return false;
    }
}
#endif



/* build a dock either small or large */
int
wkz_dockbau(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	const haus_besch_t *besch=(const haus_besch_t *)value.p;
	bool ok = false;

	// docks k�nnen nicht direkt am kartenrand gebaut werden,
	// da die schiffe dort nicht fahren k�nnen

	if(pos == INIT || pos == EXIT) {
		// init und exit ignorieren
		return true;
	}
	int hang = welt->get_slope(pos);
	// first get the size
	int len = besch->gib_groesse().y-1;
	koord dx = koord((hang_t::typ)hang);
	koord last_pos = pos - dx*len;

	// check, if we can built here ...
	ok=hang_t::ist_einfach(hang);
	if(ok) {
		for(int i=0;  i<=len;  i++  ) {
			if(!welt->ist_in_kartengrenzen(pos-dx*i)) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Zu nah am Kartenrand"), w_autodelete);
				return false;
			}
			const grund_t *gr=welt->lookup(pos-dx*i)->gib_kartenboden();
			if(i==0  &&  (gr->ist_wasser()  ||  gr->hat_wege()  ||  gr->gib_typ()!=grund_t::boden)) {
				ok = false;
			}
			if(i!=0  &&  (!gr->ist_wasser()  ||  gr->suche_obj(ding_t::gebaeude)  ||  gr->gib_depot())) {
				ok = false;
			}
		}
	}

	if(ok) {
DBG_MESSAGE("wkz_dockbau()","building dock from square (%d,%d) to (%d,%d)", pos.x, pos.y, last_pos.x, last_pos.y);

		int layout = 0;
		koord3d bau_pos = welt->lookup(pos)->gib_kartenboden()->gib_pos();
		switch(hang) {
			case hang_t::sued:
				layout = 0;
				break;
			case hang_t::ost:
				layout = 1;
				break;
			case hang_t::nord:
				layout = 2;
				bau_pos = welt->lookup(last_pos)->gib_kartenboden()->gib_pos();
				break;
			case hang_t::west:
				layout = 3;
				bau_pos = welt->lookup(last_pos)->gib_kartenboden()->gib_pos();
				break;
		}

DBG_MESSAGE("wkz_dockbau()","search for stop");
		halthandle_t halt = suche_nahe_haltestelle(sp, welt, welt->lookup(pos)->gib_kartenboden()->gib_pos() );
		bool neu = !halt.is_bound();

		if(neu) { // neues dock
DBG_MESSAGE("wkz_dockbau()","spawn new halt");
			halt = sp->halt_add(pos);
		}

DBG_MESSAGE("wkz_dockbau()","remove ground below slope");
		// remove old ground below slope
		welt->access(pos)->kartenboden_setzen(new boden_t(welt, welt->lookup(pos)->gib_kartenboden()->gib_pos()), false);

DBG_MESSAGE("wkz_dockbau()","finally errect it");
		hausbauer_t::baue(welt, halt->gib_besitzer(), bau_pos, layout,besch, 0, &halt);

DBG_MESSAGE("wkz_dockbau()","clean up");
		for(int i=0;  i<=len;  i++ ) {
			koord p=pos-dx*i;
			// Kosten
			sp->buche(umgebung_t::cst_multiply_dock*besch->gib_level(), p, COST_CONSTRUCTION);
			// dock-land anbindung geh�rt auch zur haltestelle
//			halt->add_grund(gr);
		}
DBG_MESSAGE("wkz_dockbau()","recalc station type");
		halt->recalc_station_type();

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			if(stadt) {
				const int count = sp->get_haltcount();
				const char *name = stadt->haltestellenname(pos, "Dock", count);
				welt->lookup(halt->gib_basis_pos())->gib_kartenboden()->setze_text( name );
			}
			else {
				// get a default name
				const int count = sp->get_haltcount();
				char *tmp=new char[256];
				sprintf( tmp, translator::translate("land stop %i %s"), count, translator::translate("Dock") );
				welt->lookup(halt->gib_basis_pos())->gib_kartenboden()->setze_text( tmp );
			}
		}

DBG_MESSAGE("wkz_dockbau()","rebuilt destination");
		// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
//DBG_MESSAGE("haltestelle_t::add_grund()","->rebuild_destinations()");
		const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
		slist_iterator_tpl <halthandle_t> iter (list);
		while( iter.next() ) {
			iter.get_current()->rebuild_destinations();
			INT_CHECK( "simwerkz 1080" );
		}

		ok = true;
	}
	else {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
		return false;
	}

	return ok;
}



int
wkz_senke(spieler_t *sp, karte_t *welt, koord pos)
{
DBG_MESSAGE("wkz_senke()","called on %d,%d", pos.x, pos.y);

	if(!welt->ist_in_kartengrenzen(pos)) {
		return false;
	}

	//int top = welt->lookup(pos)->gib_kartenboden()->gib_top();
	int hangtyp = welt->get_slope(pos);

	if(hangtyp == 0  &&
		!welt->lookup(pos)->gib_kartenboden()->ist_wasser()  &&
		welt->lookup(pos)->gib_kartenboden()->kann_alle_obj_entfernen(sp)==NULL)
	{

		fabrik_t *fab=leitung_t::suche_fab_4(pos);
		if(fab==NULL) {
DBG_MESSAGE("wkz_senke()","no factory near (%i,%i)",pos.x, pos.y);
			// need a factory
			return false;
		}
		// remove everything on that spot
		const char *fail = NULL;
		if(!wkz_remover_intern(sp, welt, pos, fail)) {
			if(fail) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, fail), w_autodelete);
			}
		}
		// now decide from the string whether a source or drain is built
		grund_t *gr = welt->lookup(pos)->gib_kartenboden();
		int fab_name_len = strlen( fab->gib_besch()->gib_name() );
		if(fab_name_len>11   &&  (strcmp(fab->gib_besch()->gib_name()+fab_name_len-9, "kraftwerk")==0  ||  strcmp(fab->gib_besch()->gib_name()+fab_name_len-11, "Power Plant")==0)) {
			gr->obj_add( new pumpe_t(welt, gr->gib_pos(), sp) );
		}
		else {
			gr->obj_add(new senke_t(welt, gr->gib_pos(), sp));
		}
	}
	return true;
}



// builds signal
int wkz_signal_aux(spieler_t *sp, karte_t *welt, koord pos,bool presignal)
{
	DBG_MESSAGE("wkz_signal_aux()","called on %d,%d for a %ssignal", pos.x, pos.y, presignal?"pre":"");
	if(welt->ist_in_kartengrenzen(pos)) {

		blockmanager * bm = blockmanager::gib_manager();
		const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";

		const planquadrat_t *plan=welt->lookup(pos);
		const bool backwards = (event_get_last_control_shift()==2);
		const grund_t *gr=0;
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
			// =0 is gound, all other are either bridges or tunnel
	     		if(gr->ist_im_tunnel()) {
	     			gr = NULL;
	     			continue;
	     		}
			if((gr->gib_weg(weg_t::schiene)!=NULL  ||  gr->gib_weg(weg_t::schiene_monorail)!=NULL)  &&  (gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL)) {
				// found ground
				break;
			}
			gr = NULL;
		}

		if(gr) {
			error = bm->neues_signal(welt, sp, gr->gib_pos(), (dynamic_cast<zeiger_t*>(welt->gib_zeiger()))->gib_richtung(), presignal);
		}
		if(error != NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, error), w_autodelete);
		}
		return error == NULL;
	} else {
		return false;
	}
}



int wkz_signale(spieler_t *sp, karte_t *welt, koord pos)
{
	return wkz_signal_aux(sp,welt,pos,false);
}



int wkz_presignals(spieler_t *sp, karte_t *welt, koord pos)
{
	return wkz_signal_aux(sp,welt,pos,true);
}



int wkz_roadsign(spieler_t *sp, karte_t *welt, koord pos, value_t lParam)
{
	DBG_MESSAGE("wkz_roadsign()","called on %d,%d", pos.x, pos.y);
	const roadsign_besch_t * besch = (const roadsign_besch_t *) (lParam.p);

	if(welt->ist_in_kartengrenzen(pos)) {

		grund_t *gr = NULL;
		weg_t *weg = NULL;
		const planquadrat_t *plan = welt->lookup(pos);
		const char * error = "Hier kann kein\nSignal aufge-\nstellt werden!\n";

		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
			// =0 is gound, all other are either bridges or tunnel
	     		if(gr->ist_im_tunnel()) {
	     			gr = NULL;
	     			continue;
	     		}
			weg = gr->gib_weg(weg_t::strasse);
			if(  (gr->gib_besitzer()==sp  ||  gr->gib_besitzer()==NULL)  &&  weg!=NULL) {
				// found ground
				break;
			}
			gr = NULL;
		}

		if(gr) {

			// get the sign dirction
			ribi_t::ribi dir = weg->gib_ribi_unmasked();
			if(ribi_t::ist_gerade(dir)  ||  (besch->is_traffic_light()  &&  ribi_t::ist_kreuzung(dir))) {

				// if single way, we need to reduce the allowed ribi to one
DBG_MESSAGE("wkz_roadsign()","dir is %i", dir);
				if(besch->is_single_way()  ||  besch->is_free_route()) {
					for( int i=0;  i<=4;  i++) {
						if((dir&ribi_t::nsow[i])!=0) {
							dir = ribi_t::nsow[i];
							break;
						}
					}
DBG_MESSAGE("wkz_roadsign()","single dir is %i", dir);
				}

				// if there is already a sing, we might need to inverse the direction
				roadsign_t *rs = dynamic_cast<roadsign_t *>(gr->suche_obj(ding_t::roadsign));
				if(rs) {
					// revers only if single way sign
					if(besch->is_single_way()  ||  besch->is_free_route()) {
DBG_MESSAGE("wkz_roadsign()","reverse ribi %i", ribi_t::rueckwaerts(dir) );
						rs->set_dir( ribi_t::rueckwaerts(rs->get_dir()) );
					}
				}
				else {
					// add a new roadsign at position zero!
					rs = new roadsign_t( welt, gr->gib_pos(), dir, besch );
					ding_t *dt = gr->obj_bei(0);
					if(dt) {
						gr->obj_remove( dt, dt->gib_besitzer() );
						gr->obj_pri_add(rs,0);
						gr->obj_pri_add(dt,0);
					}
					else {
						gr->obj_pri_add(rs,0);
					}
					sp->buche( -besch->gib_preis(), pos, COST_CONSTRUCTION);
				}
				error = NULL;
			}
		}

		if(error != NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, error), w_autodelete);
		}
		return error == NULL;

	} else {

		return false;
	}
}




// built all types of depots
static bool
wkz_depot_aux(spieler_t *sp, karte_t *welt, koord pos,const haus_besch_t *besch,weg_t::typ wegtype,sint32 cost)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *bd = welt->lookup(pos)->gib_kartenboden();
		int ribi = 0;

DBG_MESSAGE("wkz_depot_aux()","for depot %s with waytype %d",besch->gib_name(),wegtype);
		if(wegtype==weg_t::schiene_monorail) {
			weg_t *w=bd->gib_weg(wegtype);
			if(w==NULL) {
				// since we built on ground level2 ...
				const char *p=bd->kann_alle_obj_entfernen(sp);
				if(p) {
					create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p), w_autodelete);
					return false;
				}

				bd = welt->lookup(bd->gib_pos()+koord3d(0,0,16));	// for monorail
				if(!bd) {
	DBG_MESSAGE("wkz_depot_aux()","monorail not found!");
					// no monorail here ...
					create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Cannot built depot here!"), w_autodelete);
					return false;
				}
			}
		}

		// avoid building over a stop
		if(bd->gib_halt().is_bound()) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
			return false;
		}

		if(wegtype==weg_t::wasser  &&  bd->ist_wasser()) {
			// assume one orientation with water
			ribi = ribi_t::sued;
		}
		else {
			ribi = bd->gib_weg_ribi_unmasked(wegtype);
		}

		if(ribi_t::ist_einfach(ribi)  &&  welt->get_slope(pos)==0) {

			const char *p=bd->kann_alle_obj_entfernen(sp);
			if(p) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p), w_autodelete);
				return false;
			}

			int layout = 0;
			switch(ribi) {
				//case ribi_t::sued:layout = 0;  break;
				case ribi_t::ost:   layout = 1;    break;
				case ribi_t::nord:  layout = 2;    break;
				case ribi_t::west:  layout = 3;    break;
			}
			hausbauer_t::neues_gebaeude( welt, sp, bd->gib_pos(), layout, besch );
			sp->buche(cost, pos, COST_CONSTRUCTION);
			welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), karte_t::Z_PLAN,  NO_SOUND, NO_SOUND );
			return true;
		}
	}
	create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Cannot built depot here!"), w_autodelete);
	return false;
}



/* prissi: universal depot tool */
/* returns fals on success, so one can built something else
 * otherwise will return true to try again ... */
int wkz_depot(spieler_t *sp, karte_t *welt, koord pos,value_t w)
{
	if(pos==INIT  ||  pos==EXIT) {
		return true;
	}

	const haus_besch_t *besch=(const haus_besch_t *)w.p;
	if(hausbauer_t::bahn_depot_besch==besch) {
		return wkz_depot_aux( sp, welt, pos, besch, weg_t::schiene, umgebung_t::cst_depot_rail );
	}
	else if(hausbauer_t::monorail_depot_besch==besch) {
		// since it need also a foundations, ots slightly more complex ...
		if( wkz_depot_aux( sp, welt, pos, besch, weg_t::schiene_monorail, umgebung_t::cst_depot_rail ) ) {
			grund_t *bd = welt->lookup(pos)->gib_kartenboden();
			if(bd->gib_depot()==NULL  &&  bd->obj_bei(0)==NULL) {
				hausbauer_t::baue( welt, sp, bd->gib_pos(), 0, hausbauer_t::monorail_foundation_besch, true );
			}
		}
		return true;
	}
	else if(hausbauer_t::tram_depot_besch==besch) {
		return wkz_depot_aux( sp, welt, pos, besch, weg_t::schiene, umgebung_t::cst_depot_rail );
	}
	else if(hausbauer_t::str_depot_besch==besch) {
		return wkz_depot_aux( sp, welt, pos, besch, weg_t::strasse, umgebung_t::cst_depot_road );
	}
	else if(hausbauer_t::sch_depot_besch==besch) {
		return wkz_depot_aux( sp, welt, pos, besch, weg_t::wasser, umgebung_t::cst_depot_ship );
	}
	else if(hausbauer_t::air_depot.contains(besch)) {
		return wkz_depot_aux( sp, welt, pos, besch, weg_t::luft, umgebung_t::cst_multiply_airterminal );
	}
	else {
		DBG_MESSAGE("wkz_depot()","called with unknown besch %s",besch->gib_name() );
		return false;
	}
}





// built all types of stops but sea harbours
static bool
wkz_halt_aux(spieler_t *sp, karte_t *welt, koord pos,const haus_besch_t *besch,weg_t::typ wegtype,sint32 cost,const char *type_name)
{
DBG_MESSAGE("wkz_halt_aux()", "building %s on square %d,%d for waytype %x", besch->gib_name(), pos.x, pos.y, wegtype);

	const char *p_error=(besch->gib_all_layouts()==4)?"No terminal station here!":"No through station here!";

	const planquadrat_t *plan=welt->lookup(pos);
	if(plan) {

		// first we must find the right ground; thus we coycly through the ground list until something is found
		grund_t *bd = NULL;
		weg_t *weg = NULL;
		const bool backwards = (event_get_last_control_shift()==2);
		for(unsigned i=0;  i<plan->gib_boden_count();  i++  ) {
			grund_t *test_gr=plan->gib_boden_bei( backwards ? plan->gib_boden_count()-1-i : i );
DBG_MESSAGE("wkz_halt_aux()", "test_gr(%d)=%p",i,test_gr);
			// ignore tunnel
			if(test_gr->gib_typ()==grund_t::tunnelboden) {
				continue;
			}
			// now just for error messages, we assing a valid ground
			// and check for ownership
			if(test_gr->gib_besitzer()!=welt->gib_spieler(1)  &&  test_gr->gib_besitzer()!=NULL  &&  test_gr->gib_besitzer()!=sp) {
				continue;
			}
			// check for way
			weg = test_gr->gib_weg(wegtype);
			if(weg==NULL) {
				if(weg_t::schiene==wegtype) {
					weg = test_gr->gib_weg(weg_t::schiene_monorail);
				}
				if(weg==NULL) {
					continue;
				}
			}
			// check slope
			if(test_gr->gib_weg_hang()!=0) {
				continue;
			}
			// and finally check for the presence of a stop
			if(test_gr->gib_halt().is_bound()) {
				continue;
			}
			// ok, now we have a track, where we can build!
			bd = test_gr;
			break;
		}

DBG_MESSAGE("wkz_halt_aux()", "bd=%p",bd);
		if(bd==NULL  ||  weg==NULL) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p_error), w_autodelete);
			return false;
		}

		const char *p=bd->kann_alle_obj_entfernen(sp);
		if(p) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p), w_autodelete);
			return false;
		}

		if(bd->gib_depot()) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
			return false;
		}

		// find out orientation ...
		int layout = 0;
		if(besch->gib_all_layouts()==2) {
			// through station
			ribi_t::ribi  ribi;
			if(bd->gib_weg_nr(1)  &&  bd->gib_weg_nr(0)) {
				// can only happen with road/rail combination ...
				ribi = bd->gib_weg_ribi_unmasked(weg_t::strasse)  |  bd->gib_weg_ribi_unmasked(weg_t::schiene);
			}
			else {
				ribi = weg->gib_ribi_unmasked();
			}
			// not straight: sorry cannot built here ...
			if(!ribi_t::ist_gerade(ribi)) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p_error), w_autodelete);
				return false;
			}
			layout = (ribi & ribi_t::nordsued)?0 :1;
		}
		else {
			// terminal station
			ribi_t::ribi ribi = weg->gib_ribi_unmasked();
			// sorry cannot built here ... (not a terminal tile)
			if(!ribi_t::ist_einfach(ribi)) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p_error), w_autodelete);
				return false;
			}

			switch(ribi) {
				//case ribi_t::sued:layout = 0;  break;
				case ribi_t::ost:   layout = 1;    break;
				case ribi_t::nord:  layout = 2;    break;
				case ribi_t::west:  layout = 3;    break;
			}
		}

		// seems everything ok, lets build
		halthandle_t halt = suche_nahe_haltestelle(sp,welt,bd->gib_pos());
		bool neu = !halt.is_bound();

		if(neu) {
			halt = sp->halt_add(pos);
DBG_MESSAGE("wkz_halt_aux()", "founding new station");
		}
		else {
DBG_MESSAGE("wkz_halt_aux()", "new segment for station");
		}

		bd->setze_besitzer(halt->gib_besitzer());
		hausbauer_t::neues_gebaeude( welt, halt->gib_besitzer(), bd->gib_pos(), layout, besch, &halt);
		halt->recalc_station_type();

		if(neu) {
			stadt_t *stadt = welt->suche_naechste_stadt(pos);
			if(stadt) {
				const int count = sp->get_haltcount();
				const char *name = stadt->haltestellenname(pos, type_name, count);
				bd->setze_text( name );
			}
			else {
				// get a default name
				const int count = sp->get_haltcount();
				char *tmp=new char[256];
				sprintf( tmp, translator::translate("land stop %i %s"), count,  translator::translate(type_name) );
				bd->setze_text( tmp );
			}
		}

		// rebuild destination lists (since maybe a convoi stopped here, but there was no station yet)
DBG_MESSAGE("wkz_halt_aux()","->rebuild_destinations()");
		const slist_tpl<halthandle_t> & list = haltestelle_t::gib_alle_haltestellen();
		slist_iterator_tpl <halthandle_t> iter (list);

		while( iter.next() ) {
			iter.get_current()->rebuild_destinations();
			INT_CHECK( "simwerkz 930" );
		}
		sp->buche(cost*besch->gib_level()*besch->gib_b()*besch->gib_h(), pos, COST_CONSTRUCTION);
		return true;
	}
	create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, p_error), w_autodelete);
	return false;

}



/* built a halt on a way
 * cannot built sea harbours, since those are really different ...
 * @author prissi
 */
int
wkz_halt(spieler_t *sp, karte_t *welt, koord pos, value_t value)
{
	if(pos==INIT  ||  pos==EXIT) {
		return true;
	}

	const haus_besch_t *besch=(const haus_besch_t *)value.p;
	if(besch->gib_utyp()==hausbauer_t::bahnhof) {
		return wkz_halt_aux( sp, welt, pos, besch, weg_t::schiene, umgebung_t::cst_multiply_station, "BF" );
	}
	else if(besch->gib_utyp()==hausbauer_t::monorailstop) {
		return wkz_halt_aux( sp, welt, pos, besch, weg_t::schiene_monorail, umgebung_t::cst_multiply_station, "BF" );
	}
	else if(besch->gib_utyp()==hausbauer_t::bushalt  ||  besch->gib_utyp()==hausbauer_t::ladebucht) {
		return wkz_halt_aux( sp, welt, pos, besch, weg_t::strasse, umgebung_t::cst_multiply_roadstop, "H" );
	}
	else if(besch->gib_utyp()==hausbauer_t::binnenhafen) {
		return wkz_halt_aux( sp, welt, pos, besch, weg_t::wasser, umgebung_t::cst_multiply_dock, "Dock" );
	}
	else if(besch->gib_utyp()==hausbauer_t::airport) {
		return wkz_halt_aux( sp, welt, pos, besch, weg_t::luft, umgebung_t::cst_multiply_airterminal, "Airport" );
	}
	else {
		DBG_MESSAGE("wkz_halt()","called with unknown besch %s",besch->gib_name() );
		return false;
	}
}



/* the following three routines add points to a schedule
 *  by we do not like to stop at AIs stop, but we want still force the truck to use AI roads
 * So if there is a halt, then it must be either public or our!
 * @autor prissi
 */
static int
wkz_fahrplan_insert_aux(spieler_t *sp, karte_t *welt, koord pos, fahrplan_t *fpl, bool append)
{
	if(pos==INIT  &&  pos==EXIT) {
		return false;
	}

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
		for(  unsigned cnt=0;  cnt<pl->gib_boden_count();  cnt++  ) {
			// with control backwards
			const unsigned i = (backwards) ? pl->gib_boden_count()-1-cnt : cnt;
			// ignore tunnel
	     		if(pl->gib_boden_bei(i)->ist_im_tunnel()) {
	     			continue;
	     		}
	     		// now just for error messages, we assing a valid ground
	     		// and check for ownership
			wrong_owner = false;
	     		bd = pl->gib_boden_bei(i);
			if(bd->gib_besitzer()!=sp  &&  bd->gib_besitzer()!=NULL  &&  bd->gib_besitzer()!=welt->gib_spieler(1)) {
				wrong_owner = true;
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
				fpl->append(welt, bd );
			}
			else {
				fpl->insert(welt, bd );
			}
		}
		else {
			// here we failed
			if(wrong_owner) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Das Feld gehoert\neinem anderen Spieler\n"), w_autodelete);
			}
			else {
				fpl->zeige_fehlermeldung(welt);
			}
			return false;
		}
	}
	return true;
}



int wkz_fahrplan_add(spieler_t *sp, karte_t *welt, koord pos,value_t f)
{
	return wkz_fahrplan_insert_aux( sp, welt, pos, (fahrplan_t *)f.p, true );
}



int wkz_fahrplan_ins(spieler_t *sp, karte_t *welt, koord pos,value_t f)
{
	return wkz_fahrplan_insert_aux( sp, welt, pos, (fahrplan_t *)f.p, false );
}



int wkz_marker(spieler_t *sp, karte_t *welt, koord pos)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		create_win(-1, -1, -1, new label_frame_t(welt, sp, pos), w_autodelete, magic_label_frame);
		return true;
	}
	return false;
}



int wkz_blocktest(spieler_t *, karte_t *welt, koord pos)
{
	if(welt->lookup(pos)) {
		grund_t *gr = welt->lookup(pos)->gib_kartenboden();
		if(gr) {
			blockmanager::gib_manager()->pruefe_blockstrecke(welt, gr->gib_pos());
		}
		return true;
	}
	return false;
}



int wkz_electrify_block(spieler_t *sp, karte_t *welt, koord pos)
{
	if(welt->lookup(pos)) {
		grund_t *gr = welt->lookup(pos)->gib_kartenboden();
		if(gr && gr->gib_besitzer() == sp) {
			blockmanager::gib_manager()->setze_tracktyp(welt, gr->gib_pos(),true);
		}
		return true;
	}
	return false;
}



/**
 * found a new city
 * @author Hj. Malthaner
 */
int wkz_add_city(spieler_t *sp, karte_t *welt, koord pos)
{
	bool ok = false;

	if(welt->ist_in_kartengrenzen(pos)) {
		grund_t *gr = welt->lookup(pos)->gib_kartenboden();

		if(gr->ist_natur() &&
			!gr->ist_wasser() &&
			gr->gib_grund_hang() == 0) {

			ding_t *d = gr->obj_bei(0);
			gebaeude_t *gb = dynamic_cast<gebaeude_t *>(d);

			if(gb && gb->ist_rathaus()) {
dbg->warning("wkz_add_city()", "Already a city here");
			}
			else {

				// Hajo: if city is owned by player and player removes special
				// buildings the game crashes. To avoid this problem cities
				// always belong to player 1

				int citizens=(int)(welt->gib_einstellungen()->gib_mittlere_einwohnerzahl()*0.9);
				//  stadt_t *stadt = new stadt_t(welt, welt->gib_spieler(1), pos,citizens/10+simrand(2*citizens+1));

				// always start with 1/10 citicens
				stadt_t *stadt = new stadt_t(welt, welt->gib_spieler(1), pos,citizens/10);

				stadt->laden_abschliessen();
				welt->add_stadt(stadt);

				sp->buche(umgebung_t::cst_found_city, pos, COST_CONSTRUCTION);
				ok =  true;
			}
		}
	}

	// update an open map
	reliefkarte_t::gib_karte()->set_mode(-1);
	return ok;
}



/**
 * Create an articial slope
 * @param param the slope type
 * @author Hj. Malthaner
 */
int wkz_set_slope(spieler_t * sp, karte_t *welt, koord pos, value_t lParam)
{
	const sint8 new_slope = lParam.i;
	bool ok = false;

	if(welt->ist_in_kartengrenzen(pos)) {

		grund_t * gr1 = welt->lookup(pos)->gib_kartenboden();

		// at least a pixel away from the border?
		if(welt->min_hgt(pos)<welt->gib_grundwasser()  ||  !welt->ist_in_kartengrenzen(pos+koord(1,1))  ||  !welt->ist_in_kartengrenzen(pos+koord(-1,-1))) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Maximum tile height difference reached."), w_autodelete);
			return false;
		}

		// already covered ... ?
		if(gr1->get_flag(grund_t::is_cover_tile)) {
			if(new_slope==RESTORE_SLOPE  &&  gr1->kann_alle_obj_entfernen(sp)) {
				planquadrat_t *plan=welt->access(pos);
				plan->boden_entfernen( plan->gib_boden_bei(1) );
				sp->buche(umgebung_t::cst_alter_land, pos, COST_CONSTRUCTION);
				return true;
			}
			else {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
				return false;
			}
		}

		// special slope: 0 => makes a cover tile if possible
		if(new_slope==0) {
			planquadrat_t *plan=welt->access(pos);
			if(gr1->gib_grund_hang()!=0  ||  welt->max_hgt(pos)<=gr1->gib_hoehe()) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Cannot cover this tile."), w_autodelete);
				return false;
			}
			plan->kartenboden_insert( new boden_t(welt,koord3d(pos,welt->max_hgt(pos))) );
			sp->buche(umgebung_t::cst_alter_land, pos, COST_CONSTRUCTION);
			return true;
		}

		// finally: empty
		if(gr1->suche_obj(ding_t::gebaeude)!=NULL  ||  ((!gr1->ist_wasser())  &&  gr1->hat_wege())) {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
			return false;
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
			if(new_slope==ALL_DOWN_SLOPE  &&  welt->get_slope(pos)>0) {
				// is more intiutive: if there is a slope, first downgrade it
				change_to_slope = 0;
			}
			slope_this = (change_to_slope>=ALL_UP_SLOPE) ? 0 : change_to_slope;
			new_pos = gr1->gib_pos() + koord3d(0,0,(change_to_slope==ALL_UP_SLOPE?16:(change_to_slope==ALL_DOWN_SLOPE?-16:0)));
#ifdef DOUBLE_GROUNDS
			// if already the same, double the slope
			if(slope_this==welt->get_slope(pos)) {
				slope_this *= 2;
			}
#endif
		}

		// check, if action is valid ...
		const sint16 hgt=new_pos.z/16;

		// first left side
		const grund_t *grleft=welt->lookup(pos+koord(-1,0))->gib_kartenboden();
		if(grleft) {
			const sint16 left_hgt=grleft->gib_hoehe()/16;
			const sint8 slope=welt->get_slope(pos+koord(-1,0));

#ifndef DOUBLE_GROUNDS
			const sint8 diff_from_ground_1 = left_hgt+((slope/2)%2)-hgt;
			const sint8 diff_from_ground_2 = left_hgt+((slope/4)%2)-hgt;
#else
			const sint8 diff_from_ground_1 = left_hgt+((slope/3)%3)-hgt;
			const sint8 diff_from_ground_2 = left_hgt+((slope/9)%3)-hgt;
#endif
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Maximum tile height difference reached."), w_autodelete);
				return false;
			}
		}

		// right side
		const grund_t *grright=welt->lookup(pos+koord(1,0))->gib_kartenboden();
		if(grright) {
			const sint16 right_hgt=grright->gib_hoehe()/16;
#ifndef DOUBLE_GROUNDS
			const sint8 diff_from_ground_1 = hgt+((slope_this/2)%2)-right_hgt;
			const sint8 diff_from_ground_2 = hgt+((slope_this/4)%2)-right_hgt;
#else
			const sint8 diff_from_ground_1 = hgt+((slope_this/3)%3)-right_hgt;
			const sint8 diff_from_ground_2 = hgt+((slope_this/9)%3)-right_hgt;
#endif
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Maximum tile height difference reached."), w_autodelete);
				return false;
			}
		}

		const grund_t *grback=welt->lookup(pos+koord(0,-1))->gib_kartenboden();
		if(grback) {
			const sint16 back_hgt=grback->gib_hoehe()/16;
			const sint8 slope=welt->get_slope(pos+koord(0,-1));

#ifndef DOUBLE_GROUNDS
			const sint8 diff_from_ground_1 = back_hgt+(slope%2)-hgt;
			const sint8 diff_from_ground_2 = back_hgt+((slope/2)%2)-hgt;
#else
			const sint8 diff_from_ground_1 = back_hgt+(slope%3)-hgt;
			const sint8 diff_from_ground_2 = back_hgt+((slope/3)%3)-hgt;
#endif
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Maximum tile height difference reached."), w_autodelete);
				return false;
			}
		}

		const grund_t *grfront=welt->lookup(pos+koord(0,1))->gib_kartenboden();
		if(grfront) {
			const sint16 front_hgt=grfront->gib_hoehe()/16;
#ifndef DOUBLE_GROUNDS
			const sint8 diff_from_ground_1 = hgt+(slope_this%2)-front_hgt;
			const sint8 diff_from_ground_2 = hgt+((slope_this/2)%2)-front_hgt;
#else
			const sint8 diff_from_ground_1 = hgt+(slope_this%3)-front_hgt;
			const sint8 diff_from_ground_2 = hgt+((slope_this/3)%3)-front_hgt;
#endif
			if(diff_from_ground_1>2  ||  diff_from_ground_2>2) {
				create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Maximum tile height difference reached."), w_autodelete);
				return false;
			}
		}

		// ok, now we set the slope ...
		ok = (new_pos!=gr1->gib_pos());
		ok |= slope_this!=gr1->gib_grund_hang();

		if(ok) {
			// ok, was sucess
			if(!gr1->ist_wasser()  &&  slope_this==0  &&  new_pos.z==welt->gib_grundwasser()) {
				// now water
				welt->access(pos)->kartenboden_setzen( new wasser_t(welt,pos), true );
			}
			else if(gr1->ist_wasser()  &&  (new_pos.z>welt->gib_grundwasser()  ||  slope_this!=0)) {
				welt->access(pos)->kartenboden_setzen( new boden_t(welt,new_pos), true );
				welt->set_slope(pos, slope_this);
			}
			else {
				welt->set_slope(pos, slope_this);
				gr1->setze_pos(new_pos);
			}
			// recalc slope walls on neightbours
			for(int y=-1; y<=1; y++) {
				for(int x=-1; x<=1; x++) {
					grund_t *gr = welt->lookup(pos+koord(x,y))->gib_kartenboden();
					gr->calc_bild();
				}
			}
			sp->buche(new_slope==RESTORE_SLOPE?umgebung_t::cst_alter_land:umgebung_t::cst_set_slope, pos, COST_CONSTRUCTION);
		}

	}
	return ok;
}


/**
 * Plant a tree
 * @author Hj. Malthaner
 */
int wkz_pflanze_baum(spieler_t *, karte_t *welt, koord pos)
{
  DBG_MESSAGE("wkz_pflanze_baum()",
         "called on %d,%d", pos.x, pos.y);

  baum_t::plant_tree_on_coordinate(welt,
           pos,
           10);

  return true;
}


// --- Hajo: below are inofficial and/or testing tools


/* Hajo: unused
#include "simverkehr.h"
int wkz_test(spieler_t *, karte_t *welt, koord pos)
{
	if(welt->ist_in_kartengrenzen(pos)) {
		verkehrsteilnehmer_t *vt = new verkehrsteilnehmer_t(welt, pos);
		welt->lookup(pos)->gib_boden()->obj_add( vt );
		welt->sync_add( vt );
	}
	return true;
}
*/



/* builts a random industry chain, either in the next town */
int wkz_build_industries_land(spieler_t *, karte_t *welt, koord pos)
{
	const fabrik_besch_t *info = fabrikbauer_t::get_random_consumer(false);

	koord size = info->gib_haus()->gib_groesse();
	int rotation = 0;

	bool hat_platz = false;
	bool slope_check = size.x+size.y>2;

	if(welt->ist_platz_frei(pos, size.x, size.y, NULL, slope_check)) {
		hat_platz = true;
	}

	if(!hat_platz &&  size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, slope_check)) {
		rotation = 1;
		hat_platz = true;
	}

	if(hat_platz) {
		koord3d k = welt->lookup(pos)->gib_kartenboden()->gib_pos();
		fabrikbauer_t::baue_hierarchie(welt, NULL, info, rotation, &k,welt->gib_spieler(1));

		// crossconnect all?
		if(umgebung_t::crossconnect_factories) {
			const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
			slist_iterator_tpl <fabrik_t *> iter (list);
			while( iter.next() ) {
				iter.get_current()->add_all_suppliers();
			}
		}

		return true;
	}

	return false;
}


/* builts a random industry chain, either in the next town */
int wkz_build_industries_city(spieler_t *, karte_t *welt, koord pos)
{
	const planquadrat_t *plan = welt->lookup(pos);
	if(plan) {

		const fabrik_besch_t *info = fabrikbauer_t::get_random_consumer(true);
		koord3d pos3d = plan->gib_kartenboden()->gib_pos();
		fabrikbauer_t::baue_hierarchie(welt, NULL, info, false, &pos3d,welt->gib_spieler(1));

		// crossconnect all?
		if(umgebung_t::crossconnect_factories) {
			const slist_tpl<fabrik_t *> & list = welt->gib_fab_list();
			slist_iterator_tpl <fabrik_t *> iter (list);
			while( iter.next() ) {
				iter.get_current()->add_all_suppliers();
			}
		}

	}
	return plan != 0;
}


/* open the list of halt */
int wkz_list_halt_tool(spieler_t *sp, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(-1, -1, -1, new halt_list_frame_t(sp), w_autodelete, magic_halt_list_t);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}


/* open the list of vehicle */
int wkz_list_vehicle_tool(spieler_t *sp, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(-1, -1, -1, new convoi_frame_t(sp, welt), w_autodelete, magic_convoi_t);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}


/* open the list of towns */
int wkz_list_town_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(0, 0, new citylist_frame_t(welt), w_info);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}



/* open the list of goods */
int wkz_list_good_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {//see simworld.cc, karte_t::setze_maus_funktion
		create_win(0, 0,new goods_frame_t(welt), w_autodelete);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}



/* open the list of factories */
int wkz_list_factory_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {
		create_win(0, 0,new factorylist_frame_t(welt), w_autodelete);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}


/* open the list of attraction */
int wkz_list_curiosity_tool(spieler_t *, karte_t *welt,koord k)
{
	if(k == INIT) {
		create_win(0, 0,new curiositylist_frame_t(welt), w_autodelete);
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}


/* prissi: undo building */
int wkz_undo(spieler_t *sp, karte_t *welt)
{
	if(!sp->undo()) {
		create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, translator::translate("UNDO failed!")), w_autodelete);
	}
	return false;
}



// Werkzeuge ende



/* builds company headquarter
 * @author prissi
 */
int wkz_headquarter(spieler_t *sp, karte_t *welt, koord pos)
{
	bool ok=false;

	if(pos==INIT  ||  pos==EXIT) {
		return true;
	}
DBG_MESSAGE("wkz_headquarter()", "building headquarter at (%d,%d)", pos.x, pos.y);

	if(welt->ist_in_kartengrenzen(pos)) {

		int level = sp->get_headquarter_level();
		int besch_nr=-1;
		koord previous = sp->get_headquarter_pos();

		for(unsigned i=0;  i<hausbauer_t::headquarter.count();  i++  ) {
			if(hausbauer_t::headquarter.at(i)->gib_bauzeit()==level) {
				besch_nr = i;
				break;
			}
		}

		if(besch_nr<0) {
			// no further headquarter level
			welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
			return false;
		}

		const haus_besch_t *besch = hausbauer_t::headquarter.at(besch_nr);
		koord size = besch->gib_groesse();
		int rotate = 0;

		if(welt->ist_platz_frei(pos, size.x, size.y, NULL, false)) {
			ok = true;
		}
		if(size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, false)) {
			rotate = 1;
			ok = true;
		}

		if(ok) {
			// remove previous one
			if(previous!=koord::invalid) {
				wkz_remover(sp,welt,previous);
			}
			// then built is
			hausbauer_t::baue(welt, sp, welt->lookup(pos)->gib_kartenboden()->gib_pos(), rotate, besch, true, NULL);
			sp->add_headquarter( level+1, pos );
			sp->buche(umgebung_t::cst_multiply_headquarter*besch->gib_level()*besch->gib_b()*besch->gib_h(), pos, COST_CONSTRUCTION * size.x * size.y);
		}
		else {
			create_win(-1, -1, MESG_WAIT, new nachrichtenfenster_t(welt, "Tile not empty."), w_autodelete);
		}
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return ok;
}



/* switch to next player
 * @author prissi
 */
int wkz_switch_player(spieler_t *, karte_t *welt, koord pos)
{
	if(pos==INIT) {
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
		welt->switch_active_player();
	}
	return false;
}



/* change city size
 * @author prissi
 */
int wkz_grow_city(spieler_t *, karte_t *welt, koord pos, value_t lParam)
{
	stadt_t *city = welt->suche_naechste_stadt(pos);

	if(pos!=INIT  && pos!=EXIT  &&  city!=NULL) {
		city->change_size( lParam.i );
	}
	return true;
}


/* built random tourist attraction
 * @author prissi
 */
int wkz_add_attraction(spieler_t *, karte_t *welt, koord pos)
{
	if(pos!=INIT  && pos!=EXIT) {
		const haus_besch_t *attraction=hausbauer_t::waehle_sehenswuerdigkeit(welt->get_timeline_year_month());

		if(attraction==NULL) {
			return false;
		}

		koord size = attraction->gib_groesse();
		int rotation = 0;

		bool hat_platz = false;
		bool slope_check = size.x+size.y>2;

		if(welt->ist_platz_frei(pos, size.x, size.y, NULL, slope_check)) {
			hat_platz = true;
		}

		if(!hat_platz &&  size.y != size.x && welt->ist_platz_frei(pos, size.y, size.x, NULL, slope_check)) {
			rotation = 1;
			hat_platz = true;
		}

		// Platz gefunden ...
		if(hat_platz) {
			hausbauer_t::baue(welt, welt->gib_spieler(1), welt->lookup(pos)->gib_kartenboden()->gib_pos(), rotation, attraction);
			return true;
		}
	}
	return false;
}



// protects map from further change
int wkz_lock( spieler_t *, karte_t *welt, koord pos)
{
	if(pos!=INIT  && pos!=EXIT) {
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
		welt->gib_einstellungen()->setze_allow_player_change( false );
		destroy_all_win();
		welt->switch_active_player();
	}
	return false;
}



// setp one year forward
int wkz_step_year( spieler_t *, karte_t *welt, koord k)
{
	if(k == INIT) {
		welt->step_year();
		welt->setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), welt->Z_PLAN,  NO_SOUND, NO_SOUND );
	}
	return false;
}
