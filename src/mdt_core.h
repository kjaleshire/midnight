/*

Midnight main header file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef mdt_core_h
#define mdt_core_h

/* INCLUDES */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <getopt.h>
#include <ev.h>					// libev event handler
#include <sys/queue.h>			// queue macros

/* APP CONSTANTS */
#define APP_NAME		"midnight"
#define APP_DESC		"A simple threaded+evented HTTP server"
#define MAJOR_V			0
#define MINOR_V			1
#define PATCH_V			3
#define PRERELEASE_V	""

/* CONSTANT DEFINITIONS */
#define LISTENQ			1024
#define MAXQUEUESIZE	4
#define TIMEFMT			"%H:%M:%S %m.%d.%y"
#define TIMESTAMP_SIZE	32

/* error types */
#define ERRPROG			-1
#define ERRSYS			-2

/* logging levels */
enum {
	LOGNONE,
	LOGFATAL,
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
    STAILQ_ENTRY(conn_data) q_next;
} conn_data;

typedef struct thread_info {
	pthread_t thread_id;
	int thread_continue;
} thread_info;

struct {
	sem_t* sem_q_empty;
	sem_t* sem_q_full;
	pthread_mutex_t mtx_conn_queue;
	STAILQ_HEAD(queue_struct, conn_data) conn_queue;
} queue_info;

struct {
	int log_level;
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

static struct option optstruct[] = {
	{ "help", no_argument, NULL, 'h'},
	{ "error", no_argument, NULL, 'e'},
	{ "quiet", no_argument, NULL, 'q'},
	{ "port", required_argument, NULL, 'p'},
	{ "address", required_argument, NULL, 'a'},
	{ "docroot", required_argument, NULL, 'd'},
	{ "nthreads", required_argument, NULL, 't'},
	{ "version", no_argument, NULL, 'v'}
};

void mdt_worker(thread_info* opts);

void mdt_accept_cb(struct ev_loop* loop, ev_io* watcher_accept, int revents);
void mdt_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents);
void mdt_usage();

/* MACRO DEFINITIONS */
#ifdef DEBUG
#define LOG_FD stdout
#define mdt_log(e, m, ...)	\
		do {	\
			if((e) <= log_info.log_level) {	\
				log_info.ticks = time(NULL);	\
				log_info.current_time = localtime(&log_info.ticks);	\
				strftime(log_info.timestamp, TIMESTAMP_SIZE, TIMEFMT, log_info.current_time);	\
				pthread_mutex_lock(&log_info.mtx_term);	\
				fprintf(LOG_FD, "%s  ", log_info.timestamp);	\
				fprintf(LOG_FD, "%x:\t", (unsigned int) pthread_self());	\
				fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
				fprintf(LOG_FD, "\n");	\
				pthread_mutex_unlock(&log_info.mtx_term);	\
			}	\
		} while(0)
#define TRACE()  mdt_log(LOGDEBUG, "> %s:%d:%s", __FILE__, __LINE__, __FUNCTION__)
#else
#define LOG_FD stderr
#define mdt_log(e, m, ...)	\
		do {	\
			if((e) <= log_info.log_level) {	\
				pthread_mutex_lock(&log_info.mtx_term);	\
				fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
				fprintf(LOG_FD, "\n");	\
				pthread_mutex_unlock(&log_info.mtx_term);	\
			}	\
		} while(0)
#define TRACE()
#endif

/* spoilers: everyone dies */
#define mdt_fatal(m, ...)	\
		do {	\
		    if(LOGFATAL <= log_info.log_level) {	\
		    	log_info.ticks = time(NULL);	\
				log_info.current_time = localtime(&log_info.ticks);	\
				strftime(log_info.timestamp, TIMESTAMP_SIZE, TIMEFMT, log_info.current_time);	\
		        pthread_mutex_lock(&log_info.mtx_term);	\
		        fprintf(LOG_FD, "%s  ", log_info.timestamp);	\
		        fprintf(LOG_FD, "%x:\t> %s:%d:%s:\tfatal: \"", (unsigned int) pthread_self(), __FILE__, __LINE__, __FUNCTION__);	\
		        fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
		        fprintf(LOG_FD, "\"\n");	\
		        pthread_mutex_unlock(&log_info.mtx_term);	\
		    }	\
		    exit(ERRPROG);	\
		} while(0)

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

#define mdt_options_init()	\
		do {	\
			options_info.n_threads = 2;	\
			options_info.address = htonl(INADDR_ANY);	\
			options_info.port = htons(DEFAULT_PORT);	\
			options_info.docroot = DEFAULT_DOCROOT;	\
			log_info.log_level = LOGINFO;	\
		} while(0)
		
#define mdt_version() printf("  %s version %d.%d.%d%s\n", APP_NAME, MAJOR_V, MINOR_V, PATCH_V, PRERELEASE_V)

#endif /* mdt_core_h */