// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "screen.h"
#include "constants.h"
#include "vm.h"
#include "libc-main.h"

void panic(uint8_t *message)
{
    clearScreen();
    disableCursor();

    for (uint32_t x=1; x<4000; x++)
    {
        fillMemory((uint8_t*)(VIDEO_RAM + x), 0x10, 1);
        x++;
    }
    printString(0x1F, 1, 8, (uint8_t *)"AWESOME WORK! You triggered a kernel panic (that takes skill)!");
    printString(0x1F, 3, 9, (uint8_t *)"O'Malley would be proud. (He has done it hundreds of times...)");
    
    printString(0x1C, 5, 2, (uint8_t *)"Kernel panic!");
    printString(0x1F, 7, 2, (uint8_t *)"From: ");
    printString(0x1F, 7, 10, message);

    while (true) {}
}