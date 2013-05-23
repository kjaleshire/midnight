/*

Midnight worker thread logic

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"
#include "events.h"

#ifdef DEBUG
#define CALL(A) md_log(LOGDEBUG, "event trigger: %d", event); next = state_actions.A(state)
#else
#define CALL(A) next = state_actions.A(state)
#endif

%%{
    machine StateActions;

    import "events.h";

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

void md_worker(thread_info *opts) {
    int next;
    conn_state *state;

    state = malloc(sizeof(conn_state));

    while(!opts->thread_continue) {
        #ifdef DEBUG
        md_log(LOGDEBUG, "awaiting new connection");
        #endif

        TRACE();
        /*  wait for the sem_q_full semaphore; posting means a conn_data has been queued
            proceed to lock queue, pull conn_data off, unlock, and post to sem_q_empty */
        sem_wait(queue_info.sem_q_full);
        pthread_mutex_lock(&queue_info.mtx_conn_queue);
        state->conn = STAILQ_FIRST(&queue_info.conn_queue);
        STAILQ_REMOVE_HEAD(&queue_info.conn_queue, q_next);
        pthread_mutex_unlock(&queue_info.mtx_conn_queue);
        sem_post(queue_info.sem_q_empty);

        if(state->conn->open_sd == -1) {
            break;
        }

        #ifdef DEBUG
        md_log(LOGDEBUG, "dequeued client %s", inet_ntoa(state->conn->conn_info.sin_addr));
        #endif

        next = OPEN;

        md_state_init(state);

        for(;;) {
            if(next == DONE) {
                break;
            }
            next = md_state_event(state, next);
        }

        #ifdef DEBUG
        md_log(LOGDEBUG, "finished handling connection");
        #endif
    }

    #ifdef DEBUG
    md_log(LOGDEBUG, "thread quitting!");
    #endif
    next = 0;
    pthread_exit(&next);
}

