/*
 * simhalt.cc
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* simhalt.cc
 *
 * Haltestellen fuer Simutrans
 * 03.2000 getrennt von simfab.cc
 *
 * Hj. Malthaner
 */
#ifdef _MSC_VER
#include <string.h>
#endif
#include "simdebug.h"
#include "simmem.h"
#include "boden/grund.h"
#include "boden/wege/weg.h"
#include "simhalt.h"
#include "simfab.h"
#include "simconvoi.h"
#include "simwin.h"
#include "simworld.h"
#include "simintr.h"
#include "simpeople.h"
#include "simtime.h"

#include "simcolor.h"
#include "simgraph.h"

#include "gui/halt_info.h"
#include "gui/halt_detail.h"
#include "dings/gebaeude.h"
#ifdef LAGER_NOT_IN_USE
#include "dings/lagerhaus.h"
#endif
#include "dataobj/fahrplan.h"
#include "dataobj/warenziel.h"
#include "dataobj/einstellungen.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"

#include "utils/tocstring.h"
#include "utils/simstring.h"

#include "besch/ware_besch.h"
#include "bauer/warenbauer.h"


/**
 * This variable defines by which column the table is sorted
 * Values: 1 = station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author hsiegeln
 */
haltestelle_t::sort_mode_t haltestelle_t::sortby = by_name;


/**
 * Max number of hops in route calculation
 * @author Hj. Malthaner
 */
static int max_hops = 300;


/**
 * Max number of transfers in route calculation
 * @author Hj. Malthaner
 */
static int max_transfers = 6;


/**
 * Sets max number of hops in route calculation
 * @author Hj. Malthaner
 */
void haltestelle_t::set_max_hops(int hops)
{
  if(hops > 9994) {
    hops = 9994;
  }

  max_hops = hops;
}


/**
 * Sets max number of transfers in route calculation
 * @author Hj. Malthaner
 */
void haltestelle_t::set_max_transfers(int transfers)
{
  max_transfers = transfers;
}


/** hsiegeln
 *  @param td1, td2: pointer to travel_details
 *  @return sortorder of the two passed elements; used in qsort
 *  @author hsiegeln
 *  @date 2003-11-02
 */
int haltestelle_t::compare_ware(const void *td1, const void *td2)
{
  const travel_details * const td1p = (const travel_details*)td1;
  const travel_details * const td2p = (const travel_details*)td2;

	int order = 0;
	halthandle_t halt1 = td1p->destination;
	halthandle_t halt2 = td2p->destination;
	halthandle_t via_halt1 = td1p->via_destination;
	halthandle_t via_halt2 = td2p->via_destination;
	ware_t ware1 = td1p->ware;
	ware_t ware2 = td2p->ware;

	switch (sortby) {
	case by_name: //sort by destination name
          order = strcmp(halt1->gib_name(), halt2->gib_name());
          break;
        case by_via: // sort by via_destination name
          order = strcmp(via_halt1->gib_name(), via_halt2->gib_name());
          // if the destination is different, bit the via_destination the same, sort it by the destination (2nd level sort)
          if (order == 0)
          {
          	order = strcmp(halt1->gib_name(), halt2->gib_name());
          }
          break;
        case by_amount: // sort by ware amount
          order = ware2.menge - ware1.menge;
          // if the same amount is transported, we sort by via_destination
          if (order == 0)
          {
          	order = strcmp(via_halt1->gib_name(), via_halt2->gib_name());
	        // if the same amount goes through the same via_destination, sort by destionation
	        if (order == 0)
          	{
          		order = strcmp(halt1->gib_name(), halt2->gib_name());
          	}
          }
          break;
        }

        return order;
}

// Helfer Klassen

class HNode {
public:
  halthandle_t halt;
  int depth;
  HNode *link;
};


// Klassenvariablen

slist_tpl<halthandle_t> haltestelle_t::alle_haltestellen;


// Klassenmethoden

halthandle_t
haltestelle_t::gib_halt(const karte_t *welt, const koord pos)
{
    const planquadrat_t *plan = welt->lookup(pos);

    if(plan) {
	const grund_t *gr = plan->gib_kartenboden();

	if(gr) {
	    // wir haben alle Daten, liefere halt
	    return gr->gib_halt();
	}
    }

    // not found; return unbound handle
    return halthandle_t();
}


halthandle_t
haltestelle_t::gib_halt(const karte_t *welt, const koord3d pos)
{
  const grund_t *gr = welt->lookup(pos);

  if(gr) {
    // wir haben alle Daten, liefere halt
    return gr->gib_halt();
  }

  // not found; return unbound handle
  return halthandle_t();
}


halthandle_t
haltestelle_t::gib_halt(const karte_t *welt, const koord * const pos)
{
    // mit allen checks
    // ist sicher im Aufruf!

    if(pos != NULL) {
	return gib_halt(welt, *pos);
    }

    // not found; return unbound handle
    return halthandle_t();
}


koord
haltestelle_t::gib_basis_pos() const
{
    if(!grund.is_empty()) {
	return grund.at(0)->gib_pos().gib_2d();
    } else {
	return koord::invalid;
    }
}


void haltestelle_t::init_gui()
{
    // Lazy init at opening!
    halt_info = NULL;

    // Lazy init of name. Done on first request fo name.
    need_name = true;
}


/**
 * Station factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
halthandle_t haltestelle_t::create(karte_t *welt, koord pos, spieler_t *sp)
{
    haltestelle_t * p = new haltestelle_t(welt, pos, sp);

    return p->self;
}


/**
 * Station factory method. Returns handles instead of pointers.
 * @author Hj. Malthaner
 */
halthandle_t haltestelle_t::create(karte_t *welt, loadsave_t *file)
{
    haltestelle_t * p = new haltestelle_t(welt, file);

    return p->self;
}


/**
 * Station destruction method.
 * @author Hj. Malthaner
 */
void haltestelle_t::destroy(halthandle_t &halt)
{
    haltestelle_t * p = halt.detach();
    delete p;
}


/**
 * Station destruction method.
 * Da destroy() alle_haltestellen modifiziert kann kein Iterator benutzt
 * werden! V. Meyer
 * @author Hj. Malthaner
 */
