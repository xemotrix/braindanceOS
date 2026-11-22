[bits 32]
global __irq1
global __irq0
global __irq14
global __irq15
extern irq0_handler
extern irq1_handler
extern irq14_handler
extern irq15_handler

__irq0:
    pushad
    cld
    push esp                 ; interrupt_frame* argument
    call irq0_handler
    add esp, 4               ; remove arg from the stack
    mov esp, eax             ; point stack to returned interrupt_frame*
    popad                    ; restore registers (from returned interrupt_frame*)
    iretd

__irq1:
    pushad
    cld
    call irq1_handler
    popad
    iretd

__irq14:
    pushad
    cld
    push esp
    call irq14_handler
    add esp, 4
    mov esp, eax
    popad
    iretd

__irq15:
    pushad
    cld
    call irq15_handler
    popad
    iretd

