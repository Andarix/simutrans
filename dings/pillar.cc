/*
 * Support for bridges
 *
 * Hj. Malthaner
 */

#include <string.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simmem.h"
#include "../simimg.h"

#include "../bauer/brueckenbauer.h"

#include "../boden/grund.h"

#include "../gui/thing_info.h"

#include "../dataobj/loadsave.h"
#include "../dings/pillar.h"
#include "../dings/bruecke.h"



pillar_t::pillar_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	besch = NULL;
	hide = false;
	rdwr(file);
}



pillar_t::pillar_t(karte_t *welt, koord3d pos, spieler_t *sp, const bruecke_besch_t *besch, bruecke_besch_t::img_t img, int hoehe) : ding_t(welt, pos)
{
	this->besch = besch;
	this->dir = (uint8)img;
	set_yoff(-hoehe);
	set_besitzer( sp );
	calc_bild();
}


void pillar_t::calc_bild()
{
	hide = false;
	if(get_yoff()==0  &&  besch->has_pillar_asymmetric()) {
		if(  grund_t *gr = welt->lookup(get_pos())  ) {
			hang_t::typ h = gr->get_grund_hang();
			if(h==hang_t::nord  ||  h==hang_t::west) {
				hide = true;
			}
		}
	}
}

/**
 * @return Einen Beschreibungsstring fuer das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void pillar_t::zeige_info()
{
	planquadrat_t *plan=welt->access(get_pos().get_2d());
	for(unsigned i=0;  i<plan->get_boden_count();  i++  ) {
		grund_t *bd=plan->get_boden_bei(i);
		if(bd->ist_bruecke()) {
			bruecke_t* br = bd->find<bruecke_t>();
			if(br  &&  br->get_besch()==besch) {
				br->zeige_info();
			}
		}
	}
}



void pillar_t::rdwr(loadsave_t *file)
{
	xml_tag_t p( file, "pillar_t" );

	ding_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = besch->get_name();
		file->rdwr_str(s);
		file->rdwr_byte(dir);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		file->rdwr_byte(dir);

		besch = brueckenbauer_t::get_besch(s);
		if(besch==0) {
			if(strstr(s,"ail")) {
				besch = brueckenbauer_t::get_besch("ClassicRail");
				dbg->warning("pillar_t::rdwr()","Unknown bridge %s replaced by ClassicRail",s);
			}
			else if(strstr(s,"oad")) {
				besch = brueckenbauer_t::get_besch("ClassicRoad");
				dbg->warning("pillar_t::rdwr()","Unknown bridge %s replaced by ClassicRoad",s);
			}
		}
	}
}



void pillar_t::rotate90()
{
	ding_t::rotate90();
	// may need to hide/show asymmetric pillars
	// this is done now in calc_bild, which is called after karte_t::rotate anyway
	// we cannot decide this here, since welt->lookup(get_pos())->get_grund_hang() cannot be called
	// since we are in the middle of the rotation process

	// the rotated image parameter is just one in front/back
	dir = (dir == bruecke_besch_t::NS_Pillar) ? bruecke_besch_t::OW_Pillar : bruecke_besch_t::NS_Pillar;
}
