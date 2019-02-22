/*
 * The trees builder
 */

#ifndef gui_baum_edit_h
#define gui_baum_edit_h

#include "extend_edit.h"
#include "components/gui_image.h"


class tree_desc_t;
class tool_plant_tree_t;

class baum_edit_frame_t : public extend_edit_gui_t
{
private:
	static tool_plant_tree_t baum_tool;
	static cbuffer_t param_str;

	const tree_desc_t *desc;

	gui_image_t tree_image;

	vector_tpl<const tree_desc_t *>tree_list;

	void fill_list( bool translate ) OVERRIDE;

	void change_item_info( sint32 i ) OVERRIDE;

public:
	baum_edit_frame_t(player_t* player_);

	/**
	* in top-level windows the name is displayed in titlebar
	* @return the non-translated component name
	* @author Hj. Malthaner
	*/
	const char* get_name() const { return "baum builder"; }

	/**
	* Set the window associated helptext
	* @return the filename for the helptext, or NULL
	* @author Hj. Malthaner
	*/
	const char* get_help_filename() const OVERRIDE { return "baum_build.txt"; }
};

#endif
