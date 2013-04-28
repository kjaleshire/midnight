
#line 1 "src/worker.rl"
/*

Midnight worker thread logic

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"
#include "events.h"

#ifdef DEBUG
#define CALL(A) md_log(LOGDEBUG, "event: %d", event); next = state_actions.A(conn, req, res, parser)
#else
#define CALL(A) next = state_actions.A(conn, req, res, parser)
#endif


#line 36 "src/worker.rl"



#line 27 "src/worker.c"
static const int StateActions_start = 5;
static const int StateActions_first_final = 5;
static const int StateActions_error = 0;

static const int StateActions_en_main = 5;


#line 39 "src/worker.rl"

void md_worker(thread_info *opts) {
    int next;
    conn_data* conn;
    request* req;
    response* res;
    http_parser* parser;

    req = malloc(sizeof(request));

    res = malloc(sizeof(response));

    parser = malloc(sizeof(http_parser));
    http_parser_init(parser);
    parser->data = (void *) req;

    for(;;) {
        #ifdef DEBUG
        md_log(LOGDEBUG, "awaiting new connection");
        #endif

        TRACE();
        /*  wait for the sem_q_full semaphore; posting means a conn_data has been queued
            proceed to lock queue, pull conn_data off, unlock, and post to sem_q_empty */
        assert(sem_wait(sem_q_full) == 0);
        pthread_mutex_lock(&mtx_conn_queue);
        assert(STAILQ_FIRST(&conn_q_head) != NULL);
        conn = STAILQ_FIRST(&conn_q_head);
        STAILQ_REMOVE_HEAD(&conn_q_head, conn_q_next);
        pthread_mutex_unlock(&mtx_conn_queue);
        assert(sem_post(sem_q_empty) == 0);

        #ifdef DEBUG
        md_log(LOGDEBUG, "dequeued client %s, descriptor %d", inet_ntoa(conn->conn_info.sin_addr), conn->open_sd);
        #endif

        next = OPEN;

        md_state_init(conn, req, res, parser);

        for(;;) {
            if(next == DONE) break;
            next = md_state_change(conn, req, res, parser, next);
        }

        md_log(LOGDEBUG, "finished handling connection");

        /*
        if(sem_trywait(opts->quit) != EAGAIN) {
            #ifdef DEBUG
            md_log(LOGDEBUG, "thread quitting!");
            #endif
            int i = 0;
            pthread_exit(&i);
        }*/
    }
}

int md_state_change(conn_data *conn, request *req, response *res, http_parser *parser, int event) {
    TRACE();
    assert(event >= 10 && event <= 20 && "event out of range");

    int event_queue[2] = {0};

    event_queue[0] = event;
    int next = 0;

    const int *p = event_queue;
    const int *pe = p+1;

    
#line 107 "src/worker.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch (  conn->cs )
	{
tr4:
#line 33 "src/worker.rl"
	{ CALL(cleanup); }
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 121 "src/worker.c"
	if ( (*p) == 10 )
		goto tr8;
	goto st0;
st0:
 conn->cs = 0;
	goto _out;
tr0:
#line 27 "src/worker.rl"
	{ CALL(parse_exec); }
	goto st1;
tr8:
#line 26 "src/worker.rl"
	{ CALL(parse_init); }
	goto st1;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 140 "src/worker.c"
	switch( (*p) ) {
		case 11: goto tr0;
		case 12: goto tr2;
		case 13: goto tr3;
		case 19: goto tr4;
	}
	goto st0;
tr2:
#line 28 "src/worker.rl"
	{ CALL(read_request_method); }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 156 "src/worker.c"
	switch( (*p) ) {
		case 14: goto tr5;
		case 15: goto tr3;
	}
	goto st0;
tr5:
#line 29 "src/worker.rl"
	{ CALL(validate_get); }
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 170 "src/worker.c"
	switch( (*p) ) {
		case 16: goto tr6;
		case 17: goto tr3;
		case 18: goto tr7;
	}
	goto st0;
tr3:
#line 31 "src/worker.rl"
	{ CALL(send_request_invalid); }
	goto st4;
tr6:
#line 30 "src/worker.rl"
	{ CALL(send_get_response); }
	goto st4;
tr7:
#line 32 "src/worker.rl"
	{ CALL(send_404_response); }
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 193 "src/worker.c"
	if ( (*p) == 19 )
		goto tr4;
	goto st0;
	}
	_test_eof5:  conn->cs = 5; goto _test_eof; 
	_test_eof1:  conn->cs = 1; goto _test_eof; 
	_test_eof2:  conn->cs = 2; goto _test_eof; 
	_test_eof3:  conn->cs = 3; goto _test_eof; 
	_test_eof4:  conn->cs = 4; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 110 "src/worker.rl"

    return next;
}

int md_state_init(conn_data *conn, request *req, response *res, http_parser *parser) {
    TRACE();

    
#line 217 "src/worker.c"
	{
	 conn->cs = StateActions_start;
	}

#line 118 "src/worker.rl"

    return 1;
}

int md_parse_init(conn_data *conn, request *req, response *res, http_parser *parser) {
    TRACE();
    md_res_init(res);
    md_req_init(req);
    http_parser_reset(parser);

    return PARSE;
}

int md_parse_exec(conn_data *conn, request *req, response *res, http_parser *parser) {
    TRACE();
    md_req_read(req, conn);
    /*md_log(LOGDEBUG, "%s", req->buffer);*/
    if(req->buffer_index == 0) {
        return CLOSE;
    }

    /* TODO replace assert here with handler in case request is larger than REQSIZE */
    assert(req->buffer_index < REQSIZE && "request size too large. refactor into HTTP Error 413");
    http_parser_execute(parser, req->buffer, req->buffer_index, 0);
    TRACE();
    if(http_parser_has_error(parser)) {
        return PARSE_ERROR;
    } else if(http_parser_is_finished(parser)) {
        return PARSE_DONE;
    } else {
        return PARSE;
    }
}

