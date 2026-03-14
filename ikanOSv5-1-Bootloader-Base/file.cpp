// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "constants.h"
#include "file.h"
#include "exceptions.h"
#include "vm.h"
#include "x86.h"
#include "libc-main.h"

void createOpenFileTable(uint8_t *openFileTable)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
    
    struct globalObjectTableEntry *GlobalObjectTableEntry = (struct globalObjectTableEntry*)openFileTable;

    GlobalObjectTableEntry->openedByPid = KERNEL_OWNED;
    GlobalObjectTableEntry->userspaceBuffer = (uint8_t*)KEYBOARD_BUFFER;
    GlobalObjectTableEntry->name = (uint8_t*)"STDIN";
    GlobalObjectTableEntry->type = GOTE_TYPE_BUFFER;
    GlobalObjectTableEntry->size = PAGE_SIZE;
    GlobalObjectTableEntry++;

    GlobalObjectTableEntry->openedByPid = KERNEL_OWNED;
    GlobalObjectTableEntry->userspaceBuffer = VIDEO_RAM;
    GlobalObjectTableEntry->name = (uint8_t*)"STDOUT";
    GlobalObjectTableEntry->type = GOTE_TYPE_BUFFER;
    GlobalObjectTableEntry->size = PAGE_SIZE * 2;
    GlobalObjectTableEntry++;

    GlobalObjectTableEntry->openedByPid = KERNEL_OWNED;
    GlobalObjectTableEntry->name = (uint8_t*)"STDERR";
    GlobalObjectTableEntry->type = GOTE_TYPE_BUFFER;
    GlobalObjectTableEntry->size = PAGE_SIZE;
    GlobalObjectTableEntry->userspaceBuffer = (uint8_t*)STDERR_BUFFER;

    uint8_t *message = (uint8_t*)"This is the default initialization message when STDERR is first initialized";
    memoryCopy(message, (uint8_t*)STDERR_BUFFER, ceiling(strlen(message), 2));  

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}

}

globalObjectTableEntry *insertGlobalObjectTableEntry(uint8_t *openFileTable, uint32_t openedByPid, uint32_t inode, uint32_t type, uint32_t sourceIPAddress, uint32_t destinationIPAddress, uint16_t sourcePort, uint16_t destinationPort, uint32_t size, uint8_t *userspaceBuffer, uint32_t numberOfPagesForBuffer, uint8_t *kernelSpaceBuffer,  uint8_t *name, uint32_t readOffset, uint32_t writeOffset, uint32_t lockedForWriting)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
    
    uint32_t lastUsedEntry = 0;
    uint32_t nextAvailEntry = 0;
    uint32_t GlobalObjectTableEntryNumber = 0;

    while (nextAvailEntry == 0 && GlobalObjectTableEntryNumber < MAX_SYSTEM_OPEN_FILES)
    {
        lastUsedEntry = *(uint32_t *)(GLOBAL_OBJECT_TABLE + (sizeof(globalObjectTableEntry) * GlobalObjectTableEntryNumber));

        if ((uint32_t)lastUsedEntry == 0)
        {
            nextAvailEntry = (GlobalObjectTableEntryNumber + 1);
        }

        if (GlobalObjectTableEntryNumber == (MAX_SYSTEM_OPEN_FILES - 1))
        {
            while (!releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
            panic((uint8_t *)"file.cpp:insertGlobalObjectTableEntry() -> reached max system open files");
        }

        GlobalObjectTableEntryNumber++;
    }

    uint32_t availableOpenFileTableLocation = GLOBAL_OBJECT_TABLE + (sizeof(globalObjectTableEntry) * (nextAvailEntry - 1));
    struct globalObjectTableEntry *GlobalObjectTableEntry = (struct globalObjectTableEntry*)availableOpenFileTableLocation;

    GlobalObjectTableEntry->openedByPid = openedByPid;
    GlobalObjectTableEntry->inode = inode;
    GlobalObjectTableEntry->type = type;
    GlobalObjectTableEntry->sourceIPAddress = sourceIPAddress;
    GlobalObjectTableEntry->destinationIPAddress = destinationIPAddress;
    GlobalObjectTableEntry->sourcePort = sourcePort;
    GlobalObjectTableEntry->destinationPort = destinationPort;
    GlobalObjectTableEntry->size = size;
    GlobalObjectTableEntry->userspaceBuffer = userspaceBuffer;
    GlobalObjectTableEntry->numberOfPagesForBuffer = numberOfPagesForBuffer;
    GlobalObjectTableEntry->kernelSpaceBuffer = kernelSpaceBuffer;
    GlobalObjectTableEntry->name = name;
    GlobalObjectTableEntry->readOffset = readOffset;
    GlobalObjectTableEntry->writeOffset = writeOffset;
    GlobalObjectTableEntry->lockedForWriting = lockedForWriting;

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}

    return GlobalObjectTableEntry;
}

