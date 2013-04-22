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

%%{
    machine StateActions;

    alphtype int;

    access conn->;

    action parse_init { CALL(parse_init); }
    action parse_exec { CALL(parse_exec); }
    action read_request_method { CALL(read_request_method); }
    action validate_get { CALL(validate_get); }
    action send_get_response { CALL(send_get_response); }
    action send_request_invalid { CALL(send_request_invalid); }
    action send_404_response { CALL(send_404_response); }
    action cleanup { CALL(cleanup); }

    include ConnectionState "connection_state.rl";
}%%

%% write data;

conn_data* conn;
request* req;
response* res;
http_parser* parser;
struct ev_loop *loop;               // local thread loop (for timeouts and such)

void md_worker(thread_info *opts) {
    req = malloc(sizeof(request));

    res = malloc(sizeof(response));

    parser = malloc(sizeof(http_parser));
    http_parser_init(parser);
    parser->data = (void *) req;

    loop = ev_loop_new(EVFLAG_AUTO);

    for(;;) {
        #ifdef DEBUG
        md_log(LOGDEBUG, "awaiting new connection");
        #endif

        /*  wait for the sem_q_full semaphore; posting means a conn_data has been queued
            proceed to lock queue, pull conn_data off, unlock, and post to sem_q_empty */
        sem_wait(queue_info.sem_q_full);
        pthread_mutex_lock(queue_info.mtx_conn_queue);
        conn = STAILQ_FIRST(&conn_q_head);
        STAILQ_REMOVE_HEAD(&conn_q_head, conn_q_next);
        pthread_mutex_unlock(queue_info.mtx_conn_queue);
        sem_post(queue_info.sem_q_empty);

        #ifdef DEBUG
        md_log(LOGDEBUG, "pulled new conn_data to client %s", inet_ntoa(conn->conn_info.sin_addr));
        #endif

        int next = OPEN;

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
    int event_queue[2] = {0};

    event_queue[0] = event;
    int next = 0;

    const int *p = event_queue;
    const int *pe = p+1;

    %% write exec;

    return next;
}

int md_state_init(conn_data *conn, request *req, response *res, http_parser *parser) {

    %% write init;

    return 1;
}

int md_parse_init(conn_data *conn, request *req, response *res, http_parser *parser) {
    md_res_init(res);
    md_req_init(req);
    http_parser_reset(parser);

    return PARSE;
}

int md_parse_exec(conn_data *conn, request *req, response *res, http_parser *parser) {
    md_req_read(req, conn);
    /* TODO replace assert here with handler in case request is larger than REQSIZE */
    assert(req->buffer_index < REQSIZE && "request size too large. refactor into HTTP Error 413");
    http_parser_execute(parser, req->buffer, req->buffer_index, 0);

    if(http_parser_has_error(parser)) {
        return PARSE_ERROR;
    } else if(http_parser_is_finished(parser)) {
        return PARSE_DONE;
    } else {
        return PARSE;
    }
}

int md_read_request_method(conn_data *conn, request *req, response *res, http_parser *parser) {
    #ifdef DEBUG
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

    time_t ticks = time(NULL);

    res->http_version = HTTP11;
    res->current_time = ctime(&ticks);
    res->servername = SERVERNAME;
    res->connection = CONN_CLOSE;

    if(strcmp(req->request_method, GET) == 0) {
        return GET_REQUEST;
    } else {
        return INV_REQUEST;
    }
}

int md_send_request_invalid(conn_data *conn, request *req, response *res, http_parser *parser) {
    md_log(LOGDEBUG, "500 Internal Server Error");

    time_t ticks = time(NULL);

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
    if(req->request_path[strlen(req->request_path) - 1] == '/') {
        char *s = malloc(sizeof(char) * (strlen(req->request_path) + strlen(DEFAULT_FILE)));
        s[0] = '\0';
        strcpy(s, req->request_path);
        strcpy(&s[strlen(s)], DEFAULT_FILE);
        free(req->request_path);
        req->request_path = s;
    }

    time_t ticks = time(NULL);

    if(req->request_path == NULL || req->request_uri == NULL) {
        return GET_NOT_VALID;
    } else if (stat(req->request_path, NULL) < 0 && errno == ENOENT) {
        return GET_NOT_FOUND;
    }

    return GET_VALID;
}

int md_send_get_response(conn_data *conn, request *req, response *res, http_parser *parser) {
    md_log(LOGDEBUG, "200 OK");

    int file_fd;
    copyfile_state_t copyfile_state;

    res->status = OK_S;
    res->content_type = MIME_HTML;
    res->charset = CHARSET;
    res->expires = EXPIRESNEVER;

    md_res_write(conn, res);

    if( (file_fd = open(req->request_path, O_RDONLY)) < 0 ) {
       md_fatal("error opening file: %s", req->request_path);
    }

    copyfile_state = copyfile_state_alloc();

    if( fcopyfile(file_fd, conn->open_sd, copyfile_state, COPYFILE_DATA) < 0 ) {
        md_log(LOGDEBUG, "copy file failed");
    }

    copyfile_state_free(copyfile_state);

    close(file_fd);

    return CLOSE;
}

int md_send_404_response(conn_data *conn, request *req, response *res, http_parser *parser) {
    md_log(LOGDEBUG, "404 Not Found");

    res->status = NF_S;
    res->content_type = MIME_HTML;
    res->charset = CHARSET;
    res->expires = EXPIRESNEVER;
    res->content = "<html>                                                              \
                        <body>                                                          \
                            <p style=\"font-weight: bold; font-size: 14px; text-align: center;\">           \
                            404 File Not Found                                          \
                            </p>                                                        \
                        </body>                                                         \
                    </html>%s";

    return CLOSE;
}

int md_cleanup(conn_data *conn, request *req, response *res, http_parser *parser) {
    close(conn->open_sd);
    free(conn);

    md_req_destroy(req);

    return DONE;
}