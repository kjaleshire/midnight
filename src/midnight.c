/*

Midnight main program file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#define PORT 8080
#define ADDRESS "127.0.0.1"

#include "midnight.h"

int listen_sd;
thread_info threads[N_THREADS];

int main(int argc, char *argv[]){
	int v, listen_address;
	struct sockaddr_in servaddr;

	struct ev_loop* default_loop;
	ev_io* watcher_accept = malloc(sizeof(ev_io));
	ev_signal* watcher_sigint = malloc(sizeof(ev_signal));

	#ifdef DEBUG
	log_info.log_level = LOGDEBUG;
	#else
	log_info.log_level = LOGINFO;
	#endif

	md_set_state_actions(&state_actions);

	/* inet_pton(AF_INET, ADDRESS, &listen_address); */
	listen_address = htonl(INADDR_ANY);

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(listen_address);
	servaddr.sin_port = htons(PORT);

	v = 1;
	if( (listen_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ||
	#ifdef DEBUG
		setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0 ||	// so we can quickly reuse port
	#endif
		bind(listen_sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ||
		listen(listen_sd, LISTENQ) < 0
	) {
		md_fatal("socket create & bind fail on %s", "INADDR_ANY");
	}
	else {
		TRACE();
	}

	if( (queue_info.sem_q_empty = sem_open("empty", O_CREAT, 0644, MAXQUEUESIZE)) == SEM_FAILED ||
		(queue_info.sem_q_full = sem_open("full", O_CREAT, 0644, 0)) == SEM_FAILED ||
		(sem_unlink("empty") != 0 && errno != ENOENT) ||
		(sem_unlink("full") != 0 && errno != ENOENT) )
		 {
		 	char *s = strerror(errno);
			md_fatal("queue semaphore create fail: %s", s);
	}

	if( pthread_mutex_init(&queue_info.mtx_conn_queue, NULL) != 0 ) {
		md_fatal("queue mutex create fail");
	}

	STAILQ_INIT(&(queue_info.conn_queue));

	for(v = 0; v < N_THREADS; v++) {
		threads[v].thread_continue = 0;
		if(	pthread_create(&(threads[v].thread_id), NULL, (void *(*)(void *)) md_worker, &threads[v]) < 0) {
			md_fatal("thread pool spawn fail");
		}
	}

	default_loop = EV_DEFAULT;

	ev_signal_init(watcher_sigint, md_sigint_cb, SIGINT);
	ev_signal_start(default_loop, watcher_sigint);

	ev_io_init(watcher_accept, md_accept_cb, listen_sd, EV_READ);
	ev_io_start(default_loop, watcher_accept);

	TRACE();

	ev_run(default_loop, 0);
}

void md_accept_cb(struct ev_loop *loop, ev_io* watcher_accept, int revents) {
	TRACE();
	socklen_t sock_size = sizeof(struct sockaddr_in);
	conn_data *conn = malloc(sizeof(conn_data));

	if( (conn->open_sd = accept(listen_sd, (struct sockaddr *) &(conn->conn_info), &sock_size)) < 0) {
		md_fatal("accept fail from client %s", inet_ntoa(conn->conn_info.sin_addr));
	} else {
		#ifdef DEBUG
		md_log(LOGDEBUG, "accepted client %s", inet_ntoa(conn->conn_info.sin_addr));
		#endif
	}

	sem_wait(queue_info.sem_q_empty);
	pthread_mutex_lock(&queue_info.mtx_conn_queue);
	STAILQ_INSERT_TAIL(&queue_info.conn_queue, conn, q_next);
	pthread_mutex_unlock(&queue_info.mtx_conn_queue);
	sem_post(queue_info.sem_q_full);

	TRACE();
	#ifdef DEBUG
	md_log(LOGDEBUG, "enqueued client %s", inet_ntoa(conn->conn_info.sin_addr));
	#endif
}

void md_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents) {
	TRACE();
	close(listen_sd);
	conn_data no_conn[N_THREADS];

	for(int i = 0; i < N_THREADS; i++) {
		no_conn[i].open_sd = -1;
		sem_wait(queue_info.sem_q_empty);
		pthread_mutex_lock(&queue_info.mtx_conn_queue);
		STAILQ_INSERT_TAIL(&queue_info.conn_queue, &no_conn[i], q_next);
		pthread_mutex_unlock(&queue_info.mtx_conn_queue);
		sem_post(queue_info.sem_q_full);
	}

	if( sem_close(queue_info.sem_q_full) != 0 ||
		sem_close(queue_info.sem_q_empty) != 0 ) {
		char *s = strerror(errno);
		md_log(LOGDEBUG, "queue semaphore close fail: %s", s);
	}

	for(int i = 0; i < N_THREADS; i++) {
		pthread_join(threads[i].thread_id, NULL);
	}
	#ifdef DEBUG
	md_log(LOGDEBUG, "process quitting!");
	#endif
	exit(0);
}