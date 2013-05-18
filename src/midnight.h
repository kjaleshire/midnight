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
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <copyfile.h>
#include <ev.h>					// libev event handler
#include <sys/queue.h>			// queue macros
#include "http11_parser.h"		// parser header
#include "uthash.h"				// hash table macros

/* CONSTANT DEFINITIONS */
#define DEBUG	1
#define N_THREADS 1

#define RESSIZE			8 * 1024
#define REQSIZE			8 * 1024
#define READBUFF		16 * 1024
#define LISTENQ			1024
#define MAXQUEUESIZE	N_THREADS * 2
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

/* request types */
#define GET				"GET"
#define POST			"POST"
#define OPTIONS			"OPTIONS"
#define HEAD			"HEAD"
#define PUT				"PUT"

/* response status types */
#define HTTP11			"HTTP/1.1"
#define OK_S			"200 OK"
#define NF_S			"404 Not Found"
#define SRVERR_S		"500 Internal Server Error"

/* default filename */
#define DEFAULT_FILE	"index.html"
#define DOCROOT			"site"

/* headers */
#define DATE_H			"Date:"
#define CONTENT_H		"Content-Type:"
#define EXPIRES_H		"Expires:"
#define SERVER_H		"Server:"
#define HOST_H			"Host:"
#define CONN_H			"Connection:"

/* header stock values	*/
#define CONN_CLOSE		"close"
#define CONN_KEEPALIVE	"keep-alive"
#define SERVER_NAME		"midnight"
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

typedef struct http_header {
	char* key;
	char* value;
	UT_hash_handle hh;
} http_header;

typedef struct response {
    char buffer[RESSIZE];
    int buffer_index;

    char* content_type;
    char* charset;
    char* http_version;
    char* status;
    char* current_time;
    char* expires;
    char* servername;
    char* connection;
    char* content;				// for testing only

    char* file;
	char* mimetype;
} response;

typedef struct request {
	char buffer[REQSIZE];
	int buffer_index;

	http_header* table;

	char* request_method;
	char* request_uri;
	char* fragment;
	char* request_path;
	char* query_string;
	char* http_version;
} request;

typedef struct conn_state {
	response* res;

	http_parser* parser;

	conn_data* conn;
	conn_data* old_conn;

	int cs;
} conn_state;

typedef struct thread_info {
	pthread_t thread_id;
} thread_info;

typedef int (*conn_state_cb)(conn_state* state);

struct {
	conn_state_cb parse_init;
	conn_state_cb parse_exec;
	conn_state_cb read_request_method;
	conn_state_cb validate_get;
	conn_state_cb send_get_response;
	conn_state_cb send_request_invalid;
	conn_state_cb send_404_response;
	conn_state_cb cleanup;
} state_actions;

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

void md_worker(thread_info* opts);

int md_state_init(conn_state* state);
int md_state_event(conn_state* state, int event);

int md_parse_init(conn_state* state);
int md_parse_exec(conn_state* state);
int md_read_request_method(conn_state* state);
int md_validate_get(conn_state* state);
int md_send_get_response(conn_state* state);
int md_send_request_invalid(conn_state* state);
int md_send_404_response(conn_state* state);
int md_cleanup(conn_state* state);

void md_accept_cb(struct ev_loop* loop, ev_io* watcher_accept, int revents);
void md_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents);

/* MACRO DEFINITIONS */
#ifdef DEBUG
#define LOG_FD stdout
#define md_log(e, m, ...)	\
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
#define TRACE()  md_log(LOGDEBUG, "> %s:%d:%s", __FILE__, __LINE__, __FUNCTION__)
#else
#define LOG_FD stderr
#define md_log(e, m, ...)	\
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

#define md_log_init()	\
		do {	\
			pthread_mutexattr_t mtx_attr;	\
    		if( pthread_mutexattr_init(&mtx_attr) != 0 ||	\
	        pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||	\
	        pthread_mutex_init(&log_info.mtx_term, &mtx_attr) != 0 ) {	\
	            printf("System error: unable to initialize terminal mutex");	\
	            exit(ERRSYS);	\
        	}	\
		} while(0)

