/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include "simdebug.h"
#include "simobj.h"
#include "simfab.h"
#include "display/simgraph.h"
#include "simmenu.h"
#include "simplan.h"
#include "simwerkz.h"
#include "simworld.h"
#include "simhalt.h"
#include "player/simplay.h"
#include "simconst.h"
#include "macros.h"
#include "besch/grund_besch.h"
#include "boden/grund.h"
#include "boden/boden.h"
#include "boden/fundament.h"
#include "boden/wasser.h"
#include "boden/tunnelboden.h"
#include "boden/brueckenboden.h"
#include "boden/monorailboden.h"

#include "obj/gebaeude.h"

#include "dataobj/loadsave.h"
#include "dataobj/environment.h"

#include "gui/karte.h"


karte_ptr_t planquadrat_t::welt;


void swap(planquadrat_t& a, planquadrat_t& b)
{
	sim::swap(a.halt_list, b.halt_list);
	sim::swap(a.ground_size, b.ground_size);
	sim::swap(a.halt_list_count, b.halt_list_count);
	sim::swap(a.data, b.data);
	sim::swap(a.climate_data, b.climate_data);
}

// deletes also all grounds in this array!
planquadrat_t::~planquadrat_t()
{
	if(ground_size==0) {
		// empty
	}
	else if(ground_size==1) {
		delete data.one;
	}
	else {
		while(ground_size>0) {
			ground_size --;
			delete data.some[ground_size];
			data.some[ground_size] = 0;
		}
		delete [] data.some;
	}
	delete [] halt_list;
	halt_list_count = 0;
	// to avoid access to this tile
	ground_size = 0;
	data.one = NULL;
}


grund_t *planquadrat_t::get_boden_von_obj(obj_t *obj) const
{
	if(ground_size==1) {
		if(data.one  &&  data.one->obj_ist_da(obj)) {
			return data.one;
		}
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]->obj_ist_da(obj)) {
				return data.some[i];
			}
		}
	}
	return NULL;
}


void planquadrat_t::boden_hinzufuegen(grund_t *bd)
{
	assert(!bd->ist_karten_boden());
	if(ground_size==0) {
		// completely empty
		data.one = bd;
		ground_size = 1;
		reliefkarte_t::get_karte()->calc_map_pixel(bd->get_pos().get_2d());
		return;
	}
	else if(ground_size==1) {
		// needs to convert to array
//	assert(data.one->get_hoehe()!=bd->get_hoehe());
		if(data.one->get_hoehe()==bd->get_hoehe()) {
DBG_MESSAGE("planquadrat_t::boden_hinzufuegen()","addition ground %s at (%i,%i,%i) will be ignored!",bd->get_name(),bd->get_pos().x,bd->get_pos().y,bd->get_pos().z);
			return;
		}
		grund_t **tmp = new grund_t *[2];
		tmp[0] = data.one;
		tmp[1] = bd;
		data.some = tmp;
		ground_size = 2;
		reliefkarte_t::get_karte()->calc_map_pixel(bd->get_pos().get_2d());
		return;
	}
	else {
		// insert into array
		uint8 i;
		for(i=1;  i<ground_size;  i++) {
			if(data.some[i]->get_hoehe()>=bd->get_hoehe()) {
				break;
			}
		}
		if(i<ground_size  &&  data.some[i]->get_hoehe()==bd->get_hoehe()) {
DBG_MESSAGE("planquadrat_t::boden_hinzufuegen()","addition ground %s at (%i,%i,%i) will be ignored!",bd->get_name(),bd->get_pos().x,bd->get_pos().y,bd->get_pos().z);
			return;
		}
		bd->set_kartenboden(false);
		// extend array if needed
		grund_t **tmp = new grund_t *[ground_size+1];
		for( uint8 j=0;  j<i;  j++  ) {
			tmp[j] = data.some[j];
		}
		tmp[i] = bd;
		while(i<ground_size) {
			tmp[i+1] = data.some[i];
			i++;
		}
		ground_size ++;
		delete [] data.some;
		data.some = tmp;
		reliefkarte_t::get_karte()->calc_map_pixel(bd->get_pos().get_2d());
	}
}


