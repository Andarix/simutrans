/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

/*
 * Sub-window for Sim
 * keine Klasse, da die funktionen von C-Code aus aufgerufen werden koennen
 *
 * The function implements a WindowManager 'Object'
 * There's only one WindowManager
 *
 * 17.11.97, Hj. Malthaner
 *
 * Window now typified
 * 21.06.98, Hj. Malthaner
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../simcolor.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../simmenu.h"
#include "../simskin.h"
#include "../simsys.h"
#include "../simticker.h"
#include "simwin.h"
#include "../simintr.h"
#include "../simhalt.h"
#include "../simworld.h"

#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/tabfile.h"

#include "../besch/skin_besch.h"

#include "../dings/zeiger.h"

#include "map_frame.h"
#include "help_frame.h"
#include "messagebox.h"
#include "gui_frame.h"

#include "../player/simplay.h"
#include "../tpl/inthashtable_tpl.h"
#include "../tpl/vector_tpl.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

// needed to restore/save them
#include "werkzeug_waehler.h"
#include "player_frame_t.h"
#include "money_frame.h"
#include "halt_detail.h"
#include "halt_info.h"
#include "convoi_detail_t.h"
#include "convoi_info_t.h"
#include "fahrplan_gui.h"
#include "line_management_gui.h"
#include "schedule_list.h"
#include "stadt_info.h"
#include "message_frame_t.h"
#include "message_option_t.h"
#include "fabrik_info.h"



class inthashtable_tpl<ptrdiff_t,koord> old_win_pos;


#define dragger_size 12

// (Mathew Hounsell)
// I added a button to the map window to fix it's size to the best one.
// This struct is the flow back to the object of the refactoring.
class simwin_gadget_flags_t
{
public:
	simwin_gadget_flags_t( void ) : title(true), close( false ), help( false ), prev( false ), size( false ), next( false ), sticky( false ), gotopos( false ) { }

	bool title:1;
	bool close:1;
	bool help:1;
	bool prev:1;
	bool size:1;
	bool next:1;
	bool sticky:1;
	bool gotopos:1;
};

class simwin_t
{
public:
	koord pos;              // Window position
	uint32 dauer;           // How long should the window stay open?
	uint8 wt;               // the flags for the window type
	ptrdiff_t magic_number; // either magic number or this pointer (which is unique too)
	gui_frame_t *gui;
	bool closing;
	bool sticky;            // true if window is sticky
	bool rollup;

	simwin_gadget_flags_t flags; // (Mathew Hounsell) See Above.

	simwin_t() : flags() {}

	bool operator== (const simwin_t &) const;
};

bool simwin_t::operator== (const simwin_t &other) const { return gui == other.gui; }


#define MAX_WIN (64)
static vector_tpl<simwin_t> wins(MAX_WIN);
static vector_tpl<simwin_t> kill_list(MAX_WIN);

static karte_t* wl = NULL; // Pointer to current world is set in win_set_world

static int top_win(int win, bool keep_state );
static void display_win(int win);


// Hajo: tooltip data
static int tooltip_xpos = 0;
static int tooltip_ypos = 0;
static const char * tooltip_text = 0;
static const char * static_tooltip_text = 0;
// Knightly :	For timed tooltip with initial delay and finite visible duration.
//				Valid owners are required for timing. Invalid (NULL) owners disable timing.
static const void * tooltip_owner = 0;	// owner of the registered tooltip
static const void * tooltip_group = 0;	// group to which the owner belongs
static unsigned long tooltip_register_time = 0;	// time at which a tooltip is initially registered

static bool show_ticker=0;

/* Hajo: if we are inside the event handler,
 * the window handler has gui pointer as value,
 * to defer destruction if this window
 */
static void *inside_event_handling = NULL;

// only this gui element can set a tooltip
static void *tooltip_element = NULL;

static void destroy_framed_win(simwin_t *win);

//=========================================================================
// Helper Functions

#define REVERSE_GADGETS (!umgebung_t::window_buttons_right)
// (Mathew Hounsell) A "Gadget Box" is a windows control button.
enum simwin_gadget_et { GADGET_CLOSE, GADGET_HELP, GADGET_SIZE, GADGET_PREV, GADGET_NEXT, GADGET_GOTOPOS=36, GADGET_STICKY=21, GADGET_STICKY_PUSHED, COUNT_GADGET=255 };


/**
 * Reads theme configuration data, still not final
 * searches first in user dir, pak dir, program dir
 * @author prissi
 *
 * Max Kielland:
 * Note, there will be a theme manager later on and
 * each gui object will find their own parameters by
 * themselves after registering its class to the theme
 * manager. This will be done as the last step in
 * the chain when loading a theme.
 */

