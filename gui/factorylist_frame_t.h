/*
 * Factory list window
 * @author Hj. Malthaner
 */

#ifndef factorylist_frame_t_h
#define factorylist_frame_t_h

#include "gui_frame.h"
#include "components/gui_scrollpane.h"
#include "components/gui_label.h"
#include "factorylist_stats_t.h"



class factorylist_frame_t : public gui_frame_t, private action_listener_t
{
private:
	static const char *sort_text[factorylist::SORT_MODES];

	gui_label_t sort_label;
	button_t	sortedby;
	button_t	sorteddir;
	gui_scrolled_list_t scrolly;

public:
	factorylist_frame_t();

	/**
	 * resize window in response to a resize event
	 * @author Hj. Malthaner
	 */
	void resize(const scr_coord delta);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const {return "factorylist_filter.txt"; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	void draw(scr_coord pos, scr_size size);
};

#endif