bool planquadrat_t::boden_entfernen(grund_t *bd)
{
	assert(!bd->ist_karten_boden()  &&  ground_size>0);
	if(ground_size==1) {
		ground_size = 0;
		data.one = NULL;
		return true;
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]==bd) {
				// found
				while(i<ground_size-1) {
					data.some[i] = data.some[i+1];
					i++;
				}
				ground_size --;
				// below 2?
				if(ground_size==1) {
					grund_t *tmp=data.some[0];
					delete [] data.some;
					data.one = tmp;
				}
				return true;
			}
		}
	}
	assert(0);
	return false;
}


void planquadrat_t::kartenboden_setzen(grund_t *bd)
{
	assert(bd);
	grund_t *tmp = get_kartenboden();
	if(tmp) {
		boden_ersetzen(tmp,bd);
	}
	else {
		data.one = bd;
		ground_size = 1;
		bd->set_kartenboden(true);
	}
	bd->calc_bild();
	reliefkarte_t::get_karte()->calc_map_pixel(bd->get_pos().get_2d());
}


/**
 * replaces the map solid ground (or water) and deletes the old one
 * @author Hansj�rg Malthaner
 */
void planquadrat_t::boden_ersetzen(grund_t *alt, grund_t *neu)
{
	assert(alt!=NULL  &&  neu!=NULL  &&  !alt->is_halt()  );

	if(ground_size<=1) {
		assert(data.one==alt  ||  ground_size==0);
		data.one = neu;
		ground_size = 1;
		neu->set_kartenboden(true);
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			if(data.some[i]==alt) {
				data.some[i] = neu;
				neu->set_kartenboden(i==0);
				break;
			}
		}
	}
	// transfer text and delete
	if(alt) {
		if(alt->get_flag(grund_t::has_text)) {
			neu->set_flag(grund_t::has_text);
			alt->clear_flag(grund_t::has_text);
		}
		// transfer all objects
		while(  alt->get_top()>0  ) {
			neu->obj_add( alt->obj_remove_top() );
		}
		delete alt;
	}
}


void planquadrat_t::rdwr(loadsave_t *file, koord pos )
{
	xml_tag_t p( file, "planquadrat_t" );

	if(file->is_saving()) {
		if(ground_size==1) {
			file->wr_obj_id(data.one->get_typ());
			data.one->rdwr(file);
		}
		else {
			for(int i=0; i<ground_size; i++) {
				file->wr_obj_id(data.some[i]->get_typ());
				data.some[i]->rdwr(file);
			}
		}
		file->wr_obj_id(-1);
	}
	else {
		grund_t *gr;
		sint8 hgt = welt->get_grundwasser();
		//DBG_DEBUG("planquadrat_t::rdwr()","Reading boden");
		do {
			short gtyp = file->rd_obj_id();

			switch(gtyp) {
				case -1: gr = NULL; break;
				case grund_t::boden:	    gr = new boden_t(file, pos);                 break;
				case grund_t::wasser:	    gr = new wasser_t(file, pos);                break;
				case grund_t::fundament:	    gr = new fundament_t(file, pos);	       break;
				case grund_t::tunnelboden:	    gr = new tunnelboden_t(file, pos);       break;
				case grund_t::brueckenboden:    gr = new brueckenboden_t(file, pos);     break;
				case grund_t::monorailboden:	    gr = new monorailboden_t(file, pos); break;
				default:
					gr = 0; // Hajo: keep compiler happy, fatal() never returns
					dbg->fatal("planquadrat_t::rdwr()","Error while loading game: Unknown ground type '%d'",gtyp);
			}
			// check if we have a matching building here, otherwise set to nothing
			if (gr  &&  gtyp == grund_t::fundament  &&  gr->find<gebaeude_t>() == NULL) {
				koord3d pos = gr->get_pos();
				// show normal ground here
				grund_t *neu = new boden_t(pos, 0);
				if(gr->get_flag(grund_t::has_text)) {
					neu->set_flag(grund_t::has_text);
					gr->clear_flag(grund_t::has_text);
				}
				// transfer all objects
				while(  gr->get_top()>0  ) {
					neu->obj_add( gr->obj_remove_top() );
				}
				delete gr;
				gr = neu;
//DBG_MESSAGE("planquadrat_t::rwdr", "unknown building (or prepare for factory) at %d,%d replaced by normal ground!", pos.x,pos.y);
			}
			// we should also check for ground below factories
			if(gr) {
				if(ground_size==0) {
					data.one = gr;
					ground_size = 1;
					gr->set_kartenboden(true);
					hgt = welt->lookup_hgt(pos);
				}
				else {
					// other ground must not reset the height
					boden_hinzufuegen(gr);
					welt->set_grid_hgt( pos, hgt );
				}
			}
		} while(gr != 0);
	}
}


