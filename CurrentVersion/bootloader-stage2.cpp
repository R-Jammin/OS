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
    fillMemory((uint8_t *)KERNEL_SEMAPHORE_TABLE, 0x0, PAGE_SIZE);
    fillMemory((uint8_t *)GLOBAL_OBJECT_TABLE, 0x0, PAGE_SIZE);

    // Load the superblock and block group descriptor table
    fillMemory(SUPERBLOCK_LOC, 0x0, PAGE_SIZE);
    readBlock(SUPERBLOCK, SUPERBLOCK_LOC, false);
    readBlock(GROUP_DESCRIPTOR_BLOCK, BLOCK_GROUP_DESCRIPTOR_TABLE, false);

    uint32_t cursorRow = 0;

    if (!fsFindFile((uint8_t *)"vgafont.bin", KERNEL_TEMP_INODE_LOC, false, ROOTDIR_INODE))
    {
        panic((uint8_t *)"Bootloader-stage2.cpp -> Cannot find vga font in root directory");
    }
    loadFileFromInodeStruct(KERNEL_TEMP_INODE_LOC, (uint8_t*)VGA_CUSTOM_FONT, false);

    switchTo80x50Mode((uint8_t*)(VGA_CUSTOM_FONT));

    clearScreen();
    printString(COLOR_WHITE, cursorRow++, 0, (uint8_t *)"Bootloader stage 2...");
    printString(COLOR_WHITE, cursorRow++, 0, (uint8_t *)"Reading kernel binary from file system...");

    if (!fsFindFile((uint8_t *)"kernel", KERNEL_TEMP_INODE_LOC, false, ROOTDIR_INODE))
    {
        panic((uint8_t *)"Bootloader-stage2.cpp -> Cannot find kernel in root directory");
    }
    loadFileFromInodeStruct(KERNEL_TEMP_INODE_LOC, KERNEL_TEMP_FILE_LOC, false);
    loadElfFile(KERNEL_TEMP_FILE_LOC);

    printString(COLOR_WHITE, cursorRow++, 0, (uint8_t *)"Kernel binary loaded...jumping to kernel main...");

    struct elfHeader *ELFHeader = (struct elfHeader*)KERNEL_TEMP_FILE_LOC;

    (*(void(*)())ELFHeader->e_entry)();

    panic((uint8_t *)"bootloader-stage2.cpp -> Error launching kernel binary");

}
