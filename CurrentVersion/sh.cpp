// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Most of the code written by Dan O'Malley
// Argument tweaks/tokenization, shell script, 
// and command history written by Grok.
// 12/2025 with Grok v4.


#include "screen.h"
#include "keyboard.h"
#include "vm.h"
#include "libc-main.h"
#include "constants.h"
#include "x86.h"
#include "file.h"
#include "sound.h"

uint8_t history_buf[30][30];
uint32_t history_index = 0;
uint32_t history_count = 0;

// Commands without "\n"
uint8_t *clearScreenCommand = (uint8_t *)"cls"; 
uint8_t *helpCommand = (uint8_t *)"help";
uint8_t *historyCommand = (uint8_t *)"h";
uint8_t *psCommand = (uint8_t *)"ps";
uint8_t *deleteCommand = (uint8_t *)"rm";
uint8_t *newCommand = (uint8_t *)"new";
uint8_t *exitCommand = (uint8_t *)"q";
uint8_t *freeCommand = (uint8_t *)"free";
uint8_t *killCommand = (uint8_t *)"kill";
uint8_t *switchCommand = (uint8_t *)"sw";
uint8_t *memCommand = (uint8_t *)"m";
uint8_t *dirCommand = (uint8_t *)"ls";
uint8_t *schedCommand = (uint8_t *)"sched";
uint8_t *globalObjectsCommand = (uint8_t *)"go";
uint8_t *kernelLogCommand = (uint8_t *)"kl";
uint8_t *changeDirectoryCommand = (uint8_t *)"cd";
uint8_t *moveCommand = (uint8_t *)"mv";
uint8_t *changeFileModeCommand = (uint8_t *)"chmod";


uint32_t findShellScriptFile(uint8_t *inputString) 
{
    uint32_t len = strlen(inputString);
    if (len < 3) {
        return 0;  // Too short to end with ".sh"
    }
    // Compare the last 3 characters with ".sh"
    if (strcmp(inputString + len - 3, (uint8_t *)".sh") == 0) {
        return 1;  // True
    }
    return 0;  // False
}


void printPrompt(uint8_t myPid)
{
    //printLogo(45);
    
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

    uint8_t * mypidString = malloc(myPid, 20);
    itoa(myPid, mypidString);

    printString(COLOR_GREEN, 43, 3, (uint8_t *)"Sh v2.0");
    printString(COLOR_LIGHT_BLUE, 44, 2, (uint8_t *)"$");
    printString(COLOR_GREEN, 43, 70, (uint8_t *)"PID: ");
    printString(COLOR_GREEN, 43, 75, mypidString);

    free(mypidString);

}



