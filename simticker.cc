/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include "dataobj/koord.h"
#include "simdebug.h"
#include "simticker.h"
#include "simgraph.h"
#include "simcolor.h"
#include "tpl/slist_tpl.h"


// how much scrolling per call?
#define X_DIST 2


struct node {
	char msg[256];
	koord pos;
	PLAYER_COLOR_VAL color;
	sint16 xpos;
	long w;
};


static slist_tpl<node> list;
static koord default_pos = koord::invalid;
static bool redraw_all; // true, if also trigger background need redraw
static int next_pos;


bool ticker::empty()
{
  return list.empty();
}


void ticker::clear_ticker()
{
	list.clear();
}


void ticker::add_msg(const char* txt, koord pos, int color)
{
	// don't store more than 4 messages, it's useless.
	const int count = list.get_count();

	if(count==0) {
		redraw_all = true;
		next_pos = display_get_width();
		default_pos = koord::invalid;
	}

	if(count < 4) {
		// Don't repeat messages
		if (count == 0 || strncmp(txt, list.back().msg, strlen(txt)) != 0) {
			node n;
			int i=0;

			// remove breaks
			for(  int j=0;  i<250  &&  txt[j]!=0;  j++ ) {
					if(txt[i]=='\n'  &&  (i==0  ||  n.msg[i-1]!=' ')  ) {
					n.msg[i++] = ' ';
				}
				else {
					n.msg[i++] = txt[j];
				}
			}
			n.msg[i++] = 0;

			n.pos = pos;
			n.color = color;
			n.xpos = next_pos;
			n.w = proportional_string_width(n.msg);

			next_pos += n.w + 18;
			list.append(n);
		}
	}
}


koord ticker::get_welt_pos()
{
	return default_pos;
}


void ticker::zeichnen(void)
{
	if (!list.empty()) {
		const int start_y=display_get_height()-32;
		const int width = display_get_width();

		if(redraw_all) {
			// there should be only a single message, when this is true
			display_fillbox_wh(0, start_y, width, 1, COL_BLACK, true);
			display_fillbox_wh(0, start_y+1, width, 15, MN_GREY2, true);
		}
		else {
			display_scroll_band( start_y+4, X_DIST, 12 );
//			display_fillbox_wh(width-X_DIST, start_y, X_DIST, 1, COL_BLACK, true);
			display_fillbox_wh(width-X_DIST, start_y+1, X_DIST, 15, MN_GREY2, true);
			// ok, ready for the text
			PUSH_CLIP(width-X_DIST-1,start_y+1,X_DIST+1,15);
			for (slist_iterator_tpl<node> i(list); i.next();) {
				node* n = &i.access_current();
				n->xpos -= X_DIST;
				if(n->xpos<width) {
					display_proportional_clip(n->xpos, start_y+4, n->msg,  ALIGN_LEFT, n->color, true);
					default_pos = n->pos;
				}
			}
			// remove old news
			while (!list.empty()  &&  list.front().xpos + list.front().w < 0) {
				list.remove_first();
			}
			if(next_pos>width) {
				next_pos -= X_DIST;
			}
			POP_CLIP();
		}
		redraw_all = false;
	}
}



// complete redraw (after resizing)
void ticker::redraw_ticker()
{
	if (!list.empty()) {
		const int start_y=display_get_height()-32;
		const int width = display_get_width();

		// just draw the ticker grey ... (to be sure ... )
		display_fillbox_wh(0, start_y, width, 1, COL_BLACK, true);
		display_fillbox_wh(0, start_y+1, width, 15, MN_GREY2, true);
		for (slist_iterator_tpl<node> i(list); i.next();) {
			node* n = &i.access_current();
			n->xpos -= X_DIST;
			if(n->xpos<width) {
				display_proportional_clip(n->xpos, start_y+4, n->msg,  ALIGN_LEFT, n->color, true);
				default_pos = n->pos;
			}
		}
	}
}
