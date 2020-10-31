/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef MACROS_H
#define MACROS_H


#include "simtypes.h"

#include <cstring>


// XXX Workaround: Old GCCs choke on type check.
#if !defined __GNUC__ || GCC_ATLEAST(3, 0)
// Ensures that the argument has array type.
template <typename T, unsigned N> static inline void lengthof_check(T (&)[N]) {}
#	define lengthof(x) (1 ? sizeof(x) / sizeof(*(x)) : (lengthof_check((x)), 0))
#else
#	define lengthof(x) (sizeof(x) / sizeof(*(x)))
#endif

#define endof(x) ((x) + lengthof(x))

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

#define MEMZERON(ptr, n) memset((ptr), 0, sizeof(*(ptr)) * (n))
#define MEMZERO(obj)     MEMZERON(&(obj), 1)

// make sure, a value in within the borders
template<typename T> static inline T clamp(T v, T l, T u)
{
	return v < l ? l : (v > u ? u :v);
}


namespace sim {

template<class T> inline void swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}

// XXX Workaround for GCC 2.95
template<typename T> static inline T up_cast(T x) { return x; }

}

#endif
