/*
 * all defines that can change the compiling
 */

#ifndef simconst_h
#define simconst_h

// number of player
#define MAX_PLAYER_COUNT (16)
#define PLAYER_UNOWNED (15)

/* Flag for Intel byte order
 * SET THIS IN YOUR COMPILER COMMAND LINE!
 */
//#define LITTLE_ENDIAN

/* crossconnect industry and half heights like openTTD */
//#define OTTD_LIKE

/* single height is only 8 pixel (default 16) */
//#define HALF_HEIGHT

/* construct automatic bridges also as active player */
//#define AUTOMATIC_BRIDGES

/* construct automatic tunnels also as active player */
//#define AUTOMATIC_TUNNELS

/* citycars have a destination; if they could get near, they dissolve */
//#define DESTINATION_CITYCARS

/* need to emulate the mouse pointer with a graphic */
//#define USE_SOFTPOINTER

/* Use assembly imepmentation of routines if available
* Some routines, eg for drawing, can have assembly implementations for certaint platforms that may perform better */
#define USE_ASSEMBLER

/* Use low level C/C++ implementations of routines
 * Some routines, eg for drawing, can have low level C++ implementations that might perform better on certain platforms */
#define LOW_LEVEL

// The wind (i.e. approach direction) is random all over the map (not recommended, since it confuses players)
//#define USE_DIFFERENT_WIND

// define this for automatically joining stations next to a public stop with it
//#define AUTOJOIN_PUBLIC

// allow minspeed and private way signs on waterways (imho pointless)
//#define ENABLE_WATERWAY_SIGNS

// Maximum number of threads
#define MAX_THREADS (12)


/*********************** Useful things for debugging ... ********************/

/* will highlight marked areas and convoi will leave traces */
//#define DEBUG_ROUTES

/* shows which tiles are drawn as dings (used in boden/grund.cc) */
//#define SHOW_FORE_GRUND

/* shows with block needed update and which not */
//#define DEBUG_FLUSH_BUFFER

/* define USE_VALGRIND_MEMCHECK to make valgrind aware of the freelist memory pool */
//#define USE_VALGRIND_MEMCHECK

/* define this to check for double free and sizes for freelist */
//#define DEBUG_FREELIST


/**************************** automatic stuff ********************************/


// inclination types
// constants used in tools tool_setslope / tool_restoreslope_t
#define ALL_UP_SLOPE (82)
#define ALL_DOWN_SLOPE (83)
#define RESTORE_SLOPE (84)
#define ALL_UP_SLOPE_SINGLE (16)
#define ALL_DOWN_SLOPE_SINGLE (17)
#define RESTORE_SLOPE_SINGLE (18)


// 16 internal pixels per tile, for purposes of object visual offsets.
#define OBJECT_OFFSET_STEPS (16)
// These must be changed (along with lots of other code) if OBJECT_OFFSET_STEPS is changed.
#define tile_raster_scale_x(v, rw)   (((v)*(rw)) >> 6)
#define tile_raster_scale_y(v, rh)   (((v)*(rh)) >> 6)

#define INVALID_INDEX (65530u)

// offsets for mouse pointer
#define Z_PLAN (4)
#define Z_GRID (0)

#endif
