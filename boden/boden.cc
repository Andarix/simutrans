/*
 * Natur-Untergrund f�r Simutrans.
 * �berarbeitet Januar 2001
 * von Hj. Malthaner
 */

#include "../simworld.h"
#include "../simskin.h"

#include "../obj/baum.h"

#include "../dataobj/environment.h"
#include "../dataobj/loadsave.h"

#include "boden.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"



boden_t::boden_t(karte_t *welt, loadsave_t *file, koord pos ) : grund_t( welt, koord3d(pos,0) )
{
	grund_t::rdwr( file );

	// restoring trees (disadvantage: loosing offsets but much smaller savegame footprint)
	if(  file->get_version()>=110001  ) {
		sint16 id = file->rd_obj_id();
		while(  id!=-1  ) {
			sint32 age;
			file->rdwr_long( age );
			// check, if we still have this tree ... (if there are not trees, the first index is NULL!)
			if (id < baum_t::get_anzahl_besch() && baum_t::get_all_besch()[id]) {
				baum_t *tree = new baum_t( welt, get_pos(), (uint8)id, age, slope );
				objlist.add( tree );
			}
			else {
				dbg->warning( "boden_t::boden_t()", "Could not restore tree type %i at (%s)", id, pos.get_str() );
			}
			// check for next tree
			id = file->rd_obj_id();
		}
	}
}


boden_t::boden_t(karte_t *welt, koord3d pos, hang_t::typ sl) : grund_t(welt, pos)
{
	slope = sl;
}


// only for more compact saving of trees
void boden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(  file->get_version()>=110001  ) {
		// a server send the smallest possible savegames to clients, i.e. saves only types and age of trees
		if(  env_t::server  &&  !hat_wege()  ) {
			for(  uint8 i=0;  i<objlist.get_top();  i++  ) {
				obj_t *obj = objlist.bei(i);
				if(  obj->get_typ()==obj_t::baum  ) {
					baum_t *tree = (baum_t *)obj;
					file->wr_obj_id( tree->get_besch_id() );
					uint32 age = tree->get_age();
					file->rdwr_long( age );
				}
			}
		}
		file->wr_obj_id( -1 );
	}
}


const char *boden_t::get_name() const
{
	if(ist_uebergang()) {
		return "Kreuzung";
	} else if(hat_wege()) {
		return get_weg_nr(0)->get_name();
	}
	else {
		return "Boden";
	}
}


void boden_t::calc_bild_internal()
{
		uint8 slope_this =  get_disp_slope();
		weg_t *weg = get_weg(road_wt);

		if(  is_visible()  ) {
			if(  weg  &&  weg->hat_gehweg()  ) {
				// single or double slope? (single slopes are not divisible by 8)
				const uint8 bild_nr = (!slope_this  ||  (slope_this & 7)) ? grund_besch_t::slopetable[slope_this] : grund_besch_t::slopetable[slope_this >> 1] + 12;

				if(  (get_hoehe() >= welt->get_snowline()  ||  welt->get_climate(pos.get_2d()) == arctic_climate)  &&  skinverwaltung_t::fussweg->get_bild_nr(bild_nr + 1) != IMG_LEER  ) {
					// snow images
					set_bild( skinverwaltung_t::fussweg->get_bild_nr(bild_nr + 1) );
				}
				else if(  slope_this != 0  &&  get_hoehe() == welt->get_snowline() - 1  &&  skinverwaltung_t::fussweg->get_bild_nr(bild_nr + 2) != IMG_LEER  ) {
					// transition images
					set_bild( skinverwaltung_t::fussweg->get_bild_nr(bild_nr + 2) );
				}
				else {
					set_bild( skinverwaltung_t::fussweg->get_bild_nr(bild_nr) );
				}
			}
			else {
				set_bild( grund_besch_t::get_ground_tile(this) );
			}
		}
		else {
			set_bild(IMG_LEER);
		}
		grund_t::calc_back_bild( get_disp_height(), slope_this );
}
