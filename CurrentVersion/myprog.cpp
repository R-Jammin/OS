// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "libc-main.h"
#include "constants.h"
#include "x86.h"
#include "screen.h"
#include "keyboard.h"

void main()
{   
    uint32_t myPid = readValueFromMemLoc(RUNNING_PID_LOC); 

    uint32_t initialValue = 0;
    uint32_t *valueToCalculate = (uint32_t *)malloc(myPid, sizeof(int));

    while ((uint32_t)*valueToCalculate < 40000000)
    {
        *valueToCalculate = initialValue++;
    }

    free((uint8_t *)valueToCalculate);
    systemSwitchToParent();

    main();

}