void haltestelle_t::destroy_all()
{
    while(alle_haltestellen.count() > 0) {
        halthandle_t halt = alle_haltestellen.at(0);
        destroy(halt);
    }
    alle_haltestellen.clear();
}


// Konstruktoren

haltestelle_t::haltestelle_t(karte_t *wl, loadsave_t *file) : self(this)
{
    alle_haltestellen.insert(self);

    welt = wl;
    marke = 0;

    pax_happy = 0;
    pax_unhappy = 0;
    pax_no_route = 0;

    // @author hsiegeln
    sortierung = haltestelle_t::by_name;

    rdwr(file);

    verbinde_fabriken();

    init_gui();
}


haltestelle_t::haltestelle_t(karte_t *wl, koord pos, spieler_t *sp) : self(this)
{
    alle_haltestellen.insert(self);

    welt = wl;
    marke = 0;

    this->pos = pos;
    besitzer_p = sp;
#ifdef LAGER_NOT_IN_USE
    lager = NULL;
#endif
    pax_enabled = false;
    post_enabled = false;
    ware_enabled = false;

    pax_happy = 0;
    pax_unhappy = 0;
    pax_no_route = 0;

    sortierung = haltestelle_t::by_name;
    init_financial_history();

    verbinde_fabriken();

    init_gui();
}


haltestelle_t::~haltestelle_t()
{
//    printf("Haltestelle %s (%p) durchl�uft Destruktor\n", gib_name(), this);

    assert(grund.count() == 0);

    if(halt_info) {
	destroy_win(halt_info);
	delete halt_info;
	halt_info = 0;
    }
    alle_haltestellen.remove(self);
    self.unbind();

    for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
        const ware_besch_t *ware = warenbauer_t::gib_info(i);
	slist_tpl<ware_t> *wliste = waren.get(ware);

	if(wliste) {
	    waren.remove(ware);
	    delete wliste;
	}
    }
}


/**
 * Sets the name. Creates a copy of name.
 * @author Hj. Malthaner
 */
void
haltestelle_t::setze_name(const char *name)
{
    tstrncpy(this->name, translator::translate(name), 128);

    if(grund.count() > 0) {
	grund.at(0)->setze_text(this->name);
    }
}


void
haltestelle_t::step()
{
    ptrhashtable_iterator_tpl<const ware_besch_t *, slist_tpl<ware_t> *> waren_iter(waren);

    while(waren_iter.next()) {
	static slist_tpl<ware_t> waren_kill_queue;
	slist_tpl<ware_t> * wliste = waren_iter.get_current_value();
	slist_iterator_tpl <ware_t> ware_iter(wliste);

	waren_kill_queue.clear();

	// Hajo:
	// Step 1: re-route goods now and then to adapt to changes in
	// world layout, remove all goods which destination was removed
	// from the map

	while(ware_iter.next()) {
	    ware_t & ware = ware_iter.access_current();

	    suche_route(ware, self);

	    // check if this good can still reach its destination
	    if(!gib_halt(ware.gib_ziel()).is_bound() ||
               !gib_halt(ware.gib_zwischenziel()).is_bound()) {

		// schedule it for removal
		waren_kill_queue.insert(ware);
	    }
	}

	while( waren_kill_queue.count() ) {
	    wliste->remove( waren_kill_queue.remove_first() );
	}
    }

    // hsiegeln: update amount of waiting ware
    // puts("plopp");
    financial_history[0][HALT_WAITING] = sum_all_waiting_goods();
}


/**
 * Called every month/every 24 game hours
 * @author Hj. Malthaner
 */
