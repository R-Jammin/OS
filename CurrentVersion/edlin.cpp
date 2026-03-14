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
#include "net.h"

#define MAX_LINES 250
#define MAX_LINE_LENGTH 65

#define MAX_DISPLAY_LINES 35
#define PROMPT_ROW (MAX_DISPLAY_LINES)
#define STATUS_ROW (PROMPT_ROW + 1)

// Array to store the lines of text being edited
uint8_t textBuffer[MAX_LINES][MAX_LINE_LENGTH + 1];
uint32_t totalLines = 0;
uint32_t currentLineIndex = 0; // 0-based index
uint32_t displayStartLineNumber = 1;

// Forward declarations
void listTextLines(uint32_t startLineNumber);
uint32_t getInputColumn();
void printCommandPrompt();
void printTextLine(uint32_t color, uint32_t row, uint32_t column, uint8_t *text);

// Converts an integer value to a string representation with leading zero padding to ensure it meets the specified minimum number of digits.
// This function modifies the provided buffer in place to include the padded string.
// @param value The integer value to convert.
// @param strBuffer The buffer where the resulting string will be stored.
// @param minDigits The minimum number of digits the string should have, with leading zeros if necessary.
void padIntegerToString(uint32_t value, uint8_t *strBuffer, uint32_t minDigits)
{
    itoa(value, strBuffer);
    uint32_t strLength = strlen(strBuffer);
    if (strLength >= minDigits) return;
    uint32_t paddingLength = minDigits - strLength;
    for (uint32_t i = strLength; i > 0; i--)
    {
        strBuffer[i + paddingLength - 1] = strBuffer[i - 1];
    }
    for (uint32_t i = 0; i < paddingLength; i++)
    {
        strBuffer[i] = '0';
    }
    strBuffer[minDigits] = 0;
}

