/**
 * simconvoi.cc
 *
 * convoi_t Klasse f�r Fahrzeugverb�nde
 * von Hansj�rg Malthaner
 */

#include <math.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <malloc.h> // for alloca
#endif

#include "simdebug.h"
#include "simworld.h"
#include "simplay.h"
#include "simconvoi.h"
#include "simvehikel.h"
#include "simhalt.h"
#include "simdepot.h"
#include "simwin.h"
#include "simcolor.h"
#include "simmesg.h"
#include "blockmanager.h"
#include "simintr.h"
#include "simlinemgmt.h"
#include "simline.h"
#include "freight_list_sorter.h"

#include "gui/karte.h"
#include "gui/convoi_info_t.h"
#include "gui/fahrplan_gui.h"
#include "gui/messagebox.h"
#include "boden/grund.h"
#include "besch/vehikel_besch.h"
#include "dataobj/fahrplan.h"
#include "dataobj/loadsave.h"
#include "dataobj/translator.h"
#include "dataobj/umgebung.h"

#include "utils/tocstring.h"
#include "utils/simstring.h"
#include "utils/cbuffer_t.h"


// zeige debugging info in infofenster wenn definiert
// #define DEBUG 1

/*
 * Waiting time for loading (ms)
 * @author Hj- Malthaner
 */
#define WTT_LOADING 2000


/*
 * Debugging helper - translate state value to human readable name
 * @author Hj- Malthaner
 */
static const char * state_names[] =
{
  "INITIAL",
  "FAHRPLANEINGABE",
  "ROUTING_1",
  "ROUTING_2",
 "ROUTING_4",
  "ROUTING_5",
  "DRIVING",
  "LOADING",
  "WAITING_FOR_CLEARANCE",
  "WAITING_FOR_CLEARANCE_ONE_MONTH",
  ""	// self destruct
 };


/**
 * Calculates speed of slowest vehicle in the given array
 * @author Hj. Matthaner
 */
static int calc_min_top_speed(array_tpl <vehikel_t *> *fahr, int anz_vehikel)
{
  int min_top_speed = 9999999;
  for(int i=0; i<anz_vehikel; i++) {
    min_top_speed = min(min_top_speed, fahr->at(i)->gib_speed());
  }

  return min_top_speed;
}



/**
 * Initialize all variables with default values.
 * Each constructor must call this method first!
 * @author Hj. Malthaner
 */
void convoi_t::init(karte_t *wl, spieler_t *sp)
{
	welt = wl;
	besitzer_p = sp;

	sum_gesamtgewicht = sum_gewicht = sum_gear_und_leistung = sum_leistung = 0;
	previous_delta_v = 0;
	min_top_speed = 9999999;

	fahr = new array_tpl<vehikel_t *> (max_vehicle);

	old_fpl = fpl = NULL;
	line = linehandle_t();
	line_id = UNVALID_LINE_ID;

	for(unsigned i=0; i<fahr->get_size(); i++) {
		fahr->at(i) = NULL;
	}

	convoi_info = NULL;

	anz_vehikel = 0;
	anz_ready = 0;
	wait_lock = 0;

	jahresgewinn = 0;

	alte_richtung = ribi_t::keine;
	next_wolke = 0;

	state = INITIAL;

	*name = 0;

	freight_info_resort = true;
	freight_info_order = 0;
	loading_level = 0;
	loading_limit = 0;

	akt_speed_soll = 0;            // Sollgeschwindigkeit
	akt_speed = 0;                 // momentane Geschwindigkeit
	sp_soll = 0;

	line_update_pending = UNVALID_LINE_ID;

	home_depot = koord3d(0,0,0);
	last_stop_pos = koord3d(0,0,0);
}


convoi_t::convoi_t(karte_t *wl, loadsave_t *file) :
self(this)
{
	init(wl, 0);
	rdwr(file);
}


convoi_t::convoi_t(karte_t *wl, spieler_t *sp) :
  self(this)
{
	init(wl, sp);
	tstrncpy(name, translator::translate("Unnamed"), 128);
	akt_speed = 0;
	welt->add_convoi( self );
	init_financial_history();
}


convoi_t::~convoi_t()
{
DBG_MESSAGE("convoi_t::~convoi_t()", "destroying %d, %p", self.get_id(), this);
	// stop following
	if(welt->get_follow_convoi()==self) {
		welt->set_follow_convoi( convoihandle_t() );
	}

	if(convoi_info) {
		destroy_win(convoi_info);
		delete convoi_info;
	}
	convoi_info = 0;

	welt->sync_remove( this );
	welt->rem_convoi( self );

	// @author hsiegeln - deregister from line
	unset_line();

	// force asynchronous recalculation
	welt->set_schedule_counter();

	// if not already deleted, do it here ...
	for(unsigned i=0; i<anz_vehikel; i++) {
		// remove vehicle's marker from the reliefmap
		reliefkarte_t::gib_karte()->recalc_relief_farbe(fahr->at(i)->gib_pos().gib_2d());
		delete fahr->at(i);
	}
	delete fahr;

	// Hajo: finally clean up
	self.detach();

	fahr = 0;
	fpl = 0;
}



void
convoi_t::laden_abschliessen()
{
	if(anz_vehikel>0) {
		for( unsigned i=0;  i<anz_vehikel;  i++ ) {
			fahr->at(i)->setze_erstes( i==0 );
			fahr->at(i)->setze_letztes( i==(anz_vehikel-1u) );
			// this sets the convoi and will renew the block reservation, if needed!
			fahr->at(i)->setze_convoi( this );
		}
		// lines are still unknown during loading!
		if(line_id!=UNVALID_LINE_ID) {
			// if a line is assigned, set line!
			register_with_line(line_id);
		}
	}
	else {
		// no vehicles in this convoi?!?
		destroy();
	}
}



/**
 * Gibt die Position des Convois zur�ck.
 * @return Position des Convois
 * @author Hj. Malthaner
 */
koord3d
convoi_t::gib_pos() const
{
	if(anz_vehikel > 0 && fahr->get(0)) {
		return fahr->get(0)->gib_pos();
	} else {
		return koord3d::invalid;
	}
}


/**
 * Sets the name. Creates a copy of name.
 * @author Hj. Malthaner
 */
void
convoi_t::setze_name(const char *name)
{
  tstrncpy(this->name, translator::translate(name), 128);
}


/**
 * wird vom ersten Fahrzeug des Convois aufgerufen, um Aenderungen
 * der Grundgeschwindigkeit zu melden. Berechnet (Brems-) Beschleunigung
 * @author Hj. Malthaner
 */
void
convoi_t::setze_akt_speed_soll(int as)
{
	// first set speed limit
	akt_speed_soll=min(as,min_top_speed);
}



/**
 * Vehicles of the convoi add their running cost by using this
 * method
 * @author Hj. Malthaner
 */
void convoi_t::add_running_cost(sint32 cost)
{
  // Fahrtkosten
  jahresgewinn += cost;
  gib_besitzer()->buche(cost, COST_VEHICLE_RUN);

  // hsiegeln
  book(cost, CONVOI_OPERATIONS);
  book(cost, CONVOI_PROFIT);
}


/**
 * Vorbereitungsmethode f�r Echtzeitfunktionen eines Objekts.
 * @author Hj. Malthaner
 */
