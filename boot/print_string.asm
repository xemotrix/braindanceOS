[bits 16]
print_string:
    pusha
    mov ah, 0x0e ; scrolling teletype BIOS routine
loop_start:
    mov al, [bx]
    cmp al, 0
    je loop_end
    int 0x10
    inc bx
    jmp loop_start
loop_end:
    popa
    ret
