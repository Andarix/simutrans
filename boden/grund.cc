/*
 * Base class for grounds in simutrans.
 * by Hj. Malthaner
 */

#include <string.h>

#include "../simcolor.h"
#include "../simconst.h"
#include "../simdebug.h"
#include "../simdepot.h"
#include "../display/simgraph.h"
#include "../simhalt.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../gui/simwin.h"
#include "../simworld.h"

#include "../bauer/wegbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/haus_besch.h"
#include "../besch/kreuzung_besch.h"
#include "../besch/tunnel_besch.h"
#include "../besch/weg_besch.h"

#include "../dataobj/freelist.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../dings/baum.h"
#include "../dings/crossing.h"
#include "../dings/groundobj.h"
#include "../dings/label.h"
#include "../dings/leitung2.h"	// for construction of new ways ...
#include "../dings/roadsign.h"
#include "../dings/signal.h"
#include "../dings/tunnel.h"
#include "../dings/wayobj.h"

#include "../gui/ground_info.h"
#include "../gui/karte.h"

#include "../tpl/inthashtable_tpl.h"

#include "../utils/cbuffer_t.h"

#include "../vehicle/simpeople.h"

#include "wege/kanal.h"
#include "wege/maglev.h"
#include "wege/monorail.h"
#include "wege/narrowgauge.h"
#include "wege/runway.h"
#include "wege/schiene.h"
#include "wege/strasse.h"
#include "wege/weg.h"

#include "fundament.h"
#include "grund.h"
#include "tunnelboden.h"
#include "wasser.h"


/**
 * Pointer to the world of this ground. Static to conserve space.
 * Change to instance variable once more than one world is available.
 * @author Hj. Malthaner
 */
karte_t * grund_t::welt = NULL;
volatile bool grund_t::show_grid = false;

uint8 grund_t::offsets[4]={0,1,2/*illegal!*/,2};

sint8 grund_t::underground_level = 127;
uint8 grund_t::underground_mode = ugm_none;


// ---------------------- text handling from here ----------------------


/**
 * Table of ground texts
 * @author Hj. Malthaner
 */
static inthashtable_tpl<uint32, char*> ground_texts;

// since size_x*size_y < 0x1000000, we have just to shift the high bits
#define get_ground_text_key(k,width) ( ((k).x*(width)+(k).y) + ((k).z << 25) )

// and the reverse operation
#define get_ground_koord3d_key(key,width) koord3d( ((key&0x01ffffff))/(width), ((key)&0x01ffffff)%(width), ((key)>>25) | ( (key&0x80000000ul) ? 0x80 : 0x00 ) )

void grund_t::set_text(const char *text)
{
	const uint32 n = get_ground_text_key(pos,welt->get_size().y);
	if(  text  ) {
		char *new_text = strdup(text);
		free(ground_texts.remove(n));
		ground_texts.put(n, new_text);
		set_flag(has_text);
		set_flag(dirty);
		welt->set_dirty();
	}
	else if(  get_flag(has_text)  ) {
		char *txt=ground_texts.remove(n);
		free(txt);
		clear_flag(has_text);
		set_flag(dirty);
		welt->set_dirty();
	}
}


const char *grund_t::get_text() const
{
	const char *result = 0;
	if(  get_flag(has_text)  ) {
		result = ground_texts.get( get_ground_text_key(pos,welt->get_size().y) );
		if(result==NULL) {
			return "undef";
		}
		assert(result);
	}
	return result;
}


PLAYER_COLOR_VAL grund_t::text_farbe() const
{
	// if this gund belongs to a halt, the color should reflect the halt owner, not the grund owner!
	// Now, we use the color of label_t owner
	if(is_halt()  &&  find<label_t>()==NULL) {
		// only halt label
		const halthandle_t halt = get_halt();
		const spieler_t *sp=halt->get_besitzer();
		if(sp) {
			return PLAYER_FLAG|(sp->get_player_color1()+4);
		}
	}
	// else color according to current owner
	else if(obj_bei(0)) {
		const spieler_t *sp = obj_bei(0)->get_besitzer(); // for cityhall
		const label_t* l = find<label_t>();
		if(l) {
			sp = l->get_besitzer();
		}
		if(sp) {
			return PLAYER_FLAG|(sp->get_player_color1()+4);
		}
	}

	return COL_WHITE;
}


// ---------------- init, rdwr, and destruct from here ---------------------


void* grund_t::operator new(size_t s)
{
	return freelist_t::gimme_node(s);
}


void grund_t::operator delete(void* p, size_t s)
{
	return freelist_t::putback_node(s, p);
}


grund_t::grund_t(karte_t *wl, loadsave_t *file)
{
	// only used for saving?
	welt = wl;
	flags = 0;
	back_bild_nr = 0;
	rdwr(file);
}


void grund_t::rdwr(loadsave_t *file)
{
	koord k = pos.get_2d();

	// water saves its correct height => no need to save grid heights anymore
	sint8 z = welt->lookup_hgt( k ); // save grid height for water tiles - including partial water tiles
	sint8 z_w = welt->get_water_hgt( k );
	if(  !(get_typ() == grund_t::boden  ||  get_typ() == grund_t::wasser) || pos.z > z_w || z > z_w  ) {
		z = pos.z; // all other tiles save ground height
	}

	planquadrat_t *plan = welt->access( k );
	uint8 climate_data = plan->get_climate() + (plan->get_climate_corners() << 4);

	xml_tag_t g( file, "grund_t" );
	if(file->get_version()<101000) {
		pos.rdwr(file);
		z_w = welt->get_grundwasser();
	}
	else if(  file->get_version() < 112007  ) {
		file->rdwr_byte(z);
		pos.z = get_typ() == grund_t::wasser ? welt->get_grundwasser() : z;
		z_w = welt->get_grundwasser();
	}
	else {
		file->rdwr_byte(z);
		file->rdwr_byte(z_w);
		if(  file->is_loading()  &&  !ist_wasser()  &&  !welt->lookup_kartenboden( k )  &&  z < z_w  ) {
			// partially in water, restore correct ground height while keeping grid height
			// if kartenboden doesn't exist we will become it
			pos.z = z_w;
		}
		else if(  file->is_loading()  ) {
			pos.z = get_typ() == grund_t::wasser ? z_w : z;
		}
		file->rdwr_byte(climate_data);
		plan->set_climate((climate)(climate_data & 7));
		plan->set_climate_corners((climate_data >> 4));
	}

	if(  file->is_loading()  &&  file->get_version() < 112007  ) {
		// convert heights from old single height saved game - water already at correct height
		pos.z = get_typ() == grund_t::wasser ? pos.z : pos.z * env_t::pak_height_conversion_factor;
		z = z * env_t::pak_height_conversion_factor;
	}

	if(file->is_saving()) {
		const char *text = get_text();
		file->rdwr_str(text);
	}
	else {
		const char *text = 0;
		file->rdwr_str(text);
		if(text) {
			set_text(text);
			guarded_free(const_cast<char *>(text));
		}
	}

	if(file->get_version()<99007) {
		bool label;
		file->rdwr_bool(label);
		if(label) {
			dinge.add( new label_t(welt, pos, welt->get_spieler(0), get_text() ) );
		}
	}

	sint8 besitzer_n=-1;
	if(file->get_version()<99005) {
		file->rdwr_byte(besitzer_n);
	}

	if(file->get_version()>=88009) {
		uint8 sl = slope;
		file->rdwr_byte(sl);
		slope = sl;
	}
	else {
		// safe init for old version
		slope = 0;
	}

	if(  file->is_loading()  &&  file->get_version() < 112007  ) {
		// convert slopes from old single height saved game
		slope = (scorner1(slope) + scorner2(slope) * 3 + scorner3(slope) * 9 + scorner4(slope) * 27) * env_t::pak_height_conversion_factor;
	}
	if(  file->is_loading()  &&  !grund_besch_t::double_grounds  ) {
		// truncate double slopes to single slopes
		slope = min( corner1(slope), 1 ) + min( corner2(slope), 1 ) * 3 + min( corner3(slope), 1 ) * 9 + min( corner4(slope), 1 ) * 27;
	}

	// restore grid
	if(  file->is_loading()  ) {
		// for south/east map edges we need to restore more than one point
		if(  pos.x == welt->get_size().x-1  &&  pos.y == welt->get_size().y-1  ) {
			sint8 z_southeast = z;
			if(  get_typ() == grund_t::wasser  &&  z_southeast > z_w  ) {
				z_southeast = z_w;
			}
			else {
				z_southeast += corner2(slope);
			}
			welt->set_grid_hgt( k + koord(1,1), z_southeast );
		}
		if(  pos.x == welt->get_size().x-1  ) {
			sint8 z_east = z;
			if(  get_typ() == grund_t::wasser  &&  z_east > z_w  ) {
				z_east = z_w;
			}
			else {
				z_east += corner3(slope);
			}
			welt->set_grid_hgt( k + koord(1,0), z_east );
		}
		if(  pos.y == welt->get_size().y-1  ) {
			sint8 z_south = z;
			if(  get_typ() == grund_t::wasser  &&  z_south > z_w  ) {
				z_south = z_w;
			}
			else {
				z_south += corner1(slope);
			}
			welt->set_grid_hgt( k + koord(0,1), z_south );
		}

		if(  get_typ() == grund_t::wasser  &&  z > z_w  ) {
			z = z_w;
		}
		else {
			z += corner4(slope);
		}
		welt->set_grid_hgt( k, z );
		welt->set_water_hgt( k, z_w );
	}

	// loading ways from here on
	if(file->is_loading()) {
		waytype_t wtyp;
		int i = -1;
		do {
			wtyp = (waytype_t)file->rd_obj_id();
			weg_t *weg = NULL;

			if(++i < 2) {
				switch(wtyp) {
					default:
#if DEBUG
						if(  wtyp != invalid_wt  ) {
							dbg->error( "grund_t::rdwr()", "invalid waytype %i!", (int)wtyp );
							wtyp = invalid_wt;
						}
#endif
						break;

					case road_wt:
						weg = new strasse_t (welt, file);
						break;

					case monorail_wt:
						weg = new monorail_t (welt, file);
						break;

					case maglev_wt:
						weg = new maglev_t (welt, file);
						break;

					case narrowgauge_wt:
						weg = new narrowgauge_t (welt, file);
						break;

					case track_wt: {
						schiene_t *sch = new schiene_t (welt, file);
						if(sch->get_besch()->get_wtyp()==monorail_wt) {
							dbg->warning("grund_t::rdwr()", "converting railroad to monorail at (%i,%i)",get_pos().x, get_pos().y);
							// compatibility code: Convert to monorail
							monorail_t *w= new monorail_t(welt);
							w->set_besch(sch->get_besch());
							w->set_max_speed(sch->get_max_speed());
							w->set_ribi(sch->get_ribi_unmasked());
							delete sch;
							weg = w;
						}
						else {
							weg = sch;
						}
					} break;

					case tram_wt:
						weg = new schiene_t (welt, file);
						if(weg->get_besch()->get_styp()!=weg_t::type_tram) {
							weg->set_besch(wegbauer_t::weg_search(tram_wt,weg->get_max_speed(),0,weg_t::type_tram));
						}
						break;

					case water_wt:
						// ignore old type dock ...
						if(file->get_version()>=87000) {
							weg = new kanal_t (welt, file);
						}
						else {
							uint8 d8;
							sint16 d16;
							sint32 d32;

							file->rdwr_byte(d8);
							file->rdwr_short(d16);
							file->rdwr_long(d32);
							file->rdwr_long(d32);
							file->rdwr_long(d32);
							file->rdwr_long(d32);
							DBG_MESSAGE("grund_t::rdwr()","at (%i,%i) dock ignored",get_pos().x, get_pos().y);
						}
						break;

					case air_wt:
						weg = new runway_t (welt, file);
						break;
				}

				if(weg) {
					if(get_typ()==fundament) {
						// remove this (but we can not correct the other wasy, since possibly not yet loaded)
						dbg->error("grund_t::rdwr()","removing way from foundation at %i,%i",pos.x,pos.y);
						// we do not delete them, to keep maintenance costs correct
					}
					else {
						assert((flags&has_way2)==0);	// maximum two ways on one tile ...
						weg->set_pos(pos);
						if(besitzer_n!=-1) {
							weg->set_besitzer(welt->get_spieler(besitzer_n));
						}
						dinge.add(weg);
						if(flags&has_way1) {
							flags |= has_way2;
						}
						flags |= has_way1;
					}
				}
			}
		} while(wtyp != invalid_wt);

		flags |= dirty;
	}
	else {
		// saving all ways ...
		if (weg_t* const w = get_weg_nr(0)) {
			file->wr_obj_id(w->get_waytype());
			w->rdwr(file);
		}
		if (weg_t* const w = get_weg_nr(1)) {
			file->wr_obj_id(w->get_waytype());
			w->rdwr(file);
		}
		file->wr_obj_id(-1);   // Ende der Wege
	}

	// all objects on this tile
	dinge.rdwr(welt, file, get_pos());

	// need to add a crossing for old games ...
	if (file->is_loading()  &&  ist_uebergang()  &&  !find<crossing_t>(2)) {
		const kreuzung_besch_t *cr_besch = crossing_logic_t::get_crossing( ((weg_t *)obj_bei(0))->get_waytype(), ((weg_t *)obj_bei(1))->get_waytype(), ((weg_t *)obj_bei(0))->get_max_speed(), ((weg_t *)obj_bei(1))->get_max_speed(), 0 );
		if(cr_besch==0) {
			dbg->fatal("crossing_t::crossing_t()","requested for waytypes %i and %i but nothing defined!", ((weg_t *)obj_bei(0))->get_waytype(), ((weg_t *)obj_bei(1))->get_waytype() );
		}
		crossing_t *cr = new crossing_t( welt, obj_bei(0)->get_besitzer(), pos, cr_besch, ribi_t::ist_gerade_ns(get_weg(cr_besch->get_waytype(1))->get_ribi_unmasked()) );
		dinge.add( cr );
		crossing_logic_t::add( welt, cr, crossing_logic_t::CROSSING_INVALID );
	}
}