// start a new month (an change seasons)
void planquadrat_t::check_season(const long month)
{
	if(ground_size==0) {
	}
	else if(ground_size==1) {
		data.one->check_season(month);
	}
	else {
		for(uint8 i=0;  i<ground_size;  i++) {
			data.some[i]->check_season(month);
		}
	}
}


void planquadrat_t::correct_water()
{
	grund_t *gr = get_kartenboden();
	hang_t::typ slope = gr->get_grund_hang();
	sint8 max_height = gr->get_hoehe() + hang_t::max_diff( slope );
	koord k = gr->get_pos().get_2d();
	sint8 water_hgt = welt->get_water_hgt(k);
	if(  gr  &&  gr->get_typ() != grund_t::wasser  &&  max_height <= water_hgt  ) {
		// below water but ground => convert
		kartenboden_setzen( new wasser_t(koord3d( k, water_hgt ) ) );
	}
	else if(  gr  &&  gr->get_typ() == grund_t::wasser  &&  max_height > water_hgt  ) {
		// water above ground => to ground
		kartenboden_setzen( new boden_t(gr->get_pos(), gr->get_disp_slope() ) );
	}
	else if(  gr  &&  gr->get_typ() == grund_t::wasser  &&  gr->get_hoehe() != water_hgt  ) {
		// water at wrong height
		gr->set_hoehe( water_hgt );
	}

	gr = get_kartenboden();
	if(  gr  &&  gr->get_typ() != grund_t::wasser  &&  gr->get_disp_height() < water_hgt  &&  welt->max_hgt(k) > water_hgt  ) {
		sint8 disp_hneu = water_hgt;
		sint8 disp_hn_sw = max( gr->get_hoehe() + corner1(slope), water_hgt );
		sint8 disp_hn_se = max( gr->get_hoehe() + corner2(slope), water_hgt );
		sint8 disp_hn_ne = max( gr->get_hoehe() + corner3(slope), water_hgt );
		sint8 disp_hn_nw = max( gr->get_hoehe() + corner4(slope), water_hgt );
		const uint8 sneu = (disp_hn_sw - disp_hneu) + ((disp_hn_se - disp_hneu) * 3) + ((disp_hn_ne - disp_hneu) * 9) + ((disp_hn_nw - disp_hneu) * 27);
		gr->set_hoehe( disp_hneu );
		gr->set_grund_hang( (hang_t::typ)sneu );
	}
}


