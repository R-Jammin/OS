// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Most functions written by Dan O'Malley. 
// Grok wrote some and extended others. Noted in-line below.
// 12/2025 with Grok v4.


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
#include "schedule.h"
#include "sound.h"
#include "net.h"


uint32_t returnedArgument = 0;
uint32_t totalInterruptCount = 0;
uint32_t systemTimerInterruptCount = 0;
uint32_t keyboardInterruptCount = 0;
uint32_t otherInterruptCount = 0;
uint32_t currentFileDescriptor = 0;
uint32_t uptimeSeconds = 0;
uint32_t uptimeMinutes = 0;
struct task *SysHandlerTask;
struct task *newSysHandlerTask;
uint32_t sysHandlertaskStructLocation;
uint32_t newSysHandlertaskStructLocation;
uint32_t returnedPid;
uint32_t returnedValueFromSyscallFunction;
bool cachingEnabled = true;


void sysSound(struct soundParameter *SoundParameter)
{
    // Dan O'Malley

    generateTone(SoundParameter->frequency);

    for (uint32_t iterations=0; iterations <= SoundParameter->duration; iterations++)
    {
        sysWaitOneInterrupt();
    }

 	stopTone();
}

uint32_t sysOpen(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSOPEN", FileParameter->fileDescriptor, FileParameter->fileName);

    uint8_t *newBinaryFilenameLoc = kMalloc(currentPid, FileParameter->fileNameLength);
    strcpyRemoveNewline(newBinaryFilenameLoc, FileParameter->fileName);

    if (FileParameter->requestedPermissions == RDWRITE)
    {
        if (!fileAvailableToBeLocked((uint8_t *)GLOBAL_OBJECT_TABLE, returnInodeofFileName(newBinaryFilenameLoc, cachingEnabled, directoryInode)))
        {
            printString(COLOR_RED, 3, 2, (uint8_t *)"File locked by another process!");
            sysWait();
            sysWait();
            clearScreen();
            //return SYSCALL_FAIL;
        }
    }

    uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *Task = (struct task*)taskStructLocation;

    if (Task->nextAvailableFileDescriptor >= MAX_FILE_DESCRIPTORS) 
    {
        // Failure
        return SYSCALL_FAIL; // reached the max open files
    }
    
    uint8_t *inodePage = requestAvailablePage(currentPid, PG_USER_PRESENT_RW);

    if (inodePage == 0)
    {
        //panic((uint8_t *)"Syscalls.cpp:sysOpen() -> open request available page is null");
        return SYSCALL_FAIL;
    }

    if (!fsFindFile(newBinaryFilenameLoc, (uint8_t *)inodePage, cachingEnabled, directoryInode))
    {
        directoryInode = ROOTDIR_INODE;
        if (!fsFindFile(newBinaryFilenameLoc, (uint8_t *)inodePage, cachingEnabled, directoryInode))
        {
            directoryInode = returnInodeofFileName((uint8_t*)"code", cachingEnabled, ROOTDIR_INODE);
            
            if (!fsFindFile(newBinaryFilenameLoc, (uint8_t *)inodePage, cachingEnabled, directoryInode))
            {
                printString(COLOR_RED, 2, 5, (uint8_t *)"File not found!");
                sysWait();
                sysWait();
                sysWait();
                // Failure
                return SYSCALL_FAIL;
            }
        }
    }

    struct inode *Inode = (struct inode*)inodePage;
    uint32_t pagesNeedForTmpBinary = ceiling(Inode->i_size, PAGE_SIZE);

    uint8_t *requestedBuffer = findBuffer(currentPid, pagesNeedForTmpBinary, PG_USER_PRESENT_RW);

    //request block of pages for temporary file storage to load file based on first available page above
    for (uint32_t pageCount = 0; pageCount < pagesNeedForTmpBinary; pageCount++)
    {                
        if (!requestSpecificPage(currentPid, (uint8_t *)((uint32_t)requestedBuffer + (pageCount * PAGE_SIZE)), PG_USER_PRESENT_RW))
        {
            clearScreen();
            printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
            panic((uint8_t *)"syscalls.cpp -> USER_TEMP_FILE_LOC page request");
        }

        // Zero each page in case it has been used previously
        fillMemory((uint8_t *)((uint32_t)requestedBuffer + (pageCount * PAGE_SIZE)), 0x0, PAGE_SIZE);
    }

    loadFileFromInodeStruct((uint8_t *)inodePage, requestedBuffer, cachingEnabled);

    Task->fileDescriptor[Task->nextAvailableFileDescriptor] = (globalObjectTableEntry *)insertGlobalObjectTableEntry((uint8_t *)GLOBAL_OBJECT_TABLE, currentPid, (int)(returnInodeofFileName(newBinaryFilenameLoc, cachingEnabled, directoryInode)), GOTE_TYPE_FILE, 0, 0, 0, 0, Inode->i_size, requestedBuffer, pagesNeedForTmpBinary, 0, newBinaryFilenameLoc, 0, 0, 0);
    struct globalObjectTableEntry *GOTE = (globalObjectTableEntry *)Task->fileDescriptor[Task->nextAvailableFileDescriptor];

    if (FileParameter->requestedPermissions == RDWRITE)
    {
        if (!lockFile((uint8_t *)GLOBAL_OBJECT_TABLE, currentPid, returnInodeofFileName(newBinaryFilenameLoc, cachingEnabled, directoryInode)))
        {
            printString(COLOR_RED, 4, 2, (uint8_t *)"Unable to acquire file lock!");
            wait(1);
            clearScreen();
        }
    }

    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;

    freePage(currentPid, inodePage);
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR, ((int)Task->nextAvailableFileDescriptor));
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR_PTR, (int)requestedBuffer);
    storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[Task->nextAvailableFileDescriptor], (int)requestedBuffer);
    storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[Task->nextAvailableFileDescriptor], (int)GOTE->size);

    Task->nextAvailableFileDescriptor++;

    // Success
    return SYSCALL_SUCCESS;

}

void sysWrite(uint32_t fileDescriptorToPrint, uint32_t currentPid)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSWRITE", fileDescriptorToPrint,(uint8_t*)"NULL");

    uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *Task = (struct task*)taskStructLocation;
    struct globalObjectTableEntry *GlobalObjectTableEntry = (globalObjectTableEntry*)(uint32_t)Task->fileDescriptor[fileDescriptorToPrint];

    if ((uint32_t)Task->fileDescriptor[fileDescriptorToPrint] == 0x0)
    {
        printString(COLOR_RED, 3, 5, (uint8_t *)"No such file descriptor!");
        sysWait();
        sysWait();
        clearScreen();
        return;
    }

    if (GlobalObjectTableEntry->type == GOTE_TYPE_PIPE)
    {
        uint8_t *userBuf = GlobalObjectTableEntry->userspaceBuffer;

        // if (GlobalObjectTableEntry->lockedForWriting != currentPid)
        // {
        //     printString(COLOR_RED, 3, 2, (uint8_t *)"Pipe is read only for you!");
        //     sysWait();
        //     sysWait();
        //     sysWait();
        //     return;
        // }

        if (GlobalObjectTableEntry->writeOffset == (PIPE_BUF_SIZE - 1))
        {
            printString(COLOR_RED, 3, 2, (uint8_t *)"Pipe is full and cannot be written until reader consumers");
            sysWait();
            sysWait();
            sysWait();
            return;
        }

        
        while (!acquireLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
 
        bytecpy(GlobalObjectTableEntry->kernelSpaceBuffer, userBuf, PIPE_BUF_SIZE);
        GlobalObjectTableEntry->writeOffset = PIPE_BUF_SIZE - 1;

        releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE);

        return;
        
    }

    if (GlobalObjectTableEntry->type == GOTE_TYPE_SOCKET)
    {
        struct networkParameter *NetworkParameter = (struct networkParameter *)kMalloc(currentPid, sizeof(networkParameter));

        while (!acquireLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
 
        memoryCopy(GlobalObjectTableEntry->userspaceBuffer, GlobalObjectTableEntry->kernelSpaceBuffer, ceiling(PIPE_BUF_SIZE, 2));

        NetworkParameter->destinationIPAddress = GlobalObjectTableEntry->destinationIPAddress;
        NetworkParameter->destinationPort = GlobalObjectTableEntry->destinationPort;
        NetworkParameter->packetPayload = GlobalObjectTableEntry->kernelSpaceBuffer;
        NetworkParameter->socketName = GlobalObjectTableEntry->name;
        NetworkParameter->sourceIPAddress = GlobalObjectTableEntry->sourceIPAddress;
        NetworkParameter->sourcePort = GlobalObjectTableEntry->sourcePort;

        netUDPSend(NetworkParameter);

        //fillMemory((uint8_t*)&GlobalObjectTableEntry->userspaceBuffer, 0x0, PIPE_BUF_SIZE);

        GlobalObjectTableEntry->writeOffset = PIPE_BUF_SIZE - 1;
        GlobalObjectTableEntry->packetsTransmitted++;

        releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE);

        kFree((uint8_t*)NetworkParameter);

    }
    else
    {
        struct globalObjectTableEntry *GlobalObjectTableEntry = (globalObjectTableEntry*)(uint32_t)Task->fileDescriptor[fileDescriptorToPrint];
        printString(COLOR_WHITE, 0, 0, (uint8_t *)(GlobalObjectTableEntry->userspaceBuffer)); 
    }
}

void sysClose(uint32_t fileDescriptor, uint32_t currentPid)
{
    
    // ASSIGNMENT 4 TO DO
}

void sysPs(uint32_t currentPid)
{
    // Initial version by Dan O'Malley. Extended with Grok.
    // 12/2025 with Grok v4.
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSPS", 0 ,(uint8_t*)"NULL");

    uint32_t lastUsedPid = 0;
    uint32_t nextAvailPid = 0;
    uint32_t cursor = 0;
    uint32_t taskStructNumber = 0;

    uint32_t taskStructLocation = PROCESS_TABLE_LOC; 

    volatile uint32_t *lapic = (volatile uint32_t *)LAPIC_ADDR;
    uint32_t bsp_id = (lapic[0x20 >> 2] >> 24) & 0xFF;
    *(uint32_t *)CURRENT_PROC_RUNNING_LOC = bsp_id;

    // Allocate a single reusable buffer for all string conversions
    uint8_t *buf = kMalloc(currentPid, 32);  // Larger size to cover all needs safely

    printString(COLOR_RED, cursor, 2, (uint8_t *)"CPU: ");
    if (buf)
    {
        itoa(*(uint32_t *)CURRENT_PROC_RUNNING_LOC, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 7, buf);
    }

    printString(COLOR_RED, cursor, 12, (uint8_t *)"AP TICK: ");
    if (buf)
    {
        itoa(*(uint32_t*)SECOND_PROC_TICK_COUNT_LOC, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 21, buf);
    }

    printString(COLOR_RED, cursor, 30, (uint8_t *)"NIC Stat: ");
    printHexNumber(COLOR_LIGHT_BLUE, cursor, 40, inputIOPort(NE2000_BASE_PORT + 0x04));

    printString(COLOR_RED, cursor, 45, (uint8_t *)"TX: ");
    if (buf)
    {
        itoa(*(uint32_t*)NETWORK_PACKETS_TRANSMITTED, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 49, buf);
    }

    printString(COLOR_RED, cursor, 55, (uint8_t *)"RX: ");
    if (buf)
    {
        itoa(*(uint32_t*)NETWORK_PACKETS_RECEIVED, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 59, buf);
    }

    cursor++;
    cursor++;

    printString(COLOR_GREEN, cursor, 2, (uint8_t *)"System Frames Used: ");
    if (buf)
    {
        itoa(totalFramesUsed((uint8_t *)PAGEFRAME_MAP_BASE), buf);
        printString(COLOR_LIGHT_BLUE, cursor, 22, buf);
    }

    printString(COLOR_GREEN, cursor, 30, (uint8_t *)"Mem Total/Used/Free (KB): ");
    if (buf)
    {
        itoa((PAGEFRAME_MAP_SIZE * PAGE_SIZE) / 1024, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 56, buf);
    }
    if (buf)
    {
        itoa((totalFramesUsed((uint8_t *)PAGEFRAME_MAP_BASE) * PAGE_SIZE) / 1024, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 63, buf);
    }
    if (buf)
    {
        itoa(((PAGEFRAME_MAP_SIZE * PAGE_SIZE) / 1024) - ((totalFramesUsed((uint8_t *)PAGEFRAME_MAP_BASE) * PAGE_SIZE) / 1024), buf);
        printString(COLOR_LIGHT_BLUE, cursor, 69, buf);
    }

    cursor++;

    printString(COLOR_GREEN, cursor, 2, (uint8_t *)"PID Frames Used: ");
    if (buf)
    {
        itoa(processFramesUsed(currentPid, (uint8_t *)PAGEFRAME_MAP_BASE), buf);
        printString(COLOR_LIGHT_BLUE, cursor, 22, buf);
    }

    printString(COLOR_GREEN, cursor, 30, (uint8_t *)"PID Mem Used (KB): ");
    if (buf)
    {
        itoa((processFramesUsed(currentPid, (uint8_t *)PAGEFRAME_MAP_BASE) * PAGE_SIZE) / 1024, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 56, buf);
    }

    cursor++;

    printString(COLOR_GREEN, cursor, 2, (uint8_t *)"PID Heap Objects: ");
    if (buf)
    {
        itoa(countHeapObjects((uint8_t *)USER_HEAP), buf);
        printString(COLOR_LIGHT_BLUE, cursor, 22, buf);
    }

    printString(COLOR_GREEN, cursor, 30, (uint8_t *)"Kernel Heap Objects: ");
    if (buf)
    {
        itoa(countKernelHeapObjects((uint8_t *)KERNEL_HEAP), buf);
        printString(COLOR_LIGHT_BLUE, cursor, 56, buf);
    }

    cursor++;

    printString(COLOR_GREEN, cursor, 2, (uint8_t *)"Total Interrupts: ");
    if (buf)
    {
        itoa(totalInterruptCount, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 22, buf);
    }

    printString(COLOR_GREEN, cursor, 30, (uint8_t *)"Timer Interrupts: ");
    if (buf)
    {
        itoa(systemTimerInterruptCount, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 56, buf);
    }

    cursor++;

    printString(COLOR_GREEN, cursor, 2, (uint8_t *)"Keyboard Interrupts: ");
    if (buf)
    {
        itoa(keyboardInterruptCount, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 22, buf);
    }

    printString(COLOR_GREEN, cursor, 30, (uint8_t *)"Other Interrupts: ");
    if (buf)
    {
        itoa(otherInterruptCount, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 56, buf);
    }

    cursor++;

    printString(COLOR_GREEN, cursor, 2, (uint8_t *)"Global Objects: ");
    if (buf)
    {
        itoa(totalOpenFiles((uint8_t *)GLOBAL_OBJECT_TABLE), buf);
        printString(COLOR_LIGHT_BLUE, cursor, 22, buf);
    }

    printString(COLOR_GREEN, cursor, 30, (uint8_t *)"Cache Hits/Misses: ");
    if (buf)
    {
        itoa(*(uint32_t*)KERNEL_CACHE_HITS, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 56, buf);
    }
    if (buf)
    {
        itoa(*(uint32_t*)KERNEL_CACHE_MISSES, buf);
        printString(COLOR_LIGHT_BLUE, cursor, 63, buf);
    }

    cursor++;

    uint8_t upper_left = ASCII_UPPERLEFT_CORNER;
    printCharacter(COLOR_WHITE, cursor+1, 1, &upper_left);

    uint8_t horiz = ASCII_HORIZONTAL_LINE;
    for (uint32_t columnPos = 2; columnPos < 78; columnPos++)
    {
        printCharacter(COLOR_WHITE, cursor+1, columnPos, &horiz);
    }
    
    uint8_t upper_right = ASCII_UPPERRIGHT_CORNER;
    printCharacter(COLOR_WHITE, cursor+1, 78, &upper_right);

    cursor++;
    cursor++;

    printString(COLOR_RED, cursor-1, 2, (uint8_t *)"PID");
    printString(COLOR_RED, cursor-1, 9, (uint8_t *)"NAME");
    printString(COLOR_RED, cursor-1, 22, (uint8_t *)"STATE");
    printString(COLOR_RED, cursor-1, 33, (uint8_t *)"PPID");
    printString(COLOR_RED, cursor-1, 39, (uint8_t *)"PRI");
    printString(COLOR_RED, cursor-1, 44, (uint8_t *)"RTIME");
    printString(COLOR_RED, cursor-1, 51, (uint8_t *)"RRTIME");
    printString(COLOR_RED, cursor-1, 58, (uint8_t *)"STIME");
    printString(COLOR_RED, cursor-1, 65, (uint8_t *)"SIGNAL");
    printString(COLOR_RED, cursor-1, 73, (uint8_t *)"EXIT");

    //cursor++;

    while (taskStructNumber < MAX_PROCESSES)
    {
        taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * taskStructNumber);
    
        struct task *Task = (struct task*)taskStructLocation;
        
        lastUsedPid = *(uint32_t *)(PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * taskStructNumber));

        if ((uint32_t)lastUsedPid == 0)
        {
            nextAvailPid = (taskStructNumber + 1);
            taskStructNumber++;
        }
        else
        {
            
            uint8_t vert = ASCII_VERTICAL_LINE;
            printCharacter(COLOR_WHITE, cursor, 1, &vert);

            if (buf)
            {
                itoa(Task->pid, buf);
                printString(COLOR_LIGHT_BLUE, cursor, 2, buf);
            }

            printString(COLOR_WHITE, cursor, 9, Task->binaryName);

            uint32_t state_color = COLOR_WHITE;
            uint8_t *state_str = (uint8_t *)"UNKNOWN";
            if (Task->state == PROC_SLEEPING)
            {
                state_str = (uint8_t *)"SLEEPING";
                state_color = COLOR_GREEN;
            }
            else if (Task->state == PROC_RUNNING)
            {
                state_str = (uint8_t *)"RUNNING";
                state_color = COLOR_LIGHT_BLUE;
            }
            else if (Task->state == PROC_ZOMBIE)
            {
                state_str = (uint8_t *)"ZOMBIE";
                state_color = COLOR_RED;
            }
            else if (Task->state == PROC_KILLED)
            {
                state_str = (uint8_t *)"KILLED";
                state_color = COLOR_RED;
            }
            printString(state_color, cursor, 22, state_str);

            if (buf)
            {
                itoa(Task->ppid, buf);
                //printString(COLOR_LIGHT_BLUE, cursor, 36, (uint8_t*)"0x");
                printString(COLOR_LIGHT_BLUE, cursor, 33, buf);
            }

            if (buf)
            {
                itoa(Task->priority, buf);
                printString(COLOR_WHITE, cursor, 39, buf);
            }

            if (buf)
            {
                itoa((Task->runtime) / SYSTEM_INTERRUPTS_PER_SECOND, buf);
                printString(COLOR_GREEN, cursor, 44, buf);
            }

            if (buf)
            {
                itoa((Task->recentRuntime) / SYSTEM_INTERRUPTS_PER_SECOND, buf);
                printString(COLOR_GREEN, cursor, 51, buf);
            }

            if (buf)
            {
                itoa((Task->sleepTime) / SYSTEM_INTERRUPTS_PER_SECOND, buf);
                printString(COLOR_GREEN, cursor, 58, buf);
            }


            if (buf)
            {
                itoaHex(Task->signal, buf);
                printString(COLOR_RED, cursor, 65, buf);
            }

            if (buf)
            {
                itoaHex(Task->exitCode, buf);
                printString(COLOR_RED, cursor, 73, buf);
            }

            printCharacter(COLOR_WHITE, cursor, 78, &vert);

            cursor++;
            taskStructNumber++;   
        }
    }

    printSystemUptime(uptimeSeconds);

    uint8_t lower_left = ASCII_LOWERLEFT_CORNER;
    printCharacter(COLOR_WHITE, cursor, 1, &lower_left);

    for (uint32_t columnPos = 2; columnPos < 78; columnPos++)
    {
        printCharacter(COLOR_WHITE, cursor, columnPos, &horiz);
    }

    uint8_t lower_right = ASCII_LOWERRIGHT_CORNER;
    printCharacter(COLOR_WHITE, cursor, 78, &lower_right);

    // Free the buffer at the end
    if (buf)
    {
        kFree(buf);
    }
}

