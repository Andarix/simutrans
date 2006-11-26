/*
 * simdings.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef simdings_h
#define simdings_h

#include "simtypes.h"
#include "simimg.h"
#include "dataobj/koord3d.h"


template <class K, class V> class ptrhashtable_tpl;
class ding_infowin_t;
class cbuffer_t;
class fabrik_t;
class karte_t;
class spieler_t;

/**
 * Von der Klasse ding_t sind alle Objekte in Simutrans abgeleitet.
 * since everything is a ding on the map, we need to make this a compact and fast as possible
 *
 * @author Hj. Malthaner
 * @see planqadrat_t
 */
class ding_t
{
public:
	// flags
	enum flag_values {keine_flags=0, dirty=1, not_on_map=2, is_vehicle=4, is_wayding=8 };

private:
	/**
	* Dies ist die Koordinate des Planquadrates in der Karte zu
	* dem das Objekt geh�rt.
	* @author Hj. Malthaner
	*/
	koord3d pos;

	/**
	* Dies ist der x-Offset in Bilschirmkoordinaten bei der
	* Darstellung des Objektbildes.
	* @author Hj. Malthaner
	*/
	sint8 xoff;

	/**
	* Dies ist der y-Offset in Bilschirmkoordinaten bei der
	* Darstellung des Objektbildes.
	* @author Hj. Malthaner
	*/
	sint8 yoff;

	/**
	* Dies ist der Zeiger auf den Besitzer des Objekts.
	* @author Hj. Malthaner
	*/
	sint8 besitzer_n:4;

	/**
	* flags fuer Zustaende, etc
	* @author Hj. Malthaner
	*/
	uint8 flags:4;

	/**
	* das bild des dings
	* @author Hj. Malthaner
	*/
	image_id bild;

public:
	/**
	* Do we need to be called every step? Must be of (2^n-1)
	* public for performance reasons, this is accessed _very frequently_ !
	* @author Hj. Malthaner
	*/
	enum { max_step_frequency=255 };
	uint8 step_frequency;

private:
	/**
	* Used by all constructors to initialize all vars with safe values
	* -> single source principle
	* @author Hj. Malthaner
	*/
	void init(karte_t *welt);


protected:
	// free for additional info; specific for each object
	uint8 extra_info;

	/**
	* Basiskonstruktor
	* @author Hj. Malthaner
	*/
	ding_t(karte_t *welt);

	/**
	* Erzeugt ein Info-Fenster f�r dieses Objekt
	* @author V. Meyer
	*/
	virtual ding_infowin_t *new_info();

	/**
	* Offene Info-Fenster
	* @author V. Meyer
	*/
	static ptrhashtable_tpl<ding_t *, ding_infowin_t *> * ding_infos;

	/**
	* Pointer to the world of this thing. Static to conserve space.
	* Change to instance variable once more than one world is available.
	* @author Hj. Malthaner
	*/
	static karte_t *welt;


public:
	/**
	* setzt den Besitzer des dings
	* (public wegen Rathausumbau - V.Meyer)
	* @author Hj. Malthaner
	*/
	void setze_besitzer(spieler_t *sp);

	/**
	* Ein Objekt kann einen Besitzer haben.
	* @return Einen Zeiger auf den Besitzer des Objekts oder NULL,
	* wenn das Objekt niemand geh�rt.
	* @author Hj. Malthaner
	*/
	spieler_t * gib_besitzer() const;

	void entferne_ding_info();

	/**
	* setzt ein flag im flag-set des dings. Siehe auch flag_values
	* @author Hj. Malthaner
	*/
	inline void set_flag(enum flag_values flag) {flags |= flag;}
	inline void clear_flag(enum flag_values flag) {flags &= ~flag;}
	inline bool get_flag(enum flag_values flag) const {return ((flags & flag) != 0);}

	enum typ {
		undefined=-1, ding=0, baum=1, zeiger=2,
		wolke=3, sync_wolke=4, async_wolke=5,

		gebaeude=7,	// animated buidldings (not used any more)
		signal=8,

		bruecke=9, tunnel=10,
		bahndepot=12, strassendepot=13, schiffdepot = 14,

		raucher=15,
		leitung = 16, pumpe = 17, senke = 18,
		roadsign = 19, pillar = 20,

		airdepot = 21, monoraildepot=22, tramdepot=23,

		wayobj = 25,
		way = 26, // since 99.04 ways are normal things and stored in the dingliste_t!

		// after this only moving stuff
		// vehikel sind von 64 bis 95
		fussgaenger=64,
		verkehr=65,
		automobil=66,
		waggon=67,
		monorailwaggon=68,
		maglevwaggon=69, // currently unused
		schiff=80,
		aircraft=81,

		// other new dings (obsolete, only used during loading old games
		// lagerhaus = 24, (never really used)
		// gebaeude_alt=6,	(very, very old?)
		old_gebaeudefundament=11,	// wall below buildings, not used any more
		old_automobil=32, old_waggon=33,
		old_schiff=34, old_aircraft=35, old_monorailwaggon=36,
		old_verkehr=41,
		old_fussgaenger=42,
		old_choosesignal = 95,
		old_presignal = 96,
		old_roadsign = 97,
		old_pillar = 98,
		old_airdepot = 99,
		old_monoraildepot=100,
		old_tramdepot=101,
	};

	inline const sint8 gib_xoff() const {return xoff;}
	inline const sint8 gib_yoff() const {return yoff;}

	// true for all moving objects
	inline bool is_moving() const { return flags&is_vehicle; }

	// true for all moving objects
	inline bool is_way() const { return flags&is_wayding; }

