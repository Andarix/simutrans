/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __OBJ_BESCH_H
#define __OBJ_BESCH_H

#include <cstddef>
#include "../simtypes.h"

/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Basis aller besch_t-Klassen, d.h. Beschreibungen, die aus den .pak
 *	Files geladen werden.
 *	Keine virtuellen Methoden erlaubt !
 */
class obj_besch_t {

protected:
	template<typename T> T const* get_child(int i) const { return static_cast<T const*>(node_info[i]); }

	/*
	 * Internal Node information - the derived class knows,
	 * how many node child nodes really exist.
	 */
	obj_besch_t** node_info;

public:
	obj_besch_t() : node_info() {}

	~obj_besch_t() { delete node_info; }

	void* operator new(size_t size)
	{
		return ::operator new(size);
	}

	void* operator new(size_t size, unsigned extra)
	{
		return ::operator new(size + extra);
	}

	friend class obj_reader_t;
};

#endif
