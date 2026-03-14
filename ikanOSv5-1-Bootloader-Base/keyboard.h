// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"
/** Reads a scan code from the keyboard device and places it at the location parameter.
 * \param ioMemory The pointer where you'd like the scan code written.
 */
void readKeyboard(uint8_t *ioMemory);

/** This is the main keyboard routine and converts scan code to ASCII value and prints it to the screen.
 * \param ioMemory The pointer where the scan code is located.
 * \param vgaMemory The pointer to the portion of the screen where you want the character printed.
 */
void readCommand(uint8_t *ioMemory, uint8_t *vgaMemory);