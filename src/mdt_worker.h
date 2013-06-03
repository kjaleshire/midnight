/*

midnight thread header file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef mdt_worker_h
#define mdt_worker_h

#include <sys/stat.h>
#include <fcntl.h>

/* buffer sizes */
static const int RESPSIZE =		8 * 1024;
static const int REQSIZE =		8 * 1024;

/* request types */
static const char* GET =			"GET";
static const char* POST =			"POST";
static const char* OPTIONS =		"OPTIONS";
static const char* HEAD =			"HEAD";
static const char* PUT =			"PUT";

/* response status types */
static const char* HTTP11 =			"HTTP/1.1";
static const char* OK_S =			"200 OK";
static const char* NF_S =			"404 Not Found";
static const char* SRVERR_S =		"500 Internal Server Error";

/* HTTP headers */
static const char* DATE_H =				"Date:";
static const char* CONTENT_H =			"Content-Type:";
static const char* EXPIRES_H =			"Expires:";
static const char* SERVER_H =			"Server:";
static const char* HOST_H =				"Host:";
static const char* CONN_H =				"Connection:";
static const char* CONTENT_LENGTH_H =	"Content-Length:";

/* HTTP header stock values	*/
static const char* CONN_CLOSE	=		"close";
static const char* CONN_KEEPALIVE =		"keep-alive";
static const char* SERVER_NAME =		APP_NAME;
static const char* EXPIRES_NEVER =		"-1";

/* MIME types... */
static const char* MIME_HTML =			"text/html;";
static const char* MIME_JPG =			"image/jpeg;";
static const char* MIME_GIF =			"image/gif;";
static const char* MIME_PNG =			"image/png;";
static const char* MIME_CSS =			"text/css;";
static const char* MIME_JS =			"application/javascript;";
static const char* MIME_TXT =			"text/plain;";

/* default character set */
static const char* CHARSET =			"charset=utf-8";

/* HTTP line terminator */
static const char* CRLF =				"\r\n";

/* header formats */
static const char* HEADER_FMT =			"%s %s%s";
static const char* CONTENT_FMT =		"%s %s %s%s";
static const char* DATE_FMT =			"%s %.24s%s";

typedef struct response {
    char buffer[RESPSIZE];
    int buffer_index;
    struct stat filestat;

    char* content_type;
    char* charset;
    char* http_version;
    char* status;
    char* current_time;
    char* expires;
    char* servername;
    char* connection;
    char content_length[16];
    char* content;

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
void mdt_res_init(response* res);
void mdt_req_init(request* req);
void mdt_req_destroy(request* req);

#define mdt_res_buff(r, m, ...)	\
		do {	\
			(r)->buffer_index += snprintf(&(r)->buffer[(r)->buffer_index], \
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

#define mdt_dict_add(k, n)	\
		do {	\
			HASH_ADD_KEYPTR(hh, ((request *) data)->table, k, strlen(k), n);	\
		} while(0)

#endif /* mdt_worker_h */