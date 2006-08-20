/*
 * weg.cc
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


/*
 * weg.cc
 *
 * Basisklasse f�r Wege in Simutrans.
 *
 * 14.06.00 getrennt von simgrund.cc
 * �berarbeitet Januar 2001
 *
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "weg.h"
#include "../grund.h"
#include "../../simworld.h"
#include "../../simimg.h"
#include "../../simhalt.h"
#include "../../utils/cbuffer_t.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/loadsave.h"
#include "../../besch/weg_besch.h"

#include "../../tpl/slist_tpl.h"

/**
 * Pointer to the world of this way. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_t * weg_t::welt = NULL;


/**
 * Alle instantiierten Wege
 * @author Hj. Malthaner
 */
slist_tpl <weg_t *> alle_wege;



/**
 * Get list of all ways
 * @author Hj. Malthaner
 */
const slist_tpl <weg_t *> & weg_t::gib_alle_wege()
{
  return alle_wege;
}


/**
 * Setzt die erlaubte H�chstgeschwindigkeit
 * @author Hj. Malthaner
 */
void weg_t::setze_max_speed(unsigned int s)
{
  max_speed = s;
}


/**
 * Setzt neue Beschreibung. Ersetzt alte H�chstgeschwindigkeit
 * mit wert aus Beschreibung.
 * @author Hj. Malthaner
 */
void weg_t::setze_besch(const weg_besch_t *b)
{
  besch = b;
  max_speed = besch->gib_topspeed();
}


/**
 * initializes statistic array
 * @author hsiegeln
 */
void weg_t::init_statistics()
{
  for (int type=0; type<MAX_WAY_STATISTICS; type++) {
    for (int month=0; month<MAX_WAY_STAT_MONTHS; month++) {
      statistics[month][type] = 0;
    }
  }
}


/**
 * Initializes all member variables
 * @author Hj. Malthaner
 */
void weg_t::init(karte_t *welt)
{
  this->welt = welt;
  ribi = ribi_maske = ribi_t::keine;
  max_speed = 450;

  pos = koord3d::invalid;

  besch = 0;

  init_statistics();

  alle_wege.insert(this);
}


weg_t::weg_t(karte_t *welt)
{
  init(welt);
}


weg_t::weg_t(karte_t *welt, loadsave_t * /* file */)
{
  init(welt);
}


weg_t::~weg_t()
{
  alle_wege.remove(this);
}


void weg_t::rdwr(loadsave_t *file)
{

	uint8 ribi8;
	if(file->is_saving()) {
		// reading has been done by grund_t!
		file->wr_obj_id(
//			(gib_typ()==weg_t::schiene  &&  besch->gib_styp()==7) ? weg_t::schiene_strab : gib_typ()
			gib_typ()
		 );
		 // init ribi
		ribi8 = ribi | (ribi_maske << 4);
	}
	file->rdwr_byte(ribi8, "\n");
	if(file->is_loading()) {
		ribi = ribi8 & 0xF;
		ribi_maske = ribi8 >> 4;
	}

	file->rdwr_short(max_speed, "\n");

	for (int type=0; type<MAX_WAY_STATISTICS; type++) {
		for (int month=0; month<MAX_WAY_STAT_MONTHS; month++) {
			long w=statistics[month][type];
			file->rdwr_long(w, "\n");
			statistics[month][type] = w;
			// DBG_DEBUG("weg_t::rdwr()", "statistics[%d][%d]=%d", month, type, statistics[month][type]);
		}
	}
}


/**
 * Info-text f�r diesen Weg
 * @author Hj. Malthaner
 */
void weg_t::info(cbuffer_t & buf) const
{
	buf.append("\n");
	buf.append(translator::translate("Max. speed:"));
	buf.append(" ");
	buf.append(max_speed);
	buf.append("km/h\n");

#if 1
	buf.append(translator::translate("\nconvoi passed last\nmonth "));
      buf.append(statistics[1][1]);
  // Debug - output stats
#else
  buf.append("\n");
  for (int type=0; type<MAX_WAY_STATISTICS; type++) {
    for (int month=0; month<MAX_WAY_STAT_MONTHS; month++) {
      buf.append(statistics[month][type]);
      buf.append(" ");
    }
    buf.append("\n");
  }
#endif
  buf.append("\n");
}


