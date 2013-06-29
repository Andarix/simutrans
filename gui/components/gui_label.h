/*
 * just displays a text, will be auto-translated
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef gui_gui_label_h
#define gui_gui_label_h

#include "gui_komponente.h"
#include "../../simcolor.h"


/**
 * Eine Label-Komponente
 *
 * @author Hj. Malthaner
 * @date 04-Mar-01
 *
 * Added Aligment support
 * @author: Volker Meyer
 * @date 25.05.03
 */
class gui_label_t : public gui_komponente_t
{
public:
	enum align_t {
		left,
		centered,
		right,
		money
	};

private:
	align_t align;

	/**
	 * Farbe des Labels
	 * @author Hansj�rg Malthaner
	 */
	COLOR_VAL color;

	const char * text;	// only for direct acess of non-translateable things. Do not use!
	const char * tooltip;

public:
	gui_label_t(const char* text=NULL, COLOR_VAL color=COL_BLACK, align_t align=left);

	// one stop init
	void init( const char* t, koord p, COLOR_VAL c=COL_BLACK, align_t a=left) {
		set_pos( p );
		set_text( t );
		color = c;
		align = a;
	}

	/**
	 * Sets the text to display, after translating it.
	 * @author Hansj�rg Malthaner
	 */
	void set_text(const char *text);

	/**
	 * Sets the text without translation.
	 * @author Hansj�rg Malthaner
	 */
	void set_text_pointer(const char *text);

	/**
	 * returns the pointer (i.e. for freeing untranslater contents)
	 * @author Hansj�rg Malthaner
	 */
	const char * get_text_pointer() { return text; }

	/**
	 * returns the tooltip pointer (i.e. for freeing untranslater contents)
	 */
	const char * get_tooltip_pointer() { return tooltip; }

	/**
	 * Draws the component.
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset);

	/**
	 * Sets the colour of the label
	 * @author Owen Rudge
	 */
	void set_color(COLOR_VAL colour) { this->color = colour; }
	COLOR_VAL get_color() const { return color; }

	/**
	 * Sets the alignment of the label
	 * @author Volker Meyer
	 */
	void set_align(align_t align) { this->align = align; }

	/**
	 * Sets the tooltip of this component.
	 * @author Hj. Malthaner
	 */
	void set_tooltip(const char * t);
};

#endif
