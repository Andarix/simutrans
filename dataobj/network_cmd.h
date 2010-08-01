#ifndef _NETWORK_CMD_H_
#define _NETWORK_CMD_H_

#include "../simtypes.h"
#include "../tpl/slist_tpl.h"
#include "koord3d.h"
#include "network.h"

class packet_t;
class karte_t;
class spieler_t;
class werkzeug_t;

// actual commands
enum {
	NWC_INVALID   = 0,
	NWC_JOIN      = 1,
	NWC_SYNC      = 2,
	NWC_GAME      = 3,
	NWC_READY     = 4,
	NWC_TOOL      = 5,
	NWC_CHECK     = 6,
	NWC_COUNT
};

class network_command_t {
protected:
	packet_t *packet;
	// always send:
	uint16 id;
	uint32 our_client_id;
	// ready for sending
	bool   ready;
public:
	network_command_t(uint16 /*id*/);
	network_command_t();
	virtual ~network_command_t();
	// receive: calls rdwr from packet
	// return true on success
	bool receive(packet_t *p);
	// calls rdwr if packet is empty
	void prepare_to_send();
	// really sends to a client
	void send(SOCKET s);

	// write our data to the packet
	virtual void rdwr();

	// executes the command
	// (see network_world_command_t::execute() )
	// if returns true this can be deleted afterwards
	virtual bool execute(karte_t *) { return true;}

	virtual const char* get_name() { return "network_command_t";}

	bool is_local_cmd();

	uint16 get_id() { return id;}

	// creates an instance:
	// creates a packet, reads it from socket, get the nwc-id, and reads its data
	static network_command_t* read_from_socket(SOCKET s);
};

/**
 * nwc_join_t
 * @from-client: client wants to join the server
 *		server sends nwc_join_t to sender, nwc_sync_t to all clients
 * @from-server:
 *		@data answer == 1 (if joining now is ok)
 *		@data client_id
 *		client ignores the following nwc_sync_t, waits for nwc_ready_t
 */
class nwc_join_t : public network_command_t {
public:
	nwc_join_t() : network_command_t(NWC_JOIN), client_id(0), answer(0) {}
	virtual bool execute(karte_t *);
	virtual void rdwr();
	virtual const char* get_name() { return "nwc_join_t";}
	uint32 client_id;
	uint8 answer;
};

/**
 * nwc_ready_t
 * @from-client:
 *		@data sync_steps at which client will continue
 *		client paused, waits for unpause
 * @from-server:
 *		data is resent to client
 *		unpause client
 */
class nwc_ready_t : public network_command_t {
public:
	nwc_ready_t(uint32 sync_steps_=0) : network_command_t(NWC_READY), sync_steps(sync_steps_) {}
	virtual bool execute(karte_t *);
	virtual void rdwr();
	virtual const char* get_name() { return "nwc_ready_t";}
	uint32 sync_steps;
};

/**
 * nwc_game_t
 * @from-server:
 *		@data len of savegame
 *		client processes this in network_connect
 */
class nwc_game_t : public network_command_t {
public:
	nwc_game_t(uint32 len_=0) : network_command_t(NWC_GAME), len(len_) {}
	virtual void rdwr();
	virtual const char* get_name() { return "nwc_game_t";}
	uint32 len;
};

/**
 * commands that have to be executed at a certain sync_step
 */
class network_world_command_t : public network_command_t {
public:
	network_world_command_t() : network_command_t(), sync_step(0) {};
	network_world_command_t(uint16 /*id*/, uint32 /*sync_step*/);
	virtual void rdwr();
	virtual const char* get_name() { return "network_world_command_t";}
	// put it to the command queue
	virtual bool execute(karte_t *);
	// apply it to the world
	virtual void do_command(karte_t*) {}
	uint32 get_sync_step() const { return sync_step; }
	// ignore events that lie in the past?
	// if false: any cmd with sync_step < world->sync_step forces network disconnect
	virtual bool ignore_old_events() const { return false;}
	// for sorted data structures
	bool operator <= (network_world_command_t c) const { return sync_step <= c.sync_step; }
	static bool cmp(network_world_command_t *nwc1, network_world_command_t *nwc2) { return nwc1->get_sync_step() <= nwc2->get_sync_step(); }
protected:
	uint32 sync_step; // when this has to be executed
	// TODO: uint16 sub_step to have an order within one step
};

