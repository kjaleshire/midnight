/*

midnight core header

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef mdt_core_h
#define mdt_core_h

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <ev.h>					// libev event handler

/* app meta constants */
#define APP_NAME		"midnight"
#define APP_DESC		"A simple threaded+evented HTTP server"
#define MAJOR_V			0
#define MINOR_V			1
#define PATCH_V			3
#define PRERELEASE_V	""

/* app-wide constants */
#define LISTENQ			1024
#define TIMESTAMP_FMT	"%H:%M:%S %m.%d.%y"
#define TIMESTAMP_SIZE	32

/* error return types */
#define ERRPROG			-2
#define ERRSYS			-1

/* logging levels */
enum {
	LOGNONE,
	LOGERR,
	LOGINFO,
	LOGDEBUG
};

/* defaults */
#define DEFAULT_PORT		8080
#define DEFAULT_FILE		"index.html"
#define DEFAULT_DOCROOT		"docroot"

typedef struct conn_data {
    int open_sd;
    struct sockaddr_in conn_info;
} conn_data;

typedef struct thread_info {
	pthread_t thread_id;
	int listen_sd;
	int exec_continue;
} thread_info;

struct {
	int level;
	char timestamp[TIMESTAMP_SIZE];
	time_t ticks;
	struct tm* current_time;

	pthread_mutex_t mtx_term;
} log_info;

struct {
	int n_threads;
	char* docroot;
	uint16_t port;
	uint32_t address;
} options_info;

/* log+fatal macros */
#define mdt_log_init()	\
		do {	\
			pthread_mutexattr_t mtx_attr;	\
    		if( pthread_mutexattr_init(&mtx_attr) != 0 ||	\
	        pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||	\
	        pthread_mutex_init(&log_info.mtx_term, &mtx_attr) != 0 ) {	\
	            printf("System error: unable to initialize terminal mutex");	\
	            exit(ERRSYS);	\
        	}	\
		} while(0)
#define LOG_FD stderr
#ifdef DEBUG
#define mdt_log(e, m, ...)	\
		do {	\
			if((e) <= log_info.level) {	\
				log_info.ticks = time(NULL);	\
				log_info.current_time = localtime(&log_info.ticks);	\
				strftime(log_info.timestamp, TIMESTAMP_SIZE, TIMESTAMP_FMT, log_info.current_time);	\
				pthread_mutex_lock(&log_info.mtx_term);	\
					fprintf(LOG_FD, "%s  ", log_info.timestamp);	\
					fprintf(LOG_FD, "%x:\t", (unsigned int) pthread_self());	\
					fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
					fprintf(LOG_FD, "\n");	\
				pthread_mutex_unlock(&log_info.mtx_term);	\
			}	\
		} while(0)
#else
#define mdt_log(e, m, ...)	\
		do {	\
			if((e) <= log_info.level) {	\
				pthread_mutex_lock(&log_info.mtx_term);	\
					fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
					fprintf(LOG_FD, "\n");	\
				pthread_mutex_unlock(&log_info.mtx_term);	\
			}	\
		} while(0)
#endif

#ifdef DEBUG
#define TRACE()  mdt_log(LOGDEBUG, "> %s:%d:%s", __FILE__, __LINE__, __FUNCTION__)
#else
#define TRACE()
#endif

/* spoilers: everyone dies */
#define mdt_fatal(e, m, ...)	\
		do {	\
			if(LOGERR <= log_info.level) {	\
				log_info.ticks = time(NULL);	\
				log_info.current_time = localtime(&log_info.ticks);	\
				strftime(log_info.timestamp, TIMESTAMP_SIZE, TIMESTAMP_FMT, log_info.current_time);	\
				pthread_mutex_lock(&log_info.mtx_term);	\
			        fprintf(LOG_FD, "%s  ", log_info.timestamp);	\
			        fprintf(LOG_FD, "%x:\t> %s:%d:%s:\tfatal: \"", (unsigned int) pthread_self(), __FILE__, __LINE__, __FUNCTION__);	\
			        fprintf(LOG_FD, ":\t> %s:%d:%s:\tfatal: \"", __FILE__, __LINE__, __FUNCTION__);	\
			        fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
			        fprintf(LOG_FD, "\"\n");	\
				pthread_mutex_unlock(&log_info.mtx_term);	\
				exit((e));	\
			}	\
		} while(0)

#define mdt_version() printf("%s version %d.%d.%d%s\n", APP_NAME, MAJOR_V, MINOR_V, PATCH_V, PRERELEASE_V)

#endif /* mdt_core_h */