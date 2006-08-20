/*
 * simvehikel.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/* simvehikel.h
 *
 * Fahrzeuge in der Welt von Sim
 *
 * 01.11.99  getrennt von simdings.h
 * Hj. Malthaner
 */

#ifndef _simvehikel_h
#define _simvehikel_h

#ifndef simtypes_h
#include "simtypes.h"
#endif

#ifndef simdings_h
#include "simdings.h"
#endif

#ifndef simware_h
#include "simware.h"
#endif

#ifndef halthandle_t_h
#include "halthandle_t.h"
#endif

#ifndef fahrer_h
#include "ifc/fahrer.h"
#endif

#ifndef boden_wege_weg_h
#include "boden/wege/weg.h"
#endif

#ifndef __VEHIKEL_BESCH_H
#include "besch/vehikel_besch.h"
#endif

#ifndef tpl_slist_tpl_h
#include "tpl/slist_tpl.h"
#endif

class haltestelle_t;
class inthashtable_t;
class cstring_t;
class vehikel_besch_t;
class fahrplan_t;
class presignal_t;

struct event_t;


/*
 * Global vehicle speed conversion factor between Simutrans speed
 * and km/h
 * @author Hj. Malthaner
 */
#define VEHICLE_SPEED_FACTOR  80


/**
 * Converts speed value to km/h
 * @author Hj. Matthaner
 */
#define speed_to_kmh(speed) (((speed)*VEHICLE_SPEED_FACTOR+511) >> 10)


/**
 * Converts km/h value to speed
 * @author Hj. Matthaner
 */
#define kmh_to_speed(speed) (((speed) << 10) / VEHICLE_SPEED_FACTOR)





/*----------------------- Fahrdings ------------------------------------*/

/**
 * Basisklasse f�r alle Fahrzeuge
 *
 * @author Hj. Malthaner
 */
class vehikel_basis_t : public ding_t
{
public:

    virtual ribi_t::ribi gib_fahrtrichtung() const = 0;


protected:

    virtual void betrete_feld();
    virtual void fahre();
    virtual int  calc_height();		// Offset Bergauf/Bergab
    virtual void calc_akt_speed(int /*h_alt*/, int /*h_neu*/) {};

    virtual int  gib_dx() const = 0;
    virtual int  gib_dy() const = 0;
    virtual int  gib_hoff() const = 0;

    virtual bool hop_check() = 0;
    virtual void hop() = 0;

public:
    ribi_t::ribi calc_richtung(koord start, koord ende, sint8 &dx, sint8 &dy) const;

    /**
     * Checks if this vehicle must change the square upon next move
     * @author Hj. Malthaner
     */
    bool vehikel_basis_t::is_about_to_hop() const;

    virtual void verlasse_feld();

    vehikel_basis_t(karte_t *welt);

    vehikel_basis_t(karte_t *welt, koord3d pos);

    virtual weg_t::typ gib_wegtyp() const = 0;

    virtual ~vehikel_basis_t();
};


/**
 * Klasse f�r alle Fahrzeuge mit Route
 *
 * @author Hj. Malthaner
 */

class vehikel_t : public vehikel_basis_t, public fahrer_t
{
private:

    static slist_tpl<const vehikel_t *> list;	// Liste der Vehikel (alle !)

    /**
     * Kaufdatum in months
     * @author Hj. Malthaner
     */
    sint32 insta_zeit;

    /**
     * Aktuelle Fahrtrichtung in Bildschirm-Koordinaten
     * @author Hj. Malthaner
     */
    sint8 dx, dy;


    bool rauchen;


    /**
     * Offsets fuer Bergauf/Bergab
     * @author Hj. Malthaner
     */
    sint16 hoff;

    /* For the more physical acceleration model friction is introduced
     * frictionforce = gamma*speed*weight
     * since the total weight is needed a lot of times, we save it
     * @author prissi
     */
     uint16 sum_weight;
     /* The friction is calculated new every step, so we save it too
     * @author prissi
     */
     sint16 current_friction;


    sint32 speed_limit;

    bool hop_check();
    void ziel_erreicht();

    void hop();

    /**
     * berechnet aktuelle Geschwindigkeit aufgrund der Steigung
     * (Hoehendifferenz) der Fahrbahn
     * @param h_alt alte Hoehe
     * @param h_neu neue Hoehe
     */
    void calc_akt_speed(int h_alt, int h_neu);

    void fahre();

    void setze_speed_limit(int limit);


    ribi_t::ribi alte_fahrtrichtung;
    ribi_t::ribi fahrtrichtung;

protected:

    sint32 get_speed_limit() { return speed_limit; };

    /**
     * Current index on the route
     * @author Hj. Malthaner
     */
    uint16 route_index;


    /**
     * Previous position on our path
     * @author Hj. Malthaner
     */
    koord3d pos_prev;

    /**
     * Current position on our path
     * @author Hj. Malthaner
     */
    koord3d pos_cur;

    /**
     * Next position on our path
     * @author Hj. Malthaner
     */
    koord3d pos_next;


