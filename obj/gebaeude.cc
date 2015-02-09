/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <ctype.h>

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t add_to_city_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


#include "../bauer/hausbauer.h"
#include "../gui/money_frame.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../simfab.h"
#include "../display/simimg.h"
#include "../display/simgraph.h"
#include "../simhalt.h"
#include "../gui/simwin.h"
#include "../simcity.h"
#include "../player/simplay.h"
#include "../simdebug.h"
#include "../simintr.h"
#include "../simskin.h"

#include "../boden/grund.h"


#include "../besch/haus_besch.h"
#include "../besch/intro_dates.h"

#include "../besch/grund_besch.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simrandom.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"

#include "../gui/obj_info.h"

#include "gebaeude.h"


/**
 * Initializes all variables with save, usable values
 * @author Hj. Malthaner
 */
void gebaeude_t::init()
{
	tile = NULL;
	anim_time = 0;
	sync = false;
	zeige_baugrube = false;
	is_factory = false;
	season = 0;
	background_animated = false;
	remove_ground = true;
	anim_frame = 0;
//	insta_zeit = 0; // init in set_tile()
	ptr.fab = NULL;
}


gebaeude_t::gebaeude_t() : obj_t()
{
	init();
}


gebaeude_t::gebaeude_t(loadsave_t *file) : obj_t()
{
	init();
	rdwr(file);
	if(file->get_version()<88002) {
		set_yoff(0);
	}
	if(tile  &&  tile->get_phasen()>1) {
		welt->sync_eyecandy_add( this );
		sync = true;
	}
}


gebaeude_t::gebaeude_t(koord3d pos, player_t *player, const haus_tile_besch_t *t) :
    obj_t(pos)
{
	set_owner( player );

	init();
	if(t) {
		set_tile(t,true);	// this will set init time etc.
		player_t::add_maintenance(get_owner(), tile->get_besch()->get_maintenance(welt), tile->get_besch()->get_finance_waytype() );
	}

	// get correct y offset for bridges
	grund_t *gr=welt->lookup(pos);
	if(gr  &&  gr->get_weg_hang()!=gr->get_grund_hang()) {
		set_yoff( -gr->get_weg_yoff() );
	}
}


/**
 * Destructor. Removes this from the list of sync objects if necessary.
 *
 * @author Hj. Malthaner
 */
gebaeude_t::~gebaeude_t()
{
	if(get_stadt()) {
		ptr.stadt->remove_gebaeude_from_stadt(this);
	}

	if(sync) {
		sync = false;
		welt->sync_eyecandy_remove(this);
	}

	// tiles might be invalid, if no description is found during loading
	if(tile  &&  tile->get_besch()  &&  tile->get_besch()->ist_ausflugsziel()) {
		welt->remove_ausflugsziel(this);
	}

	if(tile) {
		player_t::add_maintenance(get_owner(), -tile->get_besch()->get_maintenance(welt), tile->get_besch()->get_finance_waytype());
	}
}


