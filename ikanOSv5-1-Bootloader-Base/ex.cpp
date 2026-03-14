// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Most of the code written by Dan O'Malley
// Argument tweaks/tokenization, and some keyboard enhancements written by Grok
// 12/2025 with Grok v4.


#include "screen.h"
#include "keyboard.h"
#include "vm.h"
#include "libc-main.h"
#include "constants.h"
#include "x86.h"
#include "file.h"
#include "net.h"

// Any commands that don't take an argument, add "\n" to the end
uint8_t *clearScreenCommand = (uint8_t *)"cls"; 
uint8_t *openFileCommand = (uint8_t *)"open";
uint8_t *saveFileCommand = (uint8_t *)"save";
uint8_t *closeFileCommand = (uint8_t *)"close";
uint8_t *viewFileCommand = (uint8_t *)"view";
uint8_t *helpCommand = (uint8_t *)"help";
uint8_t *psCommand = (uint8_t *)"ps";
uint8_t *exitCommand = (uint8_t *)"q";
uint8_t *dirCommand = (uint8_t *)"ls";
uint8_t *editFileCommand = (uint8_t *)"edit";
uint8_t *switchCommand = (uint8_t *)"sw";
uint8_t *insertCommand = (uint8_t *)"ins";
uint8_t *createPipeCommand = (uint8_t *)"cpipe";
uint8_t *createNetworkPipeCommand = (uint8_t *)"cnpipe";
uint8_t *modifyBufferCommand = (uint8_t *)"mod";
uint8_t *writePipeCommand = (uint8_t *)"wpipe";
uint8_t *readPipeCommand = (uint8_t *)"rpipe";
uint8_t *openPipeCommand = (uint8_t *)"opipe";
uint8_t *netCommand = (uint8_t *)"net";
uint8_t *globalObjectsCommand = (uint8_t *)"go";
uint8_t* memCommand = (uint8_t*)"m";
uint8_t* disassembleCommand = (uint8_t*)"d";
uint8_t* sectorCommand = (uint8_t*)"s";
uint8_t* blockCommand = (uint8_t*)"b";
uint8_t* blockRootDirectoryCommand = (uint8_t*)"br";
uint8_t* elfHeaderCommand = (uint8_t*)"e";
uint8_t *inodeDetailsCommand = (uint8_t *)"i";
uint8_t *kernelLogCommand = (uint8_t *)"kl";
uint8_t *changeDirectoryCommand = (uint8_t *)"cd";


void printPrompt(uint8_t myPid)
{   
    uint8_t *shellHorizontalLine = malloc(myPid, sizeof(uint8_t));
    if (shellHorizontalLine == 0) {
        // Handle error, but for simplicity, assume success
    }
    *shellHorizontalLine = ASCII_HORIZONTAL_LINE;
    for (uint32_t columnPos = 0; columnPos < 80; columnPos++)
    {
        printString(COLOR_LIGHT_BLUE, 43, columnPos, shellHorizontalLine);
    }

    free(shellHorizontalLine);

    printString(COLOR_GREEN, 43, 3, (uint8_t *)"Kernel and File Explorer v1.0");
    printString(COLOR_RED, 44, 2, (uint8_t *)"#");
    printString(COLOR_GREEN, 43, 70, (uint8_t *)"PID: ");
    printHexNumber(COLOR_GREEN, 43, 75, (uint8_t)myPid);

}

void printBufferToScreen(uint8_t *userSpaceBuffer)
{
    uint32_t linearPosition = 0;
    
    if (userSpaceBuffer == 0x0)
    {
        printString(COLOR_RED, 21, 3,(uint8_t *)"No such buffer!");
        return; //null pointer
    }
    
    
    uint32_t row = 0;
    uint32_t column = 0;

    while (row <= 15)
    {
        if (column > 79) { column = 0; row++; }
        if (*(uint8_t *)((uint32_t)userSpaceBuffer + linearPosition) == (uint8_t)0x0a) { row++; column=0; }
        if (*(uint8_t *)((uint32_t)userSpaceBuffer + linearPosition) == (uint8_t)0x0d) { column++; }
        if (*(uint8_t *)((uint32_t)userSpaceBuffer + linearPosition) == (uint8_t)0x09) { column=column+4; }

        if (row <= 15)
        {
            printCharacter(COLOR_WHITE, row, column, (uint8_t *)((int)userSpaceBuffer + linearPosition));
        }

        column++;
        linearPosition++;
    }

}