void haltestelle_t::neuer_monat()
{
  // Hajo: reset passenger statistics
  pax_happy = 0;
  pax_no_route = 0;
  pax_unhappy = 0;

	// hsiegeln: roll financial history
	for (int j = 0; j<MAX_HALT_COST; j++)
	{
		for (int k = MAX_MONTHS-1; k>0; k--)
		{
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}

}


/**
 * Calculates a status color for status bars
 * @author Hj. Malthaner
 */
int haltestelle_t::gib_status_farbe() const
{
  int color = GRAU6;

  if(get_pax_happy() > 0 || get_pax_no_route() > 0) {
    color = GREEN;
  }

  if(get_pax_no_route() > get_pax_happy() * 8) {
    color = GELB;
  }

  if(get_pax_unhappy() > 200) {
    color = ROT;
  } else if(get_pax_unhappy() > 40) {
    color = ORANGE;
  }

  return color;
}


/**
 * Draws some nice colored bars giving some status information
 * @author Hj. Malthaner
 */
void haltestelle_t::display_status(int xpos, int ypos) const
{
  const signed int count = warenbauer_t::gib_waren_anzahl();

  ypos -= 11;
  // all variables in the bracket MUST be signed, otherwise nothing may be drawn at all
  xpos -= (count*4 - get_tile_raster_width())/2;

  for(int i=0; i<count-1; i++) {

    // Hajo: don't show good "None"
    const ware_besch_t *wtyp = warenbauer_t::gib_info(i+1);

    const int v = MIN((gib_ware_summe(wtyp) >> 2) + 2, 128);

    display_fillbox_wh_clip(xpos+i*4, ypos-v-1, 1, v,
			    GRAU4,
			    true);

    display_fillbox_wh_clip(xpos+i*4+1, ypos-v-1, 2, v,
			    // (i & 7) * 4 + 1,
			    255 - i*4,
			    true);

    display_fillbox_wh_clip(xpos+i*4+3, ypos-v-1, 1, v,
			    GRAU1,
			    true);

    // Hajo: show up arrow for capped values
    if(v == 128) {
      display_fillbox_wh_clip(xpos+i*4+1, ypos-v-6, 2, 4,
			      WEISS,
			      true);
      display_fillbox_wh_clip(xpos+i*4, ypos-v-5, 4, 1,
			      WEISS,
			      true);
    }

  }

  const int color = gib_status_farbe();

  display_fillbox_wh_clip(xpos-1, ypos, count*4-2, 4, color, true);
}


void
haltestelle_t::verbinde_fabriken()
{
    if(!grund.is_empty()) {

	int minX=99999;
	int minY=99999;
	int maxX=0;
	int maxY=0;

	slist_iterator_tpl<grund_t *> iter( grund );

	while(iter.next()) {
	    grund_t *gb = iter.get_current();
	    koord p = gb->gib_pos().gib_2d();

	    if(p.x < minX) minX = p.x;
	    if(p.y < minY) minY = p.y;
	    if(p.x > maxX) maxX = p.x;
            if(p.y > maxY) maxY = p.y;
	}

	slist_iterator_tpl <fabrik_t *> fab_iter(fab_list);

	while( fab_iter.next() ) {
	  fab_iter.get_current()->unlink_halt(self);
	}


	vector_tpl<fabrik_t *> &fablist = fabrik_t::sind_da_welche(welt,
                                                  minX-3, minY-3,
                                                  maxX+3, maxY+3);
        fab_list.clear();

	for(uint32 i=0; i<fablist.get_count(); i++) {
	  fabrik_t * fab = fablist.at(i);
	  fab_list.insert(fab);

	  fab->link_halt(self);
	}
    }
}


/**
 * Rebuilds the list of reachable destinations
 *
 * @author Hj. Malthaner
 */
void haltestelle_t::rebuild_destinations()
{
  dbg->debug("haltestelle_t::rebuild_destinations()", "called");


  // Hajo: first, remove all old entries
  warenziele.clear();


  // dbg->message("haltestelle_t::rebuild_destinations()", "Adding new table entries");

  // Hajo: second, calculate new entries


  const slist_tpl <convoihandle_t> & convois = welt->gib_convoi_list();

  slist_iterator_tpl <convoihandle_t> iter ( convois);


  while( iter.next() ) {

    convoihandle_t cnv = iter.get_current();

    // dbg->message("haltestelle_t::rebuild_destinations()", "convoi %d %p", cnv.get_id(), cnv.get_rep());


    fahrplan_t *fpl = cnv->gib_fahrplan();

    if(fpl) {
      for(int i=0; i<=fpl->maxi; i++) {

      // Hajo: H�lt dieser convoi hier?
	const grund_t * gr = welt->lookup(fpl->eintrag.get(i).pos);
	if(gr && gr->gib_halt() == self) {

	  const int anz = cnv->gib_vehikel_anzahl();


	  for(int j=0; j<anz; j++) {
	    vehikel_t *v = cnv->gib_vehikel(j);

	    hat_gehalten(0, v->gib_fracht_typ(), fpl );
	  }
	}
      }
    }
  }
}


void
haltestelle_t::liefere_an_fabrik(const ware_t ware)
{
	slist_iterator_tpl<fabrik_t *> fab_iter(fab_list);

	while(fab_iter.next()) {
		fabrik_t * fab = fab_iter.get_current();

		const vector_tpl<ware_t> * eingang = fab->gib_eingang();

		for(uint32 i=0; i<eingang->get_count(); i++) {
			if(eingang->get(i).gib_typ() == ware.gib_typ()) {
				fab->liefere_an(ware.gib_typ(), ware.menge);
				return;
			}
		}
	}
}


/**
 * gibt true zur�ck, wenn die Ware nicht mehr reisen muss
 * weil sie schon nahe genug am ziel ist.
 *
 * false kann bedeuten: die Ware ist noch nicht am Ziel. Dabei gibt es zwei
 * F�lle: keine Route oder Route zum Ziel. ware->ziel() und Zwischenziel()
 * geben Auskunft, sind NULL, wenn keine Route existiert.
 *
 * Kann die Ware nicht zum Ziel geroutet werden (keine Route), dann werden
 * Ziel und Zwischenziel auf koord::invalid gesetzt.
 *
 * @param ware die zu routende Ware
 * @param start die Starthaltestelle
 * @author Hj. Malthaner
 */
bool
haltestelle_t::suche_route(ware_t &ware,
                           halthandle_t start)
{
    static HNode nodes[10000];

    static uint32 current_mark = 0;

    INT_CHECK("simhalt 452");


    // Need to clean up ?
    if(current_mark > (1u<<31)) {
      slist_iterator_tpl<halthandle_t > halt_iter (alle_haltestellen);

      while(halt_iter.next()) {
        halt_iter.get_current()->marke = 0;
      }

      current_mark = 0;
    }

    // alle alten markierungen ung�ltig machen
    current_mark++;


    // die Berechnung erfolgt durch eine Breitensuche fuer Graphen
    // Warteschlange fuer Breitensuche
    static slist_tpl <HNode *> queue;
    queue.clear();

    const koord ziel = ware.gib_zielpos();
    const ware_besch_t * warentyp = ware.gib_typ();
    int step = 1;
    HNode *tmp;

    nodes[0].halt = start;
    nodes[0].link = 0;
    nodes[0].depth = 0;

    queue.insert( &nodes[0] );        // init queue mit erstem feld

    start->marke = current_mark;


    // printf("\nSuche Route von %s\n", start->gib_name());


    // Breitensuche

    // long t0 = get_current_time_millis();

    do {
	tmp = queue.remove_first();

	const halthandle_t halt = tmp->halt;

	// printf("step: %s\n", halt->gib_name());


	// pr�fen ob ware am ziel ist

	// f�r fracht werden fabriken gepr�ft

	slist_iterator_tpl<fabrik_t*> fab_iter(halt->fab_list);

	while(fab_iter.next()) {
	  if(fab_iter.get_current()->gib_pos().gib_2d() == ziel) {
	    goto found;
	  }
	}

	// passagiere+post gehen bis zu 4 felder zu Fu� zu ihrem ziel

	const koord pos = halt->gib_basis_pos();
	const int distance = MAX(ABS(pos.x-ziel.x),ABS(pos.y-ziel.y));

	// printf("distance from %d %d to  %d %d is %d\n", ziel.x, ziel.y, pos.x, pos.y, distance);

	if(distance < 4) {
	  // ziel gefunden

	  // printf("distance from %d %d to  %d %d is %d\n", ziel.x, ziel.y, pos.x, pos.y, distance);

	  if(halt == start) {
	    // printf("Schon am ziel.\n");

	    // das zeugs braucht nicht zu reisen, es ist schon am ziel
	    return true;
	  }
	  goto found;
	}

	// Hajo: check for max transfers -> don't add more stations
	//      to queue if the limit is reached
	if(tmp->depth < max_transfers) {

	  // ziele pr�fen
	  slist_iterator_tpl<warenziel_t> iter(halt->warenziele);

	  while(iter.next() && step < max_hops) {
	    // check if destination if for the goods type
	    warenziel_t wz = iter.get_current();

	    // printf("checking %s\n", gib_halt(wz.gib_ziel())->gib_name());


	    if(wz.gib_typ()->is_interchangeable(warentyp)) {

	      const halthandle_t tmp_halt = gib_halt(welt, wz.gib_ziel());

	      if(tmp_halt.is_bound() &&
		 tmp_halt->marke != current_mark &&
		 (
		  (warentyp == warenbauer_t::passagiere &&
		   tmp_halt->pax_enabled) ||

		  (warentyp == warenbauer_t::post &&
		   tmp_halt->post_enabled) ||

		  (warentyp != warenbauer_t::post &&
		   warentyp != warenbauer_t::passagiere &&
		   tmp_halt->ware_enabled)
		  )
		 ) {

		// printf(" -> %s\n", tmp_halt->gib_name());

		HNode *node = &nodes[step++];

		node->halt = tmp_halt;
		node->depth = tmp->depth + 1;
		node->link = tmp;
		queue.append( node );

		// betretene Haltestellen markieren
		tmp_halt->marke = current_mark;

	      }
	    }
	  }

	  /*
	} else {
	  printf("routing %s to %s -> transfer limit reached\n",
		 ware.gib_name(),
		 gib_halt(ware.gib_ziel())->gib_name());
	  */

	} // max transfers

    } while(queue.count() && step < max_hops);

    // if the loop ends, nothing was found
    tmp = 0;

    // printf("No route found in %d steps\n", step);

found:

    // long t1 = get_current_time_millis();
    // printf("Route calc took %ld ms, %d steps\n", t1-t0, step);

    INT_CHECK("simhalt 606");

    // long t2 = get_current_time_millis();

    if(tmp) {
	// ziel gefunden
	ware.setze_ziel( tmp->halt->gib_basis_pos() );

	/*
	printf("route %s to %s with %d transfers\n",
               ware.gib_name(),
               gib_halt(ware.gib_ziel())->gib_name(),
	       tmp->depth);
	*/

	if(tmp->link == NULL) {
	    // kein zwischenziel
	  ware.setze_zwischenziel(ware.gib_ziel());
	} else {
	    // zwischenziel ermitteln

	    while(tmp->link->link) {
		tmp = tmp->link;
            }

	    ware.setze_zwischenziel(tmp->halt->gib_basis_pos());
	}

	/*
	printf("route %s to %s via %s in %d steps\n",
               ware.gib_name(),
               gib_halt(ware.gib_ziel())->gib_name(),
	       gib_halt(ware.gib_zwischenziel())->gib_name(),
	       step);
	*/
    } else {
	// Kein Ziel gefunden

	ware.setze_ziel(koord::invalid);
	ware.setze_zwischenziel(koord::invalid);
	// printf("keine route zu %d,%d nach %d steps\n", ziel.x, ziel.y, step);
    }

    // long t3 = get_current_time_millis();
    // printf("Route setup took %ld ms\n", t3-t2);

    // INT_CHECK("simhalt 659");

    return false;
}


/**
 * Found route and station uncrowded
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_happy(int n)
{
  pax_happy += n;
  book(n, HALT_HAPPY);
}


/**
 * Found no route
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_no_route(int n)
{
  pax_no_route += n;
  book(n, HALT_NOROUTE);
}


/**
 * Station crowded
 * @author Hj. Malthaner
 */
void haltestelle_t::add_pax_unhappy(int n)
{
  pax_unhappy += n;
  book(n, HALT_UNHAPPY);
}




bool
haltestelle_t::add_grund(grund_t *gr)
{
    // vielleicht sollte man hier auf eine maximale Anzahl
    // gebaeude prufen

    // append, damit grund.at(0) immer das erste Feld der Haltestelle darstellt

    assert(gr != NULL);

    if(!grund.contains( gr )) {
	gr->setze_halt(self);

	grund.append(gr);

	verbinde_fabriken();

	assert(gr->gib_halt() == self);

	return true;
    } else {
	return false;
    }
}

void
haltestelle_t::rem_grund(grund_t *gb)
{
    // namen merken
    const char *tmp = gib_name();

    if(gb) {
	grund.remove(gb);

//	printf("Haltestelle %s (%p) hat nach rem_grund noch %d b�den\n", tmp, this, grund.count());

	gb->setze_halt(halthandle_t ());

	if(!grund.is_empty()) {
	    grund_t *bd = grund.at(0);

	    if(bd->gib_text() != tmp) {
		bd->setze_text( tmp );
	    }

	    verbinde_fabriken();
	} else {
	  slist_iterator_tpl <fabrik_t *> iter(fab_list);

	  while( iter.next() ) {
	    iter.get_current()->unlink_halt(self);
	  }


	  fab_list.clear();
	}
    }
}

bool
haltestelle_t::existiert_in_welt()
{
//    printf("Haltestelle %p halt %d Fl�chen.\n", this, grund.count());
    return !grund.is_empty();
}



bool
haltestelle_t::ist_da(const koord k) const
{
    const planquadrat_t *plan = welt->lookup(k);

    return plan != NULL &&
           plan->gib_kartenboden() != NULL &&
	   plan->gib_kartenboden()->gib_halt() == self;
}


bool
haltestelle_t::gibt_ab(const ware_besch_t *wtyp) const
{
  // Exact match?
  bool ok = waren.get(wtyp) != 0;

  if(!ok) {
    // Check for category match
    ptrhashtable_iterator_tpl<const ware_besch_t *, slist_tpl<ware_t> *> iter (waren);
    while (!ok && iter.next()) {
      ok = wtyp->is_interchangeable(iter.get_current_key());
    }
  }

  return ok;
}


bool
haltestelle_t::pruefe_ziel(const ware_t &ware, const fahrplan_t *fpl) const
{
    const halthandle_t via_halt = gib_halt( ware.gib_zwischenziel() );

    // stimmt das Ziel ?
    if(!via_halt.is_bound()) {
        dbg->message("haltestelle_t::pruefe_ziel()",
		     "at stop %s there are %d %s without intermediate destination\n",
		     gib_name(),
		     ware.menge,
		     translator::translate(ware.gib_name()));
    }

    const int count = fpl->maxi + 1;

    // da wir schon an der aktuellem haltestelle halten
    // startet die schleife ab 1, d.h. dem naechsten halt

    for(int i=1; i<count; i++) {
	const int wrap_i = (i + fpl->aktuell) % count;

	halthandle_t plan_halt = gib_halt(fpl->eintrag.get(wrap_i).pos.gib_2d());

	if(plan_halt == self) {
	    // das fahrzeug kommt vor dem ziel hier noch mal vorbei,
	    // noch nicht einsteigen!
	    return false;
	} else if(plan_halt == via_halt) {
	    // ja, hier geht's zum ziel
	    // -> einsteigen
	    return true;
	}
    }

    // ziel nicht gefunden
    // nicht einsteigen
    return false;
}


halthandle_t
haltestelle_t::gib_halt(const koord pos) const
{
    return gib_halt(welt, pos);
}


ware_t
haltestelle_t::hole_ab(const ware_besch_t *wtyp, int maxi, fahrplan_t *fpl)
{
	// suche innerhalb der Liste nach ware mit passendem Ziel
	// und menge

	for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
		const ware_besch_t *ware = warenbauer_t::gib_info(i);
		slist_tpl<ware_t> * wliste = waren.get(ware);

		if(wliste) {
			slist_iterator_tpl<ware_t> iter (wliste);

			while(iter.next()) {
				ware_t &tmp = iter.access_current();

				// passt der Warentyp?
				bool ok = wtyp->is_interchangeable(tmp.gib_typ());

				// more checks: passt das Ziel ?
				if(ok && pruefe_ziel(tmp, fpl)) {
					// printf("hole_ab %d, es lagern %d %s\n", maxi, tmp->menge, tmp->name());

					// passt die Menge ?
					if(tmp.menge <= maxi) {
						// ja, alles geht ins Fahrzeug
						ware_t neu (tmp);
						bool ok = wliste->remove( tmp );
						assert(ok);

						if(wliste->count() == 0) {
							waren.remove(ware);
							delete wliste;
						}
						book(neu.menge, HALT_DEPARTED);
						return neu;

					}
					else {
						// Menge zu gro�, wir muessen aufteilen
						ware_t neu (tmp.gib_typ());
						neu.setze_ziel(tmp.gib_ziel());
						neu.setze_zwischenziel(tmp.gib_zwischenziel());
						neu.setze_zielpos(tmp.gib_zielpos());
						neu.menge = maxi;

						// abgegebene Menge von wartender Menge abziehen
						tmp.menge -= maxi;

						// printf("%s: Teile ware, gebe %d, behalte %d\n", gib_name(), maxi, tmp->menge);

						book(neu.menge, HALT_DEPARTED);
						return neu;
					}
				}
			}

			// es ist gar nichts passendes da zum abholen!
		}
	}

	// empty quantity of required type -> no effect
	return ware_t (wtyp);
}