void gebaeude_t::rotate90()
{
	obj_t::rotate90();

	// must or can rotate?
	const haus_besch_t* const haus_besch = tile->get_besch();
	if (haus_besch->get_all_layouts() > 1  ||  haus_besch->get_b() * haus_besch->get_h() > 1) {
		uint8 layout = tile->get_layout();
		koord new_offset = tile->get_offset();

		if(haus_besch->get_utyp() == haus_besch_t::unbekannt  ||  haus_besch->get_all_layouts()<=4) {
			layout = (layout & 4) + ((layout+3) % haus_besch->get_all_layouts() & 3);
		}
		else {
			static uint8 layout_rotate[16] = { 1, 8, 5, 10, 3, 12, 7, 14, 9, 0, 13, 2, 11, 4, 15, 6 };
			layout = layout_rotate[layout] % haus_besch->get_all_layouts();
		}
		// have to rotate the tiles :(
		if(  !haus_besch->can_rotate()  &&  haus_besch->get_all_layouts() == 1  ) {
			if ((welt->get_settings().get_rotation() & 1) == 0) {
				// rotate 180 degree
				new_offset = koord(haus_besch->get_b() - 1 - new_offset.x, haus_besch->get_h() - 1 - new_offset.y);
			}
			// do nothing here, since we cannot fix it properly
		}
		else {
			// rotate on ...
			new_offset = koord(haus_besch->get_h(tile->get_layout()) - 1 - new_offset.y, new_offset.x);
		}

		// such a tile exist?
		if(  haus_besch->get_b(layout) > new_offset.x  &&  haus_besch->get_h(layout) > new_offset.y  ) {
			const haus_tile_besch_t* const new_tile = haus_besch->get_tile(layout, new_offset.x, new_offset.y);
			// add new tile: but make them old (no construction)
			uint32 old_insta_zeit = insta_zeit;
			set_tile( new_tile, false );
			insta_zeit = old_insta_zeit;
			if(  haus_besch->get_utyp() != haus_besch_t::dock  &&  !tile->has_image()  ) {
				// may have a rotation, that is not recoverable
				if(  !is_factory  &&  new_offset!=koord(0,0)  ) {
					welt->set_nosave_warning();
				}
				if(  is_factory  ) {
					// there are factories with a broken tile
					// => this map rotation cannot be reloaded!
					welt->set_nosave();
				}
			}
		}
		else {
			welt->set_nosave();
		}
	}
}


/* sets the corresponding pointer to a factory
 * @author prissi
 */
void gebaeude_t::set_fab(fabrik_t *fb)
{
	// sets the pointer in non-zero
	if(fb) {
		if(!is_factory  &&  ptr.stadt!=NULL) {
			dbg->fatal("gebaeude_t::set_fab()","building already bound to city!");
		}
		is_factory = true;
		ptr.fab = fb;
	}
	else if(is_factory) {
		ptr.fab = NULL;
	}
}


/* sets the corresponding city
 * @author prissi
 */
void gebaeude_t::set_stadt(stadt_t *s)
{
	if(is_factory  &&  ptr.fab!=NULL) {
		dbg->fatal("gebaeude_t::set_stadt()","building at (%s) already bound to factory!", get_pos().get_str() );
	}
	// sets the pointer in non-zero
	is_factory = false;
	ptr.stadt = s;
}


/* make this building without construction */
void gebaeude_t::add_alter(uint32 a)
{
	insta_zeit -= min(a,insta_zeit);
}


void gebaeude_t::set_tile( const haus_tile_besch_t *new_tile, bool start_with_construction )
{
	insta_zeit = welt->get_zeit_ms();

	if(!zeige_baugrube  &&  tile!=NULL) {
		// mark old tile dirty
		mark_images_dirty();
	}

	zeige_baugrube = !new_tile->get_besch()->ist_ohne_grube()  &&  start_with_construction;
	if(sync) {
		if(new_tile->get_phasen()<=1  &&  !zeige_baugrube) {
			// need to stop animation
#ifdef MULTI_THREAD
			pthread_mutex_lock( &sync_mutex );
#endif
			welt->sync_eyecandy_remove(this);
			sync = false;
			anim_frame = 0;
#ifdef MULTI_THREAD
			pthread_mutex_unlock( &sync_mutex );
#endif
		}
	}
	else if(new_tile->get_phasen()>1  ||  zeige_baugrube) {
		// needs now animation
#ifdef MULTI_THREAD
		pthread_mutex_lock( &sync_mutex );
#endif
		anim_frame = sim_async_rand( new_tile->get_phasen() );
		anim_time = 0;
		welt->sync_eyecandy_add(this);
		sync = true;
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &sync_mutex );
#endif
	}
	tile = new_tile;
	remove_ground = tile->has_image()  &&  !tile->get_besch()->ist_mit_boden();
	set_flag(obj_t::dirty);
}


/**
 * Methode f�r Echtzeitfunktionen eines Objekts.
 * @return false wenn Objekt aus der Liste der synchronen
 * Objekte entfernt werden sol
 * @author Hj. Malthaner
 */
