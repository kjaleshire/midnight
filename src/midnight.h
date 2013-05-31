/*

midnight main program file header

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include <getopt.h>
#include <sys/socket.h>

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
void mdt_log_init();