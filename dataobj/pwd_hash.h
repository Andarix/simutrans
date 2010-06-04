#ifndef PWD_HASH_H
#define PWD_HASH_H

#include <string.h>

#include "../macros.h"
#include "../simtypes.h"

class pwd_hash_t
{
	public:
		pwd_hash_t() { clear(); }

		void clear() { memset(hash, 0, sizeof(hash)); }

		bool empty() const
		{
			for (uint8 const* i = hash; i != endof(hash); ++i) {
				if (*i != 0) return false;
			}
			return true;
		}

		uint8& operator [](size_t const i) { return hash[i]; }

		bool operator ==(uint8 const* const o) const { return memcmp(hash, o, sizeof(hash)) == 0; }

		bool operator !=(uint8 const* const o) const { return !(*this == o); }

	private:
		uint8 hash[20];
};

#endif
