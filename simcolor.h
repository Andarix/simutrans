/* simconst.h
 *
 * all color related stuff
 */

#ifndef simcolor_h
#define simcolor_h

// this is a player color => use different table for conversion
#define PLAYER_FLAG (0x8000)

#define TRANSPARENT_FLAGS (0x7800)
#define TRANSPARENT25_FLAG (0x2000)
#define TRANSPARENT50_FLAG (0x4000)
#define TRANSPARENT75_FLAG (0x6000)
#define OUTLINE_FLAG (0x0800)

typedef unsigned short PLAYER_COLOR_VAL;
typedef unsigned char COLOR_VAL;

// Menuefarben (aendern sich nicht von Tag zu Nacht)
#define MN_GREY0  (229)
#define MN_GREY1  (230)
#define MN_GREY2  (231)
#define MN_GREY3  (232)
#define MN_GREY4  (233)


// fixed colors
#define COL_BLACK    (240)
#define COL_WHITE   (215)
#define COL_RED       (131)
#define COL_DARK_RED (128)
#define COL_LIGHT_RED (134)
#define COL_YELLOW      (171)
#define COL_DARK_YELLOW      (168)
#define COL_LIGHT_YELLOW      (175)
#define COL_BLUE      (147)
#define COL_DARK_BLUE      (144)
#define COL_LIGHT_BLUE      (151)
#define COL_GREEN      (140)
#define COL_DARK_GREEN     (136)
#define COL_LIGHT_GREEN      (143)
#define COL_ORANGE     (155)
#define COL_DARK_ORANGE     (153)
#define COL_LIGHT_ORANGE      (158)
#define COL_PURPLE     (76)
#define COL_DARK_PURPLE     (73)
#define COL_LIGHT_PURPLE      (79)

// message colors
#define CITY_KI (12)
#define NEW_VEHICLE COL_PURPLE

// by niels
#define COL_GREY1  (208)
#define COL_GREY2  (210)
#define COL_GREY3  (212)
#define COL_GREY4  (11)
#define COL_GREY5  (213)
#define COL_GREY6  (15)

// Kenfarben fuer die Karte
#define STRASSE_KENN      (210)
#define SCHIENE_KENN      (185)
#define CHANNEL_KENN      (147)
#define MONORAIL_KENN      (153)
#define RUNWAY_KENN      (58)
#define POWERLINE_KENN      (28)
#define HALT_KENN         COL_RED
#define VEHIKEL_KENN      COL_YELLOW

#define WIN_TITEL       (154)

#define MONEY_PLUS COL_BLACK
#define MONEY_MINUS COL_RED

// special chart colors
#define COL_REVENUE (157)
#define COL_OPERATION (23)
#define COL_MAINTENANCE (46)
#define COL_OPS_PROFIT (215)
#define COL_NEW_VEHICLES (31)
#define COL_CONSTRUCTION (110)
#define COL_PROFIT (79)
#define COL_CASH (63)
#define COL_VEHICLE_ASSETS (55)
#define COL_WEALTH (131)
#define COL_MARGIN (122)
#define COL_TRANSPORTED (141)
#define COL_FREE_CAPACITY (39)
#define COL_CITICENS COL_WHITE
#define COL_GROWTH (122)
#define COL_HAPPY COL_WHITE
#define COL_UNHAPPY COL_RED
#define COL_NO_ROUTE COL_BLUE
#define COL_WAITING COL_YELLOW
#define COL_ARRIVED COL_DARK_ORANGE
#define COL_DEPARTED COL_DARK_YELLOW

#endif
