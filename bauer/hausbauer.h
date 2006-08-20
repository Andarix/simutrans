/*
 * hausbauer.h
 *
 * Copyright (c) 1997 - 2002 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef simhausbau_h
#define simhausbau_h

class karte_t;
class tabfileobj_t;
class spieler_t;
class gebaeude_t;

#include "../dataobj/koord3d.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../tpl/inthashtable_tpl.h"
#include "../simtypes.h"

class gebaeude_t;
class haus_besch_t;
class haus_tile_besch_t;
class werkzeug_parameter_waehler_t;

/**
 * Diese Klasse �bernimmt den Bau von Mehrteiligen Geb�uden.
 * Sie kennt die Beschreibung (fast) aller Geb�ude was
 * Typ, H�he, Gr��e, Bilder, Animationen angeht.
 * Diese Daten werden aus "gebaeude.tab" gelesen.
 *
 * F�r Denkm�ler wird eine Liste der ungebauten gef�hrt.
 *
 * @author Hj. Malthaner/V. Meyer
 * @version $Revision: 1.3 $
 */
class hausbauer_t
{
    static stringhashtable_tpl<const haus_besch_t *> besch_names;
public:
    /**
     * Unbekannte Geb�ude sind nochmal unterteilt
     */
    enum utyp { unbekannt, special, sehenswuerdigkeit, denkmal, fabrik, rathaus, weitere, firmensitz };

private:
    static slist_tpl<const haus_besch_t *> alle;
    static slist_tpl<const haus_besch_t *> sehenswuerdigkeiten;
    static slist_tpl<const haus_besch_t *> spezials;
    static slist_tpl<const haus_besch_t *> fabriken;
    static slist_tpl<const haus_besch_t *> denkmaeler;
    static slist_tpl<const haus_besch_t *> ungebaute_denkmaeler;

public:
    static slist_tpl<const haus_besch_t *> wohnhaeuser;
    static slist_tpl<const haus_besch_t *> gewerbehaeuser;
    static slist_tpl<const haus_besch_t *> industriehaeuser;

    /**
     * Geb�ude, die das Programm direkt kennen mu�
     */
    static const haus_besch_t *frachthof_besch;
    static const haus_besch_t *bahn_depot_besch;
    static const haus_besch_t *tram_depot_besch;
    static const haus_besch_t *str_depot_besch;
    static const haus_besch_t *sch_depot_besch;
    static const haus_besch_t *muehle_besch;

    // to allow for an arbitary number, we use lists
    static slist_tpl<const haus_besch_t *> hausbauer_t::station_building;
    static slist_tpl<const haus_besch_t *> hausbauer_t::post_offices;
    static slist_tpl<const haus_besch_t *> hausbauer_t::train_stops;
    static slist_tpl<const haus_besch_t *> hausbauer_t::car_stops;
    static slist_tpl<const haus_besch_t *> hausbauer_t::ship_stops;
    static slist_tpl<const haus_besch_t *> hausbauer_t::headquarter;

private:
    /**
     * Liefert den Eintrag mit passendem Level aus der Liste,
     * falls es ihn gibt.
     * Falls es ihn nicht gibt wird ein geb�ude des n�chst�heren vorhandenen
     * levels geliefert.
     * Falls es das auch nicht gibt wird das Geb�ude mit dem h�chsten
     * level aus der Liste geliefert.
     *
     * Diese Methode liefert niemals NULL!
     *
     * @author Hj. Malthaner
     */
    static const haus_besch_t * gib_aus_liste(slist_tpl<const haus_besch_t *> &liste, int level);

    /**
     * Liefert einen zuf�lligen Eintrag aus der Liste.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_aus_liste(slist_tpl<const haus_besch_t *> &liste);

    /**
     * Sucht einen Eintrag aus einer Liste passend zum Namen ("Name=..." aus gebaeude.tab).
     * @author V. Meyer
     */
    static const haus_besch_t *finde_in_liste(slist_tpl<const haus_besch_t *>  &liste, utyp utype, const char *name);