// Reads a line of input from the keyboard into the provided buffer, handling special keys like backspace, enter, and optionally page up/down for paging.
// This function manages cursor movement, character printing, and shift key states. It supports paging if enabled, which allows scrolling through displayed content.
// @param inputBuffer The buffer to store the input line.
// @param promptRow The row on the screen where input is being read.
// @param startColumn The starting column for input.
// @param enablePaging Flag to enable handling of page up/down keys for scrolling.
// @return The length of the input read (position in the buffer).
uint32_t readInputLine(uint8_t *inputBuffer, uint32_t promptRow, uint32_t startColumn, uint32_t enablePaging)
{
    uint32_t inputPosition = 0;
    uint32_t cursorColumn = startColumn;
    uint8_t shiftPressed = 0;
    moveCursor(promptRow, cursorColumn);
    while (true)
    {
        uint8_t tempScanCode;
        uint8_t scanCode;
        do
        {
            readKeyboard(&tempScanCode);
            if (tempScanCode == 0x2A || tempScanCode == 0x36)
            {
                shiftPressed = 1;
                continue;
            }
            if (tempScanCode == 0xAA || tempScanCode == 0xB6)
            {
                shiftPressed = 0;
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
            inputBuffer[inputPosition] = 0;
            break;
        }
        else if (scanCode == 0x0E)
        { // Backspace key
            if (inputPosition > 0)
            {
                inputPosition--;
                cursorColumn--;
                uint8_t spaceCharacter = 0x20;
                printCharacter(COLOR_WHITE, promptRow, cursorColumn, &spaceCharacter);
                moveCursor(promptRow, cursorColumn);
            }
        }
        else if (enablePaging && scanCode == 0x49) // Page Up
        {
            if (displayStartLineNumber > MAX_DISPLAY_LINES)
            {
                displayStartLineNumber -= MAX_DISPLAY_LINES;
            }
            else
            {
                displayStartLineNumber = 1;
            }
            listTextLines(displayStartLineNumber);
            printCommandPrompt();
            // Clear the input line
            for (uint32_t c = getInputColumn(); c < 70; c++)
            {
                uint8_t spaceCharacter = 0x20;
                printCharacter(COLOR_WHITE, promptRow, c, &spaceCharacter);
            }
            // Reprint the buffer
            for (uint32_t i = 0; i < inputPosition; i++)
            {
                printCharacter(COLOR_WHITE, promptRow, getInputColumn() + i, &inputBuffer[i]);
            }
            cursorColumn = getInputColumn() + inputPosition;
            moveCursor(promptRow, cursorColumn);
            continue;
        }
        else if (enablePaging && scanCode == 0x51) // Page Down
        {
            displayStartLineNumber += MAX_DISPLAY_LINES;
            if (displayStartLineNumber > totalLines)
            {
                displayStartLineNumber = (totalLines > MAX_DISPLAY_LINES - 1) ? totalLines - (MAX_DISPLAY_LINES - 1) : 1;
            }
            listTextLines(displayStartLineNumber);
            printCommandPrompt();
            // Clear the input line
            for (uint32_t c = getInputColumn(); c < 70; c++)
            {
                uint8_t spaceCharacter = 0x20;
                printCharacter(COLOR_WHITE, promptRow, c, &spaceCharacter);
            }
            // Reprint the buffer
            for (uint32_t i = 0; i < inputPosition; i++)
            {
                printCharacter(COLOR_WHITE, promptRow, getInputColumn() + i, &inputBuffer[i]);
            }
            cursorColumn = getInputColumn() + inputPosition;
            moveCursor(promptRow, cursorColumn);
            continue;
        }
        else
        {
            uint8_t character = 0;
            if (scanCode == 0x10)
            {
                character = shiftPressed ? 0x51 : 0x71;
            }
            else if (scanCode == 0x11)
            {
                character = shiftPressed ? 0x57 : 0x77;
            }
            else if (scanCode == 0x12)
            {
                character = shiftPressed ? 0x45 : 0x65;
            }
            else if (scanCode == 0x13)
            {
                character = shiftPressed ? 0x52 : 0x72;
            }
            else if (scanCode == 0x14)
            {
                character = shiftPressed ? 0x54 : 0x74;
            }
            else if (scanCode == 0x15)
            {
                character = shiftPressed ? 0x59 : 0x79;
            }
            else if (scanCode == 0x16)
            {
                character = shiftPressed ? 0x55 : 0x75;
            }
            else if (scanCode == 0x17)
            {
                character = shiftPressed ? 0x49 : 0x69;
            }
            else if (scanCode == 0x18)
            {
                character = shiftPressed ? 0x4F : 0x6F;
            }
            else if (scanCode == 0x19)
            {
                character = shiftPressed ? 0x50 : 0x70;
            }
            else if (scanCode == 0x1E)
            {
                character = shiftPressed ? 0x41 : 0x61;
            }
            else if (scanCode == 0x1F)
            {
                character = shiftPressed ? 0x53 : 0x73;
            }
            else if (scanCode == 0x20)
            {
                character = shiftPressed ? 0x44 : 0x64;
            }
            else if (scanCode == 0x21)
            {
                character = shiftPressed ? 0x46 : 0x66;
            }
            else if (scanCode == 0x22)
            {
                character = shiftPressed ? 0x47 : 0x67;
            }
            else if (scanCode == 0x23)
            {
                character = shiftPressed ? 0x48 : 0x68;
            }
            else if (scanCode == 0x24)
            {
                character = shiftPressed ? 0x4A : 0x6A;
            }
            else if (scanCode == 0x25)
            {
                character = shiftPressed ? 0x4B : 0x6B;
            }
            else if (scanCode == 0x26)
            {
                character = shiftPressed ? 0x4C : 0x6C;
            }
            else if (scanCode == 0x2C)
            {
                character = shiftPressed ? 0x5A : 0x7A;
            }
            else if (scanCode == 0x2D)
            {
                character = shiftPressed ? 0x58 : 0x78;
            }
            else if (scanCode == 0x2E)
            {
                character = shiftPressed ? 0x43 : 0x63;
            }
            else if (scanCode == 0x2F)
            {
                character = shiftPressed ? 0x56 : 0x76;
            }
            else if (scanCode == 0x30)
            {
                character = shiftPressed ? 0x42 : 0x62;
            }
            else if (scanCode == 0x31)
            {
                character = shiftPressed ? 0x4E : 0x6E;
            }
            else if (scanCode == 0x32)
            {
                character = shiftPressed ? 0x4D : 0x6D;
            }
            else if (scanCode == 0x02)
            {
                character = shiftPressed ? 0x21 : 0x31;
            }
            else if (scanCode == 0x03)
            {
                character = shiftPressed ? 0x40 : 0x32;
            }
            else if (scanCode == 0x04)
            {
                character = shiftPressed ? 0x23 : 0x33;
            }
            else if (scanCode == 0x05)
            {
                character = shiftPressed ? 0x24 : 0x34;
            }
            else if (scanCode == 0x06)
            {
                character = shiftPressed ? 0x25 : 0x35;
            }
            else if (scanCode == 0x07)
            {
                character = shiftPressed ? 0x5E : 0x36;
            }
            else if (scanCode == 0x08)
            {
                character = shiftPressed ? 0x26 : 0x37;
            }
            else if (scanCode == 0x09)
            {
                character = shiftPressed ? 0x2A : 0x38;
            }
            else if (scanCode == 0x0A)
            {
                character = shiftPressed ? 0x28 : 0x39;
            }
            else if (scanCode == 0x0B)
            {
                character = shiftPressed ? 0x29 : 0x30;
            }
            else if (scanCode == 0x0C)
            {
                character = shiftPressed ? 0x5F : 0x2D;
            }
            else if (scanCode == 0x0D)
            {
                character = shiftPressed ? 0x2B : 0x3D;
            }
            else if (scanCode == 0x1A)
            {
                character = shiftPressed ? 0x7B : 0x5B;
            }
            else if (scanCode == 0x1B)
            {
                character = shiftPressed ? 0x7D : 0x5D;
            }
            else if (scanCode == 0x2B)
            {
                character = shiftPressed ? 0x7C : 0x5C;
            }
            else if (scanCode == 0x33)
            {
                character = shiftPressed ? 0x3C : 0x2C;
            }
            else if (scanCode == 0x34)
            {
                character = shiftPressed ? 0x3E : 0x2E;
            }
            else if (scanCode == 0x35)
            {
                character = shiftPressed ? 0x3F : 0x2F;
            }
            else if (scanCode == 0x27)
            {
                character = shiftPressed ? 0x3A : 0x3B;
            }
            else if (scanCode == 0x28)
            {
                character = shiftPressed ? 0x22 : 0x27;
            }
            else if (scanCode == 0x39)
            {
                character = 0x20;
            }
            if (character != 0 && inputPosition < MAX_LINE_LENGTH)
            {
                inputBuffer[inputPosition] = character;
                inputPosition++;
                printCharacter(COLOR_WHITE, promptRow, cursorColumn, &character);
                cursorColumn++;
                moveCursor(promptRow, cursorColumn);
            }
        }
    }
    return inputPosition;
}

// Prints the command prompt on the screen, including the current line number padded to three digits, followed by ':' and '*'.
// This prompt indicates where the user can enter commands or line numbers.
void printCommandPrompt()
{
    uint8_t lineNumStr[10];
    padIntegerToString(currentLineIndex + 1, lineNumStr, 3);
    uint32_t strLen = 3;
    bytecpy(lineNumStr + strLen, (uint8_t *)":", 1);
    strLen += 1;
    lineNumStr[strLen] = 0;
    printString(COLOR_GREEN, PROMPT_ROW, 0, lineNumStr);
    printString(COLOR_RED, PROMPT_ROW, strLen, (uint8_t *)"*");
}

// Returns the starting column position for user input after the command prompt.
// This is hardcoded based on the prompt length (3 digits + ':' + '*') = 5 columns.
uint32_t getInputColumn()
{
    return 5; // 3 digits + ':' + '*'
}

// Prints a line of text to the screen at the specified row and column, handling tab characters by expanding them to the next tab stop (every 8 columns).
// Tabs are replaced with the appropriate number of spaces to align to tab stops.
// @param color The color to use for printing the text.
// @param row The screen row to print on.
// @param column The starting column to print from.
// @param text The text string to print.
void printTextLine(uint32_t color, uint32_t row, uint32_t column, uint8_t *text)
{
    uint32_t currentColumn = column;
    for (uint32_t i = 0; text[i] != 0; i++)
    {
        if (text[i] == 0x09)
        { // Tab character
            uint32_t nextTabStop = ((currentColumn + 8) / 8) * 8;
            uint32_t spaceCount = nextTabStop - currentColumn;
            for (uint32_t s = 0; s < spaceCount; s++)
            {
                uint8_t spaceCharacter = 0x20;
                printCharacter(color, row, currentColumn, &spaceCharacter);
                currentColumn++;
            }
        }
        else
        {
            printCharacter(color, row, currentColumn, &text[i]);
            currentColumn++;
        }
    }
}

// Lists and displays the text lines on the screen starting from the specified start line number (1-based).
// Each line is prefixed with its line number padded to three digits, followed by ': '.
// Display is limited to MAX_DISPLAY_LINES to fit the screen. Also calls to show open files.
// @param startLineNumber The 1-based line number to start displaying from.
void listTextLines(uint32_t startLineNumber)
{
    clearScreen();
    uint32_t displayRow = 0;
    for (uint32_t lineIdx = startLineNumber - 1; lineIdx < totalLines; lineIdx++)
    {
        if (displayRow >= MAX_DISPLAY_LINES) break; // Limit to screen height
        uint8_t lineNumStr[10];
        padIntegerToString(lineIdx + 1, lineNumStr, 3);
        uint32_t strLen = 3;
        bytecpy(lineNumStr + strLen, (uint8_t *)": ", 2);
        strLen += 2;
        lineNumStr[strLen] = 0;
        printString(COLOR_GREEN, displayRow, 0, lineNumStr);
        printTextLine(COLOR_WHITE, displayRow, strLen, textBuffer[lineIdx]);
        displayRow++;
    }
    printCommandPrompt();

    systemShowOpenFiles(40);
}

// Allows editing of a specific line by displaying the current content and reading new input to replace it.
// If the line number is invalid or beyond current lines, it displays an error. Updates the total lines if necessary.
// @param lineNumber The 1-based line number to edit.
void editTextLine(uint32_t lineNumber)
{
    if (lineNumber < 1 || lineNumber > MAX_LINES)
    {
        printString(COLOR_RED, STATUS_ROW, 0, (uint8_t *)"Invalid line number");
        return;
    }
    if (lineNumber > totalLines) totalLines = lineNumber;
    uint32_t lineIndex = lineNumber - 1;
    printString(COLOR_GREEN, STATUS_ROW, 0, (uint8_t *)"Editing line ");
    uint8_t tempBuffer[10];
    padIntegerToString(lineNumber, tempBuffer, 3);
    printString(COLOR_GREEN, STATUS_ROW, 13, tempBuffer);
    printTextLine(COLOR_WHITE, STATUS_ROW + 1, 0, textBuffer[lineIndex]);
    readInputLine(textBuffer[lineIndex], STATUS_ROW + 2, 0, 0);
    currentLineIndex = lineIndex;
}

// Enters insert mode to add new lines starting at the current position until a single '.' is entered on a new line.
// Shifts existing lines down to make space for new insertions. Updates total lines and current index accordingly.
void insertNewLines()
{
    printString(COLOR_GREEN, STATUS_ROW, 0, (uint8_t *)"Insert mode, end with . on empty line");
    uint32_t insertPosition = currentLineIndex;
    while (true)
    {
        uint8_t lineBuffer[MAX_LINE_LENGTH + 1];
        uint32_t inputLength = readInputLine(lineBuffer, STATUS_ROW + 1, 0, 0);
        if (inputLength == 1 && lineBuffer[0] == '.') break;
        if (totalLines >= MAX_LINES) break;
        // Shift existing lines down to make space
        for (uint32_t i = totalLines; i > insertPosition; i--)
        {
            uint32_t lineLen = strlen(textBuffer[i-1]);
            bytecpy(textBuffer[i], textBuffer[i-1], lineLen);
            textBuffer[i][lineLen] = 0;
        }
        uint32_t lineLen = strlen(lineBuffer);
        bytecpy(textBuffer[insertPosition], lineBuffer, lineLen);
        textBuffer[insertPosition][lineLen] = 0;
        totalLines++;
        insertPosition++;
    }
    currentLineIndex = insertPosition - 1;
}

// Deletes the specified line by shifting subsequent lines up and decrementing the total line count.
// Adjusts the current line index if necessary to avoid pointing beyond the new total lines.
// @param lineNumber The 1-based line number to delete.
void deleteTextLine(uint32_t lineNumber)
{
    if (lineNumber < 1 || lineNumber > totalLines) return;
    uint32_t lineIndex = lineNumber - 1;
    for (uint32_t i = lineIndex; i < totalLines - 1; i++)
    {
        uint32_t lineLen = strlen(textBuffer[i + 1]);
        bytecpy(textBuffer[i], textBuffer[i + 1], lineLen);
        textBuffer[i][lineLen] = 0;
    }
    totalLines--;
    if (currentLineIndex >= totalLines) currentLineIndex = totalLines - 1;
}

// Loads the content of a file into the text buffer, parsing it into lines separated by newlines (0x0a).
// Clears existing buffer, sets total lines based on file content, and updates display variables.
// Displays a status message upon loading.
// @param fileName The name of the file to load.
// @param processId The process ID for system calls.
void loadFileContent(uint8_t *fileName, uint32_t processId)
{
    systemOpenFile(fileName, RDWRITE);

    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    //uint32_t fd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    uint32_t pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);
    uint8_t *content = (uint8_t *)pointer;
    totalLines = 0;

    fillMemory((uint8_t *)textBuffer, 0, MAX_LINES * (MAX_LINE_LENGTH + 1));
    uint32_t filePosition = 0;
    while (content[filePosition] != 0 && totalLines < MAX_LINES)
    {
        uint32_t linePosition = 0;
        while (content[filePosition] != 0x0a && content[filePosition] != 0 && linePosition < MAX_LINE_LENGTH)
        {
            textBuffer[totalLines][linePosition++] = content[filePosition++];
        }
        textBuffer[totalLines][linePosition] = 0;
        if (content[filePosition] == 0x0a) filePosition++;
        totalLines++;
    }
    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    printString(COLOR_GREEN, STATUS_ROW, 0, (uint8_t *)"File loaded");
    currentLineIndex = 0;
    displayStartLineNumber = 1;
}

// Saves the text buffer content to a file by first writing to a temporary file and then creating the target file.
// Calculates required space, handles newlines between lines, and manages file descriptors via system calls.
// Displays a status message upon saving and shows open files.
// @param fileName The name of the file to save to.
// @param processId The process ID for system calls.
void saveFileContent(uint8_t *fileName, uint32_t processId)
{
    uint32_t totalSize = 0;
    uint32_t fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    uint32_t pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);
    
    for (uint32_t i = 0; i < totalLines; i++)
    {
        totalSize += strlen(textBuffer[i]) + 1; // +1 for newline
    }
    uint32_t pageCount = ceiling(totalSize, PAGE_SIZE);
    uint8_t *tempFileName = (uint8_t *)"edtmp";
    systemOpenEmptyFile(tempFileName, pageCount);
    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    systemCloseFile(fileDescriptor);
    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    systemOpenFile(tempFileName, RDWRITE);
    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);
    
    uint8_t *outputBuffer = (uint8_t *)pointer;
    uint32_t bufferPosition = 0;
    for (uint32_t i = 0; i < totalLines; i++)
    {
        uint32_t lineLen = strlen(textBuffer[i]);
        bytecpy(outputBuffer + bufferPosition, textBuffer[i], lineLen);
        bufferPosition += lineLen;
        outputBuffer[bufferPosition++] = 0x0a;
    }
    outputBuffer[bufferPosition] = 0;
    systemCreateFile(fileName, fileDescriptor);
    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);


    systemCloseFile((uint32_t)fileDescriptor);
    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    systemDeleteFile(tempFileName);
    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);
    printString(COLOR_GREEN, STATUS_ROW, 0, (uint8_t *)"File saved");

    systemShowOpenFiles(40);
    processId = readValueFromMemLoc(RUNNING_PID_LOC);
    fileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    pointer = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);
}

