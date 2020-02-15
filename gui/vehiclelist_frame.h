/*
 * Factory list window
 * @author Hj. Malthaner
 */

#ifndef vehiclelist_frame_t_h
#define vehiclelist_frame_t_h

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_label.h"
#include "components/gui_image.h"
#include "components/gui_tab_panel.h"
#include "components/gui_combobox.h"

class vehicle_desc_t;


class vehiclelist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	button_t sortedby, sorteddir, bt_obsolete, bt_future;
	gui_scrolled_list_t scrolly;
	gui_tab_panel_t tabs;
	gui_combobox_t ware;

	void fill_list();

	waytype_t tabs_to_wt[ 9 ], current_wt;
	int max_idx; // may waytypes available

public:
	vehiclelist_frame_t();

	const char *get_help_filename() const OVERRIDE {return "depotlist.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	bool has_min_sizer() const { return true; }
};


class vehiclelist_stats_t : public gui_scrolled_list_t::scrollitem_t
{
private:
	const vehicle_desc_t *veh;
	cbuffer_t part1, part2;

public:
	static int sort_mode;
	static bool reverse;
	static int img_width;
	static int col1_width;
	static int col2_width;

	vehiclelist_stats_t(const vehicle_desc_t *);

	char const* get_text() const;
	scr_size get_size() const { return scr_size( D_MARGIN_LEFT+img_width+col1_width+col2_width+D_MARGIN_RIGHT, LINESPACE * 5 ); }
	scr_size get_min_size() const { return scr_size( D_MARGIN_LEFT+img_width+col1_width+col2_width+D_MARGIN_RIGHT, LINESPACE * 5 ); }
	scr_size get_max_size() const { return scr_size( D_MARGIN_LEFT+img_width+col1_width+col2_width+D_MARGIN_RIGHT, LINESPACE * 5 ); }

	static bool compare(const gui_component_t *a, const gui_component_t *b );

	void draw( scr_coord offset ) OVERRIDE;
};

#endif
