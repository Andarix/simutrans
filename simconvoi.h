/**
 * Header Datei f�r dir convoi_t Klasse f�r Fahrzeugverb�nde
 * von Hansj�rg Malthaner
 */

#ifndef simconvoi_h
#define simconvoi_h

#include "simtypes.h"
#include "linehandle_t.h"

#include "ifc/sync_steppable.h"

#include "dataobj/route.h"
#include "tpl/array_tpl.h"

#include "convoihandle_t.h"
#include "halthandle_t.h"

#define MAX_CONVOI_COST   5 // Total number of cost items
#define MAX_MONTHS     12 // Max history
#define MAX_CONVOI_NON_MONEY_TYPES 2 // number of non money types in convoi's financial statistic
#define CONVOI_CAPACITY   0 // the amount of ware that could be transported, theoretically
#define CONVOI_TRANSPORTED_GOODS 1 // the amount of ware that has been transported
#define CONVOI_REVENUE		2 // the income this CONVOI generated
#define CONVOI_OPERATIONS         3 // the cost of operations this CONVOI generated
#define CONVOI_PROFIT             4 // total profit of this convoi

class depot_t;
class karte_t;
class spieler_t;
class convoi_info_t;
class vehikel_t;
class vehikel_besch_t;
class fahrplan_t;
class cbuffer_t;

/**
 * Basisklasse f�r alle Fahrzeugverb�nde. Convois k�nnnen �ber Zeiger
 * oder Handles angesprochen werden. Zeiger sind viel schneller, daf�r
 * k�nnen Handles gepr�ft werden, ob das Ziel noch vorhanden ist.
 *
 * @author Hj. Malthaner
 */
class convoi_t : public sync_steppable
{
public:
	/* Konstanten
	* @author prissi
	*/
	enum { max_vehicle=4, max_rail_vehicle = 24 };

	enum states {INITIAL,
		FAHRPLANEINGABE,
		ROUTING_1,
		DUMMY4, DUMMY5,
		NO_ROUTE,
		DRIVING,
		LOADING,
		WAITING_FOR_CLEARANCE,
		WAITING_FOR_CLEARANCE_ONE_MONTH,
		CAN_START, CAN_START_ONE_MONTH,
		SELF_DESTRUCT,
		WAITING_FOR_CLEARANCE_TWO_MONTHS,
		CAN_START_TWO_MONTHS,
		LEAVING_DEPOT,
		ENTERING_DEPOT
	};

private:
	/**
	* Route of this convoi - a sequence of coordinates. Actually
	* the path of the first vehicle
	* @author Hj. Malthaner
	*/
	route_t route;

	/**
	* assigned line
	* @author hsiegeln
	*/
	linehandle_t line;
	uint16 line_id;

	/**
	* Name of the convoi.
	* @see setze_name
	* @author V. Meyer
	*/
	uint8 name_offset;
	char name_and_id[128];

	/**
	* Information window for ourselves.
	* @author Hj. Malthaner
	*/
	convoi_info_t *convoi_info;

	/**
	* Alle vehikel-fahrplanzeiger zeigen hierauf
	* @author Hj. Malthaner
	*/
	fahrplan_t *fpl;

	/**
	* loading_level was ladegrad before. Actual percentage loaded for loadable vehicles (station length!).
	* needed as int, since used by the gui
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	sint32 loading_level;

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	* needed as int, since used by the gui
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	sint32 loading_limit;

	/**
	* The vehicles of this convoi
	*
	* @author Hj. Malthaner
	*/
	array_tpl<vehikel_t*> fahr;

	/**
	* Convoi owner
	* @author Hj. Malthaner
	*/
	spieler_t *besitzer_p;

	/**
	* Current map
	* @author Hj. Malthaner
	*/
	karte_t   *welt;

 	/**
	* the convoi is being withdrawn from service
	* @author kierongreen
	*/
	bool withdraw;

 	/**
	* nothing will be loaded onto this convoi
	* @author kierongreen
	*/
	bool no_load;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	* @author prissi
	*/
	bool freight_info_resort;

