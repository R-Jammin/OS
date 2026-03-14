// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "constants.h"
#include "x86.h"

// https://wiki.osdev.org/PC_Speaker

void generateTone(uint32_t frequency)
{
    uint32_t valueToSend;

    // 1.193180 MHz is the clock speed of the timer
    valueToSend = 1193180 / frequency;
    
    outputIOPort(PIT_COMMAND_PORT, SOUND_MODE_3_SQUARE_WAVE);
    outputIOPort(PIT_COUNTER_3, (uint8_t)valueToSend ); // Send the the low byte first
    outputIOPort(PIT_COUNTER_3, (uint8_t)valueToSend >> 8); // Send the next highest byte next
 	outputIOPort(KEYBOARD_PORT_B, inputIOPort(KEYBOARD_PORT_B) | 0x3); // In binary, "11" so PB0=1 and PB1=1 
}

void stopTone()
{
 	outputIOPort(KEYBOARD_PORT_B, inputIOPort(KEYBOARD_PORT_B) & 0xFC); // Mask to zero out low bits (so PB0=0 and PB1=0)
}
