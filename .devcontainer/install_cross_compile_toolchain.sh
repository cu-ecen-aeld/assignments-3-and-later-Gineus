#!/bin/bash

ARM_CROSS_COMPILE_DIRECTORY=~/arm-cross-compiler

if [ ! -d $ARM_CROSS_COMPILE_DIRECTORY ]; then
    mkdir $ARM_CROSS_COMPILE_DIRECTORY
fi

cd $ARM_CROSS_COMPILE_DIRECTORY

ARM_TOOLCHAIN_DIRECTORY=arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu

rm -rf $ARM_TOOLCHAIN_DIRECTORY

if [ ! -d $ARM_TOOLCHAIN_DIRECTORY ]; then
    wget https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
    tar xJf arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
    rm arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu.tar.xz
    echo "export PATH=\$PATH:$ARM_CROSS_COMPILE_DIRECTORY/$ARM_TOOLCHAIN_DIRECTORY/bin" >> ~/.bashrc
    . ~/.bashrc
fi