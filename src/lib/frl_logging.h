#ifndef GUARD_frl_logging_h
#define GUARD_frl_logging_h

#include "frl_base.h"
#include <stdio.h>

#define FLOG(type, format, ...) \
	switch (type) { \
		case FRL_LEVEL_ERROR: \
			fprintf(stderr, format, ## __VA_ARGS__); \
			break; \
		case FRL_LEVEL_WARNING: \
			fprintf(stdout, format, ## __VA_ARGS__); \
			break; \
		case FRL_LEVEL_INFO: \
			fprintf(stdout, format, ## __VA_ARGS__); \
			break; \
	}

#define FLOG_IF(type, cond, format, ...) \
	if (cond) { \
		FLOG(type, format, ## __VA_ARGS__); \
	}

#define FLOG_IF_RUN(type, cond, run, format, ...) \
	if (cond) { \
		FLOG(type, format, ## __VA_ARGS__); \
		run; \
	}

#define F_ERROR(format, ...) \
	FLOG(FRL_LEVEL_ERROR, format, ## __VA_ARGS__)

#define F_ERROR_IF(cond, format, ...) \
	FLOG_IF(FRL_LEVEL_ERROR, cond, format, ## __VA_ARGS__)

#define F_ERROR_IF_RUN(cond, run, format, ...) \
	FLOG_IF_RUN(FRL_LEVEL_ERROR, cond, run, format, ## __VA_ARGS__)

#define F_WARNING(format, ...) \
	FLOG(FRL_LEVEL_WARNING, format, ## __VA_ARGS__)

#define F_WARNING_IF(cond, format, ...) \
	FLOG_IF(FRL_LEVEL_WARNING, cond, format, ## __VA_ARGS__)

#define F_WARNING_IF_RUN(cond, run, format, ...) \
	FLOG_IF_RUN(FRL_LEVEL_WARNING, cond, run, format, ## __VA_ARGS__)

#define F_INFO(format, ...) \
	FLOG(FRL_LEVEL_INFO, format, ## __VA_ARGS__)

#define F_INFO_IF(cond, format, ...) \
	FLOG_IF(FRL_LEVEL_INFO, cond, format, ## __VA_ARGS__)

#define F_INFO_IF_RUN(cond, run, format, ...) \
	FLOG_IF_RUN(FRL_LEVEL_INFO, cond, run, format, ## __VA_ARGS__)

#endif
