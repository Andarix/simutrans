#ifndef labellist_frame_t_h
#define labellist_frame_t_h

#include "../gui/gui_frame.h"
#include "../gui/labellist_stats_t.h"
#include "components/action_listener.h"
#include "components/gui_label.h"
#include "components/gui_scrollpane.h"

class karte_t;

/**
 * label list window
 * @author Hj. Malthaner
 */
class labellist_frame_t : public gui_frame_t, private action_listener_t
{
 private:
    static const char *sort_text[labellist::SORT_MODES];

    gui_label_t sort_label;
    button_t	sortedby;
    button_t	sorteddir;
    button_t	filter;
    labellist_stats_t stats;

    gui_scrollpane_t scrolly;
    /*
     * All filter settings are static, so they are not reset each
     * time the window closes.
     */
    static labellist::sort_mode_t sortby;
    static bool sortreverse;
	static bool filter_state;
 public:
    labellist_frame_t(karte_t * welt);

    /**
     * resize window in response to a resize event
     * @author Hj. Malthaner
     */
    void resize(const koord delta);

    /**
     * Manche Fenster haben einen Hilfetext assoziiert.
     * @return den Dateinamen f�r die Hilfe, oder NULL
     * @author V. Meyer
     */
    const char * get_hilfe_datei() const {return "labellist_filter.txt"; }

     /**
     * This function refreshs the station-list
     * @author Markus Weber
     */
    void display_list();

    static labellist::sort_mode_t get_sortierung() { return sortby; }
    static void set_sortierung(const labellist::sort_mode_t sm) { sortby = sm; }

    static bool get_reverse() { return sortreverse; }
    static void set_reverse(const bool& reverse) { sortreverse = reverse;    }

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     *
     * Returns true, if action is done and no more
     * components should be triggered.
     * V.Meyer
     */
    bool action_triggered( gui_action_creator_t *komp, value_t extra);
};

#endif
