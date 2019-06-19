#include <string.h>

#include "tunnelboden.h"

#include "../simworld.h"
#include "../simskin.h"

#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../obj/tunnel.h"

#include "../descriptor/ground_desc.h"
#include "../descriptor/skin_desc.h"
#include "../descriptor/tunnel_desc.h"

#include "../utils/cbuffer_t.h"


tunnelboden_t::tunnelboden_t(loadsave_t *file, koord pos ) : boden_t(koord3d(pos,0), 0)
{
	rdwr(file);

	// some versions had tunnel without tunnel objects
	if (!find<tunnel_t>()) {
		// then we must spawn it here (a way MUST be always present, or the savegame is completely broken!)
		weg_t *weg=(weg_t *)obj_bei(0);
		if(  !weg  ) {
			dbg->error( "tunnelboden_t::tunnelboden_t()", "Loading without a way at (%s)! Assuming road tunnel!", pos.get_str() );
			obj_add(new tunnel_t(get_pos(), NULL, tunnel_builder_t::get_tunnel_desc(road_wt, 450, 0)));
		}
		else {
			obj_add(new tunnel_t(get_pos(), weg->get_owner(), tunnel_builder_t::get_tunnel_desc(weg->get_desc()->get_wtyp(), 450, 0)));
		}
		DBG_MESSAGE("tunnelboden_t::tunnelboden_t()","added tunnel to pos (%i,%i,%i)",get_pos().x, get_pos().y,get_pos().z);
	}
}


void tunnelboden_t::calc_image_internal(const bool calc_only_snowline_change)
{
	// tunnel mouth
	if(  ist_karten_boden()  ) {
		if(  grund_t::underground_mode == grund_t::ugm_all  ||  (grund_t::underground_mode == grund_t::ugm_level  &&  pos.z == grund_t::underground_level)  ) {
			if(  grund_t::underground_mode == grund_t::ugm_all  ) {
				clear_back_image();
			}
			else {
				boden_t::calc_image_internal( calc_only_snowline_change );
			}
			// default tunnel ground images
			set_image( skinverwaltung_t::tunnel_texture->get_image_id(0) );
			clear_flag( draw_as_obj );
		}
		else {
			// calculate the ground
			boden_t::calc_image_internal( calc_only_snowline_change );
			set_flag( draw_as_obj );
		}

		if(  grund_t::underground_mode == grund_t::ugm_none  ) {
			if(  (ribi_type(get_grund_hang()) == ribi_t::east  &&  abs(back_imageid) > 11)  ||  (ribi_type(get_grund_hang()) == ribi_t::south  &&  get_back_image(0) != IMG_EMPTY)  ) {
				// on east or north slope: must draw as obj, since there is a slope here nearby
				koord pos = get_pos().get_2d() + koord( get_grund_hang() );
				grund_t *gr = welt->lookup_kartenboden( pos );
				gr->set_flag( grund_t::draw_as_obj );
			}
		}
	}
	// inside tunnel
	else if(  !calc_only_snowline_change  ) {
		clear_back_image();
		// default tunnel ground images
		// single or double slope? (single slopes are not divisible by 8)
		const uint8 slope_this =  get_disp_slope();
		const uint8 imageid = (!slope_this  ||  (slope_this & 7)) ? ground_desc_t::slopetable[slope_this] : ground_desc_t::slopetable[slope_this >> 1] + 12;
		set_image( skinverwaltung_t::tunnel_texture->get_image_id( imageid ) );
	}
}


void tunnelboden_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "tunnelboden_t" );

	grund_t::rdwr(file);

	if(  file->is_version_less(88, 9)  ) {
		uint32 sl = slope;
		file->rdwr_long(sl);
		// convert slopes from old single height saved game
		slope = (scorner_sw(sl) + scorner_se(sl) * 3 + scorner_ne(sl) * 9 + scorner_nw(sl) * 27) * env_t::pak_height_conversion_factor;
	}

	// only 99.03 version save the tunnel here
	if(file->is_version_equal(99, 3)) {
		char  buf[256];
		const tunnel_desc_t *desc = NULL;
		file->rdwr_str(buf, lengthof(buf));
		if (find<tunnel_t>() == NULL) {
			desc = tunnel_builder_t::get_desc(buf);
			if(desc) {
				obj_add(new tunnel_t(get_pos(), obj_bei(0)->get_owner(), desc));
			}
		}
	}
}


void tunnelboden_t::info(cbuffer_t & buf) const
{
	const tunnel_t *tunnel = find<tunnel_t>();
	if(tunnel  &&  tunnel->get_desc()) {
		const tunnel_desc_t *desc = tunnel->get_desc();
		buf.append(translator::translate(desc->get_name()));
		buf.append("\n");
	}
	boden_t::info(buf);
}
