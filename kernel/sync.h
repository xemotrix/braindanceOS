#ifndef SPINLOCK_H
#define SPINLOCK_H

#include "scheduler.h"
#include <stdatomic.h>
#include <stdint.h>

typedef uint32_t mutex_t;

mutex_t sync_new_mutex(void);

wait_queue_id sync_mutex_wait_queue_id(mutex_t mx);
void sync_mutex_destroy(mutex_t mx);
void sync_mutex_acquire(mutex_t mx);
void sync_mutex_release(mutex_t mx);

#endif
