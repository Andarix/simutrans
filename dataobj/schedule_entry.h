/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef schedule_entry_t_h
#define schedule_entry_t_h

#include "koord3d.h"

/**
 * A schedule entry.
 */
struct schedule_entry_t
{
public:
	schedule_entry_t() {}

	schedule_entry_t(koord3d const& pos, uint const minimum_loading, sint8 const waiting_time_shift) :
		pos(pos),
		minimum_loading(minimum_loading),
		waiting_time_shift(waiting_time_shift)
	{}

	/**
	 * target position
	 */
	koord3d pos;

	/**
	 * Wait for % load at this stops
	 * (ignored on waypoints)
	 */
	uint8 minimum_loading;

	/**
	 * maximum waiting time in 1/2^(16-n) parts of a month
	 * (only active if minimum_loading!=0)
	 */
	sint8 waiting_time_shift;
};

inline bool operator ==(const schedule_entry_t &a, const schedule_entry_t &b)
{
	return a.pos == b.pos  &&  a.minimum_loading == b.minimum_loading  &&  a.waiting_time_shift == b.waiting_time_shift;
}


#endif
