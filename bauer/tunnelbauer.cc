/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <algorithm>
#include <stdio.h>

#include "../simdebug.h"

#include "tunnelbauer.h"
#include "../besch/intro_dates.h"

#include "../gui/karte.h"

#include "../simworld.h"
#include "../simwin.h"
#include "../simplay.h"
#include "../simskin.h"
#include "../simwerkz.h"
#include "../simevent.h"

#include "../besch/tunnel_besch.h"

#include "../boden/boden.h"
#include "../boden/tunnelboden.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/kanal.h"
#include "../boden/wege/strasse.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/koord3d.h"

#include "../dings/tunnel.h"
#include "../dings/signal.h"
#include "../dings/zeiger.h"

#include "../gui/messagebox.h"
#include "../gui/werkzeug_parameter_waehler.h"

#include "wegbauer.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/vector_tpl.h"


static vector_tpl<tunnel_besch_t*> tunnel(16);
static stringhashtable_tpl<tunnel_besch_t *> tunnel_by_name;



void
tunnelbauer_t::register_besch(tunnel_besch_t *besch)
{
	tunnel_by_name.put(besch->gib_name(), besch);
	tunnel.push_back(besch);
}



// now we have to convert old tunnel to new ones ...
bool
tunnelbauer_t::laden_erfolgreich()
{
	for (vector_tpl<tunnel_besch_t*>::const_iterator i = tunnel.begin(), end = tunnel.end(); i != end; ++i) {
		tunnel_besch_t* besch = *i;
		if(besch->gib_topspeed()==0) {
			// old style, need to convert
			if(strcmp(besch->gib_name(),"RoadTunnel")==0) {
				besch->wegtyp = (uint8)road_wt;
				besch->topspeed = 120;
				besch->maintenance = 500;
				besch->preis = 200000;
				besch->intro_date = DEFAULT_INTRO_DATE*12;
				besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
			}
			else {
				besch->wegtyp = (uint8)track_wt;
				besch->topspeed = 280;
				besch->maintenance = 500;
				besch->preis = 200000;
				besch->intro_date = DEFAULT_INTRO_DATE*12;
				besch->obsolete_date = DEFAULT_RETIRE_DATE*12;
			}
		}
	}
	return true;
}



const tunnel_besch_t *
tunnelbauer_t::gib_besch(const char *name)
{
  return tunnel_by_name.get(name);
}



/**
 * Find a matchin tunnel
 * @author Hj. Malthaner
 */
const tunnel_besch_t *
tunnelbauer_t::find_tunnel(const waytype_t wtyp, const uint32 min_speed,const uint16 time)
{
	const tunnel_besch_t *find_besch=NULL;

	for (vector_tpl<tunnel_besch_t*>::const_iterator i = tunnel.begin(), end = tunnel.end(); i != end; ++i) {
		const tunnel_besch_t* besch = *i;
		if(besch->gib_waytype() == wtyp) {
			if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {
				if(find_besch==NULL  ||
					(find_besch->gib_topspeed()<min_speed  &&  find_besch->gib_topspeed()<besch->gib_topspeed())  ||
					(besch->gib_topspeed()>=min_speed  &&  besch->gib_wartung()<find_besch->gib_wartung())
				) {
					find_besch = besch;
				}
			}
		}
	}
	return find_besch;
}


static bool compare_tunnels(const tunnel_besch_t* a, const tunnel_besch_t* b)
{
	int cmp = a->gib_topspeed() - b->gib_topspeed();
	if(cmp==0) {
		cmp = (int)a->get_intro_year_month() - (int)b->get_intro_year_month();
	}
	if(cmp==0) {
		cmp = strcmp(a->gib_name(), b->gib_name());
	}
	return cmp<0;
}


/**
 * Fill menu with icons of given waytype
 * @author Hj. Malthaner
 */
