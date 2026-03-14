// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "screen.h"
#include "fs.h"
#include "constants.h"
#include "exceptions.h"
#include "vm.h"
#include "libc-main.h"


void main()
{
    clearScreen();
    printString(COLOR_WHITE, 5, 5, (uint8_t *)"Welcome to CSCN-443!");
    printString(COLOR_LIGHT_BLUE, 7, 5, (uint8_t *)"Bootloader-Stage2");

    uint32_t numberToPrint = 0;
    uint32_t row = 11;
    uint32_t column = 0;

    while (numberToPrint <= 255)
    {
        if (column >= 75) { column = 0; row++; }

        printHexNumber(COLOR_RED, row, (column = column + 3), (uint8_t)numberToPrint);
        numberToPrint++;

    }

    while (true) {}

}
