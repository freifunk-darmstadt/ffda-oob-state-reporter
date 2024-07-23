#pragma once

#include <stdarg.h>
#include <stdio.h>

#define FOSR_SYSLOG_NAME "ffda-oob-state-reporter"

enum channeld_debug_level {
	MSG_FATAL = 0,
	MSG_ERROR,
	MSG_WARN,
	MSG_INFO,
	MSG_VERBOSE,
	MSG_DEBUG,
};

#define MSG(_nr, _format, ...) fosr_log_message(MSG_##_nr, __func__, __LINE__, _format, ##__VA_ARGS__)

int fosr_log_set_level(int level);
void fosr_log_set_syslog(int enable);
void fosr_log_message(int level, const char *func, int line, const char *format, ...);