void sysExec(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode)
{
    // Initial version by Dan O'Malley. Extended with owner checking by Grok.
    // 12/2025 with Grok v4.
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSEXEC", FileParameter->fileDescriptor, FileParameter->fileName);

    uint32_t oldPidTaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *oldPidTask = (struct task*)oldPidTaskStructLocation;

    uint32_t requestedNewPidStdIn;
    uint32_t requestedNewPidStdOut;
    uint32_t requestedNewPidStdErr;

    if (FileParameter->requestedStdInFd == 0)
    {
        requestedNewPidStdIn = GLOBAL_OBJECT_TABLE;
    }
    else
    {
        requestedNewPidStdIn = (uint32_t)oldPidTask->fileDescriptor[FileParameter->requestedStdInFd];
    }

    if (FileParameter->requestedStdOutFd == 0)
    {
        requestedNewPidStdOut = GLOBAL_OBJECT_TABLE + sizeof(globalObjectTableEntry);
    }
    else
    {
        requestedNewPidStdOut = (uint32_t)oldPidTask->fileDescriptor[FileParameter->requestedStdOutFd];
    }

    if (FileParameter->requestedStdErrFd == 0)
    {
        requestedNewPidStdErr = GLOBAL_OBJECT_TABLE + (sizeof(globalObjectTableEntry) * 2);
    }
    else
    {
        requestedNewPidStdErr = (uint32_t)oldPidTask->fileDescriptor[FileParameter->requestedStdErrFd];
    }

    uint8_t *newBinaryFilenameLoc = kMalloc(currentPid, strlen(FileParameter->fileName)); 
    strcpyRemoveNewline(newBinaryFilenameLoc, FileParameter->fileName);

    if (!fsFindFile(newBinaryFilenameLoc, USER_TEMP_INODE_LOC, cachingEnabled, directoryInode))
    {
        directoryInode = returnInodeofFileName((uint8_t*)"bin", cachingEnabled, ROOTDIR_INODE);
        
        if (!fsFindFile(newBinaryFilenameLoc, USER_TEMP_INODE_LOC, cachingEnabled, directoryInode))
        {
            directoryInode = returnInodeofFileName((uint8_t*)"code", cachingEnabled, ROOTDIR_INODE);
        
            if (!fsFindFile(newBinaryFilenameLoc, USER_TEMP_INODE_LOC, cachingEnabled, directoryInode))
            {
                directoryInode = ROOTDIR_INODE;
        
                if (!fsFindFile(newBinaryFilenameLoc, USER_TEMP_INODE_LOC, cachingEnabled, directoryInode))
                {
                    printString(COLOR_RED, 2, 5, (uint8_t *)"File not found!");
                    sysWait();
                    sysWait();
                    return;
                }
            }

        }
    }
      
    uint32_t newPid = 0;

    newPid = initializeTask(currentPid, PROC_SLEEPING, STACK_START_LOC, newBinaryFilenameLoc, FileParameter->requestedRunPriority, directoryInode, requestedNewPidStdIn, requestedNewPidStdOut, requestedNewPidStdErr);
    initializePageTables(newPid);

    contextSwitch(newPid);

    if (!requestSpecificPage(newPid, (uint8_t *)(STACK_PAGE - PAGE_SIZE), PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"syscalls.cpp -> STACK_PAGE - PAGE_SIZE page request");
    }

    if (!requestSpecificPage(newPid, (uint8_t *)(STACK_PAGE), PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"syscalls.cpp -> STACK_PAGE page request");
    }

    for (uint32_t userHeapLoc = 0; userHeapLoc < (USER_HEAP_PAGES * PAGE_SIZE); userHeapLoc = userHeapLoc + PAGE_SIZE)
    {
        if (!requestSpecificPage(newPid, (uint8_t*)(USER_HEAP + userHeapLoc), PG_USER_PRESENT_RW))
        {
            clearScreen();
            printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
            panic((uint8_t *)"syscalls.cpp -> USER_HEAP page request");
        }
    }

    if (!requestSpecificPage(newPid, USER_TEMP_INODE_LOC, PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"syscalls.cpp -> USER_TEMP_INODE_LOC page request");
    }
   
    // We do this function twice. First one (above) to make sure the file exists.
    // This second one (below) is because there was a context switch and a different
    // userspace.
    fsFindFile(newBinaryFilenameLoc, USER_TEMP_INODE_LOC, cachingEnabled, directoryInode);

    struct inode *Inode = (struct inode*)USER_TEMP_INODE_LOC;

    uint32_t pagesNeedForTmpBinary = ceiling(Inode->i_size, PAGE_SIZE);

    //request pages for temporary file storage to load raw ELF file
    for (uint32_t tempFileLoc = 0; tempFileLoc < (pagesNeedForTmpBinary * PAGE_SIZE); tempFileLoc = tempFileLoc + PAGE_SIZE)
    {
        if (!requestSpecificPage(newPid, (USER_TEMP_FILE_LOC + tempFileLoc), PG_USER_PRESENT_RW))
        {
            clearScreen();
            printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
            panic((uint8_t *)"syscalls.cpp -> USER_TEMP_FILE_LOC page request");
        }
    }

    loadFileFromInodeStruct(USER_TEMP_INODE_LOC, USER_TEMP_FILE_LOC, cachingEnabled); 

    if (*(uint32_t *)USER_TEMP_FILE_LOC != MAGIC_ELF)
    {
        printString(COLOR_RED, 2, 5, (uint8_t *)"File is not an ELF file!");
        sysWait();
        sysWait();
        sysKill(newPid);
        contextSwitch(currentPid);
        return;
    }

    if (!hasOwnerExecute(Inode->i_mode))
    {
        printString(COLOR_RED, 2, 5, (uint8_t *)"You don't have owner permission to execute file!");
        sysWait();
        sysWait();
        sysKill(newPid);
        contextSwitch(currentPid);
        return;
    }


    struct elfHeader *ELFHeader = (struct elfHeader*)USER_TEMP_FILE_LOC;

    if (ELFHeader->e_flags == ELF_LOAD_DYNAMIC_LIBRARIES)
    {
        for (uint32_t dynamicLibrariesLoc = 0; dynamicLibrariesLoc < ((ceiling(DYNAMIC_LIBRARIES_SIZE, PAGE_SIZE)) * PAGE_SIZE); dynamicLibrariesLoc = dynamicLibrariesLoc + PAGE_SIZE)
        {
            if (!requestSpecificPage(newPid, (uint8_t *)(SHARED_LIBRARIES_START_LOC + dynamicLibrariesLoc), PG_USER_PRESENT_RW))
            {
                clearScreen();
                printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
                panic((uint8_t *)"syscalls.cpp -> Allocating dynamic library space requests");
            }

            fillMemory((uint8_t *)(SHARED_LIBRARIES_START_LOC + dynamicLibrariesLoc), 0x0, PAGE_SIZE);

        }

        struct fileParameter *FileParameter = (fileParameter *)kMalloc(currentPid, 200);

        FileParameter->fileName = (uint8_t*)"libc.o\n";
        FileParameter->fileNameLength = strlen((uint8_t*)"libc.o\n");

        sysOpen(FileParameter, newPid, directoryInode);

        uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (newPid - 1));
        struct task *Task = (struct task*)taskStructLocation;
        uint8_t* currentFileDescriptorPointer = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);
        uint32_t currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        struct globalObjectTableEntry *GlobalObjectTableEntry = (struct globalObjectTableEntry*)(Task->fileDescriptor[currentFileDescriptor]);

        bytecpy((uint8_t*)SHARED_LIBRARIES_START_LOC, currentFileDescriptorPointer, GlobalObjectTableEntry->size);

    }


    for (int x = 0; x < ELFHeader->e_phnum; x++) 
    {
        struct pHeader *ProgramHeader = (struct pHeader*)((uint32_t)ELFHeader + ELFHeader->e_phoff + (ELFHeader->e_phentsize * x));

        if (ProgramHeader->p_type == PT_LOAD) 
        {
            for (uint32_t tempFileLoc = 0; tempFileLoc < ((ceiling(ProgramHeader->p_memsz, PAGE_SIZE)) * PAGE_SIZE); tempFileLoc = tempFileLoc + PAGE_SIZE)
            {
                if (!requestSpecificPage(newPid, (uint8_t *)(ProgramHeader->p_vaddr + tempFileLoc), PG_USER_PRESENT_RW))
                {
                    clearScreen();
                    printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
                    panic((uint8_t *)"syscalls.cpp -> USERPROG TEXT and DATA VADDR page requests");
                }

            }
           
        }
    }
    
    loadElfFile(USER_TEMP_FILE_LOC);

    struct elfHeader *ELFHeaderLaunch = (struct elfHeader*)USER_TEMP_FILE_LOC;

    updateTaskState(currentPid, PROC_SLEEPING);
    updateTaskState(newPid, PROC_RUNNING);

    // Letting the new process know its pid
    storeValueAtMemLoc(RUNNING_PID_LOC, newPid);

    struct globalObjectTableEntry *GlobalObjectTableEntryRequestedStdIn = (struct globalObjectTableEntry*)(requestedNewPidStdIn);
    struct globalObjectTableEntry *GlobalObjectTableEntryRequestedStdOut = (struct globalObjectTableEntry*)(requestedNewPidStdOut);
    struct globalObjectTableEntry *GlobalObjectTableEntryRequestedStdErr = (struct globalObjectTableEntry*)(requestedNewPidStdErr);
    struct globalObjectTableEntry *GlobalObjectTableEntryTypicalStdIn = (struct globalObjectTableEntry *)GLOBAL_OBJECT_TABLE;
    struct globalObjectTableEntry *GlobalObjectTableEntryTypicalStdOut = (struct globalObjectTableEntry *)GLOBAL_OBJECT_TABLE + sizeof(globalObjectTableEntry);
    struct globalObjectTableEntry *GlobalObjectTableEntryTypicalStdErr = (struct globalObjectTableEntry *)GLOBAL_OBJECT_TABLE + (sizeof(globalObjectTableEntry) *2);

    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;

    if (requestedNewPidStdIn == 0)
    {
        storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[0], (uint32_t)GlobalObjectTableEntryTypicalStdIn);
        storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[0], GlobalObjectTableEntryTypicalStdIn->size);
    }
    else
    {
        storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[0], (uint32_t)GlobalObjectTableEntryRequestedStdIn->userspaceBuffer);
        storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[0], (uint32_t)GlobalObjectTableEntryRequestedStdIn->size);
    }

    if (requestedNewPidStdOut == 0)
    {
        storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[1], (int)GlobalObjectTableEntryTypicalStdOut);
        storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[1], (int)GlobalObjectTableEntryTypicalStdOut->size);
    }
    else
    {
        storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[1], (uint32_t)GlobalObjectTableEntryRequestedStdOut->userspaceBuffer);
        storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[1], (uint32_t)GlobalObjectTableEntryRequestedStdOut->size);
    }

    if (requestedNewPidStdErr == 0)
    {
        storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[2], (uint32_t)GlobalObjectTableEntryTypicalStdErr);
        storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[2], GlobalObjectTableEntryTypicalStdErr->size);
    }
    else
    {
        storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[2], (uint32_t)GlobalObjectTableEntryRequestedStdErr->userspaceBuffer);
        storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[2], (uint32_t)GlobalObjectTableEntryRequestedStdErr->size);
    }

    enableInterrupts();
    
    switchToRing3LaunchBinary((uint8_t *)ELFHeaderLaunch->e_entry);

}

void sysExit(uint32_t currentPid, uint32_t exitCode)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSEXIT", 0,(uint8_t*)"NULL");

    if (currentPid == 1)
    {
        return; // PID 1 cannot exit
    }

    //fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
    
    uint32_t currentTaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    
    struct task *currentTask = (struct task*)currentTaskStructLocation;
    currentTask->exitCode = exitCode;

    updateTaskState(currentPid, PROC_ZOMBIE);
    updateTaskState(currentTask->ppid, PROC_RUNNING);

    // Letting the new process know its pid
    storeValueAtMemLoc(RUNNING_PID_LOC, (currentTask->ppid));

    // Checking the parent's status to make sure it isn't dead before switching back to it
    uint32_t parentTaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentTask->ppid - 1));
    struct task *parentTask = (struct task*)parentTaskStructLocation;

    if (parentTask->state != PROC_SLEEPING || parentTask->state != PROC_RUNNING)
    {
        contextSwitch(currentTask->ppid);
    }
    else
    {
        // Go back to init if parent is dead
        contextSwitch(0x1);
    }

}

