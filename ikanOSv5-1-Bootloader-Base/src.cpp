// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "screen.h"
#include "keyboard.h"
#include "vm.h"
#include "libc-main.h"
#include "constants.h"
#include "x86.h"
#include "file.h"
#include "sound.h"

void main() {

    clearScreen();

    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint32_t row = 3;
    uint32_t column = 5;
    uint32_t returnValue = SYSCALL_FAIL; //initial value just in case.
    uint32_t fileDescriptor;
    uint32_t filePointer;
    uint8_t *commandString = malloc(currentPid, 50);
    uint8_t *argumentString1 = malloc(currentPid, 50);
    uint8_t *argumentString2 = malloc(currentPid, 50);
    uint8_t *argumentString3 = malloc(currentPid, 50);

    returnValue = systemOpenFile((uint8_t*)(COMMAND_BUFFER + 4), RDWRITE);
    if (returnValue == SYSCALL_FAIL)
    {
        row++;
        printString(COLOR_RED, row++, 5, (uint8_t *)"Failed to open input file");
        wait(3);
        systemExit(PROCESS_EXIT_CODE_INPUT_FILE_FAILED);
    }
    returnValue = SYSCALL_FAIL; //reset for next one.

    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    filePointer = (uint32_t)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

    uint8_t *pointer = (uint8_t*)filePointer;

    while (true)
    {
        if (*pointer == 0) break;

        uint8_t *cmd_start = pointer;
        while (*pointer != ' ' && *pointer != '\n' && *pointer != 0) pointer++;
        uint32_t cmd_len = pointer - cmd_start;
        if (cmd_len == 0)
        {
            if (*pointer == '\n') pointer++;
            continue;
        }
        bytecpy(commandString, cmd_start, cmd_len);
        commandString[cmd_len] = 0;

        uint32_t num_args = 0;
        uint8_t *arg_pointers[3] = {argumentString1, argumentString2, argumentString3};
        uint32_t arg_len;

        if (*pointer == ' ')
        {
            pointer++;
            while (num_args < 3)
            {
                // Skip horizontal whitespace (spaces and tabs, not \n)
                while (*pointer == ' ' || *pointer == '\t') pointer++;
                if (*pointer == '\n' || *pointer == 0) break;

                uint8_t *this_arg = arg_pointers[num_args];
                uint8_t quoted = 0;
                uint8_t *arg_start = pointer;

                if (*pointer == '"')
                {
                    quoted = 1;
                    pointer++;
                    arg_start = pointer;
                    while (*pointer != '"' && *pointer != '\n' && *pointer != 0) pointer++;
                    arg_len = pointer - arg_start;
                    if (*pointer == '"') pointer++;
                }
                else
                {
                    arg_start = pointer;
                    while (*pointer != ' ' && *pointer != '\t' && *pointer != '\n' && *pointer != 0) pointer++;
                    arg_len = pointer - arg_start;
                }

                if (arg_len > 0)
                {
                    bytecpy(this_arg, arg_start, arg_len);
                    this_arg[arg_len] = 0;
                    num_args++;
                }
                else
                {
                    break;
                }
            }
        }

        if (*pointer == '\n') pointer++;

        // Build COMMAND_BUFFER
        uint32_t buf_pos = 0;
        bytecpy(COMMAND_BUFFER + buf_pos, commandString, cmd_len);
        buf_pos += cmd_len;
        for (uint32_t i = 0; i < num_args; i++)
        {
            COMMAND_BUFFER[buf_pos++] = ' ';
            uint8_t *arg = arg_pointers[i];
            uint32_t alen = strlen(arg);
            bytecpy(COMMAND_BUFFER + buf_pos, arg, alen);
            buf_pos += alen;
        }
        COMMAND_BUFFER[buf_pos] = 0;

        if (strcmp(commandString, (uint8_t*)"END") == 0)
        {
            break;
        }

        if (!strcmp(commandString, (uint8_t*)"echo"))
        {
            clearScreen();
            uint32_t current_col = column;
            for (uint32_t i = 0; i < num_args; i++)
            {
                if (i > 0)
                {
                    printString(COLOR_WHITE, row, current_col, (uint8_t*)" ");
                    current_col += 1;
                }
                uint8_t *arg = arg_pointers[i];
                printString(COLOR_WHITE, row, current_col, arg);
                current_col += strlen(arg);
            }
            wait(2);
            continue;
        }
        else if (!strcmp(commandString, (uint8_t*)"rm"))
        {
            systemDeleteFile(arg_pointers[0]);
            continue;
        }
        else if (!strcmp(commandString, (uint8_t*)"chmod"))
        {
            systemChangeFileMode(arg_pointers[0], arg_pointers[1]);
            continue;
        }
        else if (!strcmp(commandString, (uint8_t*)"cd"))
        {
            systemChangeDirectory(arg_pointers[0]);
            continue;
        }
        else if (!strcmp(commandString, (uint8_t*)"free"))
        {
            systemFree();
            continue;
        }
        else if (!strcmp(commandString, (uint8_t*)";"))
        {
            // This is a comment
            continue;
        }

        clearScreen();
        systemExec(commandString, 50, 0, 0, 0);
    }

    systemCloseFile(fileDescriptor);

    free(commandString);
    free(argumentString1);
    free(argumentString2);
    free(argumentString3);
    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}