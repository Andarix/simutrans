#ifndef color_gui_h
#define color_gui_h

#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_numberinput.h"

#define COLORS_MAX_BUTTONS (22)

class karte_t;

/**
 * Men� zur �nderung der Anzeigeeinstellungen.
 * @author Hj. Malthaner
 */
class color_gui_t : public gui_frame_t, private action_listener_t
{
private:
	karte_t *welt;
	button_t buttons[COLORS_MAX_BUTTONS];
	gui_numberinput_t brightness, scrollspeed, traffic_density, inp_underground_level, cursor_hide_range;

public:
	color_gui_t(karte_t *welt);

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen f�r die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char * get_hilfe_datei() const { return "display.txt"; }

	void zeichnen(koord pos, koord gr);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);

	virtual void set_fenstergroesse(koord groesse);
};

#endif