int
haltestelle_t::gib_ware_summe(const ware_besch_t *typ) const
{
    int sum = 0;

    slist_tpl<ware_t> * wliste = waren.get(typ);
    if(wliste) {
	slist_iterator_tpl<ware_t> iter (wliste);

	while(iter.next()) {
	    sum += iter.get_current().menge;
	}
    }
    return sum;
}


int
haltestelle_t::gib_ware_fuer_ziel(const ware_besch_t *typ,
				  const koord ziel) const
{
  int sum = 0;

  slist_tpl<ware_t> * wliste = waren.get(typ);
  if(wliste) {
    slist_iterator_tpl<ware_t> iter (wliste);

    while(iter.next()) {
      const ware_t &ware = iter.get_current();

      if(ware.gib_ziel() == ziel) {
	sum += ware.menge;
      }
    }
  }

  return sum;
}




/*
 * Volker an Hajo: das sieht komisch aus:
 * Wenn ware_enabled == true kriegt man f�r Pax und Post immer true.
 * Soll das so sein?
 */
bool
haltestelle_t::nimmt_an(const ware_besch_t* wtyp)
{
    if(wtyp == warenbauer_t::passagiere && pax_enabled) {
	return true;
    }

    if(wtyp == warenbauer_t::post && post_enabled) {
	return true;
    }

    if(ware_enabled) {
	return true;
    }
#ifdef LAGER_NOT_IN_USE
    if(lager != NULL) {
	return lager->nimmt_an(wtyp);
    }
#endif
    return false;
}


