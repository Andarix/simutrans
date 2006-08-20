#ifndef gui_optionen_h
#define gui_optionen_h


#include "gui_frame.h"
#include "button.h"
#include "gui_label.h"
#include "components/gui_divider.h"
#include "ifc/action_listener.h"


/**
 * Settings in the game
 * @author Hj. Malthaner
 * @version $Revision: 1.8 $
 */
class optionen_gui_t : public gui_frame_t, action_listener_t
{
private:
    button_t bt_lang;
    button_t bt_color;
    button_t bt_display;
    button_t bt_sound;
    button_t bt_player;
    button_t bt_load;
    button_t bt_save;
    button_t bt_new;
    button_t bt_quit;

	gui_divider_t seperator;
	gui_label_t txt;

	karte_t *welt;

public:
    optionen_gui_t(karte_t *welt);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen f�r die Hilfe, oder NULL
     * @author Hj. Malthaner
     */
    virtual const char * gib_hilfe_datei() const {return "options.txt";};

	/**
	 * This method is called if an action is triggered (i.e. button pressed and released)
	 * @author Hj. Malthaner
	 */
	bool action_triggered(gui_komponente_t *comp);
};

#endif
