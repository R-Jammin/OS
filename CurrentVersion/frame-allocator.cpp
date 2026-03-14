// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "vm.h"
#include "constants.h"
#include "libc-main.h"

void createPageFrameMap(uint8_t *pageFrameMap, uint32_t numberOfFrames)
{
    // ASSIGNMENT 3 TO DO
}


uint32_t allocateFrame(uint32_t pid, uint8_t *pageFrameMap)
{
    // ASSIGNMENT 3 TO DO

    return 0; // Remove me when doing the assignment

}

void freeFrame(uint32_t frameNumber)
{
    while (!acquireLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}
    
    *(uint8_t *)(PAGEFRAME_MAP_BASE + frameNumber) = (uint8_t)0x0;

    while (!releaseLock(KERNEL_OWNED, (uint8_t *)PAGEFRAME_MAP_BASE)) {}
}


void freeAllFrames(uint32_t pid, uint8_t *pageFrameMap)
{

    // ASSIGNMENT 3 TO DO
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