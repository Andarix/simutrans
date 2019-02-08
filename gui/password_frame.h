/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_password_frame_h
#define gui_password_frame_h


#include "components/action_listener.h"
#include "gui_frame.h"
#include "components/gui_container.h"
#include "components/gui_textinput.h"
#include "components/gui_label.h"


class password_frame_t : public gui_frame_t, action_listener_t
{
private:
	char ibuf[256], player_name_str[256];

protected:
	player_t *player;

	gui_textinput_t player_name;
	gui_hidden_textinput_t password;
	gui_label_t fnlabel, const_player_name;

public:
	password_frame_t( player_t *player );

	const char * get_help_filename() const OVERRIDE {return "password.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