void sysFree(uint32_t currentPid)
{    
    
    // ASSIGNMENT 4 TO DO
}

void sysMmap(uint8_t* requestedPage, uint32_t currentPid)
{    
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSMMAP",(uint32_t)requestedPage, (uint8_t*)"NULL");

    if (requestedPage == 0)
    {
        uint32_t returnedSuccessful = (unsigned int)requestAvailablePage(currentPid, PG_USER_PRESENT_RW);
    
        if (returnedSuccessful == 0)
        {
            storeValueAtMemLoc(RETURNED_MMAP_PAGE_LOC, (uint32_t)0);
        }

        storeValueAtMemLoc(RETURNED_MMAP_PAGE_LOC, (uint32_t)requestedPage);
    }
    
    if (requestedPage != 0)
    {

        uint32_t returnedSuccessful = (unsigned int)requestSpecificPage(currentPid, requestedPage, PG_USER_PRESENT_RW);
        
        if (returnedSuccessful == 0)
        {
            storeValueAtMemLoc(RETURNED_MMAP_PAGE_LOC, (uint32_t)0);
        }

        storeValueAtMemLoc(RETURNED_MMAP_PAGE_LOC, (uint32_t)requestedPage);

    }
}

void sysKill(uint32_t pidToKill)
{   
    
    // ASSIGNMENT 4 TO DO
}

void sysSwitch(uint32_t pidToSwitchTo, uint32_t currentPid)
{
    
    // ASSIGNMENT 4 TO DO
    
}


void sysDiskSectorDump(uint32_t sectorToDump)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    fillMemory((uint8_t*)SECTOR_AND_BLOCK_VIEWER_BUF_LOC, 0x0, PAGE_SIZE);
    uint32_t memAddressToDump = SECTOR_AND_BLOCK_VIEWER_BUF_LOC;


    diskReadSector(sectorToDump, (uint8_t*)memAddressToDump, cachingEnabled);
    
    clearScreen();

    for (uint32_t row = 1; row < 29; row++ )
    {
        uint32_t rowStart = memAddressToDump;

        uint8_t addrStr[9];
        itoaHex(rowStart, addrStr);
        uint32_t len = strlen(addrStr);
        if (len < 8)
        {
            uint32_t shift = 8 - len;
            for (uint32_t j = len; j > 0; j--) // Shift characters including the null terminator
            {
                addrStr[j + shift - 1] = addrStr[j - 1];
            }
            for (uint32_t j = 0; j < shift; j++)
            {
                addrStr[j] = '0';
            }
            addrStr[8] = '\0';
        }
        printString(COLOR_GREEN, row, 0, addrStr);
        printString(COLOR_WHITE, row, 8, (uint8_t *)": ");

        for (uint32_t byteIndex = 0; byteIndex < 16; byteIndex++)
        {
            uint32_t column = 10 + byteIndex * 3;
            printHexNumber(COLOR_WHITE, row, column, *(uint8_t *)memAddressToDump);
            memAddressToDump++;
        }

        printString(COLOR_WHITE, row, 57, (uint8_t *)"  ");

        for (uint32_t i = 0; i < 16; i++)
        {
            uint8_t byte = *(uint8_t *)(rowStart + i);
            uint8_t toPrint = (byte >= 32 && byte <= 126) ? byte : '.';
            printCharacter(COLOR_WHITE, row, 59 + i, &toPrint);
        }
    }

    sysShowOpenFiles(30, currentPid);

}

void sysGetInodeForUser(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode)
{
    // Dan O'Malley
    
    fillMemory((uint8_t*)SECTOR_AND_BLOCK_VIEWER_BUF_LOC, 0x0, PAGE_SIZE);
    loadInode(returnInodeofFileName(FileParameter->fileName, cachingEnabled, directoryInode), (uint8_t*)SECTOR_AND_BLOCK_VIEWER_BUF_LOC, cachingEnabled);
    bytecpy((uint8_t*)REQUESTED_INODE_LOC, (uint8_t*)SECTOR_AND_BLOCK_VIEWER_BUF_LOC, sizeof(inode));
}

void sysChangeFileMode(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode)
{
    // Dan O'Malley
    
    changeFileMode(FileParameter->fileName, FileParameter->requestedFileMode, currentPid, cachingEnabled, directoryInode);
}


void sysInodeDump(uint32_t inodeNumber, uint32_t currentPid) 
{

    // Written by Grok.
    // 12/2025 with Grok v4.
    
    fillMemory((uint8_t*)SECTOR_AND_BLOCK_VIEWER_BUF_LOC, 0x0, PAGE_SIZE);
    loadInode(inodeNumber, (uint8_t*)SECTOR_AND_BLOCK_VIEWER_BUF_LOC, cachingEnabled);
    uint8_t * inodeLocation = (uint8_t*)SECTOR_AND_BLOCK_VIEWER_BUF_LOC;

    clearScreen();
    printString(COLOR_LIGHT_BLUE, 0, 0, (uint8_t*)"Inode Raw Bytes:");
    struct inode *inode = (struct inode*)inodeLocation;
    uint32_t inode_size = INODE_SIZE;
    uint32_t total_rows = 4; // Increased to four rows
    uint8_t *mem = inodeLocation;
    uint32_t dump_start_row = 1;

    for (uint32_t r = 0; r < total_rows; r++) {
        uint32_t row = dump_start_row + r;
        if (row >= 40) break;
        uint8_t addrStr[9];
        itoaHex((uint32_t)mem, addrStr);
        uint32_t len = strlen(addrStr);
        if (len < 8) {
            uint32_t shift = 8 - len;
            for (uint32_t j = len; j > 0; j--) {
                addrStr[j + shift - 1] = addrStr[j - 1];
            }
            for (uint32_t j = 0; j < shift; j++) {
                addrStr[j] = '0';
            }
            addrStr[8] = '\0';
        }
        printString(COLOR_GREEN, row, 0, addrStr);
        printString(COLOR_WHITE, row, 8, (uint8_t*)": ");
        //uint32_t bytes_this_row = 16;
        for (uint32_t b = 0; b < 16; b++) {
            uint32_t col = 10 + b * 3;
            if ((r * 16 + b) < inode_size) {
                uint8_t byte = mem[b];
                printHexNumber(COLOR_WHITE, row, col, byte);
            } else {
                printString(COLOR_WHITE, row, col, (uint8_t*)"  ");
            }
        }
        printString(COLOR_WHITE, row, 57, (uint8_t*)"  ");
        for (uint32_t b = 0; b < 16; b++) {
            if ((r * 16 + b) < inode_size) {
                uint8_t byte = mem[b];
                uint8_t toPrint = (byte >= 32 && byte <= 126) ? byte : '.';
                printCharacter(COLOR_WHITE, row, 59 + b, &toPrint);
            } else {
                printCharacter(COLOR_WHITE, row, 59 + b, (uint8_t*)" ");
            }
        }
        mem += 16;
    }

    uint32_t left_row = dump_start_row + total_rows + 1;
    uint32_t right_row = left_row;
    if (left_row >= 40) return;

    // Left column: Inode Fields
    printString(COLOR_LIGHT_BLUE, left_row++, 0, (uint8_t*)"Inode Fields:");
    if (left_row >= 40) return;
    uint8_t buf[16];

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_mode:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_mode, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_uid:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_uid, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_size:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_size, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_atime:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_atime, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_ctime:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_ctime, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_mtime:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_mtime, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_dtime:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_dtime, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_gid:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_gid, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_links_count:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_links_count, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_blocks:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_blocks, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_flags:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_flags, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_osd1:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_osd1, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_generation:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_generation, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_file_acl:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_file_acl, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_dir_acl:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_dir_acl, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_faddr:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_faddr, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_osd2[0]:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_osd2[0], buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_osd2[1]:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_osd2[1], buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"i_osd2[2]:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(inode->i_osd2[2], buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 40) return;

    // Right column: Direct Blocks
    printString(COLOR_LIGHT_BLUE, right_row++, 40, (uint8_t*)"Direct Blocks:");
    if (right_row >= 40) return;

    for (uint32_t i = 0; i < EXT2_NUMBER_OF_DIRECT_BLOCKS; i++) {
        if (right_row >= 40) return;
        uint8_t label[16];
        strcpy(label, (uint8_t*)"i_block[");
        uint8_t idx_buf[4];
        itoa(i, idx_buf);
        uint32_t l = strlen(label);
        strcpy(label + l, idx_buf);
        l = strlen(label);
        label[l] = ']';
        label[l + 1] = ':';
        label[l + 2] = '\0';
        printString(COLOR_GREEN, right_row, 42, label);
        strcpy(buf, (uint8_t*)"0x");
        itoaHex(inode->i_block[i], buf + 2);
        printString(COLOR_WHITE, right_row++, 60, buf);
    }

    // Bottom: Indirect Blocks
    uint32_t bottom_row = (left_row > right_row ? left_row : right_row);
    if (bottom_row >= 40) return;

    uint32_t indirect_block_num = inode->i_block[EXT2_FIRST_INDIRECT_BLOCK];
    if (indirect_block_num == 0) {

        bottom_row++; 

        printString(COLOR_RED, bottom_row++, 0, (uint8_t*)"No Indirect Blocks");
        return;
    }

    readBlock(indirect_block_num, (uint8_t*)EXT2_INDIRECT_BLOCK, true);
    uint32_t *indirect_blocks = (uint32_t*)EXT2_INDIRECT_BLOCK;
    uint32_t max_indirect = EXT2_BLOCKS_PER_INDIRECT_BLOCK;

    bottom_row++;

    printString(COLOR_LIGHT_BLUE, bottom_row++, 0, (uint8_t*)"Indirect Blocks:");
    if (bottom_row >= 40) return;

    uint32_t i = 0;
    while (bottom_row < 40 && i < max_indirect) {
        // Left side
        if (i < max_indirect) {
            uint8_t label[20];
            strcpy(label, (uint8_t*)"indirect[");
            uint8_t idx_buf[4];
            itoa(i, idx_buf);
            uint32_t l = strlen(label);
            strcpy(label + l, idx_buf);
            l = strlen(label);
            label[l] = ']';
            label[l + 1] = ':';
            label[l + 2] = '\0';
            printString(COLOR_GREEN, bottom_row, 2, label);
            strcpy(buf, (uint8_t*)"0x");
            itoaHex(indirect_blocks[i], buf + 2);
            printString(COLOR_WHITE, bottom_row, 20, buf);
            i++;
        }

        // Right side
        if (i < max_indirect) {
            uint8_t label[20];
            strcpy(label, (uint8_t*)"indirect[");
            uint8_t idx_buf[4];
            itoa(i, idx_buf);
            uint32_t l = strlen(label);
            strcpy(label + l, idx_buf);
            l = strlen(label);
            label[l] = ']';
            label[l + 1] = ':';
            label[l + 2] = '\0';
            printString(COLOR_GREEN, bottom_row, 42, label);
            strcpy(buf, (uint8_t*)"0x");
            itoaHex(indirect_blocks[i], buf + 2);
            printString(COLOR_WHITE, bottom_row, 60, buf);
            i++;
        }

        bottom_row++;
    }

    waitForEnterOrQuit();
    clearScreen();
}


void sysElfHeaderDump(uint8_t *elfHeaderLocation) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    clearScreen();
    printString(COLOR_LIGHT_BLUE, 0, 0, (uint8_t*)"ELF Header Raw Bytes:");
    struct elfHeader *header = (struct elfHeader*)elfHeaderLocation;
    uint32_t ehsize = header->e_ehsize;
    uint32_t total_rows = (ehsize + 15) / 16;
    uint8_t *mem = elfHeaderLocation;
    uint32_t dump_start_row = 1;

    for (uint32_t r = 0; r < total_rows; r++) {
        uint32_t row = dump_start_row + r;
        if (row >= 35) break;
        uint8_t addrStr[9];
        itoaHex((uint32_t)mem, addrStr);
        uint32_t len = strlen(addrStr);
        if (len < 8) {
            uint32_t shift = 8 - len;
            for (uint32_t j = len; j > 0; j--) {
                addrStr[j + shift - 1] = addrStr[j - 1];
            }
            for (uint32_t j = 0; j < shift; j++) {
                addrStr[j] = '0';
            }
            addrStr[8] = '\0';
        }
        printString(COLOR_GREEN, row, 0, addrStr);
        printString(COLOR_WHITE, row, 8, (uint8_t*)": ");
        uint32_t bytes_this_row = (r == total_rows - 1) ? (ehsize % 16) : 16;
        if (bytes_this_row == 0) bytes_this_row = 16;
        for (uint32_t b = 0; b < 16; b++) {
            uint32_t col = 10 + b * 3;
            if (b < bytes_this_row) {
                uint8_t byte = mem[b];
                printHexNumber(COLOR_WHITE, row, col, byte);
            } else {
                printString(COLOR_WHITE, row, col, (uint8_t*)"  ");
            }
        }
        printString(COLOR_WHITE, row, 57, (uint8_t*)"  ");
        for (uint32_t b = 0; b < 16; b++) {
            if (b < bytes_this_row) {
                uint8_t byte = mem[b];
                uint8_t toPrint = (byte >= 32 && byte <= 126) ? byte : '.';
                printCharacter(COLOR_WHITE, row, 59 + b, &toPrint);
            } else {
                printCharacter(COLOR_WHITE, row, 59 + b, (uint8_t*)" ");
            }
        }
        mem += 16;
    }

    uint32_t left_row = dump_start_row + total_rows + 1;
    uint32_t right_row = left_row;

    if (left_row >= 35) return;

    // Left column: ELF Header Fields
    printString(COLOR_LIGHT_BLUE, left_row++, 0, (uint8_t*)"ELF Header Fields:");
    if (left_row >= 35) return;
    uint8_t buf[16];

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Magic:");
    for (uint32_t i = 0; i < 4; i++) {
        printHexNumber(COLOR_WHITE, left_row, 20 + i * 3, header->e_ident[i]);
    }
    left_row++;
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Class:");
    itoaHex(header->e_ident[4], buf);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Data:");
    itoaHex(header->e_ident[5], buf);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Version:");
    itoaHex(header->e_ident[6], buf);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Type:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_type, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Machine:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_machine, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Version:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_version, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Entry:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_entry, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Phoff:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_phoff, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Shoff:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_shoff, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Flags:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_flags, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Ehsize:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_ehsize, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Phentsize:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_phentsize, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Phnum:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_phnum, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Shentsize:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_shentsize, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Shnum:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_shnum, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"Shstrndx:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(header->e_shstrndx, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    left_row++;

    if (left_row >= 35) return;

    // Program Headers
    uint8_t *ph_loc = elfHeaderLocation + header->e_phoff;
    uint32_t phsize = header->e_phentsize;
    uint16_t phnum = header->e_phnum;

    if (phnum == 0) return;

    struct pHeader *ph[3];
    uint32_t display_num = (phnum > 3) ? 3 : phnum;
    for (uint32_t i = 0; i < display_num; i++) {
        ph[i] = (struct pHeader*)(ph_loc + i * phsize);
    }

    // Left: Program Header 0
    printString(COLOR_LIGHT_BLUE, left_row++, 0, (uint8_t*)"Program Header 0:");
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"p_type:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[0]->p_type, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"p_offset:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[0]->p_offset, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"p_vaddr:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[0]->p_vaddr, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"p_paddr:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[0]->p_paddr, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"p_filesz:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[0]->p_filesz, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"p_memsz:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[0]->p_memsz, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"p_flags:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[0]->p_flags, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    printString(COLOR_GREEN, left_row, 2, (uint8_t*)"p_align:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[0]->p_align, buf + 2);
    printString(COLOR_WHITE, left_row++, 20, buf);
    if (left_row >= 35) return;

    if (display_num < 2) return;

    // Right column: Program Header 1
    if (right_row >= 35) return;
    printString(COLOR_LIGHT_BLUE, right_row++, 40, (uint8_t*)"Program Header 1:");
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_type:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[1]->p_type, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_offset:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[1]->p_offset, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_vaddr:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[1]->p_vaddr, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_paddr:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[1]->p_paddr, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_filesz:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[1]->p_filesz, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_memsz:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[1]->p_memsz, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_flags:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[1]->p_flags, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_align:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[1]->p_align, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    right_row++;

    if (display_num < 3) return;

    if (right_row >= 35) return;

    // Program Header 2 on right
    printString(COLOR_LIGHT_BLUE, right_row++, 40, (uint8_t*)"Program Header 2:");
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_type:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[2]->p_type, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_offset:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[2]->p_offset, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_vaddr:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[2]->p_vaddr, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_paddr:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[2]->p_paddr, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_filesz:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[2]->p_filesz, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_memsz:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[2]->p_memsz, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_flags:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[2]->p_flags, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
    if (right_row >= 35) return;

    printString(COLOR_GREEN, right_row, 42, (uint8_t*)"p_align:");
    strcpy(buf, (uint8_t*)"0x");
    itoaHex(ph[2]->p_align, buf + 2);
    printString(COLOR_WHITE, right_row++, 60, buf);
}

