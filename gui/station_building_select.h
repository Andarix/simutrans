/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Building facing selector, when CTRL+Clicking over a building icon in menu
 */

#ifndef station_building_select_h
#define station_building_select_h

#include "components/action_listener.h"
#include "gui_frame.h"
#include "components/gui_button.h"


class building_desc_t;
class tool_build_station_t;


class station_building_select_t : public gui_frame_t, action_listener_t
{
	const building_desc_t *desc;

	button_t actionbutton[4];

	static tool_build_station_t tool;

public:
	station_building_select_t(const building_desc_t *desc);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