bool gebaeude_t::sync_step(uint32 delta_t)
{
	if(  zeige_baugrube  ) {
		// still under construction?
		if(  welt->get_zeit_ms() - insta_zeit > 5000  ) {
			set_flag( obj_t::dirty );
			mark_image_dirty( get_image(), 0 );
			zeige_baugrube = false;
			if(  tile->get_phasen() <= 1  ) {
				welt->sync_eyecandy_remove( this );
				sync = false;
			}
		}
	}
	else {
		// normal animated building
		anim_time += delta_t;
		if(  anim_time > tile->get_besch()->get_animation_time()  ) {
			anim_time -= tile->get_besch()->get_animation_time();

			// old positions need redraw
			if(  background_animated  ) {
				set_flag( obj_t::dirty );
				mark_images_dirty();
			}
			else {
				// try foreground
				image_id bild = tile->get_vordergrund( anim_frame, season );
				mark_image_dirty( bild, 0 );
			}

			anim_frame++;
			if(  anim_frame >= tile->get_phasen()  ) {
				anim_frame = 0;
			}

			if(  !background_animated  ) {
				// next phase must be marked dirty too ...
				image_id bild = tile->get_vordergrund( anim_frame, season );
				mark_image_dirty( bild, 0 );
			}
 		}
 	}
	return true;
}


void gebaeude_t::calc_image()
{
	grund_t *gr = welt->lookup( get_pos() );
	// need no ground?
	if(  remove_ground  &&  gr->get_typ() == grund_t::fundament  ) {
		gr->set_bild( IMG_LEER );
	}

	static uint8 effective_season[][5] = { {0,0,0,0,0}, {0,0,0,0,1}, {0,0,0,0,1}, {0,1,2,3,2}, {0,1,2,3,4} };  // season image lookup from [number of images] and [actual season/snow]

	if(  (gr->ist_tunnel()  &&  !gr->ist_karten_boden())  ||  tile->get_seasons() < 2  ) {
		season = 0;
	}
	else if(  get_pos().z - (get_yoff() / TILE_HEIGHT_STEP) >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate  ) {
		// snowy winter graphics
		season = effective_season[tile->get_seasons()][4];
	}
	else if(  get_pos().z - (get_yoff() / TILE_HEIGHT_STEP) >= welt->get_snowline() - 1  &&  welt->get_season() == 0  ) {
		// snowline crossing in summer
		// so at least some weeks spring/autumn
		season = effective_season[tile->get_seasons()][welt->get_last_month() <= 5 ? 3 : 1];
	}
	else {
		season = effective_season[tile->get_seasons()][welt->get_season()];
	}

	background_animated = tile->is_hintergrund_phases( season );
}


image_id gebaeude_t::get_image() const
{
	if(env_t::hide_buildings!=0  &&  tile->has_image()) {
		// opaque houses
		if(get_haustyp()!=unbekannt) {
			return env_t::hide_with_transparency ? skinverwaltung_t::fussweg->get_bild_nr(0) : skinverwaltung_t::construction_site->get_bild_nr(0);
		} else if(  (env_t::hide_buildings == env_t::ALL_HIDDEN_BUILDING  &&  tile->get_besch()->get_utyp() < haus_besch_t::weitere)) {
			// hide with transparency or tile without information
			if(env_t::hide_with_transparency) {
				if(tile->get_besch()->get_utyp() == haus_besch_t::fabrik  &&  ptr.fab->get_besch()->get_platzierung() == fabrik_besch_t::Wasser) {
					// no ground tiles for water things
					return IMG_LEER;
				}
				return skinverwaltung_t::fussweg->get_bild_nr(0);
			}
			else {
				uint16 kind=skinverwaltung_t::construction_site->get_bild_anzahl()<=tile->get_besch()->get_utyp() ? skinverwaltung_t::construction_site->get_bild_anzahl()-1 : tile->get_besch()->get_utyp();
				return skinverwaltung_t::construction_site->get_bild_nr( kind );
			}
		}
	}

	if(  zeige_baugrube  )  {
		return skinverwaltung_t::construction_site->get_bild_nr(0);
	}
	else {
		return tile->get_hintergrund( anim_frame, 0, season );
	}
}


