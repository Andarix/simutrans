/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * A container for other gui_components. Is itself
 * a gui_component, and can therefore be nested.
 *
 * @author Hj. Malthaner
 * @date 03-Mar-01
 */

#ifndef gui_container_h
#define gui_container_h


#include "../../simdebug.h"
#include "../../simevent.h"
#include "../../tpl/vector_tpl.h"
#include "gui_component.h"

class gui_container_t : virtual public gui_component_t
{
protected:
	vector_tpl <gui_component_t *> components;

	// holds the GUI component that has the focus in this window
	// focused component of this container can only be one of its immediate children
	gui_component_t *comp_focus;

	bool list_dirty:1;

	// true, while infowin_event is processed
	bool inside_infowin_event:1;

public:
	gui_container_t();
	virtual ~gui_container_t() {}

	// needed for WIN_OPEN events
	void clear_dirty() { list_dirty=false; }

	/**
	* Adds a Component to the Container.
	* @author Hj. Malthaner
	*/
	virtual void add_component(gui_component_t *comp);

	/**
	* Removes a Component in the Container.
	* @author Hj. Malthaner
	*/
	virtual void remove_component(gui_component_t *comp);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	* @author Hj. Malthaner
	*/
	void draw(scr_coord offset) OVERRIDE;

	/**
	* Removes all Components in the Container.
	* @author Markus Weber
	*/
	virtual void remove_all();

	/**
	 * Returns true if any child component is focusable
	 * @author Knightly
	 */
	bool is_focusable() OVERRIDE;

	/**
	 * Activates this element.
	 *
	 * @warning Calling this method from anything called from
	 * gui_container_t::infowin_event (e.g. in action_triggered)
	 * will have NO effect.
	 */
	void set_focus( gui_component_t *comp_focus );

	/**
	 * returns element that has the focus
	 * that is: go down the hierarchy as much as possible
	 */
	gui_component_t *get_focus() OVERRIDE;

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 * @author Knightly
	 */
	scr_coord get_focus_pos() OVERRIDE { return comp_focus ? pos+comp_focus->get_focus_pos() : scr_coord::invalid; }


	// FIXME
	scr_size get_min_size() const OVERRIDE { return get_size(); }
	scr_size get_max_size() const OVERRIDE { return get_size(); }
};

#endif
