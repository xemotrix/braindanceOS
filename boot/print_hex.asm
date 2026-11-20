print_hex:
    pusha
    mov bx, HEX_OUT
    mov BYTE [bx], '0'
    inc bx
    mov BYTE [bx], 'x'
    inc bx

    mov cl, 16
start_loop:
    sub cl, 4
    mov si, dx
    shr si, cl
    and si, 0x000f
    add si, HEX_CHARS
    mov al, [si]
    mov BYTE [bx], al
    inc bx

    cmp cl, 0
    jne start_loop

    mov bx, HEX_OUT
    call print_string

    popa
    ret

HEX_CHARS:
    db "0123456789abcdef"
HEX_OUT:
    db "0x0000",0
