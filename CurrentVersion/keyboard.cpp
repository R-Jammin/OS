// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Original version by Dan O'Malley
// Extended with extra keys with Grok.
// 12/2025 with Grok v4.


#include "keyboard.h"
#include "constants.h"
#include "x86.h"
#include "screen.h"


void readKeyboard(uint8_t *ioMemory)
{
    uint8_t scanCode;

    //status check
    while ((inputIOPort(KEYBOARD_STATUS_PORT) & 0x1) == 0) {}

    scanCode = inputIOPort(KEYBOARD_DATA_PORT);
    *ioMemory = ((uint8_t)scanCode);
}

void readCommand(uint8_t *ioMemory, uint8_t *vgaMemory)
{
    uint32_t cursorPosition = 4;
    uint8_t shift_down = 0;

    while ((uint8_t)(*(ioMemory -1)) != (uint8_t)0x1C) // Enter make code (was 0x9c for break)
    {
        *vgaMemory = 0x0;    // need a seed value
        *(vgaMemory +1) = 15;

        enableCursor();
        moveCursor(44, cursorPosition);

        *(vgaMemory + 2) = 0x0;
        *(vgaMemory + 3) = 0x0;

        uint8_t temp_scan;
        do {
            readKeyboard(&temp_scan);
            if (temp_scan == 0x2A || temp_scan == 0x36) {
                shift_down = 1;
                continue;
            }
            if (temp_scan == 0xAA || temp_scan == 0xB6) {
                shift_down = 0;
                continue;
            }
            if (temp_scan & 0x80) {  // Skip break codes (was !(temp_scan & 0x80))
                continue;
            }
            *ioMemory = temp_scan;
            break;
        } while (1);

        // Update all mappings to use make codes (subtract 0x80 from original break code values)
        if ((uint8_t)(*ioMemory) == (uint8_t)0x10) {
            *vgaMemory = shift_down ? 0x51 : 0x71;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x51 : 0x71;
            } //q Q
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x11) {
            *vgaMemory = shift_down ? 0x57 : 0x77;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x57 : 0x77;
            } //w W
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x12) {
            *vgaMemory = shift_down ? 0x45 : 0x65;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x45 : 0x65;
            } //e E
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x13) {
            *vgaMemory = shift_down ? 0x52 : 0x72;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x52 : 0x72;
            } //r R
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x14) {
            *vgaMemory = shift_down ? 0x54 : 0x74;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x54 : 0x74;
            } //t T
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x15) {
            *vgaMemory = shift_down ? 0x59 : 0x79;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x59 : 0x79;
            } //y Y
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x16) {
            *vgaMemory = shift_down ? 0x55 : 0x75;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x55 : 0x75;
            } //u U
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x17) {
            *vgaMemory = shift_down ? 0x49 : 0x69;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x49 : 0x69;
            } //i I
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x18) {
            *vgaMemory = shift_down ? 0x4f : 0x6f;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x4f : 0x6f;
            } //o O
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x19) {
            *vgaMemory = shift_down ? 0x50 : 0x70;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x50 : 0x70;
            } //p P
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x1E) {
            *vgaMemory = shift_down ? 0x41 : 0x61;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x41 : 0x61;
            } //a A
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x1F) {
            *vgaMemory = shift_down ? 0x53 : 0x73;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x53 : 0x73;
            } //s S
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x20) {
            *vgaMemory = shift_down ? 0x44 : 0x64;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x44 : 0x64;
            } //d D
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x21) {
            *vgaMemory = shift_down ? 0x46 : 0x66;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x46 : 0x66;
            } //f F
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x22) {
            *vgaMemory = shift_down ? 0x47 : 0x67;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x47 : 0x67;
            } //g G
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x23) {
            *vgaMemory = shift_down ? 0x48 : 0x68;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x48 : 0x68;
            } //h H
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x24) {
            *vgaMemory = shift_down ? 0x4a : 0x6a;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x4a : 0x6a;
            } //j J
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x25) {
            *vgaMemory = shift_down ? 0x4b : 0x6b;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x4b : 0x6b;
            } //k K
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x26) {
            *vgaMemory = shift_down ? 0x4c : 0x6c;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x4c : 0x6c;
            } //l L
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x2C) {
            *vgaMemory = shift_down ? 0x5a : 0x7a;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x5a : 0x7a;
            } //z Z
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x2D) {
            *vgaMemory = shift_down ? 0x58 : 0x78;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x58 : 0x78;
            } //x X
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x2E) {
            *vgaMemory = shift_down ? 0x43 : 0x63;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x43 : 0x63;
            } //c C
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x2F) {
            *vgaMemory = shift_down ? 0x56 : 0x76;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x56 : 0x76;
            } //v V
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x30) {
            *vgaMemory = shift_down ? 0x42 : 0x62;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x42 : 0x62;
            } //b B
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x31) {
            *vgaMemory = shift_down ? 0x4e : 0x6e;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x4e : 0x6e;
            } //n N
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x32) {
            *vgaMemory = shift_down ? 0x4d : 0x6d;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x4d : 0x6d;
            } //m M
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x02) {
            *vgaMemory = shift_down ? 0x21 : 0x31;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x21 : 0x31;
            } //1 !
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x03) {
            *vgaMemory = shift_down ? 0x40 : 0x32;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x40 : 0x32;
            } //2 @
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x04) {
            *vgaMemory = shift_down ? 0x23 : 0x33;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x23 : 0x33;
            } //3 #
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x05) {
            *vgaMemory = shift_down ? 0x24 : 0x34;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x24 : 0x34;
            } //4 $
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x06) {
            *vgaMemory = shift_down ? 0x25 : 0x35;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x25 : 0x35;
            } //5 %
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x07) {
            *vgaMemory = shift_down ? 0x5E : 0x36;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x5E : 0x36;
            } //6 ^
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x08) {
            *vgaMemory = shift_down ? 0x26 : 0x37;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x26 : 0x37;
            } //7 &
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x09) {
            *vgaMemory = shift_down ? 0x2A : 0x38;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x2A : 0x38;
            } //8 *
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x0A) {
            *vgaMemory = shift_down ? 0x28 : 0x39;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x28 : 0x39;
            } //9 (
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x0B) {
            *vgaMemory = shift_down ? 0x29 : 0x30;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x29 : 0x30;
            } //0 )
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x0C) {
            *vgaMemory = shift_down ? 0x5F : 0x2D;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x5F : 0x2D;
            } // - _
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x0D) {
            *vgaMemory = shift_down ? 0x2B : 0x3D;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x2B : 0x3D;
            } // = +
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x1A) {
            *vgaMemory = shift_down ? 0x7B : 0x5B;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x7B : 0x5B;
            } // [ {
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x1B) {
            *vgaMemory = shift_down ? 0x7D : 0x5D;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x7D : 0x5D;
            } // ] }
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x2B) {
            *vgaMemory = shift_down ? 0x7C : 0x5C;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x7C : 0x5C;
            } // \ |
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x33) {
            *vgaMemory = shift_down ? 0x3C : 0x2C;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x3C : 0x2C;
            } // , <
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x34) {
            *vgaMemory = shift_down ? 0x3E : 0x2e;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x3E : 0x2e;
        } // . >
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x35) {
            *vgaMemory = shift_down ? 0x3F : 0x2f;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x3F : 0x2f;
        } // / ?
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x27) {
            *vgaMemory = shift_down ? 0x3A : 0x3b;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x3A : 0x3b;
        } // ; :
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x28) {
            *vgaMemory = shift_down ? 0x22 : 0x27;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = shift_down ? 0x22 : 0x27;
        } // ' "
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x39) {  // Space key make code (was 0xB9 for break)
            *vgaMemory = 0x20;
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = 0x20;
        }
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x1C) {
            *(ioMemory + KEYBOARD_BUFFER_SIZE) = 0x0a;
            } // enter
        
        else if ((uint8_t)(*ioMemory) == (uint8_t)0x0E) //backspace make code (was 0x8e for break)
        {
            if (ioMemory == (uint8_t *)KEYBOARD_BUFFER)
            {
                // Do nothing, we are at the beginning of the buffer
            }
            else
            {
                *(vgaMemory -1) = 0x0;
                *(vgaMemory -2) = 0x0;
                vgaMemory = vgaMemory - 4;

                *(ioMemory -1) = 0x0;
                *(ioMemory -2) = 0x0;
                ioMemory = ioMemory - 2;
                cursorPosition = cursorPosition-2;
            }

        }

        vgaMemory++;
        *vgaMemory = COLOR_WHITE;
        vgaMemory++;
        ioMemory++;
        cursorPosition++;

    }
}