grund_t::grund_t(karte_t *wl, koord3d pos)
{
	this->pos = pos;
	flags = 0;
	welt = wl;
	set_bild(IMG_LEER);    // setzt   flags = dirty;
	back_bild_nr = 0;
}


grund_t::~grund_t()
{
	destroy_win((ptrdiff_t)this);

	// remove text from table
	set_text(NULL);

	dinge.loesche_alle(NULL,0);
	if(flags&is_halt_flag) {
		get_halt()->rem_grund(this);
	}
}


void grund_t::sort_trees()
{
	if (get_typ() != boden) {
		return;
	}
	uint8 trees = 0, offset = 0;
	for(  int i=0;  i<dinge.get_top();  i++  ) {
		if (obj_bei(i)->get_typ() == ding_t::baum) {
			trees++;
			offset = i;
		}
	}
	if(trees > 1) {
		dinge.sort_trees(offset-trees+1u, trees);
	}
}


void grund_t::rotate90()
{
	pos.rotate90( welt->get_size().y-1 );
	slope = hang_t::rotate90( slope );
	// then rotate the things on this tile
	uint8 trees = 0, offset = 0;
	if(  get_top()==254  ) {
		dbg->warning( "grund_t::rotate90()", "Too many stuff on (%s)", pos.get_str() );
	}
	for(  uint8 i=0;  i<dinge.get_top();  i++  ) {
		obj_bei(i)->rotate90();
		if (obj_bei(i)->get_typ() == ding_t::baum) {
			trees++;
			offset = i;
		}
	}
	// if more than one tree on a tile .. resort since offsets changed
	if(trees > 1) {
		dinge.sort_trees(offset-trees+1u, trees);
	}
}


// after processing the last tile, we recalculate the hashes of the ground texts
void grund_t::finish_rotate90()
{
	typedef inthashtable_tpl<uint32, char*> text_map;
	text_map ground_texts_rotating;
	// first get the old hashes
	FOR(text_map, iter, ground_texts) {
		koord3d k = get_ground_koord3d_key( iter.key, welt->get_size().y );
		k.rotate90( welt->get_size().y-1 );
		ground_texts_rotating.put( get_ground_text_key(k,welt->get_size().x), iter.value );
	}
	ground_texts.clear();
	// then transfer all rotated texts
	FOR(text_map, const& iter, ground_texts_rotating) {
		ground_texts.put(iter.key, iter.value);
	}
	ground_texts_rotating.clear();
}


void grund_t::enlarge_map( sint16, sint16 new_size_y )
{
	typedef inthashtable_tpl<uint32, char*> text_map;
	text_map ground_texts_enlarged;
	// we have recalculate the keys
	FOR(text_map, iter, ground_texts) {
		koord3d k = get_ground_koord3d_key( iter.key, welt->get_size().y );
		ground_texts_enlarged.put( get_ground_text_key(k,new_size_y), iter.value );
	}
	ground_texts.clear();
	// then transfer all texts back
	FOR(text_map, const& iter, ground_texts_enlarged) {
		ground_texts.put(iter.key, iter.value);
	}
	ground_texts_enlarged.clear();
}


// moves all objects from the old to the new grund_t
void grund_t::take_obj_from(grund_t* other_gr)
{
	// transfer all things
	while( other_gr->get_top() ) {
		dinge.add( other_gr->obj_remove_top() );
	}
	// transfer the way flags
	if(other_gr->get_flag(has_way1)) {
		flags |= has_way1;
		other_gr->clear_flag(has_way1);
	}
	if(other_gr->get_flag(has_way2)) {
		flags |= has_way2;
		other_gr->clear_flag(has_way2);
	}
}


void grund_t::zeige_info()
{
	int old_count = win_get_open_count();
	if(get_halt().is_bound()) {
		get_halt()->zeige_info();
		if(env_t::single_info  &&  old_count!=win_get_open_count()  ) {
			return;
		}
	}
	if(env_t::ground_info  ||  hat_wege()) {
		create_win(new grund_info_t(this), w_info, (ptrdiff_t)this);
	}
}


void grund_t::info(cbuffer_t& buf) const
{
	if(!ist_wasser()) {
		if(flags&has_way1) {
			// bridges / tunnels only carry dummy ways
			if(!ist_tunnel()  &&  !ist_bruecke()) {
				buf.append(translator::translate(get_weg_nr(0)->get_name()));
				buf.append("\n");
			}
			obj_bei(0)->info(buf);
			buf.append("\n");
			if(flags&has_way2) {
				buf.append(translator::translate(get_weg_nr(1)->get_name()));
				buf.append("\n");
				obj_bei(1)->info(buf);
				buf.append("\n");
				if(ist_uebergang()) {
					crossing_t* crossing = find<crossing_t>(2);
					buf.append(translator::translate(crossing->get_name()));
					buf.append("\n");
					crossing->get_logic()->info(buf);
					buf.append("\n");
				}
			}
		}
	}

	buf.printf("%s\n%s", get_name(), translator::translate( grund_besch_t::get_climate_name_from_bit( welt->get_climate( get_pos().get_2d() ) ) ) );
#if DEBUG >= 3
	buf.printf("\nflags $%0X", flags );
	buf.printf("\n\npos: (%s)",pos.get_str());
	buf.printf("\nslope: %i",get_grund_hang());
	buf.printf("\nback0: %i",get_back_bild(0)-grund_besch_t::slopes->get_bild(0));
	buf.printf("\nback1: %i",get_back_bild(1)-grund_besch_t::slopes->get_bild(0));
	if(  get_weg_nr(0)  ) {
		buf.printf("\nway slope %i", (int)get_weg_hang() );
	}
	if(get_weg_ribi_unmasked(water_wt)) {
		buf.printf("\nwater ribi: %i",get_weg_ribi_unmasked(water_wt));
	}
	buf.printf("\ndraw_as_ding= %i",(flags&draw_as_ding)!=0);
#endif
}


void grund_t::set_halt(halthandle_t halt)
{
	bool add = halt.is_bound();
	if(  add  ) {
		// ok, we want to add a stop: first check if it can apply to water
		if(  get_weg_ribi(water_wt)  ||  ist_wasser()  ||  (welt->get_climate(pos.get_2d())==water_climate  &&  !ist_im_tunnel()  &&  get_typ()!=brueckenboden)  ) {
			add = (halt->get_station_type() & haltestelle_t::dock) > 0;
		}
	}
	// then add or remove halt flag
	// and record the halt
	if(  add  ) {
		this_halt = halt;
		flags |= is_halt_flag|dirty;
	}
	else {
		this_halt = halthandle_t();
		flags &= ~is_halt_flag;
		flags |= dirty;
	}
}



halthandle_t grund_t::get_halt() const
{
	return (flags&is_halt_flag) ? this_halt : halthandle_t();
}


// ----------------------- image calculation stuff from here ------------------


void grund_t::calc_bild()
{
	// will automatically recalculate ways ...
	dinge.calc_bild();
	// since bridges may alter images of ways, this order is needed!
	calc_bild_internal();
}


void grund_t::set_underground_mode(const uint8 ugm, const sint8 level)
{
	underground_mode = ugm;
	switch(ugm) {
		case ugm_all:
			underground_level = -128;
			break;
		case ugm_level:
			underground_level = level;
			break;
		case ugm_none:
		default:
			underground_mode = ugm_none;
			underground_level = 127;
	}
}


image_id grund_t::get_back_bild(int leftback) const
{
	if(back_bild_nr==0) {
		return IMG_LEER;
	}
	sint8 back_bild = abs(back_bild_nr);
	back_bild = leftback ? (back_bild/11)+11 : back_bild%11;
	if(back_bild_nr<0) {
		return grund_besch_t::fundament->get_bild(back_bild);
	}
	else {
		return grund_besch_t::slopes->get_bild(back_bild);
	}
}