void planquadrat_t::abgesenkt()
{
	grund_t *gr = get_kartenboden();
	if(gr) {
		const uint8 slope = gr->get_grund_hang();

		gr->obj_loesche_alle(NULL);
		sint8 max_hgt = gr->get_hoehe() + (slope != 0 ? (slope & 7 ? 1 : 2) : 0);

		koord k(gr->get_pos().get_2d());
		if(  max_hgt <= welt->get_water_hgt( k )  &&  gr->get_typ() != grund_t::wasser  ) {
			gr = new wasser_t(gr->get_pos());
			kartenboden_setzen( gr );
			// recalc water ribis of neighbors
			for(int r=0; r<4; r++) {
				grund_t *gr2 = welt->lookup_kartenboden(k + koord::nsow[r]);
				if (gr2  &&  gr2->ist_wasser()) {
					gr2->calc_bild();
				}
			}
		}
		else {
			reliefkarte_t::get_karte()->calc_map_pixel(k);
		}
		gr->set_grund_hang( slope );
	}
}


void planquadrat_t::angehoben()
{
	grund_t *gr = get_kartenboden();
	if(gr) {
		const uint8 slope = gr->get_grund_hang();

		gr->obj_loesche_alle(NULL);
		sint8 max_hgt = gr->get_hoehe() + (slope != 0 ? (slope & 7 ? 1 : 2) : 0);

		koord k(gr->get_pos().get_2d());
		if(  max_hgt > welt->get_water_hgt( k )  &&  gr->get_typ() == grund_t::wasser  ) {
			gr = new boden_t(gr->get_pos(), slope );
			kartenboden_setzen( gr );
			// recalc water ribis
			for(int r=0; r<4; r++) {
				grund_t *gr2 = welt->lookup_kartenboden(k + koord::nsow[r]);
				if (gr2  &&  gr2->ist_wasser()) {
					gr2->calc_bild();
				}
			}
		}
		else {
			reliefkarte_t::get_karte()->calc_map_pixel(k);
		}
	}
}