void sysDiskBlockDump(uint32_t blockToDump)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    fillMemory((uint8_t*)SECTOR_AND_BLOCK_VIEWER_BUF_LOC, 0x0, PAGE_SIZE);
    uint32_t memAddressToDump = SECTOR_AND_BLOCK_VIEWER_BUF_LOC;

    readBlock(blockToDump,(uint8_t*)memAddressToDump, cachingEnabled);
    
    clearScreen();

    for (uint32_t row = 1; row < 29; row++ )
    {
        uint32_t rowStart = memAddressToDump;

        uint8_t addrStr[9];
        itoaHex(rowStart, addrStr);
        uint32_t len = strlen(addrStr);
        if (len < 8)
        {
            uint32_t shift = 8 - len;
            for (uint32_t j = len; j > 0; j--) // Shift characters including the null terminator
            {
                addrStr[j + shift - 1] = addrStr[j - 1];
            }
            for (uint32_t j = 0; j < shift; j++)
            {
                addrStr[j] = '0';
            }
            addrStr[8] = '\0';
        }
        printString(COLOR_GREEN, row, 0, addrStr);
        printString(COLOR_WHITE, row, 8, (uint8_t *)": ");

        for (uint32_t byteIndex = 0; byteIndex < 16; byteIndex++)
        {
            uint32_t column = 10 + byteIndex * 3;
            printHexNumber(COLOR_WHITE, row, column, *(uint8_t *)memAddressToDump);
            memAddressToDump++;
        }

        printString(COLOR_WHITE, row, 57, (uint8_t *)"  ");

        for (uint32_t i = 0; i < 16; i++)
        {
            uint8_t byte = *(uint8_t *)(rowStart + i);
            uint8_t toPrint = (byte >= 32 && byte <= 126) ? byte : '.';
            printCharacter(COLOR_WHITE, row, 59 + i, &toPrint);
        }
    }

    sysShowOpenFiles(30, currentPid);

}


void sysMemDump(uint32_t memAddressToDump)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    
    clearScreen();

    for (uint32_t row = 1; row < 29; row++ )
    {
        uint32_t rowStart = memAddressToDump;

        uint8_t addrStr[9];
        itoaHex(rowStart, addrStr);
        uint32_t len = strlen(addrStr);
        if (len < 8)
        {
            uint32_t shift = 8 - len;
            for (uint32_t j = len; j > 0; j--) // Shift characters including the null terminator
            {
                addrStr[j + shift - 1] = addrStr[j - 1];
            }
            for (uint32_t j = 0; j < shift; j++)
            {
                addrStr[j] = '0';
            }
            addrStr[8] = '\0';
        }
        printString(COLOR_GREEN, row, 0, addrStr);
        printString(COLOR_WHITE, row, 8, (uint8_t *)": ");

        for (uint32_t byteIndex = 0; byteIndex < 16; byteIndex++)
        {
            uint32_t column = 10 + byteIndex * 3;
            printHexNumber(COLOR_WHITE, row, column, *(uint8_t *)memAddressToDump);
            memAddressToDump++;
        }

        printString(COLOR_WHITE, row, 57, (uint8_t *)"  ");

        for (uint32_t i = 0; i < 16; i++)
        {
            uint8_t byte = *(uint8_t *)(rowStart + i);
            uint8_t toPrint = (byte >= 32 && byte <= 126) ? byte : '.';
            printCharacter(COLOR_WHITE, row, 59 + i, &toPrint);
        }
    }

    sysShowOpenFiles(30, currentPid);

}

void sysDisassemble(uint32_t memAddressToDisasm)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    
    clearScreen();

    uint8_t *reg_names[] = {(uint8_t *)"eax", (uint8_t *)"ecx", (uint8_t *)"edx", (uint8_t *)"ebx", (uint8_t *)"esp", (uint8_t *)"ebp", (uint8_t *)"esi", (uint8_t *)"edi"};

    for (uint32_t row = 1; row < 29; row++)
    {
        uint32_t rowStart = memAddressToDisasm;

        uint8_t addrStr[9];
        itoaHex(rowStart, addrStr);
        uint32_t len = strlen(addrStr);
        if (len < 8)
        {
            uint32_t shift = 8 - len;
            for (uint32_t j = len; j > 0; j--)
            {
                addrStr[j + shift - 1] = addrStr[j - 1];
            }
            addrStr[8] = '\0';
            for (uint32_t j = 0; j < shift; j++)
            {
                addrStr[j] = '0';
            }
        }
        printString(COLOR_GREEN, row, 0, addrStr);
        printString(COLOR_WHITE, row, 8, (uint8_t *)": ");

        uint8_t *ptr = (uint8_t *)memAddressToDisasm;
        uint8_t opcode = *ptr++;
        uint32_t instr_len = 1;

        uint8_t instr[16] = {0};
        uint8_t oper[64] = {0};

        strcpy(instr, (uint8_t *)"db");
        uint8_t hexStr[9];
        itoaHex(opcode, hexStr);
        strcpy(oper, (uint8_t *)"0x");
        strcpy(oper + strlen(oper), hexStr);

        if (opcode >= 0x50 && opcode <= 0x57)
        {
            strcpy(instr, (uint8_t *)"push");
            strcpy(oper, reg_names[opcode - 0x50]);
        }
        else if (opcode >= 0x58 && opcode <= 0x5F)
        {
            strcpy(instr, (uint8_t *)"pop");
            strcpy(oper, reg_names[opcode - 0x58]);
        }
        else if (opcode >= 0xB8 && opcode <= 0xBF)
        {
            strcpy(instr, (uint8_t *)"mov");
            strcpy(oper, reg_names[opcode - 0xB8]);
            strcpy(oper + strlen(oper), (uint8_t *)", 0x");
            uint32_t imm = *(uint32_t *)ptr;
            itoaHex(imm, hexStr);
            strcpy(oper + strlen(oper), hexStr);
            instr_len += 4;
            ptr += 4;
        }
        else if (opcode == 0x60)
        {
            strcpy(instr, (uint8_t *)"pusha");
            oper[0] = 0;
        }
        else if (opcode == 0x61)
        {
            strcpy(instr, (uint8_t *)"popa");
            oper[0] = 0;
        }
        else if (opcode == 0xC3)
        {
            strcpy(instr, (uint8_t *)"ret");
            oper[0] = 0;
        }
        else if (opcode == 0xCD)
        {
            strcpy(instr, (uint8_t *)"int");
            uint8_t imm = *ptr++;
            itoaHex(imm, hexStr);
            strcpy(oper, (uint8_t *)"0x");
            strcpy(oper + strlen(oper), hexStr);
            instr_len += 1;
        }
        else if (opcode == 0xE8)
        {
            strcpy(instr, (uint8_t *)"call");
            int rel = *(int *)ptr;
            uint32_t target = memAddressToDisasm + 5 + rel;
            itoaHex(target, hexStr);
            strcpy(oper, (uint8_t *)"0x");
            strcpy(oper + strlen(oper), hexStr);
            instr_len += 4;
            ptr += 4;
        }
        else if (opcode == 0xE9)
        {
            strcpy(instr, (uint8_t *)"jmp");
            int rel = *(int *)ptr;
            uint32_t target = memAddressToDisasm + 5 + rel;
            itoaHex(target, hexStr);
            strcpy(oper, (uint8_t *)"0x");
            strcpy(oper + strlen(oper), hexStr);
            instr_len += 4;
            ptr += 4;
        }
        else if (opcode == 0x0F)
        {
            uint8_t op2 = *ptr++;
            instr_len += 1;
            int rel = *(int *)ptr;
            uint32_t target = memAddressToDisasm + 6 + rel;
            itoaHex(target, hexStr);
            if (op2 == 0x84)
            {
                strcpy(instr, (uint8_t *)"je");
                strcpy(oper, (uint8_t *)"0x");
                strcpy(oper + strlen(oper), hexStr);
                instr_len += 4;
            }
            else if (op2 == 0x85)
            {
                strcpy(instr, (uint8_t *)"jne");
                strcpy(oper, (uint8_t *)"0x");
                strcpy(oper + strlen(oper), hexStr);
                instr_len += 4;
            }
            else if (op2 == 0x8E)
            {
                strcpy(instr, (uint8_t *)"jle");
                strcpy(oper, (uint8_t *)"0x");
                strcpy(oper + strlen(oper), hexStr);
                instr_len += 4;
            }
            else if (op2 == 0x8D)
            {
                strcpy(instr, (uint8_t *)"jge");
                strcpy(oper, (uint8_t *)"0x");
                strcpy(oper + strlen(oper), hexStr);
                instr_len += 4;
            }
        }
        else if (opcode == 0x81 || opcode == 0x83)
        {
            uint8_t modrm = *ptr++;
            instr_len += 1;
            uint32_t mod = (modrm >> 6) & 3;
            uint32_t ext = (modrm >> 3) & 7;
            uint32_t rm = modrm & 7;
            int has_sib = 0;
            uint8_t sib = 0;
            if (mod != 3 && rm == 4)
            {
                has_sib = 1;
                sib = *ptr++;
                instr_len += 1;
            }
            int disp = 0;
            if (mod == 1)
            {
                disp = (signed char)*ptr++;
                instr_len += 1;
            }
            else if (mod == 2)
            {
                disp = *(int *)ptr;
                ptr += 4;
                instr_len += 4;
            }
            else if (mod == 0 && rm == 5)
            {
                disp = *(int *)ptr;
                ptr += 4;
                instr_len += 4;
            }
            uint8_t disp_str[20] = {0};
            if (disp != 0)
            {
                uint8_t hex[9];
                int abs_disp = disp < 0 ? -disp : disp;
                itoaHex(abs_disp, hex);
                strcpy(disp_str, (uint8_t *)(disp < 0 ? "-" : "+"));
                strcpy(disp_str + strlen(disp_str), (uint8_t *)"0x");
                strcpy(disp_str + strlen(disp_str), hex);
            }
            uint8_t rm_str[20];
            uint32_t base = has_sib ? (sib & 7) : rm;
            if (mod == 3)
            {
                strcpy(rm_str, reg_names[rm]);
            }
            else
            {
                if (mod == 0 && rm == 5)
                {
                    uint8_t hex[9];
                    itoaHex(disp, hex);
                    strcpy(rm_str, (uint8_t *)"0x");
                    strcpy(rm_str + strlen(rm_str), hex);
                }
                else
                {
                    strcpy(rm_str, (uint8_t *)"[");
                    strcpy(rm_str + strlen(rm_str), reg_names[base]);
                    strcpy(rm_str + strlen(rm_str), disp_str);
                    strcpy(rm_str + strlen(rm_str), (uint8_t *)"]");
                }
            }
            int imm;
            if (opcode == 0x81)
            {
                imm = *(int *)ptr;
                instr_len += 4;
            }
            else
            {
                imm = (signed char)*ptr;
                instr_len += 1;
            }
            uint8_t imm_hex[9];
            itoaHex(imm < 0 ? -imm : imm, imm_hex);
            uint8_t imm_str[20];
            strcpy(imm_str, (uint8_t *)(imm < 0 ? "-0x" : "0x"));
            strcpy(imm_str + strlen(imm_str), imm_hex);
            if (ext == 0)
            {
                strcpy(instr, (uint8_t *)"add");
                strcpy(oper, rm_str);
                strcpy(oper + strlen(oper), (uint8_t *)", ");
                strcpy(oper + strlen(oper), imm_str);
            }
            else if (ext == 5)
            {
                strcpy(instr, (uint8_t *)"sub");
                strcpy(oper, rm_str);
                strcpy(oper + strlen(oper), (uint8_t *)", ");
                strcpy(oper + strlen(oper), imm_str);
            }
            else if (ext == 7)
            {
                strcpy(instr, (uint8_t *)"cmp");
                strcpy(oper, rm_str);
                strcpy(oper + strlen(oper), (uint8_t *)", ");
                strcpy(oper + strlen(oper), imm_str);
            }
        }
        else if (opcode == 0x01 || opcode == 0x29 || opcode == 0x39 || opcode == 0x89 || opcode == 0x8B)
        {
            uint8_t modrm = *ptr++;
            instr_len += 1;
            uint32_t mod = (modrm >> 6) & 3;
            uint32_t reg = (modrm >> 3) & 7;
            uint32_t rm = modrm & 7;
            int has_sib = 0;
            uint8_t sib = 0;
            if (mod != 3 && rm == 4)
            {
                has_sib = 1;
                sib = *ptr++;
                instr_len += 1;
            }
            int disp = 0;
            if (mod == 1)
            {
                disp = (signed char)*ptr++;
                instr_len += 1;
            }
            else if (mod == 2)
            {
                disp = *(int *)ptr;
                ptr += 4;
                instr_len += 4;
            }
            else if (mod == 0 && rm == 5)
            {
                disp = *(int *)ptr;
                ptr += 4;
                instr_len += 4;
            }
            uint8_t disp_str[20] = {0};
            if (disp != 0)
            {
                uint8_t hex[9];
                int abs_disp = disp < 0 ? -disp : disp;
                itoaHex(abs_disp, hex);
                strcpy(disp_str, (uint8_t *)(disp < 0 ? "-" : "+"));
                strcpy(disp_str + strlen(disp_str), (uint8_t *)"0x");
                strcpy(disp_str + strlen(disp_str), hex);
            }
            uint8_t rm_str[20];
            uint8_t reg_str[4];
            strcpy(reg_str, reg_names[reg]);
            uint32_t base = has_sib ? (sib & 7) : rm;
            if (mod == 3)
            {
                strcpy(rm_str, reg_names[rm]);
            }
            else
            {
                if (mod == 0 && rm == 5)
                {
                    uint8_t hex[9];
                    itoaHex(disp, hex);
                    strcpy(rm_str, (uint8_t *)"0x");
                    strcpy(rm_str + strlen(rm_str), hex);
                }
                else
                {
                    strcpy(rm_str, (uint8_t *)"[");
                    strcpy(rm_str + strlen(rm_str), reg_names[base]);
                    strcpy(rm_str + strlen(rm_str), disp_str);
                    strcpy(rm_str + strlen(rm_str), (uint8_t *)"]");
                }
            }
            if (opcode == 0x01)
            {
                strcpy(instr, (uint8_t *)"add");
                strcpy(oper, rm_str);
                strcpy(oper + strlen(oper), (uint8_t *)", ");
                strcpy(oper + strlen(oper), reg_str);
            }
            else if (opcode == 0x29)
            {
                strcpy(instr, (uint8_t *)"sub");
                strcpy(oper, rm_str);
                strcpy(oper + strlen(oper), (uint8_t *)", ");
                strcpy(oper + strlen(oper), reg_str);
            }
            else if (opcode == 0x39)
            {
                strcpy(instr, (uint8_t *)"cmp");
                strcpy(oper, rm_str);
                strcpy(oper + strlen(oper), (uint8_t *)", ");
                strcpy(oper + strlen(oper), reg_str);
            }
            else if (opcode == 0x89)
            {
                strcpy(instr, (uint8_t *)"mov");
                strcpy(oper, rm_str);
                strcpy(oper + strlen(oper), (uint8_t *)", ");
                strcpy(oper + strlen(oper), reg_str);
            }
            else if (opcode == 0x8B)
            {
                strcpy(instr, (uint8_t *)"mov");
                strcpy(oper, reg_str);
                strcpy(oper + strlen(oper), (uint8_t *)", ");
                strcpy(oper + strlen(oper), rm_str);
            }
        }
        else if (opcode == 0xFF)
        {
            uint8_t modrm = *ptr;
            uint32_t mod = (modrm >> 6) & 3;
            uint32_t ext = (modrm >> 3) & 7;
            uint32_t rm = modrm & 7;
            if (mod == 3 && ext == 2)
            {
                strcpy(instr, (uint8_t *)"call");
                strcpy(oper, reg_names[rm]);
                instr_len += 1;
            }
        }

        uint32_t byte_col = 10;
        for (uint32_t i = 0; i < instr_len; i++)
        {
            printHexNumber(COLOR_WHITE, row, byte_col, *(uint8_t *)(rowStart + i));
            byte_col += 3;
        }

        uint32_t instr_col = 40;
        printString(COLOR_LIGHT_BLUE, row, instr_col, instr);
        if (oper[0])
        {
            printString(COLOR_GREEN, row, instr_col + strlen(instr) + 1, oper);
        }

        memAddressToDisasm += instr_len;
    }

    sysShowOpenFiles(30, currentPid);
}

