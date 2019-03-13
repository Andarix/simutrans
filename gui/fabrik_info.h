/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Factory info dialog
 */

#ifndef fabrikinfo_t_h
#define fabrikinfo_t_h

#include "simwin.h"

#include "factory_chart.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/gui_obj_view_t.h"
#include "components/gui_container.h"
#include "components/gui_colorbox.h"
#include "components/gui_image.h"
#include "components/gui_tab_panel.h"
#include "../utils/cbuffer_t.h"

class welt_t;
class fabrik_t;
class gebaeude_t;
class button_t;




/**
 * Info window for factories
 * @author Hj. Malthaner
 */
class fabrik_info_t : public gui_frame_t, public action_listener_t
{
private:
	fabrik_t *fab;

	cbuffer_t info_buf, prod_buf, details_buf;

	factory_chart_t chart;

	obj_view_t view;

	char fabname[256];
	gui_textinput_t input;

	gui_textarea_t prod, txt;
	gui_colorbox_t indicator_color;

	gui_tab_panel_t switch_mode;

	gui_image_t boost_electric, boost_passenger, boost_mail;


	gui_aligned_container_t container_info, container_details;

	gui_aligned_container_t all_suppliers, all_consumers, all_stops, all_cities;
	uint32 old_suppliers_count, old_consumers_count, old_stops_count, old_cities_count;

	gui_scrollpane_t scroll_info;

	void rename_factory();

	void update_components();
public:
	// refreshes text, images, indicator
	void update_info();

	fabrik_info_t(fabrik_t* fab = NULL, const gebaeude_t* gb = NULL);

	virtual ~fabrik_info_t();

	void init(fabrik_t* fab, const gebaeude_t* gb);

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char *get_help_filename() const OVERRIDE {return "industry_info.txt";}

	bool has_min_sizer() const OVERRIDE {return true;}

	koord3d get_weltpos(bool) OVERRIDE { return fab->get_pos(); }

	bool is_weltpos() OVERRIDE;

	/**
	* Draw new component. The values to be passed refer to the window
	* i.e. It's the screen coordinates of the window where the
	* component is displayed.
	* @author Hj. Malthaner
	*/
	void draw(scr_coord pos, scr_size size) OVERRIDE;

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	// rotated map need new info ...
	void map_rotate90( sint16 ) OVERRIDE;

	void rdwr( loadsave_t *file ) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_factory_info; }
};

#endif