	// true, if at least one vehicle of a convoi is obsolete
	bool has_obsolete;

	// ture, if there is at least one engine that requires catenary
	bool is_electric;

	/**
	* the convoi caches its freight info; it is only recalculation after loading or resorting
	* @author prissi
	*/
	uint8 freight_info_order;

	/**
	* Number of vehicles in this convoi.
	* @author Hj. Malthaner
	*/
	uint8 anz_vehikel;

	/* Number of steps the current convoi did already
	 * (only needed for leaving/entering depot)
	 */
	sint16 steps_driven;

	/**
	* Gesamtleistung. Wird nicht gespeichert, sondern aus den Einzelleistungen
	* errechnet.
	* @author Hj. Malthaner
	*/
	uint32 sum_leistung;

	/**
	* Gesamtleistung mit Gear. Wird nicht gespeichert, sondern aus den Einzelleistungen
	* errechnet.
	* @author prissi
	*/
	sint32 sum_gear_und_leistung;

	/* sum_gewicht: leergewichte aller vehicles *
	* sum_gesamtgewicht: gesamtgewichte aller vehicles *
	* Werden nicht gespeichert, sondern aus den Einzelgewichten
	* errechnet beim beladen/fahren.
	* @author Hj. Malthaner, prissi
	*/
	sint32 sum_gewicht;
	sint32 sum_gesamtgewicht;

	/**
	* Lowest top speed of all vehicles. Doesn't get saved, but calculated
	* from the vehicles data
	* @author Hj. Malthaner
	*/
	sint32 min_top_speed;

	/**
	* this give the index of the next signal or the end of the route
	* convois will slow down before it, if this is not a waypoint or the cannot pass
	* The slowdown ist done by the vehicle routines
	* @author prissi
	*/
	uint16 next_stop_index;

	/**
	* manchmal muss eine bestimmte Zeit gewartet werden.
	* wait_lock bestimmt wie lange gewartet wird (in ms).
	* @author Hanjs�rg Malthaner
	*/
	sint32 wait_lock;

	/**
	* akkumulierter gewinn �ber ein jahr hinweg
	* @author Hanjs�rg Malthaner
	*/
	sint64 jahresgewinn;

	/**
	* Set, when there was a income calculation (avoids some cheats)
	* Since 99.15 it will stored directly in the vehikel_t
	* @author prissi
	*/
	koord3d last_stop_pos;

	// things for the world record
	sint32 max_record_speed; // current convois fastest speed ever
	koord record_pos;

	// needed for speed control/calculation
	sint32 akt_speed_soll;	// should go this
	sint32 akt_speed;			// goes this at the moment
	sint32 sp_soll;					// steps to go
	sint32 previous_delta_v;		// Stores the previous delta_v value; otherwise these digits are lost during calculation and vehicle do not accelrate

	uint32 next_wolke;	// time to next smoke

	enum states state;

	ribi_t::ribi alte_richtung;

	/**
	* Initialize all variables with default values.
	* Each constructor must call this method first!
	* @author Hj. Malthaner
	*/
	void init(karte_t *welt, spieler_t *sp);

	/**
	* Berechne route von Start- zu Zielkoordinate
	* @author Hanjs�rg Malthaner
	*/
	int  drive_to(koord3d s, koord3d z);

	/**
	* Berechne route zum n�chsten Halt
	* @author Hanjs�rg Malthaner
	*/
	void drive_to_next_stop();

	/**
	* Setup vehicles for moving in same direction than before
	* if the direction is the same as before
	* @author Hanjs�rg Malthaner
	*/
	bool can_go_alte_richtung();

	/**
	* Mark first and last vehicle.
	* @author Hanjs�rg Malthaner
	*/
	void setze_erstes_letztes();

	// returns the index of the vehikel at position length (16=1 tile)
	int get_vehicle_at_length(uint16);

	/**
	* calculate income for last hop
	* only used for entering depot or recalculating routes when a schedule window is opened
	* @author Hj. Malthaner
	*/
	void calc_gewinn();

	/**
	* Recalculates loading level and limit.
	* While driving loading_limit will be set to 0.
	* @author Volker Meyer
	* @date  20.06.2003
	*/
	void calc_loading();

