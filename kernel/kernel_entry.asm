[bits 32]

global kernel_entry
[extern main]
[extern __bss_start]
[extern _end]

kernel_entry:
    ; clear .bss section
    mov edi, __bss_start
    mov ecx, _end
    sub ecx, edi
    xor eax, eax
    cld
    rep stosb

    call main
    jmp $
