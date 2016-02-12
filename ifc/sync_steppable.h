#ifndef sync_steppable_h
#define sync_steppable_h

#include "../simtypes.h"

enum sync_result {
	SYNC_OK,     ///< object remains in list
	SYNC_REMOVE, ///< remove object from list
	SYNC_DELETE, ///< delete object and remove from list
};

/**
 * All synchronously moving things must implement this interface.
 *
 * @author Hj. Malthaner
 */
class sync_steppable
{
public:
    /**
     * Method for real-time features of an object.
     */
    virtual sync_result sync_step(uint32 delta_t) = 0;

    virtual ~sync_steppable() {}
};

#endif
