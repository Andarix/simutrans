/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef ifc_gui_komponente_h
#define ifc_gui_komponente_h

#include "../../display/scr_coord.h"
#include "../../simevent.h"
#include "../../display/simgraph.h"

struct event_t;

/**
 * Base class for all GUI components.
 *
 * @author Hj. Malthaner
 */
class gui_komponente_t
{
private:
	/**
	* allow component to show/hide itself
	* @author hsiegeln
	*/
	bool visible:1;

	/**
	* some components might not be allowed to gain focus
	* for example: gui_textarea_t
	* this flag can be set to true to deny focus request for a gui_component always
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
	gui_komponente_t(bool _focusable = false) : visible(true), focusable(_focusable) {}

	/**
	* Virtual destructor so all descendant classes are destructed right
	* @author Hj. Malthaner
	*/
	virtual ~gui_komponente_t() {}

	/**
	* Initialises the component's position and size.
	* @author Max Kielland
	*/
	virtual void init(scr_coord pos_par, scr_size size_par=scr_size(0,0)) {
		set_pos(pos_par);
		set_size(size_par);
	}

	/**
	* Set focus ability for this component.
	* @author Unknown
	*/
	virtual void set_focusable(bool yesno) {
		focusable = yesno;
	}

	// Knightly : a component can only be focusable when it is visible
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
	* @author Hj. Malthaner
	*/
	virtual bool is_visible() const {
		return visible;
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
	* @author Hj. Malthaner
	* prissi: default -> do nothing
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
	virtual gui_komponente_t *get_focus() {
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
	void align_to(gui_komponente_t* component_par, control_alignment_t alignment_par, scr_coord offset_par = scr_coord(0,0) );

	/**
	 * Align this component against a target component
	 * @param component_par the component to align against
	 * @param alignment_par the requested alignment
	 * @param offset_par Offset added to final alignment
	 * @author Max Kielland
	 */
	virtual scr_rect get_client( void ) { return scr_rect( pos, size ); }

};

#endif
