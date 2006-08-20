/*
 * route.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef route_h
#define route_h

#ifndef koord3d_h
#include "koord3d.h"
#endif

#ifndef simdebug_h
#include "../simdebug.h"
#endif

#ifndef fahrer_h
#include "../ifc/fahrer.h"
#endif

#include "../tpl/vector_tpl.h"
#include "../simdings.h"

class KNode;
class karte_t;
class Stack;
class route_block_tester_t;

template <class T> class vector_tpl;

/**
 * Routen, zB f�r Fahrzeuge
 *
 * @author Hj. Malthaner
 * @date 15.01.00
 */
class route_t
{
private:

    /**
     * Die eigentliche Routensuche
     * @author Hj. Malthaner
     */
    bool intern_calc_route(karte_t *w, koord3d start, koord3d ziel, fahrer_t *fahr, const uint32 max_kmh, const uint32 max_cost);


    vector_tpl <koord3d> route;           // Die Koordinaten fuer die Fahrtroute

    karte_t *welt;

public:

	typedef struct nodestruct{
	    struct nodestruct *parent;
	    const grund_t *gr;
	    uint32  f,g;
	    uint8 dir;
	} ANode;

	static ANode *nodes;
	static uint32 MAX_STEP;
#ifdef DEBUG
	static bool node_in_use;
	static void GET_NODE() {if(node_in_use) trap(); node_in_use =1; };
	static void RELEASE_NODE() {if(!node_in_use) trap(); node_in_use =0; };
#else
	void GET_NODE() const {}
	void RELEASE_NODE() const {}
#endif

	static inline uint32 calc_distance( const koord3d p1, const koord3d p2 )
	{
		return (abs(p1.x-p2.x)+abs(p1.y-p2.y)+abs(p1.z-p2.z)/16);
	}


    /**
     * Konstruktor, legt eine leere Route an.
     * @author Hj. Malthaner
     */
    route_t();


    /**
     * @return Koordinate an index n
     * @author Hj. Malthaner
     */
    const koord3d & position_bei(const unsigned int n) const { return route.get(n); };


    /**
     * @return letzer index in der Koordinatenliste
     * @author Hj. Malthaner
     */
    sint32 gib_max_n() const { return ((signed int)route.get_count())-1; };



    /**
     * kopiert positionen und hoehen von einer anderen route
     * @author Hj. Malthaner
     */
    void kopiere(const route_t *route);


    /**
     * kopiert positionen und hoehen von einer anderen route
     * @author prissi
     */
    void append(const route_t *route);


    /**
     * f�gt k vorne in die route ein
     * @author Hj. Malthaner
     */
    void insert(koord3d k);

    /**
     * f�gt k hinten in die route ein
     * @author prissi
     */
    void append(koord3d k);

    /**
     * removes all tiles from the route
     * @author prissi
     */
    void clear() { route.clear(); };

    /**
     * removes all tiles behind this position
     * @author prissi
     */
    void remove_koord_from(int);

    /**
     * Appends a straig line from the last koord3d in route to the desired target.
     * Will return fals if fails
     * @author prissi
     */
    bool append_straight_route( karte_t *w, koord3d );

	/* find the route to an unknow location (where tile_found becomes true)
	 * the max_depth is the maximum length of a route
	 * @author prissi
	 */
	bool find_route(karte_t *w, const koord3d start, fahrer_t *fahr, const uint32 max_khm, uint8 start_dir, uint32 max_depth );

    /**
     * berechnet eine route von start nach ziel.
     * @author Hj. Malthaner
     */
    bool calc_route(karte_t *welt, koord3d start, koord3d ziel, fahrer_t *fahr, const uint32 max_speed_kmh, const uint32 max_cost=0xFFFFFFFF);

    /**
     * L�dt/speichert eine Route
     * @author V. Meyer
     */
    void rdwr(loadsave_t *file);

	bool is_ding_there(karte_t * welt, const koord3d pos, ding_t::typ typ);
};

#endif
