/*

Midnight main header file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef __midnight_h
#define __midnight_h

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
#include <ev.h>

#include <sys/queue.h>

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

/* response types. HTTP 1.0 since we're not (yet) supporting a lot of HTTP 1.1 (specifically Connection: keep-alive) */
#define HTTP11_R		"HTTP/1.1"
#define HTTP10_R		"HTTP/1.0"
#define OK_R			"200 OK"
#define NOTFOUND_R		"404 Not Found"
#define NOTFOUND_HTML	"<html><body><p>Error 404, resource not found.</p></body></html>"

/* default filename */
#define DEFAULT_FILE		"index.html"

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

typedef struct thread_args {
	int thread_no;
} thread_args_t;

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

extern STAILQ_HEAD(conn_q_head_struct, conn_data) conn_q_head;

int log_level;
int listen_sd;
FILE* log_fd;
pthread_mutex_t mtx_term;
pthread_mutex_t mtx_conn_queue;
sem_t* sem_q_empty;
sem_t* sem_q_full;

void worker_thread(thread_args_t *);
void get_handler(reqargs_t*, void*);
char* detect_type(char*);

int log_init();
void panic(const char* message, ...);
void logmsg(int err_level, const char* message, ...);

void accept_ready_cb(struct ev_loop*, ev_io*, int);

#endif /* __midnight_h */