bool
haltestelle_t::vereinige_waren(const ware_t &ware)
{
    // pruefen ob die ware mit bereits wartender ware vereinigt
    // werden kann

//    printf("pr�fe Warenrekombination: %s nach %s\n", ware->name(), gib_halt(ware->gib_ziel())->gib_name());
    slist_tpl<ware_t> * wliste = waren.get(ware.gib_typ());
    if(wliste) {
	slist_iterator_tpl<ware_t> iter(wliste);

	while(iter.next()) {
	    ware_t &tmp = iter.access_current();

//	    printf("  mit %s nach %s\n", tmp->name(), gib_halt(tmp->gib_ziel())->gib_name());

	    // es wird auf basis von Haltestellen vereinigt
	    if(gib_halt(tmp.gib_ziel()) == gib_halt(ware.gib_ziel())) {
		tmp.menge += ware.menge;

//		printf("  passt!\n");

		return true;
	    }
	}
    }

//    printf("  nix passt...\n");

    return false;
}



int
haltestelle_t::liefere_an(ware_t ware)
{
    // ware ohne ziel kann nicht bef�rdert werden
    // gib eine warnung aus und entferne ware
    if(ware.gib_ziel() == koord::invalid ||
       ware.gib_zwischenziel() == koord::invalid) {

	// keine route ?!
        dbg->warning("haltestelle_t::liefere_an()",
		     "%d %s delivered to %s have no longer a route to their destination!",
		     ware.menge,
		     translator::translate(ware.gib_name()),
		     gib_name());
	return ware.menge;
    }

    // ist die ware am ziel ?
    // dann kann sie geloescht werden
    if(gib_halt(ware.gib_ziel()) == self) {
	if(warenbauer_t::ist_fabrik_ware(ware.gib_typ())) {
	    // muss an fabrik geliefert werden
	    liefere_an_fabrik(ware);
	} else if(ware.gib_typ() == warenbauer_t::passagiere) {

	    if(welt->gib_einstellungen()->gib_show_pax()) {
		slist_iterator_tpl<grund_t *> iter (grund);

		int menge = ware.menge;
//		printf("simhalt_t::liefere_an: erzeuge %d Fussgaenger\n", menge);

		while(menge > 0 && iter.next()) {
		    grund_t *gr = iter.get_current();

//		    printf("simhalt_t::liefere_an: erzeuge Fussgaenger, noch %d\n", menge);
		    menge = erzeuge_fussgaenger(welt, gr->gib_pos(), menge);
		}

		INT_CHECK("simhalt 938");
	    }

	}

	return ware.menge;
    }

    // passt das zu bereits wartender ware ?
    if(vereinige_waren(ware)) {
	// dann sind wir schon fertig;
	return ware.menge;
    }

    // debug-info
    const char * halt_name = gib_halt(ware.gib_ziel())->gib_name();
    const char * via_name = gib_halt(ware.gib_zwischenziel())->gib_name();

    // ware passte zu nichts
    // ware neu routen
    bool schon_da = suche_route(ware, self);


    // Das neue routing sagt, wir sind nahe genug am Ziel
    // das kann vorkommen, wenn neue Ware �bergeben wurde
    if(schon_da) {

	if(warenbauer_t::ist_fabrik_ware(ware.gib_typ())) {
	    // muss an fabrik geliefert werden
	    liefere_an_fabrik(ware);
	}

	return ware.menge;
    }



    // existiert das ziel noch ?
    // sonst kann die ware gel�scht werden
    if(!gib_halt(ware.gib_ziel()).is_bound() ||
       !gib_halt(ware.gib_zwischenziel()).is_bound()) {

	// kein zielhalt  ?!
        dbg->message("haltestelle_t::liefere_an()",
		     "%s: delivered goods (%d %s) to %s via %s could not be routed to their destination!",
		     gib_name(),
		     ware.menge,
		     translator::translate(ware.gib_name()),
		     halt_name,
		     via_name);
	return ware.menge;
    }

    // passt das zu bereits wartender ware ?
    if(vereinige_waren(ware)) {
	// dann sind wir schon fertig;
	return ware.menge;
    }

    // wenn wir hier angekommen sind, konnte die ware
    // nicht vereinigt werden, sie wird neu in die Liste
    // eingef�gt
    slist_tpl<ware_t> * wliste = waren.get(ware.gib_typ());
    if(!wliste) {
	wliste = new slist_tpl<ware_t>;
	waren.set(ware.gib_typ(), wliste);
    }
    wliste->insert( ware );

    return ware.menge;
}


