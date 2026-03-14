# Copyright (c) 2023-2026 Dan O’Malley
# This file is licensed under the MIT License. See LICENSE for details.
# Initial version by Dan O'Malley, extended with Grok to support
# incremental builds
# 12/2025 with Grok v4.


NASM := nasm -f bin
CC := gcc
CFLAGS := -ggdb -m32 -fno-pie -ffreestanding -fno-stack-protector -Wunused-variable
LD := ld -m elf_i386 -e main

CPP_SOURCES := bootloader-stage2.cpp screen.cpp fs.cpp file.cpp vm.cpp keyboard.cpp trap.cpp interrupts.cpp syscalls.cpp kernel.cpp sh.cpp libc-main.cpp frame-allocator.cpp exceptions.cpp x86.cpp net.cpp al.cpp cc.cpp sound.cpp schedule.cpp myprog.cpp ex.cpp edlin.cpp adventure.cpp top.cpp dungeon.cpp filetst.cpp cat.cpp grep.cpp init.cpp src.cpp inputtst.cpp

ASM_SOURCES := bootloader-stage1.asm second_proc_start.asm

HEADER_SOURCES := $(wildcard *.h)

CODE_SUBFOLDER_FILES := $(wildcard code/*)
OTHER_SOURCES := Makefile genesis.txt vgafont.bin gdb_script.txt $(CODE_SUBFOLDER_FILES)

IMAGE_CODE_COPIES := $(patsubst code/%, image-source/code/%, $(CODE_SUBFOLDER_FILES))

COMMON_OBJS := screen.o fs.o vm.o libc-main.o frame-allocator.o exceptions.o x86.o
KEYB_OBJS := $(COMMON_OBJS) keyboard.o

BOOT_STAGE2_OBJS := $(COMMON_OBJS) bootloader-stage2.o

KERNEL_OBJS := syscalls.o interrupts.o trap.o keyboard.o fs.o screen.o vm.o libc-main.o frame-allocator.o exceptions.o file.o sound.o schedule.o x86.o net.o kernel.o

IMAGE_BINARIES := image-source/kernel image-source/sh image-source/bin/myprog image-source/bin/ex image-source/code/al image-source/code/cc image-source/code/edlin image-source/games/adventure image-source/bin/top image-source/games/dungeon image-source/bin/filetst image-source/code/libc.o image-source/bin/cat image-source/bin/grep image-source/init image-source/code/src image-source/bin/inputtst

IMAGE_COPIES := image-source/genesis.txt image-source/vgafont.bin $(IMAGE_CODE_COPIES)

ALL_OBJS := $(CPP_SOURCES:.cpp=.o)

.PHONY: default build clean debug-stage2 debug-kernel debug-shell debug-user debug qemu qemu-debug-stage2 qemu-debug-kernel qemu-debug-shell qemu-debug-user qemu-debug

default: build qemu

debug-stage2: build qemu-debug-stage2

debug-kernel: build qemu-debug-kernel

debug-shell: build qemu-debug-shell

debug-user: build qemu-debug-user

debug: build qemu-debug

build: SUBMIT-ME.zip fs.img
	find . -name '*.cpp' -o -name '*.h' -o -name '*.asm' | xargs wc -l

%.o: %.cpp $(HEADER_SOURCES)
	$(CC) $(CFLAGS) -c $< -o $@

%: %.asm
	$(NASM) $<

bootloader-stage2: $(BOOT_STAGE2_OBJS) keyboard.o
	$(LD) -Ttext 0x9000 $^ -o $@

image-source image-source/bin image-source/code image-source/games:
	mkdir -p $@

image-source/kernel: $(KERNEL_OBJS) | image-source
	$(LD) -Ttext 0x801000 $^ -o $@

image-source/init: $(KEYB_OBJS) init.o | image-source
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/sh: $(KEYB_OBJS) sh.o | image-source
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/bin/myprog: $(KEYB_OBJS) myprog.o | image-source/bin
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/bin/ex: $(KEYB_OBJS) ex.o | image-source
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/bin/cat: $(KEYB_OBJS) cat.o | image-source
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/bin/grep: $(KEYB_OBJS) grep.o | image-source
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/code/al: $(KEYB_OBJS) al.o | image-source/code
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/code/cc: $(KEYB_OBJS) cc.o | image-source/code
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/code/edlin: $(KEYB_OBJS) edlin.o | image-source
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/games/adventure: $(KEYB_OBJS) adventure.o | image-source/games
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/bin/top: $(KEYB_OBJS) top.o | image-source/bin
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/games/dungeon: $(KEYB_OBJS) dungeon.o | image-source/games
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/bin/filetst: $(KEYB_OBJS) filetst.o | image-source/bin
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/code/src: $(KEYB_OBJS) src.o | image-source/code
	$(LD) -Ttext 0x401000 $^ -o $@

image-source/bin/inputtst: $(KEYB_OBJS) inputtst.o | image-source/bin
	$(LD) -Ttext 0x401000 $^ -o $@

libc.o: libc-main.cpp
	g++ -c libc-main.cpp x86.cpp vm.cpp -m32 -ffreestanding -nostdlib -fno-rtti -fno-exceptions -fno-pie
	ld -m elf_i386 libc-main.o x86.o vm.o keyboard.o fs.o frame-allocator.o screen.o exceptions.o -T libc.ld -o libc.elf
	objcopy -O binary libc.elf libc.o
	rm libc.elf

image-source/genesis.txt: genesis.txt | image-source
	cp $< $@

image-source/vgafont.bin: vgafont.bin | image-source
	cp $< $@

image-source/code/libc.o: libc.o | image-source/code
	cp $< $@

image-source/code/%: code/% | image-source/code
	cp $< $@

tmp-ext2fs: $(IMAGE_BINARIES) $(IMAGE_COPIES)
	dd if=/dev/zero of=$@ bs=2K count=16000
	mkfs.ext2 $@ -I 128 -b 2K -d ./image-source/

fstmp.img: bootloader-stage1 bootloader-stage2 second_proc_start
	dd if=/dev/zero of=$@ bs=1 count=262144
	dd if=bootloader-stage1 of=$@ bs=1 seek=0 conv=notrunc
	dd if=bootloader-stage2 of=$@ bs=1 seek=512 conv=notrunc
	dd if=second_proc_start of=$@ bs=1 seek=253952 conv=notrunc

fs.img: fstmp.img tmp-ext2fs
	cat fstmp.img tmp-ext2fs > $@

SUBMIT-ME.zip: $(CPP_SOURCES) $(ASM_SOURCES) $(HEADER_SOURCES) $(OTHER_SOURCES)
	zip $@ *

qemu: fs.img
	qemu-system-i386 -smp 2 -drive file=fs.img,format=raw -monitor stdio -vga virtio -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -netdev user,id=net0,hostfwd=udp::12345-:12345 -device ne2k_isa,netdev=net0,iobase=0x300,irq=9 -object filter-dump,id=f1,netdev=net0,file=dump.pcap

qemu-debug-stage2: fs.img
	gnome-terminal -- bash -c "gdb ./bootloader-stage2 -x gdb_script.txt"
	qemu-system-i386 -smp 2 -drive file=fs.img,format=raw -monitor stdio -gdb tcp::9000 -S -vga virtio -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -netdev user,id=net0 -device ne2k_isa,netdev=net0,iobase=0x300,irq=9 -object filter-dump,id=f1,netdev=net0,file=dump.pcap

qemu-debug-kernel: fs.img
	gnome-terminal -- bash -c "gdb ./image-source/kernel -x gdb_script.txt"
	qemu-system-i386 -smp 2 -drive file=fs.img,format=raw -monitor stdio -gdb tcp::9000 -S -vga virtio -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -netdev user,id=net0 -device ne2k_isa,netdev=net0,iobase=0x300,irq=9 -object filter-dump,id=f1,netdev=net0,file=dump.pcap

qemu-debug-shell: fs.img
	gnome-terminal -- bash -c "gdb ./image-source/sh -x gdb_script.txt"
	qemu-system-i386 -smp 2 -drive file=fs.img,format=raw -monitor stdio -gdb tcp::9000 -S -vga virtio -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -netdev user,id=net0 -device ne2k_isa,netdev=net0,iobase=0x300,irq=9 -object filter-dump,id=f1,netdev=net0,file=dump.pcap

qemu-debug-user: fs.img
	gnome-terminal -- bash -c "gdb ./image-source/user -x gdb_script.txt"
	qemu-system-i386 -smp 2 -drive file=fs.img,format=raw -monitor stdio -gdb tcp::9000 -S -vga virtio -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -netdev user,id=net0 -device ne2k_isa,netdev=net0,iobase=0x300,irq=9 -object filter-dump,id=f1,netdev=net0,file=dump.pcap

qemu-debug: fs.img
	gnome-terminal
	qemu-system-i386 -smp 2 -drive file=fs.img,format=raw -monitor stdio -gdb tcp::9000 -S -vga virtio -audiodev pa,id=speaker -machine pcspk-audiodev=speaker -netdev user,id=net0 -device ne2k_isa,netdev=net0,iobase=0x300,irq=9 -object filter-dump,id=f1,netdev=net0,file=dump.pcap

CLEAN_FILES := \
	fs.img \
	bootloader-stage1 \
	second_proc_start \
	bootloader-stage2 \
	bootloader-stage2.o \
	md5.txt \
	screen.o \
	fs.o \
	kernel.o \
	vm.o \
	keyboard.o \
	interrupts.o \
	trap.o \
	syscalls.o \
	kernel \
	tmp-ext2fs \
	sh.o \
	log.txt \
	libc.o \
	frame-allocator.o \
	exceptions.o \
	ex.o \
	edlin.o \
	filetst.o \
	src.o \
	inputtst.o \
	adventure.o \
	dungeon.o \
	top.o \
	al.o \
	cc.o \
	cat.o \
	grep.o \
	x86.o \
	init.o \
	net.o \
	sound.o \
	file.o \
	schedule.o \
	fstmp.img \
	myprog.o \
	dump.pcap \
	SUBMIT-ME.zip \
	$(IMAGE_CODE_COPIES)

clean:
	rm -f $(CLEAN_FILES) $(ALL_OBJS)
	rm -f ./image-source/code/*
	rmdir ./image-source/code
	rm -f ./image-source/games/*
	rmdir ./image-source/games
	rm -f ./image-source/bin/*
	rmdir ./image-source/bin
	rm -f ./image-source/*
	rmdir ./image-source 2>/dev/null || true