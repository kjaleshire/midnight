/*

Midnight main program file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

#define PORT 8080
#define ADDRESS "127.0.0.1"

int listen_sd;
thread_info threads[N_THREADS];
struct conn_q_head_struct conn_q_head = STAILQ_HEAD_INITIALIZER(conn_q_head);

int main(int argc, char *argv[]){
	int v;
	int listen_address;
	struct sockaddr_in servaddr;

	struct ev_loop* default_loop;
	ev_io* watcher_accept = malloc(sizeof(ev_io));
	ev_signal* watcher_sigint = malloc(sizeof(ev_signal));

	log_info.log_level = LOGDEBUG;

	md_log_init();
	md_set_state_actions(&state_actions);

	/* inet_pton(AF_INET, ADDRESS, &listen_address); */
	listen_address = htonl(INADDR_ANY);

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(listen_address);
	servaddr.sin_port = htons(PORT);

	if( (listen_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ||
	#ifdef DEBUG
		setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0 ||	// so we can quickly reuse port
	#endif
		bind(listen_sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ||
		listen(listen_sd, LISTENQ) < 0
	) {
		md_fatal("socket create & bind fail on %s",
			servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
	}
	else {
		TRACE();
	}

	// unlink the semaphore names because they persist after program exit/crash/terminate
	// see sem
	if( (sem_unlink("sem_q_empty") != 0 && errno != ENOENT) ||
		(sem_unlink("sem_q_full") != 0 && errno != ENOENT) ||
		(sem_q_empty = sem_open("sem_q_empty", O_CREAT, 0600, MAXQUEUESIZE)) == SEM_FAILED ||
		(sem_q_full = sem_open("sem_q_full", O_CREAT, 0600, 0)) == SEM_FAILED ||
		pthread_mutex_init(&mtx_conn_queue, NULL) != 0 ) {
			md_fatal("queue sem/mutex create fail");
	}

	STAILQ_INIT(&conn_q_head);

	for(v = 0; v < N_THREADS; v++) {
		char c[8];
		snprintf(c, sizeof(c), "tID %d", v);
		if( (threads[v].quit = sem_open(c, O_CREAT, 0644, 0)) == SEM_FAILED ||
			pthread_create(&(threads[v].thread_id), NULL, (void *(*)(void *)) md_worker, &threads[v]) < 0) {
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
	socklen_t sock_size = sizeof(struct sockaddr_in);
	conn_data *conn = malloc(sizeof(conn_data));

	if( (conn->open_sd = accept(listen_sd, (struct sockaddr *) &(conn->conn_info), &sock_size)) < 0) {
		md_fatal("accept fail from client %s", inet_ntoa(conn->conn_info.sin_addr));
	} else {
		TRACE();
		#ifdef DEBUG
		md_log(LOGDEBUG, "accepted client %s, descriptor %d", inet_ntoa(conn->conn_info.sin_addr), conn->open_sd);
		#endif
	}

	assert(sem_wait(sem_q_empty) == 0);
	pthread_mutex_lock(&mtx_conn_queue);
	STAILQ_INSERT_TAIL(&conn_q_head, conn, conn_q_next);
	pthread_mutex_unlock(&mtx_conn_queue);
	assert(sem_post(sem_q_full) == 0);

	TRACE();
	#ifdef DEBUG
	md_log(LOGDEBUG, "enqueued client %s, descriptor %d", inet_ntoa(conn->conn_info.sin_addr), conn->open_sd);
	#endif
}

void md_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents) {
	TRACE();
	close(listen_sd);
	#ifdef DEBUG
	md_log(LOGDEBUG, "Shutdown");
	#endif
	/*for(i = 0; i < N_THREADS; i++) {
		sem_post(threads[i].quit);
	}
	for(i = 0; i < N_THREADS; i++) {
		pthread_join(threads[i].thread_id, NULL);
	} */
	exit(0);
}