    const vehikel_besch_t *besch;

    slist_tpl<ware_t> fracht;   // liste der gerade transportierten g�ter

    convoi_t *cnv;		// != NULL falls das vehikel zu einem Convoi gehoert

    bool ist_erstes;            // falls vehikel im convoi f�hrt, geben diese
    bool ist_letztes;           // flags auskunft �ber die psosition


    virtual int  gib_dx() const {return dx;};
    virtual int  gib_dy() const {return dy;};
    virtual int  gib_hoff() const {return hoff;};

    virtual void calc_bild() {};

    virtual bool ist_befahrbar(const grund_t* ) const {return false;};

    virtual void betrete_feld();

public:

    virtual bool ist_weg_frei(int &/*restart_speed*/) const {return true;};

    virtual void verlasse_feld();

    virtual weg_t::typ gib_wegtyp() const = 0;

    const sint32 gib_insta_zeit() const {return insta_zeit;};

    void darf_rauchen(bool yesno ) { rauchen = yesno;};
    ribi_t::ribi gib_fahrtrichtung() const {return fahrtrichtung;};
    void setze_fahrtrichtung(ribi_t::ribi r) {fahrtrichtung=r;calc_bild();};


    void setze_offsets(int x, int y);


    /**
     * gibt das Basisbild zurueck
     * @author Hj. Malthaner
     */
    int gib_basis_bild() const {return besch->gib_basis_bild();};


    /**
     * @return vehicle description object
     * @author Hj. Malthaner
     */
    const vehikel_besch_t *gib_besch() const {return besch; }


    /**
     * @return die Betriebskosten in Cr/100Km
     * @author Hj. Malthaner
     */
    int gib_betriebskosten() const {return besch->gib_betriebskosten();};


    /**
     * spielt den Sound, wenn das Vehikel sichtbar ist
     * @author Hj. Malthaner
     */
    void play_sound() const;


    /**
     * Bereitet Fahrzeiug auf neue Fahrt vor - wird aufgerufen wenn
     * der Convoi eine neue Route ermittelt
     * @author Hj. Malthaner
     */
    void neue_fahrt();


    void starte_neue_route(koord3d k0, koord3d k1);


    vehikel_t(karte_t *welt);
    vehikel_t(karte_t *welt,
	      koord3d pos,
	      const vehikel_besch_t *besch,
	      spieler_t *sp);

    /**
     * Destructor. Frees aggregated members.
     * @author Hj. Malthaner
     */
    ~vehikel_t();


    /**
     * Vom Convoi aufgerufen.
     * @author Hj. Malthaner
     */
    void sync_step();


    void rauche();


    /**
     * �ffnet ein neues Beobachtungsfenster f�r das Objekt.
     * @author Hj. Malthaner
     */
    void zeige_info();


    /**
     * der normale Infotext
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;


    /**
     * debug info into buffer
     * @author Hj. Malthaner
     */
    char *debug_info(char *buf) const;


    /**
     * Debug info to stderr
     * @author Hj. Malthaner
     * @date 26-Aug-03
     */
    void dump() const;


    /**
     * Ermittelt fahrtrichtung
     * @author Hj. Malthaner
     */
    ribi_t::ribi richtung();


    inline const int gib_speed() const {return kmh_to_speed(besch->gib_geschw());};


    /* return friction constant: changes in hill and curves; may even negative downhill *
     * @author prissi
     */
	inline const int gib_frictionfactor() const {return current_friction;};


    /* Return total weight including freight*
     * @author prissi
     */
	inline const int gib_gesamtgewicht() const {return sum_weight;};


    /**
     * berechnet die gesamtmenge der bef�rderten waren
     */
    int gib_fracht_menge() const;


    /**
     * Berechnet Gesamtgewicht der transportierten Fracht in KG
     * @author Hj. Malthaner
     */
    int gib_fracht_gewicht() const;


    const char * gib_fracht_name() const;


    /**
     * setzt den typ der bef�rderbaren ware
     */
    const ware_besch_t *gib_fracht_typ() const {return besch->gib_ware();};


    /**
     * setzt die maximale Kapazitaet
     */
    const int gib_fracht_max() const {return besch->gib_zuladung(); }


    const char * gib_fracht_mass() const;


    /**
     * erstellt einen Info-Text zur Fracht, z.B. zur Darstellung in einem
     * Info-Fenster
     * @author Hj. Malthaner
     */
    void gib_fracht_info(cbuffer_t & buf);


    /**
     * loescht alle fracht aus dem Fahrzeug
     * @author Hj. Malthaner
     */
    void loesche_fracht();


    /**
     * Payment is done per hop. It iterates all goods and calculates
     * the income for the last hop. This method must be called upon
     * every stop.
     * @return income total for last hop
     * @author Hj. Malthaner
     */
    int  calc_gewinn(koord3d start, koord3d end) const;


    /**
     * fahrzeug an haltestelle entladen
     * @author Hj. Malthaner
     */
    void entladen(koord k, halthandle_t halt);