uint32_t totalOpenFiles(uint8_t *openFileTable)
{
    uint32_t totalSystemOpenFiles = 0;
    uint32_t GlobalObjectTableEntryNumber = 0;

    struct globalObjectTableEntry *GlobalObjectTableEntry = (struct globalObjectTableEntry*)openFileTable;

    while (GlobalObjectTableEntryNumber < MAX_SYSTEM_OPEN_FILES)
    {
        if (GlobalObjectTableEntry->openedByPid != 0 || GlobalObjectTableEntry->inode != 0)
        {
            totalSystemOpenFiles++;
        }
        
        GlobalObjectTableEntry++;
        GlobalObjectTableEntryNumber++;
    }

    return totalSystemOpenFiles;
}

void closeAllFiles(uint8_t *openFileTable, uint32_t pid)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
    
    struct globalObjectTableEntry *GlobalObjectTableEntry = (struct globalObjectTableEntry*)openFileTable;
    uint32_t GlobalObjectTableEntryNumber = 0;

    while (GlobalObjectTableEntryNumber < MAX_SYSTEM_OPEN_FILES)
    {
        if (GlobalObjectTableEntry->openedByPid == pid)
        {
            fillMemory((uint8_t *)GlobalObjectTableEntry, 0x0, sizeof(globalObjectTableEntry));
        }
        
        GlobalObjectTableEntry++;
        GlobalObjectTableEntryNumber++;
    }

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
}

bool fileAvailableToBeLocked(uint8_t *openFileTable, uint32_t inode)
{
    struct globalObjectTableEntry *GlobalObjectTableEntry = (struct globalObjectTableEntry*)openFileTable;
    uint32_t GlobalObjectTableEntryNumber = 0;

    while (GlobalObjectTableEntryNumber < MAX_SYSTEM_OPEN_FILES)
    {
        if (GlobalObjectTableEntry->inode == inode)
        {
            if (GlobalObjectTableEntry->lockedForWriting != 0)
            {
                return false; // File already locked
            }

        }
        
        GlobalObjectTableEntry++;
        GlobalObjectTableEntryNumber++;
    }

    return true;
}

bool lockFile(uint8_t *openFileTable, uint32_t pid, uint32_t inode)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
    
    struct globalObjectTableEntry *GlobalObjectTableEntry = (struct globalObjectTableEntry*)openFileTable;
    uint32_t GlobalObjectTableEntryNumber = 0;

    // Do complete scan of table to make sure no one else has it locked
    while (GlobalObjectTableEntryNumber < MAX_SYSTEM_OPEN_FILES)
    {
        if (GlobalObjectTableEntry->inode == inode)
        {
            if (GlobalObjectTableEntry->lockedForWriting != 0)
            {
                while (!releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
                return false; // File already locked
            }

        }
        
        GlobalObjectTableEntry++;
        GlobalObjectTableEntryNumber++;
    }

    // Do another scan of the table to lock the file for the requesting pid.
    GlobalObjectTableEntry = (struct globalObjectTableEntry*)openFileTable;
    GlobalObjectTableEntryNumber = 0;
    while (GlobalObjectTableEntryNumber < MAX_SYSTEM_OPEN_FILES)
    {
        if (GlobalObjectTableEntry->inode == inode && GlobalObjectTableEntry->openedByPid == pid)
        {
            GlobalObjectTableEntry->lockedForWriting = pid;
            while (!releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
            return true;
        }
        
        GlobalObjectTableEntry++;
        GlobalObjectTableEntryNumber++;
    }

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)GLOBAL_OBJECT_TABLE)) {}
    return false;
}