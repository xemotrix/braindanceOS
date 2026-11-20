ENTRY (kernel_entry)

SECTIONS
{
    . = 1M;
    __kernel_start = .;

    .text ALIGN(4K) :
    {
        *(.text)
    }
    .rodata ALIGN(4K) :
    {
        *(.rodata)
    }
    .data ALIGN(4K) :
    {
        *(.data)
    }
    .bss ALIGN(4K) :
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        _end = .;
    }
    __kernel_end = .;
}
