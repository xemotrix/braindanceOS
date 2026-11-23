#ifndef MULTITASKING_H
#define MULTITASKING_H
#include "irq.h"
#include <stdint.h>
#include <utils/dynamic_array.h>

enum task_state {
  TASK_STATE_UNUSED,
  TASK_STATE_RUNNING,
  TASK_STATE_READY,
  TASK_STATE_SLEEPING,
  TASK_STATE_WAITING,
};

typedef int32_t wait_queue_id;

wait_queue_id scheduler_create_wait_queue();
void scheduler_destroy_wait_queue(wait_queue_id id);

void scheduler_init() __attribute__((noreturn));
void scheduler_create_kernel_task(void (*startingEIP)(void), const char *name);
void kexit() __attribute__((noreturn));
uint64_t scheduler_get_time();

uint32_t scheduler_lock();
void scheduler_unlock(uint32_t flags);

#define SCH_LOCK uint32_t __scheduler_flags = scheduler_lock();
#define SCH_UNLOCK scheduler_unlock(__scheduler_flags);

#define SCH_INLOCK(block)                                                      \
  do {                                                                         \
    uint32_t __scheduler_flags = scheduler_lock();                             \
    block;                                                                     \
    scheduler_unlock(__scheduler_flags);                                       \
  } while (0);

#define SCH_DO_AND_WAIT_FOR(evt, block)                                        \
  do {                                                                         \
    uint32_t __scheduler_lock_flags = scheduler_lock();                        \
    block;                                                                     \
    scheduler_wait_for((evt));                                                 \
    scheduler_unlock(__scheduler_lock_flags);                                  \
  } while (0);

#define SCH_WAIT_WHILE_COND(evt, cond)                                         \
  do {                                                                         \
    uint32_t __scheduler_lock_flags = scheduler_lock();                        \
    while ((cond)) {                                                           \
      scheduler_wait_for((evt));                                               \
    }                                                                          \
    scheduler_unlock(__scheduler_lock_flags);                                  \
  } while (0);

void scheduler_wakeup(wait_queue_id evt);
void scheduler_wakeup_one(wait_queue_id evt);
void scheduler_wait_for(wait_queue_id evt);

void nano_sleep(int64_t nanos);
void milli_sleep(int64_t millis);
void sleep(int64_t seconds);
void scheduler_PIT_IRQ_handler();
struct interrupt_frame *scheduler_irq_exit(struct interrupt_frame *frame);

//====READ INFO=======================
struct scheduler_task_info {
  uint32_t pid;
  enum task_state state;
  uint64_t sleep_expiry;
  uint64_t time_used;
  const char *name;
};
DEFARR_STRUCT(scheduler_task_info);
struct scheduler_info {
  uint64_t time_since_boot;
  scheduler_task_info_array tasks;
};
struct scheduler_info scheduler_info();
//====================================

#endif
