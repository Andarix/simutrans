/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_weg_h
#define boden_wege_weg_h

#include "../../simimg.h"
#include "../../simtypes.h"
#include "../../simdings.h"
#include "../../besch/weg_besch.h"
#include "../../dataobj/koord3d.h"


class karte_t;
class weg_besch_t;
class cbuffer_t;
template <class T> class slist_tpl;


// maximum number of months to store information
#define MAX_WAY_STAT_MONTHS 2

// number of different statistics collected
#define MAX_WAY_STATISTICS 2

enum way_statistics {
	WAY_STAT_GOODS   = 0, ///< number of goods transported over this weg
	WAY_STAT_CONVOIS = 1  ///< number of convois that passed this weg
};


/**
 * <p>Der Weg ist die Basisklasse fuer alle Verkehrswege in Simutrans.
 * Wege "geh�ren" immer zu einem Grund. Sie besitzen Richtungsbits sowie
 * eine Maske fuer Richtungsbits.</p>
 *
 * <p>Ein Weg geh�rt immer zu genau einer Wegsorte</p>
 *
 * <p>Kreuzungen werden dadurch unterst�tzt, da� ein Grund zwei Wege
 * enthalten kann (prinzipiell auch mehrere m�glich.</p>
 *
 * <p>Wegtyp -1 ist reserviert und kann nicht f�r Wege benutzt werden<p>
 *
 * @author Hj. Malthaner
 */
class weg_t : public ding_no_info_t
{
public:
	/**
	* Get list of all ways
	* @author Hj. Malthaner
	*/
	static const slist_tpl <weg_t *> & get_alle_wege();

	enum {
		HAS_SIDEWALK   = 0x01,
		IS_ELECTRIFIED = 0x02,
		HAS_SIGN       = 0x04,
		HAS_SIGNAL     = 0x08,
		HAS_WAYOBJ     = 0x10,
		HAS_CROSSING   = 0x20,
		IS_DIAGONAL    = 0x40, // marker for diagonal image
		IS_SNOW = 0x80	// marker, if above snowline currently
	};

	enum system_type { type_flat=0, type_elevated=1, type_tram=7, type_underground=64, type_all=255 };

private:
	/**
	* array for statistical values
	* MAX_WAY_STAT_MONTHS: [0] = actual value; [1] = last month value
	* MAX_WAY_STATISTICS: see #define at top of file
	* @author hsiegeln
	*/
	sint16 statistics[MAX_WAY_STAT_MONTHS][MAX_WAY_STATISTICS];

	/**
	* Way type description
	* @author Hj. Malthaner
	*/
	const weg_besch_t * besch;

	/**
	* Richtungsbits f�r den Weg. Norden ist oben rechts auf dem Monitor.
	* 1=Nord, 2=Ost, 4=Sued, 8=West
	* @author Hj. Malthaner
	*/
	uint8 ribi:4;

	/**
	* Maske f�r Richtungsbits
	* @author Hj. Malthaner
	*/
	uint8 ribi_maske:4;

	/**
	* flags like walkway, electrification, road sings
	* @author Hj. Malthaner
	*/
	uint8 flags;

	/**
	* max speed; could not be taken for besch, since other object may modify the speed
	* @author Hj. Malthaner
	*/
	uint16 max_speed;

	image_id bild;

	/**
	* Initializes all member variables
	* @author Hj. Malthaner
	*/
	void init();

	/**
	* initializes statistic array
	* @author hsiegeln
	*/
	void init_statistics();

public:
	weg_t(karte_t* const welt, loadsave_t*) : ding_no_info_t(welt) { init(); }
	weg_t(karte_t* const welt) : ding_no_info_t(welt) { init(); }

	virtual ~weg_t();

	/* seasonal image recalculation */
	bool check_season(const long /*month*/);

	/* actual image recalculation */
	void calc_bild();

	/**
	* Setzt die erlaubte H�chstgeschwindigkeit
	* @author Hj. Malthaner
	*/
	void set_max_speed(uint16 s) { max_speed = s; }

	/**
	* Ermittelt die erlaubte H�chstgeschwindigkeit
	* @author Hj. Malthaner
	*/
	uint16 get_max_speed() const { return max_speed; }

	/**
	* Setzt neue Beschreibung. Ersetzt alte H�chstgeschwindigkeit
	* mit wert aus Beschreibung.
	* @author Hj. Malthaner
	*/
	void set_besch(const weg_besch_t *b);
	const weg_besch_t *get_besch() const { return besch; }

	// returns a way with the matching type
	static weg_t *alloc(waytype_t wt);

