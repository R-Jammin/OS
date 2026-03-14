// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "libc-main.h"

void main()
{   
    for (int x= 0; x < 10; x++)
    {
        systemShowProcesses();
        wait(1);
    }

    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}