#include "log.h"
#include <stdarg.h>
#include <stdio.h>

static int debug_level;

void set_debug(int level)
{
	debug_level = level;
}

void debug(const char *s,...)
{
	if(!debug_level)
		return;

	va_list va;
	va_start(va,s);

	vfprintf(stdout,s,va);

	va_end(va);
}

void error(const char *s,...)
{
	va_list va;
	va_start(va,s);

	vfprintf(stderr,s,va);

	va_end(va);
}

void syserror(const char *s,...)
{
	va_list va;
	va_start(va,s);

	vfprintf(stderr,s,va);

	va_end(va);
}