    /**
     * fahrzeug an haltestelle beladen
     */
    bool beladen(koord k, halthandle_t halt);


    void setze_erstes(bool janein) {ist_erstes = janein;};
    void setze_letztes(bool janein) {ist_letztes = janein;};

    void setze_convoi(convoi_t *c);


    /**
     * Remove freight that no longer can reach it's destination
     * i.e. becuase of a changed schedule
     * @author Hj. Malthaner
     */
    void remove_stale_freight();


    /**
     * erzeuge einen f�r diesen Vehikeltyp passenden Fahrplan
     * @author Hj. Malthaner
     */
    virtual fahrplan_t * erzeuge_neuen_fahrplan() const = 0;


    const char * ist_entfernbar(const spieler_t *sp);

    void rdwr(loadsave_t *file);
    virtual void rdwr(loadsave_t *file, bool force) = 0;


    int calc_restwert() const;


    /**
     * @return Gesamtanzahl aller Transportfahrzeuge
     * @author Hj. Malthaner
     */
    static inline int anzahl() { return list.count(); };
};


/**
 * Eine Klasse f�r Strassenfahrzeuge. Verwaltet das Aussehen der
 * Fahrzeuge und die Befahrbarkeit des Untergrundes.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class automobil_t : public vehikel_t
{
protected:
    bool ist_befahrbar(const grund_t *bd) const;

    virtual weg_t::typ gib_wegtyp() const { return weg_t::strasse; };

    virtual void betrete_feld();

    void calc_bild();

public:
    virtual bool ist_weg_frei(int &restart_speed) const;

    automobil_t(karte_t *welt, loadsave_t *file);
    automobil_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch,
                spieler_t *sp, convoi_t *cnv); // start und fahrplan


    /**
     * Ermittelt die f�r das Fahrzeug geltenden Richtungsbits,
     * abh�ngig vom Untergrund.
     *
     * @author Hj. Malthaner, 04.01.01
     */
    virtual ribi_t::ribi gib_ribi(const grund_t* ) const;


    ding_t::typ gib_typ() const {return automobil;};


    fahrplan_t * erzeuge_neuen_fahrplan() const;

    void rdwr(loadsave_t *file);
    void rdwr(loadsave_t *file, bool force);
};


/**
 * Eine Klasse f�r Schienenfahrzeuge. Verwaltet das Aussehen der
 * Fahrzeuge und die Befahrbarkeit des Untergrundes.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class waggon_t : public vehikel_t
{
private:

    bool ist_blockwechsel(koord3d k1, koord3d k2) const;

protected:
    bool ist_befahrbar(const grund_t *bd) const;

    void betrete_feld();

    virtual weg_t::typ gib_wegtyp() const { return weg_t::schiene; };

    void calc_bild();

public:
    virtual bool ist_weg_frei(int &restart_speed) const;

	bool is_next_block_free( presignal_t *sig ) const;

    void verlasse_feld();


    /**
     * Ermittelt die f�r das Fahrzeug geltenden Richtungsbits,
     * abh�ngig vom Untergrund.
     *
     * @author Hj. Malthaner, 04.01.01
     */
    virtual ribi_t::ribi gib_ribi(const grund_t* ) const;

    enum ding_t::typ gib_typ() const {return waggon;};

    waggon_t(karte_t *welt, loadsave_t *file);
    waggon_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch,
             spieler_t *sp, convoi_t *cnv); // start und fahrplan
    ~waggon_t();

    fahrplan_t * erzeuge_neuen_fahrplan() const;

    void rdwr(loadsave_t *file);
    void rdwr(loadsave_t *file, bool force);
};


/**
 * Eine Klasse f�r Wasserfahrzeuge. Verwaltet das Aussehen der
 * Fahrzeuge und die Befahrbarkeit des Untergrundes.
 *
 * @author Hj. Malthaner
 * @see vehikel_t
 */
class schiff_t : public vehikel_t
{
protected:
    virtual int  calc_height() {return 0;};


    bool ist_befahrbar(const grund_t *bd) const;

    virtual weg_t::typ gib_wegtyp() const { return weg_t::wasser; };

    void calc_bild();

public:

    virtual bool ist_weg_frei(int &restart_speed) const;

    /**
     * Ermittelt die f�r das Fahrzeug geltenden Richtungsbits,
     * abh�ngig vom Untergrund.
     *
     * @author Hj. Malthaner, 04.01.01
     */
    virtual ribi_t::ribi gib_ribi(const grund_t* ) const;


    schiff_t(karte_t *welt, loadsave_t *file);
    schiff_t(karte_t *welt, koord3d pos, const vehikel_besch_t *besch,
             spieler_t *sp, convoi_t *cnv); // start und fahrplan

    ding_t::typ gib_typ() const {return schiff;};

    fahrplan_t * erzeuge_neuen_fahrplan() const;

    void rdwr(loadsave_t *file);
    void rdwr(loadsave_t *file, bool force);
};


#endif