	// returns a string with the "official name of the waytype"
	static const char *waytype_to_string(waytype_t wt);

	virtual void rdwr(loadsave_t *file);

	/**
	* Info-text f�r diesen Weg
	* @author Hj. Malthaner
	*/
	virtual void info(cbuffer_t & buf) const;

	/**
	 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char *ist_entfernbar(const spieler_t *sp);

	/**
	* Wegtyp zur�ckliefern
	*/
	virtual waytype_t get_waytype() const = 0;

	/**
	* 'Jedes Ding braucht einen Typ.'
	* @return Gibt den typ des Objekts zur�ck.
	* @author Hj. Malthaner
	*/
	typ get_typ() const { return ding_t::way; }

	/**
	* Die Bezeichnung des Wegs
	* @author Hj. Malthaner
	*/
	const char *get_name() const { return besch->get_name(); }

	/**
	* Setzt neue Richtungsbits f�r einen Weg.
	*
	* Nachdem die ribis ge�ndert werden, ist das weg_bild des
	* zugeh�rigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
	* zur Reparatur mu� folgen).
	* @param ribi Richtungsbits
	*/
	void ribi_add(ribi_t::ribi ribi) { this->ribi |= (uint8)ribi;}

	/**
	* Entfernt Richtungsbits von einem Weg.
	*
	* Nachdem die ribis ge�ndert werden, ist das weg_bild des
	* zugeh�rigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
	* zur Reparatur mu� folgen).
	* @param ribi Richtungsbits
	*/
	void ribi_rem(ribi_t::ribi ribi) { this->ribi &= (uint8)~ribi;}

	/**
	* Setzt Richtungsbits f�r den Weg.
	*
	* Nachdem die ribis ge�ndert werden, ist das weg_bild des
	* zugeh�rigen Grundes falsch (Ein Aufruf von grund_t::calc_bild()
	* zur Reparatur mu� folgen).
	* @param ribi Richtungsbits
	*/
	void set_ribi(ribi_t::ribi ribi) { this->ribi = (uint8)ribi;}

	/**
	* Ermittelt die unmaskierten Richtungsbits f�r den Weg.
	*/
	ribi_t::ribi get_ribi_unmasked() const { return (ribi_t::ribi)ribi; }

	/**
	* Ermittelt die (maskierten) Richtungsbits f�r den Weg.
	*/
	ribi_t::ribi get_ribi() const { return (ribi_t::ribi)(ribi & ~ribi_maske); }

	/**
	* f�r Signale ist es notwendig, bestimmte Richtungsbits auszumaskieren
	* damit Fahrzeuge nicht "von hinten" �ber Ampeln fahren k�nnen.
	* @param ribi Richtungsbits
	*/
	void set_ribi_maske(ribi_t::ribi ribi) { ribi_maske = (uint8)ribi; }
	ribi_t::ribi get_ribi_maske() const { return (ribi_t::ribi)ribi_maske; }

	/**
	 * called during map rotation
	 * @author priss
	 */
	void rotate90();

	/**
	* book statistics - is called very often and therefore inline
	* @author hsiegeln
	*/
	void book(int amount, way_statistics type) { statistics[0][type] += amount; }

	/**
	* return statistics value
	* always returns last month's value
	* @author hsiegeln
	*/
	int get_statistics(int type) const { return statistics[1][type]; }

	/**
	* new month
	* @author hsiegeln
	*/
	void neuer_monat();

	void check_diagonal();

	void count_sign();

	/* flag query routines */
	void set_gehweg(const bool yesno) { flags = (yesno ? flags | HAS_SIDEWALK : flags & ~HAS_SIDEWALK); }
	inline bool hat_gehweg() const { return flags & HAS_SIDEWALK; }

	void set_electrify(bool janein) {janein ? flags |= IS_ELECTRIFIED : flags &= ~IS_ELECTRIFIED;}
	inline bool is_electrified() const {return flags&IS_ELECTRIFIED; }

	inline bool has_sign() const {return flags&HAS_SIGN; }
	inline bool has_signal() const {return flags&HAS_SIGNAL; }
	inline bool has_wayobj() const {return flags&HAS_WAYOBJ; }
	inline bool is_crossing() const {return flags&HAS_CROSSING; }
	inline bool is_diagonal() const {return flags&IS_DIAGONAL; }
	inline bool is_snow() const {return flags&IS_SNOW; }

	inline void set_bild( image_id b ) { bild = b; }
	image_id get_bild() const {return bild;}

	// correct maintainace
	void laden_abschliessen();
} GCC_PACKED;

#endif
