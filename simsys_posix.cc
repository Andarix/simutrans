/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#else
// need timeGetTime
#include <mmsystem.h>
#endif

#include <signal.h>

#include "macros.h"
#include "simdebug.h"
#include "simevent.h"
#include "simsys.h"


static bool sigterm_received = false;

// no autoscaling as we have no display ...
bool dr_auto_scale(bool)
{
	return false;
}

bool dr_os_init(const int*)
{
	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;
	return true;
}

resolution dr_query_screen_resolution()
{
	resolution const res = { 0, 0 };
	return res;
}

// open the window
int dr_os_open(int, int, int)
{
	return 1;
}


void dr_os_close()
{
}

// resizes screen
int dr_textur_resize(unsigned short** const textur, int, int)
{
	*textur = NULL;
	return 1;
}


unsigned short *dr_textur_init()
{
	return NULL;
}

unsigned int get_system_color(unsigned int, unsigned int, unsigned int)
{
	return 1;
}

void dr_prepare_flush()
{
}

void dr_flush()
{
}

void dr_textur(int, int, int, int)
{
}

void move_pointer(int, int)
{
}

void set_pointer(int)
{
}

int dr_screenshot(const char *,int,int,int,int)
{
	return -1;
}

static inline unsigned int ModifierKeys()
{
	return 0;
}

void GetEvents()
 {
	if(  sigterm_received  ) {
		sys_event.type = SIM_SYSTEM;
		sys_event.code = SYSTEM_QUIT;
	}
 }


void GetEventsNoWait()
{
	if(  sigterm_received  ) {
		sys_event.type = SIM_SYSTEM;
		sys_event.code = SYSTEM_QUIT;
	}
}

void show_pointer(int)
{
}

void ex_ord_update_mx_my()
{
}

#ifndef _MSC_VER
static timeval first;
#endif

uint32 dr_time()
{
#ifndef _MSC_VER
	timeval second;
	gettimeofday(&second,NULL);
	if (first.tv_usec > second.tv_usec) {
		// since those are often unsigned
		second.tv_usec += 1000000;
		second.tv_sec--;
	}

	return (second.tv_sec - first.tv_sec)*1000ul + (second.tv_usec - first.tv_usec)/1000ul;
#else
	return timeGetTime();
#endif
}

void dr_sleep(uint32 msec)
{
/*
	// this would be 100% POSIX but is usually not very accurate ...
	if(  msec>0  ) {
		struct timeval tv;
		tv.sec = 0;
		tv.usec = msec*1000;
		select(0, 0, 0, 0, &tv);
	}
*/
#ifdef _WIN32
	Sleep( msec );
#else
	usleep( 1000u * msec );
#endif
}

void dr_start_textinput()
{
}

void dr_stop_textinput()
{
}

void dr_notify_input_pos(int, int)
{
}

static void posix_sigterm(int)
{
	DBG_MESSAGE("Received SIGTERM, exiting...");
	sigterm_received = 1;
}


int main(int argc, char **argv)
 {
	signal( SIGTERM, posix_sigterm );
#ifndef _MSC_VER
 	gettimeofday(&first,NULL);
#endif
 	return sysmain(argc, argv);
 }
