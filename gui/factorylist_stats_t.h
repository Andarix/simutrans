/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Where factory stats are calculated for list dialog
 */

#ifndef factorylist_stats_t_h
#define factorylist_stats_t_h

#include "components/gui_colorbox.h"
#include "components/gui_image.h"
#include "components/gui_label.h"
#include "components/gui_scrolled_list.h"
#include "../simfab.h"

class fabrik_t;


namespace factorylist {
    enum sort_mode_t { by_name=0, by_available, by_output, by_maxprod, by_status, by_power, SORT_MODES, by_input, by_transit,  };	// the last two not used
};

/**
 * Factory list stats display
 * @author Hj. Malthaner
 */
class factorylist_stats_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	fabrik_t *fab;
	gui_colorbox_t indicator;
	gui_image_t boost_electric, boost_passenger, boost_mail;
	gui_label_buf_t label;

	void update_label();
public:
	static int sort_mode;
	static bool reverse;

	factorylist_stats_t(fabrik_t *);

	void draw( scr_coord pos) OVERRIDE;

	char const* get_text() const OVERRIDE { return fab->get_name(); }
	bool infowin_event(const event_t *) OVERRIDE;
	bool is_valid() const OVERRIDE;
	void set_size(scr_size size) OVERRIDE;

	static bool compare(const gui_component_t *a, const gui_component_t *b );
};


#endif
