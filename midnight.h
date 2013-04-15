/*

Midnight main header file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef __midnight_h
#define __midnight_h

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

/* MACRO DEFINITIONS */
#define MD_BUFF(info, msg, ...)	\
		info->buff_dex += snprintf(&(info->buff[info->buff_dex]), \
		BUFFSIZE - info->buff_dex, msg, ##__VA_ARGS__)

#ifdef DEBUG
#define MD_LOG(errlev, msg, ...)	\
		do {	\
			if(errlev <= log_level) {	\
				pthread_mutex_lock(&mtx_term);	\
				fprintf(log_fd, "tID %d", pthread_self());
				fprintf(log_fd, msg, ##__VA_ARGS__);	\
				fprintf(log_fd, "\n");	\
				pthread_mutex_unlock(&mtx_term);	\
			}	\
		} while(0)
#else
#define MD_LOG(errlev, msg, ...)	\
		do {	\
			if(errlev <= log_level) {	\
				pthread_mutex_lock(&mtx_term);	\
				fprintf(log_fd, msg, ##__VA_ARGS__);	\
				fprintf(log_fd, "\n");	\
				pthread_mutex_unlock(&mtx_term);	\
			}	\
		} while(0)
#endif

#define MD_LOG_INIT()	\
		do {	\
			pthread_mutexattr_t mtx_attr;	\
    		if( pthread_mutexattr_init(&mtx_attr) != 0 ||	\
	        pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||	\
	        pthread_mutex_init(&mtx_term, &mtx_attr) != 0 ) {	\
	            printf("System error: unable to initialize terminal mutex");	\
	            exit(ERRSYS);	\
        	}	\
		} while(0)

/* CONSTANT DEFINITIONS */
#define DEBUG
#define N_THREADS 4

#define BUFFSIZE		16384
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
#define NF_S		"404 Not Found"
#define NF_HTML	"<html><body><p>Error 404,			\
				resource not found.</p></body></html>"

/* default filename */
#define DEFAULT_FILE	"index.html"

/* response headers */
#define DATE_H			"Date: "
#define CONTENT_T_H		"Content-Type: "
#define EXPIRES_H		"Expires: -1"
#define SERVER_H		"Server: midnight"

/* MIME types... */
#define MIME_HTML		"text/html; "
#define MIME_JPG		"image/jpeg; "
#define MIME_GIF		"image/gif; "
#define MIME_PNG		"image/png; "
#define MIME_CSS		"text/css; "
#define MIME_JS			"application/javascript; "
#define MIME_TXT		"text/plain; "

/* and character set */
#define CHARSET			"charset=utf-8"

/* request headers for comparison */
#define HOST_H			"Host:"
#define CONNECTION_H	"Connection:"

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

typedef struct thread_info {
	int thread_no;

	char buff[BUFFSIZE];
    int buff_dex;
} thread_info_t;

typedef struct reqargs {
	uint32_t conn_flags;

	char* file;
	char* mimetype;

} reqargs_t;

typedef struct conn_data {
    int open_sd;
    struct sockaddr_in conn_info;
    STAILQ_ENTRY(conn_data) conn_q_next;
} conn_data_t;

int log_level;
int listen_sd;
FILE* log_fd;
pthread_mutex_t mtx_term;
pthread_mutex_t mtx_conn_queue;
sem_t* sem_q_empty;
sem_t* sem_q_full;
extern STAILQ_HEAD(conn_q_head_struct, conn_data) conn_q_head;

void md_worker(thread_info_t *t_info);
void md_panic(const char* message, ...);
void md_accept_cb(struct ev_loop*, ev_io*, int);

#endif /* __midnight_h */