void
haltestelle_t::hat_gehalten(int /*wert*/, const ware_besch_t *type,
                            const fahrplan_t *fpl)
{

  if(type != warenbauer_t::nichts) {
    for(int i=0; i<=fpl->maxi; i++) {
      const warenziel_t wz (fpl->eintrag.get(i).pos.gib_2d(),
			    type);

      // Hajo: Haltestelle selbst wird nicht in Zielliste aufgenommen
      // Hajo: Nicht existierende Ziele werden �bersprungen
      const grund_t *gr = welt->lookup(fpl->eintrag.get(i).pos);
      if(gr == 0 || gr->gib_halt() == self) {
	continue;
      }

      slist_iterator_tpl<warenziel_t> iter(warenziele);

      while(iter.next()) {
	warenziel_t &tmp = iter.access_current();

	if(tmp.gib_typ()->is_interchangeable(type) &&
	   gib_halt(tmp.gib_ziel()) == gib_halt(wz.gib_ziel())) {
	  goto skip;
	}
      }

      warenziele.insert(wz);

    skip:;
    }
  }
}


const char *
haltestelle_t::quote_bezeichnung(int quote) const
{
    const char *str = "unbekannt";

    if(quote < 0) {
	str = translator::translate("miserabel");
    } else if(quote < 30) {
	str = translator::translate("schlecht");
    } else if(quote < 60) {
	str = translator::translate("durchschnitt");
    } else if(quote < 90) {
	str = translator::translate("gut");
    } else if(quote < 120) {
	str = translator::translate("sehr gut");
    } else if(quote < 150) {
	str = translator::translate("bestens");
    } else if(quote < 180) {
	str = translator::translate("excellent");
    } else {
	str = translator::translate("spitze");
    }

    return str;
}


void haltestelle_t::info(cbuffer_t & buf) const
{
  char tmp [512];

  sprintf(tmp,
	  translator::translate("Passengers today:\n %d %c, %d %c, %d no route\n\n%s\n"),
	  pax_happy,
	  30,
	  pax_unhappy,
	  31,
	  pax_no_route,
	  translator::translate("Hier warten/lagern:")
	  );

  buf.append(tmp);
}


/**
 * @param buf the buffer to fill
 * @return Goods description text (buf)
 * @author Hj. Malthaner
 */
