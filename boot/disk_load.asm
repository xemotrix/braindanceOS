; https://wiki.osdev.org/Disk_access_using_the_BIOS_(INT_13h)

; ARGUMENTS
; dl: boot drive
; dh: num sectors to read
; ax: starting sector
; bx: destination segment
disk_load:
    pusha

    mov WORD [starting_lba_addr], 0
    mov BYTE [num_sectors], 0
    mov WORD [destination_segment], 0

    mov WORD [starting_lba_addr], ax
    mov BYTE [num_sectors], dh
    mov WORD [destination_segment], bx

    ; check if lba is supported
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    jc no_lba

    mov si, disk_address_packet
    mov ah, 0x42
    ; dl is set
    int 0x13

    jc disk_error
    

disk_load_end:
    popa
    ret

disk_address_packet:
    db 0x10    ; packet size (16 bytes, 0x10)
    db 0       ; zero
    num_sectors:
    dw 0       ; number of sectors
    dw 0       ; destination offset
    destination_segment:
    dw 0       ; destination segment 0x10000
    starting_lba_addr:
    dq 0


disk_error:
    mov bx, DISK_ERROR_MSG
    call print_string
    jmp disk_load_end
    
no_lba:
    mov bx, NO_LBA_SUPPORTED
    call print_string
    jmp disk_load_end

DISK_ERROR_MSG:
    db "Error reading disk!",0

NO_LBA_SUPPORTED:
    db "Error: no LBA supported!:(",0

