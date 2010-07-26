/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <cmath>

#include "gui_chart.h"
#include "../../dataobj/umgebung.h"
#include "../../utils/simstring.h"
#include "../../simdebug.h"
#include "../../simgraph.h"
#include "../../simcolor.h"
#include "../../simwin.h"

static char tooltip[64];

/**
 * Set background color. -1 means no background
 * @author Hj. Malthaner
 */
void gui_chart_t::set_background(int i)
{
	background = i;
}


gui_chart_t::gui_chart_t() : gui_komponente_t()
{
	// no toolstips at the start
	tooltipkoord = koord::invalid;

	seed = 0;
	show_x_axis = true;
	show_y_axis = true;
	x_elements = 0;

	// Hajo: transparent by default
	background = -1;
}


int gui_chart_t::add_curve(int color, sint64 *values, int size, int offset, int elements, int type, bool show, bool show_value, int precision )
{
	curve_t new_curve;
	new_curve.color = color;
	new_curve.values = values;
	new_curve.size = size;
	new_curve.offset = offset;
	new_curve.elements = elements;
	new_curve.show = show;
	new_curve.show_value = show_value;
	new_curve.type = type;
	new_curve.precision = precision;
	curves.append(new_curve);
	return curves.get_count();
}


void gui_chart_t::hide_curve(unsigned int id)
{
	if (id <= curves.get_count()) {
		curves.at(id).show = false;
	}
}


void gui_chart_t::show_curve(unsigned int id)
{
	if (id <= curves.get_count()) {
		curves.at(id).show = true;
	}
}