#if THEME == -1
bool themes_init(const char *dir_name) { return false; }
#else
bool themes_init(const char *dir_name)
{
	tabfile_t themesconf;

	// first take user data, then user global data
	const std::string dir = dir_name;
#if THEME > 0
	if(  !themesconf.open((dir+"/themes_" STR(THEME) ".tab").c_str())  ) {
#else
	if(  !themesconf.open((dir+"/themes.tab").c_str())  ) {
#endif
		dbg->warning("simwin.cc themes_init()", "Can't read themes.tab from %s", dir_name );
		return false;
	}

	tabfileobj_t contents;
	themesconf.read(contents);

	// first the stuff for the dialogues
	gui_frame_t::gui_titlebar_height = (uint32)contents.get_int("gui_titlebar_height", gui_frame_t::gui_titlebar_height );
	gui_frame_t::gui_frame_left =      (uint32)contents.get_int("gui_frame_left",      gui_frame_t::gui_frame_left );
	gui_frame_t::gui_frame_top =       (uint32)contents.get_int("gui_frame_top",       gui_frame_t::gui_frame_top );
	gui_frame_t::gui_frame_right =     (uint32)contents.get_int("gui_frame_right",     gui_frame_t::gui_frame_right );
	gui_frame_t::gui_frame_bottom =    (uint32)contents.get_int("gui_frame_bottom",    gui_frame_t::gui_frame_bottom );
	gui_frame_t::gui_hspace =          (uint32)contents.get_int("gui_hspace",          gui_frame_t::gui_hspace );
	gui_frame_t::gui_vspace =          (uint32)contents.get_int("gui_vspace",          gui_frame_t::gui_vspace );

	// those two will be anyway set whenever the buttons are reinitialized
	// Max Kielland: This has been moved to button_t
	button_t::gui_button_size.x = (uint32)contents.get_int("gui_button_width",  button_t::gui_button_size.x );
	button_t::gui_button_size.y = (uint32)contents.get_int("gui_button_height", button_t::gui_button_size.y );

	button_t::button_color_text = (uint32)contents.get_int("button_color_text", button_t::button_color_text );
	button_t::button_color_disabled_text = (uint32)contents.get_int("button_color_disabled_text", button_t::button_color_disabled_text );

	// maybe not the best place, rather use simwin for the static defines?
	skinverwaltung_t::theme_color_text =          (COLOR_VAL)contents.get_int("gui_text_color",          SYSCOL_TEXT);
	skinverwaltung_t::theme_color_static_text =   (COLOR_VAL)contents.get_int("gui_static_text_color",   SYSCOL_STATIC_TEXT);
	skinverwaltung_t::theme_color_disabled_text = (COLOR_VAL)contents.get_int("gui_disabled_text_color", SYSCOL_DISABLED_TEXT);
	skinverwaltung_t::theme_color_highlight =     (COLOR_VAL)contents.get_int("gui_highlight_color",     SYSCOL_HIGHLIGHT);
	skinverwaltung_t::theme_color_shadow =        (COLOR_VAL)contents.get_int("gui_shadow_color",        SYSCOL_SHADOW);
	skinverwaltung_t::theme_color_face =          (COLOR_VAL)contents.get_int("gui_face_color",          SYSCOL_FACE);
	skinverwaltung_t::theme_color_button_text =   (COLOR_VAL)contents.get_int("gui_button_text_color",   SYSCOL_BUTTON_TEXT);

	// those two may be rather an own control later on?
	gui_frame_t::gui_indicator_width =  (uint32)contents.get_int("gui_indicator_width",  gui_frame_t::gui_indicator_width );
	gui_frame_t::gui_indicator_height = (uint32)contents.get_int("gui_indicator_height", gui_frame_t::gui_indicator_height );

	// other gui parameter
	// Max Kielland: Scrollbar size is set by the arrow size in button_t
	//scrollbar_t::BAR_SIZE = (uint32)contents.get_int("gui_scrollbar_width", scrollbar_t::BAR_SIZE );
	gui_tab_panel_t::header_vsize = (uint32)contents.get_int("gui_tab_header_vsize", gui_tab_panel_t::header_vsize );

	// stuff in umgebung_t but rather GUI	umgebung_t::window_snap_distance = contents.get_int("window_snap_distance", umgebung_t::window_snap_distance );
	umgebung_t::window_buttons_right =      contents.get_int("window_buttons_right",      umgebung_t::window_buttons_right );
	umgebung_t::left_to_right_graphs =      contents.get_int("left_to_right_graphs",      umgebung_t::left_to_right_graphs );
	umgebung_t::window_frame_active =       contents.get_int("window_frame_active",       umgebung_t::window_frame_active );
	umgebung_t::second_open_closes_win =    contents.get_int("second_open_closes_win",    umgebung_t::second_open_closes_win );
	umgebung_t::remember_window_positions = contents.get_int("remember_window_positions", umgebung_t::remember_window_positions );

	umgebung_t::front_window_bar_color =   contents.get_color("front_window_bar_color",   umgebung_t::front_window_bar_color );
	umgebung_t::front_window_text_color =  contents.get_color("front_window_text_color",  umgebung_t::front_window_text_color );
	umgebung_t::bottom_window_bar_color =  contents.get_color("bottom_window_bar_color",  umgebung_t::bottom_window_bar_color );
	umgebung_t::bottom_window_text_color = contents.get_color("bottom_window_text_color", umgebung_t::bottom_window_text_color );

	umgebung_t::show_tooltips =        contents.get_int("show_tooltips",              umgebung_t::show_tooltips );
	umgebung_t::tooltip_color =        contents.get_color("tooltip_background_color", umgebung_t::tooltip_color );
	umgebung_t::tooltip_textcolor =    contents.get_color("tooltip_text_color",       umgebung_t::tooltip_textcolor );
	umgebung_t::tooltip_delay =        contents.get_int("tooltip_delay",              umgebung_t::tooltip_delay );
	umgebung_t::tooltip_duration =     contents.get_int("tooltip_duration",           umgebung_t::tooltip_duration );
	umgebung_t::toolbar_max_width =    contents.get_int("toolbar_max_width",          umgebung_t::toolbar_max_width );
	umgebung_t::toolbar_max_height =   contents.get_int("toolbar_max_height",         umgebung_t::toolbar_max_height );
	umgebung_t::cursor_overlay_color = contents.get_color("cursor_overlay_color",     umgebung_t::cursor_overlay_color );

	// parsing buttons still needs to be done after agreement what to load
	return false; //hence we return false for now ...
}
#endif

/**
 * Display a window gadget
 * @author Hj. Malthaner
 */
static int display_gadget_box(simwin_gadget_et const  code,
			      int const x, int const y,
			      int const color,
			      bool const pushed)
{

	// If we have a skin, get gadget image data
	const bild_t *img = NULL;
	if(  skinverwaltung_t::window_skin  ) {
		// "x", "?", "=", "�", "�"
		const bild_besch_t *pic = skinverwaltung_t::window_skin->get_bild(code+1);
		if (  pic != NULL  ) {
			img = pic->get_pic();
		}
	}

	if(pushed) {
		display_fillbox_wh_clip(x+1, y+1, D_GADGET_SIZE-2, D_GADGET_SIZE-2, (color & 0xF8) + max(7, (color&0x07)+2), false);
	}

	// Do we have a gadget image?
	if(  img != NULL  ) {

		// Max Kielland: This center the gadget image and compensates for any left/top margins within the image to be backward compatible with older PAK sets.
		display_color_img(img->bild_nr, x + D_GET_CENTER_ALIGN_OFFSET(img->w,D_GADGET_SIZE)-img->x, y + D_GET_CENTER_ALIGN_OFFSET(img->h,D_GADGET_SIZE)-img->y, 0, false, false);

	} else {
		const char *gadget_text = "#";
		static const char *gadget_texts[5]={ "X", "?", "=", "<", ">" };

		if(  code <= GADGET_NEXT  ) {
			gadget_text = gadget_texts[code];
		}
		else if(  code == GADGET_GOTOPOS  ) {
			gadget_text	= "*";
		}
		else if(  code == GADGET_STICKY  ) {
			gadget_text	= "s";
		}
		else if(  code == GADGET_STICKY_PUSHED  ) {
			gadget_text	= "S";
		}
		display_proportional( x+4, y+4, gadget_text, ALIGN_LEFT, COL_BLACK, false );
	}

	display_vline_wh_clip(x,                 y,   D_TITLEBAR_HEIGHT,   color+1,   false);
	display_vline_wh_clip(x+D_GADGET_SIZE-1, y+1, D_TITLEBAR_HEIGHT-2, COL_BLACK, false);
	display_vline_wh_clip(x+D_GADGET_SIZE,   y+1, D_TITLEBAR_HEIGHT-2, color+1,   false);

	// Hajo: return width of gadget
	return D_GADGET_SIZE;
}


//-------------------------------------------------------------------------
// (Mathew Hounsell) Created
static int display_gadget_boxes(
	simwin_gadget_flags_t const * const flags,
	int const x, int const y,
	int const color,
	bool const close_pushed,
	bool const sticky_pushed,
	bool const goto_pushed
) {
	int width = 0;
	const int k=(REVERSE_GADGETS?1:-1);

	// Only the close and sticky gadget can be pushed.
	if(  flags->close  ) {
		width += k*display_gadget_box( GADGET_CLOSE, x + width, y, color, close_pushed );
	}
	if(  flags->size  ) {
		width += k*display_gadget_box( GADGET_SIZE, x + width, y, color, false );
	}
	if(  flags->help  ) {
		width += k*display_gadget_box( GADGET_HELP, x + width, y, color, false );
	}
	if(  flags->prev  ) {
		width += k*display_gadget_box( GADGET_PREV, x + width, y, color, false );
	}
	if(  flags->next  ) {
		width += k*display_gadget_box( GADGET_NEXT, x + width, y, color, false );
	}
	if(  flags->gotopos  ) {
		width += k*display_gadget_box( GADGET_GOTOPOS, x + width, y, color, goto_pushed );
	}
	if(  flags->sticky  ) {
		width += k*display_gadget_box( sticky_pushed ? GADGET_STICKY_PUSHED : GADGET_STICKY, x + width, y, color, sticky_pushed );
	}


	return abs( width );
}


static simwin_gadget_et decode_gadget_boxes(
               simwin_gadget_flags_t const * const flags,
               int const x,
               int const px
) {
	int offset = px-x;
	const int w=(REVERSE_GADGETS?-D_GADGET_SIZE:D_GADGET_SIZE);

//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","offset=%i, w=%i",offset, w );

	// Only the close gadget can be pushed.
	if( flags->close ) {
		if( offset >= 0  &&  offset<D_GADGET_SIZE  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","close" );
			return GADGET_CLOSE;
		}
		offset += w;
	}
	if( flags->size ) {
		if( offset >= 0  &&  offset<D_GADGET_SIZE  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","size" );
			return GADGET_SIZE;
		}
		offset += w;
	}
	if( flags->help ) {
		if( offset >= 0  &&  offset<D_GADGET_SIZE  ) {
//DBG_MESSAGE("simwin_gadget_et decode_gadget_boxes()","help" );
			return GADGET_HELP;
		}
		offset += w;
	}
	if( flags->prev ) {
		if( offset >= 0  &&  offset<D_GADGET_SIZE  ) {
			return GADGET_PREV;
		}
		offset += w;
	}
	if( flags->next ) {
		if( offset >= 0  &&  offset<D_GADGET_SIZE  ) {
			return GADGET_NEXT;
		}
		offset += w;
	}
	if( flags->gotopos ) {
		if( offset >= 0  &&  offset<D_GADGET_SIZE  ) {
			return GADGET_GOTOPOS;
		}
		offset += w;
	}
	if( flags->sticky ) {
		if( offset >= 0  &&  offset<D_GADGET_SIZE  ) {
			return GADGET_STICKY;
		}
		offset += w;
	}
	return COUNT_GADGET;
}

//-------------------------------------------------------------------------
// (Mathew Hounsell) Re-factored
static void win_draw_window_title(const koord pos, const koord gr,
		const PLAYER_COLOR_VAL titel_farbe,
		const char * const text,
		const PLAYER_COLOR_VAL text_farbe,
		const koord3d welt_pos,
		const bool closing,
		const bool sticky,
		const bool goto_pushed,
		simwin_gadget_flags_t &flags )
{
	PUSH_CLIP(pos.x, pos.y, gr.x, gr.y);
	display_fillbox_wh_clip(pos.x, pos.y, gr.x, 1, titel_farbe+1, false);
	display_fillbox_wh_clip(pos.x, pos.y+1, gr.x, D_TITLEBAR_HEIGHT-2, titel_farbe, false);
	display_fillbox_wh_clip(pos.x, pos.y+D_TITLEBAR_HEIGHT-1, gr.x, 1, COL_BLACK, false);
	display_vline_wh_clip(pos.x+gr.x-1, pos.y,   D_TITLEBAR_HEIGHT-1, COL_BLACK, false);

	// Draw the gadgets and then move left and draw text.
	flags.gotopos = (welt_pos != koord3d::invalid);
	int width = display_gadget_boxes( &flags, pos.x+(REVERSE_GADGETS?0:gr.x-D_GADGET_SIZE-4), pos.y, titel_farbe, closing, sticky, goto_pushed );
	int titlewidth = display_proportional_clip( pos.x + (REVERSE_GADGETS?width+4:4), pos.y+(D_TITLEBAR_HEIGHT-LINEASCENT)/2, text, ALIGN_LEFT, text_farbe, false );
	if(  flags.gotopos  ) {
		display_proportional_clip( pos.x + (REVERSE_GADGETS?width+4:4)+titlewidth+8, pos.y+(D_TITLEBAR_HEIGHT-LINEASCENT)/2, welt_pos.get_2d().get_fullstr(), ALIGN_LEFT, text_farbe, false );
	}
	POP_CLIP();
}


//-------------------------------------------------------------------------

/**
 * Draw dragger widget
 * @author Hj. Malthaner
 */
static void win_draw_window_dragger(koord pos, koord gr)
{
	pos += gr;
	if(  skinverwaltung_t::window_skin  &&  skinverwaltung_t::window_skin->get_bild_nr(36)!=IMG_LEER  ) {
		const bild_besch_t *dragger = skinverwaltung_t::window_skin->get_bild(36);
		display_color_img( dragger->get_nummer(), pos.x-dragger->get_pic()->w, pos.y-dragger->get_pic()->h, 0, false, false);
	}
	else {
		for(  int x=0;  x<dragger_size;  x++  ) {
			display_fillbox_wh( pos.x-x, pos.y-dragger_size+x, x, 1, (x & 1) ? COL_BLACK : MN_GREY4, true);
		}
	}
}


//=========================================================================



// returns the window (if open) otherwise zero
gui_frame_t *win_get_magic(ptrdiff_t magic)
{
	if(  magic!=-1  &&  magic!=0  ) {
		// there is at most one window with a positive magic number
		FOR( vector_tpl<simwin_t>, const& i, wins ) {
			if(  i.magic_number == magic  ) {
				// if 'special' magic number, return it
				return i.gui;
			}
		}
	}
	return NULL;
}



/**
 * Returns top window
 * @author prissi
 */
gui_frame_t *win_get_top()
{
	return wins.empty() ? 0 : wins.back().gui;
}


/**
 * returns the focused component of the top window
 * @author Knightly
 */
gui_komponente_t *win_get_focus()
{
	return wins.empty() ? 0 : wins.back().gui->get_focus();
}


int win_get_open_count()
{
	return wins.get_count();
}


// brings a window to front, if open
bool top_win( const gui_frame_t *gui, bool keep_rollup )
{
	for(  uint i=0;  i<wins.get_count()-1;  i++  ) {
		if(wins[i].gui==gui) {
			top_win(i,keep_rollup);
			return true;
		}
	}
	// not open
	return false;
}

/**
 * Checks if a window is a top level window
 * @author Hj. Malthaner
 */
bool win_is_top(const gui_frame_t *ig)
{
	return !wins.empty() && wins.back().gui == ig;
}


// window functions


// save/restore all dialogues
void rdwr_all_win(loadsave_t *file)
{
	if(  file->get_version()>102003  ) {
		if(  file->is_saving()  ) {
			FOR(vector_tpl<simwin_t>, & i, wins) {
				uint32 id = i.gui->get_rdwr_id();
				if(  id!=magic_reserved  ) {
					file->rdwr_long( id );
					i.pos.rdwr(file);
					file->rdwr_byte(i.wt);
					file->rdwr_bool(i.sticky);
					file->rdwr_bool(i.rollup);
					i.gui->rdwr(file);
				}
			}
			uint32 end = magic_none;
			file->rdwr_long( end );
		}
		else {
			// restore windows
			while(1) {
				uint32 id;
				file->rdwr_long(id);
				// create the matching
				gui_frame_t *w = NULL;
				switch(id) {

					// end of dialogues
					case magic_none: return;

					// actual dialogues to restore
					case magic_convoi_info:    w = new convoi_info_t(wl); break;
					case magic_convoi_detail:  w = new convoi_detail_t(wl); break;
					case magic_halt_info:      w = new halt_info_t(wl); break;
					case magic_halt_detail:    w = new halt_detail_t(wl); break;
					case magic_reliefmap:      w = new map_frame_t(wl); break;
					case magic_ki_kontroll_t:  w = new ki_kontroll_t(wl); break;
					case magic_schedule_rdwr_dummy: w = new fahrplan_gui_t(wl); break;
					case magic_line_schedule_rdwr_dummy: w = new line_management_gui_t(wl); break;
					case magic_city_info_t:    w = new stadt_info_t(wl); break;
					case magic_messageframe:   w = new message_frame_t(wl); break;
					case magic_message_options: w = new message_option_t(wl); break;
					case magic_factory_info:   w = new fabrik_info_t(wl); break;

					default:
						if(  id>=magic_finances_t  &&  id<magic_finances_t+MAX_PLAYER_COUNT  ) {
							w = new money_frame_t( wl->get_spieler(id-magic_finances_t) );
						}
						else if(  id>=magic_line_management_t  &&  id<magic_line_management_t+MAX_PLAYER_COUNT  ) {
							w = new schedule_list_gui_t( wl->get_spieler(id-magic_line_management_t) );
						}
						else if(  id>=magic_toolbar  &&  id<magic_toolbar+256  ) {
							werkzeug_t::toolbar_tool[id-magic_toolbar]->update(wl,wl->get_active_player());
							w = werkzeug_t::toolbar_tool[id-magic_toolbar]->get_werkzeug_waehler();
						}
						else {
							dbg->fatal( "rdwr_all_win()", "No idea how to restore magic $%Xlu", id );
						}
				}
				/* sequence is now the same for all dialogues
				 * restore coordinates
				 * create window
				 * read state
				 * restore content
				 * restore state - gui_frame_t::rdwr() might create its own window ->> want to restore state to that window
				 */
				koord p;
				p.rdwr(file);
				uint8 win_type;
				file->rdwr_byte( win_type );
				create_win( p.x, p.y, w, (wintype)win_type, id );
				bool sticky, rollup;
				file->rdwr_bool( sticky );
				file->rdwr_bool( rollup );
				// now load the window
				uint32 count = wins.get_count();
				w->rdwr( file );

				// restore sticky / rollup status
				// ensure that the new status is to currently loaded window
				if (wins.get_count() >= count) {
					wins.back().sticky = sticky;
					wins.back().rollup = rollup;
				}
			}
		}
	}
}



int create_win(gui_frame_t* const gui, wintype const wt, ptrdiff_t const magic)
{
	return create_win( -1, -1, gui, wt, magic);
}


int create_win(int x, int y, gui_frame_t* const gui, wintype const wt, ptrdiff_t const magic)
{
	assert(gui!=NULL  &&  magic!=0);

	if(  gui_frame_t *win = win_get_magic(magic)  ) {
		if(  umgebung_t::second_open_closes_win  ) {
			destroy_win( win );
			if(  !( wt & w_do_not_delete )  ) {
				delete gui;
			}
		}
		else {
			top_win( win );
		}
		return -1;
	}

	if(  x==-1  &&  y==-1  &&  umgebung_t::remember_window_positions  ) {
		// look for window in hash table
		if(  koord *k = old_win_pos.access(magic)  ) {
			x = k->x;
			y = k->y;
		}
	}

	/* if there are too many handles (likely in large games)
	 * we search for any error/news message at the bottom of the stack and delete it
	 * => newer information might be more important ...
	 */
	if(  wins.get_count()==MAX_WIN  ) {
		// we try to remove one of the lowest news windows (magic_none)
		for(  uint i=0;  i<MAX_WIN;  i++  ) {
			if(  wins[i].magic_number == magic_none  &&  dynamic_cast<news_window *>(wins[i].gui)!=NULL  ) {
				destroy_win( wins[i].gui );
				break;
			}
		}
	}

	if(  wins.get_count() < MAX_WIN  ) {

		if (!wins.empty()) {
			// mark old dirty
			const koord gr = wins.back().gui->get_fenstergroesse();
			mark_rect_dirty_wc( wins.back().pos.x - 1, wins.back().pos.y - 1, wins.back().pos.x + gr.x + 2, wins.back().pos.y + gr.y + 2 ); // -1, +2 for umgebung_t::window_frame_active
		}

		wins.append( simwin_t() );
		simwin_t& win = wins.back();

		sint16 const menu_height = umgebung_t::iconsize.y;

		// (Mathew Hounsell) Make Sure Closes Aren't Forgotten.
		// Must Reset as the entries and thus flags are reused
		win.flags.close = true;
		win.flags.title = gui->has_title();
		win.flags.help = ( gui->get_hilfe_datei() != NULL );
		win.flags.prev = gui->has_prev();
		win.flags.next = gui->has_next();
		win.flags.size = gui->has_min_sizer();
		win.flags.sticky = gui->has_sticky();
		win.gui = gui;

		// take care of time delete windows ...
		win.wt    = wt & w_time_delete ? w_info : wt;
		win.dauer = (wt&w_time_delete) ? MESG_WAIT : -1;
		win.magic_number = magic;
		win.closing = false;
		win.rollup = false;
		win.sticky = false;

		// Hajo: Notify window to be shown
		assert(gui);
		event_t ev;

		ev.ev_class = INFOWIN;
		ev.ev_code = WIN_OPEN;
		ev.mx = 0;
		ev.my = 0;
		ev.cx = 0;
		ev.cy = 0;
		ev.button_state = 0;

		void *old = inside_event_handling;
		inside_event_handling = gui;
		gui->infowin_event(&ev);
		inside_event_handling = old;

		koord gr = gui->get_fenstergroesse();

		if(x == -1) {
			// try to keep the toolbar below all other toolbars
			y = menu_height;
			if(wt & w_no_overlap) {
				for( uint32 i=0;  i<wins.get_count()-1;  i++  ) {
					if(wins[i].wt & w_no_overlap) {
						if(wins[i].pos.y>=y) {
							sint16 lower_y = wins[i].pos.y + wins[i].gui->get_fenstergroesse().y;
							if(lower_y >= y) {
								y = lower_y;
							}
						}
					}
				}
				// right aligned
//				x = max( 0, display_get_width()-gr.x );
				// but we go for left
				x = 0;
				y = min( y, display_get_height()-gr.y );
			}
			else {
				x = min(get_maus_x() - gr.x/2, display_get_width()-gr.x);
				y = min(get_maus_y() - gr.y-32, display_get_height()-gr.y);
			}
		}
		if(x<0) {
			x = 0;
		}
		if(y<menu_height) {
			y = menu_height;
		}
		win.pos = koord(x,y);
		mark_rect_dirty_wc( x, y, x+gr.x, y+gr.y );
		return wins.get_count();
	}
	else {
		return -1;
	}
}


/* sometimes a window cannot destroyed while it is still handled;
 * in those cases it will added to kill list and it is only destructed
 * by this function
 */
static void process_kill_list()
{
	FOR(vector_tpl<simwin_t>, & i, kill_list) {
		wins.remove(i);
		destroy_framed_win(&i);
	}
	kill_list.clear();
}


/**
 * Destroy a framed window
 * @author Hj. Malthaner
 */
static void destroy_framed_win(simwin_t *wins)
{
	// mark dirty
	const koord gr = wins->gui->get_fenstergroesse();
	mark_rect_dirty_wc( wins->pos.x - 1, wins->pos.y - 1, wins->pos.x + gr.x + 2, wins->pos.y + gr.y + 2 ); // -1, +2 for umgebung_t::window_frame_active

	if(wins->gui) {
		event_t ev;

		ev.ev_class = INFOWIN;
		ev.ev_code = WIN_CLOSE;
		ev.mx = 0;
		ev.my = 0;
		ev.cx = 0;
		ev.cy = 0;
		ev.button_state = 0;

		void *old = inside_event_handling;
		inside_event_handling = wins->gui;
		wins->gui->infowin_event(&ev);
		inside_event_handling = old;
	}

	if(  (wins->wt&w_do_not_delete)==0  ) {
		// remove from kill list first
		// otherwise delete will be called again on that window
		for(  uint j = 0;  j < kill_list.get_count();  j++  ) {
			if(  kill_list[j].gui == wins->gui  ) {
				kill_list.remove_at(j);
				break;
			}
		}
		delete wins->gui;
	}
	// set dirty flag to refill background
	if(wl) {
		wl->set_background_dirty();
	}
}



bool destroy_win(const ptrdiff_t magic)
{
	const gui_frame_t *gui = win_get_magic(magic);
	if(gui) {
		return destroy_win( gui );
	}
	return false;
}



bool destroy_win(const gui_frame_t *gui)
{
	if(wins.get_count() > 1  &&  wins.back().gui == gui) {
		// destroy topped window, top the next window before removing
		// do it here as top_win manipulates the win vector
		top_win(wins.get_count() - 2, true);
	}

	for(  uint i=0;  i<wins.get_count();  i++  ) {
		if(wins[i].gui == gui) {
			if(inside_event_handling==wins[i].gui) {
				kill_list.append_unique(wins[i]);
			}
			else {
				simwin_t win = wins[i];
				wins.remove_at(i);
				if(  win.magic_number < magic_max  ) {
					// save last pos
					if(  koord *k = old_win_pos.access(win.magic_number)  ) {
						*k = win.pos;
					}
					else {
						old_win_pos.put( win.magic_number, win.pos );
					}
				}
				destroy_framed_win(&win);
			}
			return true;
			break;
		}
	}
	return false;
}



void destroy_all_win(bool destroy_sticky)
{
	for ( int curWin=0 ; curWin < (int)wins.get_count() ; curWin++ ) {
		if(  destroy_sticky  || !wins[curWin].sticky  ) {
			if(  inside_event_handling==wins[curWin].gui  ) {
				// only add this, if not already added
				kill_list.append_unique(wins[curWin]);
			}
			else {
				destroy_framed_win(&wins[curWin]);
			}
			// compact the window list
			wins.remove_at(curWin);
			curWin--;
		}
	}
}


int top_win(int win, bool keep_state )
{
	if(  (uint32)win==wins.get_count()-1  ) {
		return win;
	} // already topped

	// mark old dirty
	koord gr = wins.back().gui->get_fenstergroesse();
	mark_rect_dirty_wc( wins.back().pos.x - 1, wins.back().pos.y - 1, wins.back().pos.x + gr.x + 2, wins.back().pos.y + gr.y + 2 ); // -1, +2 for umgebung_t::window_frame_active
//    mark_rect_dirty_wc( wins.back().pos.x, wins.back().pos.y, wins.back().pos.x+wins.back().gui->get_windowsize().x, wins.back().pos.y+D_TITLEBAR_HEIGHT );

	simwin_t tmp = wins[win];
	wins.remove_at(win);
	if(  !keep_state  ) {
		tmp.rollup = false;	// make visible when topping
	}
	wins.append(tmp);

	 // mark new dirty
	gr = wins.back().gui->get_fenstergroesse();
	mark_rect_dirty_wc( wins.back().pos.x - 1, wins.back().pos.y - 1, wins.back().pos.x + gr.x + 2, wins.back().pos.y + gr.y + 2 ); // -1, +2 for umgebung_t::window_frame_active

	event_t ev;

	ev.ev_class = INFOWIN;
	ev.ev_code = WIN_TOP;
	ev.mx = 0;
	ev.my = 0;
	ev.cx = 0;
	ev.cy = 0;
	ev.button_state = 0;

	void *old = inside_event_handling;
	inside_event_handling = tmp.gui;
	tmp.gui->infowin_event(&ev);
	inside_event_handling = old;

	return wins.get_count()-1;
}


void display_win(int win)
{
	// ok, now process it
	gui_frame_t *komp = wins[win].gui;
	koord gr = komp->get_fenstergroesse();
	koord pos = wins[win].pos;
	PLAYER_COLOR_VAL title_color = (komp->get_titelcolor()&0xF8)+umgebung_t::front_window_bar_color;
	PLAYER_COLOR_VAL text_color = +umgebung_t::front_window_text_color;
	if(  (unsigned)win!=wins.get_count()-1  ) {
		// not top => maximum brightness
		title_color = (title_color&0xF8)+umgebung_t::bottom_window_bar_color;
		text_color = umgebung_t::bottom_window_text_color;
	}
	bool need_dragger = komp->get_resizemode() != gui_frame_t::no_resize;

	// %HACK (Mathew Hounsell) So draw will know if gadget is needed.
	wins[win].flags.help = ( komp->get_hilfe_datei() != NULL );
	if(  wins[win].flags.title  ) {
		win_draw_window_title(wins[win].pos,
				gr,
				title_color,
				komp->get_name(),
				text_color,
				komp->get_weltpos(false),
				wins[win].closing,
				wins[win].sticky,
				komp->is_weltpos(),
				wins[win].flags );
		if(  wins[win].gui->is_dirty()  ) {
//			mark_rect_dirty_wc( wins[win].pos.x, wins[win].pos.y, wins[win].pos.x+gr.x, wins[win].pos.y+D_TITLEBAR_HEIGHT );
		}
	}
	// mark top window, if requested
	if(umgebung_t::window_frame_active  &&  (unsigned)win==wins.get_count()-1) {
		const int y_off = wins[win].flags.title ? 0 : D_TITLEBAR_HEIGHT;
		if(!wins[win].rollup) {
			display_ddd_box( wins[win].pos.x-1, wins[win].pos.y-1 + y_off, gr.x+2, gr.y+2 - y_off, title_color, title_color+1, wins[win].gui->is_dirty() );
		}
		else {
			display_ddd_box( wins[win].pos.x-1, wins[win].pos.y-1 + y_off, gr.x+2, D_TITLEBAR_HEIGHT + 2 - y_off, title_color, title_color+1, wins[win].gui->is_dirty() );
		}
	}
	if(!wins[win].rollup) {
		komp->zeichnen(wins[win].pos, gr);

		// draw dragger
		if(need_dragger) {
			win_draw_window_dragger( pos, gr);
		}
	}
}


void display_all_win()
{
	// first: empty kill list
	process_kill_list();

	// check which window can set tooltip
	const sint16 x = get_maus_x();
	const sint16 y = get_maus_y();
	tooltip_element = NULL;
	for(  uint32 i = wins.get_count(); i-- != 0;  ) {
		if(  (!wins[i].rollup  &&  wins[i].gui->getroffen(x-wins[i].pos.x,y-wins[i].pos.y))  ||
		     (wins[i].rollup  &&  x>=wins[i].pos.x  &&  x<wins[i].pos.x+wins[i].gui->get_fenstergroesse().x  &&  y>=wins[i].pos.y  &&  y<wins[i].pos.y+D_TITLEBAR_HEIGHT)
		) {
			// tooltips are only allowed for this window
			tooltip_element = wins[i].gui;
			break;
		}
	}

	// then display windows
	for(  uint i=0;  i<wins.get_count();  i++  ) {
		void *old_gui = inside_event_handling;
		inside_event_handling = wins[i].gui;
		display_win(i);
		inside_event_handling = old_gui;
	}
}


void win_rotate90( sint16 new_ysize )
{
	FOR(vector_tpl<simwin_t>, const& i, wins) {
		i.gui->map_rotate90(new_ysize);
	}
}


static void remove_old_win()
{
	// Destroy (close) old window when life time expire
	for(  int i=wins.get_count()-1;  i>=0;  i=min(i,(int)wins.get_count())-1  ) {
		if(wins[i].dauer > 0) {
			wins[i].dauer --;
			if(wins[i].dauer == 0) {
				destroy_win( wins[i].gui );
			}
		}
	}
}


static inline void snap_check_distance( sint16 *r, const sint16 a, const sint16 b )
{
	if(  abs(a-b)<=umgebung_t::window_snap_distance  ) {
		*r = a;
	}
}


void snap_check_win( const int win, koord *r, const koord from_pos, const koord from_gr, const koord to_pos, const koord to_gr )
{
	bool resize;
	if(  from_gr==to_gr  &&  from_pos!=to_pos  ) { // check if we're moving
		resize = false;
	}
	else if(  from_gr!=to_gr  &&  from_pos==from_pos  ) { // or resizing the window
		resize = true;
	}
	else {
		return; // or nothing to do.
	}

	const int wins_count = wins.get_count();

	for(  int i=0;  i<=wins_count;  i++  ) {
		if(  i==win  ) {
			// Don't snap to self
			continue;
		}

		koord other_gr, other_pos;
		if(  i==wins_count  ) {
			// Allow snap to screen edge
			other_pos.x = 0;
			other_pos.y = werkzeug_t::toolbar_tool[0]->iconsize.y;
			other_gr.x = display_get_width();
			other_gr.y = display_get_height()-16-other_pos.y; // 16 = bottom ticker height?
			if(  show_ticker  ) {
				other_gr.y -= 16;
			}
		}
		else {
			// Snap to other window
			other_gr = wins[i].gui->get_fenstergroesse();
			other_pos = wins[i].pos;
			if(  wins[i].rollup  ) {
				other_gr.y = D_TITLEBAR_HEIGHT;
			}
		}

		// my bottom below other top  and  my top above other bottom  ---- in same vertical band
		if(  from_pos.y+from_gr.y>=other_pos.y  &&  from_pos.y<=other_pos.y+other_gr.y  ) {
			if(  resize  ) {
				// other right side and my new right side within snap
				snap_check_distance( &r->x, other_pos.x+other_gr.x-from_pos.x, to_gr.x );  // snap right - align right sides
			}
			else {
				// other right side and my new right side within snap
				snap_check_distance( &r->x, other_pos.x+other_gr.x-from_gr.x, to_pos.x );  // snap right - align right sides

				// other left side and my new left side within snap
				snap_check_distance( &r->x, other_pos.x, to_pos.x );  // snap left - align left sides
			}
		}

		// my new bottom below other top  and  my new top above other bottom  ---- in same vertical band
		if(  resize  ) {
			if(  from_pos.y+to_gr.y>other_pos.y  &&  from_pos.y<other_pos.y+other_gr.y  ) {
				// other left side and my new right side within snap
				snap_check_distance( &r->x, other_pos.x-from_pos.x, to_gr.x );  // snap right - align my right to other left
			}
		}
		else {
			if(  to_pos.y+from_gr.y>other_pos.y  &&  to_pos.y<other_pos.y+other_gr.y  ) {
				// other left side and my new right side within snap
				snap_check_distance( &r->x, other_pos.x-from_gr.x, to_pos.x );  // snap right - align my right to other left

				// other right side and my new left within snap
				snap_check_distance( &r->x, other_pos.x+other_gr.x, to_pos.x );  // snap left - align my left to other right
			}
		}

		// my right side right of other left side  and  my left side left of other right side  ---- in same horizontal band
		if(  from_pos.x+from_gr.x>=other_pos.x  &&  from_pos.x<=other_pos.x+other_gr.x  ) {
			if(  resize  ) {
				// other bottom and my new bottom within snap
				snap_check_distance( &r->y, other_pos.y+other_gr.y-from_pos.y, to_gr.y );  // snap down - align bottoms
			}
			else {
				// other bottom and my new bottom within snap
				snap_check_distance( &r->y, other_pos.y+other_gr.y-from_gr.y, to_pos.y );  // snap down - align bottoms

				// other top and my new top within snap
				snap_check_distance( &r->y, other_pos.y, to_pos.y );  // snap up - align tops
			}
		}

		// my new right side right of other left side  and  my new left side left of other right side  ---- in same horizontal band
		if (  resize  ) {
			if(  from_pos.x+to_gr.x>other_pos.x  &&  from_pos.x<other_pos.x+other_gr.x  ) {
				// other top and my new bottom within snap
				snap_check_distance( &r->y, other_pos.y-from_pos.y, to_gr.y );  // snap down - align my bottom to other top
			}
		}
		else {
			if(  to_pos.x+from_gr.x>other_pos.x  &&  to_pos.x<other_pos.x+other_gr.x  ) {
				// other top and my new bottom within snap
				snap_check_distance( &r->y, other_pos.y-from_gr.y, to_pos.y );  // snap down - align my bottom to other top

				// other bottom and my new top within snap
				snap_check_distance( &r->y, other_pos.y+other_gr.y, to_pos.y );  // snap up - align my top to other bottom
			}
		}
	}
}


void move_win(int win, event_t *ev)
{
	const koord mouse_from( ev->cx, ev->cy );
	const koord mouse_to( ev->mx, ev->my );

	const koord from_pos = wins[win].pos;
	koord from_gr = wins[win].gui->get_fenstergroesse();
	if(  wins[win].rollup  ) {
		from_gr.y = D_TITLEBAR_HEIGHT + 2;
	}

	koord to_pos = wins[win].pos+(mouse_to-mouse_from);
	const koord to_gr = from_gr;

	if(  umgebung_t::window_snap_distance>0  ) {
		snap_check_win( win, &to_pos, from_pos, from_gr, to_pos, to_gr );
	}

	// CLIP(wert,min,max)
	to_pos.x = CLIP( to_pos.x, 8-to_gr.x, display_get_width()-16 );
	to_pos.y = CLIP( to_pos.y, werkzeug_t::toolbar_tool[0]->iconsize.y, display_get_height()-24 );

	// delta is actual window movement.
	const koord delta = to_pos - from_pos;

	wins[win].pos += delta;
	// need to mark all of old and new positions dirty. -1, +2 for umgebung_t::window_frame_active
	mark_rect_dirty_wc( from_pos.x - 1, from_pos.y - 1, from_pos.x + from_gr.x + 2, from_pos.y + from_gr.y + 2 );
	mark_rect_dirty_wc( to_pos.x - 1, to_pos.y - 1, to_pos.x + to_gr.x + 2, to_pos.y + to_gr.y + 2 );
	// set dirty flag to refill background
	if(wl) {
		wl->set_background_dirty();
	}

	change_drag_start( delta.x, delta.y );
}


void resize_win(int win, event_t *ev)
{
	event_t wev = *ev;
	wev.ev_class = WINDOW_RESIZE;
	wev.ev_code = 0;

	const koord mouse_from( wev.cx, wev.cy );
	const koord mouse_to( wev.mx, wev.my );

	const koord from_pos = wins[win].pos;
	const koord from_gr = wins[win].gui->get_fenstergroesse();

	const koord to_pos = from_pos;
	koord to_gr = from_gr+(mouse_to-mouse_from);

	if(  umgebung_t::window_snap_distance>0  ) {
		snap_check_win( win, &to_gr, from_pos, from_gr, to_pos, to_gr );
	}

	// since we may be smaller afterwards
	mark_rect_dirty_wc( from_pos.x - 1, from_pos.y - 1, from_pos.x + from_gr.x + 2, from_pos.y + from_gr.y + 2 ); // -1, +2 for umgebung_t::window_frame_active
	// set dirty flag to refill background
	if(wl) {
		wl->set_background_dirty();
	}

	// adjust event mouse koord per snap
	wev.mx = wev.cx + to_gr.x - from_gr.x;
	wev.my = wev.cy + to_gr.y - from_gr.y;

	wins[win].gui->infowin_event( &wev );
}


// returns true, if gui is a open window handle
bool win_is_open(gui_frame_t *gui)
{
	FOR(vector_tpl<simwin_t>, const& i, wins) {
		if (i.gui == gui) {
			FOR(vector_tpl<simwin_t>, const& j, kill_list) {
				if (j.gui == gui) {
					return false;
				}
			}
			return true;
		}
	}
	return false;
}


koord const& win_get_pos(gui_frame_t const* const gui)
{
	for(  uint32 i = wins.get_count(); i-- != 0;  ) {
		if(  wins[i].gui == gui  ) {
			return wins[i].pos;
		}
	}
	static koord const bad(-1, -1);
	return bad;
}


void win_set_pos(gui_frame_t *gui, int x, int y)
{
	for(  uint32 i = wins.get_count(); i-- != 0;  ) {
		if(  wins[i].gui == gui  ) {
			wins[i].pos.x = x;
			wins[i].pos.y = y;
			const koord gr = wins[i].gui->get_fenstergroesse();
			mark_rect_dirty_wc( x - 1, y - 1, x + gr.x + 2, y + gr.y + 2 ); // -1, +2 for umgebung_t::window_frame_active
			return;
		}
	}
}


/* main window event handler
 * renovated may 2005 by prissi to take care of irregularly shaped windows
 * also remove some unnecessary calls
 */
bool check_pos_win(event_t *ev)
{
	static int is_resizing = -1;
	static int is_moving = -1;

	bool swallowed = false;

	const int x = ev->ev_class==EVENT_MOVE ? ev->mx : ev->cx;
	const int y = ev->ev_class==EVENT_MOVE ? ev->my : ev->cy;


	// for the moment, no none events
	if (ev->ev_class == EVENT_NONE) {
		process_kill_list();
		return false;
	}

	// we stop resizing once the user releases the button
	if(  (is_resizing>=0  ||  is_moving>=0)  &&  (IS_LEFTRELEASE(ev)  ||  (ev->button_state&1)==0)  ) {
		is_resizing = -1;
		is_moving = -1;
		if(  IS_LEFTRELEASE(ev)  ) {
			// Knightly :	should not proceed, otherwise the left release event will be fed to other components;
			//				return true (i.e. event swallowed) to prevent propagation back to the main view
			return true;
		}
	}

	// Knightly : disable any active tooltip upon mouse click by forcing expiration of tooltip duration
	if(  ev->ev_class==EVENT_CLICK  ) {
		tooltip_register_time = 0;
	}

	// click in main menu?
	if (!werkzeug_t::toolbar_tool.empty()                   &&
			werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler() &&
			werkzeug_t::toolbar_tool[0]->iconsize.y > y         &&
			ev->ev_class != EVENT_KEYBOARD) {
		event_t wev = *ev;
		inside_event_handling = werkzeug_t::toolbar_tool[0];
		werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler()->infowin_event( &wev );
		inside_event_handling = NULL;
		// swallow event
		return true;
	}

	// cursor event only go to top window (but not if rolled up)
	if(  ev->ev_class == EVENT_KEYBOARD  &&  !wins.empty()  ) {
		simwin_t &win  = wins.back();
		if(  !win.rollup  )  {
			inside_event_handling = win.gui;
			swallowed = win.gui->infowin_event(ev);
		}
		inside_event_handling = NULL;
		process_kill_list();
		return swallowed;
	}

	// just move top window until button release
	if(  is_moving>=0  &&  (unsigned)is_moving<wins.get_count()  &&  (IS_LEFTDRAG(ev)  ||  IS_LEFTREPEAT(ev))  ) {
		move_win( is_moving, ev );
		return true;
	}

	// just resize window until button release
	if(  is_resizing>=0  &&  (unsigned)is_resizing<wins.get_count()  &&  (IS_LEFTDRAG(ev)  ||  IS_LEFTREPEAT(ev))  ) {
		resize_win( is_resizing, ev );
		return true;
	}

	// swallow all other events in the infobar
	if(  ev->ev_class != EVENT_KEYBOARD  &&  y > display_get_height()-16  ) {
		// swallow event
		return true;
	}

	// swallow all other events in ticker (if there)
	if(  ev->ev_class != EVENT_KEYBOARD  &&  show_ticker  &&  y > display_get_height()-32  ) {
		if(  IS_LEFTCLICK(ev)  ) {
			// goto infowin koordinate, if ticker is active
			koord p = ticker::get_welt_pos();
			if(wl->is_within_limits(p)) {
				wl->change_world_position(koord3d(p,wl->min_hgt(p)));
			}
		}
		// swallow event
		return true;
	}

	// handle all the other events
	for(  int i=wins.get_count()-1;  i>=0  &&  !swallowed;  i=min(i,(int)wins.get_count())-1  ) {

		if(  wins[i].gui->getroffen( x-wins[i].pos.x, y-wins[i].pos.y )  ) {

			// all events in window are swallowed
			swallowed = true;

			inside_event_handling = wins[i].gui;

			// Top window first
			if(  (int)wins.get_count()-1>i  &&  IS_LEFTCLICK(ev)  &&  (!wins[i].rollup  ||  ev->cy<wins[i].pos.y+D_TITLEBAR_HEIGHT)  ) {
				i = top_win(i,false);
			}

			// Hajo: if within title bar && window needs decoration
			// Max Kielland: Use title height
			if(  y<wins[i].pos.y+D_TITLEBAR_HEIGHT  &&  wins[i].flags.title  ) {
				// no more moving
				is_moving = -1;

				// %HACK (Mathew Hounsell) So decode will know if gadget is needed.
				wins[i].flags.help = ( wins[i].gui->get_hilfe_datei() != NULL );

				// Where Was It ?
				simwin_gadget_et code = decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_fenstergroesse().x-D_GADGET_SIZE-4), x );

				switch( code ) {
					case GADGET_CLOSE :
						if (IS_LEFTCLICK(ev)) {
							wins[i].closing = true;
						}
						else if  (IS_LEFTRELEASE(ev)) {
							if (  ev->my>=wins[i].pos.y  &&  ev->my<wins[i].pos.y+D_GADGET_SIZE  &&  decode_gadget_boxes( ( & wins[i].flags ), wins[i].pos.x + (REVERSE_GADGETS?0:wins[i].gui->get_fenstergroesse().x-D_GADGET_SIZE-4), ev->mx )==GADGET_CLOSE) {
								destroy_win(wins[i].gui);
							} else {
								wins[i].closing = false;
							}
						}
						break;
					case GADGET_SIZE: // (Mathew Hounsell)
						if (IS_LEFTCLICK(ev)) {
							ev->ev_class = WINDOW_MAKE_MIN_SIZE;
							ev->ev_code = 0;
							wins[i].gui->infowin_event( ev );
						}
						break;
					case GADGET_HELP :
						if (IS_LEFTCLICK(ev)) {
							help_frame_t::open_help_on( wins[i].gui->get_hilfe_datei() );
						}
						break;
					case GADGET_PREV:
						if (IS_LEFTCLICK(ev)) {
							ev->ev_class = WINDOW_CHOOSE_NEXT;
							ev->ev_code = PREV_WINDOW;  // backward
							wins[i].gui->infowin_event( ev );
						}
						break;
					case GADGET_NEXT:
						if (IS_LEFTCLICK(ev)) {
							ev->ev_class = WINDOW_CHOOSE_NEXT;
							ev->ev_code = NEXT_WINDOW;  // forward
							wins[i].gui->infowin_event( ev );
						}
						break;
					case GADGET_GOTOPOS:
						if (IS_LEFTCLICK(ev)) {
							// change position on map (or follow)
							koord3d k = wins[i].gui->get_weltpos(true);
							if(  k!=koord3d::invalid  ) {
								spieler_t::get_welt()->change_world_position( k );
							}
						}
						break;
					case GADGET_STICKY:
						if (IS_LEFTCLICK(ev)) {
							wins[i].sticky = !wins[i].sticky;
							// mark title bar dirty
							mark_rect_dirty_wc( wins[i].pos.x, wins[i].pos.y, wins[i].pos.x + wins[i].gui->get_fenstergroesse().x, wins[i].pos.y + D_TITLEBAR_HEIGHT );
						}
						break;
					default : // Title
						if (IS_LEFTDRAG(ev)) {
							i = top_win(i,false);
							move_win(i, ev);
							is_moving = i;
						}
						if(IS_RIGHTCLICK(ev)) {
							wins[i].rollup ^= 1;
							gui_frame_t *gui = wins[i].gui;
							koord gr = gui->get_fenstergroesse();
							mark_rect_dirty_wc( wins[i].pos.x, wins[i].pos.y, wins[i].pos.x+gr.x, wins[i].pos.y+gr.y );
							if(  wins[i].rollup  ) {
								wl->set_background_dirty();
							}
						}

				}

				// It has been handled so stop checking.
				break;

			}
			else {
				if(!wins[i].rollup) {
					// click in Window / Resize?
					//11-May-02   markus weber added

					koord gr = wins[i].gui->get_fenstergroesse();

					// resizer hit ?
					const bool canresize = is_resizing>=0  ||
												(ev->cx > wins[i].pos.x + gr.x - dragger_size  &&
												 ev->cy > wins[i].pos.y + gr.y - dragger_size);

					if((IS_LEFTCLICK(ev)  ||  IS_LEFTDRAG(ev)  ||  IS_LEFTREPEAT(ev))  &&  canresize  &&  wins[i].gui->get_resizemode()!=gui_frame_t::no_resize) {
						resize_win( i, ev );
						is_resizing = i;
					}
					else {
						is_resizing = -1;
						// click in Window
						event_t wev = *ev;
						translate_event(&wev, -wins[i].pos.x, -wins[i].pos.y);
						wins[i].gui->infowin_event( &wev );
					}
				}
				else {
					swallowed = false;
				}
			}
			inside_event_handling = NULL;
		}
	}

	inside_event_handling = NULL;
	process_kill_list();

	return swallowed;
}


