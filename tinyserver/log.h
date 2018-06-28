#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>

enum loglevel{LOG_INFO_L,LOG_DEBUG_L,LOG_USER_L};

extern int log_level;

void setlogprio(int level);

void log( int level, const char *filename, int linenum, const char *format, ... );

#endif