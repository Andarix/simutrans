#ifndef monorailboden_h
#define monorailboden_h

#include "grund.h"

class monorailboden_t : public grund_t
{
protected:
	void calc_bild_internal(const bool calc_only_snowline_change);

public:
	monorailboden_t(loadsave_t *file, koord pos ) : grund_t( koord3d(pos,0) ) { rdwr(file); }
	monorailboden_t(koord3d pos,slope_t::type slope);

	virtual void rdwr(loadsave_t *file);

	const char *get_name() const {return "Monorailboden";}
	typ get_typ() const { return monorailboden; }
};

#endif
