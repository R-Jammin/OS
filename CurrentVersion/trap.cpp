// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "syscalls.h"
#include "kernel.h"
#include "screen.h"
#include "fs.h"
#include "x86.h"
#include "vm.h"
#include "libc-main.h"
#include "interrupts.h"
#include "constants.h"
#include "frame-allocator.h"
#include "exceptions.h"
#include "keyboard.h"
#include "file.h"
#include "trap.h"
#include "schedule.h"

void pageFault()
{
    //uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint32_t cr2Value;
    asm volatile ("movl %%eax, %0\n\t" : "=r" (cr2Value) : );

    insertKernelLog(*(uint32_t*)TOTAL_INTERRUPTS_COUNT_LOC,*(uint32_t*)RUNNING_PID_LOC,(uint8_t*)"PAGEFAULT", (uint32_t)cr2Value, (uint8_t*)"PAGE FAULT");

    systemExec((uint8_t*)"init", 50, 0, 0, 0);

}

void generalProtectionFault()
{
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    insertKernelLog(*(uint32_t*)TOTAL_INTERRUPTS_COUNT_LOC,currentPid,(uint8_t*)"GPFAULT", 0, (uint8_t*)"GENERAL PROTECTION FAULT");

    systemExec((uint8_t*)"init", 50, 0, 0, 0);

}

void generalFault()
{
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    insertKernelLog(*(uint32_t*)TOTAL_INTERRUPTS_COUNT_LOC,currentPid,(uint8_t*)"FAULT", 0, (uint8_t*)"GENERAL FAULT");

    systemExec((uint8_t*)"init", 50, 0, 0, 0);

}