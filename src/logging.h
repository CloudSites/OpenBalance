#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>
#include <stdarg.h>


enum
{
	LOG_EMERGENCY = 0,
	LOG_ALERT,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG
};

extern int logging_level;

char *log_level_strings[LOG_DEBUG + 1];

void log_message(int level, const char *format, ...);
void set_logging_level(int log_level);

#endif