// with double height ground tiles!
// can also happen with single height tiles
static inline uint8 get_backbild_from_diff(sint8 h1, sint8 h2)
{
	sint8 min_diff = min( h1, h2 );
	while(  min_diff > 2  ||  (min_diff > 0  &&  h1 != h2)  ) {
		h1 -= min_diff > 1 ? 2 : 1;
		h2 -= min_diff > 1 ? 2 : 1;
		min_diff -= 2;
	}

	if(h1*h2<0) {
		// middle slop of double height
		return h1<0 ? 9 : 10;
	}
	else {
		return (h1>0?h1:0)+(h2>0?h2:0)*3;
	}
}

/**
* if ground is deleted mark the old spot as dirty
*/
void grund_t::mark_image_dirty()
{
	// see ding_t::mark_image_dirty
	if(bild_nr!=IMG_LEER) {
		// better not try to twist your brain to follow the retransformation ...
		const sint16 rasterweite=get_tile_raster_width();
		const koord diff = pos.get_2d()-welt->get_world_position()-welt->get_view_ij_offset();
		const sint16 x = (diff.x-diff.y)*(rasterweite/2);
		const sint16 y = (diff.x+diff.y)*(rasterweite/4) + tile_raster_scale_y( -get_disp_height()*TILE_HEIGHT_STEP, rasterweite) + ((display_get_width()/rasterweite)&1)*(rasterweite/4);
		// mark the region after the image as dirty
		display_mark_img_dirty( bild_nr, x+welt->get_x_off(), y+welt->get_y_off() );
	}
}

// artifical walls from here on ...
void grund_t::calc_back_bild(const sint8 hgt,const sint8 slope_this)
{
	if(  underground_mode == ugm_all  ) {
		this->back_bild_nr = 0;
		return;
	}
	sint8 back_bild_nr=0;
	bool is_building = get_typ()==grund_t::fundament;
	const bool isvisible = is_visible();
	bool fence[2]={false, false};
	const koord k = get_pos().get_2d();

	clear_flag(grund_t::draw_as_ding);
	weg_t const* w;
	if (((w = get_weg_nr(0)) && w->get_besch()->is_draw_as_ding()) ||
			((w = get_weg_nr(1)) && w->get_besch()->is_draw_as_ding())) {
		set_flag(grund_t::draw_as_ding);
	}
	bool left_back_is_building = false;

	// check for foundation
	if(  const grund_t *gr=welt->lookup_kartenboden(k+koord(-1,-1))  ) {
		const sint16 left_hgt=gr->get_disp_height();
		const sint8 slope=gr->get_disp_slope();

		const sint8 diff_from_ground = left_hgt+corner2(slope)-hgt-corner4(slope_this);
		// up slope hiding something ...
		if(diff_from_ground<0)  {
			set_flag(grund_t::draw_as_ding);
		}
		else if(gr->get_flag(grund_t::draw_as_ding)  ||  gr->obj_count()>0) {
			left_back_is_building = true;
		}
	}

	for(  int i=0;  i<2;  i++  ) {
		// now enter the left/back two height differences
		if(  const grund_t *gr=welt->lookup_kartenboden(k + koord::nsow[(i-1)&3])  ) {
			const uint8 back_height = min(corner4(slope_this),(i==0?corner1(slope_this):corner3(slope_this)));

			const sint16 left_hgt=gr->get_disp_height()-back_height;
			const sint8 slope=gr->get_disp_slope();

			const uint8 corner_a = (i==0?corner1(slope_this):corner4(slope_this))-back_height;
			const uint8 corner_b = (i==0?corner4(slope_this):corner3(slope_this))-back_height;

			sint8 diff_from_ground_1 = left_hgt+(i==0?corner2(slope):corner1(slope))-hgt;
			sint8 diff_from_ground_2 = left_hgt+(i==0?corner3(slope):corner2(slope))-hgt;

			if (underground_mode==ugm_level) {
				// if exactly one of (this) and (gr) is visible, show full walls
				if ( isvisible && !gr->is_visible()){
					diff_from_ground_1 += 1;
					diff_from_ground_2 += 1;
					set_flag(grund_t::draw_as_ding);
					fence[i] = corner_a==corner_b;
				}
				else if ( !isvisible && gr->is_visible()){
					diff_from_ground_1 = 1;
					diff_from_ground_2 = 1;
				}
				// avoid walls that cover the tunnel mounds
				if ( gr->is_visible() && (gr->get_typ()==grund_t::tunnelboden) && ist_karten_boden() && gr->get_pos().z==underground_level && gr->get_grund_hang()==(i==0?hang_t::west:hang_t::nord)) {
					diff_from_ground_1 = 0;
					diff_from_ground_2 = 0;
				}
				if ( is_visible() && (get_typ()==grund_t::tunnelboden) && ist_karten_boden() && pos.z==underground_level && get_grund_hang()==(i==0?hang_t::ost:hang_t::west)) {
					diff_from_ground_1 = 0;
					diff_from_ground_2 = 0;
				}
			}

			// up slope hiding something ...
			if(diff_from_ground_1-corner_a<0  ||  diff_from_ground_2-corner_b<0)  {
				set_flag(grund_t::draw_as_ding);
				if(  corner_a==corner_b  ) {
					// ok, we need a fence here, if there is not a vertical bridgehead
					weg_t const* w;
					fence[i] = !(w = get_weg_nr(0)) || (
						!(w->get_ribi_unmasked() & ribi_t::nsow[(i-1)&3]) &&
						(!(w = get_weg_nr(1)) || !(w->get_ribi_unmasked() & ribi_t::nsow[(i-1)&3]))
					);

					// no fences between water tiles or between invisible tiles
					if(  fence[i]  &&  ( (ist_wasser() && gr->ist_wasser()) || (!isvisible && !gr->is_visible()) )  ) {
						fence[i] = false;
					}
				}
			}
			// any height difference AND something to see?
			if(  (diff_from_ground_1-corner_a>0  ||  diff_from_ground_2-corner_b>0)
				&&  (diff_from_ground_1>0  ||  diff_from_ground_2>0)  ) {
				back_bild_nr += get_backbild_from_diff( diff_from_ground_1, diff_from_ground_2 )*(i==0?1:11);
				is_building |= gr->get_typ()==grund_t::fundament;
			}
			// avoid covering of slope by building ...
			if(  (left_back_is_building  ||  gr->get_flag(draw_as_ding))  &&  (back_bild_nr>i*11  ||  gr->get_back_bild(1-i)!=IMG_LEER)) {
				set_flag(grund_t::draw_as_ding);
			}
		}
	}

	// not ground -> then not draw first ...
	if(  welt->lookup_kartenboden(k) != this  ) {
		clear_flag(grund_t::draw_as_ding);
	}

	// needs a fence?
	if(back_bild_nr==0) {
		sint8 fence_offset = fence[0] + 2 * fence[1];
		if(fence_offset) {
			back_bild_nr = 121 + fence_offset;
		}
	}
	this->back_bild_nr = (is_building!=0)? -back_bild_nr : back_bild_nr;
}


