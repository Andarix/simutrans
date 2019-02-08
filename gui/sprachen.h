/*
 * Dialog for language change
 * @author Hj. Maltahner, Niels Roest, prissi
 */

#ifndef gui_sprachen_h
#define gui_sprachen_h

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer_t.h"

#include "../tpl/vector_tpl.h"

class sprachengui_t : public gui_frame_t, private action_listener_t
{
private:
	cbuffer_t buf;
	gui_textarea_t text_label;

	struct language_button_t {
		button_t* button;
		int id;
	};
	vector_tpl<language_button_t> buttons;

	static int cmp_language_button(sprachengui_t::language_button_t a, sprachengui_t::language_button_t b);

public:
	/**
	 * Causes the required fonts for currently selected
	 * language to be loaded if true
	 * @author Hj. Malthaner
	 */
	static void init_font_from_lang(bool font);

	sprachengui_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char *get_help_filename() const OVERRIDE {return "language.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
