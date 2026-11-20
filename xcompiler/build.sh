#!/usr/bin/bash

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
# fish_add_path

# drwxr-xr-x chema chema 4.0 KB Mon Nov  9 11:58:34 2026  gcc-16.2.0
# drwxrwxr-x chema chema 4.0 KB Mon Nov  9 11:58:17 2026  binutils-2.47

BINUTILS_VERSION=binutils-2.47
GCC_VERSION=gcc-16.2.0

# binutils
#where binutils and gcc source must be downloaded
cd $HOME/src
mkdir build-binutils
cd build-binutils

../$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" \
    --with-sysroot --disable-nls --disable-werror --enable-default-execstack=no

make 
make install

# GCC
cd $HOME/src
which -- $TARGET-as || echo $TARGET-as not found in PATH

mkdir build-gcc
cd build-gcc
../$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls \
    --enable-languages=c --without-headers --enable-initfini-array

make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
