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
#include "net.h"

uint32_t currentPid = 0;
uint32_t cursorRow = 0;
uint8_t *bufferMem = (uint8_t *)KEYBOARD_BUFFER;
uint8_t *cursorMemory = (uint8_t *)SHELL_CURSOR_POS;



void kInit()
{
    disableCursor();

    createSemaphore(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE, 1, 1);
    createSemaphore(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC, 1, 1);
    createSemaphore(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE, 1, 1);

    clearScreen();

    printString(COLOR_LIGHT_BLUE, cursorRow++, 0, (uint8_t *)"Starting Kernel Initialization:");

    // zero out process table memory, kernel heap and user heap
    fillMemory((uint8_t *)(PROCESS_TABLE_LOC) , (uint8_t)0x0, PAGE_SIZE);
    fillMemory((uint8_t *)(KERNEL_HEAP) , (uint8_t)0x0, KERNEL_HEAP_SIZE);
    fillMemory((uint8_t *)(USER_HEAP) , (uint8_t)0x0, HEAP_SIZE);

    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    readBlock(BlockGroupDescriptor->bgd_block_address_of_block_usage, (uint8_t *)EXT2_BLOCK_USAGE_MAP, true);
    readBlock(BlockGroupDescriptor->bgd_block_address_of_inode_usage, (uint8_t *)EXT2_INODE_USAGE_MAP, true);

    currentPid = initializeTask(currentPid, PROC_SLEEPING, STACK_START_LOC, (uint8_t *)"init", 100, ROOTDIR_INODE, 0, 0, 0);
    createPageFrameMap((uint8_t *)PAGEFRAME_MAP_BASE, PAGEFRAME_MAP_SIZE);

    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Initialized Task Struct -> PID: ");
    printHexNumber(COLOR_GREEN, (cursorRow - 1), 38, currentPid);

    initializePageTables(currentPid);
    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Initialized Page Directory and Page Table -> PID: ");
    printHexNumber(COLOR_GREEN, (cursorRow - 1), 56, currentPid);

    contextSwitch(currentPid);

    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Paging Enabled");

    remapPIC(INTERRUPT_MASK_ALL_ENABLED, INTERRUPT_MASK_ALL_ENABLED);
    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Programmable Interrupt Controller (PIC) Remapping Complete");

    //zero memory for the new IDT
    fillMemory((uint8_t *)INTERRUPT_DESC_TABLE, (uint8_t)0x0, PAGE_SIZE);
    fillMemory((uint8_t *)(INTERRUPT_DESC_TABLE + PAGE_SIZE), (uint8_t)0x0, PAGE_SIZE);
    loadIDT((uint8_t *)INTERRUPT_DESC_TABLE);
    loadIDTR((uint8_t *)INTERRUPT_DESC_TABLE, (uint8_t *)INTERRUPT_DESC_TABLE_REG);
    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Interrupt Descriptor Table (IDT) Setup Complete");

    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Kernel Initialization Complete");
    cursorRow++;

    createOpenFileTable((uint8_t *)GLOBAL_OBJECT_TABLE);

    storeValueAtMemLoc(RUNNING_PID_LOC, currentPid);

    enableInterrupts();

    //printLogo(20);

    printString(COLOR_LIGHT_BLUE, cursorRow++, 0, (uint8_t *)"Ready to load Init into PID:   ....");
    printHexNumber(COLOR_GREEN, (cursorRow - 1), 30, currentPid);

    // Store the address at this location so second_proc_start.asm can use it as a trampoline
    // to run code in the kernel.
    storeValueAtMemLoc((uint8_t *)SECOND_PROC_STARTUP_FUNC_LOC, (uint32_t)&secondProcInitialFunc);
    storeValueAtMemLoc((uint8_t *)AP_PID_WAIT_FLAG_LOC, 0xFF);
    startApplicationProcessor();

    // Initialize NIC
    netInit();

}

void logonPrompt()
{
    clearScreen();

    printString(COLOR_WHITE, 13, 28, (uint8_t*)"Welcome to ikanOS v5!");
    printString(COLOR_LIGHT_BLUE, 14, 28, (uint8_t*)"-----------------------");

    printString(COLOR_RED, 16, 22, (uint8_t*)"ONE KERNEL OBJECT TO RULE THEM ALL!");
    printString(COLOR_LIGHT_BLUE, 17, 22, (uint8_t*)"-----------------------------------");


    printString(COLOR_WHITE, 30, 23, (uint8_t*)"Please type the user to log in as");
    printString(COLOR_LIGHT_BLUE, 31, 23, (uint8_t*)"---------------------------------");

    //printLogo(1);

    printString(COLOR_LIGHT_BLUE, 44, 0, (uint8_t*)"Usr");
    readCommand(bufferMem, cursorMemory);

    strcpyRemoveNewline(LOGGED_IN_USER, COMMAND_BUFFER);

    *(uint32_t*)(LOGIN_COMPLETE) = 1;

}