void sysUptime()
{
    // Dan O'Malley
    
    printSystemUptime(uptimeSeconds);
}

void sysSwitchToParent(uint32_t currentPid)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSPARENT",0 ,(uint8_t*)"NULL");

    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    
    uint32_t currentTaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    
    struct task *currentTask = (struct task*)currentTaskStructLocation;

    updateTaskState(currentTask->pid, PROC_SLEEPING);
    updateTaskState(currentTask->ppid, PROC_RUNNING);

    // Letting the new process know its pid
    storeValueAtMemLoc(RUNNING_PID_LOC, currentTask->ppid);

    contextSwitch(currentTask->ppid);
}

void sysWait()
{
    // Dan O'Malley
    
    // Waits one second and returns
    
    enableInterrupts();
    
    uint32_t futureSystemTimerInterruptCount = (systemTimerInterruptCount + SYSTEM_INTERRUPTS_PER_SECOND);
    
    while (systemTimerInterruptCount <= futureSystemTimerInterruptCount)
    {

    } 
    disableInterrupts();
}

void sysWaitOneInterrupt()
{
    // Dan O'Malley
    
    // Waits one second and returns
    
    enableInterrupts();
    
    uint32_t futureSystemTimerInterruptCount = (systemTimerInterruptCount + 1);
    
    while (systemTimerInterruptCount <= futureSystemTimerInterruptCount)
    {

    } 
    disableInterrupts();
}

void sysDirectory(uint32_t currentPid, uint32_t directoryInode)
{   
    // Initial version by Dan O'Malley. Extended by Grok.
    // 12/2025 with Grok v4.
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSDIR", directoryInode ,(uint8_t*)"NULL");

    uint32_t currentTaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *currentTask = (struct task*)currentTaskStructLocation;

    uint32_t cursor = 0;
    uint32_t pos = 0;
    
    fillMemory((uint8_t *)KERNEL_WORKING_DIR, 0x0, KERNEL_WORKING_DIR_SIZE);
    fillMemory((uint8_t *)KERNEL_WORKING_DIR_TEMP_INODE_LOC, 0x0, KERNEL_WORKING_DIR_TEMP_INODE_LOC_SIZE);
    loadInode(directoryInode, KERNEL_WORKING_DIR_TEMP_INODE_LOC, cachingEnabled);
    loadFileFromInodeStruct(KERNEL_WORKING_DIR_TEMP_INODE_LOC, KERNEL_WORKING_DIR, cachingEnabled);
    
    struct directoryEntry *DirectoryEntry = (directoryEntry*)(KERNEL_WORKING_DIR);
    struct ext2SuperBlock *Ext2SuperBlock = (ext2SuperBlock*)(SUPERBLOCK_LOC + 0x400); //Seems like superblock data is 0x400 into the block

    uint8_t* directoryInodeString = kMalloc(currentPid, 20);

    getFilenameFromInode(currentTask->currentDirectoryInode, directoryInodeString, cachingEnabled, ROOTDIR_INODE);
    
    printString(COLOR_GREEN, 41, 2, (uint8_t *)"Path: ");
    printString(COLOR_LIGHT_BLUE, 41, 8, (uint8_t *)"/");
    printString(COLOR_LIGHT_BLUE, 41, 9, directoryInodeString);
    printString(COLOR_GREEN, 41, 67, (uint8_t *)"Dir In: ");
    uint8_t* directoryInodeNumberString =  kMalloc(currentPid, 30);
    itoa(directoryInode, directoryInodeNumberString);
    printString(COLOR_LIGHT_BLUE, 41, 75, directoryInodeNumberString);

    kFree(directoryInodeString);
    kFree(directoryInodeNumberString);

    cursor++;
    cursor++;

    uint8_t *psUpperLeftCorner = kMalloc(currentPid, 2);
    if (psUpperLeftCorner != 0) {
        psUpperLeftCorner[0] = ASCII_UPPERLEFT_CORNER;
        psUpperLeftCorner[1] = 0;
    }

    uint8_t *psHorizontalLine = kMalloc(currentPid, 2);
    if (psHorizontalLine != 0) {
        psHorizontalLine[0] = ASCII_HORIZONTAL_LINE;
        psHorizontalLine[1] = 0;
    }
    cursor++;
    for (uint32_t columnPos = 3; columnPos < 77; columnPos++)
    {
        printString(COLOR_WHITE, cursor, columnPos, psHorizontalLine);
    }
    
    uint8_t *psVerticalLine = kMalloc(currentPid, 2);
    if (psVerticalLine != 0) {
        psVerticalLine[0] = ASCII_VERTICAL_LINE;
        psVerticalLine[1] = 0;
    }
    
    uint8_t *psUpperRightCorner = kMalloc(currentPid, 2);
    if (psUpperRightCorner != 0) {
        psUpperRightCorner[0] = ASCII_UPPERRIGHT_CORNER;
        psUpperRightCorner[1] = 0;
    }

    uint8_t *psLowerLeftCorner = kMalloc(currentPid, 2);
    if (psLowerLeftCorner != 0) {
        psLowerLeftCorner[0] = ASCII_LOWERLEFT_CORNER;
        psLowerLeftCorner[1] = 0;
    }

    uint8_t *psLowerRightCorner = kMalloc(currentPid, 2);
    if (psLowerRightCorner != 0) {
        psLowerRightCorner[0] = ASCII_LOWERRIGHT_CORNER;
        psLowerRightCorner[1] = 0;
    }

    cursor++;

    printString(COLOR_WHITE, cursor-1, 2, psUpperLeftCorner);
    printString(COLOR_RED, cursor-1, 4, (uint8_t *)"In");
    printString(COLOR_RED, cursor-1, 8, (uint8_t *)"Type");
    printString(COLOR_RED, cursor-1, 14, (uint8_t *)"Owner");
    printString(COLOR_RED, cursor-1, 20, (uint8_t *)"Group");
    printString(COLOR_RED, cursor-1, 26, (uint8_t *)"Other");
    printString(COLOR_RED, cursor-1, 32, (uint8_t *)"Bytes");
    printString(COLOR_RED, cursor-1, 40, (uint8_t *)"Filename");
    printString(COLOR_WHITE, cursor-1, 77, psUpperRightCorner);

    uint8_t *directoryFileSize = kMalloc(currentPid, 16);
    uint8_t *fileModifyTimeUnix = kMalloc(currentPid, 16);
    uint8_t *fileModifyTimeUnixYear = kMalloc(currentPid, 16);
    uint8_t *fileModifyTimeUnixDay = kMalloc(currentPid, 16);
    uint8_t *fileModifyTimeUnixHour = kMalloc(currentPid, 16);
    uint8_t *fileModifyTimeUnixMin = kMalloc(currentPid, 16);
    uint8_t *fileModifyTimeUnixSec = kMalloc(currentPid, 16);
    uint8_t *directoryFilename = kMalloc(currentPid, 20);

    while (pos < (BLOCK_SIZE * 4) && (int)DirectoryEntry->directoryInode != 0)
    {
        uint32_t name_len = DirectoryEntry->nameLength;
        //uint8_t *directoryFilename = kMalloc(currentPid, name_len + 1);
        if (directoryFilename != 0) {
            memoryCopy((uint8_t *)DirectoryEntry + 8, directoryFilename, name_len);
            directoryFilename[name_len] = 0;
        }

        printString(COLOR_WHITE, (cursor++), 2, psVerticalLine);

        fsFindFile(directoryFilename, EXT2_TEMP_INODE_STRUCTS, cachingEnabled, directoryInode);
        struct inode *Inode = (struct inode*)EXT2_TEMP_INODE_STRUCTS;

        printHexNumber(COLOR_LIGHT_BLUE, (cursor-1), 4, (uint8_t)DirectoryEntry->directoryInode);

        printString(COLOR_LIGHT_BLUE, cursor-1, 8, directoryEntryTypeTranslation((Inode->i_mode >> 12) & 0x000F));

        //Other Permissions
        printString(COLOR_RED, cursor-1, 14, octalTranslation(((Inode->i_mode >> 6) & 0b0000000000000111)));

        //Group Permissions
        printString(COLOR_RED, cursor-1, 20, octalTranslation(((Inode->i_mode >> 3) & 0b0000000000000111)));

        //User Permissions
        printString(COLOR_RED, cursor-1, 26, octalTranslation((Inode->i_mode & 0b0000000000000111)));

        if (directoryFileSize != 0) 
        {
            itoa(Inode->i_size, directoryFileSize);
            printString(COLOR_LIGHT_BLUE, cursor-1, 32, directoryFileSize);
        }

        if (directoryFilename != 0) {
            printString(COLOR_WHITE, (cursor-1), 40, directoryFilename);
        }

        printString(COLOR_WHITE, (cursor-1), 77, psVerticalLine);

        pos += DirectoryEntry->recLength;
        DirectoryEntry = (directoryEntry *)((uint32_t)DirectoryEntry + DirectoryEntry->recLength);
    }

    printString(COLOR_WHITE, cursor, 2, psLowerLeftCorner);
    printString(COLOR_WHITE, cursor, 77, psLowerRightCorner);

    for (uint32_t columnPos = 3; columnPos < 77; columnPos++)
    {
        printString(COLOR_WHITE, cursor, columnPos, psHorizontalLine);
    }

    cursor++;

    uint8_t *volumeSizeBytes = kMalloc(currentPid, 16);
    if (volumeSizeBytes != 0) {
        itoa(((Ext2SuperBlock->sb_total_blocks) * BLOCK_SIZE) / 1024000, volumeSizeBytes);
        printString(COLOR_GREEN, 0, 3, (uint8_t *)"FS Size:    MB");
        printString(COLOR_LIGHT_BLUE, 0, 12, volumeSizeBytes);
        kFree(volumeSizeBytes);
    }

    uint8_t *totalBlocksUsed = kMalloc(currentPid, 16);
    if (totalBlocksUsed != 0) {
        itoa(readTotalBlocksUsed(cachingEnabled), totalBlocksUsed);
        printString(COLOR_GREEN, 1, 3, (uint8_t *)"Total Blocks Used:");
        printString(COLOR_LIGHT_BLUE, 1, 22, totalBlocksUsed);
        kFree(totalBlocksUsed);
    }

    uint8_t *volumeRemainingBytes = kMalloc(currentPid, 16);
    if (volumeRemainingBytes != 0) {
        itoa(((Ext2SuperBlock->sb_total_blocks - readTotalBlocksUsed(cachingEnabled)) * BLOCK_SIZE) / 1024000, volumeRemainingBytes);
        printString(COLOR_GREEN, 0, 20, (uint8_t *)"Free:    MB");
        printString(COLOR_LIGHT_BLUE, 0, 26, volumeRemainingBytes);
        kFree(volumeRemainingBytes);
    }

    uint8_t *volumeTotalInodes = kMalloc(currentPid, 16);
    if (volumeTotalInodes != 0) {
        itoa((Ext2SuperBlock->sb_total_inodes), volumeTotalInodes);
        printString(COLOR_GREEN, 0, 45, (uint8_t *)"Inodes:");
        printString(COLOR_LIGHT_BLUE, 0, 53, volumeTotalInodes);
        kFree(volumeTotalInodes);
    }

    uint8_t *nextAvailableInode = kMalloc(currentPid, 16);
    if (nextAvailableInode != 0) {
        itoa(readNextAvailableInode(cachingEnabled), nextAvailableInode);
        printString(COLOR_GREEN, 1, 45, (uint8_t *)"Next Avail Inode:");
        printString(COLOR_LIGHT_BLUE, 1, 63, nextAvailableInode);
        kFree(nextAvailableInode);
    }

    uint8_t *volumeRemainingInodes = kMalloc(currentPid, 16);
    if (volumeRemainingInodes != 0) {
        itoa((Ext2SuperBlock->sb_total_unallocated_inodes), volumeRemainingInodes);
        printString(COLOR_GREEN, 0, 65, (uint8_t *)"Free:");
        printString(COLOR_LIGHT_BLUE, 0, 71, volumeRemainingInodes);
        kFree(volumeRemainingInodes);
    }

    if (psHorizontalLine != 0) kFree(psHorizontalLine);
    if (psVerticalLine != 0) kFree(psVerticalLine);
    if (psUpperLeftCorner != 0) kFree(psUpperLeftCorner);
    if (psUpperRightCorner != 0) kFree(psUpperRightCorner);
    if (psLowerLeftCorner != 0) kFree(psLowerLeftCorner);
    if (psLowerRightCorner != 0) kFree(psLowerRightCorner);
    if (directoryFilename != 0) kFree(directoryFilename);
    if (directoryFileSize != 0) kFree(directoryFileSize);
    if (fileModifyTimeUnix != 0) kFree(fileModifyTimeUnix);
    if (fileModifyTimeUnixYear != 0) kFree(fileModifyTimeUnixYear);
    if (fileModifyTimeUnixDay != 0) kFree(fileModifyTimeUnixDay);
    if (fileModifyTimeUnixHour != 0) kFree(fileModifyTimeUnixHour);
    if (fileModifyTimeUnixMin != 0) kFree(fileModifyTimeUnixMin);
    if (fileModifyTimeUnixSec != 0) kFree(fileModifyTimeUnixSec);
}


void sysToggleScheduler()
{
    // Dan O'Malley
    
    //insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSCALL", (uint8_t*)"sysToggleScheduler system call");

    struct kernelConfiguration *KernelConfiguration = (struct kernelConfiguration*)KERNEL_CONFIGURATION;

    if (KernelConfiguration->runScheduler == 0)
    {
        KernelConfiguration->runScheduler = 1;
    }
    else if (KernelConfiguration->runScheduler == 1)
    {
        KernelConfiguration->runScheduler = 0;
    }
}

