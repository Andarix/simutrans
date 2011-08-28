/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef fabrikinfo_t_h
#define fabrikinfo_t_h

#include "factory_chart.h"
#include "components/action_listener.h"
#include "components/gui_scrollpane.h"
#include "components/gui_textarea.h"
#include "components/gui_textinput.h"
#include "components/ding_view_t.h"
#include "gui_container.h"
#include "../utils/cbuffer_t.h"

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
	const fabrik_t* fab;

	cbuffer_t info_buf, prod_buf;

	factory_chart_t chart;
	button_t chart_button;

	button_t details_button;

	ding_view_t view;

	char fabname[256];
	char fabkoordname[300];
	gui_textinput_t input;

	button_t *lieferbuttons;
	button_t *supplierbuttons;
	button_t *stadtbuttons;

	gui_scrollpane_t scrolly;
	gui_container_t cont;
	gui_textarea_t prod, txt;

	// refreshes all text and location pointers
	void update_info();

 public:
	fabrik_info_t(const fabrik_t* fab, const gebaeude_t* gb);
	virtual ~fabrik_info_t();

	/**
	 * Manche Fenster haben einen Hilfetext assoziiert.
	 * @return den Dateinamen f�r die Hilfe, oder NULL
	 * @author Hj. Malthaner
	 */
	const char *get_hilfe_datei() const {return "industry_info.txt";}

	virtual bool has_min_sizer() const {return true;}

	virtual koord3d get_weltpos() { return fab->get_pos(); }

	virtual void set_fenstergroesse(koord groesse);

	/**
	* komponente neu zeichnen. Die �bergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	*
	* @author Hj. Malthaner
	*/
	virtual void zeichnen(koord pos, koord gr);

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	bool action_triggered( gui_action_creator_t *komp, value_t extra);

	// rotated map need new info ...
	void map_rotate90( sint16 ) { update_info(); }
};

#endif