void haltestelle_t::get_freight_info(cbuffer_t & buf)
{
  for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
    const ware_besch_t *wtyp = warenbauer_t::gib_info(i);

    slist_tpl<ware_t> * wliste = waren.get(wtyp);

    if(wliste) {
      slist_iterator_tpl<ware_t> iter (wliste);

      buf.append(" ");
      buf.append(gib_ware_summe(wtyp));
      buf.append(translator::translate(wtyp->gib_mass()));
      buf.append(" ");
      buf.append(translator::translate(wtyp->gib_name()));
      buf.append(" ");
      buf.append(translator::translate("waiting"));
      buf.append("\n");

      // hsiegeln
      // added sorting to ware's destination list
      int pos = 0;
      travel_details tdlist [wliste->count()];
      while(iter.next()) {
	ware_t ware = iter.get_current();
	tdlist[pos].ware = ware;
	tdlist[pos].destination = gib_halt(ware.gib_ziel());
	tdlist[pos].via_destination = gib_halt(ware.gib_zwischenziel());
	pos++;
      }
      // sort the ware's list
      qsort((void *)tdlist, pos, sizeof (travel_details), compare_ware);

      // print the ware's list to buffer - it should be in sortorder by now!
      for (int j = 0; j<pos; j++) {
	ware_t ware = tdlist[j].ware;

	const char * name = "Error in Routing";

	halthandle_t halt = gib_halt(ware.gib_ziel());
	if(halt.is_bound()) {
	  name = halt->gib_name();
	}

	buf.append("   ");
	buf.append(ware.menge);
	buf.append(translator::translate(wtyp->gib_mass()));
	buf.append(" ");
	buf.append(translator::translate(wtyp->gib_name()));
	buf.append(" > ");
	buf.append(name);

	// for debugging
	const char *via_name = "Error in Routing";
	halthandle_t via_halt = gib_halt(ware.gib_zwischenziel());
	if(via_halt.is_bound()) {
	  via_name = via_halt->gib_name();

	}

	if(via_halt != halt) {
	  char tmp [512];
	  sprintf(tmp, translator::translate("   via %s\n"), via_name);
	  buf.append(tmp);

	} else {
	  buf.append("\n");
	}
	// debug ende

      }

      buf.append("\n");
    }
  }
#ifdef LAGER_NOT_IN_USE
  if(lager != NULL) {
    buf.append("\n");
    buf = lager->info_lagerbestand(buf);
  }
#endif

  buf.append("\n");
}


void haltestelle_t::get_short_freight_info(cbuffer_t & buf)
{
  bool got_one = false;

  for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
    const ware_besch_t *wtyp = warenbauer_t::gib_info(i);

    if(gibt_ab(wtyp)) {
      if(got_one) {
	buf.append(", ");
      }

      buf.append(gib_ware_summe(wtyp));
      buf.append(translator::translate(wtyp->gib_mass()));
      buf.append(" ");
      buf.append(translator::translate(wtyp->gib_name()));

      got_one = true;
    }
  }


  if(got_one) {
    buf.append(" ");
    buf.append(translator::translate("waiting"));
    buf.append("\n");
  } else {
    buf.append(translator::translate("no goods waiting"));
    buf.append("\n");
  }
}


void
haltestelle_t::zeige_info()
{
    // sync name with ground
    access_name();
    // open window

    if(halt_info == 0) {
		halt_info = new halt_info_t(welt, self);
    }

    create_win(-1, -1, halt_info, w_info);
}


void
haltestelle_t::open_detail_window()
{
    create_win(-1, -1, new halt_detail_t(besitzer_p, welt, self), w_autodelete);
}


/**
 * @returns the sum of all waiting goods (100t coal + 10
 * passengers + 2000 liter oil = 2110)
 * @author Markus Weber
 */
int haltestelle_t::sum_all_waiting_goods() const      //15-Feb-2002    Markus Weber    Added
{
    int sum = 0;

    for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
	const ware_besch_t *wtyp = warenbauer_t::gib_info(i);

        if(gibt_ab(wtyp)) {
            sum += gib_ware_summe(wtyp);
        }
    }
    return sum;
}


bool
haltestelle_t::is_something_waiting() const
{
    for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
	const ware_besch_t *wtyp = warenbauer_t::gib_info(i);

        if(gibt_ab(wtyp)) {
	    return true;
        }
    }
    return false;
}


int haltestelle_t::get_station_type() const      //13-Jan-02     Markus Weber    Added
{
    int type=0;

    slist_iterator_tpl<grund_t *> iter( grund );

    while(iter.next()) {
	grund_t *gr = iter.get_current();
	if (gr->hat_gebaeude(hausbauer_t::frachthof_besch)) {
	    type |= loadingbay;
	}
	if (gr->hat_gebaeude(hausbauer_t::bahnhof_besch)) {
	    type |= railstation;
	}
	if (gr->hat_gebaeude(hausbauer_t::gueterbahnhof_besch)) { //14-oct-2004 added
	    type |= railstation;
	}
	if (gr->hat_gebaeude(hausbauer_t::dock_besch)) {
	    type |= dock;
	}
	if (gr->hat_gebaeude(hausbauer_t::bushalt_besch)) {
	    type |= busstop;
	}
    }

    return type;
}



const char *
haltestelle_t::name_from_ground() const
{
    const char *name = "Unknown";

    if(grund.is_empty()) {
	name = "Unnamed";
    } else {
	grund_t *bd = grund.at(0);

	if(bd != NULL && bd->gib_text() != NULL) {
	    name = bd->gib_text();
	}
    }

    return name;
}

char *
haltestelle_t::access_name()
{
    if(grund.count() > 0) {
	tstrncpy(name, name_from_ground(), 128);
	need_name = false;

	grund_t *bd = grund.at(0);

	if(bd != NULL) {
	    bd->setze_text(name);
	}
    }

    return name;
}


const char *
haltestelle_t::gib_name() const
{
    return name_from_ground();
}


int
haltestelle_t::erzeuge_fussgaenger_an(karte_t *welt,
				      const koord3d k,
				      int anzahl)
{
    // dbg->message("haltestelle_t::erzeuge_fussgaenger_an()", "called, %d description", fussgaenger_t::gib_anzahl_besch());


    if(fussgaenger_t::gib_anzahl_besch() > 0) {
	const grund_t *bd = welt->lookup(k);
	const weg_t *weg = bd->gib_weg(weg_t::strasse);

	if(weg && (weg->gib_ribi() == ribi_t::nordsued || weg->gib_ribi() == ribi_t::ostwest)) {

	    for(int i=0; i<4 && anzahl>0; i++) {
		fussgaenger_t *fg = new fussgaenger_t(welt, k);

		bool ok = welt->lookup(k)->obj_add(fg) != 0;

		if(ok) {
		    for(int i=0; i<(fussgaenger_t::count & 3); i++) {
			fg->sync_step(64*24);
		    }

		    welt->sync_add( fg );
		    anzahl --;
		} else {
		    dbg->message("haltestelle_t::erzeuge_fussgaenger_an()",
				 "Pedestrian could not be added, the pedestrians destructor will issue an error which can be ignored\n");
		    delete fg;
		}

	    }
	}
    }
    return anzahl;
}


