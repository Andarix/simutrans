/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef _simdepot_h
#define _simdepot_h

#include "tpl/slist_tpl.h"
#include "dings/gebaeude.h"
#include "convoihandle_t.h"
#include "simline.h"


class karte_t;
class vehikel_t;
class depot_frame_t;
class vehikel_besch_t;


/**
 * In Depots werden Fahrzeuge gekauft, gewartet, verkauft und gelagert.
 * @author Hj. Malthaner
 */
class depot_t : public gebaeude_t
{
protected:
	/**
	 * Reworked depot data!
	 *
	 * It can now contain any number of vehicles bought by the user (as before).
	 * And it can hold any number of convois (before only one).
	 * It is possible to have 0 convois in a depot, but an empty one shall be
	 * automatically created, when necessary.
	 * Convois are numbered 0...(n-1).
	 * Vehicles are accessed by type.
	 *
	 * @author Volker Meyer
	 * @date  30.05.2003
	 */
	slist_tpl<vehikel_t *> vehicles;
	slist_tpl<convoihandle_t> convois;

	void rdwr_vehikel(slist_tpl<vehikel_t*> &list, loadsave_t *file);

	static slist_tpl<depot_t *> all_depots;

public:
	// finds the next/previous depot relative to the current position
	static depot_t *find_depot( koord3d start, const ding_t::typ depot_type, const spieler_t *sp, bool next);

	static const slist_tpl<depot_t *>& get_depot_list() { return all_depots; }

	depot_t(karte_t *welt,loadsave_t *file);
	depot_t(karte_t *welt, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t);
	virtual ~depot_t();

	void call_depot_tool( char tool, convoihandle_t cnv, const char *extra );

	virtual simline_t::linetype get_line_type() const = 0;

	void rdwr(loadsave_t *file);

	// text for the tabs is defaulted to the train names
	virtual const char * get_electrics_name() { return "Electrics_tab"; };
	virtual const char * get_passenger_name() { return "Pas_tab"; }
	virtual const char * get_zieher_name() { return "Lokomotive_tab"; }
	virtual const char * get_haenger_name() { return "Waggon_tab"; }

	vehikel_t* find_oldest_newest(const vehikel_besch_t* besch, bool old);

	/**
	 * Access to convoi list.
	 * @author Volker Meyer
	 * @date  30.05.2003
	 */
	unsigned convoi_count() const { return convois.get_count(); }

	convoihandle_t get_convoi(unsigned int icnv) const { return icnv < convoi_count() ? convois.at(icnv) : convoihandle_t(); }

	convoihandle_t add_convoi();

	/*
	 * copies convoi and its schedule or line
	 * @author hsiegeln
	 */
	convoihandle_t copy_convoi(convoihandle_t old_cnv);

	/**
	 * Let convoi leave the depot.
	 * If not possible, a message is displayed and the function returns false.
	 * @param if local_execution is true, this method creates pop-ups in case of errors
	 * @author Volker Meyer, Dwachs
	 * @date  09.06.2003 / 27.04.2010
	 */
	bool start_convoi(convoihandle_t cnv, bool local_execution);

	/**
	 * Destroy the convoi and put the vehicles in the vehicles list (sell==false),
	 * or sell all immediately (sell==true).
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	bool disassemble_convoi(convoihandle_t cnv, bool sell);

	/**
	 * Remove vehicle from vehicle list and add it to the convoi. Two positions
	 * are possible - in front or at the rear.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void append_vehicle(convoihandle_t cnv, vehikel_t* veh, bool infront);

	/**
	 * Remove the vehicle at given position from the convoi and put it in the
	 * vehicle list.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void remove_vehicle(convoihandle_t cnv, int ipos);

	/**
	 * Access to vehicles not bound to a convoi. They are not ordered
	 * in any way.
	 * @author Volker Meyer
	 * @date  30.05.2003
	 */
	slist_tpl<vehikel_t*> const& get_vehicle_list() { return vehicles; }

	/**
	 * A new vehicle is bought and added to the vehicle list.
	 * The new vehicle in the list is returned.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	vehikel_t* buy_vehicle(const vehikel_besch_t* info);

	/**
	 * Sell a vehicle from the vehicle list.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void sell_vehicle(vehikel_t* veh);

	/**
	 * Access to vehicle types which can be bought in the depot.
	 * @author Volker Meyer
	 */
	slist_tpl<const vehikel_besch_t*>* get_vehicle_type();

