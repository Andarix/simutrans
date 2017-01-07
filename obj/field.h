#ifndef obj_field_h
#define obj_field_h

#include "../simobj.h"
#include "../display/simimg.h"


class field_class_besch_t;
class fabrik_t;

class field_t : public obj_t
{
	fabrik_t *fab;
	const field_class_besch_t *desc;

public:
	field_t(const koord3d pos, player_t *player, const field_class_besch_t *desc, fabrik_t *fab);
	virtual ~field_t();

	const char* get_name() const { return "Field"; }
	typ get_typ() const { return obj_t::field; }

	image_id get_image() const;

	/**
	 * @return Einen Beschreibungsstring f�r das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void show_info();

	/**
	 * @return NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	const char * is_deletable(const player_t *);

	void cleanup(player_t *player);
};

#endif
