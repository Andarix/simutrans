#include <algorithm>

#include "freight_list_sorter.h"
#include "simhalt.h"
#include "simtypes.h"
#include "simware.h"
#include "simfab.h"
#include "simworld.h"

#include "dataobj/translator.h"

#include "tpl/slist_tpl.h"
#include "tpl/vector_tpl.h"

#include "utils/cbuffer_t.h"


karte_ptr_t freight_list_sorter_t::welt;
freight_list_sorter_t::sort_mode_t freight_list_sorter_t::sortby=by_name;


/**
 *  @return whether w1 is less than w2
 */
bool freight_list_sorter_t::compare_ware(ware_t const& w1, ware_t const& w2)
{
	// sort according to freight
	// if w1 and w2 differ, they are sorted according to catg_index and index
	// we sort with respect to catg_indexfirst, since freights with the same category
	// will be displayed together
	int idx = w1.get_desc()->get_catg_index() - w2.get_desc()->get_catg_index();
	if(  idx == 0  ) {
		idx = w1.get_desc()->get_index() - w2.get_desc()->get_index();
	}
	if(  idx != 0  ) {
		return idx < 0;
	}

	switch (sortby) {
		default:
			dbg->error("freight_list_sorter::compare_ware()", "illegal sort mode!");
			/* FALLTHROUGH */

		case by_via_sum:
		case by_amount: { // sort by ware amount
			int const order = w2.menge - w1.menge;
			if(  order != 0  ) {
				return order < 0;
			}
		}
		/* FALLTHROUGH */

		case by_via: { // sort by via_destination name
			halthandle_t const v1 = w1.get_zwischenziel();
			halthandle_t const v2 = w2.get_zwischenziel();
			if(  v1.is_bound() && v2.is_bound()  ) {
				int const order = strcmp(v1->get_name(), v2->get_name());
				if(  order != 0) return order < 0;
			}
			else if(  v1.is_bound()  ) {
				return false;
			}
			else if(  v2.is_bound()  ) {
				return true;
			}
		}
		/* FALLTHROUGH */

		case by_name: { // sort by destination name
			halthandle_t const d1 = w1.get_ziel();
			halthandle_t const d2 = w2.get_ziel();
			if(  d1.is_bound()  &&  d2.is_bound()  ) {
				const fabrik_t *fab = NULL;
				const char *const name1 = ( w1.to_factory ? ( (fab=fabrik_t::get_fab(w1.get_zielpos())) ? fab->get_name() : "Invalid Factory" ) : d1->get_name() );
				const char *const name2 = ( w2.to_factory ? ( (fab=fabrik_t::get_fab(w2.get_zielpos())) ? fab->get_name() : "Invalid Factory" ) : d2->get_name() );
				return strcmp(name1, name2) < 0;
			}
			else if(  d1.is_bound()  ) {
				return false;
			}
			else if(  d2.is_bound()  ) {
				return true;
			}
		}
	}
	return false;
}



void freight_list_sorter_t::add_ware_heading( cbuffer_t &buf, uint32 sum, uint32 max, const ware_t *ware, const char *what_doing )
{
	// not the first line?
	if(  buf.len() > 0  ) {
		buf.append("\n");
	}
	buf.printf("%u", sum);
	if(  max != 0  ) {
		// convois
		buf.printf("/%u", max);
	}
	goods_desc_t const& desc = *ware->get_desc();
	char const*  const  unit = translator::translate(desc.get_mass());
	// special freight (catg == 0) needs own name
	char const*  const  name = translator::translate(ware->get_catg() != 0 ? desc.get_catg_name() : desc.get_name());
	char const*  const  what = translator::translate(what_doing);
	buf.printf("%s %s %s\n", unit, name, what);
}