void
convoi_t::sync_prepare()
{
  if(wait_lock > 0) {
    // Hajo: got a saved game that had wait_lock set to 345891
    // I have no idea how that can happen, but we can work
    // around such problems here. -> wait at most 1 minute

    if(wait_lock > 60000) {

      dbg->warning("convoi_t::sync_prepre()",
		   "Convoi %d: wait lock out of bounds:"
		   "wait_lock = %d, setting to 60000",
		   self.get_id(), wait_lock);

      wait_lock = 60000;
    }

  } else {
    wait_lock = 0;
  }
}


/**
 * Methode f�r Echtzeitfunktionen eines Objekts.
 * @author Hj. Malthaner
 */
bool
convoi_t::sync_step(long delta_t)
{
  // DBG_MESSAGE("convoi_t::sync_step()", "%p, state %d", this, state);

  // moved check to here, as this will apply the same update
  // logic/constraints convois have for manual schedule manipulation
  if (line_update_pending != UNVALID_LINE_ID) {
    if ((state != ROUTING_4) && (state != ROUTING_5)) {
      check_pending_updates();
    }
  }

  wait_lock -= delta_t;


  if(wait_lock <= 0) {
  	wait_lock = 0;
    // step ++;

    switch(state) {
    case INITIAL:
      // jemand mu� start aufrufen, damit der convoi von INITIAL
      // nach ROUTING_1 geht, das kann nicht automatisch gehen
      break;

	case FAHRPLANEINGABE:
		if((fpl != NULL) && (fpl->ist_abgeschlossen())) {

			setze_fahrplan(fpl);
			welt->set_schedule_counter();

			if(fpl->maxi()==0) {
				// no entry => no route ...
				state = ROUTING_2;
			}
			else {
				// V.Meyer: If we are at destination - complete loading task!
				state = gib_pos() == fpl->eintrag.get(fpl->aktuell).pos ? LOADING : ROUTING_1;
				calc_loading();
			}
		}
	break;

    case ROUTING_1:
      wait_lock += WTT_LOADING;
      state = ROUTING_2;
//     DBG_MESSAGE("convoi_t::sync_step()","Convoi wechselt von ROUTING_1 nach ROUTING_2\n");
      break;

    case ROUTING_2:
      // Hajo: async calc new route, nothing to do here
      break;

    case ROUTING_4:
	fahr->at(0)->play_sound();
	state = ROUTING_5;
	// printf("Convoi wechselt von ROUTING_4 nach ROUTING_5\n");
	break;

    case ROUTING_5:
      // Hajo: this is an async task, see step()

      // hier m�ssen alle vorr�cken
      // da wir im letzten zug-feld starten
      break;


	case DRIVING:
		// teste, ob wir neu ansetzen m�ssen ?
		if(anz_ready == 0) {

			// Prissi: more pleasant and a little more "physical" model *
			int sum_friction_weight = 0;
			sum_gesamtgewicht = 0;
			// calculate total friction
			for(unsigned i=0; i<anz_vehikel; i++) {
				int total_vehicle_weight;

				total_vehicle_weight = fahr->at(i)->gib_gesamtgewicht();
				sum_friction_weight += fahr->at(i)->gib_frictionfactor()*total_vehicle_weight;
				sum_gesamtgewicht += total_vehicle_weight;
			}

			// try to simulate quadratic friction
			if(sum_gesamtgewicht != 0) {
				/*
				* The parameter consist of two parts (optimized for good looking):
				*  - every vehicle in a convoi has a the friction of its weight
				*  - the dynamic friction is calculated that way, that v^2*weight*frictionfactor = 200 kW
				* => the more heavy and the more fast the less power for acceleration is available!
				* since delta_t can have any value, we have to scale the step size by this value.
				* however, there is a quadratic friction term => if delta_t is too large the calculation may get weird results
				* @author prissi
				*/
				/* with floats, one would write: akt_speed*ak_speed*iTotalFriction*100 / (12,8*12,8) + 32*anz_vehikel;
				* but for integer, we have to use the order below and calculate actually 64*deccel, like the sum_gear_und_leistung */
				/* since akt_speed=10/128 km/h and we want 64*200kW=(100km/h)^2*100t, we must multiply by (128*2)/100 */
				/* But since the acceleration was too fast, we just deccelerate 4x more => >>6 instead >>8 */
				sint32 deccel = ( ( (akt_speed*sum_friction_weight)>>6 )*(akt_speed>>2) ) / 25 + (sum_gesamtgewicht*64);	// this order is needed to prevent overflows!
				// we normalize delta_t to 1/64th and check for speed limit */
				sint32 delta_v = ( ( (akt_speed>akt_speed_soll?0l:sum_gear_und_leistung) - deccel) * delta_t)/sum_gesamtgewicht;
				// we need more accurate arithmetik, so we store the previous value
				delta_v += previous_delta_v;
				previous_delta_v = delta_v & 0x0FFF;
				// and finally calculate new speed
				akt_speed = max(akt_speed_soll>>4, akt_speed+(sint32)(delta_v>>12l) );
//DBG_MESSAGE("convoi_t::sync_step","accel %d, deccel %d, akt_speed %d, delta_t %d, delta_v %d, previous_delta_v %d",sum_gear_und_leistung,deccel,akt_speed,delta_t,delta_v, previous_delta_v );
			}
		else {
			// very old vehicle ...
			akt_speed += 16;
		}
		// obey speed maximum with additional const brake ...
		if(akt_speed > akt_speed_soll) {
//DBG_MESSAGE("convoi_t::sync_step","akt_speed=%d, akt_speed_soll=%d",akt_speed,akt_speed_soll);
			akt_speed -= 24;
			if(akt_speed > akt_speed_soll+kmh_to_speed(20)) {
				akt_speed = akt_speed_soll+kmh_to_speed(20);
//DBG_MESSAGE("convoi_t::sync_step","akt_speed=%d, akt_speed_soll=%d",akt_speed,akt_speed_soll);
			}
		}

		// now actually move the units
		sp_soll += (akt_speed*delta_t) / 64;
		while(1024 < sp_soll && anz_ready==0) {
			sp_soll -= 1024;

			fahr->at(0)->sync_step();
			// stopped by something!
			if(state!=DRIVING) {
				sp_soll &= 1023;
				return true;
			}
			for(unsigned i=1; i<anz_vehikel; i++) {
				fahr->at(i)->sync_step();
			}
		}

		// smoke for the engines
		next_wolke += delta_t;
		if(next_wolke>500) {
			next_wolke = 0;
			for(int i=0;  i<anz_vehikel;  i++  ) {
				fahr->at(i)->rauche();
			}
		}

	} // end if(anz_ready==0)
	else {
		// Ziel erreicht
		alte_richtung = fahr->at(0)->gib_fahrtrichtung();

		// pruefen ob wir ein depot erreicht haben
		const grund_t *gr = welt->lookup(fahr->at(0)->gib_pos());
		depot_t * dp = gr->gib_depot();


		if(dp) {
			// ok, we are entering a depot
			char buf[128];

			// Gewinn f�r transport einstreichen
			calc_gewinn();

			akt_speed = 0;
			sprintf(buf, translator::translate("!1_DEPOT_REACHED"), gib_name());
			message_t::get_instance()->add_message(buf,fahr->at(0)->gib_pos().gib_2d(),message_t::convoi,gib_besitzer()->kennfarbe,IMG_LEER);

			betrete_depot(dp);

			// Hajo: since 0.81.5exp it's safe to
			// remove the current sync object from
			// the sync list from inside sync_step()
			welt->sync_remove(this);

			state = INITIAL;
			return true;  // Hajo: convoi is still alive
		}
		else {
			// no depot reached, so book values for stop!
			const grund_t *gr = welt->lookup(fahr->at(0)->gib_pos());
			halthandle_t halt = haltestelle_t::gib_halt(welt, fahr->at(0)->gib_pos());
			// we could have reached a non-haltestelle stop, so check before booking!
			if(halt.is_bound()  &&  gr->gib_weg_ribi(fahr->at(0)->gib_wegtyp())!=0) {
				// Gewinn f�r transport einstreichen
				calc_gewinn();
				akt_speed = 0;
				halt->book(1, HALT_CONVOIS_ARRIVED);
//DBG_MESSAGE("convoi_t::sync_step()","reached station at (%i,%i)",gr->gib_pos().x,gr->gib_pos().y);
				state = LOADING;
				calc_loading();
			}
			else {
//DBG_MESSAGE("convoi_t::sync_step()","passed waypoint at (%i,%i)",gr->gib_pos().x,gr->gib_pos().y);
				// Neither depot nor station: waypoint
				drive_to_next_stop();
				state = ROUTING_2;
			}
		}

	}

	break;

    case LOADING:
      // Hajo: loading is an async task, see laden()
      break;

    case WAITING_FOR_CLEARANCE:
    case WAITING_FOR_CLEARANCE_ONE_MONTH:
      // Hajo: waiting is asynchronous => fixed waiting order and route search
      break;

    case SELF_DESTRUCT:
	 for(unsigned f=0; f<anz_vehikel; f++) {
      	besitzer_p->buche(fahr->at(f)->calc_restwert(), fahr->at(f)->gib_pos().gib_2d(), COST_NEW_VEHICLE);
      }
      destroy();
      break;
    default:
      dbg->fatal("convoi_t::sync_step()", "Wrong state %d!\n", state);
    }
  }

  return true;
}



