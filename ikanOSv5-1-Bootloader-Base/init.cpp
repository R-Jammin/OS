// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "libc-main.h"
#include "constants.h"
#include "vm.h"
#include "x86.h"

void main()
{    

    while(true)
    {
    
        if (readValueFromMemLoc(RUNNING_PID_LOC) != 1)
        {
            // assume init has been invoked due to a fault
            uint8_t *pidToKillString = malloc(readValueFromMemLoc(RUNNING_PID_LOC), 20);
            itoa(readValueFromMemLoc(RUNNING_PID_LOC) -1, pidToKillString);
            systemKill(pidToKillString);
            free(pidToKillString);

            systemTaskSwitch((uint8_t*)"1");
        }
        else
        {
            systemExec((uint8_t*)"sh", 50, 0, 0, 0);
        }

    }
    
}