int md_read_request_method(conn_data *conn, request *req, response *res, http_parser *parser) {
    TRACE();
    /*
    #ifdef DEBUG
    md_log(LOGDEBUG, "Request method: %s", req->request_method);
    md_log(LOGDEBUG, "Request URI: %s", req->request_uri);
    md_log(LOGDEBUG, "Fragment: %s", req->fragment);
    md_log(LOGDEBUG, "Request path: %s", req->request_path);
    md_log(LOGDEBUG, "Query string: %s", req->query_string);
    md_log(LOGDEBUG, "HTTP version: %s", req->http_version);
    http_header *s, *tmp;
    HASH_ITER(hh, req->table, s, tmp) {
        md_log(LOGDEBUG, "%s: %s", s->key, s->value);
    }
    #endif
    */

    time_t ticks = time(NULL);

    res->http_version = HTTP11;
    res->current_time = ctime(&ticks);
    res->servername = SERVER_NAME;
    res->connection = CONN_CLOSE;
    TRACE();
    if(strcmp(req->request_method, GET) == 0) {
        return GET_REQUEST;
    } else {
        return INV_REQUEST;
    }
}

int md_send_request_invalid(conn_data *conn, request *req, response *res, http_parser *parser) {
    TRACE();
    md_log(LOGDEBUG, "500 Internal Server Error");

    res->status = SRVERR_S;
    res->content_type = MIME_HTML;
    res->charset = CHARSET;
    res->content =  "<html>                                                              \
                        <body>                                                          \
                            <p style=\"font-weight: bold; font-size: 14px; text-align: center;\">           \
                            500 Internal Server Error                                   \
                            </p>                                                        \
                        </body>                                                         \
                    </html>%s";

    md_res_write(conn, res);

    return CLOSE;
}

int md_validate_get(conn_data *conn, request *req, response *res, http_parser *parser) {
    TRACE();
    /*
    if(req->request_path[strlen(req->request_path) - 1] == '/') {
        char *s = calloc(strlen(req->request_path) + strlen(DEFAULT_FILE), sizeof(char));
        s[0] = '\0';
        strcpy(s, req->request_path);
        strcpy(&s[strlen(s)], DEFAULT_FILE);
        free(req->request_path);
        req->request_path = s;
    }
*/
    if(req->request_path[strlen(req->request_path) - 1] == '/') {
        int n = strlen(req->request_path) + strlen(DEFAULT_FILE);
        char *s = calloc(n + 1, sizeof(char));
        sprintf(s, "%s%s", req->request_path, DEFAULT_FILE);
        md_log(LOGDEBUG, "new request_path: %s", s);
        free(req->request_path);
        req->request_path = s;
        md_log(LOGDEBUG, "requested file: %s", req->request_path);
    }
    if(req->request_path == NULL || req->request_uri == NULL) {
        return GET_NOT_VALID;
    } else if (stat(&(req->request_path[1]), NULL) < 0 && errno == ENOENT) {
        req->request_path[strlen(req->request_path) - 2] = '\0';
        if (stat(&(req->request_path[1]), NULL) < 0 && errno == ENOENT) {
            return GET_NOT_FOUND;
        }
    }

    return GET_VALID;
}

int md_send_get_response(conn_data *conn, request *req, response *res, http_parser *parser) {
    TRACE();
    #ifdef DEBUG
    md_log(LOGDEBUG, "200 OK");
    #endif

    int index = 0;
    int v = 0;
    char buffer[READBUFF];
    int file_fd;

    res->status = OK_S;
    res->content_type = MIME_HTML;
    res->charset = CHARSET;
    res->expires = EXPIRES_NEVER;

    md_res_write(conn, res);

    if( (file_fd = open(&(req->request_path[1]), O_RDONLY)) < 0 ) {
       md_fatal("error opening file: %s", req->request_path);
    }

    while( (v = read(file_fd, &buffer[index], READBUFF - index)) != 0 ) {
        if(v < 0) md_fatal("read file failure, descriptor: %d", file_fd);
        index += v;
        assert(index <= READBUFF && "index is bigger than buffer size");
        if( index == READBUFF || (index > 0 && v == 0) ) {
            if( (v = write(conn->open_sd, buffer, READBUFF)) < 0 ) {
                if(errno == ECONNRESET) {
                    md_log(LOGINFO, "connection to %s closed", inet_ntoa(conn->conn_info.sin_addr));
                    break;
                } else {
                    md_fatal("connection write failure, client %s", inet_ntoa(conn->conn_info.sin_addr));
                }
            }
            index = 0;
        }
    }

    TRACE();
    close(file_fd);

    return CLOSE;
}

int md_send_404_response(conn_data *conn, request *req, response *res, http_parser *parser) {
    TRACE();
    md_log(LOGDEBUG, "404 Not Found");

    res->status = NF_S;
    res->content_type = MIME_HTML;
    res->charset = CHARSET;
    res->expires = EXPIRES_NEVER;
    res->content = "<html>                                                              \
                        <body>                                                          \
                            <p style=\"font-weight: bold; font-size: 14px; text-align: center;\">           \
                            404 File Not Found                                          \
                            </p>                                                        \
                        </body>                                                         \
                    </html>%s";

    md_res_write(conn, res);

    return CLOSE;
}

int md_cleanup(conn_data *conn, request *req, response *res, http_parser *parser) {
    close(conn->open_sd);
    TRACE();
    free(conn);

    md_req_destroy(req);

    return DONE;
}