/**
 * Berechne route von Start- zu Zielkoordinate
 * @author Hanjs�rg Malthaner
 */
int convoi_t::drive_to(koord3d start, koord3d ziel)
{
	INT_CHECK("simconvoi 293");

	bool ok = anz_vehikel>0  ? fahr->at(0)->calc_route(welt, start, ziel, speed_to_kmh(min_top_speed), &route ) : false;
	if(!ok) {
		gib_besitzer()->bescheid_vehikel_problem(self,ziel);
	}

	INT_CHECK("simconvoi 297");
	return ok;
}



/**
 * Berechne route zum n�chsten Halt
 * @author Hanjs�rg Malthaner
 */
void convoi_t::drive_to_next_stop()
{
	fahrplan_t *fpl = gib_fahrplan();
	assert(fpl != NULL);

	if(fpl->aktuell+1 < fpl->maxi()) {
		fpl->aktuell ++;
	}
	else {
		fpl->aktuell = 0;
	}
}


/**
 * Ein Fahrzeug hat ein Problem erkannt und erzwingt die
 * Berechnung einer neuen Route
 * @author Hanjs�rg Malthaner
 */
void convoi_t::suche_neue_route()
{
  // XXX ?

  state = ROUTING_1;
}


/**
 * Advance route by one step.
 * @return next position on route
 * @author Hanjs�rg Malthaner
 */
koord3d convoi_t::advance_route(const int n) const
{
  koord3d result (-1, -1, -1);

  if(n <= route.gib_max_n()) {
    result = route.position_bei(n);
  } else {
    result = route.position_bei(route.gib_max_n());
  }

  return result;
}


/**
 * Asynchrne step methode des Convois
 * @author Hj. Malthaner
 */
void convoi_t::step()
{
	switch(state) {

		case LOADING:
			laden();
			break;

		case ROUTING_2:
			if(fpl->maxi()==0) {
				// no entries => no route ...
				state = ROUTING_2;
				gib_besitzer()->bescheid_vehikel_problem(self,fahr->at(0)->gib_pos());
			}
			else {
				// Hajo: now calculate a new route
				drive_to(fahr->at(0)->gib_pos(),fpl->eintrag.at(fpl->get_aktuell()).pos);
				if(route.gib_max_n() > 0) {
					// Hajo: ROUTING_3 is no more, go to ROUTING_4 directly
					state = ROUTING_4;
				}
			}
			break;

		case ROUTING_5:
			vorfahren();
			break;

		case WAITING_FOR_CLEARANCE:
		case WAITING_FOR_CLEARANCE_ONE_MONTH:
			{
				int restart_speed=-1;
				if(fahr->at(0)->ist_weg_frei(restart_speed)) {
					state = DRIVING;
				}
				if(restart_speed>=0) {
					akt_speed = restart_speed;
				}
			}
			break;

		default:	/* keeps compiler silent*/
			break;
	}
}


void
convoi_t::neues_jahr()
{
    jahresgewinn = 0;
}


void
convoi_t::new_month()
{
	// should not happen: leftover convoi without vehicles ...
	if(anz_vehikel==0) {
		DBG_DEBUG("convoi_t::new_month()","no vehicles => self destruct!");
		self_destruct();
		return;
	}
	// everything normal: update histroy
	for (int j = 0; j<MAX_CONVOI_COST; j++) {
		for (int k = MAX_MONTHS-1; k>0; k--) {
			financial_history[k][j] = financial_history[k-1][j];
		}
		financial_history[0][j] = 0;
	}
	// check for traffic jam
	if(state==WAITING_FOR_CLEARANCE) {
		state = WAITING_FOR_CLEARANCE_ONE_MONTH;
	}
	else if(state==WAITING_FOR_CLEARANCE_ONE_MONTH) {
		gib_besitzer()->bescheid_vehikel_problem(self,koord3d::invalid);
	}
}


void
convoi_t::betrete_depot(depot_t *dep)
{
	// Hajo: remove vehicles from world data structure
	for(unsigned i=0; i<anz_vehikel; i++) {
		fahr->at(i)->verlasse_feld();
	}

	dep->convoi_arrived(self, true);

	if(convoi_info) {
		//  V.Meyer: destroy convoi info when entering the depot
		destroy_win(convoi_info);
		delete convoi_info;
		convoi_info = NULL;
	}
}


void
convoi_t::start()
{
	if(state == INITIAL || state == ROUTING_1) {

		// set home depot to location of depot convoi is leaving
		set_home_depot(gib_pos());

		for(unsigned i=0; i<anz_vehikel; i++) {
			grund_t * gr = welt->lookup(fahr->at(i)->gib_pos());

			if(!gr->obj_ist_da(fahr->at(i))) {
				fahr->at(i)->betrete_feld();
			}
		}

		alte_richtung = ribi_t::keine;

		state = ROUTING_1;
		calc_loading();

		DBG_MESSAGE("convoi_t::start()","Convoi %s wechselt von INITIAL nach ROUTING_1", name);
	} else {
		dbg->warning("convoi_t::start()","called with state=%s\n",state_names[state]);
	}
}


/**
 * called, when the first vehicle reaches the target
 * @author Hj. Malthaner
 */
void convoi_t::ziel_erreicht(vehikel_t *)
{
	wait_lock += 8*50;
	anz_ready ++;
}


/**
 * Wartet bis Fahrzeug 0 freie Fahrt meldet
 * @author Hj. Malthaner
 */
void convoi_t::warten_bis_weg_frei(int restart_speed)
{
	if(!is_waiting()) {
		state = WAITING_FOR_CLEARANCE;
	}
	if(restart_speed>=0) {
		// langsam anfahren
		akt_speed = restart_speed;
	}
}



