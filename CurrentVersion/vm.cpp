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
    // ASSIGNMENT 3 TO DO
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
    
    // ASSIGNMENT 3 TO DO

    return 0; // Remove me when doing the assignment
    
}

uint8_t *findBuffer(uint32_t pid, uint32_t numberOfPages, uint8_t perms)
{
    
    // ASSIGNMENT 3 TO DO

    return 0; // Remove me when doing the assignment
}


uint8_t *requestAvailablePage(uint32_t pid, uint8_t perms)
{
    
    // ASSIGNMENT 3 TO DO

    return 0; // Remove me when doing the assignment
    
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