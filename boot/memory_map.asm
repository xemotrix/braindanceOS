[bits 16]
E820_BUFFER      equ 0x7500
E820_ENTRY_SIZE  equ 24
E820_MAX_ENTRIES equ 32
E820_MAGIC       equ 0x534D4150

MMAP_ENTRY_COUNT_ADDR equ 0x74FC
; https://wiki.osdev.org/Memory_Map_(x86)

memory_map:
    push es
    pushad

    xor ax, ax
    mov es, ax
    mov di, E820_BUFFER

    xor ebx, ebx
    mov dword [MMAP_ENTRY_COUNT_ADDR], 0

mmap_next:
    mov edx, E820_MAGIC
    mov eax, 0xE820
    mov ecx, 24 ; ask for 24 bytes

    mov [es:di + 20], dword 1 ; init extended attributes

    int 0x15
    jc mmap_failed

    cmp eax, E820_MAGIC
    jne mmap_failed

    cmp ecx, 20
    jne mmap_failed
    inc dword [MMAP_ENTRY_COUNT_ADDR]

    add di, E820_ENTRY_SIZE

    ;ebx == 0 -> no more entries
    test ebx, ebx
    jz mmap_end

    ; don't exceed buffer cap
    cmp dword [MMAP_ENTRY_COUNT_ADDR], E820_MAX_ENTRIES
    jae mmap_failed

    jmp mmap_next

mmap_end:
    popad
    pop es
    ret

mmap_failed:
    mov bx, MMAP_ERROR_MSG
    call print_string
    jmp mmap_end

MMAP_ERROR_MSG:
    db "Error doing mmap",0