/**
 * nwc_sync_t
 * @from-server:
 *		@data client_id this client wants to receive the game
 *		clients: pause game, save, load, wait for nwc_ready_t command to unpause
 *		server: pause game, save, load, send game to client, send nwc_ready_t command to client
 */
class nwc_sync_t : public network_world_command_t {
public:
	nwc_sync_t() : network_world_command_t(NWC_SYNC, 0) {};
	nwc_sync_t(uint32 sync_steps, uint32 send_to_client) : network_world_command_t(NWC_SYNC, sync_steps), client_id(send_to_client) {};
	virtual void rdwr();
	virtual void do_command(karte_t*);
	virtual const char* get_name() { return "nwc_sync_t";}
	uint32 client_id; // this client shall receive the game
};

/**
 * nwc_check_t
 * @from-server:
 *		@data random_seed at previous sync_step
 *		clients: check random seed, if check fails disconnect.
 *      the check is done in karte_t::interactive
 */
class nwc_check_t : public network_world_command_t {
public:
	nwc_check_t() : network_world_command_t(NWC_CHECK, 0) {};
	nwc_check_t(uint32 sync_steps, uint32 server_random_seed_, uint32 server_sync_step_) : network_world_command_t(NWC_CHECK, sync_steps), server_random_seed(server_random_seed_), server_sync_step(server_sync_step_) {};
	virtual void rdwr();
	virtual void do_command(karte_t*) {}
	virtual const char* get_name() { return "nwc_check_t";}
	uint32 server_random_seed;
	uint32 server_sync_step;
	// no action required -> can be ignored if too old
	virtual bool ignore_old_events() const { return true;}
};

/**
 * nwc_tool_t
 * @from-client: client sends tool init/work
 * @from-server: server sends nwc_tool_t to all clients with step when tool has to be executed
 *		@data client_id (the client that launched the tool, sent by server)
 *		@data player_nr
 *		@data init (if true call init else work)
 *		@data wkz_id
 *		@data pos
 *		@data default_param
 *		@data exec (if true executes, else server sends it to clients)
 */

class nwc_tool_t : public network_world_command_t {
public:
	nwc_tool_t() : network_world_command_t(NWC_TOOL, 0) { default_param = NULL; }
	nwc_tool_t(spieler_t *sp, werkzeug_t *wkz, koord3d pos, uint32 sync_steps, bool init);
	nwc_tool_t(const nwc_tool_t&);

	virtual ~nwc_tool_t();

	virtual void rdwr();
	// send to clients or put to command queue (depending on exec)
	virtual bool execute(karte_t *);
	// really executes it, here exec should be true
	virtual void do_command(karte_t*);
	virtual const char* get_name() { return "nwc_tool_t";}
private:
	char *default_param;
	uint32 tool_client_id;
	uint16 wkz_id;
	koord3d pos;
	uint8 flags;
	uint8 player_nr;
	bool init;
	bool exec;

	// contains tools of players at other clients
	class tool_node_t {
	public:
		werkzeug_t *wkz;
		uint32 client_id;
		uint8 player_id;
		const char* default_param;
		tool_node_t() : wkz(NULL), client_id(0), player_id(255), default_param(NULL) {}
		tool_node_t(werkzeug_t *_wkz, uint8 _player_id, uint32 _client_id) : wkz(_wkz), client_id(_client_id), player_id(_player_id), default_param(NULL) {}
		// compares only the ids
		inline bool operator == (const tool_node_t c) const { return client_id==c.client_id  &&  player_id==c.player_id; }
	};

	static vector_tpl<tool_node_t> tool_list;
};


#endif
