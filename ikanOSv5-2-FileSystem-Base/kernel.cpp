// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "kernel.h"
#include "screen.h"
#include "fs.h"
#include "x86.h"
#include "vm.h"
#include "keyboard.h"
#include "interrupts.h"
#include "syscalls.h"
#include "libc-main.h"
#include "constants.h"
#include "frame-allocator.h"
#include "exceptions.h"
#include "file.h"



void main()
{
    clearScreen();
    printString(COLOR_WHITE, 8, 5, (uint8_t *)"Welcome to the kernel!");

    while (true) {}
}
