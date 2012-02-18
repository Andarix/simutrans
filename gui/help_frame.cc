/*
 * Copyright (c) 1997 - 2002 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <string>

#include "../simmem.h"
#include "../simwin.h"
#include "../simmenu.h"
#include "../simsys.h"
#include "../simworld.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"

#include "help_frame.h"


void help_frame_t::set_text(const char * buf)
{
	flow.set_text(buf);

	flow.set_pos(koord(10, 10));
	flow.set_groesse(koord(220, 0));

	// try to get the following sizes
	// y<400 or, if not possible, x<620
	int last_y = 0;
	koord curr=flow.get_preferred_size();
	for( int i=0;  i<10  &&  curr.y>400  &&  curr.y!=last_y;  i++  )
	{
		flow.set_groesse(koord(260+i*40, 0));
		last_y = curr.y;
		curr = flow.get_preferred_size();
	}

	// the second line isn't redundant!!!
	flow.set_groesse(flow.get_preferred_size());
	flow.set_groesse(flow.get_preferred_size());

	set_name(flow.get_title());

	// set window size
	curr = flow.get_groesse()+koord(20, 36);
	if(curr.y>display_get_height()-64) {
		curr.y = display_get_height()-64;
	}
	set_fenstergroesse( curr );
	resize( koord(0,0) );
}


help_frame_t::help_frame_t() :
	gui_frame_t( translator::translate("Help") ),
	scrolly(&flow)
{
	set_text("<title>Unnamed</title><p>No text set</p>");
	flow.add_listener(this);
	set_resizemode(diagonal_resize);
	scrolly.set_show_scroll_x(true);
	add_komponente(&scrolly);
	set_min_windowsize(koord(16*4, 16));
}


help_frame_t::help_frame_t(char const* const filename) :
	gui_frame_t( translator::translate("Help") ),
	scrolly(&flow)
{
	// the key help texts are built automagically
	if (strcmp(filename, "keys.txt") == 0) {
		cbuffer_t buf;
		buf.append( translator::translate( "<title>Keyboard Help</title>\n<h1><strong>Keyboard Help</strong></h1><p>\n" ) );
		spieler_t *sp = spieler_t::get_welt()->get_active_player();
		const char *trad_str = translator::translate( "<em>%s</em> - %s<br>\n" );
		FOR(vector_tpl<werkzeug_t*>, const i, werkzeug_t::char_to_tool) {
			char const* c = NULL;
			char str[16];
			switch (uint16 const key = i->command_key) {
				case '<': c = "&lt;"; break;
				case '>': c = "&gt;"; break;
				case 27:  c = "ESC"; break;
				case SIM_KEY_HOME:	c=translator::translate( "[HOME]" ); break;
				case SIM_KEY_END:	c=translator::translate( "[END]" ); break;
				default:
					if (key < 32) {
						sprintf(str, "%s + %c", translator::translate("[CTRL]"), '@' + key);
					} else if (key < 256) {
						sprintf(str, "%c", key);
					} else if (key < SIM_KEY_F15) {
						sprintf(str, "F%i", key - SIM_KEY_F1 + 1);
					}
					else {
						// try unicode
						str[utf16_to_utf8(key, (utf8*)str)] = '\0';
					}
					c = str;
					break;
			}
			buf.printf(trad_str, c, i->get_tooltip(sp));
		}
		set_text(buf);
	}
	else {
		std::string file_prefix("text/");
		std::string fullname = file_prefix + translator::get_lang()->iso + "/" + filename;
		chdir(umgebung_t::program_dir);

		FILE* file = fopen(fullname.c_str(), "rb");
		if (!file) {
			//Check for the 'base' language(ie en from en_gb)
			file = fopen((file_prefix + translator::get_lang()->iso_base + "/" + filename).c_str(), "rb");
		}
		if (!file) {
			// Hajo: check fallback english
			file = fopen((file_prefix + "/en/" + filename).c_str(), "rb");
		}
		// go back to load/save dir
		chdir( umgebung_t::user_dir );

		bool success=false;
		if(file) {
			fseek(file,0,SEEK_END);
			long len = ftell(file);
			if(len>0) {
				char* const buf = MALLOCN(char, len + 1);
				fseek(file,0,SEEK_SET);
				fread(buf, 1, len, file);
				buf[len] = '\0';
				fclose(file);
				success = true;
				set_text(buf);
				free(buf);
			}
		}

		if(!success) {
			set_text("<title>Error</title>Help text not found");
		}
	}

	set_resizemode(diagonal_resize);
	scrolly.set_show_scroll_x(true);
	add_komponente(&scrolly);
	flow.add_listener(this);
	set_min_windowsize(koord(16*4, 16));
}


/**
 * Called upon link activation
 * @param the hyper ref of the link
 * @author Hj. Malthaner
 */
bool help_frame_t::action_triggered( gui_action_creator_t *, value_t extra)
{
	const char *str = (const char *)(extra.p);
	uint32 magic = 0;
	while(*str) {
		magic += *str++;
	}
	magic = (magic % 842) + magic_info_pointer;
	create_win(new help_frame_t((const char *)(extra.p)), w_info, magic );
	return true;
}



/**
 * Resize the contents of the window
 * @author Markus Weber
 */
void help_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	scrolly.set_groesse(get_client_windowsize());
	koord gr = get_client_windowsize() -flow.get_pos() - koord(scrollbar_t::BAR_SIZE, scrollbar_t::BAR_SIZE);
	flow.set_groesse( gr );
	flow.set_groesse( flow.get_text_size());
}
