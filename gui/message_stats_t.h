/*
 * Copyright (c) 1997 - 2003 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef message_stats_t_h
#define message_stats_t_h

#include "components/gui_komponente.h"
#include "../simimg.h"
#include "../simmesg.h"
#include "../tpl/slist_tpl.h"

class karte_t;

/**
 * City list stats display
 * @author Hj. Malthaner
 */
class message_stats_t : public gui_komponente_t
{
private:
	karte_t *welt;
	message_t *msg;
	sint32 message_type;								// Knightly : message type for filtering; -1 indicates no filtering
	uint32 last_count;
	sint32 message_selected;
	const slist_tpl<message_t::node *> *message_list;	// Knightly : points to the active message list (original or filtered)
	slist_tpl<message_t::node *> filtered_messages;		// Knightly : cache the list of messages belonging to a certain type

public:
	message_stats_t(karte_t *welt);
	~message_stats_t() { filtered_messages.clear(); }

	/**
	 * Filter messages by type
	 * @return whether there is a change in message filtering
	 * @author Knightly
	 */
	bool filter_messages(const sint32 msg_type);

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Recalc the current size required to display everything, and set komponente groesse
	*/
	void recalc_size();

	/**
	 * Zeichnet die Komponente
	 * @author Hj. Malthaner
	 */
	void zeichnen(koord offset);
};

#endif
