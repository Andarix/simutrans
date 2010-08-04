#ifndef dings_pillar_h
#define dings_pillar_h

#include "../simdings.h"
#include "../simworld.h"
#include "../besch/bruecke_besch.h"

/**
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

class loadsave_t;
class karte_t;

class pillar_t : public ding_t
{
	const bruecke_besch_t *besch;
	uint8 dir;
	bool asymmetric;
	image_id bild;

protected:
	void rdwr(loadsave_t *file);

public:
	pillar_t(karte_t *welt, loadsave_t *file);
	pillar_t(karte_t *welt, koord3d pos, spieler_t *sp, const bruecke_besch_t *besch, bruecke_besch_t::img_t img, int hoehe);

	const char* get_name() const { return "Pillar"; }
	typ get_typ() const { return ding_t::pillar; }

	const bruecke_besch_t* get_besch() const { return besch; }

	image_id get_bild() const { return asymmetric ? IMG_LEER : bild; }

	// asymmetric pillars are placed at the southern/eastern boundary of the tile
	// thus the images have to be displayed after vehicles
	image_id get_after_bild() const { return asymmetric ? bild : IMG_LEER;}

	// needs to check for hiding asymmetric pillars
	void calc_bild();

	/**
	 * @return Einen Beschreibungsstring fuer das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void zeige_info();

	void rotate90();
};

#endif
