/*
 * Copyright (c) 2011 Knightly
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */


#include "../dings/leitung2.h"
#include "../dataobj/translator.h"
#include "components/list_button.h"
#include "factory_chart.h"

#define CHART_WIDTH  (300)
#define CHART_HEIGHT (160)

static sint64 convert_goods(sint64 value) { return ( (value + (1<<(fabrik_t::precision_bits-1))) >> fabrik_t::precision_bits ); }
static sint64 convert_power(sint64 value) { return ( value >> POWER_TO_MW ); }
static sint64 convert_boost(sint64 value) { return ( (value * 100 + (DEFAULT_PRODUCTION_FACTOR>>1)) >> DEFAULT_PRODUCTION_FACTOR_BITS ); }

#define MAX_GOODS_COLOR (24)
static const int goods_color[MAX_GOODS_COLOR][MAX_FAB_GOODS_STAT] =
{
	/* greyish blue  */ {   2,   5,   7 },
	/* bright orange */ {  35,  37,  39 },
	/* cyan          */ {  50,  53,  55 },
	/* lemon yellow  */ {  26,  29,  31 },
	/* purple        */ {  59,  61,  63 },
	/* greyish green */ {  82,  84,  87 },
	/* lilac         */ { 107, 109, 111 },
	/* pale brown    */ {  91,  93,  95 },
	/* blue          */ { 146, 149, 151 },
	/* dark green    */ { 163, 165, 167 },
	/* dark brown    */ { 179, 181, 183 },
	/* dark blue     */ {  99, 101, 103 },
	/* green         */ {  43,  45,  47 },
	/* reddish brown */ { 115, 117, 119 },
	/* magenta       */ {  75,  77,  79 },
	/* turquoise     */ { 123, 125, 127 },
	/* red           */ { 131, 133, 135 },
	/* muddy yellow  */ { 194, 197, 199 },
	/* bright green  */ { 137, 140, 143 },
	/* dull orange   */ {  67,  69,  71 },
	/* pale yellow   */ { 169, 172, 175 },
	/* pale green    */ { 203, 205, 207 },
	/* orange        */ { 154, 157, 159 },
	/* pale purple   */ { 219, 221, 223 }
};

static const char *const input_type[MAX_FAB_GOODS_STAT] =
{
	"Storage", "Received", "Consumed"
};

static const char *const output_type[MAX_FAB_GOODS_STAT] =
{
	"Storage", "Delivered", "Produced"
};

static const gui_chart_t::convert_proc goods_convert[MAX_FAB_GOODS_STAT] =
{
	convert_goods, NULL, convert_goods
};

static const char *const prod_type[MAX_FAB_STAT] =
{
	"Production", "Usage/Output",
	"Electricity", "Passagiere", "Post",
	"Generated", "Departed", "Arrived",
	"Generated", "Departed", "Arrived"
};

static const int prod_color[MAX_FAB_STAT] =
{
	COL_LILAC, COL_LEMON_YELLOW,
	COL_LIGHT_GREEN, 23, COL_LIGHT_PURPLE,
	COL_LIGHT_TURQUOISE, 51, 49,
	COL_LIGHT_ORANGE, COL_ORANGE, COL_DARK_ORANGE
};

static const gui_chart_t::convert_proc prod_convert[MAX_FAB_STAT] =
{
	NULL, convert_power, convert_boost, convert_boost, convert_boost, NULL, NULL, NULL, NULL, NULL, NULL
};

static const gui_chart_t::convert_proc ref_convert[MAX_FAB_REF_LINE] =
{
	convert_boost, convert_boost, convert_boost, convert_power, NULL, NULL
};

static const koord button_pos[MAX_FAB_STAT] =
{
	/* Production */  koord(0, 0),              koord(2, 0),
	/* Boost      */  koord(0, 1), koord(1, 1), koord(2, 1),
	/* Max Boost  */
	/* Demand     */
	/* Pax        */  koord(0, 4), koord(1, 4), koord(2, 4),
	/* Mail       */  koord(0, 5), koord(1, 5), koord(2, 5)
};

