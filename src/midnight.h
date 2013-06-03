/*

midnight main program file header

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef midnight_h
#define midnight_h

#include <getopt.h>
#include <sys/socket.h>

static int listen_sd;
static thread_info* threads;

static struct option optstruct[] = {
	{ "help", no_argument, NULL, 'h'},
	{ "error", no_argument, NULL, 'e'},
	{ "quiet", no_argument, NULL, 'q'},
	{ "port", required_argument, NULL, 'p'},
	{ "address", required_argument, NULL, 'a'},
	{ "docroot", required_argument, NULL, 'd'},
	{ "nthreads", required_argument, NULL, 't'},
	{ "version", no_argument, NULL, 'v'}
};

void mdt_options_init();
void mdt_usage();
void mdt_log_init();
void mdt_accept_cb(struct ev_loop* loop, ev_io* watcher_accept, int revents);
void mdt_sigint_cb(struct ev_loop *loop, ev_signal* watcher_sigint, int revents);

#endif /* midnight_h */