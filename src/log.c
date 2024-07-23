#include <syslog.h>
#include <stdarg.h>

#include "log.h"

static int fosr_log_level = MSG_INFO;
static int fosr_log_syslog = 0;

static void fosr_log_message_stdout_stderr(int level, const char *func, int line, const char *format, ...)
{
	va_list ap;

	FILE* log_stream = stdout;

	if (fosr_log_level == MSG_DEBUG)
		fprintf(log_stream, "[%s:%d] ", func, line);

	if (fosr_log_level <= MSG_WARN)
		log_stream = stderr;

	va_start(ap, format);
	vfprintf(log_stream, format, ap);
	va_end(ap);
}

int fosr_log_set_level(int level)
{
	fosr_log_level = level;

	return 0;
}

void fosr_log_set_syslog(int enable)
{
	fosr_log_syslog = enable;
	if (fosr_log_syslog)
		openlog(FOSR_SYSLOG_NAME, LOG_PID, LOG_DAEMON);
	else
		closelog();
}

void fosr_log_message(int level, const char *func, int line, const char *format, ...)
{
	va_list ap;

	int syslog_level = LOG_INFO;

	switch (level) {
		case MSG_FATAL:
		case MSG_ERROR:
			syslog_level = LOG_CRIT;
			break;
		case MSG_WARN:
			syslog_level = LOG_WARNING;
			break;
		case MSG_INFO:
		default:
			syslog_level = LOG_INFO;
			break;
	}

	/* Check if log-level is okay*/
	if (level > fosr_log_level)
		return;

	if (fosr_log_syslog) {
		va_start(ap, format);
		vsyslog(syslog_level, format, ap);
		va_end(ap);
	} else {
		fosr_log_message_stdout_stderr(level, func, line, format);
	}
}

