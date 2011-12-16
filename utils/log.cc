/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "log.h"
#include "../simdebug.h"
#include "../simsys.h"


#ifdef MAKEOBJ
#define debuglevel (3)
#else
#ifdef NETTOOL
#define debuglevel (0)

#else
#define debuglevel (umgebung_t::verbose_debug)

// for display ...
#include "../gui/messagebox.h"
#include "../simgraph.h"
#include "../simwin.h"

#include "../dataobj/umgebung.h"
#endif
#endif

/**
 * writes a debug message into the log.
 * @author Hj. Malthaner
 */
void log_t::debug(const char *who, const char *format, ...)
{
	if(log_debug  &&  debuglevel==4) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                         /* nur loggen wenn schon ein log */
			fprintf(log ,"Debug: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "Debug: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}

		va_end(argptr);
	}
}


/**
 * writes a message into the log.
 * @author Hj. Malthaner
 */
void log_t::message(const char *who, const char *format, ...)
{
	if(debuglevel>2) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                         /* nur loggen wenn schon ein log */
			fprintf(log ,"Message: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "Message: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);
	}
}


/**
 * writes a warning into the log.
 * @author Hj. Malthaner
 */
void log_t::warning(const char *who, const char *format, ...)
{
	if(debuglevel>1) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                         /* nur loggen wenn schon ein log */
			fprintf(log ,"Warning: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "Warning: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");
		}
		va_end(argptr);
	}
}


/**
 * writes an error into the log.
 * @author Hj. Malthaner
 */
void log_t::error(const char *who, const char *format, ...)
{
	if(debuglevel>0) {
		va_list argptr;
		va_start(argptr, format);

		if( log ) {                         /* nur loggen wenn schon ein log */
			fprintf(log ,"ERROR: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(log, format, argptr);
			fprintf(log,"\n");

			if( force_flush ) {
				fflush(log);
			}

			fprintf(log ,"Please report all errors to\n");
			fprintf(log ,"team@64.simutrans.com\n");
		}
		va_end(argptr);

		va_start(argptr, format);
		if( tee ) {                         /* nur loggen wenn schon ein log */
			fprintf(tee, "ERROR: %s:\t",who);      /* geoeffnet worden ist */
			vfprintf(tee, format, argptr);
			fprintf(tee,"\n");

			fprintf(tee ,"Please report all errors to\n");
			fprintf(tee ,"team@64.simutrans.com\n");
		}
		va_end(argptr);
	}
}


/**
 * writes an error into the log, aborts the program.
 * @author Hj. Malthaner
 */
void log_t::fatal(const char *who, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	static char buffer[8192];

	int n = sprintf( buffer, "FATAL ERROR: %s\n", who);
	n += vsprintf( buffer+n, format, argptr);
	strcpy( buffer+n, "\n" );

	if( log ) {
		fputs( buffer, log );
		fputs( "Aborting program execution ...\n\n", log );
		fputs( "Please report all fatal errors to\n", log );
		fputs( "team@64.simutrans.com\n", log );
		if( force_flush ) {
			fflush(log);
		}
	}

	if( tee ) {
		fputs( buffer, tee );
		fputs( "Aborting program execution ...\n\n", tee );
		fputs( "Please report all fatal errors to\n", tee );
		fputs( "team@64.simutrans.com\n", tee );
	}

	if(tee==NULL  &&  log==NULL) {
		puts( buffer );
	}

	va_end(argptr);

#if defined MAKEOBJ  ||  defined NETTOOL
	// no display available
	puts( buffer );
#else
#  ifdef DEBUG
	int old_level = umgebung_t::verbose_debug;
#  endif
	umgebung_t::verbose_debug = 0;	// no more window concerning messages
	if(is_display_init()) {
		// show notification
		destroy_all_win( true );

		strcpy( buffer+n+1, "PRESS ANY KEY\n" );
		news_img* sel = new news_img(buffer,IMG_LEER);
		sel->set_fenstergroesse( sel->get_fenstergroesse()+koord(150,0) );

		koord xy( display_get_width()/2 - 180, display_get_height()/2 - sel->get_fenstergroesse().y/2 );
		event_t ev;

		create_win( xy.x, xy.y, sel, w_info, magic_none );

		while(win_is_top(sel)) {
			// do not move, do not close it!
			dr_sleep(50);
			dr_prepare_flush();
			sel->zeichnen( xy, sel->get_fenstergroesse() );
			dr_flush();
			display_poll_event(&ev);
			// main window resized
			check_pos_win(&ev);
			if(ev.ev_class==EVENT_KEYBOARD) {
				break;
			}
		}
	}
	else {
		// use OS means, if there
		dr_fatal_notify(buffer);
	}

#ifdef DEBUG
	if (old_level > 4) {
		// generate a division be zero error, if the user request it
		static int make_this_a_division_by_zero = 0;
		printf("%i", 15 / make_this_a_division_by_zero);
		make_this_a_division_by_zero &= 0xFF;
	}
#endif
#endif
	abort();
}



// create a logfile for log_debug=true
log_t::log_t(const char *logfilename, bool force_flush, bool log_debug, bool log_console, const char *greeting )
{
	log = NULL;
	this->force_flush = force_flush;    /* wenn true wird jedesmal geflusht */
										/* wenn ein Eintrag ins log geschrieben wurde */
	this->log_debug = log_debug;

	if(logfilename == NULL) {
		log = NULL;                       /* kein log */
		tee = NULL;
	} else if(strcmp(logfilename,"stdio") == 0) {
		log = stdout;
		tee = NULL;
	} else if(strcmp(logfilename,"stderr") == 0) {
		log = stderr;
		tee = NULL;
	} else {
		log = fopen(logfilename,"wb");

		if(log == NULL) {
			fprintf(stderr,"log_t::log_t: can't open file '%s' for writing\n", logfilename);
		}
		tee = stderr;
	}
	if (!log_console) {
	    tee = NULL;
	}

	if(  greeting  ) {
		if( log ) {
			fputs( greeting, log );
//			message("log_t::log_t","Starting logging to %s", logfilename);
		}
		if( tee ) {
			fputs( greeting, tee );
		}
	}
}



void log_t::close()
{
	message("log_t::~log_t","stop logging, closing log file");

	if( log ) {
		fclose(log);
		log = NULL;
	}
}


// close all logs during cleanup
log_t::~log_t()
{
	if( log ) {
		close();
	}
}
