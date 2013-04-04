/*

Simple threaded HTTP server. Read GET replies from a host and respond appropriately.

Logging & panic facilities

(c) 2012 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

/* error handler. whole program dies on main thread panic, worker dies on worker thread panic. */
void panic(int error, const char* message, ...) {
    if( (error % 10 ? LOGPANIC : LOGERR) <= log_level) {
        if (pthread_mutex_lock(&mtx_term) != 0 ) {
            printf("%u:\terror acquiring terminal mutex", (unsigned int) pthread_self());
            exit(ERRSYS);
        }
        switch(error) {
            case ERRPROG:
            case THREADERRPROG:
                printf("Program error: ");
                break;
            case ERRSYS:
            case THREADERRSYS:
                printf("System error: ");
                break;
            default:
                ;
        }
        printf("%u:\t", (unsigned int) pthread_self());
        va_list arglist;
        va_start(arglist, message);
        vfprintf(log_fd, message, arglist);
        printf("\n");
        va_end(arglist);
        if (pthread_mutex_unlock(&mtx_term) != 0 ) {
            printf("%u:\terror releasing terminal mutex", (unsigned int) pthread_self());
            exit(ERRSYS);
        }
    }
    exit(error);
}

/* log stuff */
void log(int err_level, const char* message, ...) {
    if(err_level <= log_level) {
        if (pthread_mutex_lock(&mtx_term) != 0 ) {
            oct_panic(ERRSYS, "error acquiring terminal mutex");
        }
        printf("%u:\t", (unsigned int) pthread_self());
        va_list arglist;
        va_start(arglist, message);
        vfprintf(log_fd, message, arglist);
        va_end(arglist);
        printf("\n");
        if (pthread_mutex_unlock(&mtx_term) != 0 ) {
            panic(ERRSYS, "error releasing terminal mutex");
        }
    }
}