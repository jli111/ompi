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

}ompi_wait_sync_t;

opal_mutex_t wait_sync_lock;
ompi_wait_sync_t wait_sync_head;
ompi_wait_sync_t wait_sync_tail;


#define REQUEST_PENDING        (void*)0L
#define REQUEST_COMPLETED      (void*)1L
#define WAIT_TAIL              (&wait_sync_tail)
#define WAIT_HEAD              (&wait_sync_head)

#define WAIT_SYNC_PASS_OWNERSHIP()                             \
    do{                                                        \
        pthread_mutex_lock( &(WAIT_HEAD->next)->lock);         \
        pthread_cond_signal( &(WAIT_HEAD->next)->condition );  \
        pthread_mutex_unlock( &(WAIT_HEAD->next)->lock);       \
    }while(0);                                                 \
 

#define WAIT_SYNC_INIT(sync,c)                        \
    do {                                              \
       (sync)->count = c;                             \
       (sync)->next = NULL;                           \
       (sync)->prev = NULL;                           \
       pthread_cond_init(&(sync)->condition,NULL);    \
       pthread_mutex_init(&(sync)->lock,NULL);        \
    }while(0);

#define WAIT_SYNC_RELEASE(sync)                       \
    do {                                              \
       pthread_cond_destroy(&(sync)->condition);      \
       pthread_mutex_destroy(&(sync)->lock);          \
    } while(0);

#if OPAL_ENABLE_MULTI_THREADS
#define OPAL_ATOMIC_ADD_32(a,b)         opal_atomic_add_32(a,b)   
#define OPAL_ATOMIC_SWP_PTR(a,b)        opal_atomic_swap_ptr(a,b)
              
#else
#define OPAL_ATOMIC_ADD_32(a,b)         (*a += b)
#define OPAL_ATOMIC_SWP_PTR(a,b)        (*a) = b         
#endif

static inline int sync_wait(ompi_wait_sync_t *sync){
    
    /* lock so noone can signal us during the list updating */
    pthread_mutex_lock(&sync->lock);   

    /* Insert sync to the list */
    OPAL_THREAD_LOCK(&wait_sync_lock);
    sync->prev = WAIT_TAIL->prev;
    sync->next = WAIT_TAIL;
    (WAIT_TAIL->prev)->next = sync;
    (WAIT_TAIL->prev)       = sync;
    OPAL_THREAD_UNLOCK(&wait_sync_lock);

    /*OWNERSHIP*/
    if( sync->prev == WAIT_HEAD ){
        pthread_mutex_unlock(&sync->lock);
        while(sync->count > 0){
            opal_progress();
        }

        /* Remove self, wake next one up, leave */
        OPAL_THREAD_LOCK(&wait_sync_lock);
        WAIT_HEAD->next = sync->next;
        sync->next->prev = WAIT_HEAD;
        WAIT_SYNC_PASS_OWNERSHIP();
        OPAL_THREAD_UNLOCK(&wait_sync_lock);
        
        return OPAL_SUCCESS;

    }
    else{
        pthread_cond_wait(&sync->condition, &sync->lock);
        pthread_mutex_unlock(&sync->lock);   

        OPAL_THREAD_LOCK(&wait_sync_lock);
        if( sync->count == 0 ){
            /* Remove self */
            sync->prev->next = sync->next;
            sync->next->prev = sync->prev;

            /* If progress ownership, pass it on*/
            if(sync->prev == WAIT_HEAD){
                WAIT_SYNC_PASS_OWNERSHIP();
            }
            OPAL_THREAD_UNLOCK(&wait_sync_lock);
            return OPAL_SUCCESS;
        }
        else{
            /* If you get here, you are now the successor */
            /* unlock and do the work until you're done. */
            assert( WAIT_HEAD->next == sync);
            OPAL_THREAD_UNLOCK(&wait_sync_lock);
            while(sync->count > 0){
                opal_progress();
            }

            /* You're done, remove self, pass the ownership, leave */
            OPAL_THREAD_LOCK(&wait_sync_lock);
            WAIT_HEAD->next = sync->next;
            sync->next->prev = WAIT_HEAD;
            WAIT_SYNC_PASS_OWNERSHIP();
            OPAL_THREAD_UNLOCK(&wait_sync_lock);
            return OPAL_SUCCESS;

        }

    }
    
}

static inline void wait_sync_startup(void){

    OBJ_CONSTRUCT( &wait_sync_lock, opal_mutex_t );
    WAIT_SYNC_INIT( &wait_sync_head, 0);   
    WAIT_SYNC_INIT( &wait_sync_tail, 0);   
    wait_sync_head.next = &wait_sync_tail;
    wait_sync_tail.prev = &wait_sync_head;


}

static inline void wait_sync_update(ompi_wait_sync_t *sync){
    
    assert(REQUEST_COMPLETED != sync); 
    if((OPAL_ATOMIC_ADD_32(&sync->count,-1)) <= 0){
      pthread_cond_signal(&sync->condition);
    }
}



END_C_DECLS
