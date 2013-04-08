/*

Midnight main program file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

#define PORT 80
#define ADDRESS "127.0.0.1"

int main(int argc, char *argv[]){
	int v;
	int listen_address;
	struct sockaddr_in servaddr;
	pthread_t threads[N_THREADS];

	struct ev_loop* default_loop;
	ev_io w_listen;

	log_level = LOGDEBUG;
	log_fd = stdout;

	/* initialize the terminal output mutex before we start logging stuff */
	log_init();

	/* inet_pton(AF_INET, ADDRESS, &listen_address); */
	listen_address = htonl(INADDR_ANY);

	/* set the server listening parameters: IPv4, IP address, port number */
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(listen_address);
	servaddr.sin_port = htons(PORT);

	/* set up a new socket for us to (eventually) listen us */
	v = 1;
	if( (listen_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ||
	#ifdef DEBUG
		setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0 ||
	#endif
		bind(listen_sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ||
		listen(listen_sd, LISTENQ) < 0
	) {
		panic(ERRSYS, "error creating & binding socket to %s",
			servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
	} else {
		logmsg(LOGINFO, "socket bound to %s",
			servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
	}

	if( (sem_q_empty = sem_open("q_empty", O_CREAT, 0, MAXQUEUESIZE) == SEM_FAILED) ||
		(sem_q_full = sem_open("q_full", O_CREAT, 0, 0) == SEM_FAILED) ||
		pthread_mutex_init(&mtx_conn_queue, NULL) != 0 ) {
			panic(ERRSYS, "error creating queue locking mechanisms");
	}

/*
	for(v = 0; v < N_THREADS; v++) {

		if( (pthread_create(&threads[v], NULL, (void *(*)(void *)) worker_thread, NULL)) < 0) {
			panic(ERRSYS, "thread pool spawn failure");
		}
	}
*/
	default_loop = EV_DEFAULT;

	ev_io_init(&w_listen, accept_ready_cb, listen_sd, EV_READ);
	ev_io_start(default_loop, &w_listen);

	/* enter our runloop */
	for(;;) {
		ev_run(default_loop, 0);
	}
}

static void accept_ready_cb(struct ev_loop *loop, ev_io* w_listen, int revents) {
	socklen_t sock_size = sizeof(struct sockaddr_in);
	conn_data_t *new_conn = malloc(sizeof(conn_data_t));

	if( (new_conn->open_sd = accept(listen_sd, (struct sockaddr *) &(new_conn->conn_info), &sock_size)) < 0) {
		panic(ERRSYS, "connection accept failure from client %s", inet_ntoa(new_conn->conn_info.sin_addr));
	} else {
		logmsg(LOGINFO, "accepted connection from client %s", inet_ntoa(new_conn->conn_info.sin_addr));
	}

	sem_wait(&sem_q_empty);
	pthread_mutex_lock(&mtx_conn_queue);
	STAILQ_INSERT_TAIL(&q_conn_head, new_conn, q_entries);
	pthread_mutex_unlock(&mtx_conn_queue);
	sem_post(&sem_q_full);
}