void win_get_event(event_t* const ev)
{
	display_get_event(ev);
}


void win_poll_event(event_t* const ev)
{
	display_poll_event(ev);
	// main window resized
	if(ev->ev_class==EVENT_SYSTEM  &&  ev->ev_code==SYSTEM_RESIZE) {
		// main window resized
		simgraph_resize( ev->mx, ev->my );
		ticker::redraw_ticker();
		wl->set_dirty();
		ev->ev_class = EVENT_NONE;
	}
}


// finally updates the display
void win_display_flush(double konto)
{
	const sint16 disp_width = display_get_width();
	const sint16 disp_height = display_get_height();
	const sint16 menu_height = werkzeug_t::toolbar_tool[0]->iconsize.y;

	// display main menu
	werkzeug_waehler_t *main_menu = werkzeug_t::toolbar_tool[0]->get_werkzeug_waehler();
	display_set_clip_wh( 0, 0, disp_width, menu_height+1 );
	display_fillbox_wh( 0, 0, disp_width, menu_height, MN_GREY2, false );
	// .. extra logic to enable tooltips
	tooltip_element = menu_height > get_maus_y() ? main_menu : NULL;
	void *old_inside_event_handling = inside_event_handling;
	inside_event_handling = main_menu;
	main_menu->zeichnen( koord(0,-D_TITLEBAR_HEIGHT), koord(disp_width,menu_height) );
	inside_event_handling = old_inside_event_handling;

	display_set_clip_wh( 0, menu_height, disp_width, disp_height-menu_height+1 );

	show_ticker = false;
	if (!ticker::empty()) {
		ticker::zeichnen();
		if (ticker::empty()) {
			// set dirty background for removing ticker
			if(wl) {
				wl->set_background_dirty();
			}
		}
		else {
			show_ticker = true;
			// need to adapt tooltip_y coordinates
			tooltip_ypos = min(tooltip_ypos, disp_height-15-10-16);
		}
	}

	// ok, we want to clip the height for everything!
	// unfortunately, the easiest way is by manipulating the global high
	{
		sint16 oldh = display_get_height();
		display_set_height( oldh-(wl?16:0)-16*show_ticker );

		display_all_win();
		remove_old_win();

		if(umgebung_t::show_tooltips) {
			// Hajo: check if there is a tooltip to display
			if(  tooltip_text  &&  *tooltip_text  ) {
				// Knightly : display tooltip when current owner is invalid or when it is within visible duration
				unsigned long elapsed_time;
				if(  !tooltip_owner  ||  ((elapsed_time=dr_time()-tooltip_register_time)>umgebung_t::tooltip_delay  &&  elapsed_time<=umgebung_t::tooltip_delay+umgebung_t::tooltip_duration)  ) {
					const sint16 width = proportional_string_width(tooltip_text)+7;
					display_ddd_proportional_clip(min(tooltip_xpos,disp_width-width), max(menu_height+7,tooltip_ypos), width, 0, umgebung_t::tooltip_color, umgebung_t::tooltip_textcolor, tooltip_text, true);
					if(wl) {
						wl->set_background_dirty();
					}
				}
			}
			else if(static_tooltip_text!=NULL  &&  *static_tooltip_text) {
				const sint16 width = proportional_string_width(static_tooltip_text)+7;
				display_ddd_proportional_clip(min(get_maus_x()+16,disp_width-width), max(menu_height+7,get_maus_y()-16), width, 0, umgebung_t::tooltip_color, umgebung_t::tooltip_textcolor, static_tooltip_text, true);
				if(wl) {
					wl->set_background_dirty();
				}
			}
			// Knightly : reset owner and group if no tooltip has been registered
			if(  !tooltip_text  ) {
				tooltip_owner = 0;
				tooltip_group = 0;
			}
			// Hajo : clear tooltip text to avoid sticky tooltips
			tooltip_text = 0;
		}

		display_set_height( oldh );

		if(!wl) {
			// no infos during loading etc
			return;
		}
	}

	char const *time = tick_to_string( wl->get_zeit_ms(), true );

	// bottom text background
	display_set_clip_wh( 0, 0, disp_width, disp_height );
	display_fillbox_wh(0, disp_height-16, disp_width, 1, MN_GREY4, false);
	display_fillbox_wh(0, disp_height-15, disp_width, 15, MN_GREY1, false);

	bool tooltip_check = get_maus_y()>disp_height-15;
	if(  tooltip_check  ) {
		tooltip_xpos = get_maus_x();
		tooltip_ypos = disp_height-15-10-16*show_ticker;
	}

	// season color
	display_color_img( skinverwaltung_t::seasons_icons->get_bild_nr(wl->get_season()), 2, disp_height-15, 0, false, true );
	if(  tooltip_check  &&  tooltip_xpos<14  ) {
		static char const* const seasons[] = { "q2", "q3", "q4", "q1" };
		tooltip_text = translator::translate(seasons[wl->get_season()]);
		tooltip_check = false;
	}

	KOORD_VAL right_border = disp_width-4;

	// shown if timeline game
	if(  wl->use_timeline()  &&  skinverwaltung_t::timelinesymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::timelinesymbol->get_bild_nr(0), right_border, disp_height-15, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("timeline");
			tooltip_check = false;
		}
	}

	// shown if connected
	if(  umgebung_t::networkmode  &&  skinverwaltung_t::networksymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::networksymbol->get_bild_nr(0), right_border, disp_height-15, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("Connected with server");
			tooltip_check = false;
		}
	}

	// put pause icon
	if(  wl->is_paused()  &&  skinverwaltung_t::pausesymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::pausesymbol->get_bild_nr(0), right_border, disp_height-15, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("GAME PAUSED");
			tooltip_check = false;
		}
	}

	// put fast forward icon
	if(  wl->is_fast_forward()  &&  skinverwaltung_t::fastforwardsymbol  ) {
		right_border -= 14;
		display_color_img( skinverwaltung_t::fastforwardsymbol->get_bild_nr(0), right_border, disp_height-15, 0, false, true );
		if(  tooltip_check  &&  tooltip_xpos>=right_border  ) {
			tooltip_text = translator::translate("Fast forward");
			tooltip_check = false;
		}
	}

	koord3d pos = wl->get_zeiger()->get_pos();

	static cbuffer_t info;
	info.clear();
	if(  pos!=koord3d::invalid  ) {
		info.printf( "(%s)", pos.get_str() );
	}
	if(  skinverwaltung_t::timelinesymbol==NULL  ) {
		info.printf( " %s", translator::translate(wl->use_timeline()?"timeline":"no timeline") );
	}
	if(wl->show_distance!=koord3d::invalid  &&  wl->show_distance!=pos) {
		info.printf("-(%d,%d)", wl->show_distance.x-pos.x, wl->show_distance.y-pos.y );
	}
	if(  !umgebung_t::networkmode  ) {
		// time multiplier text
		if(wl->is_fast_forward()) {
			info.printf(" %s(T~%1.2f)", skinverwaltung_t::fastforwardsymbol?"":">> ", wl->get_simloops()/50.0 );
		}
		else if(!wl->is_paused()) {
			info.printf(" (T=%1.2f)", wl->get_time_multiplier()/16.0 );
		}
		else if(  skinverwaltung_t::pausesymbol==NULL  ) {
			info.printf( " %s", translator::translate("GAME PAUSED") );
		}
	}