bool
convoi_t::add_vehikel(vehikel_t *v, bool infront)
{
DBG_MESSAGE("convoi_t::add_vehikel()","at pos %i of %i totals.",anz_vehikel,max_vehicle);
	// extend array if requested (only needed for trains)
	if(anz_vehikel == max_vehicle) {
DBG_MESSAGE("convoi_t::add_vehikel()","extend array_tpl to %i totals.",max_rail_vehicle);
		array_tpl <vehikel_t *> *f = new array_tpl<vehikel_t *> (max_rail_vehicle);
		for(unsigned i=0; i<max_vehicle; i++) {
			f->at(i) = fahr->at(i);
		}
		for(unsigned i=max_vehicle;  i<max_rail_vehicle; i++) {
			f->at(i) = NULL;
		}
		delete fahr;
		fahr = f;
	}
	// now append
	if(anz_vehikel < fahr->get_size()) {
		v->setze_convoi(this);

		if(infront) {
			for(unsigned i = anz_vehikel; i > 0; i--) {
				fahr->at(i) = fahr->at(i - 1);
			}
			fahr->at(0) = v;
		} else {
			fahr->at(anz_vehikel) = v;
		}
		anz_vehikel ++;

		const vehikel_besch_t *info = v->gib_besch();
		sum_leistung += info->gib_leistung();
		sum_gear_und_leistung += info->gib_leistung()*info->get_gear();
		sum_gewicht += info->gib_gewicht();
		min_top_speed = min(min_top_speed, v->gib_speed());
		sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		freight_info_resort = true;
	}
	else {
		return false;
	}

	// der convoi hat jetzt ein neues ende
	setze_erstes_letztes();

DBG_MESSAGE("convoi_t::add_vehikel()","now %i of %i total vehikels.",anz_vehikel,max_vehicle);
	return true;
}


vehikel_t *
convoi_t::remove_vehikel_bei(uint16 i)
{
	vehikel_t *v = NULL;
	if(i<anz_vehikel) {
		v = fahr->at(i);
		if(v != NULL) {
			for(unsigned j=i; j<anz_vehikel-1u; j++) {
				fahr->at(j) = fahr->at(j+1);
			}

			--anz_vehikel;
			fahr->at(anz_vehikel) = NULL;

			v->setze_convoi(NULL);

			const vehikel_besch_t *info = v->gib_besch();
			sum_leistung -= info->gib_leistung();
			sum_gear_und_leistung -= info->gib_leistung()*info->get_gear();
			sum_gewicht -= info->gib_gewicht();
		}
		sum_gesamtgewicht = sum_gewicht;
		calc_loading();
		freight_info_resort = true;

		// der convoi hat jetzt ein neues ende
		if(anz_vehikel > 0) {
			setze_erstes_letztes();
		}

		// Hajo: calculate new minimum top speed
		min_top_speed = calc_min_top_speed(fahr, anz_vehikel);
	}
	return v;
}

void
convoi_t::setze_erstes_letztes()
{
	// anz_vehikel muss korrekt init sein
	if(anz_vehikel>0) {
		fahr->at(0)->setze_erstes(true);
		for(unsigned i=1; i<anz_vehikel; i++) {
			fahr->at(i)->setze_erstes(false);
			fahr->at(i-1)->setze_letztes(false);
		}
		fahr->at(anz_vehikel-1)->setze_letztes(true);
	}
	else {
		dbg->warning("convoi_t::setze_erstes_letzes()", "called with anz_vehikel==0!");
	}
}


bool
convoi_t::setze_fahrplan(fahrplan_t * f)
{
	enum states old_state = state;
	state = INITIAL;	// because during a sync-step we might be called twice ...

	DBG_DEBUG("convoi_t::setze_fahrplan()", "new=%p, old=%p", f, fpl);

	if(f == NULL) {
		return false;
	}

	if(old_fpl!=NULL) {
		delete old_fpl;
	}
	old_fpl = NULL;

	// happens to be identical?
	if(fpl!=f) {
		// delete, if not equal
		if(fpl) {
			delete fpl;
		}
	}
	fpl = NULL;

	// rebuild destination for the new schedule
	fpl = f;

	// asynchronous recalculation of routes
	welt->set_schedule_counter();

	// remove wrong freight
	for(unsigned i=0; i<anz_vehikel; i++) {
		fahr->at(i)->remove_stale_freight();
	}

	// ok, now we have a schedule
	if(old_state!=INITIAL) {
		state = FAHRPLANEINGABE;
	}
	return true;
}


fahrplan_t *
convoi_t::erzeuge_fahrplan()
{
	if(fpl == NULL) {
		if(fahr->at(0) != NULL) {
			fpl = fahr->at(0)->erzeuge_neuen_fahrplan();
			fpl->eingabe_abschliessen();
		}
	}

	return fpl;
}



/* checks, if we go in the same direction;
 * true: convoy prepared
 * false: must recalculate position
 * on all error we better use the normal starting procedure ...
 */
bool
convoi_t::go_alte_richtung()
{
	// invalid route?
	if(route.gib_max_n()<=2) {
		return false;
	}

	// going backwards? then recalculate all
	sint8 dummy;
	ribi_t::ribi neue_richtung_rwr =  ribi_t::rueckwaerts( fahr->at(0)->calc_richtung(route.position_bei(0).gib_2d(), route.position_bei(2).gib_2d(), dummy, dummy) );
//DBG_MESSAGE("convoi_t::go_alte_richtung()","neu=%i,rwr_neu=%i,alt=%i",neue_richtung,ribi_t::rueckwaerts(neue_richtung),alte_richtung);
	if(neue_richtung_rwr&alte_richtung) {
		akt_speed = 8;
		return false;
	}

	// now get the actual length and the tile length
	int length=15;
	for(unsigned i=0; i<anz_vehikel; i++) {
		length+=fahr->at(i)->gib_besch()->get_length();
		if(i>0  &&  abs_distance( fahr->at(i)->gib_pos().gib_2d(), fahr->at(i-1)->gib_pos().gib_2d())>=2 ) {
			// ending here is an error!
			// this is an already broken train => restart
			dbg->warning("convoi_t::go_alte_richtung()","broken convoy (id %i) found => fixing!",self.get_id());
			akt_speed = 8;
			return false;
		}
	}
	length = min((length/16)+2,route.gib_max_n()-1);	// maximum length in tiles to check

	// we just check, wether we go back (i.e. route tiles other than zero have convoi vehicles on them)
	for( int index=1;  index<length;  index++ ) {
		grund_t *gr=welt->lookup(route.position_bei(index));
		// now check, if we are already here ...
		for(unsigned i=0; i<anz_vehikel; i++) {
			if(gr->obj_ist_da(fahr->at(i))) {
				// we are turning around => start slowly ...
				akt_speed = 8;
				return false;
			}
		}
	}

//DBG_MESSAGE("convoi_t::go_alte_richtung()","alte=%d, neu_rwr=%d",alte_richtung,neue_richtung_rwr);

	// we continue our journey; however later cars need also a correct route entry
	// eventually we need to add their positions to the convois route
	koord3d pos ( fahr->at(0)->gib_pos() );
	for(unsigned i=1; i<anz_vehikel; i++) {
		const koord3d k = fahr->at(i)->gib_pos();
		if(pos != k) {
			// ok, same => insert and continue
			route.insert(k);
			pos = k;
		}
	}

	// since we need the route for every vehicle of this convoi,
	// we mus set the current route index (instead assuming 1)
	for(unsigned i=0; i<anz_vehikel; i++) {
		// this is such akward, since it takes into account different vehicle length
		const koord3d vehicle_start_pos = fahr->at(i)->gib_pos();
		bool ok=false;
		for( int idx=0;  idx<length;  idx++  ) {
			if(route.position_bei(idx)==vehicle_start_pos) {
				fahr->at(i)->neue_fahrt( idx );
				ok = true;
				break;
			}
		}
		// too short?!?
		if(!ok) {
			return false;
		}
	}

	// on curves the vehicle may be already on the next tile but with a wrong direction
	for(unsigned i=0; i<anz_vehikel; i++) {
		uint8 richtung = fahr->at(i)->gib_fahrtrichtung();
		uint8 neu_richtung=fahr->at(i)->richtung();
		// we need to move to this place ...
		if(neu_richtung!=richtung  &&  (i!=0  ||  anz_vehikel==1)) {
			// 90 deg bend!
			return false;
		}
	}

	return true;
}



