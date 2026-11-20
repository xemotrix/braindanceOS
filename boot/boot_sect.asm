[bits 16]
[org 0x7c00]
SECOND_STG_N_DISK_SECTORS equ 8 ;

KERNEL_OFFSET equ 0x100000
    ; the BIOS puts the boot drive number in dl. we save it
    mov [BOOT_DRIVE], dl

    ; set up the stack
    mov bp, 0x9000
    mov sp, bp

    mov bx, MSG_LOAD_SND_STG
    call print_string

    mov dl, [BOOT_DRIVE]

    ; load second_stage
    mov dh, SECOND_STG_N_DISK_SECTORS ; number of disk sectors, must match the size of the kernel
    mov dl, [BOOT_DRIVE]
    mov ax, 1 ; start reading from second sector (skip boot_sect)
    mov bx, 0x1000 ; destination segment (*16)
    call disk_load

    mov bx, MSG_LOAD_SND_STG_MAYBE
    call print_string

    call memory_map ; do this here the jump fucks it up

    mov dl, [BOOT_DRIVE]
    jmp 0x1000:0x0

    jmp $

%include "boot/print_string.asm"
%include "boot/disk_load.asm"
%include "boot/memory_map.asm"

; global vars
BOOT_DRIVE: db 0
MSG_LOAD_SND_STG: db "Loading second stage into memory",13,10, 0
MSG_LOAD_SND_STG_MAYBE: db "Maybe Loaded second stage into memory", 13,10, 0

; padding
    times 510 - ($-$$) db 0
    dw 0xaa55