int
haltestelle_t::erzeuge_fussgaenger(karte_t *welt, koord3d pos, int anzahl)
{
    if(welt->lookup(pos)) {
	anzahl = erzeuge_fussgaenger_an(welt, pos, anzahl);
    }


    for(int i=0; i<4 && anzahl>0; i++) {
	if(welt->lookup(pos+koord::nsow[i])) {
	    anzahl = erzeuge_fussgaenger_an(welt, pos+koord::nsow[i], anzahl);
	}
    }
    return anzahl;
}


void
haltestelle_t::rdwr(loadsave_t *file)
{
    int spieler_n;
    koord3d k;

    if(file->is_saving()) {
	spieler_n = welt->sp2num( besitzer_p );
    }
    pos.rdwr( file );
    file->rdwr_int(spieler_n, "\n");
    file->rdwr_bool(pax_enabled, " ");
    file->rdwr_bool(post_enabled, " ");
    file->rdwr_bool(ware_enabled, "\n");

    if(file->is_loading()) {
	do {
	    k.rdwr( file );
	    if( k != koord3d::invalid) {
		grund_t *gr = welt->lookup(k);
		if(!gr) {
		    gr = welt->lookup(k.gib_2d())->gib_kartenboden();
		    dbg->warning("haltestelle_t::rdwr()", "invalid position %s (setting to ground %s)\n",
				 k3_to_cstr(k).chars(),
				 k3_to_cstr(gr->gib_pos()).chars());
		}
		add_grund(gr);

		// Hajo: some versions generated oil rigs that did not accept
		// passengers. This line will change all water-based stations
		// to accept passengers. Should be removed once the savegames
		// are converted

		// if(gr->ist_wasser()) set_pax_enabled( true );

	    }
	} while( k != koord3d::invalid);
    } else {
	slist_iterator_tpl<grund_t*> gr_iter ( grund );

	while(gr_iter.next()) {
	    k = gr_iter.get_current()->gib_pos();
	    k.rdwr( file );
	}
	k = koord3d::invalid;
	k.rdwr( file );
    }

    short count;
    const char *s;

    if(file->is_saving()) {
      for(unsigned int i=0; i<warenbauer_t::gib_waren_anzahl(); i++) {
	const ware_besch_t *ware = warenbauer_t::gib_info(i);
	slist_tpl<ware_t> * wliste = waren.get(ware);

	if(wliste) {
	  s = ware->gib_name();
	  file->rdwr_str(s, "N");

	  count = wliste ? wliste->count() : 0;
	  file->rdwr_short(count, " ");
	  if(wliste) {
	    slist_iterator_tpl<ware_t> wliste_iter(wliste);
	    while(wliste_iter.next()) {
	      ware_t ware = wliste_iter.get_current();
	      ware.rdwr(file);
	    }
	  }
	}
      }
      s = "";
      file->rdwr_str(s, "N");


      count = warenziele.count();
      file->rdwr_short(count, " ");

      slist_iterator_tpl<warenziel_t>ziel_iter(warenziele);
      while(ziel_iter.next()) {
	warenziel_t wz = ziel_iter.get_current();
	wz.rdwr(file);
      }

    } else {
      s = NULL;
      file->rdwr_str(s, "N");
      while(s && *s) {
	const ware_besch_t *ware = warenbauer_t::gib_info(s);

	file->rdwr_short(count, " ");
	if(count) {
	  slist_tpl<ware_t> *wlist = new slist_tpl<ware_t>;

	  for(int i = 0; i < count; i++) {
	    ware_t ware(file);
	    wlist->insert(ware);
	  }
	  waren.put(ware, wlist);
	}
	file->rdwr_str(s, "N");
      }

      file->rdwr_short(count, " ");

      for(int i=0; i<count; i++) {
	warenziel_t wz (file);
	warenziele.append(wz);
      }

      guarded_free(const_cast<char *>(s));
    }

    if(file->is_loading()) {
      besitzer_p = welt->gib_spieler(spieler_n);
    }

	// load statistics
	if (file->get_version() < 83001)
	{
		init_financial_history();
	} else {
		for (int j = 0; j<MAX_HALT_COST; j++)
		{
			for (int k = MAX_MONTHS-1; k>=0; k--)
			{
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
	}

}


void
haltestelle_t::laden_abschliessen()
{
#ifdef LAGER_NOT_IN_USE
    slist_iterator_tpl<grund_t*> iter( grund );

    while(iter.next()) {
	koord3d k ( iter.get_current()->gib_pos() );

	// nach sondergebaeuden suchen

	ding_t *dt = welt->lookup(k)->suche_obj(ding_t::lagerhaus);

	if(dt != NULL) {
	    lager = dynamic_cast<lagerhaus_t *>(dt);
	    break;
	}
    }
#endif
}

void
haltestelle_t::book(sint64 amount, int cost_type)
{
	if (cost_type > MAX_HALT_COST)
	{
		// THIS SHOULD NEVER HAPPEN!
		// CHECK CODE
		dbg->warning("haltestelle_t::book()", "function was called with cost_type: %i, which is not valid (MAX_HALT_COST=%i)", cost_type, MAX_HALT_COST);
		return;
	}
	financial_history[0][cost_type] += amount;
	financial_history[0][HALT_WAITING] = sum_all_waiting_goods();
}

void
haltestelle_t::init_financial_history()
{
	for (int j = 0; j<MAX_HALT_COST; j++)
	{
		for (int k = MAX_MONTHS-1; k>=0; k--)
		{
			financial_history[k][j] = 0;
		}
	}
	financial_history[0][HALT_HAPPY] = pax_happy;
	financial_history[0][HALT_UNHAPPY] = pax_unhappy;
	financial_history[0][HALT_NOROUTE] = pax_no_route;
}
