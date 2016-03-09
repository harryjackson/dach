#ifndef DACH_LOG_H
#define DACH_LOG_H
//#include <stdio.h>

typedef enum {
	DACH_LOG_NONE = 0,
	DACH_LOG_FATAL = 1,
	DACH_LOG_CRITICAL = 2,
	DACH_LOG_ERROR = 3,
	DACH_LOG_WARN = 4,
	DACH_LOG_INFO = 5,
	DACH_LOG_DEBUG = 6,
	DACH_LOG_ALL = 7
} dach_log_level;

/*
 * TODO: If it's mutable it needs to be thread safe
 * This should be considered an application global setting.
 */
static dach_log_level dach_error_log_verbosity = DACH_LOG_WARN;

#define dach_perror(dach_level, dach_fmt, ...)        \
  do {                                   \
    if (dach_error_log_verbosity >= dach_level) {     \
      fprintf(stderr, dach_fmt, __VA_ARGS__); \
      fprintf(stderr, "%s %i\n", __FILE__, __LINE__);\
    }                                    \
  } while (0)

#endif				/* DACH_LOG_H */
