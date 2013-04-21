/*

Midnight main program file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

#define PORT 8080
#define ADDRESS "127.0.0.1"

int listen_sd;
thread_opts threads[N_THREADS];
struct conn_q_head_struct conn_q_head = STAILQ_HEAD_INITIALIZER(conn_q_head);

int main(int argc, char *argv[]){
	int v;
	int listen_address;
	struct sockaddr_in servaddr;

	struct ev_loop* default_loop;
	ev_io watcher_accept;
	ev_signal watcher_sigint;

	log_level = LOGDEBUG;
	log_fd = stdout;

	md_log_init();

	md_set_state_actions(&conn_actions);

	/* inet_pton(AF_INET, ADDRESS, &listen_address); */
	listen_address = htonl(INADDR_ANY);

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(listen_address);
	servaddr.sin_port = htons(PORT);

	v = 1;
	if( (listen_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ||
	#ifdef DEBUG
		setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0 ||
	#endif
		bind(listen_sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ||
		listen(listen_sd, LISTENQ) < 0
	) {
		md_fatal("error creating & binding socket to %s",
			servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
	} else {
		#ifdef DEBUG
		md_log(LOGDEBUG, "socket bound to %s",
			servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
		#endif
	}

	if( ( (sem_q_empty = sem_open("conn_empty", O_CREAT, 0644, MAXQUEUESIZE)) == SEM_FAILED) ||
		( (sem_q_full = sem_open("conn_full", O_CREAT, 0644, 0))
		 == SEM_FAILED) ||
		(pthread_mutex_init(&mtx_conn_queue, NULL) != 0 ) ) {
			md_fatal("error creating conn_data queue locking mechanisms");
	}

	for(v = 0; v < N_THREADS; v++) {
		char c[8];
		snprintf(c, sizeof(c), "tID %d", v);
		if( (threads[v].quit = sem_open(c, O_CREAT, 0644, 0)) == SEM_FAILED ||
			pthread_create(&(threads[v].thread_id), NULL, (void *(*)(void *)) md_worker, &threads[v]) < 0) {
			md_fatal("thread pool spawn failure");
		}
	}

	STAILQ_INIT(&conn_q_head);

	default_loop = EV_DEFAULT;

	ev_signal_init(&watcher_sigint, md_sigint_cb, SIGINT);
	ev_signal_start(default_loop, &watcher_sigint);

	ev_io_init(&watcher_accept, md_accept_cb, listen_sd, EV_READ);
	ev_io_start(default_loop, &watcher_accept);

	#ifdef DEBUG
	md_log(LOGDEBUG, "entering default event loop");
	#endif

	for(;;) {
		ev_run(default_loop, 0);
	}
}

void md_accept_cb(struct ev_loop *loop, ev_io* watcher_accept, int revents) {
	socklen_t sock_size = sizeof(struct sockaddr_in);
	conn_data *new_conn = malloc(sizeof(conn_data));

	if( (new_conn->open_sd = accept(listen_sd, (struct sockaddr *) &(new_conn->conn_info), &sock_size)) < 0) {
		md_fatal("connection accept failure from client %s", inet_ntoa(new_conn->conn_info.sin_addr));
	} else {
		#ifdef DEBUG
		md_log(LOGDEBUG, "accepted connection from client %s", inet_ntoa(new_conn->conn_info.sin_addr));
		#endif
	}

	sem_wait(sem_q_empty);
	pthread_mutex_lock(&mtx_conn_queue);
	STAILQ_INSERT_TAIL(&conn_q_head, new_conn, conn_q_next);
	pthread_mutex_unlock(&mtx_conn_queue);
	sem_post(sem_q_full);
	#ifdef DEBUG
	md_log(LOGDEBUG, "queued new conn_data");
	#endif
}

void md_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents) {
	int i;
	md_log(LOGINFO, "Shutting down!");
	/*for(i = 0; i < N_THREADS; i++) {
		sem_post(threads[i].quit);
	}
	for(i = 0; i < N_THREADS; i++) {
		pthread_join(threads[i].thread_id, NULL);
	} */
	exit(0);
}