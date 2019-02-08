#ifndef message_option_h
#define message_option_h

#include "../simmesg.h"
#include "simwin.h"

#include "gui_frame.h"
#include "components/action_listener.h"
#include "components/gui_button.h"
#include "components/gui_textarea.h"
#include "components/gui_image.h"
#include "../utils/cbuffer_t.h"


class message_option_t : public gui_frame_t, private action_listener_t
{
private:
	cbuffer_t buf;
	gui_textarea_t text_label;
	button_t buttons[4*message_t::MAX_MESSAGE_TYPE];
	gui_image_t legend;
	sint32 ticker_msg, window_msg, auto_msg, ignore_msg;


public:
	message_option_t();

	/**
	 * Set the window associated helptext
	 * @return the filename for the helptext, or NULL
	 * @author Hj. Malthaner
	 */
	const char * get_help_filename() const OVERRIDE {return "mailbox.txt";}

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	uint32 get_rdwr_id() OVERRIDE { return magic_message_options; }
};

#endif