#ifdef MULTI_THREAD
void grund_t::display_boden(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const sint8 clip_num, const bool force_show_grid) const
#else
void grund_t::display_boden(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width) const
#endif
{
	const bool dirty = get_flag(grund_t::dirty);
	const koord k = get_pos().get_2d();

	// here: we are either ground(kartenboden) or visible
	const bool visible = !ist_karten_boden()  ||  is_karten_boden_visible();

	// walls, fences, foundations etc
	if(back_bild_nr!=0 && visible) {
		const uint8 abs_back_bild_nr = abs(back_bild_nr);
		const bool artificial = back_bild_nr < 0;
		if(abs_back_bild_nr>121) {
			// fence before a drop
			const sint16 offset = -tile_raster_scale_y( TILE_HEIGHT_STEP*corner4(get_grund_hang()), raster_tile_width);
#ifdef MULTI_THREAD
			display_normal( grund_besch_t::fences->get_bild( abs_back_bild_nr + (artificial ? -122 + 3 : -122) ), xpos, ypos + offset, 0, true, dirty, clip_num );
#else
			display_normal( grund_besch_t::fences->get_bild( abs_back_bild_nr + (artificial ? -122 + 3 : -122) ), xpos, ypos + offset, 0, true, dirty );
#endif
		}
		else {
			// artificial slope
			const int back_bild[2] = {abs_back_bild_nr%11, (abs_back_bild_nr/11)+11};
			sint8 yoff[2];

			// choose foundation or natural slopes
			const grund_besch_t *sl_draw = artificial ? grund_besch_t::fundament : grund_besch_t::slopes;

			// first draw left, then back slopes
			for(  int i=0;  i<2;  i++  ) {
				const uint8 back_height = min(i==0?corner1(slope):corner3(slope),corner4(slope));
				yoff[i] = tile_raster_scale_y( -TILE_HEIGHT_STEP*back_height, raster_tile_width );
				if(  back_bild[i]  ) {
					grund_t *gr = welt->lookup_kartenboden( k + koord::nsow[(i-1)&3] );
					if(  gr  ) {
						// for left we test corners 2 and 3 (east), for back we use 1 and 2 (south)
						const sint8 gr_slope = gr->get_disp_slope();
						const uint8 corner_a = corner2(gr_slope);
						const uint8 corner_b = i==0?corner3(gr_slope):corner1(gr_slope);

						sint16 hgt_diff = gr->get_disp_height() - get_disp_height() + min( corner_a, corner_b ) - back_height;
						while(  hgt_diff > 2  ||  (hgt_diff > 0  &&  corner_a != corner_b)  ) {
#ifdef MULTI_THREAD
							display_normal( sl_draw->get_bild( 4+4*(hgt_diff>1)+11*i ), xpos, ypos + yoff[i], 0, true, dirty, clip_num );
#else
							display_normal( sl_draw->get_bild( 4+4*(hgt_diff>1)+11*i ), xpos, ypos + yoff[i], 0, true, dirty );
#endif
							yoff[i] -= tile_raster_scale_y( TILE_HEIGHT_STEP * (hgt_diff > 1 ? 2 : 1), raster_tile_width );
							hgt_diff -= 2;
						}
					}
#ifdef MULTI_THREAD
					display_normal( sl_draw->get_bild( back_bild[i] ), xpos, ypos + yoff[i], 0, true, dirty, clip_num );
#else
					display_normal( sl_draw->get_bild( back_bild[i] ), xpos, ypos + yoff[i], 0, true, dirty );
#endif
				}
			}
		}
	}

	// ground
	image_id bild = get_bild();
	if(bild==IMG_LEER) {
		// only check for forced redraw (of marked ... )
		if(dirty) {
#ifdef MULTI_THREAD
			mark_rect_dirty_clip( xpos, ypos + raster_tile_width / 2, xpos + raster_tile_width - 1, ypos + raster_tile_width - 1, clip_num );
#else
			mark_rect_dirty_clip( xpos, ypos + raster_tile_width / 2, xpos + raster_tile_width - 1, ypos + raster_tile_width - 1 );
#endif
		}
	}
	else {
		if(get_typ()!=wasser) {
			// show image if tile is visible
			if (visible)  {
#ifdef MULTI_THREAD
				display_normal( get_bild(), xpos, ypos, 0, true, dirty, clip_num );
#else
				display_normal( get_bild(), xpos, ypos, 0, true, dirty );
#endif

				//display climate transitions - only needed if below snowline (snow_transition>0)
				//need to process whole tile for all heights anyway as water transitions are needed for all heights
				const planquadrat_t * plan = welt->access( k );
				uint8 climate_corners = plan->get_climate_corners();
				const sint8 snow_transition = welt->get_snowline() - pos.z;
				weg_t *weg = get_weg(road_wt);
				if(  climate_corners != 0  &&  (!weg  ||  !weg->hat_gehweg())  ) {
					uint8 water_corners = 0;

					// get neighbour corner heights
					sint8 neighbour_height[8][4];
					welt->get_neighbour_heights( k, neighbour_height );

					//look up neighbouring climates
					climate neighbour_climate[8];
					for(  int i = 0;  i < 8;  i++  ) {
						koord k_neighbour = k + koord::neighbours[i];
						if(  !welt->is_within_limits(k_neighbour)  ) {
							k_neighbour = welt->get_closest_coordinate(k_neighbour);
						}
						neighbour_climate[i] = welt->get_climate( k_neighbour );
					}

					climate climate0 = plan->get_climate();
					hang_t::typ slope_corner = get_grund_hang();

					// get transition climate - look for each corner in turn
					for(  int i = 0;  i < 4;  i++  ) {
						sint8 corner_height = get_hoehe() + (slope_corner % 3);

						climate transition_climate = climate0;
						climate min_climate = arctic_climate;

						for(  int j = 1;  j < 4;  j++ ) {
							if(  corner_height == neighbour_height[(i * 2 + j) & 7][(i + j) & 3]) {
								climate climatej = neighbour_climate[(i * 2 + j) & 7];
								climatej > transition_climate ? transition_climate = climatej : 0;
								climatej < min_climate ? min_climate = climatej : 0;
							}
						}

						if(  min_climate == water_climate  ) {
							water_corners += 1 << i;
						}
						if(  (climate_corners >> i) & 1  &&  !ist_wasser()  &&  snow_transition > 0  ) {
							// looks up sw, se, ne, nw for i=0...3
							// we compare with tile either side (e.g. for sw, w and s) and pick highest one
							if(  transition_climate > climate0  ) {
								uint8 overlay_corners = 1 << i;
								hang_t::typ slope_corner2 = slope_corner;
								for(  int j = i + 1;  j < 4;  j++  ) {
									slope_corner2 /= 3;

									// now we check to see if any of remaining corners have same climate transition (also using highest of course)
									// if so we combine into this overlay layer
									if(  (climate_corners >> j) & 1  ) {
										climate compare = climate0;
										for(  int k = 1;  k < 4;  k++  ) {
											corner_height = get_hoehe() + (slope_corner2 % 3);
											if(  corner_height == neighbour_height[(j * 2 + k) & 7][(j + k) & 3]) {
												climate climatej = neighbour_climate[(j * 2 + k) & 7];
												climatej > compare ? compare = climatej : 0;
											}
										}

										if(  transition_climate == compare  ) {
											overlay_corners += 1 << j;
											climate_corners -= 1 << j;
										}
									}
								}
								// overlay transition climates
#ifdef MULTI_THREAD
								display_alpha( grund_besch_t::get_climate_tile( transition_climate, slope ), grund_besch_t::get_alpha_tile( slope, overlay_corners ), ALPHA_GREEN | ALPHA_RED, xpos, ypos, 0, 0, true, dirty, clip_num );
#else
								display_alpha( grund_besch_t::get_climate_tile( transition_climate, slope ), grund_besch_t::get_alpha_tile( slope, overlay_corners ), ALPHA_GREEN | ALPHA_RED, xpos, ypos, 0, 0, true, dirty );
#endif
							}
						}
						slope_corner /= 3;
					}
					// finally overlay any water transition
					if(  water_corners  ) {
#ifdef MULTI_THREAD
						display_alpha( grund_besch_t::get_water_tile(slope), grund_besch_t::get_beach_tile( slope, water_corners ), ALPHA_BLUE, xpos, ypos, 0, 0, true, dirty, clip_num );
#else
						display_alpha( grund_besch_t::get_water_tile(slope), grund_besch_t::get_beach_tile( slope, water_corners ), ALPHA_BLUE, xpos, ypos, 0, 0, true, dirty );
#endif
					}
				}

				//display snow transitions if required
				if(  slope != 0  &&  (!weg  ||  !weg->hat_gehweg())  ) {
					switch(  snow_transition  ) {
						case 1: {
#ifdef MULTI_THREAD
							display_alpha( grund_besch_t::get_snow_tile(slope), grund_besch_t::get_alpha_tile(slope), ALPHA_GREEN | ALPHA_RED, xpos, ypos, 0, 0, true, dirty, clip_num );
#else
							display_alpha( grund_besch_t::get_snow_tile(slope), grund_besch_t::get_alpha_tile(slope), ALPHA_GREEN | ALPHA_RED, xpos, ypos, 0, 0, true, dirty );
#endif
							break;
						}
						case 2: {
							if(  hang_t::height(slope) > 1  ) {
#ifdef MULTI_THREAD
								display_alpha( grund_besch_t::get_snow_tile(slope), grund_besch_t::get_alpha_tile(slope), ALPHA_RED, xpos, ypos, 0, 0, true, dirty, clip_num );
#else
								display_alpha( grund_besch_t::get_snow_tile(slope), grund_besch_t::get_alpha_tile(slope), ALPHA_RED, xpos, ypos, 0, 0, true, dirty );
#endif
							}
							break;
						}
					}
				}

				// we show additionally a grid
				// for undergroundmode = ugm_all the grid is plotted in display_dinge
#ifdef MULTI_THREAD
				if(  show_grid  || force_show_grid  ) {
#else
				if(  show_grid  ){
#endif
					const uint8 hang = get_grund_hang();
#ifdef MULTI_THREAD
					display_normal( grund_besch_t::get_border_image(hang), xpos, ypos, 0, true, dirty, clip_num );
#else
					display_normal( grund_besch_t::get_border_image(hang), xpos, ypos, 0, true, dirty );
#endif
				}
			}
		}
		else {
			// take animation into account
			if(  underground_mode != ugm_all  ) {
#ifdef MULTI_THREAD
				display_normal( grund_besch_t::sea->get_bild(get_bild(),wasser_t::stage), xpos, ypos, 0, true, dirty|wasser_t::change_stage, clip_num );
#else
				display_normal( grund_besch_t::sea->get_bild(get_bild(),wasser_t::stage), xpos, ypos, 0, true, dirty|wasser_t::change_stage );
#endif
			}
			else {
#ifdef MULTI_THREAD
				display_blend( grund_besch_t::sea->get_bild(get_bild(),wasser_t::stage), xpos, ypos, 0, TRANSPARENT50_FLAG, true, dirty|wasser_t::change_stage, clip_num);
#else
				display_blend( grund_besch_t::sea->get_bild(get_bild(),wasser_t::stage), xpos, ypos, 0, TRANSPARENT50_FLAG, true, dirty|wasser_t::change_stage);
#endif
			}
			return;
		}
	}
	// display ways
	if(  visible  &&  (flags&has_way1)  ){
		const bool clip = (  (flags&draw_as_ding)  ||  !ist_karten_boden()  )  &&  !env_t::simple_drawing;
		const int hgt_step = tile_raster_scale_y( TILE_HEIGHT_STEP, raster_tile_width );
		for(  uint8 i = 0;  i < offsets[flags / has_way1];  i++  ) {
			ding_t* d = obj_bei(i);
			// clip
			// .. nonconvex n/w if not both n/w are active
			if(  clip  ) {
				const ribi_t::ribi way_ribi = (static_cast<const weg_t*>(d))->get_ribi_unmasked();
#ifdef MULTI_THREAD
				clear_all_poly_clip( clip_num );
#else
				clear_all_poly_clip();
#endif
				const uint8 non_convex = (way_ribi & ribi_t::nordwest) == ribi_t::nordwest ? 0 : 16;
				if(  way_ribi & ribi_t::west  ) {
					const int dh = corner4(get_disp_way_slope()) * hgt_step;
#ifdef MULTI_THREAD
					add_poly_clip( xpos + raster_tile_width / 2 - 1, ypos + raster_tile_width / 2 - dh, xpos - 1, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::west | non_convex, clip_num );
#else
					add_poly_clip( xpos + raster_tile_width / 2 - 1, ypos + raster_tile_width / 2 - dh, xpos - 1, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::west | non_convex );
#endif
				}
				if(  way_ribi & ribi_t::nord  ) {
					const int dh = corner4(get_disp_way_slope()) * hgt_step;
#ifdef MULTI_THREAD
					add_poly_clip( xpos + raster_tile_width - 1, ypos + 3 * raster_tile_width / 4 - 1 - dh, xpos + raster_tile_width / 2 - 1, ypos + raster_tile_width / 2 - 1 - dh, ribi_t::nord | non_convex, clip_num );
#else
					add_poly_clip( xpos + raster_tile_width - 1, ypos + 3 * raster_tile_width / 4 - 1 - dh, xpos + raster_tile_width / 2 - 1, ypos + raster_tile_width / 2 - 1 - dh, ribi_t::nord | non_convex );
#endif
				}
#ifdef MULTI_THREAD
				activate_ribi_clip( way_ribi & ribi_t::nordwest, clip_num );
#else
				activate_ribi_clip( way_ribi & ribi_t::nordwest );
#endif
			}
#ifdef MULTI_THREAD
			d->display( xpos, ypos, clip_num );
#else
			d->display( xpos, ypos );
#endif
		}
		// end of clipping
		if(  clip  ) {
#ifdef MULTI_THREAD
			clear_all_poly_clip( clip_num );
#else
			clear_all_poly_clip();
#endif
		}
	}
}


#ifdef MULTI_THREAD
void grund_t::display_border( sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const sint8 clip_num )
#else
void grund_t::display_border( sint16 xpos, sint16 ypos, const sint16 raster_tile_width )
#endif
{
	if(  pos.z < welt->get_grundwasser()  ) {
		// we do not display below water (yet)
		return;
	}

	const sint16 hgt_step = tile_raster_scale_y( TILE_HEIGHT_STEP, raster_tile_width);
	static sint8 lookup_hgt[5] = { 6, 3, 0, 1, 2 };

	if(  pos.y-welt->get_size().y+1 == 0  ) {
		// move slopes to front of tile
		sint16 x = xpos - raster_tile_width/2;
		sint16 y = ypos + raster_tile_width/4 + (pos.z-welt->get_grundwasser())*hgt_step;
		// left side border
		sint16 diff = corner1(slope)-corner2(slope);
		image_id slope_img = grund_besch_t::slopes->get_bild( lookup_hgt[ 2+diff ]+11 );
		diff = -min(corner1(slope),corner2(slope));
		sint16 zz = pos.z-welt->get_grundwasser();
		if(  diff < zz && ((zz-diff)&1)==1  ) {
#ifdef MULTI_THREAD
			display_normal( grund_besch_t::slopes->get_bild(15), x, y, 0, true, false, clip_num );
#else
			display_normal( grund_besch_t::slopes->get_bild(15), x, y, 0, true, false );
#endif
			y -= hgt_step;
			diff++;
		}
		// ok, now we have the height; since the slopes may end with a fence they are drawn in reverse order
		while(  diff < zz  ) {
#ifdef MULTI_THREAD
			display_normal( grund_besch_t::slopes->get_bild(19), x, y, 0, true, false, clip_num );
#else
			display_normal( grund_besch_t::slopes->get_bild(19), x, y, 0, true, false );
#endif
			y -= hgt_step*2;
			diff+=2;
		}
#ifdef MULTI_THREAD
		display_normal( slope_img, x, y, 0, true, false, clip_num );
#else
		display_normal( slope_img, x, y, 0, true, false );
#endif
	}

	if(  pos.x-welt->get_size().x+1 == 0  ) {
		// move slopes to front of tile
		sint16 x = xpos + raster_tile_width/2;
		sint16 y = ypos + raster_tile_width/4 + (pos.z-welt->get_grundwasser())*hgt_step;
		// right side border
		sint16 diff = corner2(slope)-corner3(slope);
		image_id slope_img = grund_besch_t::slopes->get_bild( lookup_hgt[ 2+diff ] );
		diff = -min(corner2(slope),corner3(slope));
		sint16 zz = pos.z-welt->get_grundwasser();
		if(  diff < zz && ((zz-diff)&1)==1  ) {
#ifdef MULTI_THREAD
			display_normal( grund_besch_t::slopes->get_bild(4), x, y, 0, true, false, clip_num );
#else
			display_normal( grund_besch_t::slopes->get_bild(4), x, y, 0, true, false );
#endif
			y -= hgt_step;
			diff++;
		}
		// ok, now we have the height; since the slopes may end with a fence they are drawn in reverse order
		while(  diff < zz  ) {
#ifdef MULTI_THREAD
			display_normal( grund_besch_t::slopes->get_bild(8), x, y, 0, true, false, clip_num );
#else
			display_normal( grund_besch_t::slopes->get_bild(8), x, y, 0, true, false );
#endif
			y -= hgt_step*2;
			diff+=2;
		}
#ifdef MULTI_THREAD
		display_normal( slope_img, x, y, 0, true, false, clip_num );
#else
		display_normal( slope_img, x, y, 0, true, false );
#endif
	}
}


#ifdef MULTI_THREAD
void grund_t::display_if_visible(sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const sint8 clip_num, const bool force_show_grid)
#else
void grund_t::display_if_visible(sint16 xpos, sint16 ypos, const sint16 raster_tile_width)
#endif
{
	if(  !is_karten_boden_visible()  ) {
		return;
	}

	if(  env_t::draw_earth_border  &&  (pos.x-welt->get_size().x+1 == 0  ||  pos.y-welt->get_size().y+1 == 0)  ) {
		// the last tile. might need a border
#ifdef MULTI_THREAD
		display_border( xpos, ypos, raster_tile_width, clip_num );
#else
		display_border( xpos, ypos, raster_tile_width );
#endif
	}

	if(!get_flag(grund_t::draw_as_ding)) {
#ifdef MULTI_THREAD
		display_boden( xpos, ypos, raster_tile_width, clip_num, force_show_grid );
#else
		display_boden( xpos, ypos, raster_tile_width );
#endif
	}
}


hang_t::typ grund_t::get_disp_way_slope() const
{
	if (is_visible()) {
		if (ist_bruecke()) {
			const hang_t::typ slope = get_grund_hang();
			if(  slope != 0  ) {
				// for half height slopes we want all corners at 1, for full height all corners at 2
				return (slope & 7) ? hang_t::erhoben / 2 : hang_t::erhoben;
			}
			else {
				return get_weg_hang();
			}
		}
		else if (ist_tunnel() && ist_karten_boden()) {
			if (pos.z >= underground_level) {
				return hang_t::flach;
			}
			else {
				return get_grund_hang();
			}
		}
		else {
			return get_grund_hang();
		}
	}
	else {
		return hang_t::flach;
	}
}


/**
 * The old main display routine. Used for very small tile sizes, where clipping error
 * will be only one or two pixels.
 *
 * Also used in multi-threaded display.
 */
#ifdef MULTI_THREAD
void grund_t::display_dinge_all_quick_and_dirty(const sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const bool is_global, const sint8 clip_num) const
#else
void grund_t::display_dinge_all_quick_and_dirty(const sint16 xpos, sint16 ypos, const sint16 raster_tile_width, const bool is_global) const
#endif
{
	const bool dirty = get_flag(grund_t::dirty);
	const uint8 start_offset=offsets[flags/has_way1];

	// here: we are either ground(kartenboden) or visible
	const bool visible = !ist_karten_boden()  ||  is_karten_boden_visible();

	if(  visible  ) {
		if(  is_global  &&  get_flag( grund_t::marked )  ) {
			const uint8 hang = get_grund_hang();
#ifdef MULTI_THREAD
			display_img( grund_besch_t::get_marker_image( hang, true ), xpos, ypos, dirty, clip_num );
			dinge.display_dinge_quick_and_dirty( xpos, ypos, start_offset, clip_num );
			display_img( grund_besch_t::get_marker_image( hang, false ), xpos, ypos, dirty, clip_num );
#else
			display_img( grund_besch_t::get_marker_image( hang, true ), xpos, ypos, dirty );
			dinge.display_dinge_quick_and_dirty( xpos, ypos, start_offset, is_global );
			display_img( grund_besch_t::get_marker_image( hang, false ), xpos, ypos, dirty );
#endif
			if(  !ist_karten_boden()  ) {
				const grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
				if(  pos.z > gr->get_hoehe()  ) {
					//display front part of marker for grunds in between
					for(  sint8 z = pos.z - 1;  z > gr->get_hoehe();  z--  ) {
#ifdef MULTI_THREAD
						display_img( grund_besch_t::get_marker_image(0, false), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), true, clip_num );
#else
						display_img( grund_besch_t::get_marker_image(0, false), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), true );
#endif
					}
					//display front part of marker for ground
#ifdef MULTI_THREAD
					display_img( grund_besch_t::get_marker_image( gr->get_grund_hang(), false ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), true, clip_num );
#else
					display_img( grund_besch_t::get_marker_image( gr->get_grund_hang(), false ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), true );
#endif
				}
				else if(  pos.z < gr->get_disp_height()  ) {
					//display back part of marker for grunds in between
					for(  sint8 z = pos.z + 1;  z < gr->get_disp_height();  z++  ) {
#ifdef MULTI_THREAD
						display_img( grund_besch_t::get_border_image(0), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), true, clip_num );
#else
						display_img( grund_besch_t::get_border_image(0), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), true );
#endif
					}
					//display back part of marker for ground
					const uint8 kbhang = gr->get_grund_hang() | gr->get_weg_hang();
#ifdef MULTI_THREAD
					display_img( grund_besch_t::get_border_image(kbhang), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), true, clip_num );
#else
					display_img( grund_besch_t::get_border_image(kbhang), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), true );
