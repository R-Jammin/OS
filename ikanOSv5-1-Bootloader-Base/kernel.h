// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"

/** The kernel configuration structure. This is used to hold start up configurations that can be referenced by functions to know if they should run or not at start up. It will be expanded in the future. */
struct kernelConfiguration
{
    uint32_t runScheduler;
};


struct kernelLog
{
    uint32_t interruptCount;
    uint32_t pid;
    uint8_t type[KERNEL_LOG_TYPE_SIZE];
    uint32_t value;
    uint8_t description[KERNEL_LOG_DESC_SIZE];
};

void kInit();

/** Prompts the user to enter the root password. This is normally commented out to save time for the student. The root password is "passw0rd". It compares the password entered to a simple hashed value in the mpass file. See stringHash(). */
void logonPrompt();
void loadShell();
void launchShell();
void secondProcInitialFunc();