#ifndef obj_bruecke_h
#define obj_bruecke_h

class karte_t;

#include "../besch/bruecke_besch.h"
#include "../simobj.h"

/**
 * Brueckenteile (sichtbar)
 *
 * Hj. Malthaner
 */

class bruecke_t : public obj_no_info_t
{
private:
	const bruecke_besch_t *besch;
	bruecke_besch_t::img_t img;

protected:
	void rdwr(loadsave_t *file);

public:
	bruecke_t(loadsave_t *file);
	bruecke_t(koord3d pos, player_t *player, const bruecke_besch_t *besch, bruecke_besch_t::img_t img);

	const char *get_name() const {return "Bruecke";}
	typ get_typ() const { return bruecke; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const { return besch ? besch->get_waytype() : invalid_wt; }

	const bruecke_besch_t *get_besch() const { return besch; }

	// we will always replace first way image
	image_id get_image() const { return IMG_EMPTY; }

	image_id get_front_image() const;

	void calc_image();

	/**
	 * Called whenever the season or snowline height changes
	 * return false and the obj_t will be deleted
	 */
	bool check_season(const bool calc_only_season_change) { if(  !calc_only_season_change  ) { calc_image(); } return true; }  // depends on snowline only

	void finish_rd();

	void cleanup(player_t *player);

	void rotate90();
	/**
	 * @return NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char *is_deletable(const player_t *player);
};

#endif
