

read elf sections: 
- readelf -S kernel.elf
- readelf -l kernel.elf

file system:
dd if=/dev/zero of=disk.img bs=1M count=64
mkfs.fat -F 32 disk.img

mkdir -p ./mnt
sudo mount -o loop,uid=$(id -u),gid=$(id -g),umask=022 disk.img ./mnt