void sysShowOpenFiles(uint32_t startDisplayAtRow, uint32_t currentPid)
{
    // Initial version by Dan O'Malley. Extended by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t currentTaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *currentTask = (struct task*)currentTaskStructLocation;

    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
    
    uint32_t row = startDisplayAtRow;
    uint32_t column = 0;
    uint32_t startingFileDescriptor = 0;
    uint32_t currentFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    uint8_t* currentFdString = kMalloc(currentFd, 20);
    itoaHex(currentFd, currentFdString);
    printString(COLOR_RED, row, 0, (uint8_t*)"Current Process Open Files");

    row++;

    while (startingFileDescriptor < MAX_FILE_DESCRIPTORS)
    {    
        if ((uint32_t)(currentTask->fileDescriptor[startingFileDescriptor]) >= 1)
        {
            uint32_t addressOfOpenFile = (uint32_t)currentTask->fileDescriptor[startingFileDescriptor];
            
            struct globalObjectTableEntry *gotEntry = (struct globalObjectTableEntry*)addressOfOpenFile;
        
            // Print FD
            printHexNumber(COLOR_LIGHT_BLUE, row, column, startingFileDescriptor);
            column += 3;

            // Print permissions if applicable
            if (gotEntry->openedByPid == currentPid && gotEntry->lockedForWriting == currentPid)
            {
                printCharacter(COLOR_WHITE, row, column, (uint8_t *)":");
                column += 1;
                printString(COLOR_LIGHT_BLUE, row, column, (uint8_t *)"RW");
                column += 2;
            }
            else if (gotEntry->openedByPid == currentPid && gotEntry->lockedForWriting != currentPid)
            {
                printCharacter(COLOR_WHITE, row, column, (uint8_t *)":");
                column += 1;
                printString(COLOR_LIGHT_BLUE, row, column, (uint8_t *)"RO");
                column += 2;
            }

            // Print space if permissions were printed
            if (column > 2)
            {
                printCharacter(COLOR_WHITE, row, column, (uint8_t *)" ");
                column += 1;
            }

            // Print file name
            printString(COLOR_WHITE, row, column, (uint8_t *)(uint32_t)gotEntry->name);
            // Assuming name length is handled by printString; no fixed increment

            // Move to next row for addresses
            //row++;
            column = 21;

            // Print userspace address
            printString(COLOR_GREEN, row, column, (uint8_t *)"GO UBUF:");
            column += 9;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)gotEntry->userspaceBuffer >> 24) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)gotEntry->userspaceBuffer >> 16) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)gotEntry->userspaceBuffer >> 8) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, (uint32_t)gotEntry->userspaceBuffer & 0xFF);
            column += 3;
            printString(COLOR_GREEN, row, column, (uint8_t *)"Sz:");
            column += 4;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)gotEntry->size >> 16) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)gotEntry->size >> 8) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, (uint32_t)gotEntry->size & 0xFF);
            column += 3;

            // Print space and openbuffer address
            printString(COLOR_GREEN, row, column, (uint8_t *)"OPN BUF:");
            column += 9;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)openBufferTable->buffers[startingFileDescriptor] >> 24) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)openBufferTable->buffers[startingFileDescriptor] >> 16) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)openBufferTable->buffers[startingFileDescriptor] >> 8) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, (uint32_t)openBufferTable->buffers[startingFileDescriptor] & 0xFF);
            column += 3;
            printString(COLOR_GREEN, row, column, (uint8_t *)"Sz:");
            column += 4;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)openBufferTable->bufferSize[startingFileDescriptor] >> 16) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, ((uint32_t)openBufferTable->bufferSize[startingFileDescriptor] >> 8) & 0xFF);
            column += 2;
            printHexNumber(COLOR_LIGHT_BLUE, row, column, (uint32_t)openBufferTable->bufferSize[startingFileDescriptor] & 0xFF);
            // Move to next block
            row++;
            //row++;
            column = 0;
        }

        startingFileDescriptor++;
    }

    column = 0;
    printString(COLOR_GREEN, row, column, (uint8_t*)"Current FD:");
    printString(COLOR_WHITE, row, column+=12, currentFdString);

    column = 21;
    printString(COLOR_GREEN, row, column, (uint8_t*)"Current FD PTR:");
    column += 16;
    printHexNumber(COLOR_LIGHT_BLUE, row, column, (*(uint32_t*)CURRENT_FILE_DESCRIPTOR_PTR >> 24) & 0xFF);
    column += 2;
    printHexNumber(COLOR_LIGHT_BLUE, row, column, (*(uint32_t*)CURRENT_FILE_DESCRIPTOR_PTR >> 16) & 0xFF);
    column += 2;
    printHexNumber(COLOR_LIGHT_BLUE, row, column, (*(uint32_t*)CURRENT_FILE_DESCRIPTOR_PTR >> 8) & 0xFF);
    column += 2;
    printHexNumber(COLOR_LIGHT_BLUE, row, column, *(uint32_t*)CURRENT_FILE_DESCRIPTOR_PTR & 0xFF);


    kFree(currentFdString);
}

void sysCreate(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSCREATE", directoryInode ,FileParameter->fileName);

    createFile(FileParameter->fileName, currentPid, FileParameter->fileDescriptor, cachingEnabled, directoryInode);
}

void sysMove(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSMOVE", directoryInode ,FileParameter->fileName);

    moveFile(FileParameter->fileName, FileParameter->sourceDirectory, FileParameter->destinationDirectory, cachingEnabled);
}

void sysDelete(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSDELETE",directoryInode , FileParameter->fileName);

    deleteFile(FileParameter->fileName, currentPid, cachingEnabled, directoryInode);
}

void sysOpenEmpty(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode)
{
    // Dan O'Malley
    
    uint32_t GOTESize = FileParameter->requestedSizeInPages * PAGE_SIZE;
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSOPNEMPTY", directoryInode , FileParameter->fileName);

    uint8_t *newBinaryFilenameLoc = kMalloc(currentPid, FileParameter->fileNameLength);
    strcpyRemoveNewline(newBinaryFilenameLoc, FileParameter->fileName);

    uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *Task = (struct task*)taskStructLocation;

    if (Task->nextAvailableFileDescriptor >= MAX_FILE_DESCRIPTORS) 
    {
        return; // reached the max open files
    }

    uint32_t pagesNeedForTmpBinary = FileParameter->requestedSizeInPages;

    uint8_t *requestedBuffer = findBuffer(currentPid, pagesNeedForTmpBinary, PG_USER_PRESENT_RW);

    //request block of pages for temporary file storage to load file based on first available page above
    for (uint32_t pageCount = 0; pageCount < pagesNeedForTmpBinary; pageCount++)
    {                
        if (!requestSpecificPage(currentPid, (uint8_t *)((uint32_t)requestedBuffer + (pageCount * PAGE_SIZE)), PG_USER_PRESENT_RW))
        {
            clearScreen();
            printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
            panic((uint8_t *)"syscalls.cpp -> USER_TEMP_FILE_LOC page request");
        }

        // Zero each page in case it has been used previously
        fillMemory((uint8_t *)((uint32_t)requestedBuffer + (pageCount * PAGE_SIZE)), 0x0, PAGE_SIZE);
    }

    Task->fileDescriptor[Task->nextAvailableFileDescriptor] = (globalObjectTableEntry *)insertGlobalObjectTableEntry((uint8_t *)GLOBAL_OBJECT_TABLE, currentPid, 0, GOTE_TYPE_FILE, 0, 0, 0, 0, GOTESize, requestedBuffer, pagesNeedForTmpBinary, 0, newBinaryFilenameLoc, 0, 0, 0);
    struct globalObjectTableEntry *GOTE = (globalObjectTableEntry *)Task->fileDescriptor[Task->nextAvailableFileDescriptor];

    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;

    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR, ((uint32_t)Task->nextAvailableFileDescriptor));
    storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[Task->nextAvailableFileDescriptor], (int)requestedBuffer);
    storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[Task->nextAvailableFileDescriptor], (int)GOTE->size);
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR_PTR, (uint32_t)requestedBuffer);

    createFile(newBinaryFilenameLoc, currentPid, Task->nextAvailableFileDescriptor, cachingEnabled, directoryInode);

    //sysClose(Task->nextAvailableFileDescriptor, currentPid);

}

void sysCreatePipe(struct fileParameter *FileParameter, uint32_t currentPid) 
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSCPIPE", FileParameter->fileDescriptor, FileParameter->fileName);

    uint8_t *pipeName = kMalloc(currentPid, FileParameter->fileNameLength);
    strcpyRemoveNewline(pipeName, FileParameter->fileName);

    struct task *Task = (struct task*)(PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1)));

    uint8_t *requestedBuffer = findBuffer(currentPid, 1, PG_USER_PRESENT_RW);
            
    if (!requestSpecificPage(currentPid, requestedBuffer, PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"syscalls.cpp -> PIPE page request");
    }

    // Zero each page in case it has been used previously
    fillMemory(requestedBuffer, 0x0, PAGE_SIZE);

    uint8_t *kBufferAssigned = (uint8_t*)kMalloc(currentPid, PIPE_BUF_SIZE);

    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
    Task->fileDescriptor[Task->nextAvailableFileDescriptor] = (globalObjectTableEntry *)insertGlobalObjectTableEntry((uint8_t *)GLOBAL_OBJECT_TABLE, currentPid, 0, GOTE_TYPE_PIPE, 0, 0, 0, 0, PIPE_BUF_SIZE, requestedBuffer, 1, kBufferAssigned, pipeName, 0, 0, currentPid);
    struct globalObjectTableEntry *GOTE = (globalObjectTableEntry *)Task->fileDescriptor[Task->nextAvailableFileDescriptor];
    
    storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[Task->nextAvailableFileDescriptor], (uint32_t)requestedBuffer);
    storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[Task->nextAvailableFileDescriptor], (uint32_t)GOTE->size);
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR, ((uint32_t)Task->nextAvailableFileDescriptor));
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR_PTR, (uint32_t)requestedBuffer);
    
    Task->nextAvailableFileDescriptor++;
}

void sysCreateNetworkPipe(struct networkParameter *NetworkParameter, uint32_t currentPid)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSCNPIPE", NetworkParameter->destinationIPAddress ,NetworkParameter->socketName);

    uint8_t *socketName = kMalloc(currentPid, 20);
    strcpyRemoveNewline(socketName, NetworkParameter->socketName);

    struct task *Task = (struct task*)(PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1)));

    uint8_t *requestedBuffer = findBuffer(currentPid, 1, PG_USER_PRESENT_RW);
            
    if (!requestSpecificPage(currentPid, requestedBuffer, PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"syscalls.cpp -> PIPE page request");
    }

    // Zero each page in case it has been used previously
    fillMemory(requestedBuffer, 0x0, PAGE_SIZE);

    //uint8_t *kBufferAssigned = (uint8_t*)kMalloc(currentPid, PIPE_BUF_SIZE);
    uint8_t *kBufferAssigned = (uint8_t*)NETWORK_INCOMING_PAYLOAD_BUFFER;

    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
    Task->fileDescriptor[Task->nextAvailableFileDescriptor] = (globalObjectTableEntry *)insertGlobalObjectTableEntry((uint8_t *)GLOBAL_OBJECT_TABLE, currentPid, 0, GOTE_TYPE_SOCKET, NetworkParameter->sourceIPAddress, NetworkParameter->destinationIPAddress, NetworkParameter->sourcePort, NetworkParameter->destinationPort, PIPE_BUF_SIZE, requestedBuffer, 1, kBufferAssigned, socketName, 0, 0, currentPid);
    struct globalObjectTableEntry *GOTE = (globalObjectTableEntry *)Task->fileDescriptor[Task->nextAvailableFileDescriptor];
    
    storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[Task->nextAvailableFileDescriptor], (uint32_t)requestedBuffer);
    storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[Task->nextAvailableFileDescriptor], (uint32_t)GOTE->size);
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR, ((uint32_t)Task->nextAvailableFileDescriptor));
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR_PTR, (uint32_t)requestedBuffer);
    
    Task->nextAvailableFileDescriptor++;
}

uint32_t sysOpenPipe(struct fileParameter *FileParameter, uint32_t currentPid) 
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSOPIPE",FileParameter->fileDescriptor ,FileParameter->fileName);

    uint8_t *pipeName = kMalloc(currentPid, FileParameter->fileNameLength);
    strcpyRemoveNewline(pipeName, FileParameter->fileName);

    struct task *Task = (struct task*)(PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1)));

    uint8_t *requestedBuffer = findBuffer(currentPid, 1, PG_USER_PRESENT_RW);
            
    if (!requestSpecificPage(currentPid, requestedBuffer, PG_USER_PRESENT_RW))
    {
        clearScreen();
        printString(COLOR_RED, 2, 2, (uint8_t *)"Requested page is not available");
        panic((uint8_t *)"syscalls.cpp -> PIPE page request");
    }

    // Zero each page in case it has been used previously
    fillMemory(requestedBuffer, 0x0, PAGE_SIZE);

    struct globalObjectTableEntry *GlobalObjectTableEntry = 0;
    struct globalObjectTableEntry *GlobalObjectTableEntryFound = 0;
    uint32_t tableEntryNumber = 0;

    GlobalObjectTableEntry = (struct globalObjectTableEntry*)GLOBAL_OBJECT_TABLE;
    while (tableEntryNumber < MAX_SYSTEM_OPEN_FILES)
    {
        if (GlobalObjectTableEntry->type == GOTE_TYPE_PIPE && (!strcmp(GlobalObjectTableEntry->name, pipeName)))
        {
            GlobalObjectTableEntryFound = GlobalObjectTableEntry;
            break;
        }

        GlobalObjectTableEntry++;
        tableEntryNumber++;
    }

    if (GlobalObjectTableEntryFound == 0)
    {
        //printString(COLOR_RED, 3, 2, (uint8_t *)"No pipe by that name!");
        //sysWait();
        
        return SYSCALL_FAIL;
    }

    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
    GlobalObjectTableEntry = (globalObjectTableEntry *)insertGlobalObjectTableEntry((uint8_t *)GLOBAL_OBJECT_TABLE, currentPid, 0, GOTE_TYPE_PIPE, 0, 0, 0, 0, PIPE_BUF_SIZE, requestedBuffer, 1, GlobalObjectTableEntryFound->kernelSpaceBuffer, pipeName, 0, 0, 0);

    Task->fileDescriptor[Task->nextAvailableFileDescriptor] = (globalObjectTableEntry *)GlobalObjectTableEntry;
    struct globalObjectTableEntry *GOTE = (globalObjectTableEntry *)Task->fileDescriptor[Task->nextAvailableFileDescriptor];
    
    storeValueAtMemLoc((uint8_t *)&openBufferTable->buffers[Task->nextAvailableFileDescriptor], (uint32_t)requestedBuffer);
    storeValueAtMemLoc((uint8_t *)&openBufferTable->bufferSize[Task->nextAvailableFileDescriptor], (uint32_t)GOTE->size);
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR, ((uint32_t)Task->nextAvailableFileDescriptor));
    storeValueAtMemLoc(CURRENT_FILE_DESCRIPTOR_PTR, (uint32_t)requestedBuffer);

    Task->nextAvailableFileDescriptor++;

    return SYSCALL_SUCCESS;

}

void sysRead(uint32_t fileDescriptorToRead, uint32_t currentPid)
{
    // Dan O'Malley
    
    insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSREAD", fileDescriptorToRead ,(uint8_t*)"NULL");

    uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *Task = (struct task*)taskStructLocation;
    struct globalObjectTableEntry *GlobalObjectTableEntry = (globalObjectTableEntry*)(uint32_t)Task->fileDescriptor[fileDescriptorToRead];
        
    if ((uint32_t)Task->fileDescriptor[fileDescriptorToRead] == 0x0)
    {
        printString(COLOR_RED, 3, 5, (uint8_t *)"No such file descriptor!");
        sysWait();
        sysWait();
        sysWait();
        clearScreen();
        return;
    }

    if (GlobalObjectTableEntry->type == GOTE_TYPE_PIPE) 
    { 
        uint8_t *userBuf = GlobalObjectTableEntry->userspaceBuffer; 

        uint32_t tableEntryNumber = 0;
        struct globalObjectTableEntry *GlobalObjectTableEntryFound = 0;
        struct globalObjectTableEntry *GlobalObjectTableEntryWriter = 0;

        GlobalObjectTableEntryWriter = (struct globalObjectTableEntry*)GLOBAL_OBJECT_TABLE;
        while (tableEntryNumber < MAX_SYSTEM_OPEN_FILES)
        {
            if (GlobalObjectTableEntryWriter->kernelSpaceBuffer == GlobalObjectTableEntry->kernelSpaceBuffer)
            {
                GlobalObjectTableEntryFound = GlobalObjectTableEntryWriter;
                break;
            }

            GlobalObjectTableEntryWriter++;
            tableEntryNumber++;
        }

        if (GlobalObjectTableEntryWriter->writeOffset == 0)
        {
            printString(COLOR_RED, 3, 2, (uint8_t *)"Pipe is empty and cannot be read until writer produces");
            sysWait();
            sysWait();
            sysWait();
            return;
        }

        
        while (!acquireLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}


        bytecpy(userBuf, GlobalObjectTableEntry->kernelSpaceBuffer, PIPE_BUF_SIZE); 
        GlobalObjectTableEntry->writeOffset = PIPE_BUF_SIZE - 1;
        GlobalObjectTableEntryWriter->readOffset = GlobalObjectTableEntry->readOffset;
        GlobalObjectTableEntryWriter->writeOffset = 0;

        releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE);

        return;

    }

    if (GlobalObjectTableEntry->type == GOTE_TYPE_SOCKET)
    {
        netUDPRcv();

        uint8_t *userBuf = GlobalObjectTableEntry->userspaceBuffer; 
        fillMemory(GlobalObjectTableEntry->userspaceBuffer, 0x0, PIPE_BUF_SIZE);

        uint32_t tableEntryNumber = 0;
        struct globalObjectTableEntry *GlobalObjectTableEntryFound = 0;
        struct globalObjectTableEntry *GlobalObjectTableEntryWriter = 0;

        GlobalObjectTableEntryWriter = (struct globalObjectTableEntry*)GLOBAL_OBJECT_TABLE;
        while (tableEntryNumber < MAX_SYSTEM_OPEN_FILES)
        {
            if (GlobalObjectTableEntryWriter->kernelSpaceBuffer == GlobalObjectTableEntry->kernelSpaceBuffer)
            {
                GlobalObjectTableEntryFound = GlobalObjectTableEntryWriter;
                break;
            }

            GlobalObjectTableEntryWriter++;
            tableEntryNumber++;
        }
        
        while (!acquireLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}

        bytecpy(userBuf, GlobalObjectTableEntry->kernelSpaceBuffer, PIPE_BUF_SIZE); 
        GlobalObjectTableEntry->writeOffset = 0;
        GlobalObjectTableEntryWriter->readOffset = 0;
        GlobalObjectTableEntryWriter->writeOffset = 0;

        releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE);

        //fillMemory(GlobalObjectTableEntry->kernelSpaceBuffer, 0x0, PIPE_BUF_SIZE);

        return;
    }


}