#ifdef MULTI_THREAD
void planquadrat_t::display_obj(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, bool is_global, const sint8 hmin, const sint8 hmax, const sint8 clip_num) const
#else
void planquadrat_t::display_obj(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, bool is_global, const sint8 hmin, const sint8 hmax) const
#endif
{
	grund_t *gr0 = get_kartenboden();
	const sint8 h0 = gr0->get_disp_height();
	uint8 i = 1;
	// underground
	if(  hmin < h0  ) {
		for(  ;  i < ground_size;  i++  ) {
			const grund_t* gr = data.some[i];
			const sint8 h = gr->get_hoehe();
			const hang_t::typ slope = gr->get_grund_hang();
			const sint8 htop = h + max(max(corner1(slope), corner2(slope)),max(corner3(slope), corner4(slope)));
			// above ground
			if(  h > h0  ) {
				break;
			}
			// not too low?
			if(  htop >= hmin  ) {
				const sint16 yypos = ypos - tile_raster_scale_y( (h - h0) * TILE_HEIGHT_STEP, raster_tile_width );
#ifdef MULTI_THREAD
				gr->display_boden( xpos, yypos, raster_tile_width, clip_num );
				gr->display_obj_all( xpos, yypos, raster_tile_width, is_global, clip_num );
#else
				gr->display_boden( xpos, yypos, raster_tile_width );
				gr->display_obj_all( xpos, yypos, raster_tile_width, is_global );
#endif
			}
		}
	}
	//const bool kartenboden_dirty = gr->get_flag(grund_t::dirty);
	if(  gr0->get_flag( grund_t::draw_as_obj )  ||  !gr0->is_karten_boden_visible()  ) {
#ifdef MULTI_THREAD
		gr0->display_boden( xpos, ypos, raster_tile_width, clip_num );
#else
		gr0->display_boden( xpos, ypos, raster_tile_width );
#endif
	}

	if(  env_t::simple_drawing  ) {
		// ignore trees going though bridges
#ifdef MULTI_THREAD
		gr0->display_obj_all_quick_and_dirty( xpos, ypos, raster_tile_width, is_global, clip_num );
#else
		gr0->display_obj_all_quick_and_dirty( xpos, ypos, raster_tile_width, is_global );
#endif
	}
	else {
		// clip everything at the next tile above
		clip_dimension p_cr;
		if(  i < ground_size  ) {
#ifdef MULTI_THREAD
			p_cr = display_get_clip_wh_cl( clip_num );
#else
			p_cr = display_get_clip_wh();
#endif
			for(  uint8 j = i;  j < ground_size;  j++  ) {
				const sint8 h = data.some[j]->get_hoehe();
				const hang_t::typ slope = data.some[j]->get_grund_hang();
				const sint8 htop = h + max(max(corner1(slope), corner2(slope)),max(corner3(slope), corner4(slope)));
				// too high?
				if(  h > hmax  ) {
					break;
				}
				// not too low?
				if(  htop >= hmin  ) {
					// something on top: clip horizontally to prevent trees etc shining trough bridges
					const sint16 yh = ypos - tile_raster_scale_y( (h - h0) * TILE_HEIGHT_STEP, raster_tile_width ) + ((3 * raster_tile_width) >> 2);
					if(  yh >= p_cr.y  ) {
#ifdef MULTI_THREAD
						display_set_clip_wh_cl(p_cr.x, yh, p_cr.w, p_cr.h + p_cr.y - yh, clip_num  );
#else
						display_set_clip_wh( p_cr.x, yh, p_cr.w, p_cr.h + p_cr.y - yh );
#endif
					}
					break;
				}
			}
#ifdef MULTI_THREAD
			gr0->display_obj_all( xpos, ypos, raster_tile_width, is_global, clip_num );
			display_set_clip_wh_cl( p_cr.x, p_cr.y, p_cr.w, p_cr.h, clip_num ); // restore clipping
#else
			gr0->display_obj_all( xpos, ypos, raster_tile_width, is_global );
			display_set_clip_wh( p_cr.x, p_cr.y, p_cr.w, p_cr.h ); // restore clipping
#endif
		}
		else {
#ifdef MULTI_THREAD
			gr0->display_obj_all( xpos, ypos, raster_tile_width, is_global, clip_num );
#else
			gr0->display_obj_all( xpos, ypos, raster_tile_width, is_global );
#endif
		}
	}
	// above ground
	for(  ;  i < ground_size;  i++  ) {
		const grund_t* gr = data.some[i];
		const sint8 h = gr->get_hoehe();
		const hang_t::typ slope = gr->get_grund_hang();
		const sint8 htop = h + max(max(corner1(slope), corner2(slope)),max(corner3(slope), corner4(slope)));
		// too high?
		if(  h > hmax  ) {
			break;
		}
		// not too low?
		if(  htop >= hmin  ) {
			const sint16 yypos = ypos - tile_raster_scale_y( (h - h0) * TILE_HEIGHT_STEP, raster_tile_width );
#ifdef MULTI_THREAD
			gr->display_boden( xpos, yypos, raster_tile_width, clip_num );
			gr->display_obj_all( xpos, yypos, raster_tile_width, is_global, clip_num );
#else
			gr->display_boden( xpos, yypos, raster_tile_width );
			gr->display_obj_all( xpos, yypos, raster_tile_width, is_global );
#endif
		}
	}
}


image_id overlay_img(grund_t *gr)
{
	// only transparent outline
	image_id img;
	if(  gr->get_typ()==grund_t::wasser  ) {
		// water is always flat and does not return proper image_id
		img = grund_besch_t::ausserhalb->get_bild(0);
	}
	else {
		img = gr->get_bild();
		if(  img==IMG_LEER  ) {
			// foundations or underground mode
			img = grund_besch_t::get_ground_tile( gr );
		}
	}
	return img;
}


