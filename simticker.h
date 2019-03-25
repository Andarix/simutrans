/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef SIMTICKER_H
#define SIMTICKER_H

#include "simcolor.h"
#include "display/simgraph.h"

// ticker height
#define TICKER_HEIGHT (LINESPACE+3)

// ticker vertical position from bottom of screen
extern uint16 TICKER_YPOS_BOTTOM;

class koord;

/**
 * A very simple scrolling news ticker.
 */
namespace ticker
{
	bool empty();

	/**
	 * Add a message to the message list
	 * @param pos    position of the event
	 * @param color  message color
	 */
	void add_msg(const char*, koord pos, FLAGGED_PIXVAL color = color_idx_to_rgb(COL_BLACK));

	/**
	 * Remove all messages and mark for redraw
	 */
	void clear_messages();

	/**
	 * @returns the 2D world position of the most recent visible message
	 */
	koord get_welt_pos();

	/**
	 * Update message positions and remove old messages
	 */
	void update();

	/**
	 * Redraw the ticker partially or fully (if set_redraw_all() was called)
	 */
	void draw();

	/**
	 * Set true if ticker has to be redrawn fully
	 * @sa redraw
	 */
	void set_redraw_all(bool redraw);

	/**
	 * Force a ticker redraw (e.g. after a window resize)
	 */
	void redraw();
};

#endif