	/**
	 * Returns the waytype for a certain vehicle; only way to distinguish differnt depots ...
	 * @author prissi
	 */
	virtual waytype_t get_wegtyp() const { return invalid_wt; }

	/**
	 * A convoi arrived at the depot and is added to the convoi list.
	 * If fpl_adjust is true, the current depot is removed from schedule.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	void convoi_arrived(convoihandle_t cnv, bool fpl_adjust);

	/**
	 * Parameters to determine layout and behaviour of the depot_frame_t.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	virtual int get_x_grid() const = 0;
	virtual int get_y_grid() const = 0;
	virtual int get_x_placement() const = 0;
	virtual int get_y_placement() const = 0;
	virtual unsigned get_max_convoi_length() const = 0;

	/**
	 * Opens a new info window for that object.
	 *
	 * @author Hj. Malthaner
	 */
	void zeige_info();

	/**
	 * Can object be removed?
	 * @return NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char * ist_entfernbar(const spieler_t *sp);

	/**
	 * identifies the oldest vehicle of a certain type
	 * @return NULL if no vehicle is found
	 * @author hsiegeln (stolen from Hajo)
	 */
	vehikel_t* get_oldest_vehicle(const vehikel_besch_t* besch);

	/*
	 * sets/gets the line that was selected the last time in the depot dialog
	 */
	void set_selected_line(const linehandle_t sel_line);
	linehandle_t get_selected_line();


	/*
	 * Will update all depot_frame_t (new vehicles!)
	 */
	static void update_all_win();
	static void neuer_monat();

	/*
	 * Update the depot_frame_t.
	 */
	void update_win();

private:
	linehandle_t selected_line;
};


/**
 * Depots for train vehicles.
 *
 * @author Hj. Malthaner
 * @see depot_t
 * @see gebaeude_t
 */
class bahndepot_t : public depot_t
{
public:
	bahndepot_t(karte_t *welt, loadsave_t *file) : depot_t(welt,file) {}
	bahndepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t) : depot_t(welt,pos,sp,t) {}

	virtual simline_t::linetype get_line_type() const { return simline_t::trainline; }

	void rdwr_vehicles(loadsave_t *file) { depot_t::rdwr_vehikel(vehicles,file); }

	/**
	 * Parameters to determine layout and behaviour of the depot_frame_t.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	int get_x_placement() const {return -25; }
	int get_y_placement() const {return -28; }
	int get_x_grid() const { return 24; }
	int get_y_grid() const { return 24; }
	unsigned get_max_convoi_length() const;

	virtual waytype_t get_wegtyp() const {return track_wt;}
	virtual ding_t::typ get_typ() const { return bahndepot; }
	virtual const char *get_name() const {return "Bahndepot"; }
};


class tramdepot_t : public bahndepot_t
{
public:
	tramdepot_t(karte_t *welt, loadsave_t *file):bahndepot_t(welt,file) {}
	tramdepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t): bahndepot_t(welt,pos,sp,t) {}

	virtual simline_t::linetype get_line_type() const { return simline_t::tramline; }

	virtual waytype_t get_wegtyp() const {return tram_wt;}
	virtual ding_t::typ get_typ() const { return tramdepot; }
	virtual const char *get_name() const {return "Tramdepot"; }
};

class monoraildepot_t : public bahndepot_t
{
public:
	monoraildepot_t(karte_t *welt, loadsave_t *file):bahndepot_t(welt,file) {}
	monoraildepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t): bahndepot_t(welt,pos,sp,t) {}

	virtual simline_t::linetype get_line_type() const { return simline_t::monorailline; }

	virtual waytype_t get_wegtyp() const {return monorail_wt;}
	virtual ding_t::typ get_typ() const { return monoraildepot; }
	virtual const char *get_name() const {return "Monoraildepot"; }
};

class maglevdepot_t : public bahndepot_t
{
public:
	maglevdepot_t(karte_t *welt, loadsave_t *file):bahndepot_t(welt,file) {}
	maglevdepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t): bahndepot_t(welt,pos,sp,t) {}

	virtual simline_t::linetype get_line_type() const { return simline_t::maglevline; }

	virtual waytype_t get_wegtyp() const {return maglev_wt;}
	virtual ding_t::typ get_typ() const { return maglevdepot; }
	virtual const char *get_name() const {return "Maglevdepot"; }
};

class narrowgaugedepot_t : public bahndepot_t
{
public:
	narrowgaugedepot_t(karte_t *welt, loadsave_t *file):bahndepot_t(welt,file) {}
	narrowgaugedepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t): bahndepot_t(welt,pos,sp,t) {}

	virtual simline_t::linetype get_line_type() const { return simline_t::narrowgaugeline; }

	virtual waytype_t get_wegtyp() const {return narrowgauge_wt;}
	virtual ding_t::typ get_typ() const { return narrowgaugedepot; }
	virtual const char *get_name() const {return "Narrowgaugedepot"; }
};

/**
 * Depots for street vehicles
 *
 * @author Hj. Malthaner
 * @see depot_t
 * @see gebaeude_t
 */
