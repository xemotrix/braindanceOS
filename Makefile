C_SOURCES = $(wildcard kernel/*.c drivers/*.c utils/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h utils/*.h)
ASM_BOOT =$(wildcard boot/*.asm)

ASM_STUB = $(wildcard kernel/*.asm)
OBJ = ${ASM_STUB:.asm=.o} ${C_SOURCES:.c=.o}

CFLAGS = -g -O0 -ffreestanding -std=c23 -isystem libc/include -I.

LIBK_SRC = $(wildcard libc/*/*.c)
LIBK_OBJ = $(LIBK_SRC:.c=.libk.o)

all: os-image

debug: all 
	qemu-system-i386 -m 256M \
		-display gtk,zoom-to-fit=on \
		-drive format=raw,file=os-image \
		-drive format=raw,file=disk.img \
		-s -S

run: all 
	qemu-system-i386 -m 256M \
		-display gtk,zoom-to-fit=on \
		-drive format=raw,file=os-image \
		-drive format=raw,file=disk.img 

os-image: boot/boot_sect.bin boot/second_stage.bin kernel.bin
	cat $^ > os-image 
	@# 10752 = 512(boot sector) + 512*50 (50 sectors read for the kernel)
	@# the 50 is configured in boot_sect.asm
	truncate -s 51712 os-image

# $^ is all dependencies
kernel.bin: kernel.elf
	i686-elf-objcopy -O binary $< $@

kernel.elf: kernel/kernel_entry.o ${OBJ} libk.a
	i686-elf-ld -o kernel.elf -T kernel/linker.ld $^ 

%.o: %.c ${HEADERS}
	i686-elf-gcc $(CFLAGS) -c $< -o $@

%.libk.o: %.c
	i686-elf-gcc $(CFLAGS) -D__is_libk -c $< -o $@

libk.a: $(LIBK_OBJ)
	i686-elf-ar rcs $@ $^

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm $(ASM_BOOT)
	nasm $< -f bin -o $@

clean:
	@rm -rf *.elf *.dis os-image
	@rm -rf */*.o */*.bin

kernel.dis: kernel.bin
	ndisasm -b 32 $< > $@

os.dis: os-image
	ndisasm -b 32 $< > $@

gdb:
	gdb -ex "target remote localhost:1234" \
		kernel.elf

disk.img:
	dd if=/dev/zero of=disk.img bs=1M count=64
	mkfs.fat -F 32 disk.img

mount_dir:
	@mkdir -p ./mnt

mount: disk.img mount_dir
	@sudo mount \
		-o loop,uid=$$(id -u),gid=$$(id -g),umask=022 \
		disk.img ./mnt

umount: 
	@sudo umount ./mnt

compile-commands.json: clean
	bear -- make