image_id gebaeude_t::get_outline_image() const
{
	if(env_t::hide_buildings!=0  &&  env_t::hide_with_transparency  &&  !zeige_baugrube) {
		// opaque houses
		return tile->get_hintergrund( anim_frame, 0, season );
	}
	return IMG_LEER;
}


/* gives outline colour and plots background tile if needed for transparent view */
PLAYER_COLOR_VAL gebaeude_t::get_outline_colour() const
{
	COLOR_VAL colours[] = { COL_BLACK, COL_YELLOW, COL_YELLOW, COL_PURPLE, COL_RED, COL_GREEN };
	PLAYER_COLOR_VAL disp_colour = 0;
	if(env_t::hide_buildings!=env_t::NOT_HIDE) {
		if(get_haustyp()!=unbekannt) {
			disp_colour = colours[0] | TRANSPARENT50_FLAG | OUTLINE_FLAG;
		}
		else if (env_t::hide_buildings == env_t::ALL_HIDDEN_BUILDING && tile->get_besch()->get_utyp() < haus_besch_t::weitere) {
			// special building
			disp_colour = colours[tile->get_besch()->get_utyp()] | TRANSPARENT50_FLAG | OUTLINE_FLAG;
		}
	}
	return disp_colour;
}


image_id gebaeude_t::get_image(int nr) const
{
	if(zeige_baugrube || env_t::hide_buildings) {
		return IMG_LEER;
	}
	else {
		return tile->get_hintergrund( anim_frame, nr, season );
	}
}


image_id gebaeude_t::get_front_image() const
{
	if(zeige_baugrube) {
		return IMG_LEER;
	}
	if (env_t::hide_buildings != 0 && tile->get_besch()->get_utyp() < haus_besch_t::weitere) {
		return IMG_LEER;
	}
	else {
		// Show depots, station buildings etc.
		return tile->get_vordergrund( anim_frame, season );
	}
}


/**
 * calculate the passenger level as function of the city size (if there)
 */
int gebaeude_t::get_passagier_level() const
{
	koord dim = tile->get_besch()->get_groesse();
	long pax = tile->get_besch()->get_level();
	if(  !is_factory  &&  ptr.stadt != NULL  ) {
		// belongs to a city ...
		return ((pax + 6) >> 2) * welt->get_settings().get_passenger_factor() / 16;
	}
	return pax*dim.x*dim.y;
}


int gebaeude_t::get_post_level() const
{
	koord dim = tile->get_besch()->get_groesse();
	long post = tile->get_besch()->get_post_level();
	if(  !is_factory  &&  ptr.stadt != NULL  ) {
		return ((post + 5) >> 2) * welt->get_settings().get_passenger_factor() / 16;
	}
	return post*dim.x*dim.y;
}


/**
 * @return eigener Name oder Name der Fabrik falls Teil einer Fabrik
 * @author Hj. Malthaner
 */
const char *gebaeude_t::get_name() const
{
	if(is_factory  &&  ptr.fab) {
		return ptr.fab->get_name();
	}
	switch(tile->get_besch()->get_typ()) {
		case wohnung:
			break;//return "Wohnhaus";
		case gewerbe:
			break;//return "Gewerbehaus";
		case industrie:
			break;//return "Industriegeb�ude";
		default:
			switch(tile->get_besch()->get_utyp()) {
				case haus_besch_t::attraction_city:   return "Besonderes Gebaeude";
				case haus_besch_t::attraction_land:   return "Sehenswuerdigkeit";
				case haus_besch_t::denkmal:           return "Denkmal";
				case haus_besch_t::rathaus:           return "Rathaus";
				default: break;
			}
			break;
	}
	return "Gebaeude";
}


/**
 * waytype associated with this object
 */
waytype_t gebaeude_t::get_waytype() const
{
	const haus_besch_t *besch = tile->get_besch();
	waytype_t wt = invalid_wt;
	if (besch->get_typ() == gebaeude_t::unbekannt) {
		const haus_besch_t::utyp utype = besch->get_utyp();
		if (utype == haus_besch_t::depot  ||  utype == haus_besch_t::generic_stop  ||  utype == haus_besch_t::generic_extension) {
			wt = (waytype_t)besch->get_extra();
		}
	}
	return wt;
}


