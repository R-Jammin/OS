// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"

/**
 * Enables the text mode cursor.
 * This sets the cursor to be visible by configuring the start and end scan lines
 * via I/O ports for the VGA controller. The cursor will appear as a block from
 * scan line 0 to 7.
 */
void enableCursor();

/**
 * Disables the text mode cursor.
 * This hides the cursor by setting the cursor start to a value that effectively
 * disables it via I/O ports for the VGA controller.
 */
void disableCursor();

/**
 * Moves the text mode cursor to a specific row and column.
 * This calculates the cursor position in the 80-column text mode and updates
 * the VGA cursor position registers accordingly.
 * \param row The row (line) to move the cursor to (0-based).
 * \param column The column to move the cursor to (0-based).
 */
void moveCursor(uint32_t row, uint32_t column);

/**
 * Switches the VGA display to 80x50 text mode, optionally loading a custom font.
 * This function configures VGA registers to achieve 80 columns by 50 rows text mode.
 * If font data is provided, it loads the 8x8 font into VGA memory. It temporarily
 * turns off the screen to prevent flickering during mode switch.
 * \param fontData Pointer to the font data array (256 glyphs, 8 bytes each). If NULL, font loading is skipped.
 */
void switchTo80x50Mode(uint8_t* fontData);