#endif
				}
			}
		}
		else {
#ifdef MULTI_THREAD
			dinge.display_dinge_quick_and_dirty( xpos, ypos, start_offset, clip_num );
#else
			dinge.display_dinge_quick_and_dirty( xpos, ypos, start_offset, is_global );
#endif
		}
	}
	else { // must be karten_boden
		// in undergroundmode: draw ground grid
		const uint8 hang = underground_mode==ugm_all ? get_grund_hang() : (uint8)hang_t::flach;
#ifdef MULTI_THREAD
		display_img( grund_besch_t::get_border_image(hang), xpos, ypos, dirty, clip_num );
#else
		display_img( grund_besch_t::get_border_image(hang), xpos, ypos, dirty );
#endif
		// show marker for marked but invisible tiles
		if(  is_global  &&  get_flag(grund_t::marked)  ) {
#ifdef MULTI_THREAD
			display_img( grund_besch_t::get_marker_image( hang, true ), xpos, ypos, dirty, clip_num );
			display_img( grund_besch_t::get_marker_image( hang, false ), xpos, ypos, dirty, clip_num );
#else
			display_img( grund_besch_t::get_marker_image( hang, true ), xpos, ypos, dirty );
			display_img( grund_besch_t::get_marker_image( hang, false ), xpos, ypos, dirty );
#endif
		}
	}
}


