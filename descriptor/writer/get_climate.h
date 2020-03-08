/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GET_CLIMATE_H
#define GET_CLIMATE_H

#include "../../simtypes.h"


/**
 * Convert climates string to bitfield
 */
climate_bits get_climate_bits(const char* climate_str);

#endif
