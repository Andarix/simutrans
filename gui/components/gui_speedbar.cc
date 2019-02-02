/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "gui_speedbar.h"
#include "../gui_theme.h"
#include "../../display/simgraph.h"
#include "../../simcolor.h"
#include "../../simtypes.h"


void gui_speedbar_t::set_base(sint32 base)
{
	this->base = base!=0 ? base : 1;
}


void gui_speedbar_t::add_color_value(const sint32 *value, PIXVAL color)
{
	info_t  next =  { color, value, -1 };
	values.insert(next);
}


scr_size gui_speedbar_t::get_min_size() const
{
	return D_INDICATOR_SIZE;
}

scr_size gui_speedbar_t::get_max_size() const
{
	return scr_size(scr_size::inf.w, D_INDICATOR_HEIGHT);
}

void gui_speedbar_t::draw(scr_coord offset)
{
	offset += pos;

	if(vertical) {
		sint32 from = size.h;
		FOR(slist_tpl<info_t>, const& i, values) {
			sint32 const to = size.h - min(*i.value, base) * size.h / base;
			if(to < from) {
				display_fillbox_wh_clip_rgb(offset.x, offset.y + to, size.w, from - to, i.color, true);
				from = to - 1;
			}
		}
		if(from > 0) {
			display_fillbox_wh_clip_rgb( offset.x, offset.y, size.w, from, color_idx_to_rgb(MN_GREY0), true);
		}
	}
	else {
		sint32 from = 0;
		FOR(slist_tpl<info_t>, const& i, values) {
			sint32 const to = min(*i.value, base) * size.w / base;
			if(to > from) {
				display_fillbox_wh_clip_rgb(offset.x + from, offset.y, to - from, size.h, i.color, true);
				from = to + 1;
			}
		}
		if(from < size.w) {
			display_fillbox_wh_clip_rgb(offset.x + from, offset.y, size.w - from, size.h, color_idx_to_rgb(MN_GREY0), true);
		}
	}
}
