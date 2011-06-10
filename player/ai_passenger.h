/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Simple passenger transport AI
 */

#include "ai.h"


class ai_passenger_t : public ai_t
{
private:
	enum zustand {
		NR_INIT,
		NR_SAMMLE_ROUTEN,
		NR_BAUE_ROUTE1,
		NR_BAUE_AIRPORT_ROUTE,
		NR_BAUE_STRASSEN_ROUTE,
		NR_BAUE_WATER_ROUTE,
		NR_BAUE_CLEAN_UP,
		NR_SUCCESS,
		NR_WATER_SUCCESS,
		CHECK_CONVOI
	};

	// vars f�r die KI
	zustand state;

	/*
	 * if this is false, this AI won't use rails
	 * @author prissi
	 */
	bool air_transport;

	/*
	 * if this is false, this AI won't use ships
	 * @author prissi
	 */
	bool ship_transport;

	// we will use this vehicle!
	const vehikel_besch_t *road_vehicle;

	// and the convoi will run on this track:
	const weg_besch_t *road_weg ;

	// time to wait before next contruction
	sint32 next_contruction_steps;

	// the shorter the faster construction will occur
	sint32 construction_speed;

	/* start and end stop position (and their size) */
	koord platz1, platz2;

	const stadt_t *start_stadt;
	const stadt_t *end_stadt;	// target is town
	const gebaeude_t *end_ausflugsziel;
	fabrik_t *ziel;

	halthandle_t  get_our_hub( const stadt_t *s ) const;

	koord find_area_for_hub( const koord lo, const koord ru, const koord basis ) const;
	koord find_place_for_hub( const stadt_t *s ) const;

	/* builds harbours and ferrys
	 * @author prissi
	 */
	koord find_harbour_pos(karte_t* welt, const stadt_t *s );
	bool create_water_transport_vehikel(const stadt_t* start_stadt, const koord target_pos);

	// builds a simple 3x3 three stop airport with town connection road
	halthandle_t build_airport(const stadt_t* city, koord pos, int rotate);

	/* builts airports and planes
	 * @author prissi
	 */
	bool create_air_transport_vehikel(const stadt_t *start_stadt, const stadt_t *end_stadt);

	// helper function for bus stops intown
	void walk_city( linehandle_t &line, grund_t *&start, const int limit );

	// tries to cover a city with bus stops that does not overlap much and cover as much as possible
	void cover_city_with_bus_route(koord start_pos, int number_of_stops);

	void create_bus_transport_vehikel(koord startpos,int anz_vehikel,koord *stops,int anzahl,bool do_wait);

public:
	ai_passenger_t(karte_t *wl, uint8 nr);

	virtual ~ai_passenger_t() {}

	// this type of AIs identifier
	virtual uint8 get_ai_id() const { return AI_PASSENGER; }

	virtual void bescheid_vehikel_problem(convoihandle_t cnv,const koord3d ziel);

	virtual void rdwr(loadsave_t *file);

	virtual void laden_abschliessen();

	bool set_active( bool b );

	void step();
};
