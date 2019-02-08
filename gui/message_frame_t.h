/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef message_frame_h
#define message_frame_h

#include "simwin.h"

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "components/gui_textinput.h"

#include "message_stats_t.h"
#include "components/action_listener.h"



/**
 * All messages since the start of the program
 * @author prissi
 */
class message_frame_t : public gui_frame_t, private action_listener_t
{
private:
	char ibuf[256];
	message_stats_t	stats;
	gui_scrollpane_t scrolly;
	gui_tab_panel_t tabs;		// Knightly : tab panel for filtering messages
	gui_textinput_t input;
	button_t option_bt, send_bt;
	vector_tpl<sint32> tab_categories;

public:
	message_frame_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_help_filename() const OVERRIDE {return "mailbox.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void rdwr(loadsave_t *) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_messageframe; }
};

#endif