/* The main display routine

Premise:
-- all objects on one tile are sorted such that are no graphical glitches on the tile
-- all glitches happens with objects that are not on the tile but their graphics is (vehicles on tiles before/after stations)
-- ground graphics is already painted when this routine is called

Idea:
-- clip everything along tile borders that are crossed by ways
-- vehicles of neighboring tiles may overlap our graphic, hence we have to call display-routines of neighboring tiles too

Algorithm:
0) detect way ribis on the tile and prepare the clipping (see simgraph: add_poly_clip, display_img_pc)
1) display our background (for example station graphics background)
2) display vehicles of w/nw/n neighbors (these are behind this tile) (if we have ways in this direction; clipped along the n and/or w tile border)
3) display background of s/e neighbors (clipped - only pixels that are on our tile are painted)
4) display vehicles on this tile (clipped)
5) display vehicles of ne/e/se/s/sw neighbors
6) display our foreground (foreground image of station/overheadwire piles etc) no clipping
*/
#ifdef MULTI_THREAD
void grund_t::display_dinge_all(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const bool is_global, const sint8 clip_num) const
#else
void grund_t::display_dinge_all(const sint16 xpos, const sint16 ypos, const sint16 raster_tile_width, const bool is_global) const
#endif
{
	if(  env_t::simple_drawing  ) {
#ifdef MULTI_THREAD
		display_dinge_all_quick_and_dirty( xpos, ypos, raster_tile_width, is_global, clip_num );
#else
		display_dinge_all_quick_and_dirty( xpos, ypos, raster_tile_width, is_global );
#endif
		return;
	}

	// end of clipping
#ifdef MULTI_THREAD
	clear_all_poly_clip( clip_num );
#else
	clear_all_poly_clip();
#endif

	// here: we are either ground(kartenboden) or visible
	const bool visible = !ist_karten_boden()  ||  is_karten_boden_visible();

	ribi_t::ribi ribi = ribi_t::keine;
	if (weg_t const* const w1 = get_weg_nr(0)) {
		ribi |= w1->get_ribi_unmasked();
		if (weg_t const* const w2 = get_weg_nr(1)) {
			ribi |= w2->get_ribi_unmasked();
		}
	}
	else if (ist_wasser()) {
		ribi = (static_cast<const wasser_t*>(this))->get_weg_ribi(water_wt);
	}

	// now ways? - no clipping needed, avoid all the ribi-checks
	if (ribi==ribi_t::keine) {
		// display background
#ifdef MULTI_THREAD
		const uint8 offset_vh = display_dinge_bg( xpos, ypos, is_global, true, visible, clip_num );
#else
		const uint8 offset_vh = display_dinge_bg( xpos, ypos, is_global, true, visible );
#endif
		if (visible) {
			// display our vehicles
#ifdef MULTI_THREAD
			const uint8 offset_fg = display_dinge_vh( xpos, ypos, offset_vh, ribi, true, clip_num );
			// foreground
			display_dinge_fg( xpos, ypos, is_global, offset_fg, clip_num );
#else
			const uint8 offset_fg = display_dinge_vh( xpos, ypos, offset_vh, ribi, true );
			// foreground
			display_dinge_fg( xpos, ypos, is_global, offset_fg );
#endif
		}
		return;
	}

	// ships might be larg and could be clipped by vertical walls on our tile
	const bool ontile_se = back_bild_nr  &&  ist_wasser();

	// get slope of way as displayed
	const uint8 slope = get_disp_way_slope();
	// clip
	const int hgt_step = tile_raster_scale_y( TILE_HEIGHT_STEP, raster_tile_width);
	// .. nonconvex n/w if not both n/w are active and if we have back image
	//              otherwise our backwall clips into the part of our back image that is drawn by n/w neighbor
	const uint8 non_convex = ((ribi & ribi_t::nordwest) == ribi_t::nordwest)  &&  back_bild_nr ? 0 : 16;
	if(  ribi & ribi_t::west  ) {
		const int dh = corner4(slope) * hgt_step;
#ifdef MULTI_THREAD
		add_poly_clip( xpos + raster_tile_width / 2 - 1, ypos + raster_tile_width / 2 - dh, xpos - 1, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::west | non_convex, clip_num );
#else
		add_poly_clip( xpos + raster_tile_width / 2 - 1, ypos + raster_tile_width / 2 - dh, xpos - 1, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::west | non_convex );
#endif
	}
	if(  ribi & ribi_t::nord  ) {
		const int dh = corner4(slope) * hgt_step;
#ifdef MULTI_THREAD
		add_poly_clip( xpos + raster_tile_width - 1, ypos + 3 * raster_tile_width / 4 - 1 - dh, xpos + raster_tile_width / 2 + 1, ypos + raster_tile_width / 2 - dh, ribi_t::nord | non_convex, clip_num );
#else
		add_poly_clip( xpos + raster_tile_width - 1, ypos + 3 * raster_tile_width / 4 - 1 - dh, xpos + raster_tile_width / 2 + 1, ypos + raster_tile_width / 2 - dh, ribi_t::nord | non_convex );
#endif
	}
	if(  ribi & ribi_t::ost  ) {
		const int dh = corner2(slope) * hgt_step;
#ifdef MULTI_THREAD
		add_poly_clip( xpos + raster_tile_width / 2, ypos + raster_tile_width - dh, xpos + raster_tile_width, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::ost, clip_num );
#else
		add_poly_clip( xpos + raster_tile_width / 2, ypos + raster_tile_width - dh, xpos + raster_tile_width, ypos + 3 * raster_tile_width / 4 - dh, ribi_t::ost );
#endif
	}
	if(  ribi & ribi_t::sued  ) {
		const int dh = corner2(slope) * hgt_step;
#ifdef MULTI_THREAD
		add_poly_clip( xpos, ypos + 3 * raster_tile_width / 4 + 1 - dh, xpos + raster_tile_width / 2, ypos + raster_tile_width + 1 - dh, ribi_t::sued, clip_num );
#else
		add_poly_clip( xpos, ypos + 3 * raster_tile_width / 4 + 1 - dh, xpos + raster_tile_width / 2, ypos + raster_tile_width + 1 - dh, ribi_t::sued );
#endif
	}
	// display background
	// get offset of first vehicle
#ifdef MULTI_THREAD
	activate_ribi_clip( (ribi_t::nordwest & ribi) | 16, clip_num );
	const uint8 offset_vh = display_dinge_bg( xpos, ypos, is_global, false, visible, clip_num );
#else
	activate_ribi_clip( (ribi_t::nordwest & ribi) | 16 );
	const uint8 offset_vh = display_dinge_bg( xpos, ypos, is_global, false, visible );
#endif
	if(  !visible  ) {
		// end of clipping
#ifdef MULTI_THREAD
		clear_all_poly_clip( clip_num );
#else
		clear_all_poly_clip();
#endif
		return;
	}
	// display vehicles of w/nw/n neighbors
	grund_t *gr_nw = NULL, *gr_ne = NULL, *gr_se = NULL, *gr_sw = NULL;
	if(  ribi & ribi_t::west  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::west )  ) {
#ifdef MULTI_THREAD
			gr->display_dinge_vh( xpos - raster_tile_width / 2, ypos - raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::west, false, clip_num );
#else
			gr->display_dinge_vh( xpos - raster_tile_width / 2, ypos - raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::west, false );
#endif
			if(  ribi & ribi_t::sued  ) gr->get_neighbour( gr_nw, invalid_wt, ribi_t::nord );
			if(  ribi & ribi_t::nord  ) gr->get_neighbour( gr_sw, invalid_wt, ribi_t::sued );
		}
	}
	if(  ribi & ribi_t::nord  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::nord )  ) {
#ifdef MULTI_THREAD
			gr->display_dinge_vh( xpos + raster_tile_width / 2, ypos - raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::nord, false, clip_num );
#else
			gr->display_dinge_vh( xpos + raster_tile_width / 2, ypos - raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::nord, false );
#endif
			if(  (ribi & ribi_t::ost)  &&  (gr_nw == NULL)  ) gr->get_neighbour( gr_nw, invalid_wt, ribi_t::west );
			if(  (ribi & ribi_t::west)  )                     gr->get_neighbour( gr_ne, invalid_wt, ribi_t::ost );
		}
	}
	if(  (ribi & ribi_t::nordwest)  &&  gr_nw  ) {
#ifdef MULTI_THREAD
		gr_nw->display_dinge_vh( xpos, ypos - raster_tile_width / 2 - tile_raster_scale_y( (gr_nw->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::nordwest, false, clip_num );
#else
		gr_nw->display_dinge_vh( xpos, ypos - raster_tile_width / 2-tile_raster_scale_y( (gr_nw->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::nordwest, false );
#endif
	}
	// display background s/e
	if(  ribi & ribi_t::ost  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::ost )  ) {
			const bool draw_other_ways = (flags&draw_as_ding)  ||  (gr->flags&draw_as_ding)  ||  !gr->ist_karten_boden();
#ifdef MULTI_THREAD
			activate_ribi_clip( ribi_t::ost, clip_num );
			gr->display_dinge_bg( xpos + raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), is_global, draw_other_ways, true, clip_num );
#else
			activate_ribi_clip( ribi_t::ost );
			gr->display_dinge_bg( xpos + raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), is_global, draw_other_ways, true );
#endif
		}
	}
	if(  ribi & ribi_t::sued  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::sued )  ) {
			const bool draw_other_ways = (flags&draw_as_ding)  ||  (gr->flags&draw_as_ding)  ||  !gr->ist_karten_boden();
#ifdef MULTI_THREAD
			activate_ribi_clip( ribi_t::sued, clip_num );
			gr->display_dinge_bg( xpos - raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), is_global, draw_other_ways, true, clip_num );
#else
			activate_ribi_clip( ribi_t::sued );
			gr->display_dinge_bg( xpos - raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), is_global, draw_other_ways, true );
#endif
		}
	}
	// display our vehicles
#ifdef MULTI_THREAD
	const uint8 offset_fg = display_dinge_vh( xpos, ypos, offset_vh, ribi, true, clip_num );
#else
	const uint8 offset_fg = display_dinge_vh( xpos, ypos, offset_vh, ribi, true );
