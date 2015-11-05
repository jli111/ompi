/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2014-2015 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "opal/sys/atomic.h"
#include "opal/threads/condition.h"
#include <pthread.h>

BEGIN_C_DECLS

typedef struct ompi_wait_sync_t {
    int count;
    pthread_cond_t condition;
    pthread_mutex_t lock;
    struct ompi_wait_sync_t *next;
    struct ompi_wait_sync_t *prev;
} ompi_wait_sync_t;

#define REQUEST_PENDING        (void*)0L
#define REQUEST_COMPLETED      (void*)1L

#define WAIT_SYNC_INIT(sync,c)                        \
    do {                                              \
       (sync)->count = c;                             \
       (sync)->next = NULL;                           \
       (sync)->prev = NULL;                           \
       pthread_cond_init(&(sync)->condition,NULL);    \
       pthread_mutex_init(&(sync)->lock,NULL);        \
    } while(0)

#define WAIT_SYNC_RELEASE(sync)                       \
    do {                                              \
       pthread_cond_destroy(&(sync)->condition);      \
       pthread_mutex_destroy(&(sync)->lock);          \
    } while(0);

#if OPAL_ENABLE_MULTI_THREADS
#define OPAL_ATOMIC_ADD_32(a,b)         opal_atomic_add_32(a,b)   
#define OPAL_ATOMIC_SWP_PTR(a,b)        opal_atomic_swap_ptr(a,b)
              
#else
#define OPAL_ATOMIC_ADD_32(a,b)         (*(a) += (b))
#define OPAL_ATOMIC_SWP_PTR(a,b)        *(a) = (b)
#endif

OPAL_DECLSPEC int sync_wait(ompi_wait_sync_t *sync);

static inline void wait_sync_update(ompi_wait_sync_t *sync)
{
    assert(REQUEST_COMPLETED != sync); 
    if( (OPAL_ATOMIC_ADD_32(&sync->count,-1)) <= 0) {
        pthread_cond_signal(&sync->condition);
    }
}

END_C_DECLS