class strassendepot_t : public depot_t
{
protected:
	virtual const char * get_passenger_name() { return "Bus_tab"; }
	virtual const char * get_electrics_name() { return "TrolleyBus_tab"; }
	virtual const char * get_zieher_name() { return "LKW_tab"; }
	virtual const char * get_haenger_name() { return "Anhaenger_tab"; }

public:
	strassendepot_t(karte_t *welt, loadsave_t *file) : depot_t(welt,file) {}
	strassendepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t) : depot_t(welt,pos,sp,t) {}

	virtual simline_t::linetype get_line_type() const { return simline_t::truckline; }

	/**
	 * Parameters to determine layout and behaviour of the depot_frame_t.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	int get_x_placement() const { return -20; }
	int get_y_placement() const { return -25; }
	int get_x_grid() const { return 24; }
	int get_y_grid() const { return 24; }
	unsigned get_max_convoi_length() const { return 4; }

	virtual waytype_t get_wegtyp() const {return road_wt; }
	ding_t::typ get_typ() const { return strassendepot; }
	const char *get_name() const {return "Strassendepot";}
};


/**
 * Depots for ships
 *
 * @author Hj. Malthaner
 * @see depot_t
 * @see gebaeude_t
 */
class schiffdepot_t : public depot_t
{
protected:
	virtual const char * get_passenger_name() { return "Ferry_tab"; }
	virtual const char * get_zieher_name() { return "Schiff_tab"; }
	virtual const char * get_haenger_name() { return "Schleppkahn_tab"; }

public:
	schiffdepot_t(karte_t *welt, loadsave_t *file) : depot_t(welt,file) {}
	schiffdepot_t(karte_t *welt, koord3d pos, spieler_t *sp, const haus_tile_besch_t *t) : depot_t(welt,pos,sp,t) {}

	virtual simline_t::linetype get_line_type() const { return simline_t::shipline; }

	/**
	 * Parameters to determine layout and behaviour of the depot_frame_t.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	int get_x_placement() const { return -1; }
	int get_y_placement() const { return -11; }
	int get_x_grid() const { return 60; }
	int get_y_grid() const { return 46; }

	unsigned get_max_convoi_length() const { return 4; }
	virtual waytype_t get_wegtyp() const {return water_wt; }
	ding_t::typ get_typ() const { return schiffdepot; }
	const char *get_name() const {return "Schiffdepot";}
};


/**
 * Depots for aircrafts
 */
class airdepot_t : public depot_t
{
protected:
	virtual const char * get_zieher_name() { return "aircraft_tab"; }
	virtual const char * get_passenger_name() { return "Flug_tab"; }

public:
	airdepot_t(karte_t *welt, loadsave_t *file) : depot_t(welt,file) {}
	airdepot_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t) : depot_t(welt,pos,sp,t) {}

	virtual simline_t::linetype get_line_type() const { return simline_t::airline; }

	/**
	 * Parameters to determine layout and behaviour of the depot_frame_t.
	 * @author Volker Meyer
	 * @date  09.06.2003
	 */
	int get_x_placement() const {return -10; }
	int get_y_placement() const {return -23; }
	int get_x_grid() const { return 36; }
	int get_y_grid() const { return 36; }
	unsigned get_max_convoi_length() const { return 1; }

	virtual waytype_t get_wegtyp() const { return air_wt; }
	ding_t::typ get_typ() const { return airdepot; }
	const char *get_name() const {return "Hangar";}
};

#endif
