#include "scheduler.h"
#include "../utils/dynamic_array.h"
#include "panic.h"
#include "vmm.h"
#include <stddef.h>

// #define DISABLE_PREEMTION

struct task {
  uint32_t pid;
  struct interrupt_frame *frame;
  const char *name;
  enum task_state state;
  void *stack_block;
  uint64_t sleep_expiry;
  uint64_t last_running_time;
  uint64_t time_used;
  struct task *next;
};

struct waiting_task_queue {
  struct task *head;
  struct task *tail;
};

extern void __switch_to_task(struct interrupt_frame **current,
                             struct interrupt_frame *target);
extern uint32_t __scheduler_lock(void);
extern void __scheduler_unlock(uint32_t flags);
extern void __start_task(struct interrupt_frame *target);

// ==DEFS====================
#define MAX_TASKS 256
#define MAX_PREEMPT_TICKS 50
#define NANOS_PER_TICK 1000000
#define STACK_N_PAGES 10

// ==STATE===================
struct task tasks[MAX_TASKS];
struct task idle_task;
uint32_t n_tasks;
volatile int32_t current_task = -1;
volatile uint32_t preempt_ticks = 0;
volatile uint64_t time_since_boot = 0; // nanos
volatile uint64_t last_time_count = 0;
uint32_t pid_counter;

// struct waiting_task_queue wait_queues[WAKEUP_MAX] = {
//     [WAKEUP_ATA] = {0},
// };

DEFARR_STRUCT(waiting_task_queue);
waiting_task_queue_array wait_queues;

wait_queue_id scheduler_create_wait_queue() {
  uint32_t flags = scheduler_lock();
  DA_APPEND(wait_queues, (struct waiting_task_queue){0});
  wait_queue_id idx = wait_queues.len - 1;
  scheduler_unlock(flags);
  return idx;
}

void scheduler_destroy_wait_queue(wait_queue_id id) {
  // todo
}

// ==========================
void scheduler_block_task(enum task_state state);
static int32_t scheduler_find_ready_task();
static void update_time_used();

// ==========================
uint32_t scheduler_lock() { return __scheduler_lock(); }
void scheduler_unlock(uint32_t flags) { return __scheduler_unlock(flags); }
static void scheduler_switch_to_task(struct interrupt_frame **current,
                                     struct interrupt_frame *target) {
  update_time_used();
  __switch_to_task(current, target);
}

uint64_t scheduler_get_time() { return time_since_boot; }

// must be called inside lock
void scheduler_wait_for(wait_queue_id evt) {
  ASSERT(current_task != -1);

  uint32_t flags = scheduler_lock();
  struct task *tsk = &tasks[current_task];
  ASSERT(tsk->state == TASK_STATE_RUNNING);

  struct waiting_task_queue *q = &wait_queues.elts[evt];
  if (!q->tail) {
    ASSERT(!q->head);
    q->head = tsk;
    q->tail = tsk;
  } else {
    ASSERT(q->tail->next == NULL);
    q->tail->next = tsk;
    q->tail = tsk;
  }
  scheduler_block_task(TASK_STATE_WAITING);
  scheduler_unlock(flags);
}

// must be called between locks
void scheduler_wakeup(wait_queue_id evt) {
  struct waiting_task_queue *q = &wait_queues.elts[evt];
  struct task *tsk = q->head;
  while (tsk) {
    struct task *next_tsk = tsk->next;
    tsk->next = NULL;
    tsk->state = TASK_STATE_READY;
    tsk = next_tsk;
  }
  q->head = NULL;
  q->tail = NULL;
  // don't switch inmediately, wait for preemption
}

void scheduler_wakeup_one(wait_queue_id evt) {
  struct waiting_task_queue *q = &wait_queues.elts[evt];
  if (!q->head)
    return;
  struct task *tsk = q->head;
  struct task *next_tsk = tsk->next;
  q->head = next_tsk;
  if (!q->head)
    q->tail = NULL;

  tsk->next = NULL;
  tsk->state = TASK_STATE_READY;
}

struct scheduler_info scheduler_info() {
  uint32_t flags = __scheduler_lock();
  struct scheduler_info si = {
      .time_since_boot = time_since_boot,
      .tasks = {0},
  };

  for (size_t i = 0; i < n_tasks; i++) {
    struct task t = tasks[i];
    if (t.state == TASK_STATE_UNUSED)
      continue;
    struct scheduler_task_info ti = {
        .pid = t.pid,
        .state = t.state,
        .sleep_expiry = t.sleep_expiry,
        .time_used = t.time_used,
        .name = t.name,
    };

    DA_APPEND(si.tasks, ti);
  }

  __scheduler_unlock(flags);
  return si;
}

static int32_t scheduler_find_ready_task() {
  for (size_t i = 0; i < n_tasks; i++) {
    if (tasks[i].state == TASK_STATE_SLEEPING &&
        tasks[i].sleep_expiry <= time_since_boot) {
      tasks[i].state = TASK_STATE_READY;
      tasks[i].sleep_expiry = 0;
    }
  }

  uint64_t min_time_running = UINT64_MAX;
  int32_t ready_task = -1;
  for (size_t i = 0; i < n_tasks; i++) {
    struct task t = tasks[i];
    if (t.state != TASK_STATE_READY || t.last_running_time > min_time_running)
      continue;
    min_time_running = t.last_running_time;
    ready_task = i;
  }
  return ready_task;
}

static void update_time_used() {
  uint64_t elapsed = time_since_boot - last_time_count;
  last_time_count = time_since_boot;
  if (current_task > 0)
    tasks[current_task].time_used += elapsed;
  else
    idle_task.time_used += elapsed;
}

