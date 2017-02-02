/*
 * Copyright (c) 1997 - 2002 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef goods_manager_t_h
#define goods_manager_t_h

#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

class goods_desc_t;

/**
 * Factory-Class for Goods.
 *
 * @author Hj. Malthaner
 */
class goods_manager_t
{
private:
	static stringhashtable_tpl<const goods_desc_t *> desc_table;
	static vector_tpl<goods_desc_t *> waren;

	static goods_desc_t *load_passagiere;
	static goods_desc_t *load_post;
	static goods_desc_t *load_nichts;

	// number of different good classes;
	static uint8 max_catg_index;

public:
	enum { INDEX_PAS=0, INDEX_MAIL=1, INDEX_NONE=2 };

	static const goods_desc_t *passagiere;
	static const goods_desc_t *post;
	static const goods_desc_t *nichts;

	static bool successfully_loaded();
	static bool register_desc(goods_desc_t *desc);

	static uint8 get_max_catg_index() { return max_catg_index; }

	/**
	* Search the good 'name' information and return
	* its description. Return NULL when the Good is
	* unknown.
	*
	* @param name the non-translated good name
	* @author Hj. Malthaner/V. Meyer
	*/
	static const goods_desc_t *get_info(const char* name);

	static const goods_desc_t *get_info(uint16 idx) { return waren[idx]; }

	static uint8 get_count() { return (uint8)waren.get_count(); }

	// good by catg
	static const goods_desc_t *get_info_catg(const uint8 catg);

	// good by catg_index
	static const goods_desc_t *get_info_catg_index(const uint8 catg_index);

	/*
	 * allow to multiply all prices, 1000=1.0
	 * used for the beginner mode
	 */
	static void set_multiplier(sint32 multiplier);
};

#endif
