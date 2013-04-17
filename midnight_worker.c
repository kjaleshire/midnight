/*

Midnight worker thread logic

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

void md_worker() {
    conn_data *conn;
    time_t ticks;
    response res;
    request req;
    req.table = NULL;
    int nread;
    http_parser parser;
    http_parser_init(&parser);
    parser.data = (void *) &req;
    struct ev_loop *loop;
    loop = ev_loop_new(EVFLAG_AUTO);

    for(;;) {
        #ifdef DEBUG
        md_log(LOGDEBUG, "awaiting new connection");
        #endif

        res.buffer_index = 0;
        req.buffer_index = 0;
        nread = 0;
        http_parser_reset(&parser);

        /*  wait for the sem_q_full semaphore; posting means a connection has been queued
            proceed to lock queue, pull connection off, unlock, and post to sem_q_empty */
        sem_wait(sem_q_full);
        pthread_mutex_lock(&mtx_conn_queue);
        conn = STAILQ_FIRST(&conn_q_head);
        STAILQ_REMOVE_HEAD(&conn_q_head, conn_q_next);
        pthread_mutex_unlock(&mtx_conn_queue);
        sem_post(sem_q_empty);

        #ifdef DEBUG
        md_log(LOGDEBUG, "pulled new connection to client %s", inet_ntoa(conn->conn_info.sin_addr));
        #endif

        /* validate & store the request with Shaw's parser */
        do {
            md_req_read(&req, conn);
            /* TODO replace assert here with handler in case request is larger than REQSIZE */
            assert(req.buffer_index < REQSIZE && "request size too large. refactor into HTTP Error 413");
            nread += http_parser_execute(&parser, req.buffer, req.buffer_index, nread);
        } while(!http_parser_is_finished(&parser));

        assert(!http_parser_has_error(&parser));

        #ifdef DEBUG
        md_log(LOGDEBUG, "request as parsed:");
        http_header *s, *tmp;
        HASH_ITER(hh, req.table, s, tmp) {
            md_log(LOGDEBUG, "%s: %s", s->key, s->value);
        }
        #endif

        ticks = time(NULL);

        res.http_version = HTTP11;
        res.status = OK_S;
        res.content_type = MIME_HTML;
        res.charset = CHARSET;
        res.current_time = ctime(&ticks);
        res.expires = EXPIRESNEVER;
        res.servername = SERVERNAME;
        res.connection = CLOSE;
        res.content =  "<html>                                                              \
                            <body>                                                          \
                                <p style=\"font-weight: bold; font-size: 64px;\">           \
                                Hello wurld!                                                \
                                </p>                                                        \
                            </body>                                                         \
                        </html>%s";

        md_res_buff(&res, HEADER_FMT, res.http_version, res.status, CRLF);
        md_res_buff(&res, CONTENT_FMT, CONTENT_H, res.content_type, res.charset, CRLF);
        md_res_buff(&res, DATE_FMT, DATE_H, res.current_time, CRLF);
        md_res_buff(&res, HEADER_FMT, EXPIRES_H, res.expires, CRLF);
        md_res_buff(&res, HEADER_FMT, SERVER_H, res.servername, CRLF);
        md_res_buff(&res, HEADER_FMT, CONN_H, res.connection, CRLF);
        md_res_buff(&res, CRLF);
        md_res_buff(&res, res.content, CRLF);

        md_res_write(conn, &res);

        close(conn->open_sd);
        free(conn);

        md_req_destroy(&req);
    }
}