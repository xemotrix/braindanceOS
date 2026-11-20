#!/usr/bin/bash

# https://wiki.osdev.org/GCC_Cross-Compiler

sudo apt update

sudo apt install build-essential bison \
    flex libgmp3-dev libmpc-dev libmpfr-dev \
    texinfo libisl-dev
