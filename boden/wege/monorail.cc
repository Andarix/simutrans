#include "../../simtypes.h"
#include "../../bauer/wegbauer.h"
#include "../../besch/weg_besch.h"

#include "monorail.h"

const weg_besch_t *monorail_t::default_monorail=NULL;



monorail_t::monorail_t(loadsave_t *file) : schiene_t()
{
	rdwr(file);
}



void monorail_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(get_desc()->get_wtyp()!=monorail_wt) {
		int old_max_speed = get_max_speed();
		const weg_besch_t *desc = wegbauer_t::weg_search( monorail_wt, (old_max_speed>0 ? old_max_speed : 120), 0, (systemtype_t)((get_desc()->get_styp()==type_elevated)*type_elevated) );
		if (desc==NULL) {
			dbg->fatal("monorail_t::rwdr()", "No monorail way available");
		}
		dbg->warning("monorail_t::rwdr()", "Unknown way replaced by monorail %s (old_max_speed %i)", desc->get_name(), old_max_speed );
		set_besch(desc);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
	}
}
