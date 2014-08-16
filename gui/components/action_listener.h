/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_ifc_action_listener_h
#define gui_ifc_action_listener_h

#include "../../simtypes.h"

class gui_action_creator_t;

/**
 * This interface must be implemented by all classes which want to
 * listen actions, i.e. button presses
 * @author Hj. Malthaner
 */
class action_listener_t
{
public:
	virtual ~action_listener_t() {}

	/**
	 * This method is called if an action is triggered
	 * @author Hj. Malthaner
	 *
	 * Returns true, if action is done and no more
	 * components should be triggered.
	 * V.Meyer
	 */
	virtual bool action_triggered(gui_action_creator_t *comp, value_t extra) = 0;
};

#endif
