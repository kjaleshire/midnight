/*

Midnight main header file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef midnight_h
#define midnight_h

/* INCLUDES */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <ev.h>					// libev event handler
#include <sys/queue.h>			// queue macros
#include "midnight_parser.h"	// parser header
#include "uthash.h"				// hash table macros

/* CONSTANT DEFINITIONS */
#define DEBUG
#define N_THREADS 4

#define RESSIZE			8 * 1024
#define REQSIZE			8 * 1024
#define LISTENQ			1024
#define DOCROOT			"site"
#define MAXQUEUESIZE	16

/* error types */
#define ERRPROG			-1
#define ERRSYS			-2

/* logging levels */
enum {
	LOGNONE,
	LOGPANIC,
	LOGERR,
	LOGINFO,
	LOGDEBUG
};

/* request types */
#define GET				"GET"
#define POST			"POST"
#define OPTIONS			"OPTIONS"
#define HEAD			"HEAD"
#define PUT				"PUT"

/* response types */
#define HTTP11_R		"HTTP/1.1"
#define OK_S			"200 OK"
#define NF_S			"404 Not Found"
#define NF_HTML	"<html><body><p>404	file not found.</p></body></html>"

/* default filename */
#define DEFAULT_FILE	"index.html"

/* headers */
#define DATE_H			"Date:"
#define CONTENT_H		"Content-Type:"
#define EXPIRES_H		"Expires:"
#define SERVER_H		"Server:"
#define HOST_H			"Host:"
#define CONN_H			"Connection:"

/* header stock values	*/
#define CLOSE			"close"
#define KEEPALIVE		"keep-alive"
#define SERVERNAME		"midnight"
#define EXPIRESNEVER	"-1"

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

/* special flags */
#define GET_F			1 << 0
#define OPTIONS_F		1 << 1
#define HEAD_F			1 << 2
#define POST_F			1 << 3
#define PUT_F			1 << 4

#define HTTP11_F		1 << 8

#define HOST_F			1 << 9
#define CONNECTION_F	1 << 10

/* HTTP line terminator */
#define CRLF "\r\n"

/* MACRO DEFINITIONS */
/* read a message into the response buffer */
#ifdef DEBUG
#define md_log(errlev, msg, ...)	\
		do {	\
			if(errlev <= log_level) {	\
				pthread_mutex_lock(&mtx_term);	\
				fprintf(log_fd, "%u: ", (unsigned int) pthread_self());	\
				fprintf(log_fd, msg, ##__VA_ARGS__);	\
				fprintf(log_fd, "\n");	\
				pthread_mutex_unlock(&mtx_term);	\
			}	\
		} while(0)
#else
#define md_log(errlev, msg, ...)	\
		do {	\
			if(errlev <= log_level) {	\
				pthread_mutex_lock(&mtx_term);	\
				fprintf(log_fd, msg, ##__VA_ARGS__);	\
				fprintf(log_fd, "\n");	\
				pthread_mutex_unlock(&mtx_term);	\
			}	\
		} while(0)
#endif

#define md_log_init()	\
		do {	\
			pthread_mutexattr_t mtx_attr;	\
    		if( pthread_mutexattr_init(&mtx_attr) != 0 ||	\
	        pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||	\
	        pthread_mutex_init(&mtx_term, &mtx_attr) != 0 ) {	\
	            printf("System error: unable to initialize terminal mutex");	\
	            exit(ERRSYS);	\
        	}	\
		} while(0)

#define md_res_buff(r, m, ...)	\
		do {	\
			(r)->buffer_index += snprintf(&((r)->buffer[(r)->buffer_index]), \
			RESSIZE - (r)->buffer_index, (m), ##__VA_ARGS__);	\
			assert((r)->buffer_index <= RESSIZE);	\
		} while(0)

#define md_res_write(c, r)	\
		do {	\
			if(write((c)->open_sd, (r)->buffer, (r)->buffer_index) < 0) {	\
				md_fatal("socket write failed");	\
			}	\
		} while(0)

#define md_req_read(r, c)	\
		do {	\
			if ( ((r)->buffer_index += read((c)->open_sd,	\
			&((r)->buffer[(r)->buffer_index]),	\
			REQSIZE - (r)->buffer_index)) < 0) {	\
				md_fatal("failed to read request from %s", inet_ntoa((c)->conn_info.sin_addr));	\
            }	\
		} while(0)

typedef struct response {
    char buffer[RESSIZE];
    int buffer_index;

    char* file;
	char* mimetype;
} response;

typedef struct request {
	char buffer[REQSIZE];
	int buffer_index;
	char *method;
	char *uri;
	char *fragment;
	char *request_path;
	char *query_string;
	char *http_version;
} request;

typedef struct conn_data {
    int open_sd;
    struct sockaddr_in conn_info;
    STAILQ_ENTRY(conn_data) conn_q_next;
} conn_data;

typedef struct header {
	char *name;
	char *value;
	UT_hash_handle hh;
} header;

int log_level;
int listen_sd;
FILE *log_fd;
pthread_mutex_t mtx_term;
pthread_mutex_t mtx_conn_queue;
sem_t* sem_q_empty;
sem_t* sem_q_full;
extern STAILQ_HEAD(conn_q_head_struct, conn_data) conn_q_head;

void md_worker();
void md_fatal(const char* message, ...);
void md_accept_cb(struct ev_loop* loop, ev_io* watcher_accept, int revents);
void md_int_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents);

#endif /* midnight_h */