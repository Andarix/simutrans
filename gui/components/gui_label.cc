/*
 * just displays a text, will be auto-translated
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#include <stdio.h>
#include <string.h>

#include "../../simdebug.h"
#include "gui_label.h"
#include "../gui_frame.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"
#include "../../simwin.h"

gui_label_t::gui_label_t(const char* text, COLOR_VAL color_, align_t align_) :
	align(align_),
	color(color_),
	tooltip(NULL)
{
	set_text( text );
}


void gui_label_t::set_text(const char *text)
{
	set_text_pointer(translator::translate(text));
}


void gui_label_t::set_text_pointer(const char *text)
{
	this->text = text;

	if (text) {
		koord groesse;
		groesse.x = display_calc_proportional_string_len_width(text,strlen(text));
		groesse.y = LINESPACE;
		set_groesse(groesse);
	}
}


void gui_label_t::zeichnen(koord offset)
{
	if(align == money) {
		if(text) {
			const char *seperator = NULL;

			if(  strrchr(text, '$')!=NULL  ) {
				seperator = strrchr(text, get_fraction_sep());
				if(seperator==NULL  &&  get_large_money_string()!=NULL) {
					seperator = strrchr(text, *(get_large_money_string()) );
				}
			}

			if(seperator) {
				display_proportional_clip(pos.x+offset.x, pos.y+offset.y, seperator, DT_DIRTY|ALIGN_LEFT, color, true);
				if(  seperator!=text  ) {
					display_text_proportional_len_clip(pos.x+offset.x, pos.y+offset.y, text, DT_DIRTY|ALIGN_RIGHT, color, seperator-text );
				}
			}
			else {
				display_proportional_clip(pos.x+offset.x, pos.y+offset.y, text, ALIGN_RIGHT, color, true);
			}
		}
	}

	else if(text) {
		int al;

		switch(align) {
			case left:
				al = ALIGN_LEFT;
				break;
			case centered:
				al = ALIGN_CENTER_H;
				break;
			case right:
				al = ALIGN_RIGHT;
				break;
			default:
				al = ALIGN_LEFT;
		}

		display_proportional_clip(pos.x+offset.x, pos.y+offset.y, text, al, color, true);

		// Max Kielland, tracking color for debug
		//display_proportional_clip(pos.x+offset.x, pos.y+offset.y, text, al, COL_CASH, true);
	}

	if ( tooltip  &&  getroffen(get_maus_x()-offset.x, get_maus_y()-offset.y) ) {
		const KOORD_VAL by = offset.y + pos.y;
		const KOORD_VAL bh = groesse.y;

		win_set_tooltip(get_maus_x() + TOOLTIP_MOUSE_OFFSET_X, by + bh + TOOLTIP_MOUSE_OFFSET_Y, tooltip, this);
	}
}

void gui_label_t::set_tooltip(const char * t)
{
	tooltip = t;
}
