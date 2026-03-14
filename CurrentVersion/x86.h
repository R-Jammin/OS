// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"
/** Send a byte to an I/O port
 * \param port The port number.
 * \param data The byte you want to send.
 */
void outputIOPort(uint16_t port, uint8_t data);

/** Reads a byte from an I/O port. Returns the byte.
 * \param port The port number you want to read.
 */
uint8_t inputIOPort(uint16_t port);

/** Allows you to read and transfer multiple words from a port to a memory location.
 * \param port The port number to read.
 * \param destinationMemory The memory address you want to store the words from the I/O port.
 * \param numberOfWords The number of words you want to read from the I/O port.
 */
void ioPortWordToMem(uint16_t port, uint8_t *destinationMemory, uint32_t numberOfWords);

/** Sends words of memory to an I/O port.
 * \param destinationPort The port number to send information to.
 * \param sourceMemory The memory address of the source to send to the I/O port.
 * \param numberOfWords The number of words you want to send to the I/O port. 
 */
void memToIoPortWord(uint16_t destinationPort, uint8_t *sourceMemory, uint32_t numberOfWords);

/** Copies memory from one location to another, in 16-bit words.
 * \param startingMemory The pointer to the beginning pouint32_t to copy.
 * \param destinationMemory The destination pointer that words will be copied to.
 * \param numberOfWords The number of 16-bit words to copy from source to destination.
 */
void memoryCopy(uint8_t *startingMemory, uint8_t *destinationMemory, uint32_t numberOfWords);

/** Stores a 32-bit value to a memory location.
 * \param destinationMemory The target destination.
 * \param value The 32-bit value you want to store.
 */
void storeValueAtMemLoc(uint8_t *destinationMemory, uint32_t value);

/** Reads and returns a 32-bit value from a memory location.
 * \param sourceMemory The memory address to read.
 */
uint32_t readValueFromMemLoc(uint8_t *sourceMemory);

/** The primary sysCall function. This is used by LibC function to trigger system calls from ring 3.
 * \param sysCallNumber The system call number.
 * \param arg1 An argument to pass to the kernel, if necessary. This may be an blank, an integer, a memory location, or a pointer to a certain type of structure, depending on the system call.
 * \param currentPid The pid requesting the system call.
 */
uint32_t sysCall(uint32_t sysCallNumber, uint32_t arg1, uint32_t currentPid);

/** Loads the Intel task register. This is used during kernel initialization just prior to the jump to shell in ring 3.
 * \param taskRegisterValue Used to load the tss_kernel_descriptor from bootloader-stage 1.
 */
void loadTaskRegister(uint16_t taskRegisterValue);
void startApplicationProcessor();