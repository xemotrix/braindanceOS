[bits 32]
global __isr0
global __isr14
extern isr0_handler
extern isr14_handler

; division by 0
__isr0:
    ;save general purpose registers
    pushad
    ; ensure direction flag is clear for c code
    cld
    ; call our c handler
    call isr0_handler
    ;restore registers
    popad
    ;return from interrupt
    iretd

; page fault
__isr14:
    pushad
    cld
    call isr14_handler
    popad
    iretd
