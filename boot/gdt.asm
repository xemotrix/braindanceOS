gdt_start:

gdt_null:
    dd 0
    dd 0

gdt_code:
    dw 0xffff     ; Limit (bits 0-15)
    dw 0x0        ; Base (bits 0-15)
    db 0x0        ; Base (bits 16-23)
    db 10011010b  ; 1st flags, type flags
                  ; 1st: (present)1 (privilege)00 (descriptor type)1 -> 1001b
                  ; type: (code)1 (conforming)0 (readable)1 (accessed)0 -> 1010b
    db 11001111b  ; 2nd flags, Limit (bits 16-19)
                  ; (granularity)1 (32bits default)1 (64bit seg)0 (AVL)0 -> 1100b
    db 0x0        ; Base (bits 24-31)

gdt_data:
    ; same as code but the type flags
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b ; 1st: same
                 ; type: (code)0 (conforming)0 (readable)1 (accessed)0 -> 0010b
    db 11001111b
    db 0x0
    
gdt_end:


gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; gdt size -1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
