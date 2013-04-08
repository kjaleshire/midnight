/*

Midnight logging & panic facilities

(c) 2013 Kyle J Aleshire
All rights reserved

*/

#include "midnight.h"

/* initialize the log facility */
int log_init() {
    pthread_mutexattr_t mtx_attr;
    if( pthread_mutexattr_init(&mtx_attr) != 0 ||
        pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||
        pthread_mutex_init(&mtx_term, &mtx_attr) != 0 ) {
            printf("System error: unable to initialize terminal mutex");
            exit(ERRSYS);
    }
    return 0;
}

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
void logmsg(int err_level, const char* message, ...) {
    if(err_level <= log_level) {
        if (pthread_mutex_lock(&mtx_term) != 0 ) {
            panic(ERRSYS, "error acquiring terminal mutex");
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