void freight_list_sorter_t::sort_freight(vector_tpl<ware_t> const& warray, cbuffer_t& buf, sort_mode_t sort_mode, const slist_tpl<ware_t>* full_list, const char* what_doing)
{
	sortby = sort_mode;

	// hsiegeln
	// added sorting to ware's destination list
	int pos = 0;
	ware_t* wlist = MALLOCN( ware_t, warray.get_count() );

	FOR(vector_tpl<ware_t>, const& ware, warray) {
		if(  ware.get_desc() == goods_manager_t::none  ||  ware.menge == 0  ) {
			continue;
		}
		wlist[pos] = ware;
		// for the sorting via the number for the next stop we unify entries
		if(  sort_mode == by_via_sum  ) {
			// only add it, if there is not another thing waiting with the same via but another destination
			for(  int i=0;  i<pos;  i++  ) {
				ware_t& wi = wlist[i];
				if(  wi.get_index()==ware.get_index()  &&  wi.get_zwischenziel()==ware.get_zwischenziel()  ) {
					if(  wi.get_zwischenziel().is_bound()  ) {
						if(  (  wi.get_ziel()==wi.get_zwischenziel() )==( ware.get_ziel()==ware.get_zwischenziel() ) ) {
							wi.menge += ware.menge;
							--pos;
							break;
						}
					}
					else {
						if(  wi.get_ziel() == ware.get_ziel()  ) {
							wi.menge += ware.menge;
							--pos;
							break;
						}
					}
				}
			}
		}
		pos++;
	}

	// if there, give the capacity for each freight
	slist_tpl<ware_t>                 const  dummy;
	slist_tpl<ware_t>                 const& list     = full_list ? *full_list : dummy;
	slist_tpl<ware_t>::const_iterator        full_i   = list.begin();
	slist_tpl<ware_t>::const_iterator const  full_end = list.end();

	// at least some capacity added?
	if(  pos != 0  ) {
		// sort the ware's list
		std::sort( wlist, wlist + pos, compare_ware );

		// print the ware's list to buffer - it should be in sort order by now!
		int last_goods_index = -1;
		int last_ware_catg = -1;

		for(  int j = 0;  j < pos;  j++  ) {
			halthandle_t const halt     = wlist[j].get_ziel();
			halthandle_t const via_halt = wlist[j].get_zwischenziel();

			const char * name = "Error in Routing";
			if(  halt.is_bound()  ) {
				name = halt->get_name();
			}

			ware_t const& ware = wlist[j];
			if(  last_goods_index!=ware.get_index()  &&  last_ware_catg!=ware.get_catg()  ) {
				sint32 sum = 0;
				last_goods_index = ware.get_index();
				last_ware_catg = (ware.get_catg()!=0) ? ware.get_catg() : -1;
				for(  int i=j;  i<pos;  i++  ) {
					ware_t const& sumware = wlist[i];
					if(  last_goods_index != sumware.get_index()  ) {
						if(  last_ware_catg != sumware.get_catg()  ) {
							break;	// next category reached ...
						}
					}
					sum += sumware.menge;
				}

				// special freight => handle different
				last_ware_catg = (ware.get_catg()!=0) ? ware.get_catg() : -1;

				// display all ware
				if(  full_list == NULL  ) {
					add_ware_heading( buf, sum, 0, &ware, what_doing );
				}
				else {
					// ok, we have a list of freights
					while(  full_i != full_end  ) {
						ware_t const& current = *full_i++;
						if(  last_goods_index==current.get_index()  ||  last_ware_catg==current.get_catg()  ) {
							add_ware_heading( buf, sum, current.menge, &current, what_doing );
							break;
						}
						else {
							add_ware_heading( buf, 0, current.menge, &current, what_doing );
						}
					}
				}
			}
			// detail amount
			goods_desc_t const& desc = *ware.get_desc();
			buf.printf("  %u%s %s > ", ware.menge, translator::translate(desc.get_mass()), translator::translate(desc.get_name()));
			// the target name is not correct for the via sort
			const bool is_factory_going = ( sortby!=by_via_sum  &&  ware.to_factory );	// exclude merged packets
			if(  sortby!=by_via_sum  ||  via_halt==halt  ) {
				if(  is_factory_going  ) {
					const fabrik_t *const factory = fabrik_t::get_fab( ware.get_zielpos() );
					buf.printf("%s <%i,%i>", (factory ? factory->get_name() : "Invalid Factory"), ware.get_zielpos().x, ware.get_zielpos().y);
				}
				else {
					buf.append(name);
				}
			}

			if(  via_halt!=halt  ||  is_factory_going  ) {
				if(  via_halt.is_bound()  ) {
					buf.printf(translator::translate("via %s\n"), via_halt->get_name());
				}
				else {
					if(  sortby == by_via_sum  ) {
						// do not show undecided transfer halts
						buf.append(name);
					}
					buf.append("\n");
				}
			}
			else {
				buf.append("\n");
			}
			// debug end
		}
	}

	// still entire left?
	for(  ; full_i != full_end; ++full_i  ) {
		ware_t const& g = *full_i;
		add_ware_heading(buf, 0, g.menge, &g, what_doing);
	}

	free( wlist );
}
