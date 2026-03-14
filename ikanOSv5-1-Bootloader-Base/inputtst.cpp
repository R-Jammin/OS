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

    uint32_t row = 3;
    uint32_t column = 3;

    uint32_t testInt;
    uint32_t testHex;
    uint8_t testString[50];
    uint8_t testChar;

    printf(COLOR_GREEN, row++, column, (uint8_t*)"We are testing printf and scanf.");
    row++;
    printf(COLOR_LIGHT_BLUE, row++, column, (uint8_t*)"Enter 4 values, separated by spaces.");
    printf(COLOR_LIGHT_BLUE, row++, column, (uint8_t*)"(1) a decimal number, (2) a hexadecimal number (0x0),");
    printf(COLOR_LIGHT_BLUE, row++, column, (uint8_t*)"(3) a string (we use spaces to find the end), (4) a char.");

    row+= 2;
    printf(COLOR_WHITE, row++, column, (uint8_t*)"Enter an integer, hex, a string, and a char");
    row++;
    printf(COLOR_WHITE, row++, column, (uint8_t*)"(e.g., 10 0x7f Hello z)");
    row++;

    printf(COLOR_GREEN, row, column, (uint8_t*)"Test scanf> ");
    uint32_t assignments = scanf(COLOR_WHITE, row, 15, (uint8_t*)"%d %x %s %c", &testInt, &testHex, testString, &testChar);

    row+= 3;
    printf(COLOR_LIGHT_BLUE, row++, column, (uint8_t*)"Total Inputs: %d\n", assignments);
    printf(COLOR_LIGHT_BLUE, row++, column, (uint8_t*)"Integer: %d\n", testInt);
    printf(COLOR_LIGHT_BLUE, row++, column, (uint8_t*)"Hex: 0x%x\n", testHex);
    printf(COLOR_LIGHT_BLUE, row++, column, (uint8_t*)"String: %s\n", testString);
    printf(COLOR_LIGHT_BLUE, row++, column, (uint8_t*)"Char: %c\n", testChar);

    row+= 2;
    printf(COLOR_WHITE, row++, column, (uint8_t*)"Press Enter to quit.");

    waitForEnterOrQuit();
    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}