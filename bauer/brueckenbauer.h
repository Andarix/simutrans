/*
 * simbridge.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef __simbridge_h
#define __simbridge_h

#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"
#include "../boden/wege/weg.h"

class bruecke_besch_t;
class tabfileobj_t;
class karte_t;                 // Hajo: 22-Nov-01: Added forward declaration
class spieler_t;               // Hajo: 22-Nov-01: Added forward declaration
class grund_t;

/**
 * Baut Br�cken. Br�cken sollten nicht direct instanziiert werden
 * sondern immer vom brueckenbauer_t erzeugt werden.
 *
 * Es gibt keine Instanz - nur statische Methoden.
 *
 * @author V. Meyer
 */
class brueckenbauer_t {
private:

    /*
     * Br�ckenendpunkte bei Rampen werden auf flachem Grund gebaut und m�ssen daher genauer
     * auf st�rende vorhandene Bauten �berpr�ft werden.
     *
     * @author V. Meyer
     */
    static bool ist_ende_ok(spieler_t *sp, const grund_t *gr);


    /*
     * Grund bestimmen, auf dem die Br�cke enden soll.
     *
     * @author V. Meyer
     */
    static koord3d finde_ende(karte_t *welt, koord3d pos, koord zv, weg_t::typ wegtyp);


    /*
     * Baut die Br�cke wirklich, vorher sollte alles gerpr�ft sein.
     *
     * @author V. Meyer
     */
    static bool baue_bruecke(karte_t *welt, spieler_t *sp, koord3d pos, koord3d end, koord zv, const bruecke_besch_t *besch);


    /*
     * Baut Anfang oder Ende der Br�cke.
     *
     * @author V. Meyer
     */
    static void baue_auffahrt(karte_t *welt, spieler_t *sp, koord3d pos, koord zv, const bruecke_besch_t *besch);


    brueckenbauer_t() {} // private -> no instance please


public:


    /**
     * Registers a new bridge type
     * @author V. Meyer, Hj. Malthaner
     */
    static void register_besch(const bruecke_besch_t *besch);


    static bool laden_erfolgreich();


    static const bruecke_besch_t *gib_besch(const char *name);


    /*
     * Br�ckenbau-Funktion - Prototyp passend f�r den werkzeugwaehler_t.
     * param ist ein Zeiger auf eine Br�ckenbeshreibung (besch_t).
     *
     * @author V. Meyer
     */
    static int baue(spieler_t *sp, karte_t *welt, koord pos, value_t param);


    /*
     * Br�ckenbau f�r die KI - der ist der Br�ckenstil egal.
     * Vorerst greifen wir den ersten passenden Stil - Zufallswahl k�nnte man erg�nzen.
     *
     * @author V. Meyer
     */
    static int baue(spieler_t *sp, karte_t *welt, koord pos, weg_t::typ wegtyp);

    /*
     * Br�ckenl�sch-Funktion
     *
     * @author V. Meyer
     */
    static const char *remove(karte_t *welt, spieler_t *sp, koord3d pos, weg_t::typ wegtyp);


    /*
     * Werkzeugwaehler_t mit dynamischem Br�ckenmenu� erzeugen
     *
     * @author V. Meyer
     */
    static void create_menu(karte_t *welt);
};


#endif
