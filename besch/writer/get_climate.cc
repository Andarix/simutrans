#include "../../simtypes.h"
#include <string.h>
#include "../../utils/cstring_t.h"	// for STRICMP
#include "../grund_besch.h"
#include "../../dataobj/tabfile.h"


// the water and seven climates
static const char* const climate_names[MAX_CLIMATES] =
{
	"water", "desert", "tropic", "mediterran", "temperate", "tundra", "rocky", "arctic"
};


/**
 * Convert climates string to bitfield
 * @author Hj. Malthaner
 */
climate_bits get_climate_bits(const char* climate_str)
{
	uint16 uv16 = 0;
	uint16 i;
	const char* c = climate_str;

	do {
		while (*c == ' ' || *c == ',') {
			c++;
		}
		char end[256];
		// skip the rest
		for (i = 0; i < 255 && *c > ' ' && *c != ','; i++, c++) {
			end[i] = *c;
		}
		end[i] = 0;

		// search for the string
		for (int i = 0; i < MAX_CLIMATES; i++) {
			if (STRICMP(end, climate_names[i]) == 0) {
				uv16 |= 1 << i;
				break;
			}
		}
	} while (*c > 0 && *c != '#');

	return (climate_bits)uv16;
}