void
convoi_t::go_neue_richtung()
{
	const koord3d k0 = route.position_bei(0);
	const koord3d k1 = route.position_bei(1);
	for(unsigned i=0; i<anz_vehikel; i++) {
		grund_t *gr=welt->lookup(fahr->at(i)->gib_pos());
		if(gr) {
			gr->verlasse(fahr->at(i));
			gr->obj_remove(fahr->at(i),gib_besitzer());
		}
		fahr->at(i)->neue_fahrt(0);
		fahr->at(i)->setze_pos(k0);
		fahr->at(i)->starte_neue_route(k0, k1);
		gr=welt->lookup(k0);
		if(gr) {
			gr->betrete(fahr->at(i));
			fahr->at(i)->betrete_feld();
		}
	}
}


void
convoi_t::vorfahren()
{
	// Hajo: init speed settings
	sp_soll = 0;

	anz_ready = 0;

	sint8 dummy1, dummy2;
	setze_akt_speed_soll(fahr->at(0)->gib_speed());

	INT_CHECK("simconvoi 651");

	if(!go_alte_richtung()) {
		// we reached a terminal => so we head in the reverse direction

		ribi_t::ribi neue_richtung =  fahr->at(0)->calc_richtung(route.position_bei(0).gib_2d(), route.position_bei(1).gib_2d(), dummy1, dummy2);
		go_neue_richtung();
		bool extra_check=false;	// if there are only four view, we may need to extra advance the car

		for(unsigned i=0; i<anz_vehikel; i++) {
			switch(neue_richtung) {
				case ribi_t::west:
					fahr->at(i)->setze_offsets(10,5);
					break;
				case ribi_t::ost :
					fahr->at(i)->setze_offsets(-2,-1);
					extra_check = true;
					break;
				case ribi_t::nord:
					fahr->at(i)->setze_offsets(-10,5);
					extra_check = true;
					break;
				case ribi_t::sued:
					fahr->at(i)->setze_offsets(2,-1);
					break;
				default:
					fahr->at(i)->setze_offsets(0,0);
					break;
			}
		}

		// move one train length to the start position ...
		int train_length=0;
		for(unsigned i=0; i<anz_vehikel-1u; i++) {
			train_length += fahr->at(i)->gib_besch()->get_length();	// this give the length in 1/16 of a full tile
		}

		// now advance all convoi until it is completely on the track
		fahr->at(0)->setze_erstes( false );	// switches off signal checks ...
		for(unsigned i=0; i<anz_vehikel; i++) {
			fahr->at(i)->darf_rauchen(false);
			fahr->at(i)->richtung();
			for(int j=0; j<train_length; j++) {
				fahr->at(i)->sync_step();
			}
			train_length -= fahr->at(i)->gib_besch()->get_length();	// this give the length in 1/16 of a full tile => all cars closely coupled!
			fahr->at(i)->darf_rauchen(true);
		}
		fahr->at(0)->setze_erstes( true );
	}


	INT_CHECK("simconvoi 711");
	state = DRIVING;
	calc_loading();
}