void planquadrat_t::display_overlay(const sint16 xpos, const sint16 ypos) const
{
	grund_t *gr=get_kartenboden();

	// building transformers - show outlines of factories

/*	alternative method of finding selected tool - may be more useful in future but use simpler method for now
	karte_t *welt = gr->get_welt();
	werkzeug_t *wkz = welt->get_werkzeug(welt->get_active_player_nr());
	int tool_id = wkz->get_id();

	if(tool_id==(WKZ_TRANSFORMER|GENERAL_TOOL)....	*/

	if( (grund_t::underground_mode == grund_t::ugm_all  ||  (grund_t::underground_mode == grund_t::ugm_level  &&  gr->get_hoehe() == grund_t::underground_level+1) )
		&&  gr->get_typ()==grund_t::fundament
		&&  werkzeug_t::general_tool[WKZ_TRANSFORMER]->is_selected()) {
		gebaeude_t *gb = gr->find<gebaeude_t>();
		if(gb) {
			fabrik_t* fab=gb->get_fabrik();
			if(fab) {
				PLAYER_COLOR_VAL status = COL_RED;
				if(fab->get_besch()->is_electricity_producer()) {
					status = COL_LIGHT_BLUE;
					if(fab->is_transformer_connected()) {
						status = COL_LIGHT_TURQUOISE;
					}
				}
				else {
					if(fab->is_transformer_connected()) {
						status = COL_ORANGE;
					}
					if(fab->get_prodfactor_electric()>0) {
						status = COL_GREEN;
					}
				}
				display_img_blend( overlay_img(gr), xpos, ypos, status | OUTLINE_FLAG | TRANSPARENT50_FLAG, 0, true);
			}
		}
	}

	// display station owner boxes
	if(env_t::station_coverage_show  &&  halt_list_count>0) {

		if(env_t::use_transparency_station_coverage) {

			// only transparent outline
			image_id img = overlay_img(gr);

			for(int halt_count = 0; halt_count < halt_list_count; halt_count++) {
				const PLAYER_COLOR_VAL transparent = PLAYER_FLAG | OUTLINE_FLAG | (halt_list[halt_count]->get_besitzer()->get_player_color1() + 4);
				display_img_blend( img, xpos, ypos, transparent | TRANSPARENT25_FLAG, 0, 0);
			}
/*
// unfourtunately, too expensive for display
			// plot player outline colours - we always plot in order of players so that the order of the stations in halt_list
			// doesn't affect the colour displayed [since blend(col1,blend(col2,screen)) != blend(col2,blend(col1,screen))]
			for(int spieler_count = 0; spieler_count<MAX_PLAYER_COUNT; spieler_count++) {
				spieler_t *display_player = gr->get_welt()->get_spieler(spieler_count);
				const PLAYER_COLOR_VAL transparent = PLAYER_FLAG | OUTLINE_FLAG | (display_player->get_player_color1() * 4 + 4);
				for(int halt_count = 0; halt_count < halt_list_count; halt_count++) {
					if(halt_list[halt_count]->get_besitzer() == display_player) {
						display_img_blend( img, xpos, ypos, transparent | TRANSPARENT25_FLAG, 0, 0);
					}
				}
			}
	*/
		}
		else {
			const sint16 raster_tile_width = get_tile_raster_width();
			// opaque boxes (
			const sint16 r=raster_tile_width/8;
			const sint16 x=xpos+raster_tile_width/2-r;
			const sint16 y=ypos+(raster_tile_width*3)/4-r - (gr->get_grund_hang()? tile_raster_scale_y(8,raster_tile_width): 0);
			const bool kartenboden_dirty = gr->get_flag(grund_t::dirty);
			const sint16 off = (raster_tile_width>>5);
			// suitable start search
			for (size_t h = halt_list_count; h-- != 0;) {
				display_fillbox_wh_clip(x - h * off, y + h * off, r, r, PLAYER_FLAG | (halt_list[h]->get_besitzer()->get_player_color1() + 4), kartenboden_dirty);
			}
		}
	}

	gr->display_overlay( xpos, ypos );
	if(  ground_size > 1  ) {
		const sint8 h0 = gr->get_disp_height();
		for(  uint8 i = 1;  i < ground_size;  i++  ) {
			grund_t* gr = data.some[i];
			const sint8 h = gr->get_disp_height();
			const sint16 yypos = ypos - (h - h0 ) * get_tile_raster_width() / 2;
			gr->display_overlay( xpos, yypos );
		}
	}
}


