/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_schiene_h
#define boden_wege_schiene_h


#include "weg.h"
#include "../../convoihandle_t.h"

class vehikel_t;

/**
 * Klasse f�r Schienen in Simutrans.
 * Auf den Schienen koennen Z�ge fahren.
 * Jede Schiene geh�rt zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 */
class schiene_t : public weg_t
{
protected:
	/**
	* Bound when this block was successfully reserved by the convoi
	* @author prissi
	*/
	convoihandle_t reserved;

public:
	static const weg_besch_t *default_schiene;

	static bool show_reservations;

	/**
	* File loading constructor.
	* @author Hj. Malthaner
	*/
	schiene_t(karte_t *welt, loadsave_t *file);

	schiene_t(karte_t *welt);

	virtual waytype_t get_waytype() const {return track_wt;}

	/**
	* Calculates the image of this pice of railroad track.
	*
	* @author Hj. Malthaner
	*/
	void calc_bild(koord3d) { weg_t::calc_bild(); }

	/**
	* @return additional info is reservation!
	* @author prissi
	*/
	void info(cbuffer_t & buf) const;

	/**
	* true, if this rail can be reserved
	* @author prissi
	*/
	bool can_reserve(convoihandle_t c) const { return !reserved.is_bound()  ||  c==reserved; }

	/**
	* true, if this rail can be reserved
	* @author prissi
	*/
	bool is_reserved() const { return reserved.is_bound(); }

	/**
	* true, then this rail was reserved
	* @author prissi
	*/
	bool reserve(convoihandle_t c, ribi_t::ribi dir);

	/**
	* releases previous reservation
	* @author prissi
	*/
	bool unreserve( convoihandle_t c);

	/**
	* releases previous reservation
	* @author prissi
	*/
	bool unreserve( vehikel_t *);

	/* called befor deletion;
	 * last chance to unreserve tiles ...
	 */
	virtual void entferne(spieler_t *sp);

	/**
	* gets the related convoi
	* @author prissi
	*/
	convoihandle_t get_reserved_convoi() const {return reserved;}

	void rdwr(loadsave_t *file);

	/**
	 * if a function return here a value with TRANSPARENT_FLAGS set
	 * then a transparent outline with the color form the lower 8 Bit is drawn
	 * @author kierongreen
	 */
	virtual PLAYER_COLOR_VAL get_outline_colour() const { return (show_reservations  &&  reserved.is_bound()) ? TRANSPARENT75_FLAG | OUTLINE_FLAG | COL_RED : 0;}

	/*
	 * to show reservations if needed
	 */
	virtual PLAYER_COLOR_VAL get_outline_bild() const {return weg_t::get_bild();}
};

#endif