	/* Calculates (and sets) akt_speed
	 * needed for driving, entering and leaving a depot)
	 */
	void calc_acceleration(long delta_t);

	/**
	* Convoi haelt an Haltestelle und setzt quote fuer Fracht
	* @author Hj. Malthaner
	*/
	void hat_gehalten(koord k, halthandle_t halt);

	/*
	* struct holds new financial history for convoi
	* @author hsiegeln
	*/
	sint64 financial_history[MAX_MONTHS][MAX_CONVOI_COST];

	/**
	* initialize the financial history
	* @author hsiegeln
	*/
	void init_financial_history();

	/**
	* holds id of line with pendig update
	* -1 if no pending update
	* @author hsiegeln
	*/
	uint16 line_update_pending;

	/**
	* the koordinate of the home depot of this convoi
	* the last depot visited is considered beeing the home depot
	* @author hsiegeln
	*/
	koord3d home_depot;

public:
	route_t* get_route() { return &route; }

	/**
	* Checks if this convoi has a driveable route
	* @author Hanjs�rg Malthaner
	*/
	bool hat_keine_route() const;

	/**
	* get line
	* @author hsiegeln
	*/
	linehandle_t get_line() const {return line;}

	/* true, if electrification needed for this convoi */
	bool needs_electrification() const { return is_electric; }

	/**
	* set line
	* @author hsiegeln
	*/
	void set_line(linehandle_t );

	/**
	* registers the convoy with a line, but does not apply the line's fahrplan!
	* used only during convoi restoration from savegame!
	* @author hsiegeln
	*/
	void register_with_line(uint16 line_id);

	/**
	* unset line -> remove cnv from line
	* @author hsiegeln
	*/
	void unset_line();

	/**
	* get state
	* @author hsiegeln
	*/
	int get_state() { return state; }

	/**
	* get state
	* @author hsiegeln
	*/
	bool is_waiting() { return state==WAITING_FOR_CLEARANCE  ||  state==WAITING_FOR_CLEARANCE_ONE_MONTH  ||  state==WAITING_FOR_CLEARANCE_TWO_MONTHS;}

	/**
	* reset state to no error message
	* @author prissi
	*/
	void reset_waiting() { state=WAITING_FOR_CLEARANCE; }

	/**
	* Das Handle f�r uns selbst. In Anlehnung an 'this' aber mit
	* allen checks beim Zugriff.
	* @author Hanjs�rg Malthaner
	*/
	convoihandle_t self;

	/**
	* Der Gewinn in diesem Jahr
	* @author Hanjs�rg Malthaner
	*/
	const sint64 & gib_jahresgewinn() const {return jahresgewinn;}

	/**
	* returns the total running cost for all vehicles in convoi
	* @author hsiegeln
	*/
	sint32 get_running_cost() const;

	/**
	* Constructor for loading from file,
	* @author Hj. Malthaner
	*/
	convoi_t(karte_t *welt, loadsave_t *file);

	convoi_t(spieler_t* sp);

	virtual ~convoi_t();

	/**
	* Load or save this convoi data
	* @author Hj. Malthaner
	*/
	void rdwr(loadsave_t *file);

	void laden_abschliessen();

	void rotate90( const sint16 y_size );

	/**
	* Called if a vehicle enters a depot
	* @author Hanjs�rg Malthaner
	*/
	void betrete_depot(depot_t *dep);

	/**
	* @return Current map.
	* @author Hj. Malthaner
	*/
	karte_t* get_welt() { return welt; }

	/**
	* Gibt Namen des Convois zur�ck.
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	const char *gib_internal_name() const {return name_and_id+name_offset;}

	/**
	* Allows editing ...
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	char *access_internal_name() {return name_and_id+name_offset;}

	/**
	* Gibt Namen des Convois zur�ck.
	* @return Name des Convois
	* @author Hj. Malthaner
	*/
	const char *gib_name() const {return name_and_id;}

	/**
	* Sets the name. Copies name into this->name and translates it.
	* @author V. Meyer
	*/
	void setze_name(const char *name);

