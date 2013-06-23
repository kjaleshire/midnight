/*

midnight core header

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#ifndef mdt_core_h
#define mdt_core_h

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dispatch/dispatch.h>	// grand central dispatch
#include <ev.h>					// libev event handler

/* app meta constants */
#define APP_NAME		"midnight"
#define APP_DESC		"A simple threaded+evented HTTP server"
#define MAJOR_V			0
#define MINOR_V			2
#define PATCH_V			0
#define PRERELEASE_V	""

/* app-wide constants */
#define LISTENQ			1024
#define TIMESTAMP_FMT	"%H:%M:%S %m.%d.%y"
#define TIMESTAMP_SIZE	32

/* error return types */
#define ERRPROG			-2
#define ERRSYS			-1

/* logging levels */
enum {
	LOGNONE,
	LOGERR,
	LOGINFO,
	LOGDEBUG
};

/* defaults */
#define DEFAULT_PORT		8080
#define DEFAULT_FILE		"index.html"
#define DEFAULT_DOCROOT		"docroot"

typedef struct conn_data {
    int open_sd;
    struct sockaddr_in conn_info;
} conn_data;

struct {
	dispatch_queue_t queue;

	int level;
	char timestamp[TIMESTAMP_SIZE];
	time_t ticks;
	struct tm* current_time;
} log_info;

struct {
	char* docroot;
	uint16_t port;
	uint32_t address;
} options_info;

/* log+fatal macros */
#define LOG_FD stderr
#ifdef DEBUG
#define mdt_log(e, m, ...)	\
		do {	\
			if((e) <= log_info.level) {	\
				log_info.ticks = time(NULL);	\
				log_info.current_time = localtime(&log_info.ticks);	\
				strftime(log_info.timestamp, TIMESTAMP_SIZE, TIMESTAMP_FMT, log_info.current_time);	\
				dispatch_async(log_info.queue, ^{	\
					fprintf(LOG_FD, "%s  ", log_info.timestamp);	\
					fprintf(LOG_FD, "%x:\t", (unsigned int) 0xabad1dea);	\
					fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
					fprintf(LOG_FD, "\n");	\
				});	\
			}	\
		} while(0)
#else
#define mdt_log(e, m, ...)	\
		do {	\
			if((e) <= log_info.level) {	\
				dispatch_async(log_info.queue, ^{	\
					fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
					fprintf(LOG_FD, "\n");	\
				});	\
			}	\
		} while(0)
#endif

#ifdef DEBUG
#define TRACE()  mdt_log(LOGDEBUG, "> %s:%d:%s", __FILE__, __LINE__, __FUNCTION__)
#else
#define TRACE()
#endif

/* spoilers: everyone dies */
#define mdt_fatal(e, m, ...)	\
		do {	\
			if(LOGERR <= log_info.level) {	\
				log_info.ticks = time(NULL);	\
				log_info.current_time = localtime(&log_info.ticks);	\
				strftime(log_info.timestamp, TIMESTAMP_SIZE, TIMESTAMP_FMT, log_info.current_time);	\
				dispatch_sync(log_info.queue, ^{	\
			        fprintf(LOG_FD, "%s  ", log_info.timestamp);	\
			        fprintf(LOG_FD, "%x:\t> %s:%d:%s:\tfatal: \"", (unsigned int) 0xdeadbeef, __FILE__, __LINE__, __FUNCTION__);	\
			        fprintf(LOG_FD, ":\t> %s:%d:%s:\tfatal: \"", __FILE__, __LINE__, __FUNCTION__);	\
			        fprintf(LOG_FD, (m), ##__VA_ARGS__);	\
			        fprintf(LOG_FD, "\"\n");	\
				});	\
				exit((e));	\
			}	\
		} while(0)

#define mdt_version() printf("%s version %d.%d.%d%s\n", APP_NAME, MAJOR_V, MINOR_V, PATCH_V, PRERELEASE_V)

#endif /* mdt_core_h */