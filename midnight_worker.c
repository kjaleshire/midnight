/*

Midnight worker thread

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

void worker_thread() {
    char readbuff[BUFFSIZE];
    conn_data_t *conn = NULL;

    pthread_cleanup_push( (void (*) (void *)) worker_cleanup, conn);

    for(;;) {
        sem_wait(&sem_q_full);
        pthread_mutex_lock(&mtx_conn_queue);
        conn = STAILQ_FIRST(&conn_q_head);
        STAILQ_REMOVE_HEAD(&conn_q_head, conn_q_next);
        pthread_mutex_unlock(&mtx_conn_queue);
        sem_post(&sem_q_empty);
        logmsg(LOGINFO, "pulled new connectino off queue");
        logmsg(LOGINFO, "address: %s", inet_ntoa(conn->conn_info.sin_addr));
    }

    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}



void worker_cleanup(conn_data_t *conn) {
    if(conn == NULL) {
        return;
    }

    if(close(conn->open_sd) < 0) {
        panic(ERRSYS, "connection socket close error");
    } else {
        logmsg(LOGINFO, "connection socket closed");
    }

    free(conn);
}