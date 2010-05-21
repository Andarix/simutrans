/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../player/simplay.h"
#include "../simwerkz.h"
#include "../boden/grund.h"
#include "../boden/tunnelboden.h"
#include "../simimg.h"
#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"

#include "../besch/tunnel_besch.h"

#include "tunnel.h"




tunnel_t::tunnel_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	besch = 0;
	rdwr(file);
	bild = after_bild = IMG_LEER;
	broad_type = 0;
}


tunnel_t::tunnel_t(karte_t *welt, koord3d pos, spieler_t *sp, const tunnel_besch_t *besch) :
	ding_t(welt, pos)
{
	assert(besch);
	this->besch = besch;
	set_besitzer( sp );
	bild = after_bild = IMG_LEER;
	broad_type = 0;
}



void
tunnel_t::calc_bild()
{
	const grund_t *gr = welt->lookup(get_pos());
	if(gr->ist_karten_boden()) {
		hang_t::typ hang = gr->get_grund_hang();

		broad_type = 0;
		if( besch->has_broad_portals() ) {
			ribi_t::ribi dir = ribi_t::rotate90( ribi_typ( hang ) );
			const grund_t *gr_l = welt->lookup(get_pos() + dir);
			tunnel_t* tunnel_l = gr_l ? gr_l->find<tunnel_t>() : NULL;
			if( tunnel_l && tunnel_l->get_besch() == besch && gr_l->get_grund_hang() == hang ) {
				broad_type += 1;
				if( !(tunnel_l->get_broad_type() & 2) ) {
					tunnel_l->calc_bild();
				}
			}
			const grund_t *gr_r = welt->lookup(get_pos() - dir);
			tunnel_t* tunnel_r = gr_r ? gr_r->find<tunnel_t>() : NULL;
			if( tunnel_r && tunnel_r->get_besch() == besch && gr_r->get_grund_hang() == hang ) {
				broad_type += 2;
				if( !(tunnel_r->get_broad_type() & 1) ) {
					tunnel_r->calc_bild();
				}
			}
		}

		set_bild( besch->get_hintergrund_nr(hang, get_pos().z >= welt->get_snowline(),broad_type));
		set_after_bild( besch->get_vordergrund_nr(hang, get_pos().z >= welt->get_snowline(),broad_type) );
	}
	else {
		set_bild( IMG_LEER );
		set_after_bild( IMG_LEER );
	}
}



void tunnel_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "tunnel_t" );
	ding_t::rdwr(file);
	if(file->get_version()>=99001) {
		char  buf[256];
		if(file->is_loading()) {
			file->rdwr_str(buf, lengthof(buf));
			besch = tunnelbauer_t::get_besch(buf);
			if(besch==NULL) {
				besch = tunnelbauer_t::get_besch(translator::compatibility_name(buf));
			}
		}
		else {
			strcpy( buf, besch->get_name() );
			file->rdwr_str(buf,0);
		}
	}
}



void tunnel_t::laden_abschliessen()
{
	const grund_t *gr = welt->lookup(get_pos());
	spieler_t *sp=get_besitzer();

	if(besch==NULL) {
		// find a matching besch
		besch = tunnelbauer_t::find_tunnel( (waytype_t)gr->get_weg_nr(0)->get_besch()->get_wtyp(), 450, 0);
	}

	if(sp) {
		// change maintainance
		weg_t *weg = gr->get_weg(besch->get_waytype());
		if(weg) {
			weg->set_max_speed(besch->get_topspeed());
			spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung());
		}
		spieler_t::add_maintenance( sp,  besch->get_wartung() );
	}
}



// correct speed and maitainace
void tunnel_t::entferne( spieler_t *sp2 )
{
	if(sp2==NULL) {
		// only set during destroying of the map
		return;
	}
	spieler_t *sp = get_besitzer();
	if(sp) {
		// inside tunnel => do nothing but change maitainance
		const grund_t *gr = welt->lookup(get_pos());
		if(gr) {
			weg_t *weg = gr->get_weg( besch->get_waytype() );
			weg->set_max_speed( weg->get_besch()->get_topspeed() );
			spieler_t::add_maintenance( sp,  weg->get_besch()->get_wartung());
			spieler_t::add_maintenance( sp,  -besch->get_wartung() );
		}
	}
	spieler_t::accounting(sp2, -besch->get_preis(), get_pos().get_2d(), COST_CONSTRUCTION );
}

void tunnel_t::set_bild( image_id b )
{
	mark_image_dirty( bild, get_yoff() );
	mark_image_dirty( b, get_yoff() );
	bild = b;
}

void tunnel_t::set_after_bild( image_id b )
{
	mark_image_dirty( after_bild, get_yoff() );
	mark_image_dirty( b, get_yoff() );
	after_bild = b;
}

// returns NULL, if removal is allowed
// players can remove public owned ways
const char *tunnel_t::ist_entfernbar(const spieler_t *sp)
{
	if (get_player_nr()==1) {
		return NULL;
	}
	else {
		return ding_t::ist_entfernbar(sp);
	}
}
