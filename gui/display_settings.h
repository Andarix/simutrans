/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_DISPLAY_SETTINGS_H
#define GUI_DISPLAY_SETTINGS_H


#include "gui_frame.h"
#include "components/gui_divider.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_numberinput.h"
#include "components/gui_combobox.h"
#include "components/gui_tab_panel.h"

/**
 * Menu with display settings
 */
class gui_settings_t : public gui_aligned_container_t
{
private:
	gui_label_buf_t
		frame_time_value_label,
		idle_time_value_label,
		fps_value_label,
		simloops_value_label;
public:
	gui_settings_t();
	virtual void draw( scr_coord offset ) OVERRIDE;
};

class map_settings_t : public gui_aligned_container_t, public action_listener_t
{
public:
	gui_numberinput_t
		inp_underground_level,
		brightness,
		scrollspeed;
	map_settings_t();
	virtual bool action_triggered( gui_action_creator_t *comp, value_t v ) OVERRIDE;
};

class station_settings_t : public gui_aligned_container_t
{
public:
	station_settings_t();
};

class transparency_settings_t : public gui_aligned_container_t, public action_listener_t
{
private:
	gui_numberinput_t cursor_hide_range;
	gui_combobox_t hide_buildings;
public:
	transparency_settings_t();
	virtual bool action_triggered( gui_action_creator_t *comp, value_t v );
};


class traffic_settings_t : public gui_aligned_container_t, public action_listener_t
{
private:
	gui_numberinput_t traffic_density;
	gui_combobox_t convoy_tooltip, money_booking, follow_mode;
public:
	traffic_settings_t();
	virtual bool action_triggered( gui_action_creator_t *comp, value_t v );
};

/**
 * Display settings dialog
 */
class color_gui_t : public gui_frame_t, private action_listener_t
{
private:
	gui_settings_t gui_settings;
	map_settings_t map_settings;
	transparency_settings_t transparency_settings;
	station_settings_t station_settings;
	traffic_settings_t traffic_settings;
	gui_scrollpane_t scrolly_gui, scrolly_map, scrolly_transparency, scrolly_station, scrolly_traffic;
	gui_tab_panel_t	tabs;

public:
	color_gui_t();

	virtual bool has_min_sizer() const OVERRIDE {return true;}

	/**
	 * Some windows have associated help text.
	 * @return The help file name or NULL
	 */
	const char * get_help_filename() const OVERRIDE { return "display.txt"; }

	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
