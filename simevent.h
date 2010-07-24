/*
 * Header for system independant event handling routines
 *
 * Hj. Malthaner
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#ifndef simevent_h
#define simevent_h


/* Messageverarbeitung */

/* Klassen */

#define EVENT_NONE      0
#define EVENT_KEYBOARD  1
#define EVENT_CLICK     2
#define EVENT_RELEASE   3
#define EVENT_MOVE      4
#define EVENT_DRAG      5
#define EVENT_REPEAT    6

#define INFOWIN               7   // Hajo: window event, i.e. WIN_OPEN, WIN_CLOSE
#define WINDOW_RESIZE         8  //19-may-02	markus weber   added
#define WINDOW_MAKE_MIN_SIZE  9  //11-mar-03	(Mathew Hounsell) Added
#define WINDOW_CHOOSE_NEXT   10	// @author Volker Meyer @date  11.06.2003
#define WINDOW_REZOOM	     11	// @author Volker Meyer @date  20.06.2003

#define EVENT_SYSTEM    254
#define IGNORE_EVENT    255

/* codes */

#define MOUSE_LEFTBUTTON        1
#define MOUSE_RIGHTBUTTON       2
#define MOUSE_MIDBUTTON         4
#define MOUSE_WHEELUP           8  //hsiegeln 2003-11-04 added
#define MOUSE_WHEELDOWN        16  //hsiegeln 2003-11-04 added

#define WIN_OPEN    1
#define WIN_CLOSE   2
#define WIN_TOP     3
#define WIN_UNTOP   4 // loosing focus

#define NEXT_WINDOW 1
#define PREV_WINDOW 2

// Hajo: System event codes must match those from simsys.h !!!
#define SYSTEM_QUIT             1
#define SYSTEM_RESIZE           2

/* normal keys have range 0-255, special key follow above 255 */
/* other would be better for true unicode support :( */

/* control keys */
#define SIM_KEY_BACKSPACE             8
#define SIM_KEY_TAB                   9
#define SIM_KEY_ENTER                13
#define SIM_KEY_ESCAPE               27
#define SIM_KEY_SPACE                32
#define SIM_KEY_DELETE              127

/* arrow (direction) keys */
#define SIM_KEY_UP                  273
#define SIM_KEY_DOWN                274
#define SIM_KEY_RIGHT               275
#define SIM_KEY_LEFT                276

/* other navigation keys */
#define	SIM_KEY_HOME			    278
#define SIM_KEY_END				    279
#define SIM_KEY_PGUP                 62
#define SIM_KEY_PGDN                 60

/* Function keys */
#define SIM_KEY_F1                  256
#define SIM_KEY_F2                  257
#define SIM_KEY_F3                  258
#define SIM_KEY_F4                  259
#define SIM_KEY_F5                  260
#define SIM_KEY_F6                  261
#define SIM_KEY_F7                  262
#define SIM_KEY_F8                  263
#define SIM_KEY_F9                  264
#define SIM_KEY_F10                 265
#define SIM_KEY_F11                 266
#define SIM_KEY_F12                 267
#define SIM_KEY_F13                 268
#define SIM_KEY_F14                 269
#define SIM_KEY_F15                 270


/* makros */

#define IS_LEFTCLICK(ev) ((ev)->ev_class == EVENT_CLICK && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTRELEASE(ev) ((ev)->ev_class == EVENT_RELEASE && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTDRAG(ev) ((ev)->ev_class == EVENT_DRAG && (ev)->ev_code == MOUSE_LEFTBUTTON)
#define IS_LEFTREPEAT(ev) ((ev)->ev_class == EVENT_REPEAT && (ev)->ev_code == MOUSE_LEFTBUTTON)

#define IS_RIGHTCLICK(ev) ((ev)->ev_class == EVENT_CLICK && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTRELEASE(ev) ((ev)->ev_class == EVENT_RELEASE && (ev)->ev_code == MOUSE_RIGHTBUTTON)
#define IS_RIGHTDRAG(ev) ((ev)->ev_class == EVENT_DRAG && (ev)->ev_code == MOUSE_RIGHTBUTTON)