void
convoi_t::rdwr(loadsave_t *file)
{
	long dummy;
	int besitzer_n = welt->sp2num(besitzer_p);

	if(file->is_saving()) {
		file->wr_obj_id("Convoi");
	}

	// for new line management we need to load/save the assigned line id
	// @author hsiegeln
	if(file->get_version()<88003) {
		dummy=0;
		file->rdwr_long(dummy, " ");
		line_id = dummy;
	}
	else {
		file->rdwr_short(line_id, " ");
	}

	dummy=anz_vehikel;
	file->rdwr_long(dummy, " ");
	anz_vehikel = dummy;
	dummy=anz_ready;
	file->rdwr_long(dummy, " ");
	anz_ready=dummy;
	file->rdwr_long(wait_lock, " ");
	bool dummy_bool=false;
	file->rdwr_bool(dummy_bool, " ");
	file->rdwr_long(besitzer_n, "\n");
	file->rdwr_long(akt_speed, " ");
	file->rdwr_long(akt_speed_soll, " ");
	file->rdwr_long(sp_soll, " ");
	file->rdwr_enum(state, " ");
	file->rdwr_enum(alte_richtung, " ");
	dummy=jahresgewinn;
	file->rdwr_long(dummy, "\n");
	jahresgewinn = dummy;
	route.rdwr(file);

	if(file->is_loading()) {
		jahresgewinn = dummy;	// aktually a sint64 ...

		// extend array if requested (only needed for trains)
		if(anz_vehikel > max_vehicle) {
			fahr = new array_tpl<vehikel_t *> (max_rail_vehicle);
			for(unsigned i=0; i<max_rail_vehicle; i++) {
				fahr->at(i) =NULL;
			}
		}
		besitzer_p = welt->gib_spieler( besitzer_n );

		// Hajo: sanity check for values ... plus correction
		if(sp_soll < 0) {
			sp_soll = 0;
		}
	}

    file->rdwr_str(name, sizeof(name));

	koord3d dummy_pos=koord3d(0,0,0);
	for(unsigned i=0; i<anz_vehikel; i++) {
		if(file->is_saving()) {
			fahr->at(i)->rdwr(file, true);
			dummy_pos.rdwr(file);	// will be ignored ...
		}
		else {
			ding_t::typ typ = (ding_t::typ)file->rd_obj_id();
			vehikel_t *v = 0;

			switch(typ) {
				case ding_t::automobil: v = new automobil_t(welt, file);  break;
				case ding_t::waggon:    v = new waggon_t(welt, file);     break;
				case ding_t::schiff:    v = new schiff_t(welt, file);     break;
				case ding_t::aircraft:    v = new aircraft_t(welt, file);     break;
				case ding_t::monorailwaggon:    v = new monorail_waggon_t(welt, file);     break;
				default:
				dbg->fatal("convoi_t::convoi_t()","Can't load vehicle type %d", typ);
			}
			dummy_pos.rdwr(file);

			assert(v != 0);
			fahr->at(i)  = v;

			const vehikel_besch_t *info = v->gib_besch();

			// Hajo: if we load a game from a file which was saved from a
			// game with a different vehicle.tab, there might be no vehicle
			// info
			if(info) {
				sum_leistung += info->gib_leistung();
				sum_gear_und_leistung += info->gib_leistung()*info->get_gear();
				sum_gewicht += info->gib_gewicht();
			}
			else {
				DBG_MESSAGE("convoi_t::rdwr()","no vehikel info!");
			}


			if(state != INITIAL) {
				grund_t *gr;
				gr = welt->lookup(v->gib_pos());
				if(!gr) {
					gr = welt->lookup(v->gib_pos().gib_2d())->gib_kartenboden();
dbg->fatal("convoi_t::rdwr()","invalid position %s for vehicle %s in state %d (setting to ground %s)",k3_to_cstr(v->gib_pos()).chars(), v->gib_name(), state, k3_to_cstr(gr->gib_pos()).chars());
				}
				gr->betrete(v);	// this will correct the block counter ...
				gr->obj_add(v);
			}
		}
	}
	sum_gesamtgewicht = sum_gewicht;

	bool has_fpl = fpl != NULL;
	file->rdwr_bool(has_fpl, "");
	if(has_fpl) {
		//DBG_MESSAGE("convoi_t::rdwr()","convoi has a schedule, state %s!",state_names[state]);
		if(file->is_loading() && fahr->at(0)) {
			fpl = fahr->at(0)->erzeuge_neuen_fahrplan();
		}
		// Hajo: hack to load corrupted games -> there is a schedule
		// but no vehicle so we can't determine the exact type of
		// schedule needed. This hack is safe because convois
		// without vehicles get deleted right after loading.
		if(fpl == 0) {
			fpl = new fahrplan_t();
		}

		// Hajo: now read the schedule, we have one for sure here
		fpl->rdwr( file );
	}

	if(file->is_loading()) {
		next_wolke = 0;
		calc_loading();
	}

	// Hajo: calculate new minimum top speed
	min_top_speed = calc_min_top_speed(fahr, anz_vehikel);

	// Hajo: since sp_ist became obsolete, sp_soll is
	//       used modulo 1024
	sp_soll &= 1023;

	if(file->get_version()<=88003) {
		// load statistics
		for (int j = 0; j<3; j++) {
			for (int k = MAX_MONTHS-1; k>=0; k--) {
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
		for (int j = 2; j<5; j++) {
			for (int k = MAX_MONTHS-1; k>=0; k--) {
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
	}
	else {
		// load statistics
		for (int j = 0; j<MAX_CONVOI_COST; j++) {
			for (int k = MAX_MONTHS-1; k>=0; k--) {
				file->rdwr_longlong(financial_history[k][j], " ");
			}
		}
	}

	// save/restore pending line updates
	if (file->get_version() > 84008) {
		dummy = line_update_pending;
		file->rdwr_long(dummy, "\n");	// ignore
		line_update_pending = UNVALID_LINE_ID;
	}

	if(file->get_version() > 84009) {
		home_depot.rdwr(file);
	}

	if(file->get_version()>=87001) {
		last_stop_pos.rdwr(file);
	}
	else {
		last_stop_pos = route.gib_max_n()>1 ? route.position_bei(0) : (anz_vehikel>0 ? fahr->at(0)->gib_pos() : koord3d(0,0,0) );
	}
}


void
convoi_t::zeige_info()
{
	if(!in_depot()) {

		if(umgebung_t::verbose_debug) {
			dump();
			if(anz_vehikel > 0 && fahr->at(0)) {
				fahr->at(0)->dump();
			}
		}

		if(!convoi_info) {
			convoi_info = new convoi_info_t(self);
		}
		else {
			destroy_win(convoi_info);
		}
		create_win(-1, -1, convoi_info, w_info);
	}
}


void convoi_t::info(cbuffer_t & buf) const
{
  if(fahr->at(0) != NULL) {
    char tmp[128];

    sprintf(tmp,"\n %d/%dkm/h (%1.2f$/km)\n", speed_to_kmh(min_top_speed), speed_to_kmh(fahr->at(0)->gib_speed()), get_running_cost()/100.0 );
    buf.append(tmp);

    sprintf(tmp," %s: %ikW\n", translator::translate("Leistung"), sum_leistung );
    buf.append(tmp);

    sprintf(tmp," %s: %i (%i) t\n", translator::translate("Gewicht"), sum_gewicht, sum_gesamtgewicht-sum_gewicht );
    buf.append(tmp);

    sprintf(tmp," %s: ", translator::translate("Gewinn")  );
    buf.append(tmp);

    money_to_string( tmp, jahresgewinn );
    buf.append(tmp);
    buf.append("\n");
  }
}


void convoi_t::set_sort(int sort_order)
{
	freight_info_order = sort_order;
	freight_info_resort = true;
}

void convoi_t::get_freight_info(cbuffer_t & buf)
{
	if(freight_info_resort) {
		freight_info_resort = false;
		// rebuilt the list with goods ...
		slist_tpl<ware_t> total_fracht;

		uint32 max_loaded_waren[warenbauer_t::gib_waren_anzahl()];
		memset( max_loaded_waren, 0, sizeof(uint32)*warenbauer_t::gib_waren_anzahl() );

		for(unsigned i=0; i<anz_vehikel; i++) {
			// first add to capacity indicator
			const ware_besch_t *ware_besch=fahr->at(i)->gib_besch()->gib_ware();
			const uint16 menge=fahr->at(i)->gib_besch()->gib_zuladung();
			if(menge>0  &&  ware_besch!=warenbauer_t::nichts) {
				max_loaded_waren[ware_besch->gib_index()] += menge;
			}

			// then add the actual load
			slist_iterator_tpl<ware_t> iter_vehicle_ware (fahr->at(i)->gib_fracht());
			while(iter_vehicle_ware.next()) {
				ware_t ware= iter_vehicle_ware.get_current();
				slist_iterator_tpl<ware_t> iter (total_fracht);

				const bool is_pax = (ware.gib_typ()==warenbauer_t::passagiere  ||  ware.gib_typ()==warenbauer_t::post);

				// could this be joined with existing freight?
				while(iter.next()) {
					ware_t &tmp = iter.access_current();

					// for pax: join according next stop
					// for all others we *must* use target coordinates
					if(tmp.gib_typ()==ware.gib_typ()  &&  (tmp.gib_zielpos()==ware.gib_zielpos()  ||  (is_pax   &&   haltestelle_t::gib_halt(welt,tmp.gib_ziel())==haltestelle_t::gib_halt(welt,ware.gib_ziel()))  )  ) {
						tmp.menge += ware.menge;
						ware.menge = 0;
						break;
					}
				}

				// if != 0 we could not joi it to existing => load it
				if(ware.menge != 0) {
					total_fracht.insert(ware);
				}

			}

			INT_CHECK("simvehikel 876");
		}
		buf.clear();

		// apend info on total capacity
		slist_tpl <ware_t>capacity;
		for(unsigned i=0;  i<warenbauer_t::gib_waren_anzahl();  i++  ) {
			if(max_loaded_waren[i]>0  &&  i!=2) {
				ware_t ware(warenbauer_t::gib_info(i+1));
				ware.menge = max_loaded_waren[i];
				if(ware.gib_catg()==0) {
					capacity.append( ware );
				}
				else {
					// append to catgory?
					unsigned j=0;
					while(j<capacity.count()  &&  capacity.at(j).gib_catg()<ware.gib_catg()) {
						j++;
					}
					if(j<capacity.count()  &&  capacity.at(j).gib_catg()==ware.gib_catg()) {
						// not yet there
						capacity.at(j).menge += max_loaded_waren[i];
					}
					else {
						// not yet there
						capacity.insert( ware, j );
					}
				}
			}
		}

		// show new info
		freight_list_sorter_t::sort_freight( welt, &total_fracht, buf, (freight_list_sorter_t::sort_mode_t)freight_info_order, &capacity, "loaded" );
	}
}


void
convoi_t::open_schedule_window()
{
	DBG_MESSAGE("convoi_t::open_schedule_window()","Id = %ld, State = %d, Lock = %d",self.get_id(), state, wait_lock);

	// darf der spieler diesen convoi umplanen ?
	if(gib_besitzer() != NULL &&
		gib_besitzer() != welt->get_active_player()) {
		return;
	}

	// manipulation of schedule not allowd while:
	// -convoi is in routing state 4 or 5
	// -a line update is pending
	if(state==FAHRPLANEINGABE  ||  state == ROUTING_4 || state == ROUTING_5 || line_update_pending!=UNVALID_LINE_ID) {
		create_win(-1, -1, 120, new nachrichtenfenster_t(welt, translator::translate("Not allowed!\nThe convoi's schedule can\nnot be changed currently.\nTry again later!")), w_autodelete);
		return;
	}

	if(state==DRIVING) {
		//recalc current amount of goods
		calc_gewinn();
	}

	akt_speed = 0;	// stop the train ...
	state = FAHRPLANEINGABE;
	alte_richtung = fahr->at(0)->gib_fahrtrichtung();

	old_fpl =new fahrplan_t( fpl );

	// Fahrplandialog oeffnen
	fahrplan_gui_t *fpl_gui = new fahrplan_gui_t(welt, self, fahr->at(0)->gib_besitzer());
	fpl_gui->zeige_info();
}



/* Fahrzeuge passen oft nur in bestimmten kombinationen
 * die Beschraenkungen werden hier geprueft, die f�r die Nachfolger von
 * vor gelten - daher mu� vor != NULL sein..
 */
bool
convoi_t::pruefe_nachfolger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
{
    const vehikel_besch_t *soll;

    if(!vor->gib_nachfolger_count()) {
	// Alle Nachfolger erlaubt
	return true;
    }
    for(int i=0; i < vor->gib_nachfolger_count(); i++) {
	soll = vor->gib_nachfolger(i);
	//DBG_MESSAGE("convoi_t::pruefe_an_index()",
	//    "checking successor: should be %d, is %d",
	//    soll ? soll->gib_name() : "none",
	//    hinter ? hinter->gib_name() : "none");

	if(hinter == soll) {
	    // Diese Beschr�nkung erlaubt unseren Nachfolger
	    return true;
	}
    }
    //DBG_MESSAGE("convoi_t::pruefe_an_index()",
    //		 "No matching successor found.");
    return false;
}

/* Fahrzeuge passen oft nur in bestimmten kombinationen
 * die Beschraenkungen werden hier geprueft, die f�r die Vorg�nger von
 *  hinter gelten - daher mu� hinter != NULL sein.
 */
bool
convoi_t::pruefe_vorgaenger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter)
{
    const vehikel_besch_t *soll;

    if(!hinter->gib_vorgaenger_count()) {
	// Alle Vorg�nger erlaubt
	return true;
    }
    for(int i=0; i < hinter->gib_vorgaenger_count(); i++) {
	soll = hinter->gib_vorgaenger(i);
	//DBG_MESSAGE("convoi_t::pruefe_vorgaenger()",
	//	     "checking predecessor: should be %s, is %s",
	//	     soll ? soll->gib_name() : "none",
	//	     vor ? vor->gib_name() : "none");

	if(vor == soll) {
	    // Diese Beschr�nkung erlaubt unseren Vorg�nger
	    return true;
	}
    }
    //DBG_MESSAGE("convoi_t::pruefe_vorgaenger()",
    //		 "No matching predecessor found.");
    return false;
}



bool
convoi_t::pruefe_alle()
{
    bool ok = !anz_vehikel || pruefe_vorgaenger(NULL, fahr->at(0)->gib_besch());
    unsigned i;

    for(i = 1; ok && i < anz_vehikel; i++) {
	ok =
	    pruefe_nachfolger(fahr->at(i - 1)->gib_besch(), fahr->at(i)->gib_besch()) &&
	    pruefe_vorgaenger(fahr->at(i - 1)->gib_besch(), fahr->at(i)->gib_besch());
    }
    if(ok)
	ok = pruefe_nachfolger(fahr->at(i - 1)->gib_besch(), NULL);

    return ok;
}


/**
 * Kontrolliert Be- und Entladen
 * @author Hj. Malthaner
 *
 * V.Meyer: ladegrad is now stored in the object (not returned)
 */
void convoi_t::laden()
{
	if(state == FAHRPLANEINGABE) {
		return;
	}

	const koord k = fpl->eintrag.get(fpl->aktuell).pos.gib_2d();

	halthandle_t halt = haltestelle_t::gib_halt(welt, fpl->eintrag.get(fpl->aktuell).pos);
	// eigene haltestelle ?
	if(halt.is_bound()  &&  (halt->gib_besitzer()==gib_besitzer()  ||  halt->gib_besitzer()==welt->gib_spieler(1)) ) {
		// loading/unloading ...
		hat_gehalten(k, halt);
	}

	INT_CHECK("simconvoi 1077");

	// Nun wurde ein/ausgeladen werden
	if(get_loading_level()>=get_loading_limit())  {

		// add available capacity after loading(!) to statistics
		for (unsigned i = 0; i<anz_vehikel; i++) {
			book(gib_vehikel(i)->gib_fracht_max()-gib_vehikel(i)->gib_fracht_menge(), CONVOI_CAPACITY);
		}
		wait_lock += WTT_LOADING;

		// Advance schedule
		drive_to_next_stop();
		state = ROUTING_2;
	}
	else {
		// Hajo: wait a few frames ... 250ms looks ok to me
		wait_lock = 250;
	}
}


/**
 * calculate income for last hop
 * @author Hj. Malthaner
 */
void convoi_t::calc_gewinn()
{
	sint64 gewinn = 0;

	for(unsigned i=0; i<anz_vehikel; i++) {
		vehikel_t *v = fahr->at(i);
		gewinn += v->calc_gewinn(last_stop_pos, fahr->at(0)->gib_pos() );
	}
	if(anz_vehikel>0) {
		last_stop_pos = fahr->at(0)->gib_pos();
	}

	besitzer_p->buche(gewinn, fahr->at(0)->gib_pos().gib_2d(), COST_INCOME);
	jahresgewinn += gewinn;

	// hsiegeln
	book(gewinn, CONVOI_PROFIT);
	book(gewinn, CONVOI_REVENUE);
}


/**
 * convoi an haltestelle anhalten
 * @author Hj. Malthaner
 *
 * V.Meyer: ladegrad is now stored in the object (not returned)
 */
void convoi_t::hat_gehalten(koord k, halthandle_t /*halt*/)
{
	// entladen und beladen
	for(unsigned i=0; i<anz_vehikel; i++) {

		// Nur diejenigen Fahrzeuge be-/entladen, die im Bahnhof sind
		vehikel_t *v = fahr->at(i);
		const halthandle_t &halt = haltestelle_t::gib_halt(welt, v->gib_pos());

		if(halt.is_bound()) {
			// Hajo: die waren wissen wohin sie wollen
			// zuerst alle die hier aus/umsteigen wollen ausladen
			freight_info_resort |= v->entladen(k, halt);

			// Hajo: danach neue waren einladen
			freight_info_resort |= v->beladen(k, halt);

			// Jeder zus�tzliche Waggon braucht etwas Zeit
			wait_lock += (WTT_LOADING/4);
		}
	}
	// any loading went on?
	if(freight_info_resort) {
		calc_loading();
	}
	loading_limit = fpl->eintrag.get(fpl->aktuell).ladegrad;
}


int convoi_t::calc_restwert() const
{
    int result = 0;

    for(unsigned i=0; i<anz_vehikel; i++) {
	result += fahr->at(i)->calc_restwert();
    }
    return result;
}


/**
 * Calclulate loading_level and loading_limit. This depends on current state (loading or not).
 * @author Volker Meyer
 * @date  20.06.2003
 */
void convoi_t::calc_loading()
{
	int fracht_max = 0;
	int fracht_menge = 0;
	for(unsigned i=0; i<anz_vehikel; i++) {
		vehikel_t *v = fahr->at(i);
		fracht_max += v->gib_fracht_max();
		fracht_menge += v->gib_fracht_menge();
	}
	loading_level = fracht_max > 0 ? (fracht_menge*100)/fracht_max : 100;
	loading_limit = 0;	// will be set correctly from laden() routine
}


/**
 * Schedule convoid for self destruction. Will be executed
 * upon next sync step
 * @author Hj. Malthaner
 */
void convoi_t::self_destruct()
{
  state = SELF_DESTRUCT;
}


/**
 * Helper method to remove convois from the map that cannot
 * removed normally (i.e. by sending to a depot) anymore.
 * This is a workaround for bugs in the game.
 * @author Hj. Malthaner
 * @date  12-Jul-03
 */
void convoi_t::destroy()
{
	delete this;
}


/**
 * Debug info nach stderr
 * @author Hj. Malthaner
 * @date 04-Sep-03
 */
void convoi_t::dump() const
{
    fprintf(stderr, "anz_vehikel = %d\n", anz_vehikel);
    fprintf(stderr, "anz_ready = %d\n", anz_ready);
    fprintf(stderr, "wait_lock = %d\n", wait_lock);
    fprintf(stderr, "besitzer_n = %d\n", welt->sp2num(besitzer_p));
    fprintf(stderr, "akt_speed = %d\n", akt_speed);
    fprintf(stderr, "akt_speed_soll = %d\n", akt_speed_soll);
    fprintf(stderr, "sp_soll = %d\n", sp_soll);
    fprintf(stderr, "state = %d\n", state);
    fprintf(stderr, "statename = %s\n", state_names[state]);
    fprintf(stderr, "alte_richtung = %d\n", alte_richtung);
    fprintf(stderr, "jahresgewinn = %lld\n", jahresgewinn);

    fprintf(stderr, "name = '%s'\n", name);
    fprintf(stderr, "line_id = '%d'\n", line_id);
    fprintf(stderr, "fpl = '%p'\n", fpl);
}


/**
 * Checks if this convoi has a driveable route
 * @author Hanjs�rg Malthaner
 */
bool convoi_t::hat_keine_route() const
{
  return route.gib_max_n() < 0;
}


/**
* get line
* @author hsiegeln
*/
linehandle_t convoi_t::get_line() const
{
	return line_id!=UNVALID_LINE_ID ? line : NULL;
}

/**
* set line
* since convoys must operate on a copy of the route's fahrplan, we apply a fresh copy
* @author hsiegeln
*/
void convoi_t::set_line(linehandle_t org_line)
{
	// to remove a convoi from a line, call unset_line(); passing a NULL is not allowed!
	if(!org_line.is_bound()) {
		return;
	}
	if(line.is_bound()) {
		unset_line();
	}
	line = org_line;
	line_id = org_line->get_line_id();
	fahrplan_t * new_fpl= new fahrplan_t( org_line->get_fahrplan() );
	setze_fahrplan(new_fpl);
	line->add_convoy(self);
	// force asynchronous recalculation
	welt->set_schedule_counter();
}

/**
* register_with_line
* sets the convoi's line by using the line_id, rather than the line object
* CAUTION: THIS CALL WILL NOT SET A NEW FAHRPLAN!!! IT WILL JUST REGISTER THE CONVOI WITH THE LINE
* @author hsiegeln
*/
void convoi_t::register_with_line(uint16 id)
{
	DBG_DEBUG("convoi_t::register_with_line()","%s registers for %d", name, id);

	linehandle_t new_line = besitzer_p->simlinemgmt.get_line_by_id(id);
	if(new_line.is_bound()) {
		line = new_line;
		line_id = new_line->get_line_id();
		line->add_convoy(self);
	}
}



/**
* unset line
* removes convoy from route without destroying its fahrplan
* @author hsiegeln
*/
void convoi_t::unset_line()
{
	if(line.is_bound()) {
DBG_DEBUG("convoi_t::unset_line()", "removing old destinations from line=%d, fpl=%p",line.get_id(),fpl);
		line->remove_convoy(self);
		line = linehandle_t();
		line_id = UNVALID_LINE_ID;
	}
}



void
convoi_t::prepare_for_new_schedule(fahrplan_t *f)
{
	alte_richtung = fahr->at(0)->gib_fahrtrichtung();
	if(fpl) {
		old_fpl = new fahrplan_t( fpl );
DBG_MESSAGE("convoi_t::prepare_for_new_schedule()","old=%p,fpl=%p,f=%p",old_fpl,fpl,f);
	}

	state = FAHRPLANEINGABE;
	setze_fahrplan(f);

	// Hajo: setze_fahrplan sets state to ROUTING_1
	// need to undo that
	state = FAHRPLANEINGABE;
}

void
convoi_t::book(sint64 amount, int cost_type)
{
	if (cost_type>MAX_CONVOI_COST) {
		// THIS SHOULD NEVER HAPPEN!
		// CHECK CODE
		DBG_MESSAGE("convoi_t::book()", "function was called with cost_type: %i, which is not valid (MAX_CONVOI_COST=%i)", cost_type, MAX_CONVOI_COST);
		return;
	}

	financial_history[0][cost_type] += amount;
	if (line.is_bound()) {
		line->book(amount, simline_t::convoi_to_line_catgory[cost_type] );
	}

	if (cost_type == CONVOI_TRANSPORTED_GOODS) {
		besitzer_p->buche(amount, COST_TRANSPORTED_GOODS);
	}
}

void
convoi_t::init_financial_history()
{
	for (int j = 0; j<MAX_CONVOI_COST; j++) {
		for (int k = MAX_MONTHS-1; k>=0; k--) {
			financial_history[k][j] = 0;
		}
	}
}

sint32
convoi_t::get_running_cost() const
{
	sint32 running_cost = 0;
	for (unsigned i = 0; i<gib_vehikel_anzahl(); i++) {
		running_cost += fahr->at(i)->gib_betriebskosten();
	}
	return running_cost;
}

void
convoi_t::check_pending_updates()
{
	if (line_update_pending!=UNVALID_LINE_ID) {
		linehandle_t line = besitzer_p->simlinemgmt.get_line_by_id(line_update_pending);
		// the line could have been deleted in the meantime
		// if line was deleted ignore line update; convoi will continue with existing schedule
		if (line.is_bound()) {
			int aktuell = fpl->get_aktuell(); // save current position of schedule
			fpl = new fahrplan_t(line->get_fahrplan());
			fpl->set_aktuell(aktuell); // set new schedule current position to old schedule current position
			state = FAHRPLANEINGABE;
		}
		line_update_pending = UNVALID_LINE_ID;
	}
}
