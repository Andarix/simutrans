/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Displays an information window for a convoi
 *
 * @author Hj. Malthaner
 * @date 22-Aug-01
 */

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_speedbar.h"
#include "components/gui_button.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_obj_view_t.h"
#include "components/action_listener.h"
#include "components/gui_tab_panel.h"
#include "components/gui_button_to_chart.h"
#include "../convoihandle_t.h"
#include "simwin.h"

#include "../utils/cbuffer_t.h"

class convoi_detail_t;

class convoi_info_t : public gui_frame_t, private action_listener_t
{
public:
	enum sort_mode_t { by_destination=0, by_via=1, by_amount_via=2, by_amount=3, SORT_MODES=4 };

private:

	/**
	* Buffer for freight info text string.
	* @author Hj. Malthaner
	*/
	cbuffer_t freight_info;

	gui_textarea_t text;
	obj_view_t view;
	gui_label_buf_t speed_label, profit_label, running_cost_label, weight_label, target_label, line_label;
	gui_textinput_t input;
	gui_speedbar_t filled_bar;
	gui_speedbar_t speed_bar;
	gui_speedbar_t route_bar;
	gui_chart_t chart;
	button_t button;
	button_t follow_button;
	button_t go_home_button;
	button_t no_load_button;

	gui_tab_panel_t switch_mode;
	gui_aligned_container_t container_freight, container_stats, container_line, *container_top, container_details;
	convoi_detail_t *details;
	gui_scrollpane_t scroll_freight;

	button_t sort_button;
	button_t line_button;	// goto line ...
	bool line_bound;

	convoihandle_t cnv;
	sint32 mean_convoi_speed;
	sint32 max_convoi_speed;

	// current pointer to route ...
	sint32 cnv_route_index;

	char cnv_name[256],old_cnv_name[256];

	void update_labels();

	// resets textinput to current convoi name
	// necessary after convoi was renamed
	void reset_cnv_name();

	// rename selected convoi
	// checks if possible / necessary
	void rename_cnv();

	static bool route_search_in_progress;
	static const char *sort_text[SORT_MODES];

	void show_hide_statistics( bool show );

	gui_button_to_chart_array_t button_to_chart;

	void init(convoihandle_t cnv);
public:
	convoi_info_t(convoihandle_t cnv = convoihandle_t());

	virtual ~convoi_info_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const OVERRIDE { return "convoiinfo.txt"; }

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool is_weltpos() OVERRIDE;

	koord3d get_weltpos( bool set ) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * called when convoi was renamed
	 */
	void update_data() { reset_cnv_name(); set_dirty(); }

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_convoi_info; }

	void route_search_finished() { route_search_in_progress = false; }
};
