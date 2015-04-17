#ifndef	_LOG_H_
#define	_LOG_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define	ERROR	1
#define	WARN	2
#define	INFO	3
#define	DEBUG	4

extern	pthread_mutex_t	stderr_for_log_mutex;

#define	LOG_INTERNAL(level, ...)	\
	do {		\
		time_t	t_now;			\
		struct tm	tm_now; 	\
		char	t_buf[16];		\
		size_t	t_id;			\
						\
		t_now = time(NULL);		\
		localtime_r(&t_now, &tm_now);	\
		strftime(t_buf, sizeof(t_buf), "%Y%m%d %H%M%S", &tm_now);	\
		t_id = ((size_t)pthread_self())&0xffffff;			\
		pthread_mutex_lock(&stderr_for_log_mutex);			\
		fprintf(stderr, "%s T%06lx: %s: ", t_buf, t_id, #level);	\
		fprintf(stderr, "%s: %s:%d: ", __func__, __FILE__, __LINE__);	\
		fprintf(stderr, __VA_ARGS__);					\
		fprintf(stderr, "\n");						\
		pthread_mutex_unlock(&stderr_for_log_mutex);			\
	} while(0)

#define	LOG_ERROR(...)	LOG_INTERNAL(ERROR, __VA_ARGS__)
#define	LOG_WARN(...)	LOG_INTERNAL(WARN, __VA_ARGS__)
#define	LOG_INFO(...)	LOG_INTERNAL(INFO, __VA_ARGS__)
#define	LOG_DEBUG(...)	LOG_INTERNAL(DEBUG, __VA_ARGS__)

#endif
