// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "vm.h"
#include "fs.h"
#include "constants.h"
#include "libc-main.h"
#include "frame-allocator.h"
#include "exceptions.h"
#include "file.h"
#include "screen.h"


void initializePageTables(uint32_t pid)
{
    //compute position of tables based on pid

    //get pointers to pdir base and ptable base
    uint32_t pageDirLocation = PAGE_DIR_BASE + ((pid - 1) * MAX_PGTABLES_SIZE);
    uint32_t firstPageTableLocation = pageDirLocation + PAGE_SIZE;

    //zero mem

    //first entry = pageuser present rw

    //keybord RDWRITE
    //pidinfo is ro

    uint32_t *pageDirectory = (uint32_t *)pageDirLocation;
    uint32_t *pageTables = (uint32_t *)firstPageTableLocation;
    uint32_t *lapicPageTable = (uint32_t *)(firstPageTableLocation + (PAGE_SIZE * 4));

    fillMemory((uint8_t *)pageDirLocation, 0x0, PAGE_SIZE);
    fillMemory((uint8_t *)firstPageTableLocation, 0x0, (PAGE_SIZE * 5));

    pageDirectory[0] = firstPageTableLocation | PG_USER_PRESENT_RW;
    pageDirectory[1] = (firstPageTableLocation + PAGE_SIZE) | PG_USER_PRESENT_RW;
    pageDirectory[2] = (firstPageTableLocation + (PAGE_SIZE * 2)) | PG_USER_PRESENT_RW;
    pageDirectory[3] = (firstPageTableLocation + (PAGE_SIZE * 3)) | PG_USER_PRESENT_RW;
    pageDirectory[1019] = (firstPageTableLocation + (PAGE_SIZE * 4)) | PG_KERNEL_PRESENT_RW;

    pageTables[0] = 0x0 | PG_USER_PRESENT_RW;
    pageTables[KEYBOARD_BUFFER / PAGE_SIZE] = KEYBOARD_BUFFER | PG_USER_PRESENT_RW;
    pageTables[USER_PID_INFO / PAGE_SIZE] = USER_PID_INFO | PG_USER_PRESENT_RO;
    pageTables[STDERR_BUFFER / PAGE_SIZE] = STDERR_BUFFER | PG_USER_PRESENT_RO;
    pageTables[SECOND_PROC_TMP_STACK / PAGE_SIZE] = SECOND_PROC_TMP_STACK | PG_KERNEL_PRESENT_RW;
    pageTables[SECOND_PROC_SIPI_CODE / PAGE_SIZE] = SECOND_PROC_SIPI_CODE | PG_KERNEL_PRESENT_RW;
    pageTables[GDT_LOC / PAGE_SIZE] = GDT_LOC | PG_USER_PRESENT_RO;

    pageTables[((uint32_t)VIDEO_RAM) / PAGE_SIZE] = ((uint32_t)VIDEO_RAM) | PG_USER_PRESENT_RW;
    pageTables[(((uint32_t)VIDEO_RAM) / PAGE_SIZE) + 1] = (((uint32_t)VIDEO_RAM) + PAGE_SIZE) | PG_USER_PRESENT_RW;

    for (uint32_t addr = KERNEL_BASE; addr < KERNEL_LIMIT; addr += PAGE_SIZE)
    {
        pageTables[addr / PAGE_SIZE] = addr | PG_KERNEL_PRESENT_RW;
    }

    for (uint32_t i = 0; i < ENTRIES_PER_PAGE_TABLE; i++)
    {
        lapicPageTable[i] = (LAPIC_PAGE + (i * PAGE_SIZE)) | PG_KERNEL_PRESENT_RW;
    }
}


void fillMemory(uint8_t *memLocation, uint8_t byteToFill, uint32_t numberOfBytes)
{
    for (uint32_t currentByte = 0; currentByte < numberOfBytes; currentByte++)
    {
        *memLocation = byteToFill;
        memLocation++;
    }
}


void contextSwitch(uint32_t pid)
{
    uint32_t pgdLocation = ((pid - 1) * MAX_PGTABLES_SIZE) + PAGE_DIR_BASE;
    
    asm volatile ("movl %0, %%eax\n\t" : : "r" (pgdLocation));
    asm volatile ("movl %eax, %cr3\n\t");
    asm volatile ("movl %cr0, %ebx\n\t");
    asm volatile ("or $0x80000000, %ebx\n\t");
    asm volatile ("movl %ebx, %cr0\n\t");
}