bool gebaeude_t::ist_rathaus() const
{
	return tile->get_besch()->ist_rathaus();
}


bool gebaeude_t::is_monument() const
{
	return tile->get_besch()->get_utyp() == haus_besch_t::denkmal;
}


bool gebaeude_t::ist_firmensitz() const
{
	return tile->get_besch()->ist_firmensitz();
}


gebaeude_t::typ gebaeude_t::get_haustyp() const
{
	return tile->get_besch()->get_typ();
}


void gebaeude_t::show_info()
{
	if(get_fabrik()) {
		ptr.fab->zeige_info();
		return;
	}
	int old_count = win_get_open_count();
	bool special = ist_firmensitz() || ist_rathaus();

	if(ist_firmensitz()) {
		create_win( new money_frame_t(get_owner()), w_info, magic_finances_t+get_owner()->get_player_nr() );
	}
	else if (ist_rathaus()) {
		ptr.stadt->zeige_info();
	}

	if(!tile->get_besch()->ist_ohne_info()) {
		if(!special  ||  (env_t::townhall_info  &&  old_count==win_get_open_count()) ) {
			// open info window for the first tile of our building (not relying on presence of (0,0) tile)
			get_first_tile()->obj_t::show_info();
		}
	}
}


bool gebaeude_t::is_same_building(gebaeude_t* other)
{
	return (other != NULL)  &&  (get_tile()->get_besch() == other->get_tile()->get_besch())
	       &&  (get_first_tile() == other->get_first_tile());
}


gebaeude_t* gebaeude_t::get_first_tile()
{
	const haus_besch_t* const haus_besch = tile->get_besch();
	const uint8 layout = tile->get_layout();
	koord k;
	for(k.x=0; k.x<haus_besch->get_b(layout); k.x++) {
		for(k.y=0; k.y<haus_besch->get_h(layout); k.y++) {
			const haus_tile_besch_t *tile = haus_besch->get_tile(layout, k.x, k.y);
			if (tile==NULL  ||  !tile->has_image()) {
				continue;
			}
			if (grund_t *gr = welt->lookup( get_pos() - get_tile()->get_offset() + k)) {
				gebaeude_t *gb = gr->find<gebaeude_t>();
				if (gb  &&  gb->get_tile() == tile) {
					return gb;
				}
			}
		}
	}
	return this;
}


void gebaeude_t::info(cbuffer_t & buf) const
{
	obj_t::info(buf);

	if(is_factory  &&  ptr.fab != NULL) {
		buf.append((char *)0);
	}
	else if(zeige_baugrube) {
		buf.append(translator::translate("Baustelle"));
		buf.append("\n");
	}
	else {
		const char *desc = tile->get_besch()->get_name();
		if(desc != NULL) {
			const char *trans_desc = translator::translate(desc);
			if(trans_desc==desc) {
				// no description here
				switch(get_haustyp()) {
					case wohnung:
						trans_desc = translator::translate("residential house");
						break;
					case industrie:
						trans_desc = translator::translate("industrial building");
						break;
					case gewerbe:
						trans_desc = translator::translate("shops and stores");
						break;
					default:
						// use file name
						break;
				}
				buf.append(trans_desc);
			}
			else {
				// since the format changed, we remove all but double newlines
				char *text = new char[strlen(trans_desc)+1];
				char *dest = text;
				const char *src = trans_desc;
				while(  *src!=0  ) {
					*dest = *src;
					if(src[0]=='\n') {
						if(src[1]=='\n') {
							src ++;
							dest++;
							*dest = '\n';
						}
						else {
							*dest = ' ';
						}
					}
					src ++;
					dest ++;
				}
				// remove double line breaks at the end
				*dest = 0;
				while( dest>text  &&  *--dest=='\n'  ) {
					*dest = 0;
				}

				buf.append(text);
				delete [] text;
			}
		}
		buf.append( "\n\n" );

		// belongs to which city?
		if(  !is_factory  &&  ptr.stadt != NULL  ) {
			buf.printf(translator::translate("Town: %s\n"), ptr.stadt->get_name());
		}

		if(  get_tile()->get_besch()->get_utyp() < haus_besch_t::bahnhof  ) {
			buf.printf("%s: %d\n", translator::translate("Passagierrate"), get_passagier_level());
			buf.printf("%s: %d\n", translator::translate("Postrate"),      get_post_level());
		}

		haus_besch_t const& h = *tile->get_besch();
		buf.printf("%s%u", translator::translate("\nBauzeit von"), h.get_intro_year_month() / 12);
		if (h.get_retire_year_month() != DEFAULT_RETIRE_DATE * 12) {
			buf.printf("%s%u", translator::translate("\nBauzeit bis"), h.get_retire_year_month() / 12);
		}

		buf.append("\n");
		if(get_owner()==NULL) {
			long const v = (long)( -welt->get_settings().cst_multiply_remove_haus * (tile->get_besch()->get_level() + 1) / 100 );
			buf.printf("\n%s: %ld$\n", translator::translate("Wert"), v);
		}

		if (char const* const maker = tile->get_besch()->get_copyright()) {
			buf.append("\n");
			buf.printf(translator::translate("Constructed by %s"), maker);
		}
	}
}