	/**
	 * Gibt die Position des Convois zur�ck.
	 * @return Position des Convois
	 * @author Hj. Malthaner
	 */
	koord3d gib_pos() const;

	/**
	 * sets the current target speed
	 * set from the first vehicle, and takes into account all speed limits, brakes at stations etc.
	 * @author Hj. Malthaner
	 */
	void setze_akt_speed_soll(sint32 set_akt_speed) { akt_speed_soll = min( set_akt_speed, min_top_speed ); }

	/**
	 * @return current speed, this might be different from topspeed
	 *         actual currently set speed.
	 * @author Hj. Malthaner
	 */
	const sint32& gib_akt_speed() const { return akt_speed; }

	/**
	 * @return total power of this convoi
	 * @author Hj. Malthaner
	 */
	const uint32 & gib_sum_leistung() const {return sum_leistung;}
	const sint32 & gib_min_top_speed() const {return min_top_speed;}
	const sint32 & gib_sum_gewicht() const {return sum_gewicht;}
	const sint32 & gib_sum_gesamtgewicht() const {return sum_gesamtgewicht;}

	/**
	 * Vehicles of the convoi add their running cost by using this
	 * method
	 * @author Hj. Malthaner
	 */
	void add_running_cost(sint32 cost);

	/**
	 * moving the veicles of a convoi and acceleration/deacceleration
	 * all other stuff => convoi_t::step()
	 * @author Hj. Malthaner
	 */
	bool sync_step(long delta_t);

	/**
	 * All things like route search or laoding, that may take a little
	 * @author Hj. Malthaner
	 */
	void step();

	/**
	 * Calculates total weight of freight in KG
	 * @author Hj. Malthaner
	 */
	int calc_freight_weight() const;

	/**
	* setzt einen neuen convoi in fahrt
	* @author Hj. Malthaner
	*/
	void start();

	void ziel_erreicht(); ///< Called, when the first vehicle reaches the target

	/**
	* Ein Fahrzeug hat ein Problem erkannt und erzwingt die
	* Berechnung einer neuen Route
	* @author Hanjs�rg Malthaner
	*/
	void suche_neue_route();

	/**
	* Wartet bis Fahrzeug 0 freie Fahrt meldet
	* will be called during a hop_check, if the road/track is blocked
	* @author Hj. Malthaner
	*/
	void warten_bis_weg_frei(int restart_speed);

	/**
	* @return Vehicle count
	* @author Hj. Malthaner
	*/
	unsigned int gib_vehikel_anzahl() const { return anz_vehikel; }

	/**
	 * @return Vehicle at position i
	 */
	vehikel_t* gib_vehikel(uint16 i) const { return fahr[i]; }

	/**
	* Adds a vehicel at the start or end of the convoi.
	* @author Hj. Malthaner
	*/
	bool add_vehikel(vehikel_t *v, bool infront = false);

	/**
	* Removes vehicles at position i
	* @author Hj. Malthaner
	*/
	vehikel_t * remove_vehikel_bei(unsigned short i);

	/**
	* Sets a schedule
	* @author Hj. Malthaner
	*/
	bool setze_fahrplan(fahrplan_t *f);

	/**
	* @return Current schedule
	* @author Hj. Malthaner
	*/
	fahrplan_t* gib_fahrplan() const { return fpl; }

	/**
	* Creates a new schedule if there isn't one already.
	* @return Current schedule
	* @author Hj. Malthaner
	*/
	fahrplan_t * erzeuge_fahrplan();

	/**
	* @return Owner of this convoi
	* @author Hj. Malthaner
	*/
	spieler_t * gib_besitzer() { return besitzer_p; }

	/**
	* Opens an information window
	* @author Hj. Malthaner
	* @see simwin
	*/
	void zeige_info();

	/**
	* @return Einen Beschreibungsstring f�r das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	* @see simwin
	*/
	void info(cbuffer_t & buf) const;