void tunnelbauer_t::fill_menu(werkzeug_parameter_waehler_t* wzw, const waytype_t wtyp, const int sound_ok, const int sound_ko, const karte_t* welt)
{
	const uint16 time=welt->get_timeline_year_month();
	vector_tpl<const tunnel_besch_t*> matching;
	for (vector_tpl<tunnel_besch_t*>::const_iterator i = tunnel.begin(), end = tunnel.end(); i != end; ++i) {
		const tunnel_besch_t* besch = *i;
		if (besch->gib_waytype() == wtyp && (
					time == 0 ||
					(besch->get_intro_year_month() <= time && time < besch->get_retire_year_month())
				)) {
			matching.push_back(besch);
		}
	}
	std::sort(matching.begin(), matching.end(), compare_tunnels);

	const sint32 shift_maintanance = (welt->ticks_bits_per_tag-18);	// same costs per intervall => correct display

	// now sorted ...
	for (vector_tpl<const tunnel_besch_t*>::const_iterator i = matching.begin(), end = matching.end(); i != end; ++i) {
		const tunnel_besch_t* besch = *i;
		char buf[256];

		sprintf(buf, "%s, %d$ (%d$), %dkm/h",
			  translator::translate(besch->gib_name()),
			  besch->gib_preis()/100,
			  (besch->gib_wartung()<<shift_maintanance)/100,
			  besch->gib_topspeed());

		wzw->add_param_tool(baue,
			  (const void*) besch,
			  karte_t::Z_PLAN,
			  sound_ok,
			  sound_ko,
			  besch->gib_cursor()->gib_bild_nr(1),
			  besch->gib_cursor()->gib_bild_nr(0),
			  cstring_t(buf));
	}
}



/* now construction stuff */


koord3d
tunnelbauer_t::finde_ende(karte_t *welt, koord3d pos, koord zv, waytype_t wegtyp)
{
	const grund_t *gr;

	while(true) {
		pos = pos + zv;
		if(!welt->ist_in_kartengrenzen(pos.gib_2d())) {
			return koord3d::invalid;
		}
		gr = welt->lookup(pos);

		if(gr) {
			if(gr->ist_tunnel()) {  // Anderer Tunnel l�uft quer
				return koord3d::invalid;
			}
			ribi_t::ribi ribi = gr->gib_weg_ribi_unmasked(wegtyp);

			if(ribi && koord(ribi) == zv) {
				// Ende am Hang - Endschiene vorhanden
				return pos;
			}
			if(!ribi && gr->gib_grund_hang() == hang_typ(-zv)) {
				// Ende am Hang - Endschiene fehlt oder hat keine ribis
				// Wir pr�fen noch, ob uns dort ein anderer Weg st�rt
				if(!gr->hat_wege() || gr->hat_weg(wegtyp)) {
					return pos;
				}
			}
			return koord3d::invalid;  // Was im Weg (schr�ger Hang oder so)
		}
		// Alles frei - weitersuchen
	}
}



