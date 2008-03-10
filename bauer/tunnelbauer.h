/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef __simtunnel_h
#define __simtunnel_h

#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"
#include "../besch/tunnel_besch.h"
#include "../boden/wege/weg.h"

class karte_t;                 // Hajo: 22-Nov-01: Added forward declaration
class spieler_t;               // Hajo: 22-Nov-01: Added forward declaration
class werkzeug_parameter_waehler_t;

/**
 * Baut Tunnel. Tunnel sollten nicht direkt instanziiert werden
 * sondern immer vom tunnelbauer_t erzeugt werden.
 *
 * Es gibt keine Instanz - nur statische Methoden.
 *
 * @author V. Meyer
 */
class tunnelbauer_t {
private:
	static koord3d finde_ende(karte_t *welt, koord3d pos, koord zv, waytype_t wegtyp);
	static bool baue_tunnel(karte_t *welt, spieler_t *sp, koord3d pos, koord3d end, koord zv, const tunnel_besch_t *besch);
	static void baue_einfahrt(karte_t *welt, spieler_t *sp, koord3d end, koord zv, const tunnel_besch_t *besch, const weg_besch_t *weg_besch, int &cost);

	tunnelbauer_t() {} // private -> no instance please

public:
	static void register_besch(tunnel_besch_t *besch);
	static bool laden_erfolgreich();

	static const tunnel_besch_t *gib_besch(const char *);

	static const tunnel_besch_t *find_tunnel(const waytype_t wtyp, const uint32 min_speed,const uint16 time);

	static void fill_menu(werkzeug_parameter_waehler_t *wzw, const waytype_t wtyp, const int sound_ok, const int sound_ko,const karte_t *welt);

	static int baue(enum wkz_mode_t, spieler_t *sp, karte_t *welt, koord pos, value_t param);

	static const char *remove(karte_t *welt, spieler_t *sp, koord3d pos, waytype_t wegtyp);
};

#endif
