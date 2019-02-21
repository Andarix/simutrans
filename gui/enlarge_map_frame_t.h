/*
 * Dialogue to increase map size.
 */

#ifndef bigger_map_gui_h
#define bigger_map_gui_h

#include "gui_frame.h"
#include "components/gui_label.h"
#include "components/action_listener.h"
#include "components/gui_numberinput.h"
#include "components/gui_map_preview.h"

class settings_t;

class enlarge_map_frame_t  : public gui_frame_t, private action_listener_t
{
private:
	// local settings of the new world ...
	settings_t* sets;

	/**
	* Mini Map-Preview
	* @author Hj. Malthaner
	*/
	array2d_tpl<PIXVAL> map;
	gui_map_preview_t
		map_preview;

	bool changed_number_of_towns;

	gui_numberinput_t
		inp_x_size,
		inp_y_size,
		inp_number_of_towns,
		inp_town_size;

	/* 
	 * Label to display current map seed number.
	 */
	gui_label_buf_t	map_number_label;

	button_t
		start_button;

	gui_label_buf_t
		size_label; // memory requirement


public:
	static inline koord koord_from_rotation(settings_t const*, sint16 y, sint16 x, sint16 w, sint16 h);

	enlarge_map_frame_t();
	~enlarge_map_frame_t();

	/**
	* Calculate the new Map-Preview. Initialize the new RNG!
	* public, because also the climate dialog need it
	* @author Hj. Malthaner
	*/
	void update_preview();

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_help_filename() const OVERRIDE { return "enlarge_map.txt";}

	/**
	 * Draw new component. The values to be passed refer to the window
	 * i.e. It's the screen coordinates of the window where the
	 * component is displayed.
	 * @author Hj. Malthaner
	 */
	void draw(scr_coord pos, scr_size size) OVERRIDE;
};

#endif