uint32_t sysWhatPidAmI()
{
    // Dan O'Malley
    
    //insertKernelLog(totalInterruptCount, currentPid, (uint8_t*)"SYSCALL", (uint8_t*)"sysWhatPidAmI system call");

    uint32_t currentPidPageDir;
    
    asm volatile ("movl %cr3, %eax\n\t");
    asm volatile ("movl %%eax, %0\n\t" : "=r" (currentPidPageDir) : );

    return uint32_t(PAGE_DIR_BASE - currentPidPageDir / MAX_PGTABLES_SIZE);
}

void sysChangeDirectory(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *Task = (struct task*)taskStructLocation;

    uint8_t *newDirectoryName = kMalloc(currentPid, FileParameter->fileNameLength);
    strcpyRemoveNewline(newDirectoryName, FileParameter->fileName);

    uint32_t newDirectoryInode = returnInodeofFileName(newDirectoryName, cachingEnabled, directoryInode);
    loadInode(newDirectoryInode, KERNEL_WORKING_DIR_TEMP_INODE_LOC, cachingEnabled);

    struct inode *Inode = (struct inode*)KERNEL_WORKING_DIR_TEMP_INODE_LOC;
    if (strcmp(directoryEntryTypeTranslation((Inode->i_mode >> 12) & 0x000F), (uint8_t*)"DIR"))
    {
        printString(COLOR_RED, 20, 5, (uint8_t*)"This is not a directory.");
        sysWait();
        sysWait();
        return;
    }

    Task->currentDirectoryInode = newDirectoryInode;

    kFree(newDirectoryName);
    
}

void insertKernelLog(uint32_t interruptCount, uint32_t pid, uint8_t *type, uint32_t value, uint8_t *description) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    static uint32_t next_write = 0;
    struct kernelLog *log = (struct kernelLog *)KERNEL_LOG_LOC;
    log[next_write].interruptCount = interruptCount;
    log[next_write].pid = pid;
    strcpy(log[next_write].type, type);
    log[next_write].value = value;
    strcpy(log[next_write].description, description);
    next_write = (next_write + 1) % KERNEL_LOG_MAX_EVENTS;
}

uint32_t getLatestTypeExample(uint8_t* typeToFind) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    struct kernelLog *log = (struct kernelLog *)KERNEL_LOG_LOC;
    uint32_t maxInterrupt = 0;
    uint32_t foundPID = 0;

    for (uint32_t i = 0; i < KERNEL_LOG_MAX_EVENTS; i++) 
    {
        if (strcmp(log[i].type, typeToFind) == 0) 
        {
            if (log[i].interruptCount > maxInterrupt) 
            {
                maxInterrupt = log[i].interruptCount;
                foundPID = log[i].pid;
            }
        }
    }

    return foundPID;
}

