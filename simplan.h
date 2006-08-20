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

#ifndef tpl_microvec_h
#include "tpl/microvec_tpl.h"
#endif

#ifndef boden_grund_h
#include "boden/grund.h"
#endif


#define PRI_DACH1   0
#define PRI_DACH2  11
#define PRI_DEPOT  11

#define PRI_NIEDRIG 3
#define PRI_MITTEL  6
#define PRI_HOCH    9

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
  // minivec_tpl<grund_t *>boeden;

  microvec_tpl <grund_t *> boeden;


public:

    /**
     * Constructs a planquadrat with initial capacity of one ground
     * @author Hansj�rg Malthaner
     */
    planquadrat_t() : boeden(1) {}


    /**
     * setzt alle Eintragungen auf NULL
     */
    bool destroy(spieler_t *sp);


    /**
     * Setzen des "normalen" Bodens auf Kartenniveau
     * @author V. Meyer
     */
    void kartenboden_setzen(grund_t *bd, bool mit_besitzer);

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
     * Sucht den h�chsten Boden des Spielers - das ist eine Br�cke oder
     * der Kartenboden. Im Moment nur f�r Signale auf Br�cken verwendet.
     * @author V. Meyer
     */
    grund_t * gib_obersten_boden(spieler_t *sp) const;


    /**
     * R�ckegabe des Bodens an der gegebenen H�he, falls vorhanden.
     * Inline, da von karte_t::lookup() benutzt und daher sehr(!)
     * h�ufig aufgerufen
     * @return NULL wenn Boden nicht gefunden
     * @author Hj. Malthaner
     */
    inline grund_t * gib_boden_in_hoehe(const int z) const
    {
      for(unsigned int i = 0; i < boeden.count(); i++) {
	grund_t * gr = boeden.get(i);
	if(gr->gib_hoehe() == z) {
	  return gr;
	}
      }
      return NULL;
    }


    /**
     * R�ckgabe des "normalen" Bodens auf Kartenniveau
     * @return NULL wenn boden nicht existiert
     * @author Hansj�rg Malthaner
     */
    inline grund_t * gib_kartenboden() const { return boeden.get(0); };

    /**
     * R�ckegabe des Bodens, der das gegebene Objekt enth�lt, falls vorhanden.
     * @return NULL wenn (this == NULL)
     * @author V. Meyer
     */
    grund_t * gib_boden_von_obj(ding_t *obj) const;

    /**
     * R�ckegabe des n-ten Bodens. Inlined weil sehr h�ufig aufgerufen!
     * @return NULL wenn boden nicht existiert
     * @author Hj. Malthaner
     */
    grund_t * gib_boden_bei(const unsigned int idx) const {
	return boeden.get(idx);
    };

    /**
     * @return Anzahl der B�den dieses Planquadrats
     * @author Hj. Malthaner
     */
    unsigned int gib_boden_count() const { return boeden.count(); }



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



    void rdwr(karte_t *welt, loadsave_t *file);

    void step(long delta_t, int steps);

    void display_boden(const int xpos, const int ypos, const bool dirty) const;

    void display_dinge(const int xpos, const int ypos, const bool dirty) const;
};


#endif
