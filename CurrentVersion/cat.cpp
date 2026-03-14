// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "screen.h"
#include "libc-main.h"
#include "constants.h"
#include "x86.h"
#include "file.h"


void printBufferToScreen(uint8_t *userSpaceBuffer)
{
    uint32_t linearPosition = 0;
    
    if (userSpaceBuffer == 0x0)
    {
        printString(COLOR_RED, 0, 0,(uint8_t *)"No such buffer!");
        return;
    }
    
    
    uint32_t row = 0;
    uint32_t column = 0;

    while (row <= 20)
    {
        if (column > 79) { column = 0; row++; }
        if (*(uint8_t *)((uint32_t)userSpaceBuffer + linearPosition) == (uint8_t)0x0a) { row++; column=0; }
        if (*(uint8_t *)((uint32_t)userSpaceBuffer + linearPosition) == (uint8_t)0x0d) { column++; }
        if (*(uint8_t *)((uint32_t)userSpaceBuffer + linearPosition) == (uint8_t)0x09) { column=column+4; }

        if (row <= 49)
        {
            printCharacter(COLOR_WHITE, row, column, (uint8_t *)((int)userSpaceBuffer + linearPosition));
        }

        column++;
        linearPosition++;
    }

}

void main()
{    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
    uint8_t* pipeInFdString = malloc(currentPid, 20);
    uint8_t* pipeOutFdString = malloc(currentPid, 20);
    itoa(0, pipeInFdString);
    itoa(1, pipeOutFdString);

    systemMMap((uint8_t*)openBufferTable->buffers[0]);
    systemMMap((uint8_t*)openBufferTable->buffers[1]);

    systemReadPipe(pipeInFdString);

    // Do your work on the PAGE of memory at (uint8_t*)openBufferTable->buffers[0] (STDIN) and
    // then do the bytecpy to (uint8_t*)openBufferTable->buffers[1] (STDOUT) before writing 
    // the pipe.

    bytecpy((uint8_t*)openBufferTable->buffers[1], (uint8_t*)openBufferTable->buffers[0], openBufferTable->bufferSize[0]);
    systemWritePipe(pipeOutFdString);

    systemExit(PROCESS_EXIT_CODE_SUCCESS);
    
}