void loadInit()
{
    if (!requestSpecificPage(currentPid, (uint8_t *)(STACK_PAGE - PAGE_SIZE), PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"kernel.cpp -> STACK_PAGE - PAGE_SIZE page request");
    }

    if (!requestSpecificPage(currentPid, (uint8_t *)(STACK_PAGE), PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"kernel.cpp -> STACK_PAGE page request");
    }

    for (uint32_t userHeapLoc = 0; userHeapLoc < (USER_HEAP_PAGES * PAGE_SIZE); userHeapLoc = userHeapLoc + PAGE_SIZE)
    {
        if (!requestSpecificPage(currentPid, (uint8_t*)(USER_HEAP + userHeapLoc), PG_USER_PRESENT_RW))
        {
            clearScreen();
            printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
            panic((uint8_t *)"kernel.cpp -> USER_HEAP page request");
        }
    }

    cursorRow++;
    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Loading binary to Temp File Storage");

    if (!requestSpecificPage(currentPid, USER_TEMP_INODE_LOC, PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"kernel.cpp -> USER_TEMP_INODE_LOC page request");
    }

    if (!fsFindFile((uint8_t *)"init", USER_TEMP_INODE_LOC, true, ROOTDIR_INODE))
    {
        panic((uint8_t *)"kernel.cpp -> Cannot find init in root directory");
    }

    struct inode *Inode = (struct inode*)USER_TEMP_INODE_LOC;
    uint32_t pagesNeedForTmpBinary = ceiling(Inode->i_size, PAGE_SIZE);

    //request pages for temporary file storage to load raw ELF file
    for (uint32_t tempFileLoc = 0; tempFileLoc < (pagesNeedForTmpBinary * PAGE_SIZE); tempFileLoc = tempFileLoc + PAGE_SIZE)
    {
        if (!requestSpecificPage(currentPid, (USER_TEMP_FILE_LOC + tempFileLoc), PG_USER_PRESENT_RW))
        {
            clearScreen();
            printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
            panic((uint8_t *)"kernel.cpp -> USER_TEMP_FILE_LOC page request");
        }
    }

    loadFileFromInodeStruct(USER_TEMP_INODE_LOC, USER_TEMP_FILE_LOC, true);
    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Raw binary loaded to 0x31000");

    struct elfHeader *ELFHeader = (struct elfHeader*)USER_TEMP_FILE_LOC;

    for (int x = 0; x < ELFHeader->e_phnum; x++)
    {
        struct pHeader *ProgramHeader = (struct pHeader*)((uint32_t)ELFHeader + ELFHeader->e_phoff + (ELFHeader->e_phentsize * x));

        if (ProgramHeader->p_type == PT_LOAD)
        {
            for (uint32_t tempFileLoc = 0; tempFileLoc < ((ceiling(ProgramHeader->p_memsz, PAGE_SIZE)) * PAGE_SIZE); tempFileLoc = tempFileLoc + PAGE_SIZE)
            {
                if (!requestSpecificPage(currentPid, (uint8_t *)(ProgramHeader->p_vaddr + tempFileLoc), PG_USER_PRESENT_RW))
                {
                    clearScreen();
                    printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
                    panic((uint8_t *)"kernel.cpp -> USERPROG TEXT and DATA VADDR page requests");
                }

            }

        }
    }

    loadElfFile(USER_TEMP_FILE_LOC);
    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Binary loaded from 0x31000 to desired location");
}

void launchInit()
{
    struct elfHeader *ELFHeaderLaunch = (struct elfHeader*)USER_TEMP_FILE_LOC;

    storeValueAtMemLoc(RUNNING_PID_LOC, currentPid);

    printString(COLOR_GREEN, cursorRow++, 0, (uint8_t *)"   -> Switching to Ring 3 and launching binary");

    // setting current process state to running
    updateTaskState(currentPid, PROC_RUNNING);
    setSystemTimer(SYSTEM_INTERRUPTS_PER_SECOND);

    loadTaskRegister(0x28);
    switchToRing3LaunchBinary((uint8_t *)ELFHeaderLaunch->e_entry);
}


void secondProcInitialFunc()
{
    uint32_t targetPid;
    uint32_t currentTickCount;

    while ((targetPid = readValueFromMemLoc((uint8_t *)AP_PID_WAIT_FLAG_LOC)) == 0xFF) {
        currentTickCount = *(uint32_t*)SECOND_PROC_TICK_COUNT_LOC;
        for (uint32_t x = 0; x < 4000000; x++) {}  // Your delay
        currentTickCount++;
        *(uint32_t*)SECOND_PROC_TICK_COUNT_LOC = currentTickCount;

    }

    struct task *TargetTask = (struct task*)(PROCESS_TABLE_LOC + (targetPid - 1) * TASK_STRUCT_SIZE);
    uint32_t targetPgd = TargetTask->pgd;
    uint32_t targetEip = TargetTask->eip;  // ELF entry

    asm volatile ("movl %0, %%cr3" : : "r" (targetPgd));

    storeValueAtMemLoc(RUNNING_PID_LOC, targetPid);
    updateTaskState(targetPid, PROC_RUNNING);
    storeValueAtMemLoc((uint8_t*)AP_PID_WAIT_FLAG_LOC, 0xFF);

    switchToRing3LaunchBinary((uint8_t *)targetEip);

    while(1) asm volatile("hlt");

}

void main()
{
    kInit();

    // Disabling logon prompt to save time while iterating through code.
    logonPrompt();

    insertKernelLog(0x1, currentPid, (uint8_t*)"LOGON",0 ,(uint8_t*)LOGGED_IN_USER);

    loadInit();
    launchInit();

    panic((uint8_t *)"kernel.cpp -> Unable to launch init, returned to kernel.cpp");
}