int
tunnelbauer_t::baue(spieler_t *sp, karte_t *welt, koord pos, value_t param)
{
	static koord3d start=koord3d::invalid;
	static zeiger_t *wkz_tunnelbau_bauer = NULL;

	const tunnel_besch_t *besch = (const tunnel_besch_t *)param.p;
	if(!besch) {
DBG_MESSAGE("brueckenbauer_t::baue()", "no description for tunnel type");
		return false;
	}
DBG_MESSAGE("tunnelbauer_t::baue()", "called on %d,%d", pos.x, pos.y);
	if(!welt->ist_in_kartengrenzen(pos)) {
		return false;
	}

	// remove bulldozer?
	if(start!=koord3d::invalid  &&  (pos==INIT || pos == EXIT  ||  !grund_t::underground_mode)) {
		if(wkz_tunnelbau_bauer != NULL) {
			delete wkz_tunnelbau_bauer;
			wkz_tunnelbau_bauer = NULL;
		}
		start = koord3d::invalid;
		if(grund_t::underground_mode) {
			return true;
		}
	}

	// in underground mode, new tunnel can be made ...
	if(grund_t::underground_mode) {
		// search for ground
		// start needs valid tile!
		if(start==koord3d::invalid) {
			const planquadrat_t *plan=welt->lookup(pos);
			grund_t *gr=NULL;
			for (uint i = 0; i < plan->gib_boden_count(); i++) {
				if(plan->gib_boden_bei(i)->gib_typ()==grund_t::tunnelboden) {
					if(spieler_t::check_owner( sp, plan->gib_boden_bei(i)->obj_bei(0)->gib_besitzer())) {
						gr = plan->gib_boden_bei(i);
						break;
					}
				}
			}
			if(gr==NULL) {
				return false;
			}
			start = gr->gib_pos();
			// move bulldozer to start ...
			wkz_tunnelbau_bauer = new zeiger_t(welt, start, sp);
			wkz_tunnelbau_bauer->setze_bild( skinverwaltung_t::bauigelsymbol->gib_bild_nr(0));
			gr->obj_add(wkz_tunnelbau_bauer);
			return true;
		}
		else {
			// we have a start, now just try to built it ...
			delete wkz_tunnelbau_bauer;

			int bt;
			switch(besch->gib_waytype()) {
				case track_wt:
					bt = (wegbauer_t::schiene|wegbauer_t::tunnel_flag);
					break;
				case road_wt:
					bt = (wegbauer_t::strasse|wegbauer_t::tunnel_flag);
					break;
				case monorail_wt:
					bt = (wegbauer_t::monorail|wegbauer_t::tunnel_flag);
					break;
				case water_wt:
					bt = (wegbauer_t::wasser|wegbauer_t::tunnel_flag);
					break;
				default: bt = 0; break;
			}
			weg_t *w=weg_t::alloc(besch->gib_waytype());
			const weg_besch_t *wb=w->gib_besch();
			delete w;
			// now try construction
			wegbauer_t bauigel(welt, sp);
			bauigel.route_fuer((wegbauer_t::bautyp_t)bt, wb, besch);
			bauigel.set_keep_existing_ways(false);
			bauigel.calc_straight_route(start,koord3d(pos,start.z));
			welt->mute_sound(true);
			bauigel.baue();
			welt->mute_sound(false);
		}
		start = koord3d::invalid;
		return true;
	}
	else {

		const grund_t *gr = welt->lookup(pos)->gib_kartenboden();
		koord zv;
		const waytype_t wegtyp = besch->gib_waytype();
		const weg_t *weg = gr->gib_weg(wegtyp);

		if(!weg || gr->gib_typ() != grund_t::boden) {
			if(welt->get_active_player()==sp) {
				create_win( new news_img("Tunnel must start on single way!"), w_time_delete, magic_none);
			}
			return false;
		}
		if(!hang_t::ist_einfach(gr->gib_grund_hang())) {
			create_win( new news_img("Tunnel muss an\neinfachem\nHang beginnen!\n"), w_time_delete, magic_none);
			return false;
		}
		if(weg->gib_ribi_unmasked() & ~ribi_t::rueckwaerts(ribi_typ(gr->gib_grund_hang()))) {
			create_win( new news_img("Tunnel must end on single way!"), w_time_delete, magic_none);
			return false;
		}
		zv = koord(gr->gib_grund_hang());

		// Tunnelende suchen
		koord3d end = koord3d::invalid;
		if(event_get_last_control_shift()!=2) {
			end = finde_ende(welt, gr->gib_pos(), zv, wegtyp);
		} else {
			end = gr->gib_pos()+zv;
			if(welt->lookup(end)  ||  welt->lookup_kartenboden(pos+zv)->gib_hoehe()<=end.z) {
				end = koord3d::invalid;
			}
		}

		// pruefe ob Tunnel auf strasse/schiene endet
		if(!welt->ist_in_kartengrenzen(end.gib_2d())) {
			create_win( new news_img("Tunnel must end on single way!"), w_time_delete, magic_none);
			return false;
		}

		// Anfang und ende sind geprueft, wir konnen endlich bauen
		return baue_tunnel(welt, sp, gr->gib_pos(), end, zv, besch);
	}
}



