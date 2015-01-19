/*
 * Dialog window for defining a schedule
 *
 * Hj. Malthaner
 *
 * Juli 2000
 */

#include "../simline.h"
#include "../simcolor.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simworld.h"
#include "../simmenu.h"
#include "../simconvoi.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../boden/grund.h"

#include "../obj/zeiger.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../player/simplay.h"
#include "../vehicle/simvehicle.h"

#include "../tpl/vector_tpl.h"

#include "depot_frame.h"
#include "fahrplan_gui.h"
#include "line_item.h"

#include "components/gui_button.h"
#include "karte.h"



// shows/deletes highlighting of tiles
void fahrplan_gui_stats_t::highlight_schedule( schedule_t *markfpl, bool marking )
{
	marking &= env_t::visualize_schedule;
	FOR(minivec_tpl<linieneintrag_t>, const& i, markfpl->eintrag) {
		if (grund_t* const gr = welt->lookup(i.pos)) {
			for(  uint idx=0;  idx<gr->get_top();  idx++  ) {
				obj_t *obj = gr->obj_bei(idx);
				if(  marking  ) {
					if(  !obj->is_moving()  ) {
						obj->set_flag( obj_t::highlight );
					}
				}
				else {
					obj->clear_flag( obj_t::highlight );
				}
			}
			gr->set_flag( grund_t::dirty );
			// here on water
			if(  gr->ist_wasser()  ||  gr->ist_natur()  ) {
				if(  marking  ) {
					gr->set_flag( grund_t::marked );
				}
				else {
					gr->clear_flag( grund_t::marked );
				}
			}

		}
	}
	// always remove
	if(  grund_t *old_gr = welt->lookup(aktuell_mark->get_pos())  ) {
		aktuell_mark->mark_image_dirty( aktuell_mark->get_image(), 0 );
		old_gr->obj_remove( aktuell_mark );
		old_gr->set_flag( grund_t::dirty );
		aktuell_mark->set_pos( koord3d::invalid );
	}
	// add if required
	if(  marking  &&  markfpl->get_aktuell() < markfpl->get_count() ) {
		aktuell_mark->set_pos( markfpl->eintrag[markfpl->get_aktuell()].pos );
		if(  grund_t *gr = welt->lookup(aktuell_mark->get_pos())  ) {
			gr->obj_add( aktuell_mark );
			aktuell_mark->set_flag( obj_t::dirty );
			gr->set_flag( grund_t::dirty );
		}
	}
	aktuell_mark->clear_flag( obj_t::highlight );
}


/**
 * Append description of entry to buf.
 */
static void gimme_stop_name(cbuffer_t& buf, karte_t* welt, player_t const* const player_, linieneintrag_t const& entry)
{
	char const* what;
	halthandle_t const halt = haltestelle_t::get_halt(entry.pos, player_);
	if (halt.is_bound()) {
		if (entry.ladegrad != 0) {
			buf.printf("%d%% ", entry.ladegrad);
		}
		what = halt->get_name();
	}
	else {
		grund_t const* const gr = welt->lookup(entry.pos);
		if (!gr) {
			what = translator::translate("Invalid coordinate");
		}
		else if (gr->get_depot()) {
			what = translator::translate("Depot");
		}
		else if (char const* const label_text = gr->get_text()) {
			buf.printf("%s ", translator::translate("Wegpunkt"));
			what = label_text;
		}
		else {
			what = translator::translate("Wegpunkt");
		}
	}
	buf.printf("%s (%s)", what, entry.pos.get_str());
}


void fahrplan_gui_t::gimme_short_stop_name(cbuffer_t& buf, karte_t* welt, player_t const* const player_, linieneintrag_t const& entry, int const max_chars)
{
	const char *p;
	halthandle_t halt = haltestelle_t::get_halt(entry.pos, player_);
	if(halt.is_bound()) {
		p = halt->get_name();
	}
	else {
		const grund_t* gr = welt->lookup(entry.pos);
		if(gr==NULL) {
			p = translator::translate("Invalid coordinate");
		}
		else if(gr->get_depot() != NULL) {
			p = translator::translate("Depot");
		}
		else {
			p = translator::translate("Wegpunkt");
		}
	}
	// finally append
	if(strlen(p)>(unsigned)max_chars) {
		buf.printf("%.*s...", max_chars - 3, p);
	}
	else {
		buf.append(p);
	}
}



