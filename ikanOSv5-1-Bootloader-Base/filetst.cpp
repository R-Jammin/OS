// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"
#include "libc-main.h"
#include "screen.h"
#include "vm.h"
#include "x86.h"


void main()
{
    uint32_t myProcessId;
    uint32_t fileDescriptor;
    uint8_t *bufferPointer;
    uint32_t row = 3;
    clearScreen();

    uint8_t * filenameTst = malloc(myProcessId, 30);

    printString(COLOR_LIGHT_BLUE, row++, 5, (uint8_t *)"--------- ikanOS File Operations Tester v1 --------- ");
    row++;

    printString(COLOR_GREEN, row++, 2, (uint8_t *)"This test creates an empty file using a randomly generated name,");
    printString(COLOR_GREEN, row++, 2, (uint8_t *)"closes the newly created file, opens it again, fills it with a seed value,");
    printString(COLOR_GREEN, row++, 2, (uint8_t *)"saves it with a new name, opens the newly created second file,");
    printString(COLOR_GREEN, row++, 2, (uint8_t *)"checks to make sure the seed value is the same,");
    printString(COLOR_GREEN, row++, 2, (uint8_t *)"closes the file, and then deletes the two files used in that test.");
    row++;

    myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    for (int x=1; x <= 5; x++)
    {
        // Generate randon name of file for testing.
        itoaHex((uint32_t)rand(), filenameTst);
        printString(COLOR_GREEN, row++, 5, (uint8_t*)"Filename: ");
        printString(COLOR_WHITE, row-1, 15, filenameTst);

        // Create empty file with 1 block
        // THIS REQUIRES THE NEWLINE!!!! DUMB.
        systemOpenEmptyFile(strConcat(removeExtension(filenameTst), (uint8_t*)"\n"), 1);
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

        systemCloseFile(fileDescriptor);
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

        systemOpenFile(strConcat(removeExtension(filenameTst), (uint8_t*)"\n"), RDWRITE);
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

        fillMemory(bufferPointer, x, PAGE_SIZE);

        systemCreateFile(strConcat(removeExtension(filenameTst), (uint8_t*)".new"), fileDescriptor);
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

        // Close the file
        systemCloseFile(fileDescriptor);
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

        // Open the file again
        systemOpenFile(strConcat(removeExtension(filenameTst), (uint8_t*)".new\n"), RDWRITE);
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);


        // Check if values are unchanged
        bool testPassed = true;
        uint32_t failPosition = 0;
        uint8_t actualValue = 0;
        for (uint32_t byteIndex = 0; byteIndex < PAGE_SIZE; byteIndex++)
        {
            if (bufferPointer[byteIndex] != x)
            {
                testPassed = false;
                failPosition = byteIndex;
                actualValue = bufferPointer[byteIndex];
                break;
            }
        }

        // Report result
        if (testPassed)
        {
            printString(COLOR_GREEN, row++, 5, (uint8_t *)"Test: ");
            printHexNumber(COLOR_WHITE, row-1, 11, x);
            printString(COLOR_GREEN, row++, 5, (uint8_t *)"Test passed.");
            row++;
            row++;
        }
        else
        {
            printString(COLOR_GREEN, row++, 5, (uint8_t *)"Test #");
            printHexNumber(COLOR_WHITE, row-1, 11, x);
            printString(COLOR_RED, row++, 5, (uint8_t *)"Test failed.");
            row++;
            row++;
        }

        systemCloseFile(fileDescriptor);
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

        systemDeleteFile(strConcat(removeExtension(filenameTst), (uint8_t*)""));
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

        systemDeleteFile(strConcat(removeExtension(filenameTst), (uint8_t*)".new"));
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        bufferPointer = (uint8_t *)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);
    }

    row++;
    printString(COLOR_LIGHT_BLUE, row++, 5, (uint8_t *)"--------- Testing Complete. --------- ");
    
    wait(2);

    free(filenameTst);

    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}