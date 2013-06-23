/*

Midnight main program file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include <mdt_core.h>
#include <midnight.h>

void mdt_set_state_actions();
void mdt_dispatch_connection(conn_data*);

int main(int argc, char *argv[]){
	int v;
	struct sockaddr_in servaddr;

	struct ev_loop* default_loop;
	ev_io* watcher_accept = malloc(sizeof(ev_io));
	ev_signal* watcher_sigint = malloc(sizeof(ev_signal));

	log_info.queue = dispatch_queue_create("com.kja.midnight.logqueue", DISPATCH_QUEUE_SERIAL);

	mdt_options_init();

	log_info.level = -1;

	while( (v = getopt_long(argc, argv, "e:p:a:d:vh", optstruct, NULL)) != -1 ) {
		switch(v) {
			case 'e':
				log_info.level = strtol(optarg, NULL, 0);
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
			case 'v':
				mdt_version();
				exit(0);
			case 'h':
			default:
				mdt_version();
				mdt_usage();
				exit(0);
		}
	}

	if(log_info.level == -1) {
		#ifdef DEBUG
		log_info.level = LOGDEBUG;
		#else
		log_info.level = LOGINFO;
		#endif
	}

	mdt_set_state_actions();

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = options_info.address;
	servaddr.sin_port = options_info.port;

	v = 1;
	if( (listen_sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ||
#ifdef DEBUG
	setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0 ||	// so we can quickly reuse port
#endif
	bind(listen_sd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ||
	listen(listen_sd, LISTENQ) < 0 ) {
		mdt_fatal(ERRSYS, "socket create & bind fail on %s",
		servaddr.sin_addr.s_addr == htonl(INADDR_ANY) ? "INADDR_ANY" : inet_ntoa(servaddr.sin_addr));
	}
	TRACE();

	default_loop = EV_DEFAULT;

	ev_signal_init(watcher_sigint, mdt_sigint_cb, SIGINT);
	ev_signal_start(default_loop, watcher_sigint);

	ev_io_init(watcher_accept, mdt_accept_cb, listen_sd, EV_READ);
	ev_io_start(default_loop, watcher_accept);

	TRACE();

	ev_run(default_loop, 0);
}

void mdt_accept_cb(struct ev_loop *loop, ev_io* watcher_accept, int revents) {
	TRACE();
	socklen_t sock_size = sizeof(struct sockaddr_in);
	conn_data *conn = malloc(sizeof(conn_data));

	if( (conn->open_sd = accept(listen_sd, (struct sockaddr *) &conn->conn_info, &sock_size)) < 0) {
		mdt_fatal(ERRSYS, "accept fail from client %s", inet_ntoa(conn->conn_info.sin_addr));
	} else {
		#ifdef DEBUG
		mdt_log(LOGDEBUG, "accepted client %s", inet_ntoa(conn->conn_info.sin_addr));
		#endif
	}

	mdt_dispatch_connection(conn);

	TRACE();
	#ifdef DEBUG
	mdt_log(LOGDEBUG, "enqueued client %s", inet_ntoa(conn->conn_info.sin_addr));
	#endif
}

void mdt_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents) {
	TRACE();
	close(listen_sd);

	#ifdef DEBUG
	mdt_log(LOGDEBUG, "process quitting!");
	#endif

	dispatch_sync(log_info.queue, ^{});
	exit(0);
}

void mdt_options_init() {
	options_info.address = htonl(INADDR_ANY);
	options_info.port = htons(DEFAULT_PORT);
	options_info.docroot = DEFAULT_DOCROOT;
}

void mdt_usage() {
	printf("\n");
	printf("  Usage:\n");
	printf("\tmidnight [-hv] [-e verbosity] [-a listenaddress] [-p listenport] [-d docroot]\n");
	printf("  Options:\n");
	printf("\t-h, --help\t\tthis help\n");
	printf("\t-e, --verbosity 0-3\t\tset verbosity level 0-3 (silent->debugging info)\n");
	printf("\t-p, --port portnumber\t\tport to listen on (default 8080)\n");
	printf("\t-a, --address IP-address\t\taddress to bind to (default all)\n");
	printf("\t-d, --docroot document-root\tsite document root directory (default ./docroot)\n");
	printf("\t-v, --version\t\tdisplay verison info\n");
}