zeiger_t *fahrplan_gui_stats_t::aktuell_mark = NULL;
cbuffer_t fahrplan_gui_stats_t::buf;

void fahrplan_gui_stats_t::draw(scr_coord offset)
{
	if(  !fpl  ) {
		return;
	}

	if(  fpl->empty()  ) {
		buf.clear();
		buf.append(translator::translate("Please click on the map to add\nwaypoints or stops to this\nschedule."));
		sint16 const width = display_multiline_text(offset.x + 4, offset.y, buf, SYSCOL_TEXT_HIGHLIGHT );
		set_size(scr_size(width + 4 + 16, 3 * LINESPACE));
	}
	else {
		int    i     = 0;
		size_t sel   = fpl->get_aktuell();
		sint16 width = get_size().w - 16;
		FORX(minivec_tpl<linieneintrag_t>, const& e, fpl->eintrag, (--sel, offset.y += LINESPACE + 1)) {
			if (sel == 0) {
				// highlight current entry (width is just wide enough, scrolly will do clipping)
				display_fillbox_wh_clip(offset.x, offset.y - 1, 2048, LINESPACE + 1, player->get_player_color1() + 1, false);
			}

			buf.clear();
			buf.printf("%i) ", ++i);
			gimme_stop_name(buf, welt, player, e);
			PLAYER_COLOR_VAL const c = sel == 0 ? SYSCOL_TEXT_HIGHLIGHT : SYSCOL_TEXT;
			sint16           const w = display_proportional_clip(offset.x + 4 + 10, offset.y, buf, ALIGN_LEFT, c, true);
			if (width < w) {
				width = w;
			}

			// the goto button (right arrow)
			display_img_aligned( gui_theme_t::pos_button_img[ sel == 0 ], scr_rect( offset.x, offset.y, 14, LINESPACE ), ALIGN_CENTER_V | ALIGN_CENTER_H, true );
		}
		set_size(scr_size(width + 16, fpl->get_count() * (LINESPACE + 1)));
		highlight_schedule(fpl, true);
	}
}



fahrplan_gui_stats_t::fahrplan_gui_stats_t(player_t *player_)
{
	fpl = NULL;
	player = player_;
	if(  aktuell_mark==NULL  ) {
		aktuell_mark = new zeiger_t(koord3d::invalid, NULL );
		aktuell_mark->set_bild( tool_t::general_tool[TOOL_SCHEDULE_ADD]->cursor );
	}
}



fahrplan_gui_stats_t::~fahrplan_gui_stats_t()
{
	if(  grund_t *gr = welt->lookup(aktuell_mark->get_pos())  ) {
		aktuell_mark->mark_image_dirty( aktuell_mark->get_image(), 0 );
		gr->obj_remove(aktuell_mark);
	}
	aktuell_mark->set_pos( koord3d::invalid );
}



fahrplan_gui_t::~fahrplan_gui_t()
{
	if(  player  ) {
		update_tool( false );
		// hide schedule on minimap (may not current, but for safe)
		reliefkarte_t::get_karte()->set_current_cnv( convoihandle_t() );
	}
	delete fpl;

}



