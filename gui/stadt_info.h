/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_stadt_info_h
#define gui_stadt_info_h

#include "../simcity.h"

#include "gui_frame.h"
#include "components/gui_chart.h"
#include "components/gui_textinput.h"
#include "components/action_listener.h"
#include "components/gui_label.h"
#include "components/gui_button.h"
#include "components/gui_tab_panel.h"
#include "../tpl/array2d_tpl.h"

class stadt_t;
template <class T> class sparse_tpl;

/**
 * Dies stellt ein Fenster mit den Informationen
 * ueber eine Stadt dar.
 *
 * @author Hj. Malthaner
 */
class stadt_info_t : public gui_frame_t, private action_listener_t
{
private:
	char name[256], old_name[256];

	stadt_t *stadt;

	uint32 minimapSize;	// size of minimaps

    button_t allow_growth;

	gui_textinput_t name_input;

	gui_tab_panel_t year_month_tabs;

	gui_chart_t chart, mchart;

	button_t filterButtons[MAX_CITY_HISTORY];
	bool bFilterIsActive[MAX_CITY_HISTORY];

	array2d_tpl<uint8> pax_dest_old, pax_dest_new;

	unsigned long pax_destinations_last_change;

	void init_pax_dest( array2d_tpl<uint8> &pax_dest );
	void add_pax_dest( array2d_tpl<uint8> &pax_dest, const sparse_tpl< uint8 >* city_pax_dest );

	void rename_city();

	void reset_city_name();

public:
	stadt_info_t(stadt_t *stadt);

	virtual ~stadt_info_t();

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen f�r die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "citywindow.txt";}

	virtual koord3d get_weltpos();

	/**
	* komponente neu zeichnen. Die �bergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirmkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	*/
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

	void map_rotate90( sint16 );

	// since we need to update the city pointer when topped
	bool infowin_event(const event_t *ev);

	void update_data();

	/**
	 * Does this window need a min size button in the title bar?
	 * @return true if such a button is needed
	 * @author Hj. Malthaner
	 */
	virtual bool has_min_sizer() const {return true;}

	/**
	* Set window size and adjust component sizes and/or positions accordingly
	*/
	virtual void set_fenstergroesse(koord groesse);
};

#endif