static const int ref_color[MAX_FAB_REF_LINE] =
{
	137, COL_LIGHT_BLUE, COL_LIGHT_RED,
	COL_DARK_GREEN, 100, 132
};

static const char *const label_text[MAX_PROD_LABEL] =
{
	"Production :", "Power (MW) :", "Boost % :", "Max Boost % :", "Demand :", "Passenger :", "Mail :"
};

static const koord label_pos[MAX_PROD_LABEL] =
{
	koord(0, 0), koord(240, 0), koord(0, (BUTTON_SPACER+BUTTON_HEIGHT)), koord(0, (BUTTON_SPACER+BUTTON_HEIGHT)*2),
	koord(0, (BUTTON_SPACER+BUTTON_HEIGHT)*3), koord(0, (BUTTON_SPACER+BUTTON_HEIGHT)*4), koord(0, (BUTTON_SPACER+BUTTON_HEIGHT)*5)
};


factory_chart_t::factory_chart_t(const fabrik_t *_factory) :
	gui_frame_t( _factory->get_name(), _factory->get_besitzer() ), factory(_factory), goods_buttons(NULL), goods_labels(NULL), goods_button_count(0), goods_label_count(0)
{
	const sint16 offset_below_chart = 10 + CHART_HEIGHT + 20;
	tab_panel.set_pos( koord(0, 0) );

	// GUI components for goods input/output statistics
	goods_chart.set_pos( koord(10 + 80, 10) );
	goods_chart.set_groesse( koord( CHART_WIDTH, CHART_HEIGHT ) );
	goods_chart.set_dimension(12, 10000);
	goods_chart.set_background(MN_GREY1);
	const uint32 input_count = factory->get_eingang().get_size();
	const uint32 output_count = factory->get_ausgang().get_size();
	if(  input_count>0  ||  output_count>0  ) {
		goods_buttons = new button_t[ (input_count + output_count) * 3 ];
		goods_labels = new gui_label_t[ (input_count>0 ? input_count + 1 : 0) + (output_count>0 ? output_count + 1 : 0) ];
	}
	if(  input_count>0  ) {
		goods_labels[goods_label_count].set_text( "<Input>" );
		goods_labels[goods_label_count].set_pos( koord( 10, offset_below_chart+3+(BUTTON_SPACER+BUTTON_HEIGHT)*goods_label_count ) );
		goods_cont.add_komponente( goods_labels + goods_label_count );
		++goods_label_count;
		const array_tpl<ware_production_t> &input = factory->get_eingang();
		for(  uint32 g=0;  g<input_count;  ++g  ) {
			goods_labels[goods_label_count].set_text( input[g].get_typ()->get_name() );
			goods_labels[goods_label_count].set_pos( koord( 20, offset_below_chart+3+(BUTTON_SPACER+BUTTON_HEIGHT)*goods_label_count ) );
			goods_cont.add_komponente( goods_labels + goods_label_count );
			for(  int s=0;  s<MAX_FAB_GOODS_STAT;  ++s  ) {
				goods_chart.add_curve( goods_color[goods_label_count%MAX_GOODS_COLOR][s], input[g].get_stats(), MAX_FAB_GOODS_STAT, s, MAX_MONTH, false, false, true, 0, goods_convert[s] );
				goods_buttons[goods_button_count].init(button_t::box_state, input_type[s], koord(10+100+(BUTTON_SPACER+BUTTON_WIDTH)*s, offset_below_chart+(BUTTON_SPACER+BUTTON_HEIGHT)*goods_label_count), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
				goods_buttons[goods_button_count].background = goods_color[goods_label_count%MAX_GOODS_COLOR][s];
				goods_buttons[goods_button_count].pressed = false;
				goods_buttons[goods_button_count].add_listener(this);
				goods_cont.add_komponente( goods_buttons + goods_button_count );
				++goods_button_count;
			}
			++goods_label_count;
		}
	}
	if(  output_count>0  ) {
		goods_labels[goods_label_count].set_text( "<Output>" );
		goods_labels[goods_label_count].set_pos( koord( 10, offset_below_chart+3+(BUTTON_SPACER+BUTTON_HEIGHT)*goods_label_count ) );
		goods_cont.add_komponente( goods_labels + goods_label_count );
		++goods_label_count;
		const array_tpl<ware_production_t> &output = factory->get_ausgang();
		for(  uint32 g=0;  g<output_count;  ++g  ) {
			goods_labels[goods_label_count].set_text( output[g].get_typ()->get_name() );
			goods_labels[goods_label_count].set_pos( koord( 20, offset_below_chart+3+(BUTTON_SPACER+BUTTON_HEIGHT)*goods_label_count ) );
			goods_cont.add_komponente( goods_labels + goods_label_count );
			for(  int s=0;  s<MAX_FAB_GOODS_STAT;  ++s  ) {
				goods_chart.add_curve( goods_color[goods_label_count%MAX_GOODS_COLOR][s], output[g].get_stats(), MAX_FAB_GOODS_STAT, s, MAX_MONTH, false, false, true, 0, goods_convert[s] );
				goods_buttons[goods_button_count].init(button_t::box_state, output_type[s], koord(10+100+(BUTTON_SPACER+BUTTON_WIDTH)*s, offset_below_chart+(BUTTON_SPACER+BUTTON_HEIGHT)*goods_label_count), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
				goods_buttons[goods_button_count].background = goods_color[goods_label_count%MAX_GOODS_COLOR][s];
				goods_buttons[goods_button_count].pressed = false;
				goods_buttons[goods_button_count].add_listener(this);
				goods_cont.add_komponente( goods_buttons + goods_button_count );
				++goods_button_count;
			}
			++goods_label_count;
		}
	}
	goods_cont.add_komponente( &goods_chart );
	tab_panel.add_tab( &goods_cont, translator::translate("Goods") );

	// GUI components for other production-related statistics
	prod_chart.set_pos( koord(10 + 80, 10) );
	prod_chart.set_groesse( koord( CHART_WIDTH, CHART_HEIGHT ) );
	prod_chart.set_dimension(12, 10000);
	prod_chart.set_background(MN_GREY1);
	for(  int s=0;  s<MAX_FAB_STAT;  ++s  ) {
		prod_chart.add_curve( prod_color[s], factory->get_stats(), MAX_FAB_STAT, s, MAX_MONTH, false, false, true, 0, prod_convert[s] );
		prod_buttons[s].init(button_t::box_state, prod_type[s], koord(10+100+(BUTTON_SPACER+BUTTON_WIDTH)*button_pos[s].x, offset_below_chart+(BUTTON_SPACER+BUTTON_HEIGHT)*button_pos[s].y), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		prod_buttons[s].background = prod_color[s];
		prod_buttons[s].pressed = false;
		if(  s==FAB_BOOST_ELECTRIC  &&  factory->get_besch()->is_electricity_producer()  ) {
			prod_buttons[s].disable();
		}
		prod_buttons[s].add_listener(this);
		prod_cont.add_komponente( prod_buttons + s );
		if(  s==FAB_BOOST_MAIL  ) {
			// insert the reference line buttons here to ensure correct tab order
			for(  int r=0;  r<MAX_FAB_REF_LINE;  ++r  ) {
				prod_chart.add_line( ref_color[r], prod_ref_line_data + r, MAX_MONTH, false, true, 0, ref_convert[r] );
				prod_ref_line_buttons[r].init(button_t::box_state, prod_type[2+(r%3)], koord(10+100+(BUTTON_SPACER+BUTTON_WIDTH)*(r%3), offset_below_chart+(BUTTON_SPACER+BUTTON_HEIGHT)*(2+(r/3))), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
				prod_ref_line_buttons[r].background = ref_color[r];
				prod_ref_line_buttons[r].pressed = false;
				if(  (r==FAB_REF_MAX_BOOST_ELECTRIC  ||  r==FAB_REF_DEMAND_ELECTRIC)  &&  factory->get_besch()->is_electricity_producer()  ) {
					prod_ref_line_buttons[r].disable();
				}
				prod_ref_line_buttons[r].add_listener(this);
				prod_cont.add_komponente( prod_ref_line_buttons + r );
			}
		}
	}
	for(  int i=0;  i<MAX_PROD_LABEL;  ++i  ) {
		prod_labels[i].set_text( label_text[i] );
		prod_labels[i].set_pos( koord( 10+label_pos[i].x, offset_below_chart+3+label_pos[i].y ) );
		if(  i==1  ) {
			prod_labels[i].set_align(gui_label_t::centered);	// centre alignment for Power label
		}
		prod_cont.add_komponente( prod_labels + i );
	}
	prod_cont.add_komponente( &prod_chart );
	tab_panel.add_tab( &prod_cont, translator::translate("Production") );

	add_komponente( &tab_panel );
	const int max_rows = max( goods_label_count, button_pos[MAX_FAB_STAT-1].y+1 );
	const koord size( 20+80+CHART_WIDTH, gui_tab_panel_t::HEADER_VSIZE+20+CHART_HEIGHT+20+max_rows*BUTTON_HEIGHT+(max_rows-1)*BUTTON_SPACER );
	tab_panel.set_groesse( size );
	set_fenstergroesse( size + koord(0, 16) );
	set_resizemode(no_resize);

	// initialize reference lines' data (these do not change over time)
	prod_ref_line_data[FAB_REF_MAX_BOOST_ELECTRIC] = factory->get_besch()->get_electric_boost();
	prod_ref_line_data[FAB_REF_MAX_BOOST_PAX] = factory->get_besch()->get_pax_boost();
	prod_ref_line_data[FAB_REF_MAX_BOOST_MAIL] = factory->get_besch()->get_mail_boost();
}


factory_chart_t::~factory_chart_t()
{
	delete[] goods_buttons;
	delete[] goods_labels;
}


bool factory_chart_t::action_triggered(gui_action_creator_t *komp, value_t extra)
{
	if(  tab_panel.get_active_tab_index()==0  ) {
		// case : goods statistics' buttons
		for(  int b=0;  b<goods_button_count;  ++b  ) {
			if(  komp==&goods_buttons[b]  ) {
				goods_buttons[b].pressed = !goods_buttons[b].pressed;
				if(  goods_buttons[b].pressed  ) {
					goods_chart.show_curve(b);
				}
				else {
					goods_chart.hide_curve(b);
				}
				return true;
			}
		}
	}
	else {
		// first, check for buttons of other production-related statistics
		for(  int s=0;  s<MAX_FAB_STAT;  ++s  ) {
			if(  komp==&prod_buttons[s]  ) {
				prod_buttons[s].pressed = !prod_buttons[s].pressed;
				if(  prod_buttons[s].pressed  ) {
					prod_chart.show_curve(s);
				}
				else {
					prod_chart.hide_curve(s);
				}
				return true;
			}
		}
		// second, check for buttons of reference lines
		for(  int r=0;  r<MAX_FAB_REF_LINE;  ++r  ) {
			if(  komp==&prod_ref_line_buttons[r]  ) {
				prod_ref_line_buttons[r].pressed = !prod_ref_line_buttons[r].pressed;
				if(  prod_ref_line_buttons[r].pressed  ) {
					prod_chart.show_line(r);
				}
				else {
					prod_chart.hide_line(r);
				}
				return true;
			}
		}
	}
	return false;
}


void factory_chart_t::zeichnen(koord pos, koord gr)
{
	// update reference lines' data (these might change over time)
	prod_ref_line_data[FAB_REF_DEMAND_ELECTRIC] = ( factory->get_besch()->is_electricity_producer() ? 0 : factory->get_scaled_electric_amount() );
	prod_ref_line_data[FAB_REF_DEMAND_PAX] = factory->get_scaled_pax_demand();
	prod_ref_line_data[FAB_REF_DEMAND_MAIL] = factory->get_scaled_mail_demand();

	gui_frame_t::zeichnen(pos, gr);
}
