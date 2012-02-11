/*
 * Copyright (c) 1997 - 2004 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef banner_h
#define banner_h

#include "components/gui_button.h"
#include "components/gui_image.h"
#include "gui_frame.h"

class karte_t;

/**
 * Eine Klasse, die ein Fenster zur Auswahl von bis zu acht
 * Parametern f�r ein Werkzeug per Icon darstellt.
 *
 * @author Hj. Malthaner
 */
class banner_t : public gui_frame_t, action_listener_t
{
private:
	sint32 last_ms;
	int line;
	sint16 xoff, yoff;

	button_t new_map, load_map, join_map, quit;
	gui_image_t logo;

	karte_t *welt;

public:
	banner_t( karte_t *w );

	bool has_sticky() const { return false; }

	virtual bool has_title() const { return false; }

	/**
	* Fenstertitel
	* @author Hj. Malthaner
	*/
	const char *get_name() const {return ""; }

	/**
	* gibt farbinformationen fuer Fenstertitel, -r�nder und -k�rper
	* zur�ck
	* @author Hj. Malthaner
	*/
	PLAYER_COLOR_VAL get_titelcolor() const {return WIN_TITEL; }

	bool getroffen(int, int) OVERRIDE { return true; }

	/* Events werden hiermit an die GUI-Komponenten
	* gemeldet
	* @author Hj. Malthaner
	*/
	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* komponente neu zeichnen. Die �bergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	* @author Hj. Malthaner
	*/
	void zeichnen(koord pos, koord gr);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

#endif
