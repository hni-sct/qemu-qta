#!/usr/bin/env bash

# WARNING: Always run script inside this folder or fix relative paths first!

init() {
  git submodule update --init
	rm -f libqta.so
}

qemu_setup() {
	echo '|  --> fetched qemu source code                                |'
	echo '+--------------------------------------------------------------+'
	git checkout master
	git reset --hard
	git clean -fd
	echo '|  --> checked out latest QEMU changes (master)                |'
	patch -p1 < ../../qemu-fix-llvm-exported-symbols.patch &> /dev/null || exit 1
	echo '|  --> applied patch "qemu-fix-llvm-exported-symbols.patch"    |'
	cd ../bin/
	../src/configure --target-list=arm-softmmu,riscv32-softmmu --enable-plugins --enable-libxml2 || exit 1
	echo '|  --> configured qemu source code                             |'
	echo '+--------------------------------------------------------------+'
}

qemu_compile() {
	make || exit 1
	echo '|  --> compilation finished                                    |'
	echo '+--------------------------------------------------------------+'
}

# Remove previous build files
echo '+-------------------------------------+                         '
echo '| STEP 1 / 2: Download & Compile QEMU |                         '
echo '+-------------------------------------+------------------------+'
echo '|  ---> getting the QEMU source tree...                        |'
init
mkdir -p qemu/bin
mkdir -p qemu/src
cd qemu/src
qemu_setup
qemu_compile
cd ../..
echo '|  --> DONE                                                    |'
echo '+--------------------------------------------------------------+'
echo ''

# Simulate all tutorial programs and verify the result against ref.
echo '+--------------------------------+                              '
echo '| STEP 2 / 2: Compile QTA plugin |                              '
echo '+--------------------------------+-----------------------------+'
make libqta.so
echo '|  --> DONE                                                    |'
echo '+--------------------------------------------------------------+'
echo ''
