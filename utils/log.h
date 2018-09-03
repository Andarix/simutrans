/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#ifndef tests_log_h
#define tests_log_h

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include "../simtypes.h"


#if defined(MAKEOBJ)
extern int debuglevel;
#endif

/**
 * Logging facility
 * @author Hj. Malthaner
 */
class log_t
{
private:
	/**
	 * Primary log file.
	 * @author Hj. Malthaner
	 */
	FILE *log;

	/**
	 * Secondary log file, currently fixed to stderr
	 * @author Hj. Malthaner
	 */
	FILE * tee;

	bool force_flush;

	/**
	 * Logging level - include debug messages ?
	 * @author Hj. Malthaner
	 */
	bool log_debug;

	/**
	 * Syslog tag
	 */
	const char* tag;

	/**
	 * Log to syslog?
	 */
	bool syslog;

	std::string doublettes;

public:
	/**
	 * writes a debug message into the log.
	 * @author Hj. Malthaner
	 */
	void debug(const char *who, const char *format, ...);

	/**
	 * writes a message into the log.
	 * @author Hj. Malthaner
	 */
	void message(const char *who, const char *format, ...);

	/**
	 * writes a warning into the log.
	 * @author Hj. Malthaner
	 */
	void warning(const char *who, const char *format, ...);

	/* special error handling for double objects */
	void doubled( const char *what, const char *name );
	bool had_overlaid() { return !doublettes.empty(); }
	void clear_overlaid() { doublettes.clear(); }
	std::string get_overlaid() { return doublettes; }

	/**
	 * writes an error into the log.
	 * @author Hj. Malthaner
	 */
	void error(const char *who, const char *format, ...);

	/**
	 * writes an error into the log, aborts the program.
	 * @author Hj. Malthaner
	 */
	void NORETURN fatal(const char* who, const char* format, ...);

	/**
	 * writes to log using va_lists
	 */
	void vmessage(const char *what, const char *who, const char *format,  va_list args );


	void close();


	/**
	 * @param syslogtag only used with syslog
	 */
	log_t(const char* logname, bool force_flush, bool log_debug, bool log_console, const char* greeting, const char* syslogtag=NULL);

	~log_t();
};

#endif