int md_state_event(conn_state* state, int event) {
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

int md_state_init(conn_state* state) {
    TRACE();

    %% write init;

    return 1;
}

int md_parse_init(conn_state* state) {
    TRACE();

    state->parser = malloc(sizeof(http_parser));
    http_parser_init(state->parser);

    state->parser->data = malloc(sizeof(request));
    md_req_init((request *) state->parser->data);

    return PARSE;
}

int md_parse_exec(conn_state* state) {
    request* req = (request *) state->parser->data;

    TRACE();
    md_req_read(state->conn, req);
    if(req->buffer_index == 0) {
        return CLOSE;
    }

    /* TODO replace assert here with handler in case request is larger than REQSIZE */
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

int md_read_request_method(conn_state* state) {
    request* req = (request *) state->parser->data;
    response* res;

    if(state->res == NULL) {
        state->res = malloc(sizeof(response));
    }

    res = state->res;

    md_res_init(res);

    time_t ticks = time(NULL);

    res->http_version = HTTP11;
    res->current_time = ctime(&ticks);
    res->servername = SERVER_NAME;
    res->connection = CONN_CLOSE;

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
    state->res = res;
    if(strcmp(req->request_method, GET) == 0) {
        return GET_REQUEST;
    } else {
        return INV_REQUEST;
    }
}

int md_send_request_invalid(conn_state* state) {
    request* req = (request*) state->parser->data;
    response* res;
    time_t ticks;

    if(state->res == NULL) {
        state->res = malloc(sizeof(response));

        state->res->http_version = HTTP11;
        state->res->current_time = ctime(&ticks);
        state->res->servername = APP_NAME;
        state->res->connection = CONN_CLOSE;
    }

    res = state->res;

    TRACE();
    md_log(LOGINFO, "500 Internal Server Error");

    res->status = SRVERR_S;
    res->content_type = MIME_HTML;
    res->charset = CHARSET;
    res->content =  "<html>\n                                                             \
                        <body>\n                                                          \
                            <p style=\"font-weight: bold; font-size: 14px; text-align: center;\">\n   \
                            500 Internal Server Error\n                                   \
                            </p>\n                                                        \
                        </body>\n                                                         \
                    </html>%s";

    md_res_write(state->conn, res);

    return CLOSE;
}

int md_validate_get(conn_state* state) {
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

    sprintf(s, "%s%s%s", options_info.docroot, req->request_path, f);

    assert(strlen(s) == n);
    md_log(LOGDEBUG, "new request_path: %s", s);

    free(req->request_path);
    req->request_path = s;
    md_log(LOGDEBUG, "requested file: %s", req->request_path);

    if(req->request_path == NULL || req->request_uri == NULL) {
        return GET_NOT_VALID;
    } else if (stat(req->request_path, NULL) < 0 && errno == ENOENT) {
        *(req->request_path + strlen(req->request_path) - 2) = '\0';
        if (stat(req->request_path, NULL) < 0 && errno == ENOENT) {
            return GET_NOT_FOUND;
        }
    }

    return GET_VALID;
}

int md_send_get_response(conn_state* state) {
    response* res = state->res;
    request* req = (request *) state->parser->data;
    off_t file_len;

    TRACE();
    #ifdef DEBUG
    md_log(LOGDEBUG, "200 OK");
    #endif

    int index = 0;
    int v = 0;
    char buffer[READBUFF];
    int file_fd;
    struct stat filestat;

    assert( stat(req->request_path, &filestat) >=0 );

    snprintf(res->content_length, 16,"%lld", filestat.st_size);
    res->status = OK_S;
    res->content_type = md_detect_type(req->request_path);
    res->charset = CHARSET;
    res->expires = EXPIRES_NEVER;

    md_res_write(state->conn, res);

    if( (file_fd = open(req->request_path, O_RDONLY)) < 0 ) {
        char *s = strerror(errno);
        md_fatal("error opening file: %s", s);
    }

    file_len = 0;
    if( sendfile(file_fd, state->conn->open_sd, 0, &file_len, NULL, 0) < 0 ) {
        char *s = strerror(errno);
        md_fatal("error writing file: %s", s);
    }

    TRACE();
    close(file_fd);

    return CLOSE;
}

int md_send_404_response(conn_state* state) {
    response* res = state->res;
    TRACE();
    md_log(LOGDEBUG, "404 Not Found");

    res->status = NF_S;
    res->content_type = MIME_HTML;
    res->charset = CHARSET;
    res->expires = EXPIRES_NEVER;
    res->content = "<html>\n                                                            \
                        <body>\n                                                        \
                            <p style=\"font-weight: bold; font-size: 18px; text-align: center;\">\n         \
                            404 File Not Found\n                                        \
                            </p>\n                                                      \
                        </body>\n                                                       \
                    </html>%s";

    md_res_write(state->conn, res);

    return CLOSE;
}

int md_cleanup(conn_state* state) {
    close(state->conn->open_sd);
    TRACE();

    if(state->old_conn != NULL) {
        free(state->old_conn);
    }
    state->old_conn = state->conn;

    free(state->res); state->res = NULL;
    md_req_destroy((request *) state->parser->data);
    free(state->parser->data);
    free(state->parser);

    return DONE;
}

char* md_detect_type(char* filename) {
    char *c;
    if( (c = strrchr(filename, '.')) == NULL || strcmp(c, ".txt") == 0 ) {
        c = MIME_TXT;
    } else if( strcmp(c, ".html") == 0 || strcmp(c, ".htm") == 0) {
        c = MIME_HTML;
    } else if( strcmp(c, ".jpeg") == 0 || strcmp(c, ".jpg") == 0) {
        c = MIME_JPG;
    } else if( strcmp(c, ".gif") == 0 ) {
        c = MIME_GIF;
    } else if( strcmp(c, ".png") == 0 ) {
        c = MIME_PNG;
    } else if( strcmp(c, ".css") == 0 ) {
        c = MIME_CSS;
    } else if( strcmp(c, ".js") == 0 ) {
        c = MIME_JS;
    } else {
        c = MIME_TXT;
    }
    md_log(LOGDEBUG, "detected MIME type: %s", c);
    return c;
}