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
	int listen_fd;
	int listen_address;
	struct sockaddr_in servaddr;
	pthread_t thread_id;
	pthread_attr_t attr;
	pthread_mutexattr_t mtx_attr;
	threadargs_t *t_args;
	reqargs_t *request;

	log_level = LOGDEBUG;
	log_fd = stdout;

	/* initialize the terminal output mutex before we start logging stuff */
	if( pthread_mutexattr_init(&mtx_attr) != 0 ||
		pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||
		pthread_mutex_init(&mtx_term, &mtx_attr) != 0 ) {
			printf("System error: unable to initialize terminal mutex");
		exit(ERRSYS);
	}

	/* inet_pton(AF_INET, ADDRESS, &listen_address); */
	listen_address = htonl(INADDR_ANY);

	/* set up a new socket for us to (eventually) listen us */
	if( (listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		oct_panic(ERRSYS, "socket create error");
	} else {
		oct_log(LOGINFO, "socket create success");
	}

	/* setsocketopt() sets a socket option to allow multiple processes to
	   listen. this is only needed during testing so we can quickly restart
	   the server without waiting for the port to close. */
	v = 1;
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0) {
		oct_panic(ERRSYS, "setsockopt() failed");
	} else {
		oct_log(LOGINFO, "setsockopt() SO_REUSEADDR success");
	}

	/* set the server listening parameters: IPv4, IP address, port number */
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(listen_address);
	servaddr.sin_port = htons(PORT);

	/* bind our socket to the specified address/port */
	if( bind(listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		oct_panic(ERRSYS, "socket error binding to %s", servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
	} else {
		oct_log(LOGINFO, "socket bound to %s", servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : ADDRESS);
	}

	/* set the socket to passive listening on the already-bound address/port */
	if( listen(listen_fd, LISTENQ) < 0 ) {
		oct_panic(ERRSYS, "socket listen error on port %d", PORT);
	} else {
		oct_log(LOGINFO, "socket set to listen on port %d", PORT);
	}

	/* set up a thread attributes structure so we don't need an extra call to pthread_detach() */
	if( pthread_attr_init(&attr) < 0 || pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) < 0) {
		oct_panic(ERRSYS, "error initializing thread attributes structure");
	} else {
		oct_log(LOGDEBUG, "initialized thread attributes structure");
	}

	/* save the size of the sockaddr structure since we need it held in a variable */
	v = sizeof(struct sockaddr_in);

	/* enter our socket listen/accept() loop */
	for(;;) {
		/* set up our thread argument structure. we need to pass the new connection descriptor and client address structure as well
		   as create read and write buffers */
		t_args = malloc(sizeof(threadargs_t));
		t_args->request = malloc(sizeof(reqargs_t));

		/* at the call to accept(), the main thread blocks until a client connects */
		if( (t_args->conn_fd = accept(listen_fd, (struct sockaddr *) &(t_args->conn_info), (socklen_t*) &v)) < 0) {
			oct_panic(ERRSYS, "connection accept failure from client %s", inet_ntoa(t_args->conn_info.sin_addr));
		} else {
			oct_log(LOGINFO, "accepted connection from client %s", inet_ntoa(t_args->conn_info.sin_addr));
		}

		/* spawn a new thread to handle the accept()'ed connection */
		if( (pthread_create(&thread_id, &attr, (void *(*)(void *)) oct_worker_thread, t_args)) < 0) {
			oct_panic(ERRSYS, "spawn connection thread failed with ID %d and address %s", thread_id, inet_ntoa(t_args->conn_info.sin_addr));
		}
	}
}