# Scheduler
The scheduler manages context switches so several programs/tasks can run at once.

A context switch can be triggered in two ways:
- **cooperative**: a process/task tells the scheduler it yields the CPU to any
  other available task, the scheduler chooses which one and swiches to it. This
  could be a call to sleep(), or any operation that should trigger a switch
  like sleeps or syscalls for example.
- **preemptive**: the scheduler defines a max time for tasks to run
  continously, if reached, it tries to find another task to run and switches to
  it.


## Preemptive 

Via the timer irq `irq0`, we build a `interrupt_frame` in the stack, call a 
function that will return another frame, and use the other frame to jump to
a new task:

1. When entering the interrupt handler `irq0` the CPU has already left in the
   stack the first 3 fields: `eflags`, `CS` and the `IP` pointing to where we
   were before the interrupt (where we will want to jump to switch to that
   task).
2. Via `pushad` we save the registers also to the stack. Note that esp is also
   saved but it doesn't matter, we don't need that value (reasons later).
3. At this point we have the `interrupt_frame` struct built in the stack so
   `esp` could be treated as a `interrupt_frame *`. So we push `esp` to set
   that up as an argument and call the C function `irq0_handler`.
4. That function will do whatever (explained later) it must, but at the end it
   will return another `interrupt_frame *`, leaving it on `eax`. (It could be
   the same pointer if we don't want to switch tasks).
5. We point `esp` to that returned pointer, and undo the `pushad` with `popad`.
   `popad` doesn't overrwite `esp`, so it doesn't matter what was there before,
   the stack pointer is only increased (stack goes downwards) by what we poped.
   At this point we have reseted the state from a previous task and effectively
   switched contexts: the registers are as they were when we left that task,
   and the stack is alsos the same, except it has the `eflags`, `CS` and `IP`
   that were pushed when this task was left. 
6. `iret` finalizes the switch, popping these 3 values, restoring `eflags` and
   `CS` and jumping to the `IP`.


Basically, every not-running task has the interrupt_frame in the end of it's
stack, when we want to switch we build a frame in the running task stack,
switch stacks and undo

## Cooperative

Any form of cooperative task switching calls `__switch_to_task` (after appropiately
modifying any task state)

After the call from a C function, `__switch_to_task` has in the stack:

```
esp+8: to interrupt_frame*
esp+4: from interrupt_frame**
esp+0: return ip
```

- `to` is a pointer to the target task frame.
- `from` is a pointer to the current task's frame pointer, we'll want to create a new frame
in the stack and switch the pointer.
- return ip is left there by the `call`

This is (conveniently) structurally similar to the end of an `interrupt_frame`. We'll only need
to put `eflags` where the `to` is and `cs` where the `from` is. The return ip is alredy were we
want it.

Then we pushad, filling the rest of the `interrupt_frame`, leaving the stack like this:
```
esp+40: to frame*
esp+36: from frame**
esp+32: ret ip*
esp+28: eax
esp+24: ecx
esp+20: edx
esp+16: ebx
esp+12: esp
esp+08: ebp
esp+04: esi
esp+00: edi
```

To convert it into a proper interrupt_frame we only need to substitute the `to
frame*` and `from frame*` to eflags and CS respectively. As we already saved gp
registers we can save the input arguments in any register.

Once done, we overrwite the from pointer with `esp`, and similarly to the preemptive
case, we point the stack to the `to`, `popad` and `iret`.

## Race conditions
There is a chance for race conditions between cooperative and preemptive task switches.
For example, while task is going to sleep, it modifies the tasks state (it's own state, 
searches for a ready task to run, etc.). An interrupt could happen in between, and mess
up everything.

While the kernel is single CPU (not SMP), the only thing we need to avoid races is to 
disable interrupts while we modify task state from inside any task. The interrupt handler
already disables interrupts on enter and reenables them on return (by restoring `eflags`, 
where the `IF` enables or disables interrupts).

For that there are two functions: `__scheduler_lock` and `__scheduler_unlock`. The first one
just returns the current eflags and disables interrupts (`cli`). The second one takes flags
as argument, and if the `IF` flag was clear, disables interrupts, if not, just returns.

This way, in sleep for example we do somehting like:
```c
void sleep_until(t) {
    uint32_t flags = __scheduler_lock();
    // modify tasks state
    switch_to_task(from, to);
    __scheduler_unlock(flags);
}
```
The thing is that when we `switch_to_task` two things happen:
- The current tasks frame is saved for when we switch back (including the `eflags` with
interrupts disabled by `__scheduler_lock`.
- The next tasks frame `eflags` are restored (re-enabling interrupts if the task we are switching
to had them enabled).

When we switch back to this task that was put to sleep, it's `eflags` will be restored (interrupts
will be disabled) but just after, in `__scheduler_unlock` they'll be reenabled (if they were enabled
when calling `__scheduler_lock`)


