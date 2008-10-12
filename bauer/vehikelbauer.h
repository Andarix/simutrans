/*
 * Copyright (c) 1997 - 2002 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef vehikelbauer_t_h
#define vehikelbauer_t_h


#include "../dataobj/koord3d.h"
#include "../simimg.h"
#include "../simtypes.h"

class vehikel_t;
class cstring_t;
class spieler_t;
class convoi_t;
class vehikel_besch_t;
class ware_besch_t;
template <class T> class slist_tpl;
template <class T> class vector_tpl;


/**
 * Baut Fahrzeuge. Fahrzeuge sollten nicht direct instanziiert werden
 * sondern immer von einem vehikelbauer_t erzeugt werden.
 *
 * @author Hj. Malthaner
 */
class vehikelbauer_t
{
public:
	static bool speedbonus_init(cstring_t objfilename);
	static sint32 get_speedbonus( sint32 monthyear, waytype_t wt );

	static bool register_besch(const vehikel_besch_t *besch);
	static bool alles_geladen();

	static vehikel_t* baue(koord3d k, spieler_t* sp, convoi_t* cnv, const vehikel_besch_t* vb );

	static const vehikel_besch_t * gib_info(const char *name);
	static slist_tpl<const vehikel_besch_t*>* gib_info(waytype_t typ);

	/* extended sreach for vehicles for KI
	* @author prissi
	*/
	static const vehikel_besch_t *vehikel_search(waytype_t typ,const uint16 month_now,const uint32 target_power,const uint32 target_speed,const ware_besch_t * target_freight, bool include_electric, bool not_obsolete );
};

#endif
