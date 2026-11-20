#include "sync.h"
#include "panic.h"
#include <stdatomic.h>

struct mutex_data {
  atomic_flag aflag;
  wait_queue_id qid;
};

DEFARR_STRUCT(mutex_data);
mutex_data_array mutexes;

wait_queue_id sync_mutex_wait_queue_id(mutex_t mx) {
  ASSERT(mx != 0);
  return mutexes.elts[mx].qid;
}

mutex_t sync_new_mutex(void) {
  SCH_LOCK;

  if (mutexes.len == 0) {
    // create the null mutex (never valid)
    DA_APPEND(mutexes, ((struct mutex_data){.qid = -1}));
  }

  for (size_t i = 1; i < mutexes.len; i++) {
    if (mutexes.elts[i].qid < 0) {
      mutexes.elts[i] = (struct mutex_data){
          .aflag = (atomic_flag){0},
          .qid = scheduler_create_wait_queue(),
      };
      SCH_UNLOCK;
      return i;
    }
  }
  size_t idx = mutexes.len;
  DA_APPEND(mutexes, ((struct mutex_data){
                         .aflag = (atomic_flag){0},
                         .qid = scheduler_create_wait_queue(),
                     }));
  SCH_UNLOCK;
  return idx;
}

void sync_mutex_destroy(mutex_t mx) {
  ASSERT(mx != 0);
  SCH_INLOCK({
    scheduler_destroy_wait_queue(mutexes.elts[mx].qid);
    mutexes.elts[mx].qid = -1;
    mutexes.elts[mx].aflag = (atomic_flag){0};
  });
}

void sync_mutex_acquire(mutex_t mx) {
  ASSERT(mx != 0);
  SCH_WAIT_ON_COND(mutexes.elts[mx].qid,
                   atomic_flag_test_and_set(&mutexes.elts[mx].aflag));
}

void sync_mutex_release(mutex_t mx) {
  ASSERT(mx != 0);
  SCH_INLOCK({
    atomic_flag_clear(&mutexes.elts[mx].aflag);
    scheduler_wakeup_one(mutexes.elts[mx].qid);
  });
}
