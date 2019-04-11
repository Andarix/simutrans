/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef ifc_gui_component_h
#define ifc_gui_component_h

#include "../../display/scr_coord.h"
#include "../../simevent.h"
#include "../../display/simgraph.h"

struct event_t;
class karte_ptr_t;

/**
 * Base class for all GUI components.
 *
 * @author Hj. Malthaner
 */
class gui_component_t
{
private:
	/**
	 * Allow component to show/hide itself
	 * @author hsiegeln
	 */
	bool visible:1;

	/**
	 * If invisible, still reserves space.
	 */
	bool rigid:1;

	/**
	 * Some components might not be allowed to gain focus.
	 * For example: gui_textarea_t
	 * This flag can be set to false to deny focus request for a gui_component.
	 * @author hsiegeln
	 */
	bool focusable:1;

protected:
	/**
	 * Component's bounding box position.
	 * @author Hj. Malthaner
	 */
	scr_coord pos;

	/**
	 * Component's bounding box size.
	 * @author Hj. Malthaner
	 */
	scr_size size;

public:
	/**
	 * Basic constructor, initialises member variables
	 * @author Hj. Malthaner
	 */
	gui_component_t(bool _focusable = false) : visible(true), rigid(false), focusable(_focusable) {}

	/**
	 * Virtual destructor so all descendant classes are destructed right
	 * @author Hj. Malthaner
	 */
	virtual ~gui_component_t() {}

	/**
	 * Initialises the component's position and size.
	 * @author Max Kielland
	 */
	void init(scr_coord pos_par, scr_size size_par=scr_size(0,0)) {
		set_pos(pos_par);
		set_size(size_par);
	}

	/**
	 * Set focus ability for this component.
	 */
	virtual void set_focusable(bool yesno) {
		focusable = yesno;
	}

	/**
	 * Returns focus ability for this component.
	 * Knightly : a component can only be focusable when it is visible.
	 */
	virtual bool is_focusable() {
		return ( visible && focusable );
	}

	/**
	 * Sets component to be shown/hidden
	 * @author Hj. Malthaner
	 */
	virtual void set_visible(bool yesno) {
		visible = yesno;
	}

	/**
	 * Checks if component should be displayed.
	 * If invisible and not rigid compoment will be ignore for gui-positioning and resizing.
	 * @author Hj. Malthaner
	 */
	virtual bool is_visible() const {
		return visible;
	}

	void set_rigid(bool yesno) {
		rigid = yesno;
	}

	bool is_rigid() const {
		return rigid;
	}

	/**
	 * Set this component's position.
	 * @author Hj. Malthaner
	 */
	virtual void set_pos(scr_coord pos_par) {
		pos = pos_par;
	}

	/**
	 * Get this component's bounding box position.
	 * @author Hj. Malthaner
	 */
	virtual scr_coord get_pos() const {
		return pos;
	}

	/**
	 * Set this component's bounding box size.
	 * @author Hj. Malthaner
	 */
	virtual void set_size(scr_size size_par) {
		size = size_par;
	}

	/**
	 * Get this component's bounding box size.
	 * @author Hj. Malthaner
	 */
	virtual scr_size get_size() const {
		return size;
	}

	/**
	 * Set this component's width.
	 * @author Max Kielland
	 */
	virtual void set_width(scr_coord_val width_par) {
		set_size(scr_size(width_par,size.h));
	}

	/**
	 * Set this component's height.
	 * @author Max Kielland
	 */
	virtual void set_height(scr_coord_val height_par) {
		set_size(scr_size(size.w,height_par));
	}

	/**
	 * Get this component's maximal bounding box size.
	 * If this is larger than get_min_size(), then the element will be enlarged if possible
	 *   according to min_size of elements in the same row/column.
	 * If w/h is equal to scr_size::inf.w/h, then the
	 *   element is enlarged as much as possible if their is available space.
	 */
	virtual scr_size get_max_size() const {
		return scr_size::inf;
	}

	/**
	 * Get this component's minimal bounding box size.
	 * Elements will be as least as small.
	 */
	virtual scr_size get_min_size() const {
		return scr_size(0,0);
	}


	/**
	 * Checks if the given position is inside the component area.
	 * @return true if the coordinates are inside this component area.
	 * @author Hj. Malthaner
	 */
	virtual bool getroffen(int x, int y) {
		return ( pos.x <= x && pos.y <= y && (pos.x+size.w) > x && (pos.y+size.h) > y );
	}

	/**
	 * deliver event to a component if
	 * - component has focus
	 * - mouse is over this component
	 * - event for all components
	 * @return: true for swallowing this event
	 */
	virtual bool infowin_event(const event_t *) {
		return false;
	}

	/**
	 * Pure virtual paint method
	 * @author Hj. Malthaner
	 */
	virtual void draw(scr_coord offset) = 0;

	/**
	 * returns the element that has focus.
	 * child classes like scrolled list of tabs should
	 * return a child component.
	 */
	virtual gui_component_t *get_focus() {
		return is_focusable() ? this : 0;
	}

	/**
	 * Get the relative position of the focused component.
	 * Used for auto-scrolling inside a scroll pane.
	 * @author Knightly
	 */
	virtual scr_coord get_focus_pos() {
		return pos;
	}

	/**
	 * Align this component against a target component
	 * @param component_par the component to align against
	 * @param alignment_par the requested alignment
	 * @param offset_par Offset added to final alignment
	 * @author Max Kielland
	 */
	void align_to(gui_component_t* component_par, control_alignment_t alignment_par, scr_coord offset_par = scr_coord(0,0) );

	/**
	 * @returns bounding box position and size.
	 */
	virtual scr_rect get_client( void ) { return scr_rect( pos, size ); }

};


/**
 * Base class for all GUI components that need access to the world.
 */
class gui_world_component_t: public gui_component_t
{
protected:
	static karte_ptr_t welt;
};


/**
 * Class for an empty element, to simulate empty cells in tables.
 */
class gui_empty_t : public gui_component_t
{
	gui_component_t* ref; ///< this empty cell should have same min and max size as ref
public:
	gui_empty_t(gui_component_t* r = NULL): ref(r) {}

	void draw(scr_coord) OVERRIDE { }

	void set_ref(gui_component_t* r) { ref = r; }

	scr_size get_min_size() const OVERRIDE { return ref ? ref->get_min_size() : gui_component_t::get_min_size(); }

	scr_size get_max_size() const OVERRIDE { return ref ? ref->get_max_size() : gui_component_t::get_min_size(); }
};

/**
 * Class for an empty element with potential infinite width.
 * Can force neighboring cells to the boundary of the array
 */
class gui_fill_t : public gui_component_t
{
	bool fill_x;
	bool fill_y;
public:
	gui_fill_t(bool x=true, bool y=false) : fill_x(x), fill_y(y) {}

	void draw(scr_coord) OVERRIDE { }

	scr_size get_min_size() const OVERRIDE { return scr_size(0,0); }

	scr_size get_max_size() const OVERRIDE { return scr_size( fill_x ? scr_size::inf.w : 0, fill_y ? scr_size::inf.h : 0);  }
};


#endif