// finds halt belonging to player sp
halthandle_t planquadrat_t::get_halt(spieler_t *sp) const
{
	for(  uint8 i=0;  i < get_boden_count();  i++  ) {
		halthandle_t my_halt = get_boden_bei(i)->get_halt();
		if(  my_halt.is_bound()  &&  (sp == NULL  ||  sp == my_halt->get_besitzer())  ) {
			return my_halt;
		}
	}
	return halthandle_t();
}


// these functions are private helper functions for halt_list
void planquadrat_t::halt_list_remove( halthandle_t halt )
{
	for( uint8 i=0;  i<halt_list_count;  i++ ) {
		if(halt_list[i]==halt) {
			for( uint8 j=i+1;  j<halt_list_count;  j++  ) {
				halt_list[j-1] = halt_list[j];
			}
			halt_list_count--;
			break;
		}
	}
}


void planquadrat_t::halt_list_insert_at( halthandle_t halt, uint8 pos )
{
	// extend list?
	if((halt_list_count%4)==0) {
		halthandle_t *tmp = new halthandle_t[halt_list_count+4];
		if(halt_list!=NULL) {
			// now insert
			for( uint8 i=0;  i<halt_list_count;  i++ ) {
				tmp[i] = halt_list[i];
			}
			delete [] halt_list;
		}
		halt_list = tmp;
	}
	// now insert
	for( uint8 i=halt_list_count;  i>pos;  i-- ) {
		halt_list[i] = halt_list[i-1];
	}
	halt_list[pos] = halt;
	halt_list_count ++;
}


/* The following functions takes at least 8 bytes of memory per tile but speed up passenger generation *
 * @author prissi
 */
void planquadrat_t::add_to_haltlist(halthandle_t halt)
{
	if(halt.is_bound()) {
		// quick and dirty way to our 2d koodinates ...
		const koord pos = get_kartenboden()->get_pos().get_2d();

		if(  halt_list_count > 0  ) {

			// since only the first one gets all, we want the closest halt one to be first
			halt_list_remove(halt);
			const koord halt_next_pos = halt->get_next_pos(pos);
			for(unsigned insert_pos=0;  insert_pos<halt_list_count;  insert_pos++) {

				if(  koord_distance(halt_list[insert_pos]->get_next_pos(pos), pos) > koord_distance(halt_next_pos, pos)  ) {
					halt_list_insert_at( halt, insert_pos );
					return;
				}
			}
			// not found
		}
		// first just or just append to the end ...
		halt_list_insert_at( halt, halt_list_count );
	}
}


/**
 * removes the halt from a ground
 * however this funtion check, whether there is really no other part still reachable
 * @author prissi, neroden
 */
void planquadrat_t::remove_from_haltlist(halthandle_t halt)
{
	halt_list_remove(halt);

	// We might still be connected (to a different tile on the halt, in which case reconnect.

	// quick and dirty way to our 2d koodinates ...
	const koord pos = get_kartenboden()->get_pos().get_2d();
	int const cov = welt->get_settings().get_station_coverage();
	for (int y = -cov; y <= cov; y++) {
		for (int x = -cov; x <= cov; x++) {
			koord test_pos = pos+koord(x,y);
			const planquadrat_t *pl = welt->access(test_pos);
			if (pl) {
				for(  uint i = 0;  i < pl->get_boden_count();  i++  ) {
					if (  pl->get_boden_bei(i)->get_halt() == halt  ) {
						// still connected
						// Reset distance computation
						add_to_haltlist(halt);
					}
				}
			}
		}
	}
}


/**
 * true, if this halt is reachable from here
 * @author prissi
 */
bool planquadrat_t::is_connected(halthandle_t halt) const
{
	for( uint8 i=0;  i<halt_list_count;  i++  ) {
		if(halt_list[i]==halt) {
			return true;
		}
	}
	return false;
}
