/*

midnight worker thread logic

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include <mdt_core.h>
#include <http11_parser.h>
#include <mdt_hash.h>
#include <mdt_worker.h>
#include <mdt_conn_state.h>

#ifdef DEBUG
#define CALL(A) mdt_log(LOGDEBUG, "event trigger: %d", event); next = state_actions.A(state)
#else
#define CALL(A) next = state_actions.A(state)
#endif

%%{
	machine StateActions;

	import "mdt_conn_state.h";

	alphtype int;

	access state->;

	action parse_init { CALL(parse_init); }
	action parse_exec { CALL(parse_exec); }
	action read_request_method { CALL(read_request_method); }
	action validate_get { CALL(validate_get); }
	action send_get_response { CALL(send_get_response); }
	action send_request_invalid { CALL(send_request_invalid); }
	action send_404_response { CALL(send_404_response); }
	action cleanup { CALL(cleanup); }

	connection = (
		start: (
			OPEN @parse_init -> parse_continue
		),

		parse_continue: (
			PARSE @parse_exec -> parse_continue             |
			PARSE_DONE @read_request_method -> request_read |
			PARSE_ERROR @send_request_invalid -> close      |
			CLOSE @cleanup -> final
		),

		request_read: (
			GET_REQUEST @validate_get -> get_validating     |
			INV_REQUEST @send_request_invalid -> close
		),

		get_validating: (
			GET_VALID @send_get_response -> close           |
			GET_NOT_VALID @send_request_invalid -> close    |
			GET_NOT_FOUND @send_404_response -> close
		)

		close: (
			CLOSE @cleanup -> final
		)
	);

	main := (connection)*;

}%%

%% write data;

mdt_conn_handler = ^(conn_data* conn) {
	int next;
	conn_state *state;

	state = malloc(sizeof(conn_state));

	#ifdef DEBUG
	mdt_log(LOGDEBUG, "awaiting new connection");
	#endif

	TRACE();
	state->conn = conn;

	next = OPEN;

	mdt_state_init(state);

	for(;;) {
		if(next == DONE) {
			break;
		}
		next = mdt_state_event(state, next);
	}

	#ifdef DEBUG
	mdt_log(LOGDEBUG, "finished handling connection");
	#endif
}

int mdt_state_event(conn_state* state, int event) {
	TRACE();
	assert(event >= 10 && event <= 20 && "event out of range");

	int event_queue[2] = {0};

	event_queue[0] = event;
	int next = 0;

	const int *p = event_queue;
	const int *pe = p+1;

	%% write exec;

	return next;
}

int mdt_state_init(conn_state* state) {
	TRACE();

	%% write init;

	return 1;
}

int mdt_parse_init(conn_state* state) {
	TRACE();

	state->parser = malloc(sizeof(http_parser));
	http_parser_init(state->parser);

	state->parser->data = malloc(sizeof(request));
	mdt_req_init((request *) state->parser->data);

	return PARSE;
}

int mdt_parse_exec(conn_state* state) {
	request* req = (request *) state->parser->data;

	TRACE();
	mdt_req_read(state->conn, req);
	if(req->buffer_index == 0) {
		return CLOSE;
	}

	assert(req->buffer_index < REQSIZE && "request size too large. refactor into HTTP Error 413");
	http_parser_execute(state->parser, req->buffer, req->buffer_index, state->parser->nread);
	TRACE();
	if(http_parser_has_error(state->parser)) {
		return PARSE_ERROR;
	} else if(http_parser_is_finished(state->parser)) {
		return PARSE_DONE;
	} else {
		return PARSE;
	}
}

int mdt_read_request_method(conn_state* state) {
	request* req = (request *) state->parser->data;
	response* res;

	if(state->res == NULL) {
		state->res = malloc(sizeof(response));
	}

	res = state->res;

	mdt_res_init(res);

	TRACE();

	state->res = res;
	if(strcmp(req->request_method, GET) == 0) {
		return GET_REQUEST;
	} else {
		return INV_REQUEST;
	}
}

int mdt_send_request_invalid(conn_state* state) {
	response* res;
	time_t ticks;

	state->res = malloc(sizeof(response));
	res = state->res;

	TRACE();
	mdt_log(LOGINFO, "500 Internal Server Error");

	ticks = time(NULL);

	res->http_version = HTTP11;
	res->status = SRVERR_S;
	res->current_time = ctime(&ticks);
	res->servername = APP_NAME;
	res->connection = CONN_CLOSE;
	res->content_type = MIME_HTML;
	res->charset = CHARSET;
	res->content =  "<html>\n                                                             \
						<body>\n                                                          \
							<p style=\"font-weight: bold; font-size: 14px; text-align: center;\">\n   \
							500 Internal Server Error\n                                   \
							</p>\n                                                        \
						</body>\n                                                         \
					</html>";

	mdt_res_write(state->conn, res);

	return CLOSE;
}

int mdt_validate_get(conn_state* state) {
	request* req = (request *) state->parser->data;
	char *f;
	TRACE();

	int n = strlen(req->request_path) + strlen(options_info.docroot);

	if(*(req->request_path + strlen(req->request_path) - 1) == '/') {
		n += strlen(DEFAULT_FILE);
		f = DEFAULT_FILE;
	} else {
		f = "";
	}

	char *s = calloc(n + 1, sizeof(char));

	snprintf(s, n, "%s%s%s", options_info.docroot, req->request_path, f);

	assert(strlen(s) == n);
	mdt_log(LOGDEBUG, "new request_path: %s", s);

	free(req->request_path);
	req->request_path = s;
	mdt_log(LOGDEBUG, "requested file: %s", req->request_path);

	if(req->request_path == NULL || req->request_uri == NULL) {
		return GET_NOT_VALID;
	} else if (stat(req->request_path, &state->res->filestat) < 0 && errno == ENOENT) {
		*(req->request_path + strlen(req->request_path) - 2) = '\0';
		if (stat(req->request_path, &state->res->filestat) < 0 && errno == ENOENT) {
			return GET_NOT_FOUND;
		}
	}

	return GET_VALID;
}

int mdt_send_get_response(conn_state* state) {
	response* res = state->res;
	request* req = (request *) state->parser->data;
	time_t ticks;

	TRACE();
	#ifdef DEBUG
	mdt_log(LOGDEBUG, "200 OK");
	#endif

	off_t file_len = 0;
	int file_fd;

	ticks = time(NULL);

	res->http_version = HTTP11;
	res->status = OK_S;
	res->current_time = ctime(&ticks);
	res->servername = SERVER_NAME;
	res->connection = CONN_CLOSE;
	snprintf(res->content_length, 16,"%lld", res->filestat.st_size);
	res->content_type = mdt_detect_type(req->request_path);
	res->charset = CHARSET;
	res->expires = EXPIRES_NEVER;

	if(mdt_res_write(state->conn, res) < 0 ) {
		return CLOSE;
	}

	if( (file_fd = open(req->request_path, O_RDONLY)) < 0 ) {
		mdt_fatal(ERRSYS, "error opening file: %s", strerror(errno));
	}

	if( sendfile(file_fd, state->conn->open_sd, 0, &file_len, NULL, 0) < 0 ) {
		mdt_fatal(ERRSYS, "error writing file: %s", strerror(errno));
	}

	TRACE();
	close(file_fd);

	return CLOSE;
}

int mdt_send_404_response(conn_state* state) {
	response* res = state->res;
	time_t ticks;
	TRACE();
	mdt_log(LOGDEBUG, "404 Not Found");

	res->http_version = HTTP11;
	res->status = NF_S;
	res->current_time = ctime(&ticks);
	res->servername = SERVER_NAME;
	res->connection = CONN_CLOSE;
	res->content_type = MIME_HTML;
	res->charset = CHARSET;
	res->expires = EXPIRES_NEVER;
	res->content = "<html>\n                                                            \
						<body>\n                                                        \
							<p style=\"font-weight: bold; font-size: 18px; text-align: center;\">\n         \
							404 File Not Found\n                                        \
							</p>\n                                                      \
						</body>\n                                                       \
					</html>";

	mdt_res_write(state->conn, res);

	return CLOSE;
}

int mdt_cleanup(conn_state* state) {
	close(state->conn->open_sd);
	TRACE();

	if(state->old_conn != NULL) {
		free(state->old_conn);
	}
	state->old_conn = state->conn;

	free(state->res); state->res = NULL;
	mdt_req_destroy((request *) state->parser->data);
	free(state->parser->data);
	free(state->parser);

	return DONE;
}

void mdt_res_init(response* res) {
	res->buffer_index = 0;
	res->content_type = NULL;
	res->charset = NULL;
	res->http_version = NULL;
	res->status = NULL;
	res->current_time = NULL;
	res->expires = NULL;
	res->servername = NULL;
	res->connection = NULL;
	res->content = NULL;
	res->file = NULL;
	res->mimetype = NULL;
}

void mdt_req_init(request* req) {
	req->buffer_index = 0;
	req->table = NULL;
	req->request_method = NULL;
	req->request_uri = NULL;
	req->fragment = NULL;
	req->request_path = NULL;
	req->query_string = NULL;
	req->http_version = NULL;
}

void mdt_req_destroy(request* req) {
	http_header *s, *tmp;
	HASH_ITER(hh, req->table, s, tmp) {
		HASH_DEL(req->table, s);
		free(s->key);
		free(s->value);
		free(s);
	}
	req->buffer[0] = '\0';
	if(req->request_method != NULL) {
		free(req->request_method);
	}
    if(req->request_uri != NULL) {
    	free(req->request_uri);
    }
    if(req->fragment != NULL) {
    	free(req->fragment);
    }
    if(req->request_path != NULL) {
    	free(req->request_path);
    }
    if(req->query_string != NULL) {
    	free(req->query_string);
    }
    if(req->http_version != NULL) {
    	free(req->http_version);
  	}
}

int mdt_res_write(conn_data* conn, response* res) {
	assert((res)->http_version != NULL && res->status != NULL && "HTTP version & status not set");

	mdt_res_buff(res, HEADER_FMT, res->http_version, res->status, CRLF);

	if(res->content_type != NULL && res->charset != NULL) {
		mdt_res_buff(res, CONTENT_FMT, CONTENT_H, res->content_type, res->charset, CRLF);
	}
	if(res->current_time != NULL) {
		mdt_res_buff(res, DATE_FMT, DATE_H, res->current_time, CRLF);
	}
	if(res->content_length != NULL) {
		mdt_res_buff(res, HEADER_FMT, CONTENT_LENGTH_H, res->content_length, CRLF);
	}
	if(res->expires != NULL) {
		mdt_res_buff(res, HEADER_FMT, EXPIRES_H, res->expires, CRLF);
	}
	if(res->expires != NULL) {
		mdt_res_buff(res, HEADER_FMT, SERVER_H, res->servername, CRLF);
	}
	if(res->connection != NULL) {
		mdt_res_buff(res, HEADER_FMT, CONN_H, res->connection, CRLF);
	}

	mdt_res_buff(res, "%s", CRLF);

	if(res->content != NULL) {
		mdt_res_buff(res, "%s", res->content);
		mdt_res_buff(res, "%s", CRLF);
	}
	if(write(conn->open_sd, res->buffer, res->buffer_index) < 0) {
		mdt_log(LOGINFO, "socket write failed");
		return -1;
	}
	return 0;
}

char* mdt_detect_type(char* filename) {
	char *c;
	if( (c = strrchr(filename, '.')) == NULL || strcmp(c, ".txt") == 0 ) {
		return MIME_TXT;
	} else if( strcmp(c, ".html") == 0 || strcmp(c, ".htm") == 0) {
		return MIME_HTML;
	} else if( strcmp(c, ".jpeg") == 0 || strcmp(c, ".jpg") == 0) {
		return MIME_JPG;
	} else if( strcmp(c, ".gif") == 0 ) {
		return MIME_GIF;
	} else if( strcmp(c, ".png") == 0 ) {
		return MIME_PNG;
	} else if( strcmp(c, ".css") == 0 ) {
		return MIME_CSS;
	} else if( strcmp(c, ".js") == 0 ) {
		return MIME_JS;
	} else {
		return MIME_TXT;
	}
}

void mdt_set_state_actions() {
	state_actions.parse_init = mdt_parse_init;
	state_actions.parse_exec = mdt_parse_exec;
	state_actions.read_request_method = mdt_read_request_method;
	state_actions.validate_get = mdt_validate_get;
	state_actions.send_get_response = mdt_send_get_response;
	state_actions.send_request_invalid = mdt_send_request_invalid;
	state_actions.send_404_response = mdt_send_404_response;
	state_actions.cleanup = mdt_cleanup;
}