void printKernelLog(uint32_t currentPid) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    clearScreen();
    printString(COLOR_LIGHT_BLUE, 0, 0, (uint8_t *)"Kernel Log");
    printString(COLOR_RED, 2, 1, (uint8_t *)"ID");
    printString(COLOR_RED, 2, 6, (uint8_t *)"Interrupt");
    printString(COLOR_RED, 2, 17, (uint8_t *)"PID");
    printString(COLOR_RED, 2, 22, (uint8_t *)"Type");
    printString(COLOR_RED, 2, 37, (uint8_t *)"Value");
    printString(COLOR_RED, 2, 48, (uint8_t *)"Description");

    struct kernelLog *log = (struct kernelLog *)KERNEL_LOG_LOC;
    uint32_t count = 0;
    for (uint32_t i = 0; i < KERNEL_LOG_MAX_EVENTS; i++) 
    {
        if (log[i].interruptCount != 0) 
        {
            count++;
        }
    }

    if (count == 0) 
    {
        printString(COLOR_RED, 4, 1, (uint8_t *)"No log entries available.");
        return;
    }

    uint32_t indices[KERNEL_LOG_MAX_EVENTS];
    uint32_t valid_index = 0;
    for (uint32_t i = 0; i < KERNEL_LOG_MAX_EVENTS; i++) 
    {
        if (log[i].interruptCount != 0) 
        {
            indices[valid_index++] = i;
        }
    }

    // Bubble sort indices by interruptCount descending
    for (uint32_t i = 0; i < count - 1; i++) 
    {
        for (uint32_t j = 0; j < count - i - 1; j++) 
        {
            if (log[indices[j]].interruptCount < log[indices[j + 1]].interruptCount) 
            {
                uint32_t temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }

    uint32_t max_display_lines = 31;
    uint32_t display_start = 0;
    uint8_t isShiftDown = 0;

    while (true)
    {
        clearScreen();
        printString(COLOR_LIGHT_BLUE, 0, 0, (uint8_t *)"Kernel Log");
        printString(COLOR_RED, 2, 1, (uint8_t *)"ID");
        printString(COLOR_RED, 2, 6, (uint8_t *)"Interrupt");
        printString(COLOR_RED, 2, 17, (uint8_t *)"PID");
        printString(COLOR_RED, 2, 22, (uint8_t *)"Type");
        printString(COLOR_RED, 2, 37, (uint8_t *)"Value");
        printString(COLOR_RED, 2, 48, (uint8_t *)"Description");

        uint32_t row = 4;
        for (uint32_t i = display_start; i < display_start + max_display_lines && i < count; i++) 
        {
            uint32_t idx = indices[i];
            uint8_t *idLoc = kMalloc(currentPid, 10);
            itoa(idx + 1, idLoc);
            printString(COLOR_WHITE, row, 1, idLoc);

            uint8_t *icLoc = kMalloc(currentPid, 20);
            itoa(log[idx].interruptCount, icLoc);
            printString(COLOR_GREEN, row, 6, icLoc);

            uint8_t *pidLoc = kMalloc(currentPid, 10);
            itoa(log[idx].pid, pidLoc);
            printString(COLOR_LIGHT_BLUE, row, 17, pidLoc);

            printString(COLOR_WHITE, row, 22, log[idx].type);

            uint8_t *valLoc = kMalloc(currentPid, 20);
            itoaHex(log[idx].value, valLoc);
            printString(COLOR_GREEN, row, 37, valLoc);

            printString(COLOR_LIGHT_BLUE, row, 48, log[idx].description);

            kFree(idLoc);
            kFree(icLoc);
            kFree(pidLoc);
            kFree(valLoc);

            row++;
        }

        printString(COLOR_GREEN, 35, 1, (uint8_t *)"Use Page Up/Down to scroll, Enter to exit");

        uint8_t tempScanCode;
        uint8_t scanCode;
        do
        {
            readKeyboard(&tempScanCode);
            if (tempScanCode == 0x2A || tempScanCode == 0x36)
            {
                isShiftDown = 1;
                continue;
            }
            if (tempScanCode == 0xAA || tempScanCode == 0xB6)
            {
                isShiftDown = 0;
                continue;
            }
            if (tempScanCode & 0x80)
            {
                continue;
            }
            scanCode = tempScanCode;
            break;
        } while (1);

        if (scanCode == 0x1C)
        { // Enter key
            break;
        }
        else if (scanCode == 0x49) // Page Up
        {
            if (display_start > max_display_lines)
            {
                display_start -= max_display_lines;
            }
            else
            {
                display_start = 0;
            }
        }
        else if (scanCode == 0x51) // Page Down
        {
            if (display_start + max_display_lines < count)
            {
                display_start += max_display_lines;
            }
        }
    }
}

void sysShowGlobalObjects(uint32_t currentPid)
{
    // Dan O'Malley    

    uint32_t cursor = 0;
    struct globalObjectTableEntry *GlobalObjectTableEntry;
    
    uint8_t *psUpperLeftCorner = kMalloc(currentPid, sizeof(uint8_t));
    *psUpperLeftCorner = ASCII_UPPERLEFT_CORNER;

    uint8_t *psHorizontalLine = kMalloc(currentPid, sizeof(uint8_t));
    *psHorizontalLine = ASCII_HORIZONTAL_LINE;
    cursor++;
    cursor++;
    for (uint32_t columnPos = 3; columnPos < 77; columnPos++)
    {
        printString(COLOR_WHITE, cursor, columnPos, psHorizontalLine);
    }
    
    uint8_t *psVerticalLine = kMalloc(currentPid, sizeof(uint8_t));
    *psVerticalLine = ASCII_VERTICAL_LINE;
    
    uint8_t *psUpperRightCorner = kMalloc(currentPid, sizeof(uint8_t));
    *psUpperRightCorner = ASCII_UPPERRIGHT_CORNER;

    uint8_t *psLowerLeftCorner = kMalloc(currentPid, sizeof(uint8_t));
    *psLowerLeftCorner = ASCII_LOWERLEFT_CORNER;

    uint8_t *psLowerRightCorner = kMalloc(currentPid, sizeof(uint8_t));
    *psLowerRightCorner = ASCII_LOWERRIGHT_CORNER;


    cursor++;
    printString(COLOR_LIGHT_BLUE, 0, 2, (uint8_t *)"Global Object Table Entries");
    printString(COLOR_WHITE, cursor-1, 2, psUpperLeftCorner);
    printString(COLOR_RED, cursor-1, 4, (uint8_t *)"Name");
    printString(COLOR_RED, cursor-1, 16, (uint8_t *)"Type");
    printString(COLOR_RED, cursor-1, 24, (uint8_t *)"PID");
    printString(COLOR_RED, cursor-1, 28, (uint8_t *)"Lk");
    printString(COLOR_RED, cursor-1, 38, (uint8_t *)"R-Pos");
    printString(COLOR_RED, cursor-1, 44, (uint8_t *)"W-Pos");
    printString(COLOR_RED, cursor-1, 51, (uint8_t *)"In");
    printString(COLOR_RED, cursor-1, 55, (uint8_t *)"Size");
    printString(COLOR_RED, cursor-1, 62, (uint8_t *)"U-Buf");
    printString(COLOR_RED, cursor-1, 70, (uint8_t *)"K-Buf");
    printString(COLOR_WHITE, cursor-1, 77, psUpperRightCorner);

    for (uint32_t GlobalObjectTableEntryNumber = 0; GlobalObjectTableEntryNumber < MAX_SYSTEM_OPEN_FILES; GlobalObjectTableEntryNumber++)
    {
        GlobalObjectTableEntry = (struct globalObjectTableEntry*)(GLOBAL_OBJECT_TABLE + (sizeof(globalObjectTableEntry) * GlobalObjectTableEntryNumber));

        if (GlobalObjectTableEntry->type != 0)
        {
            printString(COLOR_WHITE, (cursor++), 2, psVerticalLine);

            printString(COLOR_WHITE, cursor-1, 4, GlobalObjectTableEntry->name);

            if ((uint32_t)GlobalObjectTableEntry->type == GOTE_TYPE_BUFFER) {printString(COLOR_LIGHT_BLUE, (cursor -1), 16, (uint8_t *)"BUFFER");}
            if ((uint32_t)GlobalObjectTableEntry->type == GOTE_TYPE_FILE) {printString(COLOR_LIGHT_BLUE, (cursor -1), 16, (uint8_t *)"FILE");}
            if ((uint32_t)GlobalObjectTableEntry->type == GOTE_TYPE_PIPE) {printString(COLOR_LIGHT_BLUE, (cursor -1), 16, (uint8_t *)"PIPE");}
            if ((uint32_t)GlobalObjectTableEntry->type == GOTE_TYPE_SOCKET) {printString(COLOR_LIGHT_BLUE, (cursor -1), 16, (uint8_t *)"SOCKET");}

            printHexNumber(COLOR_LIGHT_BLUE, cursor-1, 24, GlobalObjectTableEntry->openedByPid);

            printHexNumber(COLOR_LIGHT_BLUE, cursor-1, 28, GlobalObjectTableEntry->lockedForWriting);

            uint8_t *readPos = kMalloc(currentPid, 10);
            itoaHex((uint32_t)(GlobalObjectTableEntry->readOffset), readPos);
            printString(COLOR_GREEN, cursor-1, 38, readPos);
            kFree(readPos);

            uint8_t *writePos = kMalloc(currentPid, 10);
            itoaHex((uint32_t)(GlobalObjectTableEntry->writeOffset), writePos);
            printString(COLOR_GREEN, cursor-1, 44, writePos);
            kFree(writePos);

            printHexNumber(COLOR_LIGHT_BLUE, cursor-1, 51, GlobalObjectTableEntry->inode);

            uint8_t *fileSize = kMalloc(currentPid, 10);
            itoaHex((uint32_t)(GlobalObjectTableEntry->size), fileSize);
            printString(COLOR_GREEN, cursor-1, 55, fileSize);
            kFree(fileSize);

            uint8_t *userspaceAddress = kMalloc(currentPid, 10);
            itoaHex((uint32_t)(GlobalObjectTableEntry->userspaceBuffer), userspaceAddress);
            printString(COLOR_GREEN, cursor-1, 62, userspaceAddress);
            kFree(userspaceAddress);

            uint8_t *kernelspaceAddress = kMalloc(currentPid, 10);
            itoaHex((uint32_t)(GlobalObjectTableEntry->kernelSpaceBuffer), kernelspaceAddress);
            printString(COLOR_RED, cursor-1, 70, kernelspaceAddress);
            kFree(kernelspaceAddress);

            printString(COLOR_WHITE, (cursor), 77, psVerticalLine);
            printString(COLOR_WHITE, (cursor-1), 77, psVerticalLine);
        }
    }

    printString(COLOR_WHITE, (cursor), 2, psVerticalLine);

    printString(COLOR_WHITE, cursor, 2, psLowerLeftCorner);
    printString(COLOR_WHITE, cursor, 77, psLowerRightCorner);

    for (uint32_t columnPos = 3; columnPos < 77; columnPos++)
    {
        printString(COLOR_WHITE, cursor, columnPos, psHorizontalLine);
    }


    cursor++;
    cursor++;

    for (uint32_t columnPos = 3; columnPos < 77; columnPos++)
    {
        printString(COLOR_WHITE, cursor+1, columnPos, psHorizontalLine);
    }

    printString(COLOR_LIGHT_BLUE, cursor++, 2, (uint8_t *)"Open Socket Detail");
    printString(COLOR_WHITE, cursor, 2, psUpperLeftCorner);
    printString(COLOR_RED, cursor, 4, (uint8_t *)"Name");
    printString(COLOR_RED, cursor, 16, (uint8_t *)"Src-IP");
    printString(COLOR_RED, cursor, 29, (uint8_t *)"Src-Pt");
    printString(COLOR_RED, cursor, 37, (uint8_t *)"Dst-IP");
    printString(COLOR_RED, cursor, 49, (uint8_t *)"Dst-Pt");
    printString(COLOR_RED, cursor, 60, (uint8_t *)"TX");
    printString(COLOR_RED, cursor, 65, (uint8_t *)"RX");

    printString(COLOR_WHITE, cursor, 77, psUpperRightCorner);

    cursor++;
    cursor++;

    printString(COLOR_WHITE, (cursor-1), 2, psVerticalLine);
    printString(COLOR_WHITE, (cursor-1), 77, psVerticalLine);

    for (uint32_t GlobalObjectTableEntryNumber = 0; GlobalObjectTableEntryNumber < MAX_SYSTEM_OPEN_FILES; GlobalObjectTableEntryNumber++)
    {
        GlobalObjectTableEntry = (struct globalObjectTableEntry*)(GLOBAL_OBJECT_TABLE + (sizeof(globalObjectTableEntry) * GlobalObjectTableEntryNumber));

        if (GlobalObjectTableEntry->type == GOTE_TYPE_SOCKET)
        {
            printString(COLOR_WHITE, (cursor), 2, psVerticalLine);
            printString(COLOR_WHITE, cursor-1, 4, GlobalObjectTableEntry->name);

            uint8_t *sourceIPAddress = kMalloc(currentPid, 10);
            itoIPAddressString((uint32_t)(GlobalObjectTableEntry->sourceIPAddress), sourceIPAddress);
            printString(COLOR_GREEN, cursor-1, 16, sourceIPAddress);
            kFree(sourceIPAddress);

            uint8_t *sourcePort = kMalloc(currentPid, 10);
            itoa((uint32_t)(GlobalObjectTableEntry->sourcePort), sourcePort);
            printString(COLOR_GREEN, cursor-1, 29, sourcePort);
            kFree(sourcePort);

            uint8_t *destinationIPAddress = kMalloc(currentPid, 10);
            itoIPAddressString((uint32_t)(GlobalObjectTableEntry->destinationIPAddress), destinationIPAddress);
            printString(COLOR_GREEN, cursor-1, 37, destinationIPAddress);
            kFree(destinationIPAddress);

            uint8_t *destinationPort = kMalloc(currentPid, 10);
            itoa((uint32_t)(GlobalObjectTableEntry->destinationPort), destinationPort);
            printString(COLOR_GREEN, cursor-1, 49, destinationPort);
            kFree(destinationPort);

            uint8_t *packetsTransmitted = kMalloc(currentPid, 10);
            itoa((uint32_t)(GlobalObjectTableEntry->packetsTransmitted), packetsTransmitted);
            printString(COLOR_RED, cursor-1, 60, packetsTransmitted);
            kFree(packetsTransmitted);

            uint8_t *packetsReceived = kMalloc(currentPid, 10);
            itoa((uint32_t)(GlobalObjectTableEntry->packetsReceived), packetsReceived);
            printString(COLOR_RED, cursor-1, 65, packetsReceived);
            kFree(packetsReceived);


            printString(COLOR_WHITE, (cursor), 77, psVerticalLine);
            printString(COLOR_WHITE, (cursor-1), 77, psVerticalLine);
            cursor++;
        }
    }

    printString(COLOR_WHITE, (cursor-1), 2, psVerticalLine);

    printString(COLOR_WHITE, cursor-1, 2, psLowerLeftCorner);
    printString(COLOR_WHITE, cursor-1, 77, psLowerRightCorner);

    for (uint32_t columnPos = 3; columnPos < 77; columnPos++)
    {
        printString(COLOR_WHITE, cursor-1, columnPos, psHorizontalLine);
    }

    kFree(psHorizontalLine);
    kFree(psVerticalLine);
    kFree(psUpperLeftCorner);
    kFree(psUpperRightCorner);
    kFree(psLowerLeftCorner);
    kFree(psLowerRightCorner);

    sysShowOpenFiles(30, currentPid);
}

void syscallHandler()
{     
    // Dan O'Malley
    
    // This is very sensitive (guru code below).
    // Don't Touch!
    asm volatile ("pusha\n\t");
   
    uint32_t syscallNumber;
    uint32_t arg1;
    uint32_t currentPid;
    uint32_t directoryInode;
    returnedValueFromSyscallFunction = SYSCALL_FAIL; // set to fail by default
    asm volatile ("movl %%eax, %0\n\t" : "=r" (syscallNumber) : );
    asm volatile ("movl %%ebx, %0\n\t" : "=r" (arg1) : );
    asm volatile ("movl %%ecx, %0\n\t" : "=r" (currentPid) : );

    sysHandlertaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    SysHandlerTask = (struct task*)sysHandlertaskStructLocation;
    directoryInode = SysHandlerTask->currentDirectoryInode;

    // These manually grab from the stack are very sensitive to local variables.
    // The stack prior to iret should be:
    // (top to bottom) -> EIP -> CS -> UKNOWN (EAX) -> ESP

    asm volatile ("movl %esp, %edx\n\t");
    asm volatile ("add $8, %edx\n\t");
    asm volatile ("movl (%edx), %edx\n\t");
    asm volatile ("movl %%edx, %0\n\t" : "=r" ((uint32_t)(SysHandlerTask->ebp)) : );

    asm volatile ("movl %esp, %edx\n\t");
    asm volatile ("add $60, %edx\n\t");
    asm volatile ("movl (%edx), %edx\n\t");
    asm volatile ("movl %%edx, %0\n\t" : "=r" ((uint32_t)(SysHandlerTask->eip)) : );

    asm volatile ("movl %esp, %edx\n\t");
    asm volatile ("add $64, %edx\n\t");
    asm volatile ("movl (%edx), %edx\n\t");
    asm volatile ("mov %%dx, %0\n\t" : "=r" ((uint16_t)(SysHandlerTask->cs)) : );

    asm volatile ("movl %esp, %edx\n\t");
    asm volatile ("add $68, %edx\n\t");
    asm volatile ("movl (%edx), %edx\n\t");
    asm volatile ("movl %%edx, %0\n\t" : "=r" ((uint32_t)(SysHandlerTask->eax)) : );

    asm volatile ("movl %esp, %edx\n\t");
    asm volatile ("add $72, %edx\n\t");
    asm volatile ("movl (%edx), %edx\n\t");
    asm volatile ("movl %%edx, %0\n\t" : "=r" ((uint32_t)(SysHandlerTask->esp)) : );
  
         if ((unsigned int)syscallNumber == SYS_SOUND)                  { sysSound((struct soundParameter *)arg1); }
    else if ((unsigned int)syscallNumber == SYS_OPEN)                   { returnedValueFromSyscallFunction = sysOpen((struct fileParameter *)arg1, currentPid, directoryInode); }
    else if ((unsigned int)syscallNumber == SYS_WRITE)                  { sysWrite(arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_CLOSE)                  { sysClose(arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_PS)                     { sysPs(currentPid); }
    else if ((unsigned int)syscallNumber == SYS_EXEC)                   { sysExec((struct fileParameter *)arg1, currentPid, directoryInode); }
    else if ((unsigned int)syscallNumber == SYS_EXIT)                   { sysExit(currentPid, arg1); }
    else if ((unsigned int)syscallNumber == SYS_FREE)                   { sysFree(currentPid); }
    else if ((unsigned int)syscallNumber == SYS_MMAP)                   { sysMmap((uint8_t*)arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_KILL)                   { sysKill(arg1); }
    else if ((unsigned int)syscallNumber == SYS_SWITCH_TASK)            { sysSwitch(arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_MEM_DUMP)               { sysMemDump(arg1); }
    else if ((unsigned int)syscallNumber == SYS_UPTIME)                 { sysUptime(); }
    else if ((unsigned int)syscallNumber == SYS_SWITCH_TASK_TO_PARENT)  { sysSwitchToParent(currentPid); }
    else if ((unsigned int)syscallNumber == SYS_WAIT)                   { sysWait(); }
    else if ((unsigned int)syscallNumber == SYS_DIR)                    { sysDirectory(currentPid, directoryInode); }
    else if ((unsigned int)syscallNumber == SYS_TOGGLE_SCHEDULER)       { sysToggleScheduler(); }
    else if ((unsigned int)syscallNumber == SYS_SHOW_OPEN_FILES)        { sysShowOpenFiles(arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_CREATE)                 { sysCreate((struct fileParameter *)arg1, currentPid, directoryInode); }
    else if ((unsigned int)syscallNumber == SYS_DELETE)                 { sysDelete((struct fileParameter *)arg1, currentPid, directoryInode); }
    else if ((unsigned int)syscallNumber == SYS_OPEN_EMPTY)             { sysOpenEmpty((struct fileParameter *)arg1, currentPid, directoryInode); }
    else if ((unsigned int)syscallNumber == SYS_CREATE_PIPE)            { sysCreatePipe((struct fileParameter *)arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_READ)                   { sysRead(arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_OPEN_PIPE)              { returnedValueFromSyscallFunction = sysOpenPipe((struct fileParameter *)arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_NET_SEND)               { netUDPSend((struct networkParameter *)arg1); }
    else if ((unsigned int)syscallNumber == SYS_SHOW_GLOBAL_OBJECTS)    { sysShowGlobalObjects(currentPid); }
    else if ((unsigned int)syscallNumber == SYS_CREATE_NETWORK_PIPE)    { sysCreateNetworkPipe((struct networkParameter *)arg1, currentPid);  }
    else if ((unsigned int)syscallNumber == SYS_NET_RCV)                { netUDPRcv(); }
    else if ((unsigned int)syscallNumber == SYS_WHAT_PID_AM_I)          { returnedValueFromSyscallFunction = sysWhatPidAmI(); }
    else if ((unsigned int)syscallNumber == SYS_MEM_DISASSEMBLY)        { sysDisassemble(arg1); }
    else if ((unsigned int)syscallNumber == SYS_SECTOR_TO_VIEW)         { sysDiskSectorDump(arg1); }
    else if ((unsigned int)syscallNumber == SYS_BLOCK_TO_VIEW)          { sysDiskBlockDump(arg1); }
    else if ((unsigned int)syscallNumber == SYS_ELF_HEADER_TO_VIEW)     { sysElfHeaderDump((uint8_t*)arg1); }
    else if ((unsigned int)syscallNumber == SYS_INODE_TO_VIEW)          { sysInodeDump(arg1, currentPid); }
    else if ((unsigned int)syscallNumber == SYS_KERNEL_LOG_VIEW)        { printKernelLog(currentPid); }
    else if ((unsigned int)syscallNumber == SYS_CHANGE_DIR)             { sysChangeDirectory((struct fileParameter *)arg1, currentPid, directoryInode);}
    else if ((unsigned int)syscallNumber == SYS_MOVE_FILE)              { sysMove((struct fileParameter *)arg1, currentPid, directoryInode);}
    else if ((unsigned int)syscallNumber == SYS_GET_INODE_STRUCT)       { sysGetInodeForUser((struct fileParameter *)arg1, currentPid, directoryInode);}
    else if ((unsigned int)syscallNumber == SYS_CHANGE_FILE_MODE)       { sysChangeFileMode((struct fileParameter *)arg1, currentPid, directoryInode);}

    scheduler(currentPid);

    returnedPid = readValueFromMemLoc(RUNNING_PID_LOC);
    newSysHandlertaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (returnedPid - 1));
    newSysHandlerTask = (struct task*)newSysHandlertaskStructLocation;

    asm volatile ("popa\n\t");
    asm volatile ("leave\n\t");

    // pop off the important registers from the last task and save

    asm volatile ("pop %edx\n\t"); // burn
    asm volatile ("pop %edx\n\t"); // burn
    asm volatile ("pop %edx\n\t"); // burn
    asm volatile ("pop %edx\n\t"); // burn

    // Now recreate the stack for the IRET after saving the registers
    // Pushing these in reverse order so they are ready for the iret

    asm volatile ("movl %0, %%edx\n\t" : : "r" ((uint32_t)newSysHandlerTask->esp));
    asm volatile ("pushl %edx\n\t");

    asm volatile ("movl %0, %%edx\n\t" : : "r" ((uint32_t)newSysHandlerTask->eax));
    asm volatile ("pushl %edx\n\t");

    asm volatile ("mov %0, %%dx\n\t" : : "r" ((uint16_t)newSysHandlerTask->cs));
    asm volatile ("pushl %eax\n\t");

    asm volatile ("movl %0, %%edx\n\t" : : "r" ((uint32_t)newSysHandlerTask->eip));
    asm volatile ("pushl %edx\n\t");

    asm volatile ("movl %0, %%ebp\n\t" : : "r" ((uint32_t)newSysHandlerTask->ebp));

    asm volatile ("movl %0, %%eax\n\t" : : "r" ((uint32_t)returnedValueFromSyscallFunction));

    asm volatile ("iret\n\t");

}

void systemInterruptHandler()
{    
    // Dan O'Malley
    
    asm volatile ("pusha\n\t");

    uint8_t currentInterrupt = 0;
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    disableInterrupts();

    totalInterruptCount++;

    *(uint32_t*)TOTAL_INTERRUPTS_COUNT_LOC = totalInterruptCount;
    *(uint32_t*)USER_TOTAL_INTERRUPTS_COUNT_LOC = totalInterruptCount;

    uint32_t currentTaskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *currentTask;
    currentTask = (struct task*)currentTaskStructLocation;

    currentTask->runtime++;
    currentTask->recentRuntime++;

    uint32_t taskStructNumber = 0;
    struct task *Task = (struct task*)PROCESS_TABLE_LOC;

    while (taskStructNumber < MAX_PROCESSES)
    {
        if (Task->state == PROC_SLEEPING)
        {
            Task->sleepTime++;
        }
        taskStructNumber++;
        Task++;
    }


    outputIOPort(MASTER_PIC_COMMAND_PORT, 0xA);
    outputIOPort(MASTER_PIC_COMMAND_PORT, 0xB);
    currentInterrupt = inputIOPort(MASTER_PIC_COMMAND_PORT);
    
    if ((currentInterrupt & 0b0000001) == 0x1) // system timer IRQ 0
    {
        systemTimerInterruptCount++;
        
        if (totalInterruptCount % SYSTEM_INTERRUPTS_PER_SECOND)
        {
            uptimeSeconds = totalInterruptCount / SYSTEM_INTERRUPTS_PER_SECOND;
            if (uptimeSeconds == 60) { uptimeSeconds = 0; }
            uptimeMinutes = ((totalInterruptCount / SYSTEM_INTERRUPTS_PER_SECOND) / 60);
        }
    }
    else if ((currentInterrupt & 0b0000010) == 0x2) // keyboard interrupt IRQ 1
    {
        keyboardInterruptCount++;

    }
    else
    {
        otherInterruptCount++; // capture any other interrupts
    }

    outputIOPort(MASTER_PIC_COMMAND_PORT, INTERRUPT_END_OF_INTERRUPT);
    outputIOPort(SLAVE_PIC_COMMAND_PORT, INTERRUPT_END_OF_INTERRUPT);

    enableInterrupts();

    sysUptime();

    // currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    // scheduler(currentPid);

    asm volatile ("popa\n\t");
    asm volatile ("leave\n\t");
    asm volatile ("iret\n\t");

}

void printSystemUptime(uint32_t uptimeSeconds)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    if (*(uint32_t*)(LOGIN_COMPLETE) == 1)
    {
        uint32_t hours = uptimeSeconds / 3600;
        uint32_t remainingSeconds = uptimeSeconds % 3600;
        uint32_t minutes = remainingSeconds / 60;
        uint32_t seconds = remainingSeconds % 60;

        uint8_t *hoursLoc = kMalloc(KERNEL_OWNED, 10);
        uint8_t *minutesLoc = kMalloc(KERNEL_OWNED, 10);
        uint8_t *secondsLoc = kMalloc(KERNEL_OWNED, 10);

        itoa((unsigned int)(hours), hoursLoc);
        itoa((unsigned int)(minutes), minutesLoc);
        itoa((unsigned int)(seconds), secondsLoc);

        uint8_t *mmStr = kMalloc(KERNEL_OWNED, 3);
        if (minutes < 10)
        {
            mmStr[0] = '0';
            mmStr[1] = minutesLoc[0];
            mmStr[2] = '\0';
        }
        else
        {
            strcpy(mmStr, minutesLoc);
        }

        uint8_t *ssStr = kMalloc(KERNEL_OWNED, 3);
        if (seconds < 10)
        {
            ssStr[0] = '0';
            ssStr[1] = secondsLoc[0];
            ssStr[2] = '\0';
        }
        else
        {
            strcpy(ssStr, secondsLoc);
        }

        printString(COLOR_GREEN, 48, 69, (uint8_t *)"Up   :");

        uint32_t col = 72;
        printString(COLOR_LIGHT_BLUE, 48, col, hoursLoc);
        col += strlen(hoursLoc);
        printString(COLOR_LIGHT_BLUE, 48, col, (uint8_t *)":");
        col += 1;
        printString(COLOR_LIGHT_BLUE, 48, col, mmStr);
        col += 2;
        printString(COLOR_LIGHT_BLUE, 48, col, (uint8_t *)":");
        col += 1;
        printString(COLOR_LIGHT_BLUE, 48, col, ssStr);

        printString(COLOR_GREEN, 48, 53, (uint8_t*)"User:");
        printString(COLOR_LIGHT_BLUE, 48, 59, (uint8_t*)LOGGED_IN_USER);

        kFree(hoursLoc);
        kFree(minutesLoc);
        kFree(secondsLoc);
        kFree(mmStr);
        kFree(ssStr);
    }
    
}