#ifdef DEBUG
	if(  umgebung_t::verbose_debug>3  ) {
		if(  haltestelle_t::get_rerouting_status()==RECONNECTING  ) {
			info.append( " +" );
		}
		else if(  haltestelle_t::get_rerouting_status()==REROUTING  ) {
			info.append( " *" );
		}
	}
#endif

	KOORD_VAL w_left = 20+display_proportional(20, disp_height-12, time, ALIGN_LEFT, COL_BLACK, true);
	KOORD_VAL w_right  = display_proportional(right_border-4, disp_height-12, info, ALIGN_RIGHT, COL_BLACK, true);
	KOORD_VAL middle = (disp_width+((w_left+8)&0xFFF0)-((w_right+8)&0xFFF0))/2;

	if(wl->get_active_player()) {
		char buffer[256];
		display_proportional( middle-5, disp_height-12, wl->get_active_player()->get_name(), ALIGN_RIGHT, PLAYER_FLAG|(wl->get_active_player()->get_player_color1()+0), true);
		money_to_string(buffer, konto);
		display_proportional( middle+5, disp_height-12, buffer, ALIGN_LEFT, konto >= 0.0?MONEY_PLUS:MONEY_MINUS, true);
	}
}


void win_set_world(karte_t *world)
{
	wl = world;
	// remove all save window positions
	old_win_pos.clear();
}


