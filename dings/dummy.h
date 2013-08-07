#ifndef dings_dummy_h
#define dings_dummy_h

#include "../simdings.h"
#include "../display/simimg.h"


/**
 * prissi: a dummy typ for old things, which are now ignored
 */
class dummy_ding_t : public ding_t
{
	public:
		dummy_ding_t(karte_t* welt, loadsave_t* file) :
			ding_t(welt)
		{
			rdwr(file);
			// do not remove from this position, since there will be nothing
			ding_t::set_flag(ding_t::not_on_map);
		}

		typ      get_typ()  const { return ding_t::undefined; }
		image_id get_bild() const { return IMG_LEER; }
};

#endif