#define IS_WINDOW_RESIZE(ev) ((ev)->ev_class == WINDOW_RESIZE) //19-may-02	markus weber	added
#define IS_WINDOW_MAKE_MIN_SIZE(ev) ((ev)->ev_class == WINDOW_MAKE_MIN_SIZE) // 11-Mar-03 (Mathew Hounsell) Added
#define IS_WINDOW_CHOOSE_NEXT(ev) ((ev)->ev_class == WINDOW_CHOOSE_NEXT) // 11-Mar-03 (Mathew Hounsell) Added
#define IS_WINDOW_REZOOM(ev) ((ev)->ev_class == WINDOW_REZOOM)

#define IS_WHEELUP(ev) ((ev)->ev_class == EVENT_CLICK && (ev)->ev_code == MOUSE_WHEELUP)
#define IS_WHEELDOWN(ev) ((ev)->ev_class == EVENT_CLICK && (ev)->ev_code == MOUSE_WHEELDOWN)

// This macro is to determine if the event should be also handled by children of containers.
#define DOES_WINDOW_CHILDREN_NEED(ev) ((ev)->ev_class == INFOWIN || (ev)->ev_class == WINDOW_RESIZE || (ev)->ev_class == WINDOW_MAKE_MIN_SIZE ) // 11-Mar-03 (Mathew Hounsell) Added

#define IS_WINDOW_TOP(ev) ((ev)->ev_class == INFOWIN || (ev)->ev_class == WIN_TOP )

#define IS_LEFT_BUTTON_PRESSED(ev) ((ev)->button_state&1)
#define IS_RIGHT_BUTTON_PRESSED(ev) (((ev)->button_state&2)>>1)
#define IS_MIDDLE_BUTTON_PRESSED(ev) (((ev)->button_state&4)>>2)

#define IS_SHIFT_PRESSED(ev) ((ev)->ev_key_mod&1u)
#define IS_CONTROL_PRESSED(ev) (((ev)->ev_key_mod&2u)>>1)

/**
 * Slight explanation of event_t structure:
 * ev_class and ev_code speak for itself.
 * ev_class = EVENT_NONE:      nothing defined
 * ev_class = EVENT_KEYBOARD:  code = key pressed (released key is ignored)
 * ev_class = EVENT_CLICK:     mx/my/cx/cy point to mouse click place,
 *                             code = pressed mouse button
 * ev_class = EVENT_RELEASE:   cx/cy point to mouse click place,
 *                             mx/my point to mouse release place,
 *                             code = mouse release button
 * ev_class = EVENT_MOVE:      cx/cy is last click place, mx/my is to.
 * ev_class = EVENT_DRAG:      cx/cy is last click place, mx/my is to,
 *                             code = mouse button
 * ev_class = EVENT_REPEAT:    code = button pressed
 *
 * @author Hj. Malthaner, Niels Roest
 */
struct event_t {
	unsigned int ev_class;
	unsigned int ev_code;
	int mx, my;

	/**
	 * position of last mouse click
	 */
	int cx, cy;

	/**
	 * current mouse button state
	 */
	int button_state;

	/**
	 * mod key (SHIFT; ALT; STRG; etc) pressed while event as triggered
	 * @author hsiegeln
	 */
	unsigned int ev_key_mod;
};

#ifdef __cplusplus
/**
 * Translate event origin. Useful when transferring events to sub-components.
 * @author Hj. Malthaner
 */
static inline void translate_event(struct event_t *ev, int x, int y)
{
	ev->mx += x;
	ev->cx += x;
	ev->my += y;
	ev->cy += y;
}
#endif

void display_poll_event(struct event_t *ev);
void display_get_event(struct event_t *ev);
void change_drag_start(int x, int y);

int event_get_last_control_shift(void);

#endif
