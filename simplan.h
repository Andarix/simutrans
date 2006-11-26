/*
 * simplan.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simplan_h
#define simplan_h

#include "halthandle_t.h"
#include "boden/grund.h"


class spieler_t;
class zeiger_t;
class karte_t;
class grund_t;
class weg_t;
class ding_t;


/**
 * Die Karte ist aus Planquadraten zusammengesetzt.
 * Planquadrate speichern Untergr�nde (B�den) der Karte.
 * @author Hj. Malthaner
 */
class planquadrat_t
{
private:
	/* list of stations that are reaching to this tile (saves lots of time for lookup) */
	halthandle_t *halt_list;

	uint8 ground_size, halt_list_count;

	/* only one station per ground xy tile */
	halthandle_t this_halt;

	union {
		grund_t ** some;    // valid if capacity > 1
		grund_t * one;      // valid if capacity == 1
	} data;

public:
	/**
	 * Constructs a planquadrat with initial capacity of one ground
	 * @author Hansj�rg Malthaner
	 */
	planquadrat_t() { ground_size=0; data.one = NULL; halt_list_count=0;  halt_list=NULL; }

	~planquadrat_t();

	/**
	* Setzen des "normalen" Bodens auf Kartenniveau
	* @author V. Meyer
	*/
	void kartenboden_setzen(grund_t *bd);

	/**
	* Ersetzt Boden alt durch neu, l�scht Boden alt.
	* @author Hansj�rg Malthaner
	*/
	void boden_ersetzen(grund_t *alt, grund_t *neu);

	/**
	* Setzen einen Br�cken- oder Tunnelbodens
	* @author V. Meyer
	*/
	void boden_hinzufuegen(grund_t *bd);

	/**
	* L�schen eines Br�cken- oder Tunnelbodens
	* @author V. Meyer
	*/
	bool boden_entfernen(grund_t *bd);

	/**
	* R�ckegabe des Bodens an der gegebenen H�he, falls vorhanden.
	* Inline, da von karte_t::lookup() benutzt und daher sehr(!)
	* h�ufig aufgerufen
	* @return NULL wenn Boden nicht gefunden
	* @author Hj. Malthaner
	*/
	inline grund_t *gib_boden_in_hoehe(const sint16 z) const {
		if(ground_size<=1) {
			if(data.one  &&  data.one->gib_hoehe()==z) {
				return data.one;
			}
			//assert(ground_size==0  &&  data.one==NULL);
		}
		else {
			for(uint8 i=0;  i<ground_size;  i++) {
				if(data.some[i]->gib_hoehe()==z) {
					return data.some[i];
				}
			}
		}
		return NULL;
	}

	/**
	* R�ckgabe des "normalen" Bodens auf Kartenniveau
	* @return NULL wenn boden nicht existiert
	* @author Hansj�rg Malthaner
	*/
	inline grund_t *gib_kartenboden() const { return (ground_size<=1) ? data.one : data.some[0]; }

	/**
	* R�ckegabe des Bodens, der das gegebene Objekt enth�lt, falls vorhanden.
	* @return NULL wenn (this == NULL)
	* @author V. Meyer
	*/
	grund_t *gib_boden_von_obj(ding_t *obj) const;

	/**
	* R�ckegabe des n-ten Bodens. Inlined weil sehr h�ufig aufgerufen!
	* @return NULL wenn boden nicht existiert
	* @author Hj. Malthaner
	*/
	inline grund_t *gib_boden_bei(const unsigned idx) const { return (idx<ground_size) ? (ground_size<=1 ? data.one : data.some[idx]) : NULL; }

	/**
	* @return Anzahl der B�den dieses Planquadrats
	* @author Hj. Malthaner
	*/
	unsigned int gib_boden_count() const { return ground_size; }

	/**
	* konvertiert Land zu Wasser wenn unter Grundwasserniveau abgesenkt
	* @author Hj. Malthaner
	*/
	void abgesenkt(karte_t *welt);

	/**
	* konvertiert Wasser zu Land wenn �ber Grundwasserniveau angehoben
	* @author Hj. Malthaner
	*/
	void angehoben(karte_t *welt);

	/**
	* since stops may be multilevel, but waren uses pos, we mirror here any halt that is on this square
	* @author Hj. Malthaner
	*/
	void setze_halt(halthandle_t halt);

	/**
	* returns a halthandle, if some ground here has a stop
	* @return NULL wenn keine Haltestelle, sonst Zeiger auf Haltestelle
	* @author Hj. Malthaner
	*/
	const halthandle_t gib_halt() const {return this_halt;}

private:
	// these functions are private helper functions for halt_list corrections
	void halt_list_remove( halthandle_t halt );
	void halt_list_insert_at( halthandle_t halt, uint8 pos );

public:
	/*
	* The following three functions takes about 4 bytes of memory per tile but speed up passenger generation
	* @author prissi
	*/
	void add_to_haltlist(halthandle_t halt);

	/**
	* removes the halt from a ground
	* however this funtion check, whether there is really no other part still reachable
	* @author prissi
	*/
	void remove_from_haltlist(karte_t *welt, halthandle_t halt);

	bool is_connected(halthandle_t halt) const;

	/**
	* returns the internal array of halts
	* @author prissi
	*/
	const halthandle_t *get_haltlist() const { return halt_list; }
	const uint8 &get_haltlist_count() const { return halt_list_count; }

	void rdwr(karte_t *welt, loadsave_t *file);

	// will toggle the seasons ...
	void check_season(const long month);

	void display_boden(const sint16 xpos, const sint16 ypos, const sint16 scale, const bool dirty) const;

	void display_dinge(const sint16 xpos, const sint16 ypos, const sint16 scale, const bool dirty) const;
};

#endif
