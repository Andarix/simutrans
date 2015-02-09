/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef simbau_h
#define simbau_h

#include "../boden/wege/weg.h"
#include "../tpl/vector_tpl.h"
#include "../simtypes.h"


class weg_besch_t;
class bruecke_besch_t;
class tunnel_besch_t;
class karte_ptr_t;
class player_t;
class grund_t;
class tool_selector_t;


/**
 * way building class with its own route finding
 * @author Hj. Malthaner
 */
class wegbauer_t
{
	static karte_ptr_t welt;
public:
	static const weg_besch_t *leitung_besch;

	static bool register_besch(weg_besch_t *besch);
	static bool alle_wege_geladen();

	// generates timeline message
	static void neuer_monat();

	/**
	 * Finds a way with a given speed limit for a given waytype
	 * @author prissi
	 */
	static const weg_besch_t *  weg_search(const waytype_t wtyp,const sint32 speed_limit, const uint16 time, const weg_t::system_type system_type);

	static const weg_besch_t * get_besch(const char *way_name,const uint16 time=0);

	static const weg_besch_t *get_earliest_way(const waytype_t wtyp);

	static const weg_besch_t *get_latest_way(const waytype_t wtyp);

	static bool waytype_available( const waytype_t wtyp, uint16 time );

	/**
	 * Fill menu with icons of given waytype
	 * @author Hj. Malthaner
	 */
	static void fill_menu(tool_selector_t *tool_selector, const waytype_t wtyp, const weg_t::system_type styp, sint16 ok_sound);

	enum bautyp_t {
		strasse=road_wt,
		schiene=track_wt,
		schiene_tram=tram_wt, // Dario: Tramway
		monorail=monorail_wt,
		maglev=maglev_wt,
		wasser=water_wt,
		luft=air_wt,
		narrowgauge=narrowgauge_wt,
		leitung=powerline_wt,
		river=127,
		bautyp_mask=255,
		bot_flag=0x100,					// do not connect to other ways
		elevated_flag=0x200,			// elevated structure
		terraform_flag=0x400,
		tunnel_flag=0x800				// underground structure
	};

private:
	/// flags used in intern_calc_route, saved in the otherwise unused route_t::ANode->count
	enum build_type_t {
		build_straight = 1,      ///< next step has to be straight
		terraform      = 2,      ///< terraform this tile
		build_tunnel_bridge = 4, ///< bridge/tunnel ends here
	};

	struct next_gr_t
	{
		next_gr_t() {}
		next_gr_t(grund_t* gr_, sint32 cost_, uint8 flag_=0) : gr(gr_), cost(cost_), flag(flag_) {}

		grund_t* gr;
		sint32   cost;
		uint8    flag;
	};
	vector_tpl<next_gr_t> next_gr;

	player_t *player_builder;

	/**
	 * Type of building operation
	 * @author Hj. Malthaner
	 */
	bautyp_t bautyp;

	/**
	 * Type of way to build
	 * @author Hj. Malthaner
	 */
	const weg_besch_t * besch;

	/**
	 * Type of bridges to build (zero=>no bridges)
	 * @author Hj. Malthaner
	 */
	const bruecke_besch_t * bruecke_besch;

	/**
	 * Type of bridges to build (zero=>no bridges)
	 * @author Hj. Malthaner
	 */
	const tunnel_besch_t * tunnel_besch;

	/**
	 * If a way is built on top of another way, should the type
	 * of the former way be kept or replaced (true == keep)
	 * @author Hj. Malthaner
	 */
	bool keep_existing_ways;
	bool keep_existing_faster_ways;
	bool keep_existing_city_roads;

	bool build_sidewalk;

	uint32 maximum;    // hoechste Suchtiefe

	koord3d_vector_t route;
	// index in route with terraformed tiles
	vector_tpl<uint32> terraform_index;

public:
	/* This is the core routine for the way search
	* it will check
	* A) allowed step
	* B) if allowed, calculate the cost for the step from from to to
	* @author prissi
	*/
	bool is_allowed_step(const grund_t *from, const grund_t *to, sint32 *costs);

private:
	// checks, if we can built a bridge here ...
	// may modify next_gr array!
	void check_for_bridge(const grund_t* parent_from, const grund_t* from, const vector_tpl<koord3d> &ziel);

	sint32 intern_calc_route(const vector_tpl<koord3d> &start, const vector_tpl<koord3d> &ziel);
	void intern_calc_straight_route(const koord3d start, const koord3d ziel);

	// runways need to meet some special conditions enforced here
	bool intern_calc_route_runways(koord3d start, const koord3d ziel);

	void baue_tunnel_und_bruecken();

	// adds the ground before underground construction (always called before the following construction routines)
	bool baue_tunnelboden();

	// adds the grounds for elevated tracks
	void baue_elevated();

	void baue_strasse();
	void baue_schiene();
	void baue_leitung();
	void baue_fluss();

	uint32 calc_distance( const koord3d &pos, const koord3d &mini, const koord3d &maxi );

public:
	const koord3d_vector_t &get_route() const { return route; }

	uint32 get_count() const { return route.get_count(); }

	sint32 n;

	/**
	 * If a way is built on top of another way, should the type
	 * of the former way be kept or replaced (true == keep)
	 * @author Hj. Malthaner
	 */
	void set_keep_existing_ways(bool yesno);

	/* If a way is built on top of another way, should the type
	 * of the former way be kept or replaced, if the current way is faster (true == keep)
	 * @author Hj. Malthaner
	 */
	void set_keep_existing_faster_ways(bool yesno);

	/* Always keep city roads (for AI)
	 * @author prissi
	 */
	void set_keep_city_roads(bool yesno) { keep_existing_city_roads = yesno; }

	void set_build_sidewalk(bool yesno) { build_sidewalk = yesno; }

	void route_fuer(bautyp_t wt, const weg_besch_t * besch, const tunnel_besch_t *tunnel_besch=NULL, const bruecke_besch_t *bruecke_besch=NULL);

	void set_maximum(uint32 n) { maximum = n; }

	wegbauer_t(player_t *player_);

	void calc_straight_route(const koord3d start, const koord3d ziel);
	void calc_route(const koord3d &start3d, const koord3d &ziel);
	void calc_route(const vector_tpl<koord3d> &start3d, const vector_tpl<koord3d> &ziel);

	/* returns the amount needed to built this way
	* author prissi
	*/
	sint64 calc_costs();

	bool check_crossing(const koord zv, const grund_t *bd,waytype_t wtyp, const player_t *player_) const;
	bool check_for_leitung(const koord zv, const grund_t *bd) const;
	// allowed owner?
	bool check_owner( const player_t *player1, const player_t *player2 ) const;
	// checks whether buildings on the tile allow to leave in direction dir
	bool check_building( const grund_t *to, const koord dir ) const;
	// allowed slope?
	bool check_slope( const grund_t *from, const grund_t *to );

	bool check_terraforming( const grund_t *from, const grund_t *to, uint8* new_from_slope=NULL, uint8* new_to_slope=NULL);
	void do_terraforming();

	void baue();
};

ENUM_BITSET(wegbauer_t::bautyp_t);

#endif
