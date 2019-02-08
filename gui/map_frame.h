/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#ifndef gui_map_frame_h
#define gui_map_frame_h

#include "gui_frame.h"
#include "simwin.h"
#include "components/gui_scrollpane.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"
#include "../descriptor/factory_desc.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../player/simplay.h"
#include "../simline.h"

class karte_ptr_t;

#define MAP_MAX_BUTTONS (22)

/**
 * Minimap window
 *
 * @author Hj. Malthaner
 * @date 03-Mar-01
 */
class map_frame_t :
	public gui_frame_t,
	public action_listener_t
{
private:
	static karte_ptr_t welt;

	/**
	 * This is kind of hack: we know there can only be one map frame
	 * at a time, and we want to save the current size for the next object
	 * so we use a static variable here.
	 * @author Hj. Malthaner
	 */
	static scr_size window_size;
	static scr_coord screenpos;

	static bool legend_visible;
	static bool scale_visible;
	static bool directory_visible;
	static bool filter_factory_list;

	static bool is_cursor_hidden;

	/**
	 * We need to keep track of drag/click events
	 * @author Hj. Malthaner
	 */
	bool is_dragging;

	/**
	 * remember that we zoomed
	 * to center map
	 */
	bool zoomed;

	int viewable_players[MAX_PLAYER_COUNT+1];

	vector_tpl<const goods_desc_t *> viewable_freight_types;

	gui_aligned_container_t filter_container, scale_container, directory_container, *zoom_row;

	gui_scrollpane_t* p_scrolly;

	button_t	filter_buttons[MAP_MAX_BUTTONS],
				zoom_buttons[2],
				b_rotate45,
				b_show_legend,
				b_show_scale,
				b_show_directory,
				b_overlay_networks,
				b_overlay_networks_load_factor,
				b_filter_factory_list;

	gui_label_buf_t zoom_value_label;

	gui_combobox_t	viewed_player_c,
					transport_type_c,
					freight_type_c;

	void zoom(bool zoom_out);
	void update_buttons();
	void update_factory_legend();
	void show_hide_legend(const bool show);
	void show_hide_scale(const bool show);
	void show_hide_directory(const bool show);

public:

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_help_filename() const OVERRIDE {return "map.txt";}

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 * @author Hj. Malthaner
	 */
	bool has_min_sizer() const OVERRIDE {return true;}

	/**
	 * Constructor. Adds all necessary Subcomponents.
	 * @author Hj. Malthaner
	 */
	map_frame_t();

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_reliefmap; }

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	 * Sets the window sizes
	 * @author (Mathew Hounsell)
	 * @date   11-Mar-2003
	 */
	void set_windowsize(scr_size size) OVERRIDE;

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
