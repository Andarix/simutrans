/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef gui_optionen_h
#define gui_optionen_h


class gui_frame_t;
class action_listener_t;
class button_t;
class gui_action_creator_t;

/*
 * Settings in the game
 *
 * Dialog for game options/Main menu
 */
class optionen_gui_t : public gui_frame_t, action_listener_t
{
	private:
		button_t option_buttons[11];

	public:
		optionen_gui_t();

		/**
		 * Set the window associated helptext
		 * @return the filename for the helptext, or NULL
		 */
		const char * get_help_filename() const OVERRIDE {return "options.txt";}

		bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
