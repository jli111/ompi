#include "opal/sys/atomic.h"
#include "opal/threads/condition.h"


BEGIN_C_DECLS

struct ompi_wait_sync_t {
    int count;
    opal_condition_t condition;
    opal_mutex_t lock;

};

typedef struct ompi_wait_sync_t ompi_wait_sync_t;

#define WAIT_SYNC_INIT(sync,c)                        \
    do {                                              \
       (sync)->count = c;                             \
       OBJ_CONSTRUCT(&((sync)->condition), opal_condition_t); \
       OBJ_CONSTRUCT(&((sync)->lock), opal_mutex_t );         \
    }while(0);

#define WAIT_SYNC_RELEASE(sync)                       \
    do {                                              \
       OBJ_DESTRUCT( &((sync)->condition ));              \
       OBJ_DESTRUCT( &((sync)->lock ));                   \
    } while(0);

#if OPAL_ENABLE_PROGRESS_THREADS
#define OPAL_ATOMIC_ADD_32(a,b)         opal_atomic_add_32(a,b);     
#define OPAL_ATOMIC_SWP_PTR(a,b)        opal_atomic_swap_ptr(a,b);              
#else
#define OPAL_ATOMIC_ADD_32(a,b)         (*a += b);
#define OPAL_ATOMIC_SWP_PTR(a,b)        (*a) = b;         
#endif

static inline void wait_sync_update(ompi_wait_sync_t *sync){

    assert(sync != NULL);
    if(OPAL_ATOMIC_ADD_32(&sync->count,-1) <= 0){
      opal_condition_signal(&sync->condition);
}



END_C_DECLS
