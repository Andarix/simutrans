#ifndef gui_baum_edit_h
#define gui_baum_edit_h

#include "extend_edit.h"

#include "components/gui_label.h"


class baum_besch_t;
class wkz_plant_tree_t;

class baum_edit_frame_t : public extend_edit_gui_t
{
private:
	static wkz_plant_tree_t baum_tool;
	static char param_str[256];

	const baum_besch_t *besch;

	vector_tpl<const baum_besch_t *>baumlist;

	void fill_list( bool translate );

	virtual void change_item_info( sint32 i );

public:
	baum_edit_frame_t(spieler_t* sp,karte_t* welt);

	/**
	* in top-level fenstern wird der Name in der Titelzeile dargestellt
	* @return den nicht uebersetzten Namen der Komponente
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "baum builder"; }

	/**
	* Manche Fenster haben einen Hilfetext assoziiert.
	* @return den Dateinamen f�r die Hilfe, oder NULL
	* @author Hj. Malthaner
	*/
	const char* get_hilfe_datei() const { return "baum_build.txt"; }
};

#endif