	/**
	* @param buf the buffer to fill
	* @return Freight dscription text (buf)
	* @author Hj. Malthaner
	*/
	void get_freight_info(cbuffer_t & buf);
	void set_sortby(uint8 order);
	uint8 get_sortby() const { return freight_info_order; }

	/**
	* Opens the schedule window
	* @author Hj. Malthaner
	* @see simwin
	*/
	void open_schedule_window();

	static bool pruefe_vorgaenger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter);
	static bool pruefe_nachfolger(const vehikel_besch_t *vor, const vehikel_besch_t *hinter);

	/**
	* pruefe ob Beschraenkungen fuer alle Fahrzeuge erfuellt sind
	* @author Hj. Malthaner
	*/
	bool pruefe_alle();

	/**
	* Kontrolliert Be- und Entladen.
	* V.Meyer: returns nothing
	* @author Hj. Malthaner
	*/
	void laden();

	/**
	* Setup vehicles before starting to move
	* @author Hanjs�rg Malthaner
	*/
	void vorfahren();

	/**
	* Calculate the total value of the convoi as the sum of all vehicle values.
	* @author Volker Meyer
	* @date  09.06.2003
	*/
	uint32 calc_restwert() const;

	/**
	* Check if this convoi has entered a depot.
	* @author Volker Meyer
	* @date  09.06.2003
	*/
	bool in_depot() const { return state == INITIAL; }

	/**
	* loading_level was ladegrad before. Actual percentage loaded of loadable
	* vehicles.
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	const sint32 &get_loading_level() const { return loading_level; }

	/**
	* At which loading level is the train allowed to start? 0 during driving.
	* @author Volker Meyer
	* @date  12.06.2003
	*/
	const sint32 &get_loading_limit() const { return loading_limit; }

	/**
	* Schedule convoid for self destruction. Will be executed
	* upon next sync step
	* @author Hj. Malthaner
	*/
	void self_destruct();

	/**
	* Helper method to remove convois from the map that cannot
	* removed normally (i.e. by sending to a depot) anymore.
	* This is a workaround for bugs in the game.
	* @author Hj. Malthaner
	* @date  12-Jul-03
	*/
	void destroy();

	/**
	* Debug info nach stderr
	* @author Hj. Malthaner
	* @date 04-Sep-03
	*/
	void dump() const;

	/**
	* prepares the convoi to receive a new schedule
	* @author hsiegeln
	*/
	void prepare_for_new_schedule(fahrplan_t *);

	/**
	* book a certain amount into the convois financial history
	* is called from vehicle during un/load
	* @author hsiegeln
	*/
	void book(sint64 amount, int cost_type);

	/**
	* return a pointer to the financial history
	* @author hsiegeln
	*/
	sint64* get_finance_history() { return *financial_history; }

	/**
	* return a specified element from the financial history
	* @author hsiegeln
	*/
	sint64 get_finance_history(int month, int cost_type) { return financial_history[month][cost_type]; }

	/**
	* only purpose currently is to roll financial history
	* @author hsiegeln
	*/
	void new_month();

	/**
	 * Methode fuer jaehrliche aktionen
	 * @author Hj. Malthaner
	 */
	void neues_jahr();

	int get_line_update_pending() { return line_update_pending; }

	void set_line_update_pending(int id) { line_update_pending = id; }

	void check_pending_updates();

	void set_home_depot(koord3d hd) { home_depot = hd; }

	koord3d get_home_depot() { return home_depot; }

	/**
	* this give the index of the next signal or the end of the route
	* convois will slow down before it, if this is not a waypoint or the cannot pass
	* The slowdown ist done by the vehicle routines
	* @author prissi
	*/
	uint16 get_next_stop_index() const {return next_stop_index;}
	void set_next_stop_index(uint16 n) {next_stop_index=n;}

	/* the current state of the convoi */
	uint8 get_status_color() const;

	bool has_obsolete_vehicles() const { return has_obsolete; }

	bool get_withdraw() const { return withdraw; }

	void set_withdraw(bool new_withdraw) { withdraw = new_withdraw; }

	bool get_no_load() const { return no_load; }

	void set_no_load(bool new_no_load) { no_load = new_no_load; }
};

#endif
