/*

Midnight worker thread logic

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

void md_worker(thread_info_t *t_info) {
    conn_data_t *conn;
    time_t ticks;
    http_parser_t parser;
    http_parser_alloc(&parser);

    for(;;) {
        #ifdef DEBUG
        MD_LOG(LOGDEBUG, "waiting for new connection");
        #endif
        t_info->buff_dex = 0;
        http_parser_init(&parser);


        sem_wait(sem_q_full);
        // acquiring the mutex means there's a connection queued and waiting to be opened
        pthread_mutex_lock(&mtx_conn_queue);
        conn = STAILQ_FIRST(&conn_q_head);
        STAILQ_REMOVE_HEAD(&conn_q_head, conn_q_next);
        pthread_mutex_unlock(&mtx_conn_queue);
        // pull the connection info off and release the mutex, don't waste time
        sem_post(sem_q_empty);

        MD_LOG(LOGDEBUG, "pulled new connection to client %s", inet_ntoa(conn->conn_info.sin_addr));

        ticks = time(NULL);

        /* handle connection here: parse request & send response */


        assert(http_parser_finish);

        MD_BUFF(t_info, "%s %s%s", HTTP11_R, OK_S, CRLF);
        MD_BUFF(t_info, "%s%s%s%s", CONTENT_T_H, MIME_HTML, CHARSET, CRLF);
        MD_BUFF(t_info, "%s%.24s%s", DATE_H, ctime(&ticks), CRLF);
        MD_BUFF(t_info, "%s%s%s%s%s", EXPIRES_H, CRLF, SERVER_H, CRLF, CRLF);
        MD_BUFF(t_info, "<html><body><p style=\"font-weight: bold\">Hello!</p></body></html>%s", CRLF);

        write(conn->open_sd, t_info->buff, t_info->buff_dex);

        close(conn->open_sd);
        free(conn);
    }
}