bool tunnelbauer_t::baue_tunnel(karte_t *welt, spieler_t *sp, koord3d start, koord3d end, koord zv, const tunnel_besch_t *besch)
{
	ribi_t::ribi ribi;
	weg_t *weg;
	koord3d pos = start;
	int cost = 0;
	const weg_besch_t *weg_besch;
	waytype_t wegtyp = besch->gib_waytype();

DBG_MESSAGE("tunnelbauer_t::baue()","build from (%d,%d,%d) to (%d,%d,%d) ", pos.x, pos.y, pos.z, end.x, end.y, end.z );

	// get a way for enty/exit
	weg = welt->lookup(start)->gib_weg(wegtyp);
	if(weg==NULL) {
		weg = welt->lookup(end)->gib_weg(wegtyp);
	}
	if(weg==NULL) {
		dbg->error("tunnelbauer_t::baue_tunnel()","No way found!");
		return false;
	}
	weg_besch = weg->gib_besch();

	baue_einfahrt(welt, sp, pos, zv, besch, weg_besch, cost);

	ribi = welt->lookup(pos)->gib_weg_ribi_unmasked(wegtyp);
	// don't move on to next tile if only one tile long
	if(end!=start) pos = pos + zv;

	// Now we build the invisible part
	while(pos!=end) {
		assert(pos.gib_2d()!=end.gib_2d());
		tunnelboden_t *tunnel = new tunnelboden_t(welt, pos, 0);
		// use the fastest way
		weg = weg_t::alloc(besch->gib_waytype());
		weg->setze_besch(weg_besch);
		weg->setze_max_speed(besch->gib_topspeed());
		welt->access(pos.gib_2d())->boden_hinzufuegen(tunnel);
		tunnel->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
		tunnel->obj_add(new tunnel_t(welt, pos, sp, besch));
		assert(!tunnel->ist_karten_boden());
		spieler_t::add_maintenance( sp,  -weg->gib_besch()->gib_wartung() );
		spieler_t::add_maintenance( sp,  besch->gib_wartung() );
		cost += besch->gib_preis();
		pos = pos + zv;
	}

	// if end is above ground construct an exit
	if(welt->lookup(end.gib_2d())->gib_kartenboden()->gib_pos().z==end.z) {
		baue_einfahrt(welt, sp, pos, -zv, besch, weg_besch, cost);
	} else {
		tunnelboden_t *tunnel = new tunnelboden_t(welt, pos, 0);
		// use the fastest way
		weg = weg_t::alloc(besch->gib_waytype());
		weg->setze_besch(weg_besch);
		weg->setze_max_speed(besch->gib_topspeed());
		welt->access(pos.gib_2d())->boden_hinzufuegen(tunnel);
		tunnel->neuen_weg_bauen(weg, ribi_t::doppelt(ribi), sp);
		tunnel->obj_add(new tunnel_t(welt, pos, sp, besch));
		assert(!tunnel->ist_karten_boden());
		spieler_t::add_maintenance( sp,  -weg->gib_besch()->gib_wartung() );
		spieler_t::add_maintenance( sp,  besch->gib_wartung() );
		cost += besch->gib_preis();
	}

	spieler_t::accounting(sp, -cost, start.gib_2d(), COST_CONSTRUCTION);
	return true;
}



void
tunnelbauer_t::baue_einfahrt(karte_t *welt, spieler_t *sp, koord3d end, koord zv, const tunnel_besch_t *besch, const weg_besch_t *weg_besch, int &cost)
{
	grund_t *alter_boden = welt->lookup(end);
	ribi_t::ribi ribi = alter_boden->gib_weg_ribi_unmasked(besch->gib_waytype()) | ribi_typ(zv);

	tunnelboden_t *tunnel = new tunnelboden_t(welt, end, alter_boden->gib_grund_hang());
	tunnel->obj_add(new tunnel_t(welt, end, sp, besch));

	weg_t *weg=alter_boden->gib_weg( besch->gib_waytype() );
	// take care of everything on that tile ...
	tunnel->take_obj_from( alter_boden );
	welt->access(end.gib_2d())->kartenboden_setzen( tunnel );
	if(weg) {
		// has already a way
		tunnel->weg_erweitern(besch->gib_waytype(), ribi);
	}
	else {
		// needs still one
		weg = weg_t::alloc( besch->gib_waytype() );
		tunnel->neuen_weg_bauen( weg, ribi, sp );
	}
	weg->setze_max_speed( besch->gib_topspeed() );
	tunnel->calc_bild();

	if(sp!=NULL) {
		spieler_t::add_maintenance( sp,  -weg_besch->gib_wartung() );
		spieler_t::add_maintenance( sp,  besch->gib_wartung() );
	}
	cost += besch->gib_preis();
}