	// while in principle, this should trigger the dirty, it takes just too much time to do it
	// TAKE CARE OF SET IT DIRTY YOURSELF!!!
	inline void setze_xoff(sint8 xoff) {this->xoff = xoff; }
	inline void setze_yoff(sint8 yoff) {this->yoff = yoff; }

	/**
	 * Mit diesem Konstruktor werden Objekte aus einer Datei geladen
	 *
	 * @param welt Zeiger auf die Karte, in die das Objekt geladen werden soll.
	 * @param file Dateizeiger auf die Datei, die die Objektdaten enth�lt.
	 * @author Hj. Malthaner
	 */
	ding_t(karte_t *welt, loadsave_t *file);

	/**
	 * Mit diesem Konstruktor werden Objekte f�r den Boden[x][y][z] erzeugt,
	 * diese Objekte m�ssem nach der Erzeugung mit
	 * plan[x][y][z].obj_add explizit auf das Planquadrat gesezt werden.
	 *
	 * @param welt Zeiger auf die Karte, zu der das Objekt geh�ren soll.
	 * @param pos Die Koordinate des Planquadrates.
	 * @author Hj. Malthaner
	 */
	ding_t(karte_t *welt, koord3d pos);

	/**
	 * Der Destruktor schlie�t alle Beobachtungsfenster f�r dieses Objekt.
	 * Er entfernt das Objekt aus der Karte.
	 * @author Hj. Malthaner
	 */
	virtual ~ding_t();

	/**
	 * Zum buchen der Abrisskosten auf das richtige Konto
	 * @author Hj. Malthaner
	 */
	virtual void entferne(spieler_t *) {}

	/**
	 * 'Jedes Ding braucht einen Namen.'
	 * @return Gibt den un�bersetzten(!) Namen des Objekts zur�ck.
	 * @author Hj. Malthaner
	 */
	virtual const char *gib_name() const {return "Ding";}

	/**
	 * 'Jedes Ding braucht einen Typ.'
	 * @return Gibt den typ des Objekts zur�ck.
	 * @author Hj. Malthaner
	 */
	virtual enum ding_t::typ gib_typ() const = 0;

	/**
	 * Methode f�r asynchrone Funktionen eines Objekts.
	 * @return false to be deleted after the step, true to live on
	 * @author Hj. Malthaner
	 */
	virtual bool step(long /*delta_t*/) {return true;}

	/*
	* called whenever the snowline height changes
	* return fals and the ding_t will be deleted
	* @author prissi
	*/
	virtual bool check_season(const long /*month*/) { return true; }

	/**
	 * Jedes Objekt braucht ein Bild.
	 * @return Die Nummer des aktuellen Bildes f�r das Objekt.
	 * @author Hj. Malthaner
	 */
	inline image_id gib_bild() const {return bild;}

	/**
	 * give image for height > 0 (max. height currently 3)
	 * IMG_LEER is no images
	 * @author Hj. Malthaner
	 */
	virtual image_id gib_bild(int /*height*/) const {return IMG_LEER;}

	/**
	 * this image is draw after all gib_bild() on this tile
	 * Currently only single height is supported for this feature
	 */
	virtual image_id gib_after_bild() const {return IMG_LEER;}

	/**
	 * Setzt ein Bild des Objects, normalerweise ist nur bild 0
	 * setzbar.
	 * @param n bild index
	 * @param bild bild nummer
	 * @author Hj. Malthaner
	 */
	virtual void setze_bild(int n, image_id bild);

	/**
	 * Ein Objekt kann zu einer Fabrik geh�ren.
	 * @return Einen Zeiger auf die Fabrik zu der das Objekt geh�rt oder NULL,
	 * wenn das Objekt zu keiner Fabrik geh�rt.
	 * @author Hj. Malthaner
	 */
	virtual inline fabrik_t* get_fabrik() const {return NULL;}

	/**
	 * Speichert den Zustand des Objekts.
	 *
	 * @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
	 * soll.
	 * @author Hj. Malthaner
	 */
	virtual void rdwr(loadsave_t *file);

	/**
	 * Wird nach dem Laden der Welt aufgerufen - �blicherweise benutzt
	 * um das Aussehen des Dings an Boden und Umgebung anzupassen
	 *
	 * @author Hj. Malthaner
	 */
	virtual void laden_abschliessen();

	/**
	 * Ein Objekt geh�rt immer zu einem grund_t
	 * @return Die aktuellen Koordinaten des Grundes.
	 * @author V. Meyer
	 * @see ding_t#ding_t
	 */
	inline koord3d gib_pos() const {return pos;}

	// only zeiger_t overlays this function, so virtual definition is overkill
	inline void setze_pos(koord3d k) { if(k!=pos) { set_flag(dirty); pos = k;} }

	/**
	 * @return Einen Beschreibungsstring f�r das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	virtual void info(cbuffer_t & buf) const;

	/**
	 * �ffnet ein neues Beobachtungsfenster f�r das Objekt.
	 * @author Hj. Malthaner
	 */
	virtual void zeige_info();

	/**
	 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char * ist_entfernbar(const spieler_t *sp);

	/**
	 * Ding zeichnen
	 * @author Hj. Malthaner
	 */
	void display(int xpos, int ypos, bool dirty) const;

	/**
	 * 2. Teil zeichnen (was hinter Fahrzeugen kommt
	 * @author V. Meyer
	 */
	virtual void display_after(int xpos, int ypos, bool dirty) const;

	/*
	* when a vehicle moves or a cloud moves, it needs to mark the old spot as dirty (to copy to screen)
	* sometimes they have an extra offset, this the yoff parameter
	* @author prissi
	*/
	void mark_image_dirty(image_id bild,sint8 yoff);

	/**
	 * Dient zur Neuberechnung des Bildes
	 * @author Hj. Malthaner
	 */
	virtual void calc_bild() {}
} GCC_PACKED;

#endif
