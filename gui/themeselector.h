/*
 * selection of paks at the start time
 */

#ifndef themeselector_h
#define themeselector_h

#include "simwin.h"
#include "savegame_frame.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer_t.h"

class themeselector_t : public savegame_frame_t
{
protected:
	static std::string undo; // undo buffer

	bool        item_action   ( const char *fullpath ) OVERRIDE;
	bool        ok_action     ( const char *fullpath ) OVERRIDE;
	bool        cancel_action ( const char *fullpath ) OVERRIDE;
	const char* get_info      ( const char *fname    ) OVERRIDE;
	bool        check_file    ( const char *filename, const char *suffix ) OVERRIDE;
	void        fill_list       ( void ) OVERRIDE;

public:
	themeselector_t ( void );

	const char* get_help_filename ( void ) const OVERRIDE { return NULL; }

	uint32      get_rdwr_id     ( void ) OVERRIDE { return magic_themes; }
	void        rdwr            ( loadsave_t *file ) OVERRIDE;
};

#endif
