/*

midnight thread header file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef mdt_worker_h
#define mdt_worker_h

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

/* buffer sizes */
static const int RESPSIZE =		8 * 1024;
static const int REQSIZE =		8 * 1024;

/* request types */
static char* GET =			"GET";
static char* POST =			"POST";
static char* OPTIONS =		"OPTIONS";
static char* HEAD =			"HEAD";
static char* PUT =			"PUT";

/* response status types */
static char* HTTP11 =		"HTTP/1.1";
static char* OK_S =			"200 OK";
static char* NF_S =			"404 Not Found";
static char* SRVERR_S =		"500 Internal Server Error";

/* HTTP headers */
static char* DATE_H =			"Date:";
static char* CONTENT_H =		"Content-Type:";
static char* EXPIRES_H =		"Expires:";
static char* SERVER_H =			"Server:";
static char* HOST_H =			"Host:";
static char* CONN_H =			"Connection:";
static char* CONTENT_LENGTH_H =	"Content-Length:";

/* HTTP header stock values	*/
static char* CONN_CLOSE	=		"close";
static char* CONN_KEEPALIVE =	"keep-alive";
static char* SERVER_NAME =		APP_NAME;
static char* EXPIRES_NEVER =	"-1";

/* MIME types... */
static char* MIME_HTML =		"text/html;";
static char* MIME_JPG =			"image/jpeg;";
static char* MIME_GIF =			"image/gif;";
static char* MIME_PNG =			"image/png;";
static char* MIME_CSS =			"text/css;";
static char* MIME_JS =			"application/javascript;";
static char* MIME_TXT =			"text/plain;";

/* default character set */
static char* CHARSET =			"charset=utf-8";

/* HTTP line terminator */
static char* CRLF =				"\r\n";

/* header formats */
static char* HEADER_FMT =		"%s %s%s";
static char* CONTENT_FMT =		"%s %s %s%s";
static char* DATE_FMT =			"%s %.24s%s";

typedef struct response {
    char buffer[RESPSIZE];
    int buffer_index;

    char* content_type;
    char* charset;
    char* http_version;
    char* status;
    char* current_time;
    char* expires;
    char* servername;
    char* connection;
    char content_length[16];
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

void mdt_worker(thread_info* opts);

int mdt_state_init(conn_state* state);
int mdt_state_event(conn_state* state, int event);

int mdt_parse_init(conn_state* state);
int mdt_parse_exec(conn_state* state);
int mdt_read_request_method(conn_state* state);
int mdt_validate_get(conn_state* state);
int mdt_send_get_response(conn_state* state);
int mdt_send_request_invalid(conn_state* state);
int mdt_send_404_response(conn_state* state);
int mdt_cleanup(conn_state* state);

char* mdt_detect_type(char* filename);
int mdt_res_write(conn_data* conn, response* res);

#define mdt_res_buff(r, m, ...)	\
		do {	\
			(r)->buffer_index += snprintf(&((r)->buffer[(r)->buffer_index]), \
			RESPSIZE - (r)->buffer_index, (m), ##__VA_ARGS__);	\
			assert((r)->buffer_index < RESPSIZE);	\
		} while(0)

#define mdt_req_read(c, r)	\
		do {	\
			if ( ((r)->buffer_index += read((c)->open_sd, (r)->buffer + (r)->buffer_index, REQSIZE - (r)->buffer_index)) < 0) {	\
				mdt_fatal(ERRSYS, "read request fail from %s, sd: %d", inet_ntoa((c)->conn_info.sin_addr), (c)->open_sd);	\
            }	\
            assert((r)->buffer_index < REQSIZE - 1);	\
            (r)->buffer[(r)->buffer_index] = '\0';	\
		} while(0)

#define mdt_res_init(r)	\
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

#define mdt_req_init(r)	\
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

#define mdt_req_destroy(r)	\
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

#define mdt_set_state_actions(a)	\
		do {	\
			(a)->parse_init = mdt_parse_init;	\
			(a)->parse_exec = mdt_parse_exec;	\
			(a)->read_request_method = mdt_read_request_method;	\
			(a)->validate_get = mdt_validate_get;	\
			(a)->send_get_response = mdt_send_get_response;	\
			(a)->send_request_invalid = mdt_send_request_invalid;	\
			(a)->send_404_response = mdt_send_404_response;	\
			(a)->cleanup = mdt_cleanup;	\
		} while(0)

#define mdt_dict_add(k, n)	\
		do {	\
			HASH_ADD_KEYPTR(hh, ((request *) data)->table, k, strlen(k), n);	\
		} while(0)

#endif /* mdt_worker_h */