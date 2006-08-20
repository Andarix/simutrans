/*
 * gebaeude.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef dings_gebaeude_h
#define dings_gebaeude_h

// use my own functions,m but does not work properly
#define USE_NEW_GEBAUDE 0

#include "../ifc/sync_steppable.h"
#include "../simdings.h"

#ifdef USE_NEW_GEBAUDE
#include "../dataobj/freelist.h"
#endif

class haus_tile_besch_t;
class fabrik_t;
class stadt_t;

/**
 * Asynchron oder synchron animierte Gebaeude f�r Simutrans.
 * @author Hj. Malthaner
 */
class gebaeude_t : public ding_t, sync_steppable
{
public:

    enum  {NOT_HIDDEN=0, SOME_HIDDEN, ALL_HIDDEN};

    /**
     * Vom typ "unbekannt" sind auch spezielle gebaeude z.B. das Rathaus
     * @author Hj. Malthaner
     */
    enum typ {wohnung, gewerbe, industrie, unbekannt};

    /**
     * Set to true to hide all non-station buildings
     * @author Hj. Malthaner
     */
    static uint8 hide;


private:

    const haus_tile_besch_t *tile;


	/**
	 * either point to a factory or a city
	 * @author Hj. Malthaner
	 */
	union {
		fabrik_t  *fab;
		stadt_t *stadt;
	} ptr;

    /**
     * Zeitpunkt an dem das Gebaeude Gebaut wurde
     * @author Hj. Malthaner
     */
    uint32 insta_zeit;

    /**
     * Time control for animation progress.
     * @author Hj. Malthaner
     */
    uint16 anim_time;

    /**
     * Current anim frame
     * @author Hj. Malthaner
     */
    uint8 count;

    /**
     * Is this a sync animated object?
     * @author Hj. Malthaner
     */
    uint8 sync:1;

    /**
     * Boolean flag if a construction site or buildings image
     * shall be displayed.
     * @author Hj. Malthaner
     */
    uint8 zeige_baugrube:1;

    /**
     * if true, this ptr union contains a factory pointer
     * @author Hj. Malthaner
     */
    uint8 is_factory:1;

    /**
     * Initializes all variables with save, usable values
     * @author Hj. Malthaner
     */
    void init(const haus_tile_besch_t *t);


protected:
    gebaeude_t(karte_t *welt);

    /**
     * Erzeugt ein Info-Fenster f�r dieses Objekt
     * @author V. Meyer
     */
    virtual ding_infowin_t *new_info();

public:
    gebaeude_t(karte_t *welt, loadsave_t *file);
    gebaeude_t(karte_t *welt, koord3d pos,spieler_t *sp, const haus_tile_besch_t *t);
    /**
     * Destructor. Removes this from the list of sync objects if neccesary.
     *
     * @author Hj. Malthaner
     */
    ~gebaeude_t();

    typ gib_haustyp() const;

    void add_alter(uint32 a);

    void setze_fab(fabrik_t *fb);
    void setze_stadt(stadt_t *s);

    /**
     * Ein Gebaeude kann zu einer Fabrik geh�ren.
     * @return Einen Zeiger auf die Fabrik zu der das Objekt geh�rt oder NULL,
     * wenn das Objekt zu keiner Fabrik geh�rt.
     * @author Hj. Malthaner
     */
    virtual inline fabrik_t* get_fabrik() const {return is_factory?ptr.fab:NULL;}
    stadt_t* get_stadt() const {return is_factory?NULL:ptr.stadt;}

    enum ding_t::typ gib_typ() const {return ding_t::gebaeude;}

    void setze_count(uint8 count);
    void setze_anim_time(uint16 t) {anim_time = t;}

    /**
     * @return einen Zeiger auf die Karte zu der das Ding geh�rt
     * @author Hj. Malthaner
     */
    virtual inline karte_t* gib_karte() const {return welt;}

    /**
     * Should only be called after everything is set up to play
     * the animation actually. Sets sync flag and register/deregisters
     * this as a sync object, but only if phasen > 1
     *
     * @author Hj. Malthaner
     */
    void setze_sync(bool yesno);

    bool step(long delta_t);
    image_id gib_bild() const;
    image_id gib_bild(int nr) const;
    image_id gib_after_bild() const;
    image_id gib_after_bild(int nr) const;

    /**
     * @return eigener Name oder Name der Fabrik falls Teil einer Fabrik
     * @author Hj. Malthaner
     */
    virtual const char *gib_name() const;

    bool ist_rathaus() const;

    bool ist_firmensitz() const;

	bool is_monument() const;

    /**
     * setzt das Baudatum auf die aktuelle Zeit und das
     * Baugruben-Flag auf true
     * @author Hj. Malthaner
     */
    void renoviere();


    /**
     * @return Einen Beschreibungsstring f�r das Objekt, der z.B. in einem
     * Beobachtungsfenster angezeigt wird.
     * @author Hj. Malthaner
     */
    void info(cbuffer_t & buf) const;


    void rdwr(loadsave_t *file);


    /**
     * Vorbereitungsmethode f�r Echtzeitfunktionen eines Objekts.
     * @author Hj. Malthaner
     */
    virtual void sync_prepare() {};


    /**
     * Methode f�r Echtzeitfunktionen eines Objekts. Spielt animation.
     * @return true
     * @author Hj. Malthaner
     */
    virtual bool sync_step(long delta_t);

    /**
     * @return Den level (die Ausbaustufe) des Gebaudes
     * @author Hj. Malthaner
     */
    int gib_passagier_level() const;

    int gib_post_level() const;

    void setze_tile(const haus_tile_besch_t *t) { tile = t; }

    const haus_tile_besch_t *gib_tile() const { return tile; }

    /**
     * �ffnet ein neues Beobachtungsfenster f�r das Geb�ude,
     * wenn es kein Fundament ist
     * @author Hj. Malthaner
     */
    virtual void zeige_info();

    void entferne(spieler_t *sp);

#if USE_NEW_GEBAUDE
    virtual void * operator new(size_t s) { return (gebaeude_t *)freelist_t::gimme_node(sizeof(gebaeude_t)); }
    virtual void operator delete(void *p) { freelist_t::putback_node(sizeof(gebaeude_t),p); };
#endif
};

#endif
