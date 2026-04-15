// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "vm.h"
#include "constants.h"
#include "libc-main.h"

void createPageFrameMap(uint8_t *pageFrameMap, uint32_t numberOfFrames)
{//fix
//fill memory using existing page fra,e map


    fillMemory(pageFrameMap, PAGEFRAME_AVAILABLE, numberOfFrames);

    for (uint32_t frameNumber = 0; frameNumber < numberOfFrames; frameNumber++)
    {
        uint32_t physicalAddress = frameNumber * PAGE_SIZE;

        if (physicalAddress == 0x0 ||
            physicalAddress == KEYBOARD_BUFFER ||
            physicalAddress == USER_PID_INFO ||
            physicalAddress == SECOND_PROC_TMP_STACK ||
            physicalAddress == SECOND_PROC_SIPI_CODE ||
            physicalAddress == GDT_LOC ||
            (physicalAddress >= VIDEO_AND_BIOS_RESERVED_START && physicalAddress <= VIDEO_AND_BIOS_RESERVED_END) ||
            (physicalAddress >= KERNEL_BASE && physicalAddress < KERNEL_LIMIT))
        {
            *(uint8_t *)(pageFrameMap + frameNumber) = KERNEL_OWNED;
        }
    }
}


uint32_t allocateFrame(uint32_t pid, uint8_t *pageFrameMap)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}

    for (uint32_t frameNumber = 0; frameNumber < PAGEFRAME_MAP_SIZE; ++frameNumber)
    {
        if (*(uint8_t *)(pageFrameMap [frameNumber]) == PAGEFRAME_AVAILABLE)
        {
            *(uint8_t *)(pageFrameMap [frameNumber]) = (uint8_t)pid;

            while (!releaseLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}
            return frameNumber;
        }
    }

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}
    return 0;
}

void freeAllFrames(uint32_t pid, uint8_t *pageFrameMap)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}

    for (uint32_t frameNumber = 0; frameNumber < PAGEFRAME_MAP_SIZE; ++frameNumber)
    {
        if (*(uint8_t *)(pageFrameMap[frameNumber]) == pid)
        {
            *(uint8_t *)(pageFrameMap[ frameNumber]) = PAGEFRAME_AVAILABLE;
        }
    }

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}
}

void freeFrame(uint32_t frameNumber)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}
    
    *(uint8_t *)(PAGEFRAME_MAP_BASE + frameNumber) = (uint8_t)0x0;

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}
}




uint32_t processFramesUsed(uint32_t pid, uint8_t *pageFrameMap)
{
    uint8_t lastUsedFrame = PAGEFRAME_AVAILABLE;
    uint32_t framesUsed = 0;

    for (uint32_t frameNumber = 0; frameNumber < (KERNEL_BASE / PAGE_SIZE); frameNumber++)
    {
        lastUsedFrame = *(uint8_t *)(pageFrameMap + frameNumber);

        if (lastUsedFrame == pid)
        {
            framesUsed++;      
        }

    }
    return (unsigned int)framesUsed;

}

uint32_t totalFramesUsed(uint8_t *pageFrameMap)
{
    uint8_t lastUsedFrame = PAGEFRAME_AVAILABLE;
    uint32_t framesUsed = 0;

    for (uint32_t frameNumber = 0; frameNumber < PAGEFRAME_MAP_SIZE; frameNumber++)
    {
        lastUsedFrame = *(uint8_t *)(pageFrameMap + frameNumber);

        if (lastUsedFrame != PAGEFRAME_AVAILABLE)
        {
            framesUsed++;
            
        }

    }
    return (uint32_t)framesUsed;

}