uint32_t initializeTask(uint32_t ppid, uint16_t state, uint32_t stack, uint8_t *binaryName, uint32_t priority, uint32_t directoryInode, uint32_t requestedStdIn, uint32_t requestedStdOut, uint32_t requestedStdErr)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
    
    uint32_t lastUsedPid = 0;
    uint32_t nextAvailPid = 0;
    uint32_t taskStructNumber = 0;

    if (ppid == 0) { ppid = KERNEL_OWNED; }

    while (nextAvailPid == 0 && taskStructNumber < MAX_PROCESSES)
    {
        lastUsedPid = *(uint32_t *)(PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * taskStructNumber));

        if ((unsigned int)lastUsedPid == 0)
        {
            nextAvailPid = (taskStructNumber + 1);
        }

        if (taskStructNumber == (MAX_PROCESSES - 1))
        {
            while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
            panic((uint8_t *)"vm.cpp:initializeTask() -> reached max process number");
        }

        taskStructNumber++;
    }
    
    uint32_t physMemStart = (MAX_PROCESS_SIZE * (nextAvailPid - 1));
    uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (nextAvailPid - 1));
    uint32_t pgd = ((nextAvailPid - 1) * MAX_PGTABLES_SIZE) + PAGE_DIR_BASE;
    
    struct task *Task = (struct task*)taskStructLocation;

    Task->pid = nextAvailPid;
    Task->ppid = ppid;
    Task->state = state;
    Task->pgd = pgd;
    Task->stack = stack;
    Task->physMemStart = physMemStart;

    if (requestedStdIn == 0)
    {
        Task->fileDescriptor[0] = (globalObjectTableEntry *)(GLOBAL_OBJECT_TABLE); // STDIN
    }
    else
    {
        Task->fileDescriptor[0] = (globalObjectTableEntry *)requestedStdIn;
    }

    if (requestedStdOut == 0)
    {
        Task->fileDescriptor[1] = (globalObjectTableEntry *)(GLOBAL_OBJECT_TABLE + (sizeof(globalObjectTableEntry))); //STDOUT
    }
    else
    {
        Task->fileDescriptor[1] = (globalObjectTableEntry *)requestedStdOut;
    }

    if (requestedStdErr == 0)
    {
        Task->fileDescriptor[2] = (globalObjectTableEntry *)(GLOBAL_OBJECT_TABLE + (sizeof(globalObjectTableEntry) * 2)); //STDERR
    }
    else
    {
        Task->fileDescriptor[2] = (globalObjectTableEntry *)requestedStdErr; 
    }
    
    Task->nextAvailableFileDescriptor = 3;
    Task->priority = priority;
    Task->runtime = 0;
    Task->binaryName = binaryName;
    Task->currentDirectoryInode = directoryInode;

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}

    return nextAvailPid;
}

void updateTaskState(uint32_t pid, uint16_t state)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
    
    uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (pid - 1));
    
    struct task *Task = (struct task*)taskStructLocation;
    Task->state = state;

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
}


uint32_t requestSpecificPage(uint32_t pid, uint8_t *pageMemoryLocation, uint8_t perms)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}

    uint32_t frameNumber = allocateFrame(pid, (uint8_t *)PAGEFRAME_MAP_BASE);
    if (frameNumber == 0)
    {
        while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
        return 0;
    }

    uint32_t ptLocation = ((pid - 1) * MAX_PGTABLES_SIZE) + PAGE_TABLE_BASE;
    uint32_t pageNumber = ((uint32_t)pageMemoryLocation / PAGE_SIZE);
    uint32_t *pageEntry = (uint32_t *)(ptLocation + (pageNumber * 4));

    if (*pageEntry != 0x0)
    {
        freeFrame(frameNumber);
        while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
        return 0;
    }

    *pageEntry = (frameNumber * PAGE_SIZE) | perms;

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
    return 1;
}

uint8_t *findBuffer(uint32_t pid, uint32_t numberOfPages, uint8_t perms)
{
    (void)perms;

    uint32_t ptLocation = ((pid - 1) * MAX_PGTABLES_SIZE) + PAGE_TABLE_BASE;
    uint32_t startPage = TEMP_FILE_START_LOC / PAGE_SIZE;
    uint32_t endPage = USER_SPACE_LIMIT / PAGE_SIZE;

    uint32_t contiguousPages = 0;
    uint32_t firstPage = 0;

    for (uint32_t currentPage = startPage; currentPage <= endPage; currentPage++)
    {
        uint32_t pageEntry = *(uint32_t *)(ptLocation + (currentPage * 4));

        if (pageEntry == 0x0)
        {
            if (contiguousPages == 0)
            {
                firstPage = currentPage;
            }

            contiguousPages++;

            if (contiguousPages == numberOfPages)
            {
                return (uint8_t *)(firstPage * PAGE_SIZE);
            }
        }
        else
        {
            contiguousPages = 0;
        }
    }

    return 0;
}