const char *
tunnelbauer_t::remove(karte_t *welt, spieler_t *sp, koord3d start, waytype_t wegtyp)
{
	marker_t    marker(welt->gib_groesse_x(),welt->gib_groesse_y());
	slist_tpl<koord3d>  end_list;
	slist_tpl<koord3d>  part_list;
	slist_tpl<koord3d>  tmp_list;
	const char    *msg;
	koord3d   pos = start;

	// Erstmal das ganze Au�ma� des Tunnels bestimmen und sehen,
	// ob uns was im Weg ist.
	tmp_list.insert(pos);
	marker.markiere(welt->lookup(pos));

	do {
		pos = tmp_list.remove_first();

		// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
		grund_t *from = welt->lookup(pos);
		grund_t *to;
		koord zv = koord::invalid;

		if(from->ist_karten_boden()) {
			// Der Grund ist Tunnelanfang/-ende - hier darf nur in
			// eine Richtung getestet werden.
			zv = koord(from->gib_grund_hang());
			end_list.insert(pos);
		}
		else {
			part_list.insert(pos);
		}
		// Alle Tunnelteile auf Entfernbarkeit pr�fen!
		msg = from->kann_alle_obj_entfernen(sp);

		if(msg != NULL) {
		return "Der Tunnel ist nicht frei!\n";
		}
		// Nachbarn raussuchen
		for(int r = 0; r < 4; r++) {
			if((zv == koord::invalid || zv == koord::nsow[r]) &&
				from->get_neighbour(to, wegtyp, koord::nsow[r]) &&
				!marker.ist_markiert(to))
			{
				tmp_list.insert(to->gib_pos());
				marker.markiere(to);
			}
		}
	} while (!tmp_list.empty());

	assert(!end_list.empty());

	// Jetzt geht es ans l�schen der Tunnel
	while (!part_list.empty()) {
		pos = part_list.remove_first();
		grund_t *gr = welt->lookup(pos);
		gr->remove_everything_from_way(sp,wegtyp,ribi_t::keine);	// removes stop and signals correctly
		// we may have a second way here ...
		gr->obj_loesche_alle(sp);
		welt->access(pos.gib_2d())->boden_entfernen(gr);
		reliefkarte_t::gib_karte()->calc_map_pixel( pos.gib_2d() );
		delete gr;
	}

	// Und die Tunnelenden am Schlu�
	while (!end_list.empty()) {
		pos = end_list.remove_first();

		grund_t *gr = welt->lookup(pos);
		ribi_t::ribi ribi = gr->gib_weg_ribi_unmasked(wegtyp);
		if(gr->gib_grund_hang()!=hang_t::flach) {
			ribi &= ~ribi_typ(gr->gib_grund_hang());
		}
		else {
			ribi &= ~ribi_typ(hang_t::gegenueber(gr->gib_weg_hang()));
		}

		// removes single signals, bridge head, pedestrians, stops, changes catenary etc
		gr->remove_everything_from_way(sp,wegtyp,ribi);	// removes stop and signals correctly

		// remove tunnel portals
		tunnel_t *t = gr->find<tunnel_t>();
		if(t) {
			t->entferne(sp);
			delete t;
		}

		// corrects the ways
		weg_t *weg=gr->gib_weg_nr(0);
		if(weg) {
			// fails if it was preivously the last ribi
			weg->setze_besch(weg->gib_besch());
			weg->setze_ribi( ribi );
			if(gr->gib_weg_nr(1)) {
				gr->gib_weg_nr(1)->setze_ribi( ribi );
			}
		}

		// then add the new ground, copy everything and replace the old one
		grund_t *gr_new = new boden_t(welt, pos, gr->gib_grund_hang());
		gr_new->take_obj_from( gr );
		welt->access(pos.gib_2d())->kartenboden_setzen(gr_new );
	}
	return NULL;
}