    /**
     * Sucht eine Eintrag aus der Spezialgeb�udeliste mit der passenden Bev�lkerung.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_special_intern(int bev, enum utyp utype);

    static void insert_sorted(slist_tpl<const haus_besch_t *> &liste, const haus_besch_t *besch);
public:

    static const haus_tile_besch_t *find_tile(const char *name, int idx);

    static bool register_besch(const haus_besch_t *besch);
    static bool alles_geladen();

    /* Fill menu with icons of given stops from the list
     * @author prissi
     */
    static void fill_menu(werkzeug_parameter_waehler_t *wzw,
    		slist_tpl<const haus_besch_t *>&stops,
    		int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
    		int sound_ok, int sound_ko,int cost);

    /**
     * Sucht ein Geb�ude, welches bei der gegebenen Bev�lkerungszahl gebaut
     * werden soll.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_special(int bev)
    { return gib_special_intern(bev, special);  }

    /**
     * Sucht ein Rathaus, welches bei der gegebenen Bev�lkerungszahl gebaut
     * werden soll.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_rathaus(int bev)
    { return gib_special_intern(bev, rathaus);  }

    /**
     * Gewerbegeb�ude passend zum Level liefern. Zur Zeit sind die Eintr�ge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_gewerbe(int level)
    { return gib_aus_liste(gewerbehaeuser, level); }

    /**
     * Industriegeb�ude passend zum Level liefern. Zur Zeit sind die Eintr�ge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_industrie(int level)
    { return gib_aus_liste(industriehaeuser, level); }

    /**
     * Wohnhaus passend zum Level liefern. Zur Zeit sind die Eintr�ge
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *gib_wohnhaus(int level)
    { return gib_aus_liste(wohnhaeuser, level); }

    /**
     * Fabrikbeschreibung anhand des Namens raussuchen.
     * eindeutig aufsteigend.
     * @author V. Meyer
     */
    static const haus_besch_t *finde_fabrik(const char *name)
    { return finde_in_liste(fabriken, fabrik, name); }

    /**
     * Denkmal anhand des Namens raussuchen.
     * @author V. Meyer
     */
    static const haus_besch_t *finde_denkmal(const char *name)
    { return finde_in_liste(denkmaeler, denkmal, name ); }

    /**
     * Liefert per Zufall die Beschreibung eines Sehenswuerdigkeit,
     * die bei Kartenerstellung gebaut werden kann.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_sehenswuerdigkeit()
    { return waehle_aus_liste(sehenswuerdigkeiten); }

    /**
     * Liefert per Zufall die Beschreibung eines ungebauten Denkmals.
     * @author V. Meyer
     */
    static const haus_besch_t *waehle_denkmal()
    { return waehle_aus_liste(ungebaute_denkmaeler); }

    /**
     * Diese Funktion setzt intern alle Denkm�ler auf ungebaut. Damit wir
     * keines doppelt bauen.
     * @author V. Meyer
     */
    static void neue_karte();

    /**
     * Dem Hausbauer Bescheid sagen, dass ein bestimmtes Denkmal gebaut wurde.
     * @author V. Meyer
     */
    static void denkmal_gebaut(const haus_besch_t *besch)
    {    ungebaute_denkmaeler.remove(besch); }


    /*
     * Baut H�user um!
     * @author V. Meyer
     */
    static void umbauen(karte_t *welt,
                        gebaeude_t *gb,
			const haus_besch_t *besch);

    /**
     * Baut alles was in gebaeude.tab beschrieben ist.
     * Es werden immer gebaeude_t-Objekte erzeugt.
     * @author V. Meyer
     */
    static void baue(karte_t *welt,
		     spieler_t *sp,
		     koord3d pos,
		     int layout,
		     const haus_besch_t *besch,
		     bool clear = true,
		     void *param = NULL);


    static gebaeude_t *neues_gebaeude( karte_t *welt,
			    spieler_t *sp,
			    koord3d pos,
			    int layout,
			    const haus_besch_t *besch,
			    void *param = NULL);
};

#endif
