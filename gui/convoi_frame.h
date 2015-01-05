/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Displays a scrollable list of all convois of a player
 *
 * @author Hj. Malthaner, Sort/Filtering by V. Meyer
 * @date 15-Jun-01
 */

#ifndef __convoi_frame_h
#define __convoi_frame_h

#include "convoi_filter_frame.h"
#include "gui_frame.h"
#include "components/gui_container.h"
#include "components/gui_scrollbar.h"
#include "components/gui_speedbar.h"
#include "components/gui_label.h"
#include "components/action_listener.h"  // 28-Dec-2001  Markus Weber    Added
#include "components/gui_button.h"
#include "../convoihandle_t.h"

class spieler_t;
class ware_besch_t;

class convoi_frame_t :
	public gui_frame_t,
	private action_listener_t  //28-Dec-01     Markus Weber    Added , private action_listener_t
{
public:
	enum sort_mode_t { nach_name=0, nach_gewinn=1, nach_typ=2, nach_id=3, SORT_MODES=4 };

private:
	spieler_t *owner;

	static const char *sort_text[SORT_MODES];

	/**
	* Handle the convoi to be displayed.
	* @author Hj. Malthaner
	*/
	vector_tpl<convoihandle_t> convois;
	uint32 last_world_convois;

	// since the scrollpane can be larger than 32767, we use explicitly a scroll bar
	scrollbar_t vscroll;

	// these are part of the top UI
	gui_label_t sort_label;
	button_t	sortedby;
	button_t	sorteddir;
	gui_label_t filter_label;
	button_t	filter_on;
	button_t	filter_details;

	// actual filter setting
	bool filter_is_on;
	const slist_tpl<const ware_besch_t *>*waren_filter;
	char *name_filter;
	uint32 filter_flags;

	bool get_filter(uint32 filter) { return (filter_flags & filter) != 0; }
	void set_filter(uint32 filter, bool on) { filter_flags = on ? (filter_flags | filter) : (filter_flags & ~filter); }

	/*
	 * All filter settings are static, so they are not reset each
	 * time the window closes.
	 */
	static sort_mode_t sortby;
	static bool sortreverse;

	static bool compare_convois(convoihandle_t, convoihandle_t);

	/**
	 * Check all filters for one convoi.
	 * returns true, if it is not filtered away.
	 * @author V. Meyer
	 */
	bool passes_filter(convoihandle_t cnv);

	void sort_list();

public:
	/**
	 * Resorts convois
	 */
	void sort_list( char *name, uint32 filter, const slist_tpl<const ware_besch_t *> *wares );

	convoi_frame_t(spieler_t *sp);

	~convoi_frame_t();

	/**
	 * Events werden hiermit an die GUI-Komponenten
	 * gemeldet
	 * @author V. Meyer
	 */
	bool infowin_event(const event_t *ev);

	/**
	 * This method is called if the size of the window should be changed
	 * @author Markus Weber
	 */
	void resize(const scr_coord size_change);                       // 28-Dec-01        Markus Weber Added

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author V. Meyer
	 */
	const char * get_help_filename() const {return "convoi.txt"; }

	static sort_mode_t get_sortierung() { return sortby; }
	static void set_sortierung(sort_mode_t sm) { sortby = sm; }

	static bool get_reverse() { return sortreverse; }
	static void set_reverse(bool reverse) { sortreverse = reverse; }

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