/**
 * Die Bezeichnung des Wegs
 * @author Hj. Malthaner
 */
const char * weg_t::gib_name() const {
  if(besch) {
    return besch->gib_name();
  } else {
    return gib_typ_name();
  }
}


void
weg_t::calc_bild()
{
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(pos);
	grund_t *to;

	if(from==NULL  ||  besch==NULL) {
		bild_nr = IMG_LEER;
	}

	hang_t::typ hang = from->gib_weg_hang();
	if(hang != hang_t::flach) {
		bild_nr = besch->gib_hang_bild_nr(hang);
		return;
	}

	const ribi_t::ribi ribi = gib_ribi_unmasked();

	if(ribi_t::ist_kurve(ribi)) {
		ribi_t::ribi r1 = ribi_t::keine, r2 = ribi_t::keine;

		bool diagonal = false;
		switch(ribi) {
			case ribi_t::nordost:
				if(from->get_neighbour(to, gib_typ(), koord::ost))
					r1 = to->gib_weg_ribi_unmasked(gib_typ());
				if(from->get_neighbour(to, gib_typ(), koord::nord))
					r2 = to->gib_weg_ribi_unmasked(gib_typ());
				diagonal =
					(r1 == ribi_t::suedwest || r2 == ribi_t::suedwest) &&
					r1 != ribi_t::nordwest &&
					r2 != ribi_t::suedost;
			break;

			case ribi_t::suedost:
				if(from->get_neighbour(to, gib_typ(), koord::ost))
					r1 = to->gib_weg_ribi_unmasked(gib_typ());
				if(from->get_neighbour(to, gib_typ(), koord::sued))
					r2 = to->gib_weg_ribi_unmasked(gib_typ());
				diagonal =
					(r1 == ribi_t::nordwest || r2 == ribi_t::nordwest) &&
					r1 != ribi_t::suedwest &&
					r2 != ribi_t::nordost;
			break;

			case ribi_t::nordwest:
				if(from->get_neighbour(to, gib_typ(), koord::west))
					r1 = to->gib_weg_ribi_unmasked(gib_typ());
				if(from->get_neighbour(to, gib_typ(), koord::nord))
					r2 = to->gib_weg_ribi_unmasked(gib_typ());
				diagonal =
					(r1 == ribi_t::suedost || r2 == ribi_t::suedost) &&
					r1 != ribi_t::nordost &&
					r2 != ribi_t::suedwest;
			break;

			case ribi_t::suedwest:
				if(from->get_neighbour(to, gib_typ(), koord::west))
					r1 = to->gib_weg_ribi_unmasked(gib_typ());
				if(from->get_neighbour(to, gib_typ(), koord::sued))
					r2 = to->gib_weg_ribi_unmasked(gib_typ());
				diagonal =
					(r1 == ribi_t::nordost || r2 == ribi_t::nordost) &&
					r1 != ribi_t::suedost &&
					r2 != ribi_t::nordwest;
				break;
		}

		if(diagonal) {
			static int rekursion = 0;

			if(rekursion == 0) {
				rekursion++;
				for(int r = 0; r < 4; r++) {
					if(from->get_neighbour(to, gib_typ(), koord::nsow[r])) {
						to->calc_bild();
					}
				}
				rekursion--;
			}

			int bild = besch->gib_diagonal_bild_nr(ribi);
			if(bild != -1) {
				bild_nr = bild;
				return;
			}
		}
	}

	bild_nr = besch->gib_bild_nr(ribi);
//	DBG_MESSAGE("bild_nr_offset","%i",bild_nr);
}


/**
 * new month
 * @author hsiegeln
 */
void weg_t::neuer_monat()
{
	for (int type=0; type<MAX_WAY_STATISTICS; type++) {
		for (int month=MAX_WAY_STAT_MONTHS-1; month>0; month--) {
			statistics[month][type] = statistics[month-1][type];
		}
		statistics[0][type] = 0;
	}
}