fahrplan_gui_t::fahrplan_gui_t(schedule_t* fpl_, player_t* player_, convoihandle_t cnv_) :
	gui_frame_t( translator::translate("Fahrplan"), player_),
	lb_line("Serves Line:"),
	lb_wait("month wait time"),
	lb_waitlevel(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
	lb_load("Full load"),
	stats(player_),
	scrolly(&stats),
	old_fpl(fpl_),
	player(player_),
	cnv(cnv_)
{
	old_fpl->eingabe_beginnen();
	fpl = old_fpl->copy();
	stats.set_fahrplan(fpl);
	if(  !cnv.is_bound()  ) {
		old_line = new_line = linehandle_t();
		show_line_selector(false);
	}
	else {
		// set this schedule as current to show on minimap if possible
		reliefkarte_t::get_karte()->set_current_cnv( cnv );
		old_line = new_line = cnv_->get_line();
	}
	old_line_count = 0;

	scr_coord_val ypos = 0;
	if(  cnv.is_bound()  ) {
		// things, only relevant to convois, like creating/selecting lines
		bt_promote_to_line.init( button_t::roundbox, "promote to line", scr_coord( BUTTON3_X, ypos ) );
		bt_promote_to_line.set_tooltip("Create a new line based on this schedule");
		bt_promote_to_line.add_listener(this);
		add_component(&bt_promote_to_line);

		lb_line.align_to( &bt_promote_to_line, ALIGN_CENTER_V, scr_coord( D_MARGIN_LEFT, 0 ) );
		add_component( &lb_line );

		ypos += D_BUTTON_HEIGHT+1;

		line_selector.set_pos(scr_coord(2, ypos));
		line_selector.set_size(scr_size(BUTTON4_X-2, D_BUTTON_HEIGHT));
		line_selector.set_max_size(scr_size(BUTTON4_X-2, 13*LINESPACE+D_TITLEBAR_HEIGHT-1));
		line_selector.set_highlight_color(player->get_player_color1() + 1);
		line_selector.clear_elements();

		init_line_selector();
		line_selector.add_listener(this);
		add_component(&line_selector);

		ypos += D_BUTTON_HEIGHT+3;
	}

	// loading level and return tickets
	scr_coord_val label_width = min( (D_BUTTON_WIDTH<<1) + D_H_SPACE, max( lb_load.get_size().w, lb_wait.get_size().w ) );

	numimp_load.set_pos( scr_coord( D_MARGIN_LEFT + label_width + D_H_SPACE, ypos ) );
	numimp_load.set_width( 60 );
	numimp_load.set_value( fpl->get_current_eintrag().ladegrad );
	numimp_load.set_limits( 0, 100 );
	numimp_load.set_increment_mode( gui_numberinput_t::PROGRESS );
	numimp_load.add_listener(this);
	add_component(&numimp_load);

	lb_load.set_width( label_width );
	lb_load.align_to( &numimp_load, ALIGN_CENTER_V, scr_coord( D_MARGIN_LEFT, 0 ) );
	add_component( &lb_load );

	ypos += numimp_load.get_size().h;

	if(  fpl->get_current_eintrag().waiting_time_shift==0  ) {
		strcpy( str_parts_month, translator::translate("off") );
	}
	else {
		sprintf( str_parts_month, "1/%d",  1<<(16-fpl->get_current_eintrag().waiting_time_shift) );
	}
	lb_waitlevel.set_text_pointer( str_parts_month );
	lb_waitlevel.set_size( numimp_load.get_size() - scr_size( D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH , 0 ) );
	lb_waitlevel.align_to( &numimp_load, ALIGN_EXTERIOR_V | ALIGN_TOP | ALIGN_LEFT, scr_coord( gui_theme_t::gui_arrow_left_size.w, 0 ) );
	add_component(&lb_waitlevel);

	// waiting in parts per month
	bt_wait_prev.set_typ( button_t::arrowleft );
	bt_wait_prev.align_to( &lb_waitlevel, ALIGN_EXTERIOR_H | ALIGN_RIGHT | ALIGN_CENTER_V );
	bt_wait_prev.add_listener(this);
	add_component( &bt_wait_prev );

	bt_wait_next.set_typ( button_t::arrowright );
	bt_wait_next.align_to( &lb_waitlevel, ALIGN_EXTERIOR_H | ALIGN_LEFT | ALIGN_CENTER_V );
	bt_wait_next.add_listener(this);
	lb_waitlevel.set_width( bt_wait_next.get_pos().x-bt_wait_prev.get_pos().x-bt_wait_prev.get_size().w );
	add_component( &bt_wait_next );

	lb_wait.set_width( label_width );
	lb_wait.align_to( &lb_waitlevel, ALIGN_CENTER_V, scr_coord( D_MARGIN_LEFT, 0 ) );
	add_component( &lb_wait );

	if(  !env_t::hide_rail_return_ticket  ||  fpl->get_waytype()==road_wt  ||  fpl->get_waytype()==air_wt  ||  fpl->get_waytype()==water_wt  ) {
		//  hide the return ticket on rail stuff, where it causes much trouble
		bt_return.init(button_t::roundbox, "return ticket", scr_coord(BUTTON3_X, ypos ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
		bt_return.set_tooltip("Add stops for backward travel");
		bt_return.add_listener(this);
		add_component(&bt_return);
	}

	ypos += lb_waitlevel.get_size().h;

	bt_add.init(button_t::roundbox_state, "Add Stop", scr_coord(BUTTON1_X, ypos ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	bt_add.set_tooltip("Appends stops at the end of the schedule");
	bt_add.add_listener(this);
	bt_add.pressed = true;
	add_component(&bt_add);

	bt_insert.init(button_t::roundbox_state, "Ins Stop", scr_coord(BUTTON2_X, ypos ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	bt_insert.set_tooltip("Insert stop before the current stop");
	bt_insert.add_listener(this);
	bt_insert.pressed = false;
	add_component(&bt_insert);

	bt_remove.init(button_t::roundbox_state, "Del Stop", scr_coord(BUTTON3_X, ypos ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT) );
	bt_remove.set_tooltip("Delete the current stop");
	bt_remove.add_listener(this);
	bt_remove.pressed = false;
	add_component(&bt_remove);

	ypos += D_BUTTON_HEIGHT+2;

	scrolly.set_pos( scr_coord( 0, ypos ) );
	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly);

	mode = adding;
	update_selection();

	set_windowsize( scr_size(BUTTON4_X, ypos+D_BUTTON_HEIGHT+(fpl->get_count()>0 ? min(15,fpl->get_count()) : 15)*(LINESPACE+1)+D_TITLEBAR_HEIGHT) );
	set_min_windowsize( scr_size(BUTTON4_X, ypos+D_BUTTON_HEIGHT+3*(LINESPACE+1)+D_TITLEBAR_HEIGHT) );

	set_resizemode(diagonal_resize);
	resize( scr_coord(0,0) );
}


void fahrplan_gui_t::update_tool(bool set)
{
	if(!set  ||  mode==removing  ||  mode==undefined_mode) {
		// reset tools, if still selected ...
		if(welt->get_tool(player->get_player_nr())==tool_t::general_tool[TOOL_SCHEDULE_ADD]) {
			if(tool_t::general_tool[TOOL_SCHEDULE_ADD]->get_default_param()==(const char *)fpl) {
				welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
			}
		}
		else if(welt->get_tool(player->get_player_nr())==tool_t::general_tool[TOOL_SCHEDULE_INS]) {
			if(tool_t::general_tool[TOOL_SCHEDULE_INS]->get_default_param()==(const char *)fpl) {
				welt->set_tool( tool_t::general_tool[TOOL_QUERY], player );
			}
		}
	}
	else {
		//  .. or set them again
		if(mode==adding) {
			tool_t::general_tool[TOOL_SCHEDULE_ADD]->set_default_param((const char *)fpl);
			welt->set_tool( tool_t::general_tool[TOOL_SCHEDULE_ADD], player );
		}
		else if(mode==inserting) {
			tool_t::general_tool[TOOL_SCHEDULE_INS]->set_default_param((const char *)fpl);
			welt->set_tool( tool_t::general_tool[TOOL_SCHEDULE_INS], player );
		}
	}
}


void fahrplan_gui_t::update_selection()
{
	bt_wait_prev.disable();
	lb_wait.set_color( COL_GREY3 );
	strcpy( str_parts_month, translator::translate("off") );
	lb_waitlevel.set_color( COL_GREY3 );
	bt_wait_next.disable();

	if(  !fpl->empty()  ) {
		fpl->set_aktuell( min(fpl->get_count()-1,fpl->get_aktuell()) );
		const uint8 aktuell = fpl->get_aktuell();
		if(  haltestelle_t::get_halt(fpl->eintrag[aktuell].pos, player).is_bound()  ) {
			lb_load.set_color( SYSCOL_TEXT );
			numimp_load.enable();
			numimp_load.set_value( fpl->eintrag[aktuell].ladegrad );
			if(  fpl->eintrag[aktuell].ladegrad>0  ) {
				bt_wait_prev.enable();
				lb_wait.set_color( SYSCOL_TEXT );
				if(  fpl->eintrag[aktuell].waiting_time_shift>0  ) {
					sprintf( str_parts_month, "1/%d",  1<<(16-fpl->eintrag[aktuell].waiting_time_shift) );
				}
				lb_waitlevel.set_color( SYSCOL_TEXT_HIGHLIGHT );
				bt_wait_next.enable();
			}
		}
		else {
			lb_load.set_color( COL_GREY3 );
			numimp_load.disable();
			numimp_load.set_value( 0 );
		}
	}
}


/**
 * Mouse clicks are hereby reported to its GUI-Components
 */
bool fahrplan_gui_t::infowin_event(const event_t *ev)
{
	if( (ev)->ev_class == EVENT_CLICK  &&  !((ev)->ev_code==MOUSE_WHEELUP  ||  (ev)->ev_code==MOUSE_WHEELDOWN)  &&  !line_selector.getroffen(ev->cx, ev->cy-16)  )  {

		// close combo box; we must do it ourselves, since the box does not receive outside events ...
		line_selector.close_box();

		if(  ev->my>=scrolly.get_pos().y+16  ) {
			// we are now in the multiline region ...
			const int line = ( ev->my - scrolly.get_pos().y + scrolly.get_scroll_y() - 16)/(LINESPACE+1);

			if(  line >= 0 && line < fpl->get_count()  ) {
				if(  IS_RIGHTCLICK(ev)  ||  ev->mx<16  ) {
					// just center on it
					welt->get_viewport()->change_world_position( fpl->eintrag[line].pos );
				}
				else if(  ev->mx<scrolly.get_size().w-11  ) {
					fpl->set_aktuell( line );
					if(  mode == removing  ) {
						stats.highlight_schedule( fpl, false );
						fpl->remove();
						action_triggered( &bt_add, value_t() );
					}
					update_selection();
				}
			}
		}
	}
	else if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  &&  fpl!=NULL  ) {

		for(  int i=0;  i<fpl->get_count();  i++  ) {
			stats.highlight_schedule( fpl, false );
		}

		update_tool( false );
		fpl->cleanup();
		fpl->eingabe_abschliessen();
		// now apply the changes
		if(  cnv.is_bound()  ) {
			// do not send changes if the convoi is about to be deleted
			if(  cnv->get_state() != convoi_t::SELF_DESTRUCT  ) {
				// if a line is selected
				if(  new_line.is_bound()  ) {
					// if the selected line is different to the convoi's line, apply it
					if(  new_line!=cnv->get_line()  ) {
						char id[16];
						sprintf( id, "%i,%i", new_line.get_id(), fpl->get_aktuell() );
						cnv->call_convoi_tool( 'l', id );
					}
					else {
						cbuffer_t buf;
						fpl->sprintf_schedule( buf );
						cnv->call_convoi_tool( 'g', buf );
					}
				}
				else {
					cbuffer_t buf;
					fpl->sprintf_schedule( buf );
					cnv->call_convoi_tool( 'g', buf );
				}

				if(  cnv->in_depot()  ) {
					const grund_t *const ground = welt->lookup( cnv->get_home_depot() );
					if(  ground  ) {
						const depot_t *const depot = ground->get_depot();
						if(  depot  ) {
							depot_frame_t *const frame = dynamic_cast<depot_frame_t *>( win_get_magic( (ptrdiff_t)depot ) );
							if(  frame  ) {
								frame->update_data();
							}
						}
					}
				}
			}
		}
	}
	else if(  ev->ev_class == INFOWIN  &&  (ev->ev_code == WIN_TOP  ||  ev->ev_code == WIN_OPEN)  &&  fpl!=NULL  ) {
		// just to be sure, renew the tools ...
		update_tool( true );
	}

	return gui_frame_t::infowin_event(ev);
}


bool fahrplan_gui_t::action_triggered( gui_action_creator_t *komp, value_t p)
{
DBG_MESSAGE("fahrplan_gui_t::action_triggered()","komp=%p combo=%p",komp,&line_selector);

	if(komp == &bt_add) {
		mode = adding;
		bt_add.pressed = true;
		bt_insert.pressed = false;
		bt_remove.pressed = false;
		update_tool( true );
	}
	else if(komp == &bt_insert) {
		mode = inserting;
		bt_add.pressed = false;
		bt_insert.pressed = true;
		bt_remove.pressed = false;
		update_tool( true );
	}
	else if(komp == &bt_remove) {
		mode = removing;
		bt_add.pressed = false;
		bt_insert.pressed = false;
		bt_remove.pressed = true;
		update_tool( false );
	}
	else if(komp == &numimp_load) {
		if (!fpl->empty()) {
			fpl->eintrag[fpl->get_aktuell()].ladegrad = (uint8)p.i;
			update_selection();
		}
	}
	else if(komp == &bt_wait_prev) {
		if(!fpl->empty()) {
			sint8& wait = fpl->eintrag[fpl->get_aktuell()].waiting_time_shift;
			if(wait>7) {
				wait --;
			}
			else if(  wait>0  ) {
				wait = 0;
			}
			else {
				wait = 16;
			}
			update_selection();
		}
	}
	else if(komp == &bt_wait_next) {
		if(!fpl->empty()) {
			sint8& wait = fpl->eintrag[fpl->get_aktuell()].waiting_time_shift;
			if(wait==0) {
				wait = 7;
			}
			else if(wait<16) {
				wait ++;
			}
			else {
				wait = 0;
			}
			update_selection();
		}
	}
	else if(komp == &bt_return) {
		fpl->add_return_way();
	}
	else if(komp == &line_selector) {
		uint32 selection = p.i - !new_line.is_bound();
//DBG_MESSAGE("fahrplan_gui_t::action_triggered()","line selection=%i",selection);
		if(  line_scrollitem_t *li = dynamic_cast<line_scrollitem_t*>(line_selector.get_element(selection))  ) {
			new_line = li->get_line();
			stats.highlight_schedule( fpl, false );
			fpl->copy_from( new_line->get_schedule() );
			fpl->eingabe_beginnen();
		}
		else {
			// remove line
			new_line = linehandle_t();
			line_selector.set_selection( 0 );
		}
	}
	else if(komp == &bt_promote_to_line) {
		// update line schedule via tool!
		tool_t *tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
		cbuffer_t buf;
		buf.printf( "c,0,%i,%ld,", (int)fpl->get_type(), (long)old_fpl );
		fpl->sprintf_schedule( buf );
		tool->set_default_param(buf);
		welt->set_tool( tool, player );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
	// recheck lines
	if(  cnv.is_bound()  ) {
		// unequal to line => remove from line ...
		if(  new_line.is_bound()  &&  !fpl->matches(welt,new_line->get_schedule())  ) {
			new_line = linehandle_t();
			line_selector.set_selection(0);
		}
		// only assign old line, when new_line is not equal
		if(  !new_line.is_bound()  &&  old_line.is_bound()  &&   fpl->matches(welt,old_line->get_schedule())  ) {
			new_line = old_line;
			init_line_selector();
		}
	}
	return true;
}


void fahrplan_gui_t::init_line_selector()
{
	line_selector.clear_elements();
	int selection = 0;
	vector_tpl<linehandle_t> lines;

	player->simlinemgmt.get_lines(fpl->get_type(), &lines);

	// keep assignment with identical schedules
	if(  new_line.is_bound()  &&  !fpl->matches( welt, new_line->get_schedule() )  ) {
		if(  old_line.is_bound()  &&  fpl->matches( welt, old_line->get_schedule() )  ) {
			new_line = old_line;
		}
		else {
			new_line = linehandle_t();
		}
	}
	if(  !new_line.is_bound()  ) {
		selection = 0;
		line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate("<no line>"), SYSCOL_TEXT ) );
	}

	FOR(  vector_tpl<linehandle_t>,  line,  lines  ) {
		line_selector.append_element( new line_scrollitem_t(line) );
		if(  !new_line.is_bound()  ) {
			if(  fpl->matches( welt, line->get_schedule() )  ) {
				selection = line_selector.count_elements()-1;
				new_line = line;
			}
		}
		else if(  new_line == line  ) {
			selection = line_selector.count_elements()-1;
		}
	}

	line_selector.set_selection( selection );
	line_scrollitem_t::sort_mode = line_scrollitem_t::SORT_BY_NAME;
	line_selector.sort( 1, NULL );
	old_line_count = player->simlinemgmt.get_line_count();
	last_schedule_count = fpl->get_count();
}



void fahrplan_gui_t::draw(scr_coord pos, scr_size size)
{
	if(  player->simlinemgmt.get_line_count()!=old_line_count  ||  last_schedule_count!=fpl->get_count()  ) {
		// lines added or deleted
		init_line_selector();
		last_schedule_count = fpl->get_count();
	}

	// after loading in network games, the schedule might still being updated
	if(  cnv.is_bound()  &&  cnv->get_state()==convoi_t::FAHRPLANEINGABE  &&  fpl->ist_abgeschlossen()  ) {
		assert( convoi_t::FAHRPLANEINGABE==1 ); // convoi_t::FAHRPLANEINGABE is 1
		fpl->eingabe_beginnen();
		cnv->call_convoi_tool( 's', "1" );
	}

	// always dirty, to cater for shortening of halt names and change of selections
	set_dirty();
	gui_frame_t::draw(pos,size);
}



/**
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void fahrplan_gui_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);

	size = get_windowsize()-scr_size(0,16+1);
	scrolly.set_size(size-scr_size(0,scrolly.get_pos().y));

	line_selector.set_max_size(scr_size(BUTTON4_X-2, size.h-line_selector.get_pos().y -16-1));
}


void fahrplan_gui_t::map_rotate90( sint16 y_size)
{
	fpl->rotate90(y_size);
}


fahrplan_gui_t::fahrplan_gui_t():
gui_frame_t( translator::translate("Fahrplan"), NULL),
	lb_line("Serves Line:"),
	lb_wait("month wait time"),
	lb_waitlevel(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
	lb_load("Full load"),
	stats(NULL),
	scrolly(&stats),
	fpl(NULL),
	old_fpl(NULL),
	player(NULL),
	cnv()
{
	// just a dummy
}


void fahrplan_gui_t::rdwr(loadsave_t *file)
{
	// this handles only schedules of bound convois
	// lines are handled by line_management_gui_t

	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	// convoy data
	if (file->get_version() <=112002) {
		// dummy data
		uint8 player_nr = 0;
		koord3d cnv_pos( koord3d::invalid);
		char name[128];
		name[0] = 0;
		file->rdwr_byte( player_nr );
		file->rdwr_str( name, lengthof(name) );
		cnv_pos.rdwr( file );
	}
	else {
		// handle
		convoi_t::rdwr_convoihandle_t(file, cnv);
	}

	// schedules
	if(  file->is_loading()  ) {
		// dummy types
		old_fpl = new autofahrplan_t();
		fpl = new autofahrplan_t();
	}
	old_fpl->rdwr(file);
	fpl->rdwr(file);

	if(  file->is_loading()  ) {
		if(  cnv.is_bound() ) {
			// now we can open the window ...
			scr_coord const& pos = win_get_pos(this);
			fahrplan_gui_t *w = new fahrplan_gui_t( cnv->get_schedule(), cnv->get_owner(), cnv );
			create_win(pos.x, pos.y, w, w_info, (ptrdiff_t)cnv->get_schedule());
			w->set_windowsize( size );
			w->fpl->copy_from( fpl );
			cnv->get_schedule()->eingabe_abschliessen();
			w->fpl->eingabe_abschliessen();
		}
		else {
			dbg->error( "fahrplan_gui_t::rdwr", "Could not restore schedule window for (%d)", cnv.get_id() );
		}
		player = NULL;
		delete old_fpl;
		delete fpl;
		fpl = old_fpl = NULL;
		cnv = convoihandle_t();
		destroy_win( this );
	}
}
