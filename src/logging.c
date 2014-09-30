#include "logging.h"


char *log_level_strings[] = {"EMERGENCY", "ALERT", "ERROR", "WARNING", "INFO",
                             "DEBUG"};

int logging_level = LOG_WARNING;

void log_message(int level, const char *format, ...)
{
	va_list argptr;

	/* Higher priority logs are lower in number, anything above the configured
	    level should be skipped.
	*/
	if(level > logging_level)
	{
		return;
	}

	va_start(argptr, format);

	printf(" [%s] ", log_level_strings[level]);
	vprintf(format, argptr);

	va_end(argptr);
}

void set_logging_level(int log_level)
{
	logging_level = log_level;
}
