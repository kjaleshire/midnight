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
void panic(const char* message, ...) {
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
    exit(-1);
}

/* log stuff */
void logmsg(int err_level, const char* message, ...) {
    if(err_level <= log_level) {
        pthread_mutex_lock(&mtx_term);
        printf("%u:\t", (unsigned int) pthread_self());
        va_list arglist;
        va_start(arglist, message);
        vfprintf(log_fd, message, arglist);
        va_end(arglist);
        printf("\n");
        pthread_mutex_unlock(&mtx_term);
    }
}