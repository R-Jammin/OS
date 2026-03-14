// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Most functions written by Dan O'Malley, The ones generated 
// with Grok by xAI are noted below.
// 12/2025 with Grok v4.


#include "screen.h"
#include "x86.h"
#include "constants.h"
#include "libc-main.h"


// Function to enable the text mode cursor.
// This sets the cursor to be visible by configuring the start and end scan lines
// via I/O ports for the VGA controller. The cursor will appear as a block from
// scan line 0 to 7.
void enableCursor()
{
    // Dan O'Malley
    
    outputIOPort(0x3D4, 0x0A);
    outputIOPort(0x3D5, (inputIOPort(0x3D5) & 0xC0) | 0x0); // Set cursor start scan line

    outputIOPort(0x3D4, 0x0B);
    outputIOPort(0x3D5, (inputIOPort(0x3D5) & 0xE0) | 0x7); // Set cursor end scan line
}

// Function to disable the text mode cursor.
// This hides the cursor by setting the cursor start to a value that effectively
// disables it via I/O ports for the VGA controller.
void disableCursor()
{
    // Dan O'Malley
    
    outputIOPort(0x3D4, 0x0A);
    outputIOPort(0x3D5, 0x20);
}

// Function to move the text mode cursor to a specific row and column.
// This calculates the cursor position in the 80-column text mode and updates
// the VGA cursor position registers accordingly.
// Parameters:
// - targetRow: The row (line) to move the cursor to (0-based).
// - targetColumn: The column to move the cursor to (0-based).
void moveCursor(uint32_t targetRow, uint32_t targetColumn)
{
    // Dan O'Malley
    
    uint16_t cursorPosition = targetRow * 80 + targetColumn;

    outputIOPort(0x3D4, 0x0F);
    outputIOPort(0x3D5, (uint8_t)((uint16_t)cursorPosition & 0xFF));
    outputIOPort(0x3D4, 0x0E);
    outputIOPort(0x3D5, (uint8_t)(((uint16_t)cursorPosition >> 8) & 0xFF));
}

// Function to switch the VGA display to 80x50 text mode, optionally loading a custom font.
// This function configures VGA registers to achieve 80 columns by 50 rows text mode.
// If font data is provided, it loads the 8x8 font into VGA memory. It temporarily
// turns off the screen to prevent flickering during mode switch.
// Parameters:
// - fontData: Pointer to the font data array (256 glyphs, 8 bytes each). If NULL, font loading is skipped.
void switchTo80x50Mode(uint8_t* fontData)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    // Font load (if data provided)
    //if (fontData) {
        // Save relevant registers (to restore later)
        outputIOPort(0x3C4, 0x02);
        uint8_t savedSeqMapMask = inputIOPort(0x3C5);
        outputIOPort(0x3C4, 0x04);
        uint8_t savedSeqMemoryMode = inputIOPort(0x3C5);
        outputIOPort(0x3CE, 0x05);
        uint8_t savedGraphicsMode = inputIOPort(0x3CF);
        outputIOPort(0x3CE, 0x06);
        uint8_t savedGraphicsMisc = inputIOPort(0x3CF);

        // Configure for font write: Write mode 0, map to 0xA0000, select plane 2, disable chain4/odd-even
        outputIOPort(0x3CE, 0x05);
        outputIOPort(0x3CF, 0x00);  // GC 5: Write mode 0, read mode 0
        outputIOPort(0x3CE, 0x06);
        outputIOPort(0x3CF, 0x04);  // GC 6: Map to 0xA0000-BFFFF, graphics mode
        outputIOPort(0x3C4, 0x02);
        outputIOPort(0x3C5, 0x04);  // SEQ 2: Map mask plane 2
        outputIOPort(0x3C4, 0x04);
        outputIOPort(0x3C5, 0x06);  // SEQ 4: Extended memory, sequential addressing

        // Copy font data: 256 glyphs, 8 bytes each, into 32-byte slots at 0xA0000
        volatile uint8_t* fontMemoryBase = (volatile uint8_t*)0xA0000;
        for (uint32_t glyphIndex = 0; glyphIndex < 256; ++glyphIndex) {
            for (uint32_t fontRow = 0; fontRow < 8; ++fontRow) {
                fontMemoryBase[glyphIndex * 32 + fontRow] = fontData[glyphIndex * 8 + fontRow];
            }
            // Zero the remaining 24 bytes per slot (optional but cleans up)
            for (uint32_t fontRow = 8; fontRow < 32; ++fontRow) {
                fontMemoryBase[glyphIndex * 32 + fontRow] = 0x00;
            }
        }

        // Restore registers to standard text mode
        outputIOPort(0x3C4, 0x02);
        outputIOPort(0x3C5, savedSeqMapMask);
        outputIOPort(0x3C4, 0x04);
        outputIOPort(0x3C5, savedSeqMemoryMode);
        outputIOPort(0x3CE, 0x05);
        outputIOPort(0x3CF, savedGraphicsMode);
        outputIOPort(0x3CE, 0x06);
        outputIOPort(0x3CF, savedGraphicsMisc);
    //}

    // Turn off screen to avoid flicker
    outputIOPort(0x3C4, 0x01);
    uint8_t clockingModeValue = inputIOPort(0x3C5) | 0x20;
    outputIOPort(0x3C5, clockingModeValue);

    // CRTC Reg 0x09: Max Scan Line = 7 (8 scanlines/char -1; preserve other bits)
    outputIOPort(0x3D4, 0x09);
    outputIOPort(0x3D5, 0x47);

    // Adjust cursor to full block
    outputIOPort(0x3D4, 0x0A);
    outputIOPort(0x3D5, 0x00);
    outputIOPort(0x3D4, 0x0B);
    outputIOPort(0x3D5, 0x07);

    // Turn on screen
    outputIOPort(0x3C4, 0x01);
    clockingModeValue = inputIOPort(0x3C5) & ~0x20;
    outputIOPort(0x3C5, clockingModeValue);
}