/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * The window frame all dialogs are based
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <stdio.h>

#include "gui_frame.h"
#include "../simcolor.h"
#include "../dataobj/environment.h"
#include "../display/simgraph.h"
#include "simwin.h"
#include "../simworld.h"
#include "../player/simplay.h"

#include "../descriptor/reader/obj_reader.h"
#include "../descriptor/skin_desc.h"
#include "../simskin.h"

karte_ptr_t gui_frame_t::welt;

// Insert the container
gui_frame_t::gui_frame_t(char const* const name, player_t const* const player)
{
	this->name = name;
	size = scr_size(200, 100);
	min_windowsize = scr_size(0,0);
	owner = player;
	set_resizemode(no_resize);  //25-may-02  markus weber  added
	opaque = true;
	dirty = true;

	// set default margin and spacing
	gui_aligned_container_t::set_margin_from_theme();
	gui_aligned_container_t::set_spacing_from_theme();
	// initialize even if we cannot call has_title() here
	gui_aligned_container_t::set_pos(scr_coord(0, D_TITLEBAR_HEIGHT));
}


/**
 * Set the window size
 * @author Hj. Malthaner
 */
void gui_frame_t::set_windowsize(scr_size size)
{
	gui_aligned_container_t::set_pos(scr_coord(0, has_title()*D_TITLEBAR_HEIGHT));

	if(  size != this->size  ) {
		// mark old size dirty
		scr_coord const& pos = win_get_pos(this);
		mark_rect_dirty_wc( pos.x, pos.y, pos.x+this->size.w, pos.y+this->size.h );

		// minimum size //25-may-02  markus weber  added
		size.clip_lefttop(min_windowsize);

		this->size = size;
		dirty = true;

		// TODO respect gui_aligned_container_t::get_max_size()
	}
	// recompute always, to react on resize(scr_coord(0,0))
	if (gui_aligned_container_t::is_table()) {
		gui_aligned_container_t::set_size(get_client_windowsize());
	}
}


void gui_frame_t::reset_min_windowsize()
{
	gui_aligned_container_t::set_pos(scr_coord(0, has_title()*D_TITLEBAR_HEIGHT));

	if (gui_aligned_container_t::is_table()) {
		bool at_min_size = size == min_windowsize;
		scr_size csize = gui_aligned_container_t::get_min_size();
		scr_coord pos  = gui_aligned_container_t::get_pos();
		set_min_windowsize( scr_size(csize.w + pos.x, csize.h + pos.y) );

		if (at_min_size) {
			set_windowsize(min_windowsize);
		}
		else {
			scr_size wsize = size;
			wsize.clip_lefttop(min_windowsize);

			set_windowsize(wsize);
		}
	}
}


/**
 * get color information for the window title
 * -borders and -body background
 * @author Hj. Malthaner
 */
FLAGGED_PIXVAL gui_frame_t::get_titlecolor() const
{
	return owner ? PLAYER_FLAG|color_idx_to_rgb(owner->get_player_color1()+1) : env_t::default_window_title_color;
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_frame_t::infowin_event(const event_t *ev)
{
	// %DB0 printf( "\nMessage: gui_frame_t::infowin_event( event_t const * ev ) : Fenster|Window %p : Event is %d", (void*)this, ev->ev_class );
	if (ev->ev_code == SYSTEM_THEME_CHANGED) {
		if (gui_aligned_container_t::is_table()) {
			reset_min_windowsize();
		}
		return true;
	}

	if(IS_WINDOW_RESIZE(ev)) {
		scr_coord delta (  resize_mode & horizontal_resize ? ev->mx - ev->cx : 0,
		               resize_mode & vertical_resize  ? ev->my - ev->cy : 0);
		resize(delta);
		return true;  // don't pass to children!
	}
	else if(IS_WINDOW_MAKE_MIN_SIZE(ev)) {
		set_windowsize( get_min_windowsize() ) ;
		resize( scr_coord(0,0) ) ;
		return true;  // don't pass to children!
	}
	else if(ev->ev_class==INFOWIN  &&  (ev->ev_code==WIN_CLOSE  ||  ev->ev_code==WIN_OPEN  ||  ev->ev_code==WIN_TOP)) {
		dirty = true;
		gui_aligned_container_t::clear_dirty();
	}
	event_t ev2 = *ev;
	translate_event(&ev2, 0, -has_title()*D_TITLEBAR_HEIGHT);
	return gui_aligned_container_t::infowin_event(&ev2);
}



/**
 * resize window in response to a resize event
 * @author Markus Weber, Hj. Malthaner
 * @date 11-may-02
 */
void gui_frame_t::resize(const scr_coord delta)
{
	dirty = true;
	scr_size new_size = size + delta;

	// resize window to the minimum size
	new_size.clip_lefttop(min_windowsize);

	scr_coord size_change = new_size - size;

	// resize window
	set_windowsize(new_size);

	// change drag start
	change_drag_start(size_change.x, size_change.y);
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 *
 * @author Hj. Malthaner
 */
void gui_frame_t::draw(scr_coord pos, scr_size size)
{
	scr_size titlebar_size(0, ( has_title()*D_TITLEBAR_HEIGHT ));
	// draw background
	if(  opaque  ) {
		display_img_stretch( gui_theme_t::windowback, scr_rect( pos + titlebar_size, size - titlebar_size ) );
		if(  dirty  ) {
			mark_rect_dirty_wc(pos.x, pos.y, pos.x + size.w, pos.y + titlebar_size.h );
		}
	}
	else {
		if(  dirty  ) {
			mark_rect_dirty_wc(pos.x, pos.y, pos.x + size.w, pos.y + size.h + titlebar_size.h );
		}
		display_blend_wh_rgb( pos.x+1, pos.y+titlebar_size.h, size.w-2, size.h-titlebar_size.h, color_transparent, percent_transparent );
	}
	dirty = false;

	PUSH_CLIP_FIT(pos.x+1, pos.y+titlebar_size.h, size.w-2, size.h-titlebar_size.h);
	gui_aligned_container_t::draw(pos);
	POP_CLIP();

	// for shadows of the windows
	if(  gui_theme_t::gui_drop_shadows  ) {
		display_blend_wh_rgb( pos.x+size.w, pos.y+1, 2, size.h, color_idx_to_rgb(COL_BLACK), 50 );
		display_blend_wh_rgb( pos.x+1, pos.y+size.h, size.w, 2, color_idx_to_rgb(COL_BLACK), 50 );
	}
}
