/*

Midnight main program file

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include <mdt_core.h>
#include <midnight.h>

void mdt_set_state_actions();
void mdt_worker(thread_info* opts);


thread_info *threads;

int main(int argc, char *argv[]){
	int v;
	struct sockaddr_in servaddr;

	struct ev_loop* default_loop;
	ev_signal* watcher_sigint = malloc(sizeof(ev_signal));

	mdt_log_init();

	mdt_options_init();

	log_info.level = -1;

	while( (v = getopt_long(argc, argv, "e:p:a:d:t:vh", optstruct, NULL)) != -1 ) {
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
			case 't':
				options_info.n_threads = strtol(optarg, NULL, 0);
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

	threads = calloc(options_info.n_threads, sizeof(thread_info));
	for(v = 0; v < options_info.n_threads; v++) {
		threads[v].listen_sd = listen_sd;
		threads[v].exec_continue = 1;
		if(	pthread_create(&threads[v].thread_id, NULL, (void *(*)(void *)) mdt_worker, &threads[v]) < 0) {
			mdt_fatal(ERRSYS, "thread pool spawn fail");
		}
	}

	default_loop = EV_DEFAULT;

	ev_signal_init(watcher_sigint, mdt_sigint_cb, SIGINT);
	ev_signal_start(default_loop, watcher_sigint);

	TRACE();

	ev_run(default_loop, 0);
}

void mdt_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents) {
	TRACE();
	close(listen_sd);

	#ifdef DEBUG
	mdt_log(LOGDEBUG, "process quitting!");
	#endif
	exit(0);
}

void mdt_options_init() {
	options_info.n_threads = 2;
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
	printf("\t-t, --threads threadnumber\t\tnumber of threads to use (default 2)\n");
	printf("\t-d, --docroot document-root\tsite document root directory (default ./docroot)\n");
	printf("\t-v, --version\t\tdisplay verison info\n");
}