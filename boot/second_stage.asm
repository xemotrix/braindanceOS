[bits 16]
[org 0x10000]
KERNEL_OFFSET equ 0x100000
KERNEL_N_DISK_SECTORS equ 100 ; number of disk sectors, must match the size of the kernel
SECOND_STG_N_DISK_SECTORS equ 8 ;

second_stage:
    mov ax, cs
    mov ds, ax
    mov [BOOT_DRIVE], dl

    mov bx, MSG_SND_STG
    call print_string

    call load_kernel
    call enable_a20
    call switch_to_pm

load_kernel:
    mov bx, MSG_LOAD_KERNEL
    call print_string

    mov dh, KERNEL_N_DISK_SECTORS ; number of disk sectors, must match the size of the kernel
    mov dl, [BOOT_DRIVE]
    mov ax, 1+SECOND_STG_N_DISK_SECTORS ; start reading from ninth sector (skip boot_sect + second_stage)
    mov bx, 0x1100 ; destination segment (ES:BX (ES*16+BX))
    call disk_load

    ret

enable_a20:
    mov ax, 0x2401
    int 0x15
    ret

switch_to_pm:
    cli ; clear (disable) interrupts

    ; load gdt_descriptor
    lgdt [gdt_descriptor]

    ; to switch to protected mode set the first bit of cr0 to 1
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp dword CODE_SEG:init_pm


%include "boot/disk_load.asm"
%include "boot/print_string_pm.asm"
%include "boot/print_string.asm"
%include "boot/gdt.asm"

[bits 32]
init_pm:
    ;update segment registers all to DATA_SEG
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;update stack
    mov ebp, 0x90000
    mov esp, ebp

    call begin_pm

begin_pm:
    mov ebx, MSG_PROT_MODE
    call print_string_pm

    ;copy kernel from 0x1100 to KERNEL_OFFSET
    cld 
    mov esi, 0x11000 ; es:bx
    mov edi, KERNEL_OFFSET
    mov ecx, KERNEL_N_DISK_SECTORS*512/4
    rep movsd

    call KERNEL_OFFSET
    
    jmp $

BOOT_DRIVE: db 0
MSG_PROT_MODE: db "Landed in 32-bit protected mode", 0
MSG_SND_STG: db "Reached second stage", 13,10,0
MSG_LOAD_KERNEL: db "Loading kernel...", 13,10,0

; padding
    times 4096 - ($-$$) db 0
