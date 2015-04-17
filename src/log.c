#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t	stderr_for_log_mutex = PTHREAD_MUTEX_INITIALIZER;
