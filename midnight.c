/*

Midnight main program file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

#define PORT 8080
#define ADDRESS "127.0.0.1"

struct conn_q_head_struct conn_q_head = STAILQ_HEAD_INITIALIZER(conn_q_head);

int main(int argc, char *argv[]){
	int v;
	int listen_address;
	struct sockaddr_in servaddr;
	pthread_t threads[N_THREADS];
	struct main_thread_info {
		int thread_no;
	};

	struct main_thread_info *main_info = malloc(sizeof(struct main_thread_info));
	main_info->thread_no = 0;

	struct ev_loop* default_loop;
	ev_io w_listen;

	log_level = LOGDEBUG;
	log_fd = stdout;

	/* initialize the terminal output mutex before we start logging stuff */
	MD_LOG_INIT();

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
		md_panic("error creating & binding socket to %s",
			servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
	} else {
		MD_LOG(LOGINFO, "socket bound to %s",
			servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
	}

	if( ( (sem_q_empty = sem_open("conn_empty", O_CREAT, 0644, MAXQUEUESIZE)) == SEM_FAILED) ||
		( (sem_q_full = sem_open("conn_full", O_CREAT, 0644, 0))
		 == SEM_FAILED) ||
		(pthread_mutex_init(&mtx_conn_queue, NULL) != 0 ) ) {
			md_panic("error creating connection queue locking mechanisms");
	}

	for(v = 0; v < N_THREADS; v++) {
		thread_info_t* t_info = malloc(sizeof(thread_info_t));
		t_info->thread_no = v + 1;
		if( (pthread_create(&threads[v], NULL, (void *(*)(void *)) md_worker, t_info)) < 0) {
			md_panic("thread pool spawn failure");
		}
	}

	STAILQ_INIT(&conn_q_head);

	default_loop = EV_DEFAULT;

	ev_io_init(&w_listen, md_accept_cb, listen_sd, EV_READ);
	ev_io_start(default_loop, &w_listen);

	MD_LOG(LOGINFO, "entering event loop");

	for(;;) {
		ev_run(default_loop, 0);
	}

	MD_LOG(LOGINFO, "shutting down!");
}

void md_accept_cb(struct ev_loop *loop, ev_io* w_listen, int revents) {
	socklen_t sock_size = sizeof(struct sockaddr_in);
	conn_data_t *new_conn = malloc(sizeof(conn_data_t));

	if( (new_conn->open_sd = accept(listen_sd, (struct sockaddr *) &(new_conn->conn_info), &sock_size)) < 0) {
		md_panic("connection accept failure from client %s", inet_ntoa(new_conn->conn_info.sin_addr));
	} else {
		MD_LOG(LOGINFO, "accepted connection from client %s", inet_ntoa(new_conn->conn_info.sin_addr));
	}

	sem_wait(sem_q_empty);
	pthread_mutex_lock(&mtx_conn_queue);
	STAILQ_INSERT_TAIL(&conn_q_head, new_conn, conn_q_next);
	pthread_mutex_unlock(&mtx_conn_queue);
	sem_post(sem_q_full);
	MD_LOG(LOGINFO, "queued new connection");
}

/* error handler. whole program dies on main thread md_panic, worker dies on worker thread md_panic. */
void md_panic(const char* message, ...) {
    if( LOGPANIC <= log_level) {
        //enter log output critical section
        pthread_mutex_lock(&mtx_term);
        #ifdef DEBUG
        printf("%u:\t", (unsigned int) pthread_self());
        #endif
        va_list arglist;
        va_start(arglist, message);
        vfprintf(log_fd, message, arglist);
        printf("\n");
        va_end(arglist);
        // leave log output critical section, no need to unlock (terminating)
    }
    exit(ERRPROG);
}