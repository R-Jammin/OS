// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.


#include "interrupts.h"
#include "syscalls.h"
#include "x86.h"
#include "constants.h"
#include "trap.h"


void enableInterrupts()
{
    asm volatile ("sti\n\t");
}

void disableInterrupts()
{
    asm volatile ("cli\n\t");
}

void remapPIC(uint8_t masterPICMask, uint8_t slavePICMask)
{
    // This will remap IRQs 0x0 -> 0xF to 0x20 -> 0x2F
    // This is due to a conflict with Intel CPU exceptions
    outputIOPort(MASTER_PIC_COMMAND_PORT, 0x11); // start initialization (cascade mode)
    outputIOPort(SLAVE_PIC_COMMAND_PORT, 0x11);// start initialization (cascade mode)
    outputIOPort(MASTER_PIC_DATA_PORT, 0x20); // offset value of new interrupts
    outputIOPort(SLAVE_PIC_DATA_PORT, 0x28); // offset values of new interrupts
    outputIOPort(MASTER_PIC_DATA_PORT, 0x04); // tell master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outputIOPort(SLAVE_PIC_DATA_PORT, 0x02); // tell slave PIC its cascade identity (0000 0010)
    outputIOPort(MASTER_PIC_DATA_PORT, 0x01); // 8086/88 (MCS-80/85) mode
    outputIOPort(SLAVE_PIC_DATA_PORT, 0x01); // 8086/88 (MCS-80/85) mode
    outputIOPort(MASTER_PIC_DATA_PORT, masterPICMask);
    outputIOPort(SLAVE_PIC_DATA_PORT, slavePICMask);

}

void loadIDT(uint8_t *idtMemory)
{  
    uint32_t x = 0;

    for (x=0; x < 13; x++)
    {
        *(uint16_t *)idtMemory = ((uint32_t)(&generalFault) & 0xffff); //lsbOffset ->trigger clear screen? 80986
        *(uint16_t *)(idtMemory + 2) = (uint8_t)0x8; //selector
        *(idtMemory + 4) = (uint8_t)0x00; //reserved
        *(idtMemory + 5) = (uint8_t)0xee; //attributes (was 0x8e)
        *(uint16_t *)(idtMemory + 6) = ((uint32_t)(&generalFault) >> 16);

        idtMemory = idtMemory + 8;
        
    }

    *(uint16_t *)idtMemory = ((uint32_t)(&generalProtectionFault) & 0xffff); //lsbOffset ->trigger clear screen? 80986
    *(uint16_t *)(idtMemory + 2) = (uint8_t)0x8; //selector
    *(idtMemory + 4) = (uint8_t)0x00; //reserved
    *(idtMemory + 5) = (uint8_t)0xee; //attributes (was 0x8e)
    *(uint16_t *)(idtMemory + 6) = ((uint32_t)(&generalProtectionFault) >> 16);

    idtMemory = idtMemory + 8;

    *(uint16_t *)idtMemory = ((uint32_t)(&pageFault) & 0xffff); //lsbOffset ->trigger clear screen? 80986
    *(uint16_t *)(idtMemory + 2) = (uint8_t)0x8; //selector
    *(idtMemory + 4) = (uint8_t)0x00; //reserved
    *(idtMemory + 5) = (uint8_t)0xee; //attributes (was 0x8e)
    *(uint16_t *)(idtMemory + 6) = ((uint32_t)(&pageFault) >> 16);

    idtMemory = idtMemory + 8;


    for (x=15; x < 128; x++)
    {
        *(uint16_t *)idtMemory = ((uint32_t)(&systemInterruptHandler) & 0xffff); //lsbOffset ->trigger clear screen? 80986
        *(uint16_t *)(idtMemory + 2) = (uint8_t)0x8; //selector
        *(idtMemory + 4) = (uint8_t)0x00; //reserved
        *(idtMemory + 5) = (uint8_t)0xee; //attributes (was 0x8e)
        *(uint16_t *)(idtMemory + 6) = ((uint32_t)(&systemInterruptHandler) >> 16);

        idtMemory = idtMemory + 8;
        
    }

    // This is the interrupt handler for uint32_t 0x80 (which is the 128th handler)
    *(uint16_t *)idtMemory = ((uint32_t)(&syscallHandler) & 0xffff); //lsbOffset ->trigger clear screen? 80986
    *(uint16_t *)(idtMemory + 2) = (uint8_t)0x8; //selector
    *(idtMemory + 4) = (uint8_t)0x00; //reserved
    *(idtMemory + 5) = (uint8_t)0xee; //attributes (was 0x8e)
    *(uint16_t *)(idtMemory + 6) = ((uint32_t)(&syscallHandler) >> 16);

    idtMemory = idtMemory + 8;


    for (x=129; x < 256; x++)
    {
        *(uint16_t *)idtMemory = ((uint32_t)(&generalFault) & 0xffff); //lsbOffset ->trigger clear screen? 80986
        *(uint16_t *)(idtMemory + 2) = (uint8_t)0x8; //selector
        *(idtMemory + 4) = (uint8_t)0x00; //reserved
        *(idtMemory + 5) = (uint8_t)0xee; //attributes (was 0x8e)
        *(uint16_t *)(idtMemory + 6) = ((uint32_t)(&generalFault) >> 16);

        idtMemory = idtMemory + 8;
    }

}

void loadIDTR(uint8_t *idtMemory, uint8_t *idtrMemory)
{
    *idtrMemory = (uint8_t)0x00; 
    *(idtrMemory + 1) = (uint8_t)0x8; //0x800 in length
    *(idtrMemory + 2) = (uint8_t)0x00; 
    *(idtrMemory + 3) = (uint32_t)idtMemory >> 8; 
    *(idtrMemory + 4) = (uint32_t)idtMemory >> 16; 

    asm volatile ("lidt (%0)\n\t" : : "r" (idtrMemory));

}

void setSystemTimer(uint32_t frequency)
{
    uint32_t divisor = PIT_FREQUENCY / frequency;
    outputIOPort(PIT_COMMAND_PORT, 0x36);
    outputIOPort(PIT_COUNTER_1, (uint8_t)(divisor & 0xFF));
    outputIOPort(PIT_COUNTER_1, (uint8_t)((divisor >> 8) & 0xFF));
}

void switchToRing3LaunchBinary(uint8_t *binaryEntryAddress)
{
    asm volatile ("movl $0x23, %eax\n\t"); //this is the ring 3 data segment (requesting DPL=3)
    asm volatile ("mov %ax, %ds\n\t");
    asm volatile ("mov %ax, %es\n\t");
    asm volatile ("mov %ax, %fs\n\t");
    asm volatile ("mov %ax, %gs\n\t");
    asm volatile ("movl %esp, %eax\n\t");
    asm volatile ("pushl $0x23\n\t");
    asm volatile ("pushl %0\n\t" : : "r" ((uint32_t)(STACK_START_LOC)));
    asm volatile ("pushf\n\t");
    asm volatile ("pushl $0x1b\n\t"); //this is the ring 3 code segment
    asm volatile ("movl %0, %%eax\n\t" : : "r" ((uint32_t)(binaryEntryAddress)));
    asm volatile ("pushl %eax\n\t");
    asm volatile ("sti\n\t");

    asm volatile ("iret\n\t");
}