void screenEditor(uint32_t myPid, uint32_t currentFileDescriptor)
{
    uint32_t row = 0;
    uint32_t column = 0;
    uint32_t rowBasedOnBuffer = 0;
    uint32_t columnBasedOnBuffer = 0;
    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;

    uint32_t bufferPosition = (uint32_t)(uint8_t *)openBufferTable->buffers[currentFileDescriptor];
    uint32_t bufferPositionStart = bufferPosition;

    uint8_t *bufferPositionMem = malloc(myPid, sizeof(uint8_t *));
    uint8_t *rowPositionMem = malloc(myPid, sizeof(uint8_t *));
    uint8_t *columnPositionMem = malloc(myPid, sizeof(uint8_t *));

    uint8_t shift_down = 0;

    while ((uint8_t)*(uint8_t *)KEYBOARD_BUFFER != 0x81) //esc
    {       
        moveCursor(row + rowBasedOnBuffer, column + columnBasedOnBuffer);

        uint8_t temp_scan;
        do {
            readKeyboard(&temp_scan);
            if (temp_scan == 0x2A || temp_scan == 0x36) {
                shift_down = 1;
                continue;
            }
            if (temp_scan == 0xAA || temp_scan == 0xB6) {
                shift_down = 0;
                continue;
            }
            if (!(temp_scan & 0x80)) {
                continue;
            }
            *(uint8_t *)KEYBOARD_BUFFER = temp_scan;
            break;
        } while (1);

        if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x90) {
            *(uint8_t*)bufferPosition = shift_down ? 0x51 : 0x71;
            bufferPosition++;
            } //q Q
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x91) {
            *(uint8_t*)bufferPosition = shift_down ? 0x57 : 0x77;
            bufferPosition++;
            } //w W
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x92) {
            *(uint8_t*)bufferPosition = shift_down ? 0x45 : 0x65;
            bufferPosition++;
            } //e E
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x93) {
            *(uint8_t*)bufferPosition = shift_down ? 0x52 : 0x72;
            bufferPosition++;
            } //r R
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x94) {
            *(uint8_t*)bufferPosition = shift_down ? 0x54 : 0x74;
            bufferPosition++;
            } //t T
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x95) {
            *(uint8_t*)bufferPosition = shift_down ? 0x59 : 0x79;
            bufferPosition++;
            } //y Y
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x96) {
            *(uint8_t*)bufferPosition = shift_down ? 0x55 : 0x75;
            bufferPosition++;
            } //u U
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x97) {
            *(uint8_t*)bufferPosition = shift_down ? 0x49 : 0x69;
            bufferPosition++;
            } //i I
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x98) {
            *(uint8_t*)bufferPosition = shift_down ? 0x4f : 0x6f;
            bufferPosition++;
            } //o O
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x99) {
            *(uint8_t*)bufferPosition = shift_down ? 0x50 : 0x70;
            bufferPosition++;
            } //p P
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x9e) {
            *(uint8_t*)bufferPosition = shift_down ? 0x41 : 0x61;
            bufferPosition++;
            } //a A
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x9f) {
            *(uint8_t*)bufferPosition = shift_down ? 0x53 : 0x73;
            bufferPosition++;
            } //s S
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa0) {
            *(uint8_t*)bufferPosition = shift_down ? 0x44 : 0x64;
            bufferPosition++;
            } //d D
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa1) {
            *(uint8_t*)bufferPosition = shift_down ? 0x46 : 0x66;
            bufferPosition++;
            } //f F
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa2) {
            *(uint8_t*)bufferPosition = shift_down ? 0x47 : 0x67;
            bufferPosition++;
            } //g G
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa3) {
            *(uint8_t*)bufferPosition = shift_down ? 0x48 : 0x68;
            bufferPosition++;
            } //h H
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa4) {
            *(uint8_t*)bufferPosition = shift_down ? 0x4a : 0x6a;
            bufferPosition++;
            } //j J
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa5) {
            *(uint8_t*)bufferPosition = shift_down ? 0x4b : 0x6b;
            bufferPosition++;
            } //k K
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa6) {
            *(uint8_t*)bufferPosition = shift_down ? 0x4c : 0x6c;
            bufferPosition++;
            } //l L
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xac) {
            *(uint8_t*)bufferPosition = shift_down ? 0x5a : 0x7a;
            bufferPosition++;
            } //z Z
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xad) {
            *(uint8_t*)bufferPosition = shift_down ? 0x58 : 0x78;
            bufferPosition++;
            } //x X
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xae) {
            *(uint8_t*)bufferPosition = shift_down ? 0x43 : 0x63;
            bufferPosition++;
            } //c C
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xaf) {
            *(uint8_t*)bufferPosition = shift_down ? 0x56 : 0x76;
            bufferPosition++;
            } //v V
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xb0) {
            *(uint8_t*)bufferPosition = shift_down ? 0x42 : 0x62;
            bufferPosition++;
            } //b B
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xb1) {
            *(uint8_t*)bufferPosition = shift_down ? 0x4e : 0x6e;
            bufferPosition++;
            } //n N
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xb2) {
            *(uint8_t*)bufferPosition = shift_down ? 0x4d : 0x6d;
            bufferPosition++;
            } //m M
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x82) {
            *(uint8_t*)bufferPosition = shift_down ? 0x21 : 0x31;
            bufferPosition++;
            } //1 !
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x83) {
            *(uint8_t*)bufferPosition = shift_down ? 0x40 : 0x32;
            bufferPosition++;
            } //2 @
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x84) {
            *(uint8_t*)bufferPosition = shift_down ? 0x23 : 0x33;
            bufferPosition++;
            } //3 #
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x85) {
            *(uint8_t*)bufferPosition = shift_down ? 0x24 : 0x34;
            bufferPosition++;
            } //4 $
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x86) {
            *(uint8_t*)bufferPosition = shift_down ? 0x25 : 0x35;
            bufferPosition++;
            } //5 %
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x87) {
            *(uint8_t*)bufferPosition = shift_down ? 0x5E : 0x36;
            bufferPosition++;
            } //6 ^
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x88) {
            *(uint8_t*)bufferPosition = shift_down ? 0x26 : 0x37;
            bufferPosition++;
            } //7 &
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x89) {
            *(uint8_t*)bufferPosition = shift_down ? 0x2A : 0x38;
            bufferPosition++;
            } //8 *
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x8a) {
            *(uint8_t*)bufferPosition = shift_down ? 0x28 : 0x39;
            bufferPosition++;
            } //9 (
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x8b) {
            *(uint8_t*)bufferPosition = shift_down ? 0x29 : 0x30;
            bufferPosition++;
            } //0 )
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x8c) {
            *(uint8_t*)bufferPosition = shift_down ? 0x5F : 0x2D;
            bufferPosition++;
            } // - _
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x8d) {
            *(uint8_t*)bufferPosition = shift_down ? 0x2B : 0x3D;
            bufferPosition++;
            } // = +
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x9a) {
            *(uint8_t*)bufferPosition = shift_down ? 0x7B : 0x5B;
            bufferPosition++;
            } // [ {
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x9b) {
            *(uint8_t*)bufferPosition = shift_down ? 0x7D : 0x5D;
            bufferPosition++;
            } // ] }
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xab) {
            *(uint8_t*)bufferPosition = shift_down ? 0x7C : 0x5C;
            bufferPosition++;
            } // \ |
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xb3) {
            *(uint8_t*)bufferPosition = shift_down ? 0x3C : 0x2C;
            bufferPosition++;
            } // , <
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xb4) {
            *(uint8_t*)bufferPosition = shift_down ? 0x3E : 0x2e;
            bufferPosition++;
        } // . >
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xb5) {
            *(uint8_t*)bufferPosition = shift_down ? 0x3F : 0x2f;
            bufferPosition++;
        } // / ?
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa7) {
            *(uint8_t*)bufferPosition = shift_down ? 0x3A : 0x3b;
            bufferPosition++;
        } // ; :
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xa8) {
            *(uint8_t*)bufferPosition = shift_down ? 0x22 : 0x27;
            bufferPosition++;
        } // ' "
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xB9) {  // Space key (break code)
            *(uint8_t*)bufferPosition = 0x20;
            bufferPosition++;
        }
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x9c) {
            *(uint8_t*)bufferPosition = 0x0a;
            bufferPosition++;
            column = 0;
            row++;
            } // enter
        
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0x8e) //backspace
        {
            *(uint8_t*)bufferPosition = 0x20;

            if (bufferPosition <= bufferPositionStart)
            {
                bufferPosition = bufferPositionStart;
            }
            else
            {
                bufferPosition--;
            }

        }
        else if ((uint8_t)(*(uint8_t *)KEYBOARD_BUFFER) == (uint8_t)0xb9) {
            *(uint8_t*)bufferPosition = 0x20;
            bufferPosition++;
            } // space

        if ( (uint8_t)*(uint8_t *)KEYBOARD_BUFFER == 0xCD ) //right
        { 
            if (column >= 79)
            {
                column = 0;
                row++;
            }
            else
            {
                column++;
            } 
        }
        else if ( (uint8_t)*(uint8_t *)KEYBOARD_BUFFER == 0xCB ) //left
        { 
            if (column <= 0)
            {
                if (row == 0)
                {
                    row = 0;
                    column = 0;
                }
                else
                {
                    column = 79;
                    row--;
                }
            }
            else
            {
                column--;
            }

        }
        else if ( (uint8_t)*(uint8_t *)KEYBOARD_BUFFER == 0xD0 ) //down
        { 
            row++; 
        }
        else if ( (uint8_t)*(uint8_t *)KEYBOARD_BUFFER == 0xC8 ) //up
        { 
            if (row <= 0)
            {
                row = 0;
            }
            else
            {
                row--;
            }
        }


        if (row <= 1)
        {
            fillMemory(VIDEO_RAM, 0x0, 2720);
            
            if ((bufferPosition - 160) < bufferPosition)
            {
                printBufferToScreen((uint8_t *)(bufferPositionStart));
                moveCursor(row, column);
            }

        }

        if (row >= 2 && row < 15)
        {
            fillMemory(VIDEO_RAM, 0x0, 2720);
            printBufferToScreen((uint8_t *)(bufferPositionStart + (80 * row)));
            moveCursor(row, column);
        }

        if (row >= 15)
        {
            fillMemory(VIDEO_RAM, 0x0, 2720);
            printBufferToScreen((uint8_t *)(bufferPositionStart + (80 * row)));
            moveCursor(15, column);

        }

        rowBasedOnBuffer = (bufferPosition - bufferPositionStart) / 80;
        columnBasedOnBuffer = (bufferPosition - bufferPositionStart) % 80;

        printString(COLOR_WHITE, 22, 13, (uint8_t *)"        ");
        itoa((uint32_t)bufferPosition, bufferPositionMem);
        printString(COLOR_RED, 22, 1, (uint8_t *)"Buffer Pos:");
        printString(COLOR_WHITE, 22, 13, bufferPositionMem);

        printString(COLOR_WHITE, 21, 13, (uint8_t *)"        ");
        itoa((uint32_t)row + rowBasedOnBuffer, rowPositionMem);
        printString(COLOR_RED, 21, 1, (uint8_t *)"Row Pos:");
        printString(COLOR_WHITE, 21, 13, rowPositionMem);

        printString(COLOR_WHITE, 21, 43, (uint8_t *)"        ");
        itoa((uint32_t)column + columnBasedOnBuffer, columnPositionMem);
        printString(COLOR_RED, 21, 31, (uint8_t *)"Col Pos:");
        printString(COLOR_WHITE, 21, 43, columnPositionMem);

    }

    free(bufferPositionMem);
    free(rowPositionMem);
    free(columnPositionMem);

}