#define md_res_buff(r, m, ...)	\
		do {	\
			(r)->buffer_index += snprintf(&((r)->buffer[(r)->buffer_index]), \
			RESSIZE - (r)->buffer_index, (m), ##__VA_ARGS__);	\
			assert((r)->buffer_index < RESSIZE);	\
		} while(0)

#define md_res_write(c, r)	\
		do {	\
    		assert((r)->http_version != NULL && (r)->status != NULL && "HTTP version & status not set");	\
    			md_res_buff((r), HEADER_FMT, (r)->http_version, (r)->status, CRLF);		\
    		if((r)->content_type != NULL && (r)->charset != NULL) {		\
    			md_res_buff((r), CONTENT_FMT, CONTENT_H, (r)->content_type, (r)->charset, CRLF); }	\
    		if((r)->current_time != NULL) {		\
    			md_res_buff((r), DATE_FMT, DATE_H, (r)->current_time, CRLF); }	\
    		if((r)->expires != NULL) {	\
    			md_res_buff((r), HEADER_FMT, EXPIRES_H, (r)->expires, CRLF); }	\
    		if((r)->expires != NULL) {	\
    			md_res_buff((r), HEADER_FMT, SERVER_H, (r)->servername, CRLF); }	\
    		if((r)->connection != NULL) {	\
    			md_res_buff((r), HEADER_FMT, CONN_H, (r)->connection, CRLF); }	\
    		md_res_buff((r), CRLF);	\
    		if((r)->content != NULL) {	\
    			md_res_buff((r), (r)->content, CRLF); }		\
			if(write((c)->open_sd, (r)->buffer, (r)->buffer_index) < 0) {	\
				md_log(LOGINFO, "socket write failed");	\
			}	\
		} while(0)

#define md_req_read(c, r)	\
		do {	\
			if ( ((r)->buffer_index += read((c)->open_sd,	\
			(r)->buffer, REQSIZE - (r)->buffer_index)) < 0) {	\
				md_fatal("read request fail from %s, sd: %d", inet_ntoa((c)->conn_info.sin_addr), (c)->open_sd);	\
            }	\
            assert((r)->buffer_index < REQSIZE - 1);	\
            (r)->buffer[(r)->buffer_index] = '\0';	\
		} while(0)

#define md_res_init(r)	\
		do {	\
			(r)->buffer_index = 0;	\
			(r)->content_type = NULL;	\
			(r)->charset = NULL;	\
			(r)->http_version = NULL;	\
			(r)->status = NULL;	\
			(r)->current_time = NULL;	\
			(r)->expires = NULL;	\
			(r)->servername = NULL;	\
			(r)->connection = NULL;	\
			(r)->content = NULL;	\
			(r)->file = NULL;	\
			(r)->mimetype = NULL;	\
		} while(0)

#define md_req_init(r)	\
		do {	\
			(r)->buffer_index = 0;	\
			(r)->table = NULL;	\
			(r)->request_method = NULL;	\
			(r)->request_uri = NULL;	\
			(r)->fragment = NULL;	\
			(r)->request_path = NULL;	\
			(r)->query_string = NULL;	\
			(r)->http_version = NULL;	\
		} while(0)

#define md_req_destroy(r)	\
		do {	\
			http_header *s, *tmp;	\
			HASH_ITER(hh, (r)->table, s, tmp) {	\
				HASH_DEL((r)->table, s);	\
				free(s->key);	\
				free(s->value);	\
				free(s);	\
			}	\
			(r)->buffer[0] = '\0';	\
			if((r)->request_method != NULL) free((r)->request_method);	\
	        if((r)->request_uri != NULL) free((r)->request_uri);	\
	        if((r)->fragment != NULL) free((r)->fragment);	\
	        if((r)->request_path != NULL) free((r)->request_path);	\
	        if((r)->query_string != NULL) free((r)->query_string);	\
	        if((r)->http_version != NULL) free((r)->http_version);	\
		} while (0)

#define md_set_state_actions(a)	\
		do {	\
			(a)->parse_init = md_parse_init;	\
			(a)->parse_exec = md_parse_exec;	\
			(a)->read_request_method = md_read_request_method;	\
			(a)->validate_get = md_validate_get;	\
			(a)->send_get_response = md_send_get_response;	\
			(a)->send_request_invalid = md_send_request_invalid;	\
			(a)->send_404_response = md_send_404_response;	\
			(a)->cleanup = md_cleanup;	\
		} while(0)

#endif /* midnight_h */