void gebaeude_t::rdwr(loadsave_t *file)
{
	// do not save factory buildings => factory will reconstruct them
	assert(!is_factory);
	xml_tag_t d( file, "gebaeude_t" );

	obj_t::rdwr(file);

	char buf[128];
	short idx;

	if(file->is_saving()) {
		const char *s = tile->get_besch()->get_name();
		file->rdwr_str(s);
		idx = tile->get_index();
	}
	else {
		file->rdwr_str(buf, lengthof(buf));
	}
	file->rdwr_short(idx);
	file->rdwr_long(insta_zeit);

	if(file->is_loading()) {
		tile = hausbauer_t::find_tile(buf, idx);
		if(tile==NULL) {
			// try with compatibility list first
			tile = hausbauer_t::find_tile(translator::compatibility_name(buf), idx);
			if(tile==NULL) {
				DBG_MESSAGE("gebaeude_t::rdwr()","neither %s nor %s, tile %i not found, try other replacement",translator::compatibility_name(buf),buf,idx);
			}
			else {
				DBG_MESSAGE("gebaeude_t::rdwr()","%s replaced by %s, tile %i",buf,translator::compatibility_name(buf),idx);
			}
		}
		if(tile==NULL) {
			// first check for special buildings
			if(strstr(buf,"TrainStop")!=NULL) {
				tile = hausbauer_t::find_tile("TrainStop", idx);
			} else if(strstr(buf,"BusStop")!=NULL) {
				tile = hausbauer_t::find_tile("BusStop", idx);
			} else if(strstr(buf,"ShipStop")!=NULL) {
				tile = hausbauer_t::find_tile("ShipStop", idx);
			} else if(strstr(buf,"PostOffice")!=NULL) {
				tile = hausbauer_t::find_tile("PostOffice", idx);
			} else if(strstr(buf,"StationBlg")!=NULL) {
				tile = hausbauer_t::find_tile("StationBlg", idx);
			}
			else {
				// try to find a fitting building
				int level=atoi(buf);
				gebaeude_t::typ type = gebaeude_t::unbekannt;

				if(level>0) {
					// May be an old 64er, so we can try some
					if(strncmp(buf+3,"WOHN",4)==0) {
						type = gebaeude_t::wohnung;
					} else if(strncmp(buf+3,"FAB",3)==0) {
						type = gebaeude_t::industrie;
					}
					else {
						type = gebaeude_t::gewerbe;
					}
					level --;
				}
				else if(buf[3]=='_') {
					/* should have the form of RES/IND/COM_xx_level
					 * xx is usually a number by can be anything without underscores
					 */
					level = atoi(strrchr( buf, '_' )+1);
					if(level>0) {
						switch(toupper(buf[0])) {
							case 'R': type = gebaeude_t::wohnung; break;
							case 'I': type = gebaeude_t::industrie; break;
							case 'C': type = gebaeude_t::gewerbe; break;
						}
					}
					level --;
				}
				// we try to replace citybuildings with their matching counterparts
				// if none are matching, we try again without climates and timeline!
				switch(type) {
					case gebaeude_t::wohnung:
						{
							const haus_besch_t *hb = hausbauer_t::get_residential( level, welt->get_timeline_year_month(), welt->get_climate( get_pos().get_2d() ) );
							if(hb==NULL) {
								hb = hausbauer_t::get_residential(level,0, MAX_CLIMATES );
							}
							if( hb) {
								dbg->message("gebaeude_t::rwdr", "replace unknown building %s with residence level %i by %s",buf,level,hb->get_name());
								tile = hb->get_tile(0);
							}
						}
						break;

					case gebaeude_t::gewerbe:
						{
							const haus_besch_t *hb = hausbauer_t::get_commercial( level, welt->get_timeline_year_month(), welt->get_climate( get_pos().get_2d() ) );
							if(hb==NULL) {
								hb = hausbauer_t::get_commercial(level,0, MAX_CLIMATES );
							}
							if(hb) {
								dbg->message("gebaeude_t::rwdr", "replace unknown building %s with commercial level %i by %s",buf,level,hb->get_name());
								tile = hb->get_tile(0);
							}
						}
						break;

					case gebaeude_t::industrie:
						{
							const haus_besch_t *hb = hausbauer_t::get_industrial( level, welt->get_timeline_year_month(), welt->get_climate( get_pos().get_2d() ) );
							if(hb==NULL) {
								hb = hausbauer_t::get_industrial(level,0, MAX_CLIMATES );
								if(hb==NULL) {
									hb = hausbauer_t::get_residential(level,0, MAX_CLIMATES );
								}
							}
							if (hb) {
								dbg->message("gebaeude_t::rwdr", "replace unknown building %s with industrie level %i by %s",buf,level,hb->get_name());
								tile = hb->get_tile(0);
							}
						}
						break;

					default:
						dbg->warning("gebaeude_t::rwdr", "description %s for building at %d,%d not found (will be removed)!", buf, get_pos().x, get_pos().y);
						welt->add_missing_paks( buf, karte_t::MISSING_BUILDING );
				}
			}
		}	// here we should have a valid tile pointer or nothing ...

		/* avoid double construction of monuments:
		 * remove them from selection lists
		 */
		if (tile  &&  tile->get_besch()->get_utyp() == haus_besch_t::denkmal) {
			hausbauer_t::denkmal_gebaut(tile->get_besch());
		}
		if (tile) {
			remove_ground = tile->has_image()  &&  !tile->get_besch()->ist_mit_boden();
		}
	}

	if(file->get_version()<99006) {
		// ignore the sync flag
		uint8 dummy=sync;
		file->rdwr_byte(dummy);
	}

	// restore city pointer here
	if(  file->get_version()>=99014  ) {
		sint32 city_index = -1;
		if(  file->is_saving()  &&  ptr.stadt!=NULL  ) {
			city_index = welt->get_staedte().index_of( ptr.stadt );
		}
		file->rdwr_long(city_index);
		if(  file->is_loading()  &&  city_index!=-1  &&  (tile==NULL  ||  tile->get_besch()==NULL  ||  tile->get_besch()->is_connected_with_town())  ) {
			ptr.stadt = welt->get_staedte()[city_index];
		}
	}

	if(file->is_loading()) {
		anim_frame = 0;
		anim_time = 0;
		sync = false;

		// Hajo: rebuild tourist attraction list
		if(tile && tile->get_besch()->ist_ausflugsziel()) {
			welt->add_ausflugsziel( this );
		}
	}
}


/**
 * Wird nach dem Laden der Welt aufgerufen - �blicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void gebaeude_t::finish_rd()
{
	player_t::add_maintenance(get_owner(), tile->get_besch()->get_maintenance(welt), tile->get_besch()->get_finance_waytype());

	// citybuilding, but no town?
	if(  tile->get_offset()==koord(0,0)  ) {
		if(  tile->get_besch()->is_connected_with_town()  ) {
			stadt_t *city = (ptr.stadt==NULL) ? welt->suche_naechste_stadt( get_pos().get_2d() ) : ptr.stadt;
			if(city) {
#ifdef MULTI_THREAD
				pthread_mutex_lock( &add_to_city_mutex );
#endif
				city->add_gebaeude_to_stadt(this, true);
#ifdef MULTI_THREAD
				pthread_mutex_unlock( &add_to_city_mutex );
#endif
			}
		}
		else if(  !is_factory  ) {
			ptr.stadt = NULL;
		}
	}
}


void gebaeude_t::cleanup(player_t *player)
{
//	DBG_MESSAGE("gebaeude_t::entferne()","gb %i");
	// remove costs
	sint64 cost = welt->get_settings().cst_multiply_remove_haus;

	// tearing down halts is always single costs only
	if (tile->get_besch()->get_utyp() < haus_besch_t::bahnhof) {
		cost *= tile->get_besch()->get_level() + 1;
	}

	player_t::book_construction_costs(player, cost, get_pos().get_2d(), tile->get_besch()->get_finance_waytype());

	// may need to update next buildings, in the case of start, middle, end buildings
	if(tile->get_besch()->get_all_layouts()>1  &&  get_haustyp()==unbekannt) {

		// realign surrounding buildings...
		uint32 layout = tile->get_layout();

		// detect if we are connected at far (north/west) end
		grund_t * gr = welt->lookup( get_pos() );
		if(gr) {
			sint8 offset = gr->get_weg_yoff()/TILE_HEIGHT_STEP;
			gr = welt->lookup( get_pos()+koord3d( (layout & 1 ? koord::ost : koord::sued), offset) );
			if(!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup( get_pos()+koord3d( (layout & 1 ? koord::ost : koord::sued),offset - 1) );
				if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
			}
			if(gr) {
				gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->get_tile()->get_besch()->get_all_layouts()>4u) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if((layoutbase & 1u) == (layout & 1u)) {
						layoutbase |= 4u; // set far bit on neighbour
						gb->set_tile( gb->get_tile()->get_besch()->get_tile(layoutbase, xy.x, xy.y), false );
					}
				}
			}

			// detect if near (south/east) end
			gr = welt->lookup( get_pos()+koord3d( (layout & 1 ? koord::west : koord::nord), offset) );
			if(!gr) {
				// check whether bridge end tile
				grund_t * gr_tmp = welt->lookup( get_pos()+koord3d( (layout & 1 ? koord::west : koord::nord),offset - 1) );
				if(gr_tmp && gr_tmp->get_weg_yoff()/TILE_HEIGHT_STEP == 1) {
					gr = gr_tmp;
				}
			}
			if(gr) {
				gebaeude_t* gb = gr->find<gebaeude_t>();
				if(gb  &&  gb->get_tile()->get_besch()->get_all_layouts()>4) {
					koord xy = gb->get_tile()->get_offset();
					uint8 layoutbase = gb->get_tile()->get_layout();
					if((layoutbase & 1u) == (layout & 1u)) {
						layoutbase |= 2u; // set near bit on neighbour
						gb->set_tile( gb->get_tile()->get_besch()->get_tile(layoutbase, xy.x, xy.y), false );
					}
				}
			}
		}
	}
	mark_images_dirty();
}


void gebaeude_t::mark_images_dirty() const
{
	// remove all traces from the screen
	image_id img;
	if(  zeige_baugrube  ||
			(!env_t::hide_with_transparency  &&
				env_t::hide_buildings>(get_haustyp()!=unbekannt ? env_t::NOT_HIDE : env_t::SOME_HIDDEN_BUILDING))  ) {
		img = skinverwaltung_t::construction_site->get_bild_nr(0);
	}
	else {
		img = tile->get_hintergrund( anim_frame, 0, season ) ;
	}
	for(  int i=0;  img!=IMG_LEER;  img=get_image(++i)  ) {
		mark_image_dirty( img, -(i*get_tile_raster_width()) );
	}
}