void main()
{
    uint32_t myPid;
    bool fileContentsOnScreen = false;

    //disableCursor();
    clearScreen();

    myPid = readValueFromMemLoc(RUNNING_PID_LOC);

    // clearing shared buffer area
    fillMemory((uint8_t *)KEYBOARD_BUFFER, (uint8_t)0x0, (KEYBOARD_BUFFER_SIZE * 2));

    systemShowOpenFiles(30);
    myPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint8_t *commandArgument1 = malloc(myPid, 32);
    uint8_t *commandArgument2 = malloc(myPid, 32);
    uint8_t *commandArgument3 = malloc(myPid, 32);
    uint8_t *commandArgument4 = malloc(myPid, 32);
    uint8_t *commandArgument5 = malloc(myPid, 32);
    if (commandArgument1 == 0 || commandArgument2 == 0 || commandArgument3 == 0 || commandArgument4 == 0 || commandArgument5 == 0) {
        printString(COLOR_RED, 26, 3, (uint8_t *)"Malloc failed for arguments!");
        systemExit(PROCESS_EXIT_CODE_MALLOC_FAILED);
    }

    while (true)
    {

        uint8_t *bufferMem = (uint8_t *)KEYBOARD_BUFFER;
        uint8_t *cursorMemory = (uint8_t *)SHELL_CURSOR_POS;

        myPid = readValueFromMemLoc(RUNNING_PID_LOC);     
        
        fillMemory((uint8_t *)KEYBOARD_BUFFER, (uint8_t)0x0, (KEYBOARD_BUFFER_SIZE * 2));

        printPrompt(myPid);     
        readCommand(bufferMem, cursorMemory);

        uint8_t *command = COMMAND_BUFFER;

        // Tokenization
        // Ensure null terminator after \n
        uint8_t *end = COMMAND_BUFFER;
        while (*end != 0x0A && *end != 0x00) end++;
        if (*end == 0x0A) *end = 0x00;  // Replace \n with \0

        // Tokenize: commandArgument1/2/3/4/5 will point to tokens or NULL if missing
        command = skipWhitespace(COMMAND_BUFFER);
        uint8_t *p = command;
        while (*p && !isSpace(*p)) p++;
        if (*p) *p++ = 0x00;  // Null-terminate command
        p = skipWhitespace(p);
        commandArgument1 = (*p) ? p : NULL;
        if (commandArgument1) {
            while (*p && !isSpace(*p)) p++;
            if (*p) *p++ = 0x00;
            p = skipWhitespace(p);
        }
        commandArgument2 = (*p) ? p : NULL;
        if (commandArgument2) {
            while (*p && !isSpace(*p)) p++;
            if (*p) *p++ = 0x00;
            p = skipWhitespace(p);
        }
        commandArgument3 = (*p) ? p : NULL;
        if (commandArgument3) {
            while (*p && !isSpace(*p)) p++;
            if (*p) *p++ = 0x00;
            p = skipWhitespace(p);
        }
        commandArgument4 = (*p) ? p : NULL;
        if (commandArgument4) {
            while (*p && !isSpace(*p)) p++;
            if (*p) *p++ = 0x00;
            p = skipWhitespace(p);
        }
        commandArgument5 = (*p) ? p : NULL;

        if (strcmp(command, clearScreenCommand) == 0)
        {
            disableCursor();
            clearScreen();

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
        }
        else if (strcmp(command, insertCommand) == 0)
        {
            printString(COLOR_WHITE, 19, 3,(uint8_t *)"                                ");
            fillMemory((uint8_t *)KEYBOARD_BUFFER, (uint8_t)0x0, (KEYBOARD_BUFFER_SIZE * 2));
            
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            if (fileContentsOnScreen)
            {
                screenEditor(myPid, currentFileDescriptor);
            }

        }
        else if (strcmp(command, globalObjectsCommand) == 0)
        {
            clearScreen();
            printPrompt(myPid);  
            systemShowGlobalObjects();

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, memCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid address for m - usage: m <hex addr>");
                wait(2);
                continue;
            }
            clearScreen();

            systemShowMemory((uint8_t*)(hextoi(commandArgument1)));
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, elfHeaderCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid address for e - usage: e <hex addr>");
                wait(2);
                continue;
            }
            clearScreen();

            systemShowElfHeader((uint8_t*)(hextoi(commandArgument1)));
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, sectorCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid sector for s - usage: s <sector hex>");
                wait(2);
                continue;
            }
            clearScreen();

            systemDiskSectorViewer((uint32_t)(uint8_t*)(hextoi(commandArgument1)));
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, blockCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid block for b - usage: b <block hex>");
                wait(2);
                continue;
            }
            clearScreen();

            systemBlockViewer((uint32_t)(uint8_t*)(hextoi(commandArgument1)));
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, kernelLogCommand) == 0)
        {
            clearScreen();

            systemKernelLogViewer();
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, blockRootDirectoryCommand) == 0)
        {
            clearScreen();

            systemBlockViewer(ROOTDIR_BLOCK);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, disassembleCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid address for d - usage: d <hex addr>");
                wait(2);
                continue;
            }
            clearScreen();

            systemShowDisassembly((uint8_t*)(hextoi(commandArgument1)));
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, createPipeCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid name for cpipe - usage: cpipe <name>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();
            printPrompt(myPid);           

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemCreatePipe(commandArgument1);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

        }
        else if (strcmp(command, inodeDetailsCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0 || !isDigit(commandArgument1[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid inode for i - usage: i <hex inode>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();
            printPrompt(myPid);           

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemShowInodeDetail(hextoi(commandArgument1));

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

        }
        else if (strcmp(command, changeDirectoryCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid directory for cd - usage: cd <dir>");
                wait(2);
                continue;
            }
            clearScreen();
            printPrompt(myPid);      

            systemChangeDirectory(commandArgument1);
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, createNetworkPipeCommand) == 0)
        {
            if (commandArgument1 == NULL || commandArgument2 == NULL || commandArgument3 == NULL ||
                strlen(commandArgument1) == 0 || strlen(commandArgument2) == 0 || strlen(commandArgument3) == 0 ||
                !isDigit(commandArgument3[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid arguments for cnpipe - usage: cnpipe <name> <dstip> <dstpt>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();
            printPrompt(myPid);           

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            uint32_t sourceIPAddress = 0x0A00020F;  // 10.0.2.15
            uint32_t destinationIPAddress = ipAddressTointeger(commandArgument2);  // 10.0.2.2
            uint16_t sourcePort = 12345;
            uint16_t destinationPort = atoi(commandArgument3);

            systemCreateNetworkPipe(commandArgument1, sourceIPAddress, destinationIPAddress, sourcePort, destinationPort);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

        }
        else if (strcmp(command, openPipeCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid name for opipe - usage: opipe <name>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();
            printPrompt(myPid);           

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemOpenPipe(commandArgument1);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

        }
        else if (strcmp(command, netCommand) == 0)
        {
            // if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
            //     clearScreen();
            //     printPrompt(myPid);
            //     printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid subcommand for net - usage: net send <msg> or net rcv");
            //     wait(2);
            //     continue;
            // }
            // if (strcmp(commandArgument1, (uint8_t*)"send"))
            // {
            //     // if (commandArgument2 == NULL || strlen(commandArgument2) == 0) {
            //     //     clearScreen();
            //     //     printPrompt(myPid);
            //     //     printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid message for net send - usage: net send <msg>");
            //     //     wait(2);
            //     //     continue;
            //     // }
            //     disableCursor();
            //     clearScreen();
            //     printPrompt(myPid);           

            //     myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            //     uint32_t sourceIPAddress = 0x0A00020F;  // 10.0.2.15
            //     uint32_t destinationIPAddress = 0x0A000202;  // 10.0.2.2
            //     uint16_t sourcePort = 12345;
            //     uint16_t destinationPort = 1234;

            //     systemNetSend(sourceIPAddress, destinationIPAddress, sourcePort, destinationPort, commandArgument2);

            //     myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            //     uint32_t currentFileDescriptor;
            //     currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            //     systemShowOpenFiles(30);
            //     myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            //     currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
            // }

             if (strcmp(commandArgument1, (uint8_t*)"rcv\n"))
            {
                disableCursor();
                clearScreen();
                printPrompt(myPid);           

                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                printString(COLOR_WHITE, 5, 5, (uint8_t*)"Waiting to receive");

                systemNetRcv();

                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                uint32_t currentFileDescriptor;
                currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

                systemShowOpenFiles(30);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
            }
            

        }
        else if (strcmp(command, openFileCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid file for open - usage: open <file>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();
            printPrompt(myPid);    
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            clearScreen();

            systemOpenFile(commandArgument1, RDONLY);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
            printBufferToScreen((uint8_t *)openBufferTable->buffers[currentFileDescriptor]);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        }
        else if (strcmp(command, saveFileCommand) == 0)
        {
            if (commandArgument1 == NULL || commandArgument2 == NULL ||
                strlen(commandArgument1) == 0 || strlen(commandArgument2) == 0 ||
                !isDigit(commandArgument2[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid arguments for save - usage: save <name> <fd>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();
            printPrompt(myPid);    
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            clearScreen();

            systemCreateFile(commandArgument1, atoi(commandArgument2));

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
        }
        else if (strcmp(command, viewFileCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0 || !isDigit(commandArgument1[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid fd for view - usage: view <fd>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();
            printPrompt(myPid);           

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
            printBufferToScreen((uint8_t *)openBufferTable->buffers[atoi(commandArgument1)]);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            fileContentsOnScreen = true;
        }
        else if (strcmp(command, editFileCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid file for edit - usage: edit <file>");
                wait(2);
                continue;
            }
            clearScreen();
            enableCursor();

            moveCursor(1, 1);

            printPrompt(myPid);    

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemOpenFile(commandArgument1, RDWRITE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
            printBufferToScreen((uint8_t *)openBufferTable->buffers[currentFileDescriptor]);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            fileContentsOnScreen = true;

        }
        else if (strcmp(command, modifyBufferCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0 || !isDigit(commandArgument1[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid fd for mod - usage: mod <fd>");
                wait(2);
                continue;
            }
            clearScreen();
            enableCursor();

            moveCursor(1, 1);

            printPrompt(myPid);           

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            uint32_t currentFileDescriptor;
            currentFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

            struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
            printBufferToScreen((uint8_t *)openBufferTable->buffers[atoi(commandArgument1)]);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            fileContentsOnScreen = true;

        }
        else if (strcmp(command, closeFileCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0 || !isDigit(commandArgument1[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid fd for close - usage: close <fd>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();

            fileContentsOnScreen = false;

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            printPrompt(myPid);           

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemCloseFile(atoi(commandArgument1));
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            clearScreen();

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, writePipeCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0 || !isDigit(commandArgument1[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid fd for wpipe - usage: wpipe <fd>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();

            fileContentsOnScreen = false;

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            printPrompt(myPid);           

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemWritePipe(commandArgument1);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            clearScreen();

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, readPipeCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0 || !isDigit(commandArgument1[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid fd for rpipe - usage: rpipe <fd>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();

            fileContentsOnScreen = false;

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            printPrompt(myPid);           

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemReadPipe(commandArgument1);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            clearScreen();

            struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
            printBufferToScreen((uint8_t *)openBufferTable->buffers[atoi(commandArgument1)]);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, switchCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0 || !isDigit(commandArgument1[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid PID for switch - usage: switch <pid>");
                wait(2);
                continue;
            }
            disableCursor();
            clearScreen();
            printPrompt(myPid);            

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            
            clearScreen();

            systemTaskSwitch(commandArgument1);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, exitCommand) == 0)
        {
            disableCursor();
            clearScreen();
            printPrompt(myPid);  

            systemExit(PROCESS_EXIT_CODE_SUCCESS);       
        }
        else if (strcmp(command, helpCommand) == 0)
        {
            disableCursor();
            clearScreen();
            printString(COLOR_LIGHT_BLUE, 1, 0, (uint8_t *)"Commands available:");
            printString(COLOR_WHITE, 3, 3, (uint8_t *)"cls = Clear screen");
            printString(COLOR_WHITE, 4, 3, (uint8_t *)"open = Open a file as read-only"); 
            printString(COLOR_WHITE, 5, 3, (uint8_t *)"close = Close file"); 
            printString(COLOR_WHITE, 6, 3, (uint8_t *)"view = View a file");
            printString(COLOR_WHITE, 7, 3, (uint8_t *)"help = This menu"); 
            printString(COLOR_WHITE, 8, 3, (uint8_t *)"ps = Show processes");
            printString(COLOR_WHITE, 9, 3, (uint8_t *)"q = Exit Kernel Explorer");
            printString(COLOR_WHITE, 10, 3, (uint8_t *)"ls = Show root directory");
            printString(COLOR_WHITE, 11, 3, (uint8_t *)"edit = Edit a file");
            printString(COLOR_WHITE, 12, 3, (uint8_t *)"sw = Switch processes");
            printString(COLOR_WHITE, 13, 3, (uint8_t *)"ins = Insert in the screen editor");
            printString(COLOR_WHITE, 14, 3, (uint8_t *)"save <name> <fd> = save");
            printString(COLOR_WHITE, 15, 3, (uint8_t *)"mod = Modify an open buffer");
            printString(COLOR_WHITE, 3, 40, (uint8_t *)"wpipe = write FD to pipe");
            printString(COLOR_WHITE, 4, 40, (uint8_t *)"rpipe = read pipe to FD");
            printString(COLOR_WHITE, 5, 40, (uint8_t *)"opipe = Open a pipe by name");
            printString(COLOR_WHITE, 6, 40, (uint8_t *)"cpipe = Create a pipe with a name");
            printString(COLOR_WHITE, 7, 40, (uint8_t *)"cnpipe = Net pipe <name> <dstip> <dstpt>");
            printString(COLOR_WHITE, 8, 40, (uint8_t *)"net send <msg> = Send UDP packet");
            printString(COLOR_WHITE, 9, 40, (uint8_t *)"go = Global objects");
            printString(COLOR_WHITE, 10, 40, (uint8_t *)"m <hex addr> = View memory");
            printString(COLOR_WHITE, 11, 40, (uint8_t *)"d <hex addr> = Disassemble memory");
            printString(COLOR_WHITE, 12, 40, (uint8_t *)"s <sector hex> = Sector dump");
            printString(COLOR_WHITE, 13, 40, (uint8_t *)"b <block hex> = Block dump");
            printString(COLOR_WHITE, 14, 40, (uint8_t *)"br = Block dump of root dir");
            printString(COLOR_WHITE, 15, 40, (uint8_t *)"e <hex addr> = ELF header view");
            printString(COLOR_WHITE, 16, 3, (uint8_t *)"i <hex inode> = Inode details");
            printString(COLOR_WHITE, 16, 40, (uint8_t *)"kl = View kernel log");
            printString(COLOR_WHITE, 17, 3, (uint8_t *)"cd = Change directory");
        }
        else if (strcmp(command, psCommand) == 0)
        {
            disableCursor();
            clearScreen();
            printPrompt(myPid);  
            systemShowProcesses();

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, dirCommand) == 0)
        {
            disableCursor();
            clearScreen();
            printPrompt(myPid);  
            systemListDirectory();

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else
        {
            printPrompt(myPid);             
            printString(COLOR_RED, 21, 3,(uint8_t *)"Command not recognized!");
            //systemBeep();
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            fillMemory((uint8_t *)KEYBOARD_BUFFER, (uint8_t)0x0, (KEYBOARD_BUFFER_SIZE * 2));
            fillMemory((uint8_t *)SHELL_CURSOR_POS, (uint8_t)0x0, 40);
            wait(1);
            printString(COLOR_WHITE, 21, 3,(uint8_t *)"                                ");

            systemShowOpenFiles(30);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            //freeAll(myPid);
            //main();

        }    
        freeAll(myPid);

    }


}