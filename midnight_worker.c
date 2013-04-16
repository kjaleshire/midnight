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
    int nread;
    http_parser parser;
    http_parser_alloc(&parser);

    for(;;) {
        #ifdef DEBUG
        md_log(LOGDEBUG, "awaiting new connection");
        #endif

        res.buffer_index = 0;
        req.buffer_index = 0;
        nread = 0;
        http_parser_init(&parser);

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

        /* parse the request with Shaw's parser */
        do {
            md_req_read(&res, conn);
            /* TODO replace assert here with handler in case request is larger than REQSIZE */
            assert( req.buffer_index <= REQSIZE - 1);
            nread += http_parser_execute(&parser, req.buffer, req.buffer_index, nread);
        } while(!http_parser_is_finished(&parser));

        assert(!http_parser_has_error(&parser));

        #ifdef DEBUG
        md_log(LOGDEBUG, "nread = %d, req.buffer_index = %d", nread, req.buffer_index);
        md_log(LOGDEBUG, "connection read %d bytes as request:\n%s", req.buffer_index, req.buffer);
        #endif

        ticks = time(NULL);

        md_res_buff(&res, "%s %s%s", HTTP11_R, OK_S, CRLF);
        md_res_buff(&res, "%s %s %s%s", CONTENT_H, MIME_HTML, CHARSET, CRLF);
        md_res_buff(&res, "%s %.24s%s", DATE_H, ctime(&ticks), CRLF);
        md_res_buff(&res, "%s %s%s", EXPIRES_H, EXPIRESNEVER, CRLF);
        md_res_buff(&res, "%s %s%s", SERVER_H, SERVERNAME, CRLF);
        md_res_buff(&res, "%s %s%s", CONN_H, CLOSE, CRLF);
        md_res_buff(&res, "%s", CRLF);
        md_res_buff(&res, "<html><body><p style=\"font-weight: bold\">Hello!</p></body></html>%s", CRLF);

        md_res_write(conn, &res);

        close(conn->open_sd);
        free(conn);
    }
}