void main()
{
    uint32_t myPid;

    //disableCursor();

    myPid = readValueFromMemLoc(RUNNING_PID_LOC);

    clearScreen();

    // clearing shared buffer area
    fillMemory((uint8_t *)KEYBOARD_BUFFER, (uint8_t)0x0, PAGE_SIZE);
    uint8_t *commandArgument1 = malloc(myPid, 32);
    uint8_t *commandArgument2 = malloc(myPid, 32);
    uint8_t *commandArgument3 = malloc(myPid, 32);
    uint8_t *commandArgument4 = malloc(myPid, 32);
    if (commandArgument1 == 0 || commandArgument2 == 0 || commandArgument3 == 0 || commandArgument4 == 0) {
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

        if (strlen(COMMAND_BUFFER) > 1)
        {
            uint32_t len = 30;
            bytecpy(history_buf[history_index], COMMAND_BUFFER, len);
            history_buf[history_index][len] = 0x0;
            if (history_buf[history_index][len - 1] == 0x0a) history_buf[history_index][len - 1] = 0x0;
            history_index = (history_index + 1) % 30;
            if (history_count < 30) history_count++;
        }

        // Tokenization
        // Ensure null terminator after \n
        uint8_t *end = COMMAND_BUFFER;
        while (*end != 0x0A && *end != 0x00) end++;
        if (*end == 0x0A) *end = 0x00;  // Replace \n with \0

        // Tokenize: commandArgument1/2/3/4 will point to tokens or NULL if missing
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

        
        
        if (strcmp(command, clearScreenCommand) == 0)
        {
            clearScreen();
        }
        else if (strcmp(command, helpCommand) == 0)
        {
            clearScreen();
            printString(COLOR_LIGHT_BLUE, 26, 0, (uint8_t *)"Commands available:");
            printString(COLOR_WHITE, 28, 3, (uint8_t *)"cls = Clear screen");
            printString(COLOR_WHITE, 29, 3, (uint8_t *)"h = Show command history");
            printString(COLOR_WHITE, 30, 3, (uint8_t *)"ps = Show processes");
            printString(COLOR_WHITE, 31, 3, (uint8_t *)"q = Exit the Shell");
            printString(COLOR_WHITE, 32, 3, (uint8_t *)"free = Free all zombie processes");
            printString(COLOR_WHITE, 33, 3, (uint8_t *)"kill = Kill a process");
            printString(COLOR_WHITE, 34, 3, (uint8_t *)"sw = Switch processes");
            printString(COLOR_WHITE, 35, 3, (uint8_t *)"cd = Change directory");
            printString(COLOR_WHITE, 36, 3, (uint8_t *)"mv = Move <file> <src path> <des path>");
            printString(COLOR_WHITE, 28, 47, (uint8_t *)"mem <hex addr> = Show memory");
            printString(COLOR_WHITE, 29, 47, (uint8_t *)"ls = Show root directory");
            printString(COLOR_WHITE, 30, 47, (uint8_t *)"sched = Toggle kernel scheduler");
            printString(COLOR_WHITE, 31, 47, (uint8_t *)"rm <file> = Delete a file");
            printString(COLOR_WHITE, 32, 47, (uint8_t *)"new = Create a new empty file");
            printString(COLOR_WHITE, 33, 47, (uint8_t *)"go = Global objects");
            printString(COLOR_WHITE, 34, 47, (uint8_t *)"kl = View kernel log");
            printString(COLOR_WHITE, 35, 47, (uint8_t *)"chmod = chmod <file> <-rwxrwxrwx>");

            
        }
        else if (strcmp(command, historyCommand) == 0)
        {
            clearScreen();
            printPrompt(myPid);
            uint32_t row = 5;
            printString(COLOR_LIGHT_BLUE, row-1, 3, (uint8_t*)"Command History");
            row++;
            for (uint32_t i = 0; i < history_count; i++)
            {
                uint32_t idx = (history_index + i - history_count + 30) % 30;
                printString(COLOR_WHITE, row + i, 3, history_buf[idx]);
            }
            //waitForEnterOrQuit();
        }
        else if (strcmp(command, exitCommand) == 0)
        {
            clearScreen();
            printPrompt(myPid);  
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
            systemExit(PROCESS_EXIT_CODE_SUCCESS);            
        }
        else if (strcmp(command, freeCommand) == 0)
        {
            clearScreen();
            printPrompt(myPid);  

            systemFree();
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

            clearScreen();
            printString(COLOR_WHITE, 26, 5, (uint8_t *)"All zombie processes freed, along with their page frames...");

        }
        else if (strcmp(command, killCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0 || !isDigit(commandArgument1[0])) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid PID for kill - usage: kill <pid>");
                wait(2);
                continue;
            }
            clearScreen();
            printPrompt(myPid);             
            systemKill(commandArgument1);
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
        }
        else if (strcmp(command, kernelLogCommand) == 0)
        {
            clearScreen();

            systemKernelLogViewer();
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
            clearScreen();
            printPrompt(myPid);            

            systemTaskSwitch(commandArgument1);
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, memCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid address for mem - usage: m <hex addr>");
                wait(2);
                continue;
            }
            clearScreen();

            systemShowMemory((uint8_t*)(hextoi(commandArgument1)));
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, psCommand) == 0)
        {
            clearScreen();
            printPrompt(myPid);  
            systemShowProcesses();
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, deleteCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid file for rm - usage: rm <file>");
                wait(2);
                continue;
            }
            clearScreen();
            printPrompt(myPid);      

            systemDeleteFile(commandArgument1);
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, changeFileModeCommand) == 0)
        {
            if (commandArgument1 == NULL || strlen(commandArgument1) == 0) {
                clearScreen();
                printPrompt(myPid);
                printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid file for chmod - usage: chmod -rwxrwxrwx");
                wait(2);
                continue;
            }
            clearScreen();
            printPrompt(myPid);      

            systemChangeFileMode(commandArgument1, commandArgument2);
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, moveCommand) == 0)
        {
            // if (commandArgument1 == NULL || commandArgument2 == NULL || commandArgument3 == NULL ||
            //     strlen(commandArgument1) == 0 || strlen(commandArgument2) == 0 || strlen(commandArgument3) == 0 ||
            //     !isAlpha(commandArgument2[0]) || !isAlpha(commandArgument3[0])) {
            //     clearScreen();
            //     printPrompt(myPid);
            //     printString(COLOR_RED, 26, 3, (uint8_t *)"Invalid arguments for mv - usage: mv <src> <inode1> <inode2>");
            //     wait(2);
            //     continue;
            // }
            clearScreen();
            printPrompt(myPid);      

            systemMoveFile(commandArgument1, commandArgument2, commandArgument3);
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

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
        else if (strcmp(command, newCommand) == 0)
        {
            if (commandArgument1 == NULL || commandArgument2 == NULL ||
                strlen(commandArgument1) == 0 || strlen(commandArgument2) == 0 ||
                !isDigit(commandArgument2[0]) || atoi(commandArgument2) == 0) {
                clearScreen();
                printPrompt(myPid);             
                printString(COLOR_RED, 26, 3,(uint8_t *)"Invalid arguments for new - usage: new <file> <pages>");
                wait(2);
                continue;
            }
            clearScreen();
            printPrompt(myPid);       

            systemOpenEmptyFile(commandArgument1, atoi(commandArgument2));

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, dirCommand) == 0)
        {
            myPid = readValueFromMemLoc(RUNNING_PID_LOC);
            
            clearScreen();
            printPrompt(myPid);  
            systemListDirectory();
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, globalObjectsCommand) == 0)
        {
            clearScreen();
            printPrompt(myPid);  
            systemShowGlobalObjects();
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else if (strcmp(command, schedCommand) == 0)
        {
            clearScreen();
            systemSchedulerToggle();
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

            myPid = readValueFromMemLoc(RUNNING_PID_LOC);

        }
        else
        {
            clearScreen();
            printPrompt(myPid);         
            
            uint32_t priority = 50;
            if (commandArgument1 != NULL && strlen(commandArgument1) > 0 && isDigit(commandArgument1[0])) {
                priority = atoi(commandArgument1);
                if (priority == 0) priority = 50;
                if (priority > 255) {
                    clearScreen();
                    printPrompt(myPid);             
                    printString(COLOR_RED, 26, 3,(uint8_t *)"Run priority too high...must be <= 255.");
                    myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                    fillMemory((uint8_t *)KEYBOARD_BUFFER, (uint8_t)0x0, (KEYBOARD_BUFFER_SIZE * 2));
                    wait(2);

                    clearScreen();
                    continue;
                }
            }

            if (commandArgument2 != NULL && strcmp((uint8_t*)commandArgument2, (uint8_t*)"|") == 0 )
            {
                uint32_t pipeInFd;
                uint32_t pipeOutFd;
                uint32_t currentInputFileDescriptor;
                uint32_t currentTempFileDescriptor;
                uint8_t* commandString = malloc(myPid, 30);
                strcpy(commandString, command);
                uint8_t* commandArgument1String = malloc(myPid, 30);
                strcpy(commandArgument1String, commandArgument1);
                uint8_t* commandArgument2String = malloc(myPid, 30);
                strcpy(commandArgument2String, commandArgument2);
                uint8_t* commandArgument3String = malloc(myPid, 30);
                strcpy(commandArgument3String, commandArgument3);
                uint8_t* commandArgument4String = malloc(myPid, 30);
                strcpy(commandArgument4String, commandArgument4);
                uint32_t rowToPrint = 25;

                struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;

                printString(COLOR_LIGHT_BLUE, rowToPrint++, 5, (uint8_t*)"Shell opening file: ");
                printString(COLOR_WHITE, rowToPrint-1, 25, commandArgument1String);
                wait(1);

                systemOpenFile((uint8_t*)commandArgument1String, RDONLY);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                currentInputFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

                systemOpenEmptyFile(strConcat(removeExtension((uint8_t*)commandArgument1String), (uint8_t*)".tmp\n"), 1);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                currentTempFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

                systemCloseFile(currentTempFileDescriptor);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                currentTempFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

                systemOpenFile(strConcat(removeExtension((uint8_t*)commandArgument1String), (uint8_t*)".tmp\n"), RDWRITE);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                currentTempFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

                printString(COLOR_LIGHT_BLUE, rowToPrint++, 5, (uint8_t*)"Shell creating pipe: ");
                printString(COLOR_WHITE, rowToPrint-1, 26, (uint8_t*)"pipein");
                wait(1);
                
                systemCreatePipe((uint8_t*)"pipein");
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                pipeInFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

                bytecpy((uint8_t*)openBufferTable->buffers[pipeInFd], (uint8_t*)openBufferTable->buffers[currentInputFileDescriptor], (uint32_t)openBufferTable->bufferSize[currentInputFileDescriptor]);

                uint8_t* pipeInFdString = malloc(myPid, 20);
                if (pipeInFdString == 0) {
                    clearScreen();
                    printPrompt(myPid);
                    printString(COLOR_RED, 26, 3, (uint8_t *)"Malloc failed - out of memory!");
                    wait(2);
                    continue;
                }

                printString(COLOR_LIGHT_BLUE, rowToPrint++, 5, (uint8_t*)"Shell writing input file to pipe: ");
                printString(COLOR_WHITE, rowToPrint-1, 39, (uint8_t*)"pipein");
                wait(1);

                itoa(pipeInFd, pipeInFdString);
                systemWritePipe(pipeInFdString);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                printString(COLOR_LIGHT_BLUE, rowToPrint++, 5, (uint8_t*)"Shell creating pipe: ");
                printString(COLOR_WHITE, rowToPrint-1, 26, (uint8_t*)"pipeout");
                wait(1);

                systemCreatePipe((uint8_t*)"pipeout");
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                pipeOutFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

                printString(COLOR_LIGHT_BLUE, rowToPrint++, 5, (uint8_t*)"Shell executing: ");
                printString(COLOR_WHITE, rowToPrint-1, 22, commandString);
                printString(COLOR_GREEN, rowToPrint-1, 30, (uint8_t*)"STDIN = pipein, STDOUT = pipeout");
                wait(1);

                systemExec(commandString, 50, pipeInFd, pipeOutFd, 2);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                fillMemory(COMMAND_BUFFER, 0x0, 100);
                bytecpy(COMMAND_BUFFER, commandArgument4String, strlen(commandArgument4String));

                printString(COLOR_LIGHT_BLUE, rowToPrint++, 5, (uint8_t*)"Shell executing: ");
                printString(COLOR_WHITE, rowToPrint-1, 22, commandArgument3String);
                printString(COLOR_GREEN, rowToPrint-1, 30, (uint8_t*)"STDIN = pipeout, STDOUT = pipein");
                wait(1);

                systemExec(commandArgument3String, 50, pipeOutFd, pipeInFd ,0);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                printPrompt(myPid);

                printString(COLOR_LIGHT_BLUE, rowToPrint++, 5, (uint8_t*)"Shell reading from pipe: ");
                printString(COLOR_WHITE, rowToPrint-1, 30, (uint8_t*)"pipein");
                wait(1);

                systemReadPipe(pipeInFdString);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                printString(COLOR_LIGHT_BLUE, rowToPrint++, 5, (uint8_t*)"Shell writing output file and closing pipes and files");
                wait(1);

                systemCreateFile(strConcat(removeExtension((uint8_t*)commandArgument1String), (uint8_t*)".out"), pipeInFd);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                systemCloseFile(currentInputFileDescriptor);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                systemCloseFile(currentTempFileDescriptor);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                systemCloseFile(pipeInFd);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                systemCloseFile(pipeOutFd);
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                systemDeleteFile(strConcat(removeExtension((uint8_t*)commandArgument1String), (uint8_t*)".tmp"));
                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                free(pipeInFdString);
                free(commandString);
                free(commandArgument1String);
                free(commandArgument2String);
                free(commandArgument3String);
                free(commandArgument4String);

            }
            else if (findShellScriptFile(command))
            {
                uint8_t * savedCommand = malloc(myPid, 30);
                strcpy(savedCommand, command);
                
                bytecpy(COMMAND_BUFFER, (uint8_t*)"src " , strlen((uint8_t*)"src "));
                bytecpy(COMMAND_BUFFER + strlen((uint8_t*)"src "), savedCommand, strlen(savedCommand) + strlen((uint8_t*)"src "));

                systemExec((uint8_t*)"src", priority, 0, 0, 0);
                fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

                myPid = readValueFromMemLoc(RUNNING_PID_LOC);
                free(savedCommand);

                clearScreen();
                printPrompt(myPid);
            }
            else
            {

                systemExec(command, priority, 0, 0, 0);
                fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);

                myPid = readValueFromMemLoc(RUNNING_PID_LOC);

                clearScreen();
                printPrompt(myPid);
            
            }
        }    
        freeAll(myPid);

    }


}