#endif

	// display vehicles of ne/e/se/s/sw neighbors
	if(  ribi & ribi_t::ost  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::ost )  ) {
#ifdef MULTI_THREAD
			gr->display_dinge_vh( xpos + raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::ost, ontile_se, clip_num );
#else
			gr->display_dinge_vh( xpos + raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::ost, ontile_se );
#endif
			if(  (ribi & ribi_t::sued)  &&  (gr_ne == NULL)  ) gr->get_neighbour( gr_ne, invalid_wt, ribi_t::nord );
			if(  (ribi & ribi_t::nord)  &&  (gr_se == NULL)  ) gr->get_neighbour( gr_se, invalid_wt, ribi_t::sued );
		}
	}
	if(  ribi & ribi_t::sued  ) {
		grund_t *gr;
		if(  get_neighbour( gr, invalid_wt, ribi_t::sued )  ) {
#ifdef MULTI_THREAD
			gr->display_dinge_vh( xpos - raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::sued, ontile_se, clip_num );
#else
			gr->display_dinge_vh( xpos - raster_tile_width / 2, ypos + raster_tile_width / 4 - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::sued, ontile_se );
#endif
			if(  (ribi & ribi_t::ost)  &&  (gr_sw == NULL))  gr->get_neighbour( gr_sw, invalid_wt, ribi_t::west );
			if(  (ribi & ribi_t::west)  &&  (gr_se == NULL)) gr->get_neighbour( gr_se, invalid_wt, ribi_t::ost );
		}
	}
	if(  (ribi & ribi_t::nordost)  &&  gr_ne  ) {
#ifdef MULTI_THREAD
		gr_ne->display_dinge_vh( xpos + raster_tile_width, ypos - tile_raster_scale_y( (gr_ne->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::nordost, ontile_se, clip_num );
#else
		gr_ne->display_dinge_vh( xpos + raster_tile_width, ypos - tile_raster_scale_y( (gr_ne->get_hoehe() - pos.z)*TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::nordost, ontile_se );
#endif
	}
	if(  (ribi & ribi_t::suedwest)  &&  gr_sw  ) {
#ifdef MULTI_THREAD
		gr_sw->display_dinge_vh( xpos - raster_tile_width, ypos - tile_raster_scale_y( (gr_sw->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::suedwest, ontile_se, clip_num );
#else
		gr_sw->display_dinge_vh( xpos - raster_tile_width, ypos - tile_raster_scale_y( (gr_sw->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::suedwest, ontile_se );
#endif
	}
	if(  (ribi & ribi_t::suedost)  &&  gr_se  ) {
#ifdef MULTI_THREAD
		gr_se->display_dinge_vh( xpos, ypos + raster_tile_width / 2 - tile_raster_scale_y( (gr_se->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::suedost, ontile_se, clip_num );
#else
		gr_se->display_dinge_vh( xpos, ypos + raster_tile_width / 2 - tile_raster_scale_y( (gr_se->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, ribi_t::suedost, ontile_se );
#endif
	}

#ifdef MULTI_THREAD
	// end of clipping
	clear_all_poly_clip( clip_num );

	// foreground
	display_dinge_fg( xpos, ypos, is_global, offset_fg, clip_num );
#else
	// end of clipping
	clear_all_poly_clip();

	// foreground
	display_dinge_fg( xpos, ypos, is_global, offset_fg );
#endif
}


#ifdef MULTI_THREAD
uint8 grund_t::display_dinge_bg(const sint16 xpos, const sint16 ypos, const bool is_global, const bool draw_ways, const bool visible, const sint8 clip_num) const
#else
uint8 grund_t::display_dinge_bg(const sint16 xpos, const sint16 ypos, const bool is_global, const bool draw_ways, const bool visible) const
#endif
{
	const bool dirty = get_flag(grund_t::dirty);

	if(  visible  ) {
		// display back part of markers
		if(  is_global  &&  get_flag( grund_t::marked )  ) {
#ifdef MULTI_THREAD
			display_normal( grund_besch_t::get_marker_image( get_grund_hang(), true ), xpos, ypos, 0, true, dirty, clip_num );
#else
			display_normal( grund_besch_t::get_marker_image( get_grund_hang(), true ), xpos, ypos, 0, true, dirty );
#endif
			if(  !ist_karten_boden()  ) {
				const grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
				const sint16 raster_tile_width = get_current_tile_raster_width();
				if(  pos.z < gr->get_disp_height()  ) {
					//display back part of marker for grunds in between
					for(  sint8 z = pos.z + 1;  z < gr->get_disp_height();  z++  ) {
#ifdef MULTI_THREAD
						display_normal( grund_besch_t::get_marker_image(0, true), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true, clip_num );
#else
						display_normal( grund_besch_t::get_marker_image(0, true), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true );
#endif
					}
					//display back part of marker for ground
#ifdef MULTI_THREAD
					display_normal( grund_besch_t::get_marker_image( gr->get_grund_hang() | gr->get_weg_hang(), true ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true, clip_num );
#else
					display_normal( grund_besch_t::get_marker_image( gr->get_grund_hang() | gr->get_weg_hang(), true ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true );
#endif
				}
			}
		}
		// display background images of everything but vehicles
		const uint8 start_offset = draw_ways ? 0 : offsets[flags/has_way1];
#ifdef MULTI_THREAD
		return dinge.display_dinge_bg( xpos, ypos, start_offset, clip_num );
#else
		return dinge.display_dinge_bg( xpos, ypos, start_offset );
#endif
	}
	else { // must be karten_boden
		// in undergroundmode: draw ground grid
		const uint8 hang = underground_mode == ugm_all ? get_grund_hang() : (hang_t::typ)hang_t::flach;
#ifdef MULTI_THREAD
		display_normal( grund_besch_t::get_border_image(hang), xpos, ypos, 0, true, dirty, clip_num );
#else
		display_normal( grund_besch_t::get_border_image(hang), xpos, ypos, 0, true, dirty );
#endif
		// show marker for marked but invisible tiles
		if(  is_global  &&  get_flag( grund_t::marked )  ) {
#ifdef MULTI_THREAD
			display_img( grund_besch_t::get_marker_image( hang, true ), xpos, ypos, dirty, clip_num );
			display_img( grund_besch_t::get_marker_image( hang, false ), xpos, ypos, dirty, clip_num );
#else
			display_img( grund_besch_t::get_marker_image( hang, true ), xpos, ypos, dirty );
			display_img( grund_besch_t::get_marker_image( hang, false ), xpos, ypos, dirty );
#endif
		}
		return 255;
	}
}


#ifdef MULTI_THREAD
uint8 grund_t::display_dinge_vh(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile, const sint8 clip_num) const
{
	return dinge.display_dinge_vh( xpos, ypos, start_offset, ribi, ontile, clip_num );
}
#else
uint8 grund_t::display_dinge_vh(const sint16 xpos, const sint16 ypos, const uint8 start_offset, const ribi_t::ribi ribi, const bool ontile) const
{
	return dinge.display_dinge_vh( xpos, ypos, start_offset, ribi, ontile );
}
#endif



#ifdef MULTI_THREAD
void grund_t::display_dinge_fg(const sint16 xpos, const sint16 ypos, const bool is_global, const uint8 start_offset, const sint8 clip_num) const
#else
void grund_t::display_dinge_fg(const sint16 xpos, const sint16 ypos, const bool is_global, const uint8 start_offset) const
#endif
{
	const bool dirty = get_flag(grund_t::dirty);
#ifdef MULTI_THREAD
	dinge.display_dinge_fg( xpos, ypos, start_offset, clip_num );
#else
	dinge.display_dinge_fg( xpos, ypos, start_offset, is_global );
#endif
	// display front part of markers
	if(  is_global  &&  get_flag( grund_t::marked )  ) {
#ifdef MULTI_THREAD
		display_normal( grund_besch_t::get_marker_image( get_grund_hang(), false ), xpos, ypos, 0, true, dirty, clip_num );
#else
		display_normal( grund_besch_t::get_marker_image( get_grund_hang(), false ), xpos, ypos, 0, true, dirty );
#endif
		if(  !ist_karten_boden()  ) {
			const grund_t *gr = welt->lookup_kartenboden(pos.get_2d());
			const sint16 raster_tile_width = get_tile_raster_width();
			if(  pos.z > gr->get_hoehe()  ) {
				//display front part of marker for grunds in between
				for(  sint8 z = pos.z - 1;  z > gr->get_hoehe();  z--  ) {
#ifdef MULTI_THREAD
					display_normal( grund_besch_t::get_marker_image( 0, false ), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true, clip_num );
#else
					display_normal( grund_besch_t::get_marker_image( 0, false ), xpos, ypos - tile_raster_scale_y( (z - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true );
#endif
				}
				//display front part of marker for ground
#ifdef MULTI_THREAD
				display_normal( grund_besch_t::get_marker_image( gr->get_grund_hang(), false ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true, clip_num );
#else
				display_normal( grund_besch_t::get_marker_image( gr->get_grund_hang(), false ), xpos, ypos - tile_raster_scale_y( (gr->get_hoehe() - pos.z) * TILE_HEIGHT_STEP, raster_tile_width ), 0, true, true );
#endif
			}
		}
	}
}


void grund_t::display_overlay(const sint16 xpos, const sint16 ypos)
{
	const bool dirty = get_flag(grund_t::dirty);
#ifdef MULTI_THREAD
	dinge.display_dinge_overlay( xpos, ypos );
#endif
	// marker/station text
	if(  get_flag(has_text)  &&  env_t::show_names  ) {
		if(  env_t::show_names&1  ) {
			const char *text = get_text();
			const sint16 raster_tile_width = get_tile_raster_width();
			const int width = proportional_string_width(text)+7;
			int new_xpos = xpos - (width-raster_tile_width)/2;
			PLAYER_COLOR_VAL pc = text_farbe();

			switch( env_t::show_names >> 2 ) {
				case 0:
					display_ddd_proportional_clip( new_xpos, ypos, width, 0, pc, COL_BLACK, text, dirty );
					break;
				case 1:
					display_outline_proportional( new_xpos, ypos-(LINESPACE/2), pc+3, COL_BLACK, text, dirty );
					break;
				case 2:
					display_outline_proportional( 16+new_xpos, ypos-(LINESPACE/2), COL_YELLOW, COL_BLACK, text, dirty );
					display_ddd_box_clip( new_xpos, ypos-(LINESPACE/2), LINESPACE, LINESPACE, pc-2, pc+2 );
					display_fillbox_wh( new_xpos+1, ypos-(LINESPACE/2)+1, LINESPACE-2, LINESPACE-2, pc, dirty );
					break;
			}
		}

		// display station waiting information/status
		if(env_t::show_names & 2) {
			const halthandle_t halt = get_halt();
			if(halt.is_bound()  &&  halt->get_basis_pos3d()==pos) {
				halt->display_status(xpos, ypos);
			}
		}
	}

#ifdef SHOW_FORE_GRUND
	if(get_flag(grund_t::draw_as_ding)) {
		const sint16 raster_tile_width = get_tile_raster_width();
		display_fillbox_wh_clip( xpos+raster_tile_width/2, ypos+(raster_tile_width*3)/4, 16, 16, 0, dirty);
	}
#endif

	clear_flag(grund_t::dirty);
}


// ---------------- way and wayobj handling -------------------------------


ribi_t::ribi grund_t::get_weg_ribi(waytype_t typ) const
{
	weg_t *weg = get_weg(typ);
	return (weg) ? weg->get_ribi() : (ribi_t::ribi)ribi_t::keine;
}


ribi_t::ribi grund_t::get_weg_ribi_unmasked(waytype_t typ) const
{
	weg_t *weg = get_weg(typ);
	return (weg) ? weg->get_ribi_unmasked() : (ribi_t::ribi)ribi_t::keine;
}


/**
* Falls es hier ein Depot gibt, dieses zurueckliefern
* @author Volker Meyer
*/
depot_t* grund_t::get_depot() const
{
	return dynamic_cast<depot_t *>(first_obj());
}


bool grund_t::weg_erweitern(waytype_t wegtyp, ribi_t::ribi ribi)
{
	weg_t   *weg = get_weg(wegtyp);
	if(weg) {
		weg->ribi_add(ribi);
		weg->count_sign();
		if(weg->is_electrified()) {
			wayobj_t *wo = get_wayobj( wegtyp );
			if( (ribi & wo->get_dir()) == 0 ) {
				// ribi isn't set at wayobj;
				for( uint8 i = 0; i < 4; i++ ) {
					// Add ribis to adjacent wayobj.
					if( ribi_t::nsow[i] & ribi ) {
						grund_t *next_gr;
						if( get_neighbour( next_gr, wegtyp, ribi_t::nsow[i] ) ) {
							wayobj_t *wo2 = next_gr->get_wayobj( wegtyp );
							if( wo2 ) {
								wo->set_dir( wo->get_dir() | ribi_t::nsow[i] );
								wo2->set_dir( wo2->get_dir() | ribi_t::rueckwaerts(ribi_t::nsow[i]) );
							}
						}
					}
				}
			}
		}
		calc_bild();
		set_flag(dirty);
		return true;
	}
	return false;
}

/**
 * remove trees and groundobjs on this tile
 * called before building way or powerline
 * @return costs
 */
sint64 grund_t::remove_trees()
{
	sint64 cost=0;
	// remove all trees ...
	while (baum_t* const d = find<baum_t>(0)) {
		// we must mark it by hand, sinc ewe want to join costs
		d->mark_image_dirty( get_bild(), 0 );
		delete d;
		cost -= welt->get_settings().cst_remove_tree;
	}
	// remove all groundobjs ...
	while (groundobj_t* const d = find<groundobj_t>(0)) {
		cost += d->get_besch()->get_preis();
		delete d;
	}
	return cost;
}


sint64 grund_t::neuen_weg_bauen(weg_t *weg, ribi_t::ribi ribi, spieler_t *sp)
{
	sint64 cost=0;

	// not already there?
	const weg_t * alter_weg = get_weg(weg->get_waytype());
	if(alter_weg==NULL) {
		// ok, we are unique

		if((flags&has_way1)==0) {
			// new first way here, clear trees
			cost += remove_trees();

			// add
			weg->set_ribi(ribi);
			weg->set_pos(pos);
			dinge.add( weg );
			flags |= has_way1;
		}
		else {
			weg_t *other = (weg_t *)obj_bei(0);
			// another way will be added
			if(flags&has_way2) {
				dbg->fatal("grund_t::neuen_weg_bauen()","cannot built more than two ways on %i,%i,%i!",pos.x,pos.y,pos.z);
				return 0;
			}
			// add the way
			dinge.add( weg );
			weg->set_ribi(ribi);
			weg->set_pos(pos);
			flags |= has_way2;
			if(ist_uebergang()) {
				// no tram => crossing needed!
				waytype_t w2 =  other->get_waytype();
				const kreuzung_besch_t *cr_besch = crossing_logic_t::get_crossing( weg->get_waytype(), w2, weg->get_max_speed(), other->get_besch()->get_topspeed(), welt->get_timeline_year_month() );
				if(cr_besch==0) {
					dbg->fatal("crossing_t::crossing_t()","requested for waytypes %i and %i but nothing defined!", weg->get_waytype(), w2 );
				}
				crossing_t *cr = new crossing_t( welt, obj_bei(0)->get_besitzer(), pos, cr_besch, ribi_t::ist_gerade_ns(get_weg(cr_besch->get_waytype(1))->get_ribi_unmasked()) );
				dinge.add( cr );
				cr->laden_abschliessen();
			}
		}

		// just add the maintenance
		if(sp && !ist_wasser()) {
			spieler_t::add_maintenance( sp, weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype() );
			weg->set_besitzer( sp );
		}

		// may result in a crossing, but the wegebauer will recalc all images anyway
		weg->calc_bild();
	}
	return cost;
}


sint32 grund_t::weg_entfernen(waytype_t wegtyp, bool ribi_rem)
{
DBG_MESSAGE("grund_t::weg_entfernen()","this %p",this);
	weg_t *weg = get_weg(wegtyp);
DBG_MESSAGE("grund_t::weg_entfernen()","weg %p",weg);
	if(weg!=NULL) {

		if(ribi_rem) {
			ribi_t::ribi ribi = weg->get_ribi();
			grund_t *to;

			for(int r = 0; r < 4; r++) {
				if((ribi & ribi_t::nsow[r]) && get_neighbour(to, wegtyp, ribi_t::nsow[r])) {
					weg_t *weg2 = to->get_weg(wegtyp);
					if(weg2) {
						weg2->ribi_rem(ribi_t::rueckwaerts(ribi_t::nsow[r]));
						to->calc_bild();
					}
				}
			}
		}

		sint32 costs=weg->get_besch()->get_preis();	// costs for removal are construction costs
		weg->entferne( NULL );
		delete weg;

		// delete the second way ...
		if(flags&has_way2) {
			flags &= ~has_way2;

			// reset speed limit/crossing info (maybe altered by crossing)
			// Not all ways (i.e. with styp==7) will imply crossins, so wie hav to check
			crossing_t* cr = find<crossing_t>(1);
			if(cr) {
				cr->entferne(0);
				delete cr;
				// restore speed limit
				weg_t* w = (weg_t*)obj_bei(0);
				w->set_besch(w->get_besch());
				w->count_sign();
			}
		}
		else {
			flags &= ~has_way1;
		}

		calc_bild();
		reliefkarte_t::get_karte()->calc_map_pixel(get_pos().get_2d());

		return costs;
	}
	return 0;
}


// this funtion is called many many times => make it as fast as possible
// i.e. no reverse lookup of ribis from koord
bool grund_t::get_neighbour(grund_t *&to, waytype_t type, ribi_t::ribi ribi) const
{
	// must be a single direction
	assert( ribi_t::ist_einfach(ribi) );

	if (type != invalid_wt   &&   (get_weg_ribi_unmasked(type) & ribi) == 0) {
		// no way on this tile in the given direction
		return false;
	}

	const planquadrat_t * plan = welt->access(pos.get_2d() + koord(ribi) );
	if(!plan) {
		return false;
	}
	const ribi_t::ribi back = ribi_t::rueckwaerts(ribi);

	// most common on empty ground => check this first
	if(  get_grund_hang() == hang_t::flach  &&  get_weg_hang() == hang_t::flach  ) {
		if(  grund_t *gr = plan->get_boden_in_hoehe( pos.z )  ) {
			if(  gr->get_grund_hang() == hang_t::flach  &&  gr->get_weg_hang() == hang_t::flach  ) {
				if(  type == invalid_wt  ||  (gr->get_weg_ribi_unmasked(type) & back)  ) {
					to = gr;
					return true;
				}
			}
		}
	}

	// most common on empty round => much faster this way
	const sint16 this_height = get_vmove(ribi);
	for( unsigned i=0;  i<plan->get_boden_count();  i++  ) {
		grund_t* gr = plan->get_boden_bei(i);
		if(gr->get_vmove(back)==this_height) {
			// test, if connected
			if(  type == invalid_wt  ||  (gr->get_weg_ribi_unmasked(type) & back)  ) {
				to = gr;
				return true;
			}
		}
	}
	return false;
}


int grund_t::get_max_speed() const
{
	int max = 0;
	if (weg_t const* const w = get_weg_nr(0)) {
		max = w->get_max_speed();
	}
	if (weg_t const* const w = get_weg_nr(1)) {
		max = min(max, w->get_max_speed());
	}
	return max;
}


bool grund_t::remove_everything_from_way(spieler_t* sp, waytype_t wt, ribi_t::ribi rem)
{
	// check, if the way must be totally removed?
	weg_t *weg = get_weg(wt);
	if(weg) {
		waytype_t wt = weg->get_waytype();
		waytype_t finance_wt = weg->get_besch()->get_finance_waytype();
		const koord here = pos.get_2d();

		// stopps
		if(flags&is_halt_flag  &&  (get_halt()->get_besitzer()==sp  || sp==welt->get_spieler(1))) {
			bool remove_halt = get_typ()!=boden;
			// remove only if there is no other way
			if(get_weg_nr(1)==NULL) {
				remove_halt = true;
			}
			else {
#ifdef delete_matching_stops
				// delete halts with the same waytype ... may lead to confusing / unexpected but completely logical results ;)
				gebaeude_t *gb = find<gebaeude_t>();
				if (gb) {
					waytype_t halt_wt = (waytype_t)gb->get_tile()->get_besch()->get_extra();
					if (halt_wt == wt || (wt==track_wt && halt_wt==tram_wt) || (wt==tram_wt && halt_wt==track_wt)) {
						remove_halt = true;
					}
				}
#else
				remove_halt = false;
#endif
			}
			if (remove_halt) {
				if (!haltestelle_t::remove(welt, sp, pos)) {
					return false;
				}
			}
		}
		// remove ribi from canals to sea level
		if(  wt == water_wt  &&  pos.z == welt->get_water_hgt( here )  &&  slope != hang_t::flach  ) {
			rem &= ~ribi_t::doppelt(ribi_typ(slope));
		}

		ribi_t::ribi add=(weg->get_ribi_unmasked()&rem);
		sint32 costs = 0;

		bool signs_deleted = false;

		for(  sint16 i=get_top();  i>=0;  i--  ) {
			// we need to delete backwards, since we might miss things otherwise
			if(  i>=get_top()  ) {
				continue;
			}

			ding_t *d=obj_bei((uint8)i);
			// do not delete ways
			if(  d->get_typ()==ding_t::way  ) {
				continue;
			}
			if (roadsign_t* const sign = ding_cast<roadsign_t>(d)) {
				// roadsigns: check dir: dirs changed => delete
				if (sign->get_besch()->get_wtyp() == wt && (sign->get_dir() & ~add) != 0) {
					costs -= sign->get_besch()->get_preis();
					delete sign;
					signs_deleted = true;
				}
			} else if (signal_t* const signal = ding_cast<signal_t>(d)) {
				// singal: not on crossings => remove all
				if (signal->get_besch()->get_wtyp() == wt) {
					costs -= signal->get_besch()->get_preis();
					delete signal;
					signs_deleted = true;
				}
			} else if (wayobj_t* const wayobj = ding_cast<wayobj_t>(d)) {
				// wayobj: check dir
				if (add == ribi_t::keine && wayobj->get_besch()->get_wtyp() == wt) {
					uint8 new_dir=wayobj->get_dir()&add;
					if(new_dir) {
						// just change dir
						wayobj->set_dir(new_dir);
					}
					else {
						costs -= wayobj->get_besch()->get_preis();
						delete wayobj;
					}
				}
			} else if (stadtauto_t* const citycar = ding_cast<stadtauto_t>(d)) {
				// citycar: just delete
				if (wt == road_wt) delete citycar;
			} else if (fussgaenger_t* const pedestrian = ding_cast<fussgaenger_t>(d)) {
				// pedestrians: just delete
				if (wt == road_wt) delete pedestrian;
			} else if (tunnel_t* const tunnel = ding_cast<tunnel_t>(d)) {
				// remove tunnel portal, if not the last tile ...
				// must be done before weg_entfernen() to get maintenance right
				uint8 wt = tunnel->get_besch()->get_waytype();
				if (weg->get_waytype()==wt) {
					if((flags&has_way2)==0) {
						if (add==ribi_t::keine) {
							// last way was belonging to this tunnel
							tunnel->entferne(sp);
							delete tunnel;
						}
					}
					else {
						// we must leave the way to prevent destroying the other one
						add |= get_weg_nr(1)->get_ribi_unmasked();
						weg->calc_bild();
					}
				}
			}
		}

		// need to remove railblocks to recalcualte connections
		// remove all ways or just some?
		if(add==ribi_t::keine) {
			costs -= weg_entfernen(wt, true);
			if(flags&is_kartenboden) {
				// remove ribis from sea tiles
				if(  wt == water_wt  &&  pos.z == welt->get_water_hgt( here )  &&  slope != hang_t::flach  ) {
					grund_t *gr = welt->lookup_kartenboden(here - ribi_typ(slope));
					if (gr  &&  gr->ist_wasser()) {
						gr->calc_bild(); // to recalculate ribis
					}
				}
				// make tunnel portals to normal ground
				if (get_typ()==tunnelboden  &&  (flags&has_way1)==0) {
					// remove remaining dings
					obj_loesche_alle( sp );
					// set to normal ground
					welt->access(here)->kartenboden_setzen( new boden_t( welt, pos, slope ) );
					// now this is already deleted !
				}
			}
		}
		else {
DBG_MESSAGE("wkz_wayremover()","change remaining way to ribi %d",add);
			// something will remain, we just change ribis
			weg->set_ribi(add);
			if (signs_deleted) {
				weg->count_sign();
			}
			calc_bild();
		}
		// we have to pay?
		if(costs) {
			spieler_t::book_construction_costs(sp, costs, here, finance_wt);
		}
	}
	return true;
}


wayobj_t *grund_t::get_wayobj( waytype_t wt ) const
{
	waytype_t wt1 = ( wt == tram_wt ) ? track_wt : wt;

	// since there might be more than one, we have to iterate through all of them
	for(  uint8 i = 0;  i < get_top();  i++  ) {
		ding_t *d = obj_bei(i);
		if (wayobj_t* const wayobj = ding_cast<wayobj_t>(d)) {
			waytype_t wt2 = wayobj->get_besch()->get_wtyp();
			if(  wt2 == tram_wt  ) {
				wt2 = track_wt;
			}
			if(  wt1 == wt2  ) {
				return wayobj;
			}
		}
	}
	return NULL;
}
