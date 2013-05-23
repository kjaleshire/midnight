/*

Midnight main program file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

int listen_sd;
thread_info *threads;

int main(int argc, char *argv[]){
	int v;
	struct sockaddr_in servaddr;

	struct ev_loop* default_loop;
	ev_io* watcher_accept = malloc(sizeof(ev_io));
	ev_signal* watcher_sigint = malloc(sizeof(ev_signal));

	md_options_init();

	#ifdef DEBUG
	log_info.log_level = LOGDEBUG;
	#endif

	while( (v = getopt_long(argc, argv, "eqp:a:d:t:vh", optstruct, NULL)) != -1 ) {
		switch(v) {
			case 'e':
				log_info.log_level = LOGERR;
				break;
			case 'q':
				log_info.log_level = LOGNONE;
				break;
			case 'd':
				options_info.docroot = optarg;
				break;
			case 'a':
				inet_pton(AF_INET, optarg, &options_info.address);
				break;
			case 'p':
				options_info.port = htons(strtol(optarg, NULL, 0));
				break;
			case 't':
				options_info.n_threads = strtol(optarg, NULL, 0);
				break;
			case 'v':
				md_version();
				exit(0);
			case 'h':
			default:
				md_version();
				md_usage();
				exit(0);
		}
	}

	md_set_state_actions(&state_actions);

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = options_info.address;
	servaddr.sin_port = options_info.port;

	v = 1;
	if( (listen_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ||
	#ifdef DEBUG
		setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0 ||	// so we can quickly reuse port
	#endif
		bind(listen_sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ||
		listen(listen_sd, LISTENQ) < 0
	) {
		md_fatal("socket create & bind fail on %s",
			servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : inet_ntoa(servaddr.sin_addr));
	}
	else {
		TRACE();
	}

	if( (queue_info.sem_q_empty = sem_open("empty", O_CREAT, 0644, options_info.n_threads * MAXQUEUESIZE)) == SEM_FAILED ||
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

	threads = calloc(options_info.n_threads, sizeof(thread_info));
	for(v = 0; v < options_info.n_threads; v++) {
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
	conn_data no_conn[options_info.n_threads];

	for(int i = 0; i < options_info.n_threads; i++) {
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

	for(int i = 0; i < options_info.n_threads; i++) {
		pthread_join(threads[i].thread_id, NULL);
	}
	#ifdef DEBUG
	md_log(LOGDEBUG, "process quitting!");
	#endif
	exit(0);
}

void md_usage() {
	printf("\n");
	printf("  Usage:\n");
	printf("\tmidnight [-hevq] [-t threadnum] [-a listenaddress] [-p listenport] [-d docroot]\n");
	printf("  Options:\n");
	printf("\t-h, --help\t\tthis help\n");
	printf("\t-e, --error\t\terror-only logging\n");
	printf("\t-q, --quiet\t\tno logging\n");
	printf("\t-p, --port\t\tport to listen on (default 8080)\n");
	printf("\t-a, --address\t\taddress to bind to (default all)\n");
	printf("\t-d, --docroot\t\tsite document root directory (default ./docroot)\n");
	printf("\t-t, --nthreads\t\tnumber of threads to run with (default 2)\n");
	printf("\t-v, --version\t\tdisplay verison info\n");
}