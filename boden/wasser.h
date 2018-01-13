#ifndef boden_wasser_h
#define boden_wasser_h

#include "grund.h"

/**
 * The water ground models rivers and lakes in Simutrans.
 *
 * @author Hj. Malthaner
 */

class wasser_t : public grund_t
{
protected:
	/// cache ribis to tiles connected by water
	ribi_t::ribi ribi;
	/// helper variable, ribis to canal tiles, where ships cannot turn left or right
	ribi_t::ribi canal_ribi;

	/**
	 * This method also recalculates ribi and cache_ribi!
	 */
	void calc_image_internal(const bool calc_only_snowline_change);

public:

	wasser_t(loadsave_t *file, koord pos ) : grund_t(koord3d(pos,0) ), ribi(ribi_t::none) { rdwr(file); }
	wasser_t(koord3d pos) : grund_t(pos), ribi(ribi_t::none) {}

	inline bool is_water() const { return true; }

	// returns correct directions for water and none for the rest ...
	ribi_t::ribi get_weg_ribi(waytype_t typ) const { return (typ==water_wt) ? ribi : (ribi_t::ribi)ribi_t::none; }
	ribi_t::ribi get_weg_ribi_unmasked(waytype_t typ) const  { return (typ==water_wt) ? ribi : (ribi_t::ribi)ribi_t::none; }

	const char *get_name() const {return "Water";}
	grund_t::typ get_typ() const {return wasser;}

	// map rotation
	void rotate90();

	ribi_t::ribi get_canal_ribi() const { return canal_ribi; }

	// static stuff from here on for water animation
	static int stage;
	static bool change_stage;

	static void prepare_for_refresh();
};

#endif