void win_redraw_world()
{
	if(wl) {
		wl->set_dirty();
	}
}


bool win_change_zoom_factor(bool magnify)
{
	return magnify ? zoom_factor_up() : zoom_factor_down();
}


/**
 * Sets the tooltip to display.
 * Has to be called from within gui_frame_t::zeichnen
 * @param owner : owner==NULL disables timing (initial delay and visible duration)
 * @author Hj. Malthaner, Knightly
 */
void win_set_tooltip(int xpos, int ypos, const char *text, const void *const owner, const void *const group)
{
	// check whether the right window will set the tooltip
	if (inside_event_handling != tooltip_element) {
		return;
	}
	// must be set every time as win_display_flush() will reset them
	tooltip_text = text;

	// update ownership if changed
	if(  owner!=tooltip_owner  ) {
		tooltip_owner = owner;
		// update register time only if owner is valid
		if(  owner  ) {
			const unsigned long current_time = dr_time();
			if(  group  &&  group==tooltip_group  ) {
				// case : same group
				const unsigned long elapsed_time = current_time - tooltip_register_time;
				const unsigned long threshold = umgebung_t::tooltip_delay - (umgebung_t::tooltip_delay>>2);	// 3/4 of delay
				if(  elapsed_time>threshold  &&  elapsed_time<=umgebung_t::tooltip_delay+umgebung_t::tooltip_duration  ) {
					// case : threshold was reached and duration not expired -> delay time is reduced to 1/4
					tooltip_register_time = current_time - threshold;
				}
				else {
					// case : either before threshold or duration expired
					tooltip_register_time = current_time;
				}
			}
			else {
				// case : owner has no associated group or group is different -> simply reset to current time
				tooltip_group = group;
				tooltip_register_time = current_time;
			}
		}
		else {
			// no owner to associate with a group even if the group is valid
			tooltip_group = 0;
		}
	}

	tooltip_xpos = xpos;
	tooltip_ypos = ypos;
}



/**
 * Sets the tooltip to display.
 * @author Hj. Malthaner
 */
void win_set_static_tooltip(const char *text)
{
	static_tooltip_text = text;
}
