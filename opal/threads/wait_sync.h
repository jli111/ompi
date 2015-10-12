#include "opal/sys/atomic.h"
#include "opal/threads/condition.h"


BEGIN_C_DECLS

struct ompi_wait_sync_t {
    int count;
    opal_condition_t *condition;
    opal_mutex_t *lock;

};

typedef struct ompi_wait_sync_t ompi_wait_sync_t;

#define WAIT_SYNC_INIT(sync,c)                        \
    do {                                              \
       (sync)->count = c;                             \
       (sync)->condition = OBJ_NEW(opal_condition_t); \
       (sync)->lock = OBJ_NEW(opal_mutex_t);          \
    }while(0);

#define WAIT_SYNC_RELEASE(sync)                       \
    do {                                              \
       OBJ_RELEASE( (sync)->condition );              \
       OBJ_RELEASE( (sync)->lock );                   \
    } while(0);

#if (HAVE_PTHREAD_H == 1)
#define OPAL_ATOMIC_ADD_32(a,b)    opal_atomic_add_32(a,b);                   
#else
#define OPAL_ATOMIC_ADD_32(a,b)    (*a += b);
#endif /* HAVE_PTHREAD_H */

static inline void wait_sync_update(ompi_wait_sync_t *sync){

    OPAL_ATOMIC_ADD_32(&sync->count,-1);
    if(sync->count == 0)
      opal_condition_signal(sync->condition);
}



END_C_DECLS
