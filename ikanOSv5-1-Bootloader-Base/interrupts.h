// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"

/** The interrupt descriptor structure. This is the layout of the entries in the interrupt descriptor table. */
struct interruptDescriptor {
    uint16_t lsbOffset;
    uint16_t selector;
    uint8_t zero;
    uint8_t attributes;
    uint16_t msbOffset;
};

/** Remaps the programmable interrupt controller (PIC) due to an Intel flaw. See: https://wiki.osdev.org/8259_PIC
 * \param masterPICMask The mask to use to enable/disable certain interrupts for master PIC.
 * \param slavePICMask The mask to use to enable/disable certain interrupts for slave PIC.
 */
void remapPIC(uint8_t masterPICMask, uint8_t slavePICMask);

/** Populates the interrupt descriptor table given a starting location.
 * \param idtMemory The pointer to the start of the interrupt descriptor table.
*/
void loadIDT(uint8_t *idtMemory);

/** Creates the Intel-prescribed format of the entry to be loaded into the interrupt descriptor table register.
 * \param idtMemory The pointer to the start of the interrupt descriptor table.
 * \param idtrMemory The pointer where the interrupt descriptor table register entry should be stored. 
 */
void loadIDTR(uint8_t *idtMemory, uint8_t *idtrMemory);

/** Sets the system timer to determine the number of interrupts per second.
 * \param frequency The number of interrupts per second. Determined by constant: SYSTEM_INTERRUPTS_PER_SECOND
 */
void setSystemTimer(uint32_t frequency);

/** Turns on system interrupts. */
void enableInterrupts();

/** Turns off system interrupts. This is usually done to avoid race conditions or corruption during critical operations. */
void disableInterrupts();

/** There are several ways to get the Intel processor to move from Ring 0 to Ring 3. I am using an interrupt return method here. See: https://wiki.osdev.org/Getting_to_Ring_3
 * \param binaryEntryAddress The ELF entry address of the binary you want to run as Ring 3.
 */
void switchToRing3LaunchBinary(uint8_t *binaryEntryAddress);