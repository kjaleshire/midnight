/*

Midnight queue header file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef __midnight_queue_h
#define __midnight_queue_h

#include <sys/queue.h>

typedef struct conn_data {
    int open_sd;
    struct sockaddr_in conn_info;
    STAILQ_ENTRY(conn_data) conn_q_next;
} conn_data_t;

STAILQ_HEAD(, conn_data) conn_q_head = STAILQ_HEAD_INITIALIZER(conn_q_head);

#endif /* __midnight_queue_h */