// Waits for the user to press the Enter key, ignoring other key presses including shift modifiers.
// This is used to pause the screen, such as after displaying help, until the user acknowledges.
void waitForEnter()
{
    uint8_t shiftPressed = 0;
    while (true)
    {
        uint8_t tempScanCode;
        uint8_t scanCode;
        do
        {
            readKeyboard(&tempScanCode);
            if (tempScanCode == 0x2A || tempScanCode == 0x36)
            {
                shiftPressed = 1;
                continue;
            }
            if (tempScanCode == 0xAA || tempScanCode == 0xB6)
            {
                shiftPressed = 0;
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
    }
}

// Displays a help screen listing all available commands and their descriptions.
// Clears the screen, prints the help text, and waits for Enter before redisplaying the editor content.
void showHelp()
{
    clearScreen();
    printString(COLOR_GREEN, 0, 0, (uint8_t*)"Edlin Commands:");
    printString(COLOR_WHITE, 2, 0, (uint8_t*)"number: Jump to line number and display from there");
    printString(COLOR_WHITE, 3, 0, (uint8_t*)"e/E: Edit the current line");
    printString(COLOR_WHITE, 4, 0, (uint8_t*)"l/L: List text lines starting from display start");
    printString(COLOR_WHITE, 5, 0, (uint8_t*)"i/I: Insert new lines at current position (end with . on a new line)");
    printString(COLOR_WHITE, 6, 0, (uint8_t*)"d/D: Delete the current line");
    printString(COLOR_WHITE, 7, 0, (uint8_t*)"q/Q: Quit the editor");
    printString(COLOR_WHITE, 8, 0, (uint8_t*)"p/P: Switch to parent process");
    printString(COLOR_WHITE, 9, 0, (uint8_t*)"w <filename>: Write (save) to the specified file");
    //printString(COLOR_WHITE, 10, 0, (uint8_t*)"o/O <filename>: Open (load) the specified file");
    printString(COLOR_WHITE, 10, 0, (uint8_t*)"s/S: Save to the current file");
    printString(COLOR_WHITE, 11, 0, (uint8_t*)"h/H: Show this help");
    printString(COLOR_WHITE, 12, 0, (uint8_t*)"Page Up/Page Down: Scroll the display up/down");
    printString(COLOR_GREEN, 13, 0, (uint8_t*)"Press Enter to return...");
    waitForEnter();
    listTextLines(displayStartLineNumber);
    printCommandPrompt();
}

// The main entry point of the editor program.
// Initializes the text buffer, loads an initial file if provided via command line, and enters the main loop to process user commands.
// Handles commands like jumping to lines, editing, inserting, deleting, saving, loading, quitting, and showing help.
void main()
{
    uint32_t myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    fillMemory((uint8_t *)textBuffer, 0x0, MAX_LINES * (MAX_LINE_LENGTH + 1));

    uint8_t *initialFileName = (uint8_t *)malloc(myProcessId, MAX_LINE_LENGTH + 1);
    uint8_t *commandLine = (uint8_t *)COMMAND_BUFFER;
    uint8_t *argPointer = commandLine;
    while (*argPointer) argPointer++;
    argPointer++; // skip \0
    // Now pointer at filename
    if (*argPointer && *argPointer != 0x0a)
    {
        uint32_t argLength = 0;
        uint8_t *tempPtr = argPointer;
        while (*tempPtr != 0x0a && *tempPtr != 0) { argLength++; tempPtr++; }
        bytecpy(initialFileName, argPointer, argLength);
        initialFileName[argLength] = 0;

        // This is to handle the situation where the argument to edlin is a number 
        // as in the priority to run edlin. If that is the case, there is no filename supplied.
        if (isDigit(initialFileName[0]))
        {
            initialFileName[0] = 0;
        }

    }
    else
    {
        initialFileName[0] = 0;
    }

    clearScreen();

    if (initialFileName[0] != 0) {
        loadFileContent(initialFileName, myProcessId);
    }

    listTextLines(displayStartLineNumber);
    printCommandPrompt();

    uint8_t commandBuffer[MAX_LINE_LENGTH + 1];

    while (true)
    {
        systemShowOpenFiles(40);
        myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
        
        uint32_t inputLength = readInputLine(commandBuffer, PROMPT_ROW, getInputColumn(), 1);
        if (commandBuffer[0] >= '0' && commandBuffer[0] <= '9')
        {
            uint32_t lineNumber = atoi(commandBuffer);
            if (lineNumber > 0)
            {
                displayStartLineNumber = lineNumber;
                currentLineIndex = lineNumber - 1;
                listTextLines(displayStartLineNumber);
            }
        }
        else if ((commandBuffer[0] == 'e' || commandBuffer[0] == 'E') && commandBuffer[1] == 0)
        {
            editTextLine(currentLineIndex + 1);
            listTextLines(displayStartLineNumber);
        }
        else if ((commandBuffer[0] == 'l' || commandBuffer[0] == 'L') && commandBuffer[1] == 0)
        {
            listTextLines(displayStartLineNumber);
        }
        else if ((commandBuffer[0] == 'i' || commandBuffer[0] == 'I') && commandBuffer[1] == 0)
        {
            insertNewLines();
            listTextLines(displayStartLineNumber);
        }
        else if ((commandBuffer[0] == 'd' || commandBuffer[0] == 'D') && commandBuffer[1] == 0)
        {
            deleteTextLine(currentLineIndex + 1);
            listTextLines(displayStartLineNumber);
        }
        else if ((commandBuffer[0] == 'q' || commandBuffer[0] == 'Q') && commandBuffer[1] == 0)
        {
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
            systemExit(PROCESS_EXIT_CODE_SUCCESS);
        }
        else if ((commandBuffer[0] == 'p' || commandBuffer[0] == 'P') && commandBuffer[1] == 0)
        {
            fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
            systemSwitchToParent();
        }
        else if ((commandBuffer[0] == 'w' || commandBuffer[0] == 'W') && commandBuffer[1] == 0x20)
        {
            uint8_t *saveFileName = commandBuffer + 2;
            saveFileName[inputLength - 2] = 0; // Ensure null-terminated
            saveFileContent(saveFileName, myProcessId);
        }
        // else if ((commandBuffer[0] == 'o' || commandBuffer[0] == 'O') && commandBuffer[1] == 0x20)
        // {
        //     uint8_t *fileName = commandBuffer + 2;
        //     fileName[inputLength - 2] = 0; // Ensure null-terminated
        //     loadFileContent(fileName, myProcessId);
        //     uint32_t len = strlen(fileName);
        //     bytecpy(initialFileName, fileName, len);
        //     initialFileName[len] = 0;
        //     listTextLines(displayStartLineNumber);
        // }
        else if ((commandBuffer[0] == 's' || commandBuffer[0] == 'S') && commandBuffer[1] == 0)
        {
            if (initialFileName[0] != 0)
            {
                saveFileContent(initialFileName, myProcessId);
            }
            else
            {
                printString(COLOR_RED, STATUS_ROW, 0, (uint8_t *)"No file specified");
            }
        }
        else if ((commandBuffer[0] == 'h' || commandBuffer[0] == 'H') && commandBuffer[1] == 0)
        {
            showHelp();
        }
        else
        {
            printString(COLOR_RED, STATUS_ROW, 0, (uint8_t *)"Unknown command");
        }
        printCommandPrompt();
    }

    //free(myProcessId, (uint32_t)initialFileName);
    fillMemory((uint8_t*)KEYBOARD_BUFFER, 0x0, PAGE_SIZE);
    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}