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
#include <uthash.h>				// hash table macros

/* APP CONSTANTS */
#define APP_NAME		"midnight"
#define APP_DESC		"A simple threaded+evented HTTP server"
#define MAJOR_V			0
#define MINOR_V			1
#define PATCH_V			3
#define PRERELEASE_V	""

/* CONSTANT DEFINITIONS */
#define RESPSIZE		8 * 1024
#define REQSIZE			8 * 1024
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

/* headers */
#define DATE_H			"Date:"
#define CONTENT_H		"Content-Type:"
#define EXPIRES_H		"Expires:"
#define SERVER_H		"Server:"
#define HOST_H			"Host:"
#define CONN_H			"Connection:"
#define CONTENT_LENGTH_H	"Content-Length:"

/* header stock values	*/
#define CONN_CLOSE		"close"
#define CONN_KEEPALIVE	"keep-alive"
#define SERVER_NAME		APP_NAME
#define EXPIRES_NEVER	"-1"

/* MIME types... */
#define MIME_HTML		"text/html;"
#define MIME_JPG		"image/jpeg;"
#define MIME_GIF		"image/gif;"
#define MIME_PNG		"image/png;"
#define MIME_CSS		"text/css;"
#define MIME_JS			"application/javascript;"
#define MIME_TXT		"text/plain;"

/* default character set */
#define CHARSET			"charset=utf-8"

/* HTTP line terminator */
#define CRLF "\r\n"

/* header formats */
#define HEADER_FMT		"%s %s%s"
#define CONTENT_FMT		"%s %s %s%s"
#define DATE_FMT		"%s %.24s%s"

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

void md_worker(thread_info* opts);

void sig_usr1_handler(int signum);

void md_accept_cb(struct ev_loop* loop, ev_io* watcher_accept, int revents);
void md_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents);
void md_usage();

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
#define md_fatal(m, ...)	\
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

#define md_options_init()	\
		do {	\
			options_info.n_threads = 2;	\
			options_info.address = htonl(INADDR_ANY);	\
			options_info.port = htons(DEFAULT_PORT);	\
			options_info.docroot = DEFAULT_DOCROOT;	\
			log_info.log_level = LOGINFO;	\
		} while(0)
		
#define md_version() printf("  %s version %d.%d.%d%s\n", APP_NAME, MAJOR_V, MINOR_V, PATCH_V, PRERELEASE_V)

#endif /* mdt_core_h */