void gui_chart_t::zeichnen(koord offset)
{
	offset += pos;

	sint64 last_year=0, tmp=0;
	char cmin[128] = "0", cmax[128] = "0", digit[8];

	sint64 baseline = 0;
	sint64* pbaseline = &baseline;

	float scale = 0;
	float* pscale = &scale;

	// calc baseline and scale
	calc_gui_chart_values(pbaseline, pscale, cmin, cmax);

	// Hajo: draw background if desired
	if(background != -1) {
		display_fillbox_wh_clip(offset.x, offset.y, groesse.x, groesse.y, background, false);
	}
	int tmpx, factor;
	if(umgebung_t::left_to_right_graphs) {
		tmpx = offset.x + groesse.x - groesse.x % (x_elements - 1);
		factor = -1;
	}
	else {
		tmpx = offset.x;
		factor = 1;
	}

	// draw zero line
	display_direct_line(offset.x+1, offset.y+baseline, offset.x+groesse.x-2, offset.y+baseline, MN_GREY4);

	if (show_y_axis) {

		// draw zero number only, if it will not disturb any other printed values!
		if ((baseline > 18) && (baseline < groesse.y -18)) {
			display_proportional_clip(offset.x - 4, offset.y+baseline-3, "0", ALIGN_RIGHT, COL_WHITE, true );
		}

		// display min/max money values
		display_proportional_clip(offset.x - 4, offset.y-5, cmax, ALIGN_RIGHT, COL_WHITE, true );
		display_proportional_clip(offset.x - 4, offset.y+groesse.y-5, cmin, ALIGN_RIGHT, COL_WHITE, true );
	}

	// draw chart frame
	display_ddd_box_clip(offset.x, offset.y, groesse.x, groesse.y, COL_GREY1, COL_WHITE);

	// draw chart lines
	for (int i = 0; i<x_elements; i++) {
		if (show_x_axis) {
			// display x-axis
			sprintf(digit, "%i", abs(seed-i));
			display_proportional_clip(tmpx+factor*(groesse.x / (x_elements - 1))*i - (seed != i ? (int)(2*log((double)abs((seed-i)))) : 0), offset.y+groesse.y+6, digit, ALIGN_LEFT, MN_GREY4, true );
		}
		// year's vertical lines
		display_vline_wh_clip(tmpx+factor*(groesse.x / (x_elements - 1))*i, offset.y+1, groesse.y-2, MN_GREY4, false);
	}

	// display current value?
	int tooltip_n=-1;
	if(tooltipkoord!=koord::invalid) {
		if(umgebung_t::left_to_right_graphs) {
			tooltip_n = x_elements-1-(tooltipkoord.x*x_elements+4)/(groesse.x|1);
		}
		else {
			tooltip_n = (tooltipkoord.x*x_elements+4)/(groesse.x|1);
		}
	}

	// draw chart's curves
	for (slist_iterator_tpl<curve_t> i(curves); i.next();) {
		const curve_t& c = i.get_current();
		if (c.show) {
			// for each curve iterate through all elements and display curve
			for (int i=0;i<c.elements;i++) {
				//tmp=c.values[year*c.size+c.offset];
				c.type == 0 ? tmp = c.values[i*c.size+c.offset] : tmp = c.values[i*c.size+c.offset] / 100;
				// display marker(box) for financial value
				display_fillbox_wh_clip(tmpx+factor*(groesse.x / (x_elements - 1))*i-2, offset.y+baseline- (int)(tmp/scale)-2, 5, 5, c.color, true);

				// display tooltip?
				if(i==tooltip_n  &&  abs((int)(baseline-(int)(tmp/scale)-tooltipkoord.y))<10) {
					number_to_string(tooltip, tmp, c.precision);
					win_set_tooltip( get_maus_x()+8, get_maus_y()-12, tooltip, NULL );
				}

				// draw line between two financial markers; this is only possible from the second value on
				if (i>0) {
					display_direct_line(tmpx+factor*(groesse.x / (x_elements - 1))*(i-1),
						offset.y+baseline-(int)(last_year/scale),
						tmpx+factor*(groesse.x / (x_elements - 1))*(i),
						offset.y+baseline-(int)(tmp/scale),
						c.color);
				}
				else {
					// for the first element print the current value (optionally)
					// only print value if not too narrow to min/max/zero
					if(  c.show_value  ) {
						if(  umgebung_t::left_to_right_graphs  ) {
							number_to_string(cmin, tmp, c.precision);
							const sint16 width = proportional_string_width(cmin)+7;
							display_ddd_proportional( tmpx + 8, offset.y+baseline-(int)(tmp/scale)-4, width, 0, COL_GREY4, c.color, cmin, true);
						}
						else if(  (baseline-tmp/scale-8) > 0  &&  (baseline-tmp/scale+8) < groesse.y  &&  abs((int)(tmp/scale)) > 9  ) {
							number_to_string(cmin, tmp, c.precision);
							display_proportional_clip(tmpx - 4, offset.y+baseline-(int)(tmp/scale)-4, cmin, ALIGN_RIGHT, c.color, true );
						}
					}
				}
				last_year=tmp;
			}
		}
		last_year=tmp=0;
	}
}


void gui_chart_t::calc_gui_chart_values(sint64 *baseline, float *scale, char *cmin, char *cmax) const
{
	sint64 tmp=0;
	sint64 min = 0, max = 0;
	int precision = 0;

	for(  slist_iterator_tpl<curve_t> i(curves);  i.next();  ) {
		const curve_t& c = i.get_current();
		if(  c.show  ) {
			for(  int i=0;  i<c.elements;  i++  ) {
				c.type == 0 ? tmp = c.values[i*c.size+c.offset] : tmp = c.values[i*c.size+c.offset] / 100;
				if (min > tmp) {
					min = tmp ;
					precision = c.precision;
				}
				if (max < tmp) {
					max = tmp;
					precision = c.precision;
				}
			}
		}
	}

	number_to_string(cmin, min, precision);
	number_to_string(cmax, max, precision);

	// scale: factor to calculate money with, to get y-pos offset
	*scale = (float)(max - min) / (groesse.y-2);
	if(*scale==0.0) {
		*scale = 1.0;
	}

	// baseline: y-pos for the "zero" line in the chart
	*baseline = (sint64)(groesse.y - abs((int)(min / *scale )));
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_chart_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTREPEAT(ev)  ||  IS_LEFTCLICK(ev)) {
		// tooptip to show?
		tooltipkoord = koord(ev->mx,ev->my);
	}
	else {
		tooltipkoord = koord::invalid;
		tooltip[0] = 0;
	}
	return true;
}