uint8_t *requestAvailablePage(uint32_t pid, uint8_t perms)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}

    uint32_t frameNumber = allocateFrame(pid, (uint8_t *)PAGEFRAME_MAP_BASE);
    if (frameNumber == 0)
    {
        while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
        return 0;
    }

    uint32_t ptLocation = ((pid - 1) * MAX_PGTABLES_SIZE) + PAGE_TABLE_BASE;
    uint32_t startPage = TEMP_FILE_START_LOC / PAGE_SIZE;
    uint32_t endPage = USER_SPACE_LIMIT / PAGE_SIZE;

    for (uint32_t currentPage = startPage; currentPage <= endPage; currentPage++)
    {
        uint32_t *pageEntry = (uint32_t *)(ptLocation + (currentPage * 4));

        if (*pageEntry == 0x0)
        {
            *pageEntry = (frameNumber * PAGE_SIZE) | perms;

            while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
            return (uint8_t *)(currentPage * PAGE_SIZE);
        }
    }

    freeFrame(frameNumber);
    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
    return 0;
}

void freePage(uint32_t pid, uint8_t *pageToFree)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}

    uint32_t ptLocation = ((pid - 1) * MAX_PGTABLES_SIZE) + PAGE_TABLE_BASE;
    uint32_t pageNumberToFree = (uint32_t)pageToFree / PAGE_SIZE;
    uint32_t physicalAddressToFree = *(uint32_t *)((int)ptLocation + (pageNumberToFree * 4));

    freeFrame((physicalAddressToFree / PAGE_SIZE));
    fillMemory(pageToFree, 0x0, PAGE_SIZE);
    *(uint32_t *)((uint32_t)ptLocation + (pageNumberToFree * 4)) = 0x0;

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PROCESS_TABLE_LOC)) {}
}

bool acquireLock(uint32_t currentPid, uint8_t *memoryLocation)
{
    uint32_t semaphoreNumber = 0;
    struct semaphore *Semaphore = (struct semaphore*)KERNEL_SEMAPHORE_TABLE;

    while (semaphoreNumber < MAX_SEMAPHORE_OBJECTS)
    {
        // This allows multiple pids to lock same userspace address since userspace is not identity mapped
        // Kernel space is identity mapped so multiple pids cannot lock the same memory address
        if ((Semaphore->pid == currentPid && Semaphore->memoryLocationToLock == memoryLocation && (uint32_t)memoryLocation < 0x800000) || (Semaphore->memoryLocationToLock == memoryLocation && (uint32_t)memoryLocation >= 0x800000))
        {
            if (Semaphore->currentValue == 0)
            {
                return false; // Cannot acquire lock
            }
            
            asm volatile ("movl %0, %%ecx\n\t" : : "r" (Semaphore->currentValue - 1));
            asm volatile ("movl %0, %%edx\n\t" : : "r" (&Semaphore->currentValue));
            asm volatile ("xchg %ecx, (%edx)\n\t");
            return true;
        }

        semaphoreNumber++;
        Semaphore++;
    }

    return false;
}

bool releaseLock(uint32_t currentPid, uint8_t *memoryLocation)
{
    uint32_t semaphoreNumber = 0;
    struct semaphore *Semaphore = (struct semaphore*)KERNEL_SEMAPHORE_TABLE;

    while (semaphoreNumber < MAX_SEMAPHORE_OBJECTS)
    {
        if (Semaphore->pid == currentPid && Semaphore->memoryLocationToLock == memoryLocation)
        {
            if (Semaphore->currentValue == Semaphore->maxValue)
            {
                return false; // At max amount
            }
            
            asm volatile ("movl %0, %%ecx\n\t" : : "r" (Semaphore->currentValue + 1));
            asm volatile ("movl %0, %%edx\n\t" : : "r" (&Semaphore->currentValue));
            asm volatile ("xchg %ecx, (%edx)\n\t");
            return true;
        }

        semaphoreNumber++;
        Semaphore++;
    }

    return false;
}

bool createSemaphore(uint32_t currentPid, uint8_t *memoryLocation, uint32_t currentValue, uint32_t maxValue)
{
    uint32_t semaphoreNumber = 0;
    struct semaphore *Semaphore = (struct semaphore*)KERNEL_SEMAPHORE_TABLE;

    while (semaphoreNumber < MAX_SEMAPHORE_OBJECTS)
    {
        if (Semaphore->pid == 0)
        {
            Semaphore->pid = currentPid;
            Semaphore->memoryLocationToLock = memoryLocation;
            Semaphore->currentValue = currentValue;
            Semaphore->maxValue = maxValue;
            return true;  
        }

        semaphoreNumber++;
        Semaphore++;
    }
    // return false if unable to create
    return false;
}