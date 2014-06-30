/* 
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#ifndef OPAL_CONDITION_SPINLOCK_H
#define OPAL_CONDITION_SPINLOCK_H

#include "opal_config.h"
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include <pthread.h>

#include "opal/threads/mutex.h"
#include "opal/runtime/opal_progress.h"

#include "opal/runtime/opal_cr.h"
#include "opal/mca/timer/base/base.h"
BEGIN_C_DECLS
extern int btl_progress_signal_count;
double elapsed_time1,elapsed_time2;
/*
 * Combine pthread support w/ polled progress to allow run-time selection
 * of threading vs. non-threading progress.
 */

struct opal_condition_t {
    opal_object_t super;
    pthread_cond_t btl_progress_cond;
    opal_mutex_t request_lock;
    int btl_progress_thread;
    volatile int c_waiting;
    volatile int c_signaled;
};
typedef struct opal_condition_t opal_condition_t;

OPAL_DECLSPEC OBJ_CLASS_DECLARATION(opal_condition_t);


static inline int opal_condition_wait(opal_condition_t *c, opal_mutex_t *m)
{
    int rc = 0;
    c->c_waiting++;

    if (opal_using_threads()) {
        if (c->c_signaled) {
            c->c_waiting--;
            opal_mutex_unlock(m);
            opal_progress();
            OPAL_CR_TEST_CHECKPOINT_READY_STALL();
            opal_mutex_lock(m);
            return 0;
        }
	elapsed_time1 = (double)opal_timer_base_get_usec();
        while (c->c_signaled == 0) {
	    elapsed_time2 = (double)opal_timer_base_get_usec();
            opal_mutex_unlock(m);
            opal_progress();
            OPAL_CR_TEST_CHECKPOINT_READY_STALL();

	/*
 	 * If we have btl progress thread, we can do the sleep wait instead of 
 	 * busy wait to reduce the cpu usage after some certain amount of time.
 	 */
	    
	    if(c->btl_progress_thread > 0 && elapsed_time2 - elapsed_time1 > 90){
		pthread_mutex_lock(&(c->request_lock).btl_progress_lock);
		while(btl_progress_signal_count < 0)
			pthread_cond_wait(&c->btl_progress_cond,&c->request_lock.btl_progress_lock);
		btl_progress_signal_count--;
		pthread_mutex_unlock(&c->request_lock.btl_progress_lock);
	    	elapsed_time1 = (double)opal_timer_base_get_usec();
	    }	

            opal_mutex_lock(m);
        }
    } else {
        while (c->c_signaled == 0) {
            opal_progress();
            OPAL_CR_TEST_CHECKPOINT_READY_STALL();
        }
    }

    c->c_signaled--;
    c->c_waiting--;
    return rc;
}

static inline int opal_condition_timedwait(opal_condition_t *c,
                                           opal_mutex_t *m,
                                           const struct timespec *abstime)
{
    struct timeval tv;
    struct timeval absolute;
    int rc = 0;

    c->c_waiting++;
    if (opal_using_threads()) {
        absolute.tv_sec = abstime->tv_sec;
        absolute.tv_usec = abstime->tv_nsec * 1000;
        gettimeofday(&tv,NULL);
        if (c->c_signaled == 0) {
            do {
                opal_mutex_unlock(m);
                opal_progress();
                gettimeofday(&tv,NULL);
                opal_mutex_lock(m);
                } while (c->c_signaled == 0 &&  
                         (tv.tv_sec <= absolute.tv_sec ||
                          (tv.tv_sec == absolute.tv_sec && tv.tv_usec < absolute.tv_usec)));
        }
    } else {
        absolute.tv_sec = abstime->tv_sec;
        absolute.tv_usec = abstime->tv_nsec * 1000;
        gettimeofday(&tv,NULL);
        if (c->c_signaled == 0) {
            do {
                opal_progress();
                gettimeofday(&tv,NULL);
                } while (c->c_signaled == 0 &&  
                         (tv.tv_sec <= absolute.tv_sec ||
                          (tv.tv_sec == absolute.tv_sec && tv.tv_usec < absolute.tv_usec)));
        }
    }

    if (c->c_signaled != 0) c->c_signaled--;
    c->c_waiting--;
    return rc;
}

static inline int opal_condition_signal(opal_condition_t *c)
{
    if (c->c_waiting) {
        c->c_signaled++;
    }
    pthread_cond_signal(&c->btl_progress_cond);
    return 0;
}

static inline int opal_condition_broadcast(opal_condition_t *c)
{
    c->c_signaled = c->c_waiting;
    pthread_cond_signal(&c->btl_progress_cond);
    btl_progress_signal_count++;
    return 0;
}

END_C_DECLS

#endif

