#ifndef gui_schedule_list_h
#define gui_schedule_list_h

#include "gui_frame.h"
#include "gui_container.h"
#include "components/gui_label.h"
#include "components/gui_chart.h"
#include "components/gui_textinput.h"
#include "components/gui_scrolled_list.h"
#include "components/gui_scrollpane.h"
#include "components/gui_tab_panel.h"
#include "gui_convoiinfo.h"
#include "../simline.h"

class spieler_t;

/**
 * Window.
 * Will display list of schedules.
 * Contains buttons: edit new remove
 * Resizable.
 *
 * @author Niels Roest
 * @author hsiegeln: major redesign
 */
class schedule_list_gui_t : public gui_frame_t, public action_listener_t
{
private:
	spieler_t *sp;

	button_t bt_new_line, bt_change_line, bt_delete_line, bt_withdraw_line;;
	gui_container_t cont, cont_haltestellen;
	gui_scrollpane_t scrolly_convois, scrolly_haltestellen;
	gui_scrolled_list_t scl;
	gui_speedbar_t filled_bar;
	gui_textinput_t inp_name, inp_filter;
	gui_label_t lbl_filter;
	gui_chart_t chart;
	button_t filterButtons[MAX_LINE_COST];
	gui_tab_panel_t tabs;

	sint32 selection, capacity, load, loadfactor;

	uint32 old_line_count;
	sint32 last_schedule_count;
	uint32 last_vehicle_count;

	// only show schedules containing ...
	char schedule_filter[512], old_schedule_filter[512];

	// so even japanese can have long enough names ...
	char line_name[512], old_line_name[512];

	// resets textinput to current line name
	// necessary after line was renamed
	void reset_line_name();

	// rename selected line
	// checks if possible / necessary
	void rename_line();

	void display(koord pos);

	void update_lineinfo(linehandle_t new_line);

	linehandle_t line;

	vector_tpl<linehandle_t> lines;

	void build_line_list(int filter);

public:
	schedule_list_gui_t(spieler_t* sp);
	~schedule_list_gui_t();
	/**
	* in top-level fenstern wird der Name in der Titelzeile dargestellt
	* @return den nicht uebersetzten Namen der Komponente
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "Line Management"; }

	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen f�r die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	const char* get_hilfe_datei() const { return "linemanagement.txt"; }

	/**
	* Does this window need a min size button in the title bar?
	* @return true if such a button is needed
	* @author Hj. Malthaner
	*/
	bool has_min_sizer() const {return true;}

	/**
	* komponente neu zeichnen. Die �bergebenen Werte beziehen sich auf
	* das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
	* in dem die Komponente dargestellt wird.
	*
	* @author Hj. Malthaner
	*/
	void zeichnen(koord pos, koord gr);

	/**
	* Set window size and adjust component sizes and/or positions accordingly
	* @author Hj. Malthaner
	*/
	virtual void set_fenstergroesse(koord groesse);

	/**
	* Mausklicks werden hiermit an die GUI-Komponenten
	* gemeldet
	*/
	bool infowin_event(const event_t *ev);

	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;

	/**
	 * Select line and show its info
	 * @author isidoro
	 */
	void show_lineinfo(linehandle_t line);

	/**
	 * called after renaming of line
	 */
	void update_data(linehandle_t changed_line);

	// follwoing: rdwr stuff
	void rdwr( loadsave_t *file );
	uint32 get_rdwr_id();
};

#endif
