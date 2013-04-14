/*

Midnight worker thread logic

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

void worker_thread(thread_args_t *t_args) {
    char buff[BUFFSIZE];
    int buff_dex;
    conn_data_t *conn;

    time_t ticks;

    for(;;) {
        #ifdef DEBUG
        logmsg(LOGDEBUG, "waiting for new connection");
        #endif
        buff_dex = 0;

        sem_wait(sem_q_full);
        // acquiring the mutex means there's a connection queued and waiting to be opened
        pthread_mutex_lock(&mtx_conn_queue);
        conn = STAILQ_FIRST(&conn_q_head);
        STAILQ_REMOVE_HEAD(&conn_q_head, conn_q_next);
        pthread_mutex_unlock(&mtx_conn_queue);
        // pull the connection info off and release the mutex, don't waste time
        sem_post(sem_q_empty);

        logmsg(LOGDEBUG, "pulled new connection to client %s", inet_ntoa(conn->conn_info.sin_addr));

        ticks = time(NULL);



        /* handle connection here: parse request & send response */
        buff_dex += snprintf(&(buff[buff_dex]), BUFFSIZE - buff_dex, "%s %s%s", HTTP10_R, OK_R, CRLF);
        buff_dex += snprintf(&(buff[buff_dex]), BUFFSIZE - buff_dex, "%s%s%s%s", CONTENT_T_H, MIME_HTML, CHARSET, CRLF);
        buff_dex += snprintf(&(buff[buff_dex]), BUFFSIZE - buff_dex, "%s%.24s%s", DATE_H, ctime(&ticks), CRLF);
        buff_dex += snprintf(&(buff[buff_dex]), BUFFSIZE - buff_dex, "%s%s%s%s%s", EXPIRES_H, CRLF, SERVER_H, CRLF, CRLF);
        buff_dex += snprintf(&(buff[buff_dex]), BUFFSIZE - buff_dex, "<html><body><p>Hello!</p></body></html>%s", CRLF);

        write(conn->open_sd, buff, buff_dex);

        close(conn->open_sd);
        free(conn);
    }
}