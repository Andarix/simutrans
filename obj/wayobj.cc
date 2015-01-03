/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Things on ways like catenary or something like this
 *
 * von prissi
 */

#include <algorithm>

#include "../boden/grund.h"
#include "../simworld.h"
#include "../display/simimg.h"
#include "../simobj.h"
#include "../player/simplay.h"
#include "../simtool.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/ribi.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../besch/bruecke_besch.h"
#include "../besch/tunnel_besch.h"
#include "../besch/way_obj_besch.h"

#include "../gui/werkzeug_waehler.h"

#include "../boden/grund.h"
#include "../boden/wege/weg.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../utils/simstring.h"

#include "bruecke.h"
#include "tunnel.h"
#include "wayobj.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t wayobj_calc_bild_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

// the descriptions ...
const way_obj_besch_t *wayobj_t::default_oberleitung=NULL;
stringhashtable_tpl<const way_obj_besch_t *> wayobj_t::table;


wayobj_t::wayobj_t(loadsave_t* const file) : obj_no_info_t()
{
	rdwr(file);
}


wayobj_t::wayobj_t(koord3d const pos, spieler_t* const besitzer, ribi_t::ribi const d, way_obj_besch_t const* const b) : obj_no_info_t(pos)
{
	besch = b;
	dir = d;
	set_besitzer(besitzer);
}


wayobj_t::~wayobj_t()
{
	if(!besch) {
		return;
	}
	if(get_besitzer()) {
		spieler_t::add_maintenance(get_besitzer(), -besch->get_wartung(), get_waytype());
	}
	if(besch->get_own_wtyp()==overheadlines_wt) {
		grund_t *gr=welt->lookup(get_pos());
		weg_t *weg=NULL;
		if(gr) {
			const waytype_t wt = (besch->get_wtyp()==tram_wt) ? track_wt : besch->get_wtyp();
			weg = gr->get_weg(wt);
			if(weg) {
				// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
				weg->set_electrify(false);
				// restore old speed limit
				sint32 max_speed = weg->hat_gehweg() ? 50 : weg->get_besch()->get_topspeed();
				if(gr->get_typ()==grund_t::tunnelboden) {
					tunnel_t *t = gr->find<tunnel_t>(1);
					if(t) {
						max_speed = t->get_besch()->get_topspeed();
					}
				}
				if(gr->get_typ()==grund_t::brueckenboden) {
					bruecke_t *b = gr->find<bruecke_t>(1);
					if(b) {
						max_speed = b->get_besch()->get_topspeed();
					}
				}
				weg->set_max_speed(max_speed);
			}
			else {
				dbg->warning("wayobj_t::~wayobj_t()","ground was not a way!");
			}
		}
	}

	mark_image_dirty( get_after_bild(), 0 );
	mark_image_dirty( get_bild(), 0 );
	grund_t *gr = welt->lookup( get_pos() );
	if( gr ) {
		for( uint8 i = 0; i < 4; i++ ) {
			// Remove ribis from adjacent wayobj.
			if( ribi_t::nsow[i] & get_dir() ) {
				grund_t *next_gr;
				if( gr->get_neighbour( next_gr, besch->get_wtyp(), ribi_t::nsow[i] ) ) {
					wayobj_t *wo2 = next_gr->get_wayobj( besch->get_wtyp() );
					if( wo2 ) {
						wo2->mark_image_dirty( wo2->get_after_bild(), 0 );
						wo2->mark_image_dirty( wo2->get_bild(), 0 );
						wo2->set_dir( wo2->get_dir() & ribi_t::get_forward(ribi_t::nsow[i]) );
						wo2->mark_image_dirty( wo2->get_after_bild(), 0 );
						wo2->mark_image_dirty( wo2->get_bild(), 0 );
						wo2->set_flag(obj_t::dirty);
					}
				}
			}
		}
	}
}


void wayobj_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "wayobj_t" );
	obj_t::rdwr(file);
	if(file->get_version()>=89000) {
		file->rdwr_byte(dir);
		if(file->is_saving()) {
			const char *s = besch->get_name();
			file->rdwr_str(s);
		}
		else {
			char bname[128];
			file->rdwr_str(bname, lengthof(bname));

			besch = wayobj_t::table.get(bname);
			if(besch==NULL) {
				besch = wayobj_t::table.get(translator::compatibility_name(bname));
				if(besch==NULL) {
					if(strstr(bname,"atenary")  ||  strstr(bname,"electri")) {
						besch = default_oberleitung;
					}
				}
				if(besch==NULL) {
					dbg->warning("wayobj_t::rwdr", "description %s for wayobj_t at %d,%d not found, will be removed!", bname, get_pos().x, get_pos().y );
					welt->add_missing_paks( bname, karte_t::MISSING_WAYOBJ );
				}
				else {
					dbg->warning("wayobj_t::rwdr", "wayobj %s at %d,%d replaced by %s", bname, get_pos().x, get_pos().y, besch->get_name() );
				}
			}
		}
	}
	else {
		besch = default_oberleitung;
		dir = 255;
	}
}


void wayobj_t::entferne(spieler_t *sp)
{
	if(besch) {
		spieler_t::book_construction_costs(sp, -besch->get_preis(), get_pos().get_2d(), besch->get_wtyp());
	}
}


// returns NULL, if removal is allowed
// players can remove public owned wayobjs
const char *wayobj_t::ist_entfernbar(const spieler_t *sp)
{
	if(  get_player_nr()==1  ) {
		return NULL;
	}
	return obj_t::ist_entfernbar(sp);
}


void wayobj_t::laden_abschliessen()
{
	// (re)set dir
	if(dir==255) {
		const waytype_t wt = (besch->get_wtyp()==tram_wt) ? track_wt : besch->get_wtyp();
		weg_t *w=welt->lookup(get_pos())->get_weg(wt);
		if(w) {
			dir = w->get_ribi_unmasked();
		}
		else {
			dir = ribi_t::alle;
		}
	}

	// electrify a way if we are a catenary
	if(besch->get_own_wtyp()==overheadlines_wt) {
		const waytype_t wt = (besch->get_wtyp()==tram_wt) ? track_wt : besch->get_wtyp();
		weg_t *weg = welt->lookup(get_pos())->get_weg(wt);
		if(weg) {
			// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
			weg->set_electrify(true);
			if(weg->get_max_speed()>besch->get_topspeed()) {
				weg->set_max_speed(besch->get_topspeed());
			}
		}
		else {
			dbg->warning("wayobj_t::laden_abschliessen()","ground was not a way!");
		}
	}

	if(get_besitzer()) {
		spieler_t::add_maintenance(get_besitzer(), besch->get_wartung(), besch->get_wtyp());
	}
}


void wayobj_t::rotate90()
{
	obj_t::rotate90();
	dir = ribi_t::rotate90( dir);
	hang = hang_t::rotate90( hang );
}


// helper function: gets the ribi on next tile
ribi_t::ribi wayobj_t::find_next_ribi(const grund_t *start, ribi_t::ribi const dir, const waytype_t wt) const
{
	grund_t *to;
	ribi_t::ribi r1 = ribi_t::keine;
	if(start->get_neighbour(to,wt,dir)) {
		const wayobj_t* wo = to->get_wayobj( wt );
		if(wo) {
			r1 = wo->get_dir();
		}
	}
	return r1;
}


void wayobj_t::calc_bild()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &wayobj_calc_bild_mutex );
#endif
	grund_t *gr = welt->lookup(get_pos());
	diagonal = false;
	if(gr) {
		const waytype_t wt = (besch->get_wtyp()==tram_wt) ? track_wt : besch->get_wtyp();
		weg_t *w=gr->get_weg(wt);
		if(!w) {
			dbg->error("wayobj_t::calc_bild()","without way at (%s)", get_pos().get_str() );
			// well, we are not on a way anymore? => delete us
			entferne(get_besitzer());
			delete this;
			gr->set_flag(grund_t::dirty);
#ifdef MULTI_THREAD
			pthread_mutex_unlock( &wayobj_calc_bild_mutex );
#endif
			return;
		}

		set_yoff( -gr->get_weg_yoff() );
		dir &= w->get_ribi_unmasked();

		// if there is a slope, we are finished, only four choices here (so far)
		hang = gr->get_weg_hang();
		if(hang!=hang_t::flach) {
#ifdef MULTI_THREAD
			pthread_mutex_unlock( &wayobj_calc_bild_mutex );
#endif
			return;
		}

		// find out whether using diagonals or straight lines
		if(ribi_t::ist_kurve(dir)  &&  besch->has_diagonal_bild()) {
			ribi_t::ribi r1 = ribi_t::keine, r2 = ribi_t::keine;

			// get the ribis of the ways that connect to us
			// r1 will be 45 degree clockwise ribi (eg nordost->ost), r2 will be anticlockwise ribi (eg nordost->nord)
			r1 = find_next_ribi( gr, ribi_t::rotate45(dir), wt );
			r2 = find_next_ribi( gr, ribi_t::rotate45l(dir), wt );

			// diagonal if r1 or r2 are our reverse and neither one is 90 degree rotation of us
			diagonal = (r1 == ribi_t::rueckwaerts(dir) || r2 == ribi_t::rueckwaerts(dir)) && r1 != ribi_t::rotate90l(dir) && r2 != ribi_t::rotate90(dir);

			if(diagonal) {
				// with this, we avoid calling us endlessly
				// HACK (originally by hajo?)
				static int rekursion = 0;

				if(rekursion == 0) {
					grund_t *to;
					rekursion++;
					for(int r = 0; r < 4; r++) {
						if(gr->get_neighbour(to, wt, ribi_t::nsow[r])) {
							wayobj_t* wo = to->get_wayobj( wt );
							if(wo) {
								wo->calc_bild();
							}
						}
					}
					rekursion--;
				}

				image_id after = besch->get_front_diagonal_image_id(dir);
				image_id bild = besch->get_back_diagonal_image_id(dir);
				if(bild==IMG_LEER  &&  after==IMG_LEER) {
					// no diagonals available
					diagonal = false;
				}
			}
		}
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &wayobj_calc_bild_mutex );
#endif
}



/******************** static stuff from here **********************/


/* better use this constrcutor for new wayobj; it will extend a matching obj or make an new one
 */
void wayobj_t::extend_wayobj_t(koord3d pos, spieler_t *besitzer, ribi_t::ribi dir, const way_obj_besch_t *besch)
{
	grund_t *gr=welt->lookup(pos);
	if(gr) {
		wayobj_t *existing_wayobj = gr->get_wayobj( besch->get_wtyp() );
		if( existing_wayobj ) {
			if(  existing_wayobj->get_besch()->get_topspeed() < besch->get_topspeed()  &&  spieler_t::check_owner(besitzer, existing_wayobj->get_besitzer())  ) {
				// replace slower by faster
				dir = dir | existing_wayobj->get_dir();
				gr->set_flag(grund_t::dirty);
				delete existing_wayobj;
			}
			else {
				// extend this one instead
				existing_wayobj->set_dir(dir|existing_wayobj->get_dir());
				existing_wayobj->mark_image_dirty( existing_wayobj->get_after_bild(), 0 );
				existing_wayobj->mark_image_dirty( existing_wayobj->get_bild(), 0 );
				existing_wayobj->set_flag(obj_t::dirty);
				return;
			}
		}

		// nothing found => make a new one
		wayobj_t *wo = new wayobj_t(pos,besitzer,dir,besch);
		gr->obj_add(wo);
		wo->laden_abschliessen();
		wo->calc_bild();
		wo->mark_image_dirty( wo->get_after_bild(), 0 );
		wo->set_flag(obj_t::dirty);
		spieler_t::book_construction_costs( besitzer,  -besch->get_preis(), pos.get_2d(), besch->get_wtyp());

		for( uint8 i = 0; i < 4; i++ ) {
		// Extend wayobjects around the new one, that aren't already connected.
			if( ribi_t::nsow[i] & ~wo->get_dir() ) {
				grund_t *next_gr;
				if( gr->get_neighbour( next_gr, besch->get_wtyp(), ribi_t::nsow[i] ) ) {
					wayobj_t *wo2 = next_gr->get_wayobj( besch->get_wtyp() );
					if( wo2 ) {
						wo2->set_dir( wo2->get_dir() | ribi_t::rueckwaerts(ribi_t::nsow[i]) );
						wo2->mark_image_dirty( wo2->get_after_bild(), 0 );
						wo->set_dir( wo->get_dir() | ribi_t::nsow[i] );
						wo->mark_image_dirty( wo->get_after_bild(), 0 );
					}
				}
			}
		}
	}
}



// to sort wayobj for always the same menu order
static bool compare_wayobj_besch(const way_obj_besch_t* a, const way_obj_besch_t* b)
{
	int diff = a->get_wtyp() - b->get_wtyp();
	if (diff == 0) {
		diff = a->get_topspeed() - b->get_topspeed();
	}
	if (diff == 0) {
		/* Some speed: sort by name */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


bool wayobj_t::alles_geladen()
{
	if(table.empty()) {
		dbg->warning("wayobj_t::alles_geladen()", "No obj found - may crash when loading catenary.");
	}

	way_obj_besch_t const* def = 0;
	FOR(stringhashtable_tpl<way_obj_besch_t const*>, const& i, table) {
		way_obj_besch_t const& b = *i.value;
		if (b.get_own_wtyp() != overheadlines_wt)           continue;
		if (b.get_wtyp()     != track_wt)                   continue;
		if (def && def->get_topspeed() >= b.get_topspeed()) continue;
		def = &b;
	}
	default_oberleitung = def;

	return true;
}


bool wayobj_t::register_besch(way_obj_besch_t *besch)
{
	// avoid duplicates with same name
	const way_obj_besch_t *old_besch = table.get(besch->get_name());
	if(old_besch) {
		dbg->warning( "wayobj_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		table.remove(besch->get_name());
		tool_t::general_tool.remove( old_besch->get_builder() );
		delete old_besch->get_builder();
		delete old_besch;
	}

	if(  besch->get_cursor()->get_bild_nr(1)!=IMG_LEER  ) {
		// only add images for wayobjexts with cursor ...
		tool_build_wayobj_t *tool = new tool_build_wayobj_t();
		tool->set_icon( besch->get_cursor()->get_bild_nr(1) );
		tool->cursor = besch->get_cursor()->get_bild_nr(0);
		tool->set_default_param(besch->get_name());
		tool_t::general_tool.append( tool );
		besch->set_builder( tool );
	}
	else {
		besch->set_builder( NULL );
	}

	table.put(besch->get_name(), besch);
DBG_DEBUG( "wayobj_t::register_besch()","%s", besch->get_name() );
	return true;
}


/**
 * Fill menu with icons of given wayobjects from the list
 * @author Hj. Malthaner
 */
void wayobj_t::fill_menu(werkzeug_waehler_t *wzw, waytype_t wtyp, sint16 /*sound_ok*/)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), TOOL_BUILD_WAYOBJ | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time=welt->get_timeline_year_month();

	vector_tpl<const way_obj_besch_t *>matching;

	FOR(stringhashtable_tpl<way_obj_besch_t const*>, const& i, table) {
		way_obj_besch_t const* const besch = i.value;
		if(  besch->is_available(time)  ) {

			DBG_DEBUG("wayobj_t::fill_menu()", "try to add %s(%p)", besch->get_name(), besch);
			if(  besch->get_builder()  &&  wtyp==besch->get_wtyp()  ) {
				// only add items with a cursor
				matching.append(besch);
			}
		}
	}
	// sort the tools before adding to menu
	std::sort(matching.begin(), matching.end(), compare_wayobj_besch);
	FOR(vector_tpl<way_obj_besch_t const*>, const i, matching) {
		wzw->add_werkzeug(i->get_builder());
	}
}


const way_obj_besch_t *wayobj_t::wayobj_search(waytype_t wt, waytype_t own, uint16 time)
{
	FOR(stringhashtable_tpl<way_obj_besch_t const*>, const& i, table) {
		way_obj_besch_t const* const besch = i.value;
		if(  besch->is_available(time)  &&  besch->get_wtyp()==wt  &&  besch->get_own_wtyp()==own  ) {
			return besch;
		}
	}
	return NULL;
}


const way_obj_besch_t* wayobj_t::find_besch(const char *str)
{
	return wayobj_t::table.get(str);
}
