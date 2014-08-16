#include "gui_map_preview.h"
#include "../../simworld.h"

gui_map_preview_t::gui_map_preview_t() :
	gui_component_t()
{
	map_data = NULL;
	map_size = scr_size(0,0);
	set_size (scr_size( MAP_PREVIEW_SIZE_X,MAP_PREVIEW_SIZE_Y ));
}