// intended for cooperative scheduling, not from timer interrupts
// must be called inside locked section.
void scheduler_block_task(enum task_state state) {
  ASSERT(current_task != -1);
  ASSERT(tasks[current_task].state == TASK_STATE_RUNNING);

  tasks[current_task].state = state;
  struct interrupt_frame **curr_frame_ptr = &(tasks[current_task].frame);

  int32_t next_idx = scheduler_find_ready_task();
  struct interrupt_frame *next_frame;
  if (next_idx < 0) {
    current_task = -1;
    next_frame = idle_task.frame;
  } else {
    current_task = next_idx;
    next_frame = tasks[next_idx].frame;
    tasks[next_idx].state = TASK_STATE_RUNNING;
  }

  scheduler_switch_to_task(curr_frame_ptr, next_frame);
}

void kexit() {
  scheduler_lock();
  ASSERT(current_task != -1);
  struct task t = tasks[current_task];
  tasks[current_task] = (struct task){0}; // TASK_STATE_UNUSED
  int32_t next_idx = scheduler_find_ready_task();

  struct interrupt_frame *next_frame;
  if (next_idx < 0) {
    current_task = -1;
    next_frame = idle_task.frame;
  } else {
    current_task = next_idx;
    next_frame = tasks[next_idx].frame;
    tasks[next_idx].state = TASK_STATE_RUNNING;
  }
  // TODO: free somehow. if freed here it insta page faults after
  // free pages (account for protection pages)
  // for (size_t i = 0; i < STACK_N_PAGES; i++) {
  //   vmm_free_page((size_t)t.stack_block + i * PAGE_SIZE);
  // }
  __start_task(next_frame);
  UNREACHABLE;
}

void nano_sleep_until(int64_t when) {
  uint32_t lock_flags = scheduler_lock();
  ASSERT(current_task != -1);
  if (when < time_since_boot)
    return;

  tasks[current_task].sleep_expiry = when;
  tasks[current_task].last_running_time = time_since_boot;
  scheduler_block_task(TASK_STATE_SLEEPING);
  scheduler_unlock(lock_flags);
}

void nano_sleep(int64_t nanos) { nano_sleep_until(time_since_boot + nanos); }
void milli_sleep(int64_t millis) { nano_sleep(millis * 1000000); }
void sleep(int64_t seconds) { milli_sleep(seconds * 1000); }

static struct task scheduler_build_kernel_task(void (*startingEIP)(void),
                                               const char *name) {
  size_t task_stack_end = (size_t)vmm_get_n_pages(RANGE_HEAP, STACK_N_PAGES);
  size_t *stack = (size_t *)(task_stack_end + STACK_N_PAGES * PAGE_SIZE);
  *--stack = 0x202;               // eflags
  *--stack = 0x8;                 // cs
  *--stack = (size_t)startingEIP; //
                                  // dummy regs:
  *--stack = 0xdeadface;          // edi
  *--stack = 0xdeadface;          // esi
  *--stack = 0xdeadface;          // ebp
  *--stack = 0xdeadface;          // esp
  *--stack = 0xdeadface;          // ebx
  *--stack = 0xdeadface;          // edx
  *--stack = 0xdeadface;          // ecx
  *--stack = 0xdeadface;          // eax

  return (struct task){
      .name = name,
      .frame = (struct interrupt_frame *)stack,
      .state = TASK_STATE_READY,
      .sleep_expiry = 0,
      .stack_block = (void *)task_stack_end,
      .time_used = 0,
  };
}

void scheduler_create_kernel_task(void (*startingEIP)(void), const char *name) {
  if (n_tasks >= MAX_TASKS)
    PANIC("too many tasks");

  struct task t = scheduler_build_kernel_task(startingEIP, name);
  t.pid = pid_counter++;

  for (size_t i = 0; i < n_tasks; i++) {
    if (tasks[i].state != TASK_STATE_UNUSED)
      continue;
    tasks[i] = t;
    return;
  }
  tasks[n_tasks++] = t;
}

struct interrupt_frame *
scheduler_PIT_IRQ_handler(struct interrupt_frame *frame) {
  time_since_boot += NANOS_PER_TICK;
#ifdef DISABLE_PREEMTION
  return frame;
#endif

  preempt_ticks++;
  if (preempt_ticks < MAX_PREEMPT_TICKS)
    return frame;

  // kprintf("preempt\n");

  preempt_ticks = 0;
  update_time_used();

  int32_t task_idx = scheduler_find_ready_task();
  if (task_idx < 0) {
    return frame;
  }

  if (current_task >= 0) {
    tasks[current_task].frame = frame;
    tasks[current_task].state = TASK_STATE_READY;
    tasks[current_task].last_running_time = time_since_boot;
  } else {
    idle_task.frame = frame;
  }

  current_task = task_idx;
  tasks[task_idx].state = TASK_STATE_RUNNING;
  return tasks[task_idx].frame;
}

static void idle() {
  for (;;) {
    HLT;
  }
}

void scheduler_init() {
  // also needs big stack because interrupts may run on the idle task stack
  idle_task = scheduler_build_kernel_task(&idle, "IDLE");

  uint32_t next_idx = scheduler_find_ready_task();
  ASSERT(next_idx != -1);
  struct interrupt_frame *next_frame = tasks[next_idx].frame;
  current_task = next_idx;
  tasks[next_idx].state = TASK_STATE_RUNNING;
  __start_task(next_frame);
  UNREACHABLE
}
