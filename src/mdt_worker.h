/*

Worker thread header file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef mdt_worker_h
#define mdt_worker_h

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
char* md_detect_type(char* filename);

#define md_res_buff(r, m, ...)	\
		do {	\
			(r)->buffer_index += snprintf(&((r)->buffer[(r)->buffer_index]), \
			RESPSIZE - (r)->buffer_index, (m), ##__VA_ARGS__);	\
			assert((r)->buffer_index < RESPSIZE);	\
		} while(0)

#define md_res_write(c, r)	\
		do {	\
    		assert((r)->http_version != NULL && (r)->status != NULL && "HTTP version & status not set");	\
    			md_res_buff((r), HEADER_FMT, (r)->http_version, (r)->status, CRLF);		\
    		if((r)->content_type != NULL && (r)->charset != NULL) {		\
    			md_res_buff((r), CONTENT_FMT, CONTENT_H, (r)->content_type, (r)->charset, CRLF); }	\
    		if((r)->current_time != NULL) {		\
    			md_res_buff((r), DATE_FMT, DATE_H, (r)->current_time, CRLF); }	\
    		if((r)->content_length != NULL) {	\
    			md_res_buff((r), HEADER_FMT, CONTENT_LENGTH_H, (r)->content_length, CRLF); }	\
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
				mdt_log(LOGINFO, "socket write failed");	\
			}	\
		} while(0)

#define md_req_read(c, r)	\
		do {	\
			if ( ((r)->buffer_index += read((c)->open_sd, (r)->buffer + (r)->buffer_index, REQSIZE - (r)->buffer_index)) < 0) {	\
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

#define md_dict_add(k, n)	\
		do {	\
			HASH_ADD_KEYPTR(hh, ((request *) data)->table, k, strlen(k), n);	\
		} while(0)

#endif /* mdt_worker_h */