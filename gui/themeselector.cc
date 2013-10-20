/*
 * selection of paks at the start time
 */

#include <string>

#include "themeselector.h"
#include "simwin.h"
#include "../simsys.h"
#include "../simevent.h"
#include "gui_theme.h"
#include "../utils/simstring.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/tabfile.h"

#define L_ADDON_WIDTH (150)

std::string themeselector_t::undo = "";

themeselector_t::themeselector_t() :
	savegame_frame_t( ".tab" )//, false, NULL, false )
{
	// remove unnecessary buttons
	remove_komponente( &input );
	delete_enabled = false;
	label_enabled  = false;

	set_name( translator::translate( "Theme selector" ) );
	fnlabel.set_text_pointer( translator::translate( "Select a theme for display" ) );
	if( undo.empty() ) {
		undo = gui_theme_t::get_current_theme();
		set_fenstergroesse(get_min_windowsize());
	}
}

bool themeselector_t::check_file(const char *filename, const char *suffix)
{
	return savegame_frame_t::check_file(filename,suffix);
}



// A theme button was pressed
bool themeselector_t::item_action(const char *fullpath)
{
	gui_theme_t::themes_init(fullpath);

	event_t *ev = new event_t();
	ev->ev_class = EVENT_SYSTEM;
	ev->ev_code = SYSTEM_RELOAD_WINDOWS;
	queue_event( ev );

	return false;
}



// Ok button was pressed
bool themeselector_t::ok_action(const char *fullpath)
{
	undo = "";
	return true;
}



// Cancel button was pressed
bool themeselector_t::cancel_action(const char *fullpath)
{
	item_action(undo.c_str());
	undo = "";
	return true;
}


// returns the additional name of the file
const char *themeselector_t::get_info(const char *fn )
{
	const char *info = "";
	tabfile_t themesconf;

	if(  themesconf.open(fn)  ) {
		tabfileobj_t contents;

		// get trimmed theme name
		themesconf.read(contents);
		std::string name( contents.get( "name" ) );
		info = strdup( trim( name ).c_str() );

	}
	themesconf.close();
	return info;
}


void themeselector_t::fill_list()
{
	add_path( ((std::string)env_t::program_dir+"themes/").c_str() );
	add_path( ((std::string)env_t::user_dir+"themes/").c_str() );

	// do the search ...
	savegame_frame_t::fill_list();

	FOR(slist_tpl<dir_entry_t>, const& i, entries) {

		if (i.type == LI_HEADER) {
			continue;
		}

		delete[] i.button->get_text(); // free up default allocation.
		i.button->set_typ(button_t::roundbox_state);
		i.button->set_text(i.label->get_text_pointer());
		i.button->pressed = !strcmp( gui_theme_t::get_current_theme(), i.label->get_text_pointer() ); // mark current theme
		i.label->set_text_pointer( NULL ); // remove reference to prevent conflicts at delete[]

		// Get a new label buffer since the original is now owned by i.button.
		// i.label->set_text_pointer( strdup( get_filename(i.info).c_str() ) );

	}

	if(entries.get_count() <= this->num_sections+1) {
		// less than two themes exist => we coudl close now ...
	}

	// force new resize after we have rearranged the gui
	resize(koord(0,0));
}


void themeselector_t::rdwr( loadsave_t *file )
{
	koord gr = get_fenstergroesse();
	gr.rdwr( file );
	if(  file->is_loading()  ) {
		set_fenstergroesse( gr );
		resize( koord(0,0) );
	}
}
