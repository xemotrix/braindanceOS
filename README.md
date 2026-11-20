# braindanceOS

Hobby 32-bit x86 kernel in C and assembly. Very early WIP.

So far: two-stage bootloader, paging, kernel heap, preemptive and cooperative multitasking, mutexes, ATA driver, FAT32.

## Run

Needs `i686-elf-gcc`, `nasm` and `qemu-system-i386`.

```sh
make disk.img
make run
```

Notes: [scheduler](docs/scheduler.md)
