#include "dach/dach_lock.h"
#include "dach/dach_mem.h"
#include <assert.h>
#include <pthread.h>

static int lock_lock   (dach_lock *lock);
static int lock_unlock (dach_lock *lock);

static dach_lock_ops b_ops = {
  .lock    = lock_lock,
  .unlock  = lock_unlock,
};

typedef struct dach_lock_obj {
  int              type;
  pthread_t        thread_id; //invariant (only one thread should be able to work on a block)
} dach_lock_obj;

typedef struct dach_lock_impl {
  dach_lock_obj *obj;
  dach_lock_ops *ops;
} dach_lock_impl;

dach_lock * dach_lock_new() {
  dach_lock_impl *dl_impl;
  DACH_NEW(dl_impl);
  return (dach_lock*)dl_impl;
}

void dach_lock_free(dach_lock *lock) {
  assert(lock);
  dach_lock_impl *dl_impl = (dach_lock_impl*)lock;
  DACH_FREE(dl_impl);

}


