#ifndef KOORD_H
#define KOORD_H

#include <stdlib.h>

#include "ribi.h"
#include "../simtypes.h"

class loadsave_t;

/**
 * 2d Koordinaten
 */
class koord
{
public:
	sint16 x;
	sint16 y;

	koord() : x(0), y(0) {}

	koord(short xp, short yp) : x(xp), y(yp) {}
	koord(loadsave_t* file);
	koord(ribi_t::ribi ribi) { *this = from_ribi[ribi]; }
	koord(hang_t::typ hang)  { *this = from_hang[hang]; }

	// use this instead of koord(simrand(x),simrand(y)) to avoid
	// different order on different compilers
	static koord koord_random(uint16 xrange, uint16 yrange);

	void rdwr(loadsave_t *file);

	const char *get_str() const;

	const koord& operator += (const koord & k)
	{
		x += k.x;
		y += k.y;
		return *this;
	}

	const koord& operator -= (const koord & k)
	{
		x -= k.x;
		y -= k.y;
		return *this;
	}

	void rotate90( sint16 y_size )
	{
		if(  (x&y)<0  ) {
			// do not rotate illegal coordinates
			return;
		}
		sint16 new_x = y_size-y;
		y = x;
		x = new_x;
	}

	static const koord invalid;
	static const koord nord;
	static const koord sued;
	static const koord ost;
	static const koord west;
	// die 4 Grundrichtungen als Array
	static const koord nsow[4];
	// 8 next neighbours
	static const koord neighbours[8];

private:
	static const koord from_ribi[16];
	static const koord from_hang[16];
};


static inline uint32 koord_distance(const koord &a, const koord &b)
{
	return abs(a.x - b.x) + abs(a.y - b.y);
}

// Knightly : shortest distance in cardinal (N, E, S, W) and ordinal (NE, SE, SW, NW) directions
static inline uint32 shortest_distance(const koord &a, const koord &b)
{
	const uint32 x_offset = abs(a.x - b.x);
	const uint32 y_offset = abs(a.y - b.y);
	// square root of 2 is estimated by 181/128; 64 is for rounding
	if(  x_offset>=y_offset  ) {
		return (x_offset - y_offset) + ( ((y_offset * 181u) + 64u) >> 7 );
	}
	else {
		return (y_offset - x_offset) + ( ((x_offset * 181u) + 64u) >> 7 );
	}
}

// Knightly : multiply the value by the distance weight
static inline uint32 weight_by_distance(const uint32 value, const uint32 distance)
{
	return (uint32)( ((sint64)value << 10) / (sint64)(distance < 4u ? 4u : distance) );
}

static inline koord operator * (const koord &k, const sint16 m)
{
	return koord(k.x * m, k.y * m);
}


static inline koord operator / (const koord &k, const sint16 m)
{
	return koord(k.x / m, k.y / m);
}


static inline bool operator == (const koord &a, const koord &b)
{
	// only this works with O3 optimisation!
	return ((a.x-b.x)|(a.y-b.y))==0;
}


static inline bool operator != (const koord &a, const koord &b)
{
	// only this works with O3 optimisation!
	return ((a.x-b.x)|(a.y-b.y))!=0;
}


static inline koord operator + (const koord &a, const koord &b)
{
	return koord(a.x + b.x, a.y + b.y);
}


static inline koord operator - (const koord &a, const koord &b)
{
	return koord(a.x - b.x, a.y - b.y);
}


static inline koord operator - (const koord &a)
{
	return koord(-a.x, -a.y);
}

#endif
