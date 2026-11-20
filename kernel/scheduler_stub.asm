[bits 32]
global __switch_to_task
global __scheduler_lock
global __scheduler_unlock
global __start_task

; docs/scheduler.md

__switch_to_task:
    ; args:
    ; mov edx, [esp+8]    ; to frame *
    ; mov ecx, [esp+4]    ; from frame **
    ; mov eax, [esp]      ; ret ip

    ; build interrupt frame in the stack for next task
    pushad

    ; eflags where to_frame* was:
    mov eax, [esp+40]
    pushfd
    pop DWORD [esp+40] ; NOT +4, first pops, then calcs offset

    ; cs where from_frame** was:
    mov ebx, [esp+36]
    mov ecx, cs
    mov dword [esp+36], ecx

    ; overwrite the from_frame** to point to this frame we built
    mov [ebx], esp

    ; switch stack to the to_frame*:
    mov esp, eax
    popad
    iretd

__scheduler_lock:
    pushfd
    pop eax
    cli
    ret

__scheduler_unlock:
    mov eax, [esp+4]
    and eax, 0x200 ; IF flag
    jz __scheduler_unlock_end
    sti
__scheduler_unlock_end:
    ret
    
; only to be used when we don't care to loose the source task
__start_task:
    add esp, 4 ; remove return address from stack
    pop esp    ; point esp to argument (first task frame)
    popad
    iretd
