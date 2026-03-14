// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// This is a combination of work by Dan O'Malley and Grok.
// 12/2025 with Grok v4.


#include "libc-main.h"
#include "constants.h"
#include "vm.h"
#include "x86.h"
#include "sound.h"
#include "file.h"
#include "screen.h"
#include "net.h"
#include "keyboard.h"

/**
 * Delays execution for a specified number of seconds by making system calls to wait.
 * Each system call waits for one second, so this loops for the given time.
 * @param timeToWait The number of seconds to wait.
 */
void wait(uint32_t timeToWait)
{
    // Dan O'Malley
    
    // Retrieve the current process ID from memory.
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    // Loop for each second to wait.
    for (uint32_t secondsCounter = 0; secondsCounter < timeToWait; secondsCounter++)
    {
        // Make a system call to wait for one second.
        sysCall(SYS_WAIT, 0x0, currentPid);
    }
}

/**
 * Compares two null-terminated strings lexicographically.
 * Returns a negative value if firstString < secondString,
 * zero if equal, positive if firstString > secondString.
  * @param firstString The first string to compare.
 * @param secondString The second string to compare.
 * @return The comparison result.
 */
uint32_t strcmp(uint8_t *firstString, uint8_t *secondString)
{
    // Written by Grok.
    // 12/2025 with Grok v4.

    while (*firstString && *firstString == *secondString) 
    {
        firstString++;
        secondString++;
    }
    return *firstString - *secondString;

}

/**
 * Computes the length of a null-terminated string.
 * @param targetString The string to measure.
 * @return The length of the string excluding the null terminator.
 */
uint32_t strlen(uint8_t *targetString)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t length = 0;
    while (*targetString != 0) 
    {
        targetString++;
        length++;
    }
    return length;
}

/**
 * Copies a null-terminated string from source to destination.
 * @param destinationString The buffer to copy into.
 * @param sourceString The string to copy from.
 */
void strcpy(uint8_t *destinationString, uint8_t *sourceString)
{
    // Written by Grok.
    // 12/2025 with Grok v4.

    while (*sourceString) 
    {
        *destinationString++ = *sourceString++;
    }
    *destinationString = 0;
}

/**
 * Copies a string from source to destination, stopping at newline (0xA).
 * Based on strcpy, modified to remove newline.
 * @param destinationString The buffer to copy into.
 * @param sourceString The string to copy from.
 */
void strcpyRemoveNewline(uint8_t *destinationString, uint8_t *sourceString)
{
    // Written by Grok. 
    // 12/2025 with Grok v4.
    
    while (*sourceString && *sourceString != '\n') 
    {
        *destinationString++ = *sourceString++;
    }
    *destinationString = 0;
}

/**
 * Copies a specified number of bytes from source to destination memory.
 * @param destinationMemory The target memory location.
 * @param sourceMemory The source memory location.
 * @param numberOfBytes The number of bytes to copy.
 */
void bytecpy(uint8_t *destinationMemory, uint8_t *sourceMemory, uint32_t numberOfBytes)
{
    // Dan O'Malley
    
    // Loop through each byte.
    for (uint32_t byteIndex = 0; byteIndex < numberOfBytes; byteIndex++)
    {
        *destinationMemory = *sourceMemory;
        destinationMemory++;
        sourceMemory++;
    }
}

/**
 * Reverses a null-terminated string in place.
 * @param targetString The string to reverse.
 */
void reverseString(uint8_t *targetString)
{
    // Written by Grok
    // 12/2025 with Grok v4.

    uint8_t *start = targetString;
    uint8_t *end = targetString;
    while (*end) 
    {
        end++;
    }
    end--;
    while (start < end) 
    {
        uint8_t temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}

/**
 * Computes number raised to the power of exponent.
 * @param number The base number.
 * @param exponent The exponent.
 * @return The result of number^exponent.
 */
uint32_t power(uint32_t number, uint32_t exponent)
{
    // Written by Grok.
    // 12/2025 with Grok v4.

    uint32_t result = 1;
    while (exponent > 0) 
    {
        if (exponent % 2 == 1) 
        {
            result *= number;
        }
        number *= number;
        exponent /= 2;
    }
    return result;

}

/**
 * Converts an integer to its ASCII string representation.
 * Handles negative numbers and stores result in destination.
 * @param number The integer to convert.
 * @param destinationMemory The buffer to store the string.
 */
void itoa(uint32_t number, uint8_t *destinationMemory)
{
    // Written by Grok.
    // 12/2025 with Grok v4.

    if (number == 0) 
    {
        destinationMemory[0] = '0';
        destinationMemory[1] = 0;
        return;
    }

    int negative = (number & 0x80000000) != 0;
    uint32_t abs_num = number;
    if (negative) 
    {
        abs_num = (~abs_num) + 1;
    }

    uint8_t buffer[11]; // Enough for -2147483648
    int i = 0;
    while (abs_num > 0) 
    {
        buffer[i++] = (abs_num % 10) + '0';
        abs_num /= 10;
    }
    if (negative) 
    {
        buffer[i++] = '-';
    }

    // Reverse into destination
    int j = 0;
    while (i > 0) 
    {
        destinationMemory[j++] = buffer[--i];
    }
    destinationMemory[j] = 0;

}

/**
 * Converts an unsigned integer to hexadecimal string.
 * @param number The number to convert.
 * @param destinationMemory The buffer for the hex string.
 */
void itoaHex(uint32_t number, uint8_t *destinationMemory)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    // Index for buffer.
    uint32_t index = 0;

    // Special case for zero.
    if (number == 0)
    {
        destinationMemory[index++] = '0';
    }
    else
    {
        // Extract digits.
        while (number > 0)
        {
            uint32_t remainder = number % 16;
            destinationMemory[index++] = (remainder < 10) ? (remainder + '0') : (remainder + 'A' - 10);
            number /= 16;
        }
    }

    // Null terminate.
    destinationMemory[index] = '\0';
    // Reverse to correct order.
    reverseString(destinationMemory);
}

/**
 * Converts a hexadecimal string to an unsigned integer.
 * Handles optional '0x' prefix.
 * @param hexString The hex string to convert.
 * @return The integer value.
 */
uint32_t hextoi(uint8_t *hexString)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    // Starting index and result.
    uint32_t index = 0;
    uint32_t result = 0;

    // Skip '0x' if present.
    if (hexString[0] == '0' && (hexString[1] == 'x' || hexString[1] == 'X'))
    {
        index = 2;
    }

    // Process each character.
    while (hexString[index] != '\0')
    {
        uint8_t currentChar = hexString[index];
        uint32_t digitValue;

        // Convert char to digit.
        if (currentChar >= '0' && currentChar <= '9')
        {
            digitValue = currentChar - '0';
        }
        else if (currentChar >= 'A' && currentChar <= 'F')
        {
            digitValue = currentChar - 'A' + 10;
        }
        else if (currentChar >= 'a' && currentChar <= 'f')
        {
            digitValue = currentChar - 'a' + 10;
        }
        else
        {
            break; // Invalid char.
        }

        // Accumulate value.
        result = result * 16 + digitValue;
        index++;
    }

    return result;
}

/**
 * Converts a 32-bit IPv4 address to dotted decimal string.
 * @param ipAddress The integer IP address.
 * @param destinationMemory Buffer for the string (at least 16 bytes).
 */
void itoIPAddressString(uint32_t ipAddress, uint8_t *destinationMemory)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    // Extract octets.
    uint32_t octet1 = (ipAddress >> 24) & 0xFF;
    uint32_t octet2 = (ipAddress >> 16) & 0xFF;
    uint32_t octet3 = (ipAddress >> 8) & 0xFF;
    uint32_t octet4 = ipAddress & 0xFF;

    // Buffers for octet strings.
    uint8_t octetStr1[4];
    uint8_t octetStr2[4];
    uint8_t octetStr3[4];
    uint8_t octetStr4[4];

    // Convert to strings.
    itoa(octet1, octetStr1);
    itoa(octet2, octetStr2);
    itoa(octet3, octetStr3);
    itoa(octet4, octetStr4);

    // Build the IP string.
    strcpy(destinationMemory, octetStr1);
    destinationMemory += strlen(octetStr1);
    *destinationMemory++ = '.';
    strcpy(destinationMemory, octetStr2);
    destinationMemory += strlen(octetStr2);
    *destinationMemory++ = '.';
    strcpy(destinationMemory, octetStr3);
    destinationMemory += strlen(octetStr3);
    *destinationMemory++ = '.';
    strcpy(destinationMemory, octetStr4);
    destinationMemory += strlen(octetStr4);
    *destinationMemory = '\0';
}

/**
 * Converts a dotted decimal IPv4 string to 32-bit integer.
 * Validates format and octet ranges.
 * @param ipAddress The string IP address.
 * @return The integer IP or 0 if invalid.
 */
uint32_t ipAddressTointeger(uint8_t *ipAddress)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    // Octet string buffers.
    uint8_t octetStr1[4] = {0};
    uint8_t octetStr2[4] = {0};
    uint8_t octetStr3[4] = {0};
    uint8_t octetStr4[4] = {0};

    // Tracking positions.
    uint32_t position = 0;
    uint32_t index = 0;

    // Extract first octet.
    while (ipAddress[position] != '.' && ipAddress[position] != '\0' && index < 3)
    {
        octetStr1[index++] = ipAddress[position++];
    }
    octetStr1[index] = '\0';
    if (ipAddress[position] != '.') return 0;
    position++;
    index = 0;

    // Second octet.
    while (ipAddress[position] != '.' && ipAddress[position] != '\0' && index < 3)
    {
        octetStr2[index++] = ipAddress[position++];
    }
    octetStr2[index] = '\0';
    if (ipAddress[position] != '.') return 0;
    position++;
    index = 0;

    // Third octet.
    while (ipAddress[position] != '.' && ipAddress[position] != '\0' && index < 3)
    {
        octetStr3[index++] = ipAddress[position++];
    }
    octetStr3[index] = '\0';
    if (ipAddress[position] != '.') return 0;
    position++;
    index = 0;

    // Fourth octet.
    while (ipAddress[position] != '\0' && index < 3)
    {
        octetStr4[index++] = ipAddress[position++];
    }
    octetStr4[index] = '\0';
    if (ipAddress[position] != '\0') return 0;

    // Convert to ints.
    uint32_t octetInt1 = atoi(octetStr1);
    uint32_t octetInt2 = atoi(octetStr2);
    uint32_t octetInt3 = atoi(octetStr3);
    uint32_t octetInt4 = atoi(octetStr4);

    // Validate range.
    if (octetInt1 > 255 || octetInt2 > 255 || octetInt3 > 255 || octetInt4 > 255) return 0;

    // Combine into 32-bit.
    return (octetInt1 << 24) | (octetInt2 << 16) | (octetInt3 << 8) | octetInt4;
}

/**
 * Converts ASCII string to integer.
 * Handles sign and skips whitespace.
 * @param sourceString The string to convert.
 * @return The integer value.
 */
uint32_t atoi(uint8_t *sourceString)
{
    // Written by Grok.
    // 12/2025 with Grok v4.

    uint8_t *p = sourceString;
    // Skip whitespace
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\v' || *p == '\f') 
    {
        p++;
    }
    int sign = 1;
    if (*p == '-') 
    {
        sign = -1;
        p++;
    } 
    else if (*p == '+') 
    {
        p++;
    }
    uint32_t result = 0;
    while (*p >= '0' && *p <= '9') 
    {
        result = result * 10 + (*p - '0');
        p++;
    }
    if (sign == -1) 
    {
        result = (uint32_t) - (int)result;
    }
    return result;
}

/**
 * Computes the ceiling of number divided by base.
 * @param number The dividend.
 * @param base The divisor.
 * @return The ceiling value.
 */
uint32_t ceiling(uint32_t number, uint32_t base)
{
    // Dan O'Malley
    
    return ((number + base - 1) / base);
}

/**
 * Allocates a heap object for the user process.
 * Searches for free heap object and assigns to PID.
 * @param currentPid The process ID.
 * @param objectSize The size requested.
 * @return Pointer to allocated memory or 0 if failed.
 */
uint8_t *malloc(uint32_t currentPid, uint32_t objectSize)
{  
    // Dan O'Malley
    
    uint32_t heapIndex = 0;
    struct heapObjectUser *heapObject = (struct heapObjectUser *)USER_HEAP;

    // Check size limit.
    if (objectSize > HEAP_OBJ_USABLE_SIZE)
    {
        return 0;
    }

    // Find free object.
    while (heapIndex < MAX_HEAP_OBJECTS)
    {
        if (heapObject->pid == 0)
        {
            heapObject->pid = (uint8_t)currentPid;
            return &heapObject->heapUseable[0];    
        }
        heapIndex++;
        heapObject++;
    }
    // No free object.
    return 0;    
}

/**
 * Frees a user heap object by zeroing it.
 * @param heapObject Pointer to the usable part.
 */
void free(uint8_t *heapObject)
{
    // Dan O'Malley
    
    // Zero entire object including PID.
    fillMemory((heapObject - 1), 0x0, HEAP_OBJ_SIZE);
}

/**
 * Frees all heap objects owned by the PID.
 * @param currentPid The process ID.
 */
void freeAll(uint32_t currentPid)
{
    // Dan O'Malley
    
    uint32_t heapIndex = 0;
    uint32_t pidInHeap = 0;

    // Scan all objects.
    while (heapIndex < MAX_HEAP_OBJECTS)
    {

        pidInHeap = *(uint32_t *)(USER_HEAP + (HEAP_OBJ_SIZE * heapIndex));

        // If matches, zero it.
        if (pidInHeap == currentPid)
        {
            fillMemory((uint8_t *)(USER_HEAP + (HEAP_OBJ_SIZE * heapIndex)), 0x0, HEAP_OBJ_SIZE);
        }

        heapIndex++;
    }
}

/**
 * Allocates a kernel heap object.
 * Similar to malloc but for kernel.
 * @param currentPid The process ID (though kernel).
 * @param objectSize The size.
 * @return Pointer or 0.
 */
uint8_t *kMalloc(uint32_t currentPid, uint32_t objectSize)
{
    // Dan O'Malley
    
    uint32_t heapIndex = 0;
    struct heapObjectKernel *heapObject = (struct heapObjectKernel *)KERNEL_HEAP;

    if (objectSize > KERNEL_HEAP_OBJ_USABLE_SIZE)
    {
        return 0;
    }

    while (heapIndex < KERNEL_MAX_HEAP_OBJECTS)
    {
        if (heapObject->pid == 0)
        {
            heapObject->pid = (uint8_t)currentPid;
            return &heapObject->heapUseable[0];    
        }
        heapIndex++;
        heapObject++;
    }
    return 0;
}

/**
 * Frees a kernel heap object.
 * @param heapObject Pointer to usable part.
 */
void kFree(uint8_t *heapObject)
{
    // Dan O'Malley
    
    fillMemory((heapObject - 1), 0x0, KERNEL_HEAP_OBJ_SIZE);
}

/**
 * Frees all kernel heap objects for PID.
 * @param currentPid The process ID.
 */
void kFreeAll(uint32_t currentPid)
{
    // Dan O'Malley
    
    uint32_t heapIndex = 0;
    uint32_t pidInHeap = 0;

    while (heapIndex < KERNEL_MAX_HEAP_OBJECTS)
    {

        pidInHeap = *(uint32_t *)(KERNEL_HEAP + (KERNEL_HEAP_OBJ_SIZE * heapIndex));

        if (pidInHeap == currentPid)
        {
            fillMemory((uint8_t *)(KERNEL_HEAP + (KERNEL_HEAP_OBJ_SIZE * heapIndex)), 0x0, KERNEL_HEAP_OBJ_SIZE);
        }

        heapIndex++;
    }
}

/**
 * Computes a simple hash of a string by summing characters.
 * @param messageToHash The string to hash.
 * @return The hash value.
 */
uint32_t stringHash(uint8_t *messageToHash)
{
    // Dan O'Malley
    
    uint32_t hashSum = 0;

    // Sum each character.
    while (*messageToHash != 0x0)
    { 
        hashSum += *messageToHash;
        messageToHash++;
    } 

    return hashSum;
}

/**
 * Makes a sound with given frequency and duration.
 * Allocates params, makes syscall, frees.
 * @param frequency The sound frequency.
 * @param duration The duration in some units.
 */
void makeSound(uint32_t frequency, uint32_t duration)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    
    struct soundParameter *soundParams = (struct soundParameter *)(malloc(currentPid, sizeof(soundParameter)));

    soundParams->frequency = frequency;
    soundParams->duration = duration;
    sysCall(SYS_SOUND, (uint32_t)soundParams, currentPid); 

    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)soundParams);
}

/**
 * Requests system uptime via syscall.
 */
void systemUptime()
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_UPTIME, 0x0, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Exits the process with given code.
 * @param exitCode The exit status.
 */
void systemExit(uint32_t exitCode)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_EXIT, exitCode, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Requests free memory info via syscall.
 */
void systemFree()
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_FREE, 0x0, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Requests memory mapping for a page.
 * @param requestedPage The page to map.
 * @return The mapped page or original.
 */
uint8_t *systemMMap(uint8_t* requestedPage)
{
    // Dan O'Malley
    
    uint8_t* returnedPage;
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_MMAP, (uint32_t)requestedPage, currentPid);
    returnedPage = (uint8_t*)readValueFromMemLoc(RETURNED_MMAP_PAGE_LOC);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    return returnedPage;
}

/**
 * Switches to parent process if not PID 1.
 */
void systemSwitchToParent()
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    if (currentPid == 1)
    {
        return; // No parent.
    }

    sysCall(SYS_SWITCH_TASK_TO_PARENT, 0x0, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Shows running processes via syscall.
 */
void systemShowProcesses()
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_PS, 0x0, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Lists current directory via syscall.
 */
void systemListDirectory()
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_DIR, 0x0, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Toggles the scheduler via syscall.
 */
void systemSchedulerToggle()
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_TOGGLE_SCHEDULER, 0x0, currentPid);
    printString(COLOR_WHITE, 2, 5, (uint8_t *)"Scheduler toggled.");
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Executes a file with given parameters.
 * @param fileName The file to exec.
 * @param requestedRunPriority Priority.
 * @param requestedStdIn Stdin FD.
 * @param requestedStdOut Stdout FD.
 * @param requestedStdErr Stderr FD.
 */
void systemExec(uint8_t *fileName, uint32_t requestedRunPriority, uint32_t requestedStdIn, uint32_t requestedStdOut, uint32_t requestedStdErr)
{
    // Dan O'Malley
    
    if (fileName == 0)
    {
        return;
    }
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(fileName);
    strcpy(fileParams->fileName, fileName);
    fileParams->requestedRunPriority = requestedRunPriority;
    fileParams->requestedStdInFd = requestedStdIn;
    fileParams->requestedStdOutFd = requestedStdOut;
    fileParams->requestedStdErrFd = requestedStdErr;

    sysCall(SYS_EXEC, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)fileParams);
}

/**
 * Kills a process by PID.
 * Cannot kill self.
 * @param pidToKill String PID.
 */
void systemKill(uint8_t *pidToKill)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint8_t *pidToKillMem = malloc(currentPid, sizeof(uint32_t));
    bytecpy(pidToKillMem, pidToKill, 1);

    if (atoi(pidToKillMem) == currentPid)
    {
        printString(COLOR_RED, 1, 2, (uint8_t *)"You cannot kill yourself!");

        free(pidToKillMem);
        return;
    }

    sysCall(SYS_KILL, atoi(pidToKillMem), currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free(pidToKillMem);
}

/**
 * Switches task to another PID.
 * Cannot switch to self.
 * @param pidToSwitchTo String PID.
 */
void systemTaskSwitch(uint8_t *pidToSwitchTo)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint8_t *pidToSwitch = malloc(currentPid, 20);
    bytecpy(pidToSwitch, pidToSwitchTo, 20);

    if (hextoi(pidToSwitch) == currentPid)
    { 
        printString(COLOR_RED, 1, 2, (uint8_t *)"You cannot switch to yourself!");

        return;
    }

    sysCall(SYS_SWITCH_TASK, atoi(pidToSwitch), currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free(pidToSwitch);
}

/**
 * Shows memory dump from location.
 * @param memoryLocation Start address.
 */
void systemShowMemory(uint8_t *memoryLocation)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_MEM_DUMP, (uint32_t)memoryLocation, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Shows ELF header from memory.
 * @param memoryLocation ELF start.
 */
void systemShowElfHeader(uint8_t *memoryLocation)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_ELF_HEADER_TO_VIEW, (uint32_t)memoryLocation, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Shows disassembly from memory.
 * @param memoryLocation Code start.
 */
void systemShowDisassembly(uint8_t *memoryLocation)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_MEM_DISASSEMBLY, (uint32_t)memoryLocation, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Shows open files starting at row.
 * @param startDisplayAtRow Display start row.
 */
void systemShowOpenFiles(uint32_t startDisplayAtRow)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_SHOW_OPEN_FILES, startDisplayAtRow, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Shows global objects via syscall.
 */
void systemShowGlobalObjects()
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_SHOW_GLOBAL_OBJECTS, 0x0, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Shows inode details.
 * @param inodeNumber The inode.
 */
void systemShowInodeDetail(uint32_t inodeNumber)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    sysCall(SYS_INODE_TO_VIEW, (uint32_t)inodeNumber, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Opens a file with permissions.
 * @param fileName The file.
 * @param requestedPermissions Permissions.
 * @return FD or fail.
 */
uint32_t systemOpenFile(uint8_t *fileName, uint32_t requestedPermissions)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint32_t returnValue = SYSCALL_FAIL; // Default fail.

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    
    fileParams->fileNameLength = strlen(fileName);
    fileParams->requestedPermissions = requestedPermissions;
    strcpy(fileParams->fileName, fileName);

    returnValue = sysCall(SYS_OPEN, (uint32_t)fileParams, currentPid);    
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    free((uint8_t *)fileParams);

    return returnValue;
}

/**
 * Gets inode structure for file.
 * @param fileName The file.
 */
void systemGetInodeStructureOfFile(uint8_t *fileName)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(fileName);
    strcpy(fileParams->fileName, fileName);
    sysCall(SYS_GET_INODE_STRUCT, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)fileParams);
}

/**
 * Changes file mode.
 * @param fileName The file.
 * @param requestMode String mode.
 */
void systemChangeFileMode(uint8_t *fileName, uint8_t *requestMode)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(fileName);
    strcpy(fileParams->fileName, fileName);
    fileParams->requestedFileMode = stringToOctal(requestMode);
    sysCall(SYS_CHANGE_FILE_MODE, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)fileParams);
}

/**
 * Changes current directory.
 * @param fileName Directory name.
 */
void systemChangeDirectory(uint8_t *fileName)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(fileName);
    strcpy(fileParams->fileName, fileName);
    sysCall(SYS_CHANGE_DIR, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)fileParams);
}

/**
 * Opens an empty file with size.
 * @param fileName The name.
 * @param requestedSizeInPages Size in pages.
 */
void systemOpenEmptyFile(uint8_t *fileName, uint32_t requestedSizeInPages)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(fileName);
    fileParams->requestedPermissions = RDWRITE;
    fileParams->requestedSizeInPages = requestedSizeInPages;
    strcpy(fileParams->fileName, fileName);
    sysCall(SYS_OPEN_EMPTY, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)fileParams);
}

/**
 * Creates a file from FD.
 * @param fileName Name.
 * @param fileDescriptor FD.
 */
void systemCreateFile(uint8_t *fileName, uint32_t fileDescriptor)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(fileName);
    fileParams->fileDescriptor = fileDescriptor;
    strcpy(fileParams->fileName, fileName);
    sysCall(SYS_CREATE, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)fileParams);
}

/**
 * Moves a file between directories.
 * @param fileName The file.
 * @param sourceDirectory Source dir.
 * @param destinationDirectory Dest dir.
 */
void systemMoveFile(uint8_t *fileName, uint8_t *sourceDirectory, uint8_t *destinationDirectory)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    uint8_t *fileNameParam = malloc(currentPid, 30);
    uint8_t *sourceDirParam = malloc(currentPid, 50);
    uint8_t *destDirParam = malloc(currentPid, 50);

    strcpy(fileNameParam, fileName);
    strcpy(sourceDirParam, sourceDirectory);
    strcpy(destDirParam, destinationDirectory);

    fileParams->fileNameLength = strlen(fileName);
    fileParams->sourceDirectory = sourceDirParam;
    fileParams->destinationDirectory = destDirParam;   
    fileParams->fileName = fileNameParam;

    sysCall(SYS_MOVE_FILE, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)fileParams);
    free(fileNameParam);
    free(sourceDirParam);
    free(destDirParam);
}

/**
 * Deletes a file.
 * @param fileName The file.
 */
void systemDeleteFile(uint8_t *fileName)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(fileName);
    strcpy(fileParams->fileName, fileName);
    sysCall(SYS_DELETE, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    free((uint8_t *)fileParams);
}

/**
 * Closes a file by FD.
 * @param fileDescriptor The FD.
 */
void systemCloseFile(uint32_t fileDescriptor)
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_CLOSE, fileDescriptor, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Translates octal permissions to string like "RWX".
 * @param permissions The octal value.
 * @return String representation.
 */
uint8_t *octalTranslation(uint8_t permissions)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    if (permissions == 0x0) { return (uint8_t *)"---"; }
    else if (permissions == 0x1) { return (uint8_t *)"--X"; }
    else if (permissions == 0x2) { return (uint8_t *)"-W-"; }
    else if (permissions == 0x3) { return (uint8_t *)"-WX"; }
    else if (permissions == 0x4) { return (uint8_t *)"R--"; }
    else if (permissions == 0x5) { return (uint8_t *)"R-X"; }
    else if (permissions == 0x6) { return (uint8_t *)"RW-"; }
    else if (permissions == 0x7) { return (uint8_t *)"RWX"; }
    else { return (uint8_t *)"UNK"; }
}

/**
 * Converts string mode like "-rwxr-xr-x" to octal.
 * @param modeString The string mode.
 * @return The mode value.
 */
uint16_t stringToOctal(uint8_t *modeString)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint16_t mode = 0;

    // File type.
    uint8_t type = modeString[0];
    if (type == '-') {
        mode |= 0x8000; // Regular file
    } else if (type == 'd') {
        mode |= 0x4000; // Directory
    } else if (type == 'l') {
        mode |= 0xA000; // Symlink
    } else if (type == 'c') {
        mode |= 0x2000; // Character device
    } else if (type == 'b') {
        mode |= 0x6000; // Block device
    } else if (type == 'p') {
        mode |= 0x1000; // FIFO
    } else if (type == 's') {
        mode |= 0xC000; // Socket
    } else {
        return 0; // Invalid
    }

    // User perms.
    if (modeString[1] == 'r') mode |= 0400;
    if (modeString[2] == 'w') mode |= 0200;
    if (modeString[3] == 'x') mode |= 0100;
    else if (modeString[3] == 's') { mode |= 0100 | 04000; }
    else if (modeString[3] == 'S') { mode |= 04000; }

    // Group perms.
    if (modeString[4] == 'r') mode |= 0040;
    if (modeString[5] == 'w') mode |= 0020;
    if (modeString[6] == 'x') mode |= 0010;
    else if (modeString[6] == 's') { mode |= 0010 | 02000; }
    else if (modeString[6] == 'S') { mode |= 02000; }

    // Other perms.
    if (modeString[7] == 'r') mode |= 0004;
    if (modeString[8] == 'w') mode |= 0002;
    if (modeString[9] == 'x') mode |= 0001;
    else if (modeString[9] == 't') { mode |= 0001 | 01000; }
    else if (modeString[9] == 'T') { mode |= 01000; }

    return mode;
}

/**
 * Checks if mode has owner execute bit.
 * @param mode The mode.
 * @return True if set.
 */
bool hasOwnerExecute(uint16_t mode)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    return (mode & 0100) != 0;
}

/**
 * Checks if mode has group execute bit.
 * @param mode The mode.
 * @return True if set.
 */
bool hasGroupExecute(uint16_t mode)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    return (mode & 0010) != 0;
}

/**
 * Checks if mode has other execute bit.
 * @param mode The mode.
 * @return True if set.
 */
bool hasOtherExecute(uint16_t mode)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    return (mode & 0001) != 0;
}

/**
 * Translates directory entry type to string.
 * @param type The type code.
 * @return "FILE", "DIR", or "UNK".
 */
uint8_t *directoryEntryTypeTranslation(uint8_t type)
{
    // Dan O'Malley
    
    if (type == EXT2_DIRECTORY_ENTRY_FILE) { return (uint8_t *)"FILE"; }
    else if (type == EXT2_DIRECTORY_ENTRY_DIR) { return (uint8_t *)"DIR"; }
    else { return (uint8_t *)"UNK"; }
}

/**
 * Counts used user heap objects.
 * @param heapLoc Heap start.
 * @return Count of used objects.
 */
uint32_t countHeapObjects(uint8_t *heapLoc)
{
    // Dan O'Malley
    
    uint32_t heapIndex = 0;
    uint32_t lastUsedObj = *(uint8_t *)(heapLoc + (HEAP_OBJ_SIZE * heapIndex));
    uint32_t objectCount = 0;

    while (heapIndex < MAX_HEAP_OBJECTS)
    {      
        lastUsedObj = *(uint8_t *)(heapLoc + (HEAP_OBJ_SIZE * heapIndex));

        if ((uint8_t)lastUsedObj != 0)
        {
            objectCount++;
        }
        heapIndex++;
    } 

    return objectCount;
}

/**
 * Counts used kernel heap objects.
 * @param heapLoc Heap start.
 * @return Count.
 */
uint32_t countKernelHeapObjects(uint8_t *heapLoc)
{
    // Dan O'Malley
    
    uint32_t heapIndex = 0;
    uint32_t lastUsedObj = *(uint8_t *)(heapLoc + (KERNEL_HEAP_OBJ_SIZE * heapIndex));
    uint32_t objectCount = 0;

    while (heapIndex < KERNEL_MAX_HEAP_OBJECTS)
    {      
        lastUsedObj = *(uint8_t *)(heapLoc + (KERNEL_HEAP_OBJ_SIZE * heapIndex));

        if ((uint8_t)lastUsedObj != 0)
        {
            objectCount++;
        }
        heapIndex++;
    } 

    return objectCount;
}

/**
 * Converts struct time to Unix timestamp.
 * @param Time The time struct.
 * @return Unix time.
 */
uint32_t convertToUnixTime(struct time* Time)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t sec = Time->sec;
    uint32_t min = Time->min;
    uint32_t hour = Time->hour;
    uint32_t doy = Time->dayOfYear;
    uint32_t year = Time->year;

    uint32_t leaps1969 = 1969 / 4 - 1969 / 100 + 1969 / 400;
    uint32_t leaps = ((year - 1) / 4 - (year - 1) / 100 + (year - 1) / 400) - leaps1969;

    uint32_t days = (year - 1970) * 365 + leaps + (doy - 1);

    return days * SECONDS_IN_DAY + hour * SECONDS_IN_HOUR + min * SECONDS_IN_MIN + sec;
}

/**
 * Converts Unix time to struct time.
 * Adjusts for year 2000 base.
 * @param unixTime The timestamp.
 * @param Time Output struct.
 */
void convertFromUnixTime(uint32_t unixTime, struct time *Time)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    unixTime -= 946684800;  // To 2000.

    unsigned long epoch = unixTime;

    Time->sec = epoch % 60;
    epoch /= 60;
    Time->min = epoch % 60;
    epoch /= 60;
    Time->hour = epoch % 24;
    epoch /= 24;

    unsigned int years = epoch / (365 * 4 + 1) * 4;
    epoch %= (365 * 4 + 1);

    unsigned int subyear;
    static const unsigned short days_start[4] = {0, 366, 731, 1096};
    for (subyear = 3; subyear > 0; subyear--) {
        if (epoch >= days_start[subyear]) break;
    }

    Time->dayOfYear = epoch - days_start[subyear] + 1;

    Time->year = 2000 + years + subyear;
}

/**
 * Creates a named pipe.
 * @param pipeName The name.
 */
void systemCreatePipe(uint8_t *pipeName) 
{  
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(pipeName);
    strcpy(fileParams->fileName, pipeName);
    sysCall(SYS_CREATE_PIPE, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Creates a network pipe (socket).
 * @param socketName Name.
 * @param sourceIPAddress Source IP.
 * @param destinationIPAddress Dest IP.
 * @param sourcePort Source port.
 * @param destinationPort Dest port.
 */
void systemCreateNetworkPipe(uint8_t *socketName, uint32_t sourceIPAddress, uint32_t destinationIPAddress, uint16_t sourcePort, uint16_t destinationPort)
{  
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct networkParameter *networkParams = (struct networkParameter *)(malloc(currentPid, sizeof(networkParameter)));
    networkParams->sourceIPAddress = sourceIPAddress;
    networkParams->destinationIPAddress = destinationIPAddress;
    networkParams->sourcePort = sourcePort;
    networkParams->destinationPort = destinationPort;
    strcpy(networkParams->socketName, socketName);

    sysCall(SYS_CREATE_NETWORK_PIPE, (uint32_t)networkParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Opens a pipe by name.
 * @param pipeName The name.
 * @return FD or fail.
 */
uint32_t systemOpenPipe(uint8_t *pipeName) 
{  
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint32_t returnValue = SYSCALL_FAIL;

    struct fileParameter *fileParams = (struct fileParameter *)(malloc(currentPid, sizeof(fileParameter)));
    fileParams->fileNameLength = strlen(pipeName);
    strcpy(fileParams->fileName, pipeName);
    returnValue = sysCall(SYS_OPEN_PIPE, (uint32_t)fileParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    return returnValue;
}

/**
 * Writes to pipe by FD string.
 * @param fileDescriptor Str FD.
 */
void systemWritePipe(uint8_t *fileDescriptor) 
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_WRITE, atoi(fileDescriptor), currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Reads from pipe by FD string.
 * @param fileDescriptor Str FD.
 */
void systemReadPipe(uint8_t *fileDescriptor) 
{
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    sysCall(SYS_READ, atoi(fileDescriptor), currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Sends network packet.
 * @param sourceIPAddress Source IP.
 * @param destinationIPAddress Dest IP.
 * @param sourcePort Source port.
 * @param destinationPort Dest port.
 * @param packetPayload Payload.
 */
void systemNetSend(uint32_t sourceIPAddress, uint32_t destinationIPAddress, uint16_t sourcePort, uint16_t destinationPort, uint8_t *packetPayload) 
{  
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);

    struct networkParameter *networkParams = (struct networkParameter *)(malloc(currentPid, sizeof(networkParameter)));
    networkParams->destinationIPAddress = destinationIPAddress;
    networkParams->destinationPort = destinationPort;
    strcpy(networkParams->packetPayload, packetPayload);
    networkParams->sourceIPAddress = sourceIPAddress;
    networkParams->sourcePort = sourcePort;
    
    sysCall(SYS_NET_SEND, (uint32_t)networkParams, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Receives network data.
 */
void systemNetRcv() 
{  
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    
    sysCall(SYS_NET_RCV, 0, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Views disk sector.
 * @param diskSectorToView Sector number.
 */
void systemDiskSectorViewer(uint32_t diskSectorToView) 
{  
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    
    sysCall(SYS_SECTOR_TO_VIEW, diskSectorToView, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Views file system block.
 * @param blockToView Block number.
 */
void systemBlockViewer(uint32_t blockToView) 
{  
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    
    sysCall(SYS_BLOCK_TO_VIEW, blockToView, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Views kernel log.
 */
void systemKernelLogViewer() 
{  
    // Dan O'Malley
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    
    sysCall(SYS_KERNEL_LOG_VIEW, 0x0, currentPid);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
}

/**
 * Waits for enter (continue) or Q (quit).
 * @return 1 for enter, 0 for Q.
 */
uint32_t waitForEnterOrQuit()
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint8_t isShiftDown = 0;
    while (true)
    {
        uint8_t tempScanCode;
        uint8_t scanCode;
        do
        {
            uint8_t status;
            do {
                status = inputIOPort(0x64);
            } while ((status & 0x01) == 0);
            tempScanCode = inputIOPort(0x60);
            if (tempScanCode == 0x2A || tempScanCode == 0x36)
            {
                isShiftDown = 1;
                continue;
            }
            if (tempScanCode == 0xAA || tempScanCode == 0xB6)
            {
                isShiftDown = 0;
                continue;
            }
            if (tempScanCode & 0x80)
            {
                continue;
            }
            scanCode = tempScanCode;
            break;
        } while (1);

        if (scanCode == 0x1C)
        { 
            return 1;
        }
        else if (scanCode == 0x10)
        { 
            return 0;
        }
    }
}

/**
 * Concatenates two strings.
 * Allocates new buffer.
 * @param first First string.
 * @param second Second string.
 * @return New string or 0.
 */
uint8_t* strConcat(uint8_t* first, uint8_t* second) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint32_t len1 = strlen(first);
    uint32_t len2 = strlen(second);
    uint8_t* result = malloc(currentPid, len1 + len2 + 1);

    if (result == 0) {
        return 0;
    }
    strcpy(result, first);
    strcpy(result + len1, second);
    return result;
}

/**
 * Removes extension from filename.
 * @param filename The name.
 * @return Base name or 0.
 */
uint8_t* removeExtension(uint8_t* filename) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint32_t len = strlen(filename);
    uint32_t i = len;
    while (i > 0) {
        i--;
        if (filename[i] == '.') {
            break;
        }
    }
    uint32_t base_len;
    if (i == 0 && filename[i] != '.') {
        base_len = len;
    } else {
        base_len = i;
    }
    uint8_t* result = malloc(currentPid, base_len + 1);
    if (result == 0) {
        return 0;
    }
    bytecpy(result, filename, base_len);
    result[base_len] = 0;
    return result;
}

/**
 * Gets substring removing chars from right.
 * @param original The string.
 * @param charsToRemoveFromRight Number to remove.
 * @return New string or 0.
 */
uint8_t* subString(uint8_t* original, uint32_t charsToRemoveFromRight) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint32_t len = strlen(original);
    if (charsToRemoveFromRight >= len) {
        uint8_t* result = malloc(currentPid, 1);
        if (result == 0) {
            return 0;
        }
        result[0] = 0;
        return result;
    }
    uint32_t newLen = len - charsToRemoveFromRight;
    uint8_t* result = malloc(currentPid, newLen + 1);
    if (result == 0) {
        return 0;
    }
    bytecpy(result, original, newLen);
    result[newLen] = 0;
    return result;
}

/**
 * Checks if character is whitespace.
 * @param character The char.
 * @return 1 if yes.
 */
int isSpace(uint8_t character) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    return character == ' ' || character == '\t' || character == '\r' || character == '\n' || character == '\f' || character == '\v';
}

/**
 * Skips whitespace in string.
 * @param stringPointer Pointer.
 * @return New pointer.
 */
uint8_t *skipWhitespace(uint8_t *stringPointer) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    while (*stringPointer && isSpace(*stringPointer)) stringPointer++;
    return stringPointer;
}

/**
 * Converts to lowercase.
 * @param character The char.
 * @return Lowercase.
 */
unsigned char toLower(unsigned char character)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    return (character >= 'A' && character <= 'Z') ? character + 32 : character;
}

/**
 * Checks if digit.
 * @param character Char.
 * @return 1 if yes.
 */
int isDigit(unsigned char character)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    return character >= '0' && character <= '9';
}

/**
 * Checks if alphabetic.
 * @param character Char.
 * @return 1 if yes.
 */
int isAlpha(unsigned char character)
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

/**
 * Simple random number generator.
 * Uses interrupts count as seed.
 * @return Random 0-32767.
 */
uint32_t rand()
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t seed = readValueFromMemLoc((uint8_t*)USER_TOTAL_INTERRUPTS_COUNT_LOC);
    seed = seed * 1103515245 + 12345;
    return (seed / 65536) % 32768;
}

/**
 * Prints formatted string to screen at position.
 * Supports %d, %x, %s, %c, %%.
 * @param color Color.
 * @param row Row.
 * @param column Column.
 * @param format Format string.
 * @param ... Args.
 */
void printf(uint32_t color, uint32_t row, uint32_t column, uint8_t *format, ...) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    va_list ap;
    va_start(ap, format);

    while (*format != '\0') {
        if (row >= 50) {
            break;
        }
        if (*format == '%') {
            format++;
            if (*format == '\0') break;

            switch (*format) {
                case 'd': {
                    uint32_t num = va_arg(ap, uint32_t);
                    uint8_t buf[12];
                    itoa(num, buf);
                    uint8_t *p = buf;
                    while (*p != '\0') {
                        if (row >= 50) break;
                        printCharacter(color, row, column, p);
                        column++;
                        if (column == 80) {
                            column = 0;
                            row++;
                        }
                        p++;
                    }
                    break;
                }
                case 'x': {
                    uint32_t num = va_arg(ap, uint32_t);
                    uint8_t buf[9];
                    itoaHex(num, buf);
                    uint8_t *p = buf;
                    while (*p != '\0') {
                        if (row >= 50) break;
                        printCharacter(color, row, column, p);
                        column++;
                        if (column == 80) {
                            column = 0;
                            row++;
                        }
                        p++;
                    }
                    break;
                }
                case 's': {
                    uint8_t *str = (uint8_t *)va_arg(ap, uint32_t);
                    uint8_t *p = str;
                    while (*p != '\0') {
                        if (row >= 50) break;
                        printCharacter(color, row, column, p);
                        column++;
                        if (column == 80) {
                            column = 0;
                            row++;
                        }
                        p++;
                    }
                    break;
                }
                case 'c': {
                    uint32_t ch = va_arg(ap, uint32_t);
                    uint8_t c = (uint8_t)ch;
                    if (row < 50) {
                        printCharacter(color, row, column, &c);
                        column++;
                        if (column == 80) {
                            column = 0;
                            row++;
                        }
                    }
                    break;
                }
                case '%': {
                    uint8_t c = '%';
                    if (row < 50) {
                        printCharacter(color, row, column, &c);
                        column++;
                        if (column == 80) {
                            column = 0;
                            row++;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            format++;
        } else if (*format == '\n') {
            row++;
            column = 0;
            format++;
        } else if (*format == '\r') {
            column = 0;
            format++;
        } else {
            printCharacter(color, row, column, format);
            column++;
            if (column == 80) {
                column = 0;
                row++;
            }
            format++;
        }
    }

    va_end(ap);
}

/**
 * Gets ASCII from keyboard scan code.
 * @param scanCode Code.
 * @param shift_down Shift state.
 * @return ASCII or 0.
 */
uint8_t getAsciiFromScanCode(uint8_t scanCode, uint8_t shift_down) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    switch (scanCode) {
        case 0x02: return shift_down ? '!' : '1';
        case 0x03: return shift_down ? '@' : '2';
        case 0x04: return shift_down ? '#' : '3';
        case 0x05: return shift_down ? '$' : '4';
        case 0x06: return shift_down ? '%' : '5';
        case 0x07: return shift_down ? '^' : '6';
        case 0x08: return shift_down ? '&' : '7';
        case 0x09: return shift_down ? '*' : '8';
        case 0x0A: return shift_down ? '(' : '9';
        case 0x0B: return shift_down ? ')' : '0';
        case 0x0C: return shift_down ? '_' : '-';
        case 0x0D: return shift_down ? '+' : '=';
        case 0x10: return shift_down ? 'Q' : 'q';
        case 0x11: return shift_down ? 'W' : 'w';
        case 0x12: return shift_down ? 'E' : 'e';
        case 0x13: return shift_down ? 'R' : 'r';
        case 0x14: return shift_down ? 'T' : 't';
        case 0x15: return shift_down ? 'Y' : 'y';
        case 0x16: return shift_down ? 'U' : 'u';
        case 0x17: return shift_down ? 'I' : 'i';
        case 0x18: return shift_down ? 'O' : 'o';
        case 0x19: return shift_down ? 'P' : 'p';
        case 0x1A: return shift_down ? '{' : '[';
        case 0x1B: return shift_down ? '}' : ']';
        case 0x1E: return shift_down ? 'A' : 'a';
        case 0x1F: return shift_down ? 'S' : 's';
        case 0x20: return shift_down ? 'D' : 'd';
        case 0x21: return shift_down ? 'F' : 'f';
        case 0x22: return shift_down ? 'G' : 'g';
        case 0x23: return shift_down ? 'H' : 'h';
        case 0x24: return shift_down ? 'J' : 'j';
        case 0x25: return shift_down ? 'K' : 'k';
        case 0x26: return shift_down ? 'L' : 'l';
        case 0x27: return shift_down ? ':' : ';';
        case 0x28: return shift_down ? '"' : '\'';
        case 0x29: return shift_down ? '~' : '`';
        case 0x2B: return shift_down ? '|' : '\\';
        case 0x2C: return shift_down ? 'Z' : 'z';
        case 0x2D: return shift_down ? 'X' : 'x';
        case 0x2E: return shift_down ? 'C' : 'c';
        case 0x2F: return shift_down ? 'V' : 'v';
        case 0x30: return shift_down ? 'B' : 'b';
        case 0x31: return shift_down ? 'N' : 'n';
        case 0x32: return shift_down ? 'M' : 'm';
        case 0x33: return shift_down ? '<' : ',';
        case 0x34: return shift_down ? '>' : '.';
        case 0x35: return shift_down ? '?' : '/';
        case 0x39: return ' ';
        default: return 0;
    }
}

/**
 * Reads formatted input from keyboard at position.
 * Supports %d, %x, %s, %c.
 * Handles backspace, enter.
 * @param color Color.
 * @param row Row.
 * @param column Column.
 * @param format Format.
 * @param ... Pointers for input.
 * @return Number of assignments.
 */
uint32_t scanf(uint32_t color, uint32_t row, uint32_t column, uint8_t *format, ...) 
{
    // Written by Grok.
    // 12/2025 with Grok v4.
    
    va_list ap;
    va_start(ap, format);

    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    uint8_t *buffer = malloc(currentPid, 200);
    if (buffer == 0) {
        va_end(ap);
        return 0;
    }
    uint8_t *buf_ptr = buffer;
    uint32_t buf_len = 0;

    uint8_t shift_down = 0;
    //enableCursor();
    //moveCursor(row, column);

    while (1) {
        uint8_t temp_scan;
        uint8_t scanCode;
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
            if (temp_scan & 0x80) {
                continue;
            }
            scanCode = temp_scan;
            break;
        } while (1);

        if (scanCode == 0x1C) {
            *buf_ptr++ = '\n';
            buf_len++;
            row++;
            if (row == 50) {
                bytecpy((uint8_t *)VIDEO_RAM, (uint8_t *)VIDEO_RAM + 160, 7840);
                fillMemory((uint8_t *)VIDEO_RAM + 160 * 49, 0x0, 160);
                row = 49;
            }
            column = 0;
            break;
        } else if (scanCode == 0x0E) {
            if (buf_len > 0) {
                buf_len--;
                buf_ptr--;
                if (column == 0) {
                    column = 79;
                    row--;
                } else {
                    column--;
                }
                uint8_t empty = ' ';
                printCharacter(color, row, column, &empty);
                //moveCursor(row, column);
            }
            continue;
        } else {
            uint8_t ascii = getAsciiFromScanCode(scanCode, shift_down);
            if (ascii != 0 && buf_len < 200) {
                *buf_ptr++ = ascii;
                buf_len++;
                printCharacter(color, row, column, &ascii);
                column++;
                if (column == 80) {
                    column = 0;
                    row++;
                    if (row == 50) {
                        bytecpy((uint8_t *)VIDEO_RAM, (uint8_t *)VIDEO_RAM + 160, 7840);
                        fillMemory((uint8_t *)VIDEO_RAM + 160 * 49, 0x0, 160);
                        row = 49;
                    }
                }
                //moveCursor(row, column);
            }
        }
    }

    *buf_ptr = '\0';

    uint8_t *input_ptr = buffer;
    uint32_t assignments = 0;

    while (*format != '\0') {
        if (*format == '%') {
            format++;
            if (*format == '\0') break;

            input_ptr = skipWhitespace(input_ptr);

            switch (*format) {
                case 'd': {
                    if (!isDigit(*input_ptr) && *input_ptr != '-') break;
                    uint32_t *dest = va_arg(ap, uint32_t *);
                    *dest = atoi(input_ptr);
                    assignments++;
                    if (*input_ptr == '-') input_ptr++;
                    while (isDigit(*input_ptr)) input_ptr++;
                    break;
                }
                case 'x': {
                    if (!isDigit(*input_ptr) && !((*input_ptr >= 'a' && *input_ptr <= 'f') || (*input_ptr >= 'A' && *input_ptr <= 'F')) && *input_ptr != '0') break;
                    uint32_t *dest = va_arg(ap, uint32_t *);
                    *dest = hextoi(input_ptr);
                    assignments++;
                    if (*input_ptr == '0' && (*(input_ptr + 1) == 'x' || *(input_ptr + 1) == 'X')) input_ptr += 2;
                    while (isDigit(*input_ptr) || ((*input_ptr >= 'a' && *input_ptr <= 'f') || (*input_ptr >= 'A' && *input_ptr <= 'F'))) input_ptr++;
                    break;
                }
                case 's': {
                    if (isSpace(*input_ptr)) break;
                    uint8_t *dest = va_arg(ap, uint8_t *);
                    uint8_t *start = input_ptr;
                    while (*input_ptr && !isSpace(*input_ptr)) input_ptr++;
                    bytecpy(dest, start, input_ptr - start);
                    dest[input_ptr - start] = '\0';
                    assignments++;
                    break;
                }
                case 'c': {
                    if (*input_ptr == '\0') break;
                    uint8_t *dest = va_arg(ap, uint8_t *);
                    *dest = *input_ptr++;
                    assignments++;
                    break;
                }
                default:
                    break;
            }
            format++;
        } else {
            format++;
        }
    }

    free(buffer);
    va_end(ap);
    if (row >= 50) row = 49;
    if (column >= 80) column = 79;
    //moveCursor(row, column);
    return assignments;
}

/**
 * Prints string to screen at position.
 * @param color Color.
 * @param row Row.
 * @param column Column.
 * @param message String.
 */
void printString(uint32_t color, uint32_t row, uint32_t column, uint8_t *message)
{

    // ASSIGNMENT 1 TO DO
    const int SCREEN_WIDTH = 80, SCREEN_HEIGHT = 25;
    uint32_t curRow = row;
    uint32_t curCol = column;

    for (uint32_t i = 0; message[i] != '\0'; i++) {
        // Call printCharacter on each character
        printCharacter(color, curRow, curCol, &message[i]);

        // Handle special characters (mirroring printCharacter behavior)
        if (message[i] == '\n') {
            curRow = (curRow + 1) % SCREEN_HEIGHT;
            curCol = 0;
        } else if (message[i] == '\t') {
            const uint32_t tabWidth = 4;
            curCol = ((curCol / tabWidth) + 1) * tabWidth;
            if (curCol >= SCREEN_WIDTH) {
                curCol = 0;
                curRow = (curRow + 1) % SCREEN_HEIGHT;
            }
        } else if (message[i] != '\r') { // normal char
            curCol++;
            if (curCol >= SCREEN_WIDTH) {
                curCol = 0;
                curRow = (curRow + 1) % SCREEN_HEIGHT;
            }
        }
    }

    moveCursor(curRow, curCol);

}

/**
 * Clears the entire screen.
 */
void clearScreen()
{       
    
    // ASSIGNMENT 1 TO DO
    const int SCREEN_WIDTH = 80, SCREEN_HEIGHT = 25;
     for(uint32_t i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT*2; ++i)
        VIDEO_RAM[i]=0;

    



}

/**
 * Prints a single character at position.
 * Handles newline, carriage return, tab.
 * @param color Color.
 * @param row Row.
 * @param column Column.
 * @param message Char pointer.
 */
void printCharacter(uint32_t color, uint32_t row, uint32_t column, uint8_t *message)
{
    
    // ASSIGNMENT 1 TO DO
    const int SCREEN_WIDTH = 80, SCREEN_HEIGHT = 25;
    moveCursor(row, column);

    // Handle special characters
    if (*message == 0x0D) { // carriage return
        return;
    }

    if (*message == 0x0A) { // newline
        moveCursor((row + 1) % SCREEN_HEIGHT, 0);
        return;
    }

    if (*message == 0x09) { // tab
        const uint32_t tabWidth = 4;
        uint32_t nextTab = ((column / tabWidth) + 1) * tabWidth;
        if (nextTab >= SCREEN_WIDTH) {
            row = (row + 1) % SCREEN_HEIGHT;
            nextTab = 0;
        }
        moveCursor(row, nextTab);
        return;
    }

    // print normal character
    uint32_t offset = (row * SCREEN_WIDTH + column) * 2;
    VIDEO_RAM[offset] = *message;
    VIDEO_RAM[offset + 1] = (uint8_t)color;
    

}

/**
 * Prints a byte as hex at position.
 * @param color Color.
 * @param row Row.
 * @param column Column.
 * @param number Byte.
 */
void printHexNumber(uint32_t color, uint32_t row, uint32_t column, uint8_t number)
{
    // Dan O'Malley

    uint8_t *vgaMemory = (uint8_t *)VIDEO_RAM;
    vgaMemory = vgaMemory + (160 * row);
    vgaMemory = vgaMemory + (2 * column);

    uint8_t lowNybble = number & 0x0F;
    uint8_t highNybble = number >> 4;

    if (highNybble < 10)
    {
        *vgaMemory++ = highNybble + 0x30;
        *vgaMemory++ = color;
    }
    else
    {
        highNybble = highNybble + 0x57;
        
        *vgaMemory++ = highNybble;
        *vgaMemory++ = color;
    }


    if (lowNybble < 10)
    {
        lowNybble = lowNybble + 0x30;
        
        *vgaMemory++ = lowNybble;
        *vgaMemory++ = color;
    }
    else
    {
        lowNybble = lowNybble + 0x57;
        
        *vgaMemory++ = lowNybble;
        *vgaMemory++ = color;
    }

}

// Struct and jump table written by Grok.
// 12/2025 with Grok v4.

typedef struct {
    const char* name;
    void* func;
} JumpTableEntry;

const JumpTableEntry lib_jump_table[] __attribute__((section(".jumptable"))) =
{
    {"wait", (void*)wait},
    {"strcmp", (void*)strcmp},
    {"strlen", (void*)strlen},
    {"strcpy", (void*)strcpy},
    {"strcpyRemoveNewline", (void*)strcpyRemoveNewline},
    {"bytecpy", (void*)bytecpy},
    {"reverseString", (void*)reverseString},
    {"power", (void*)power},
    {"itoa", (void*)itoa},
    {"itoaHex", (void*)itoaHex},
    {"hextoi", (void*)hextoi},
    {"itoIPAddressString", (void*)itoIPAddressString},
    {"ipAddressTointeger", (void*)ipAddressTointeger},
    {"atoi", (void*)atoi},
    {"ceiling", (void*)ceiling},
    {"malloc", (void*)malloc},
    {"free", (void*)free},
    {"freeAll", (void*)freeAll},
    {"kMalloc", (void*)kMalloc},
    {"kFree", (void*)kFree},
    {"kFreeAll", (void*)kFreeAll},
    {"stringHash", (void*)stringHash},
    {"makeSound", (void*)makeSound},
    {"systemUptime", (void*)systemUptime},
    {"systemExit", (void*)systemExit},
    {"systemFree", (void*)systemFree},
    {"systemMMap", (void*)systemMMap},
    {"systemSwitchToParent", (void*)systemSwitchToParent},
    {"systemShowProcesses", (void*)systemShowProcesses},
    {"systemListDirectory", (void*)systemListDirectory},
    {"systemSchedulerToggle", (void*)systemSchedulerToggle},
    {"systemExec", (void*)systemExec},
    {"systemKill", (void*)systemKill},
    {"systemTaskSwitch", (void*)systemTaskSwitch},
    {"systemShowMemory", (void*)systemShowMemory},
    {"systemShowElfHeader", (void*)systemShowElfHeader},
    {"systemShowDisassembly", (void*)systemShowDisassembly},
    {"systemShowOpenFiles", (void*)systemShowOpenFiles},
    {"systemShowGlobalObjects", (void*)systemShowGlobalObjects},
    {"systemShowInodeDetail", (void*)systemShowInodeDetail},
    {"systemOpenFile", (void*)systemOpenFile},
    {"systemOpenEmptyFile", (void*)systemOpenEmptyFile},
    {"systemCreateFile", (void*)systemCreateFile},
    {"systemDeleteFile", (void*)systemDeleteFile},
    {"systemCloseFile", (void*)systemCloseFile},
    {"octalTranslation", (void*)octalTranslation},
    {"directoryEntryTypeTranslation", (void*)directoryEntryTypeTranslation},
    {"countHeapObjects", (void*)countHeapObjects},
    {"countKernelHeapObjects", (void*)countKernelHeapObjects},
    {"convertToUnixTime", (void*)convertToUnixTime},
    {"convertFromUnixTime", (void*)convertFromUnixTime},
    {"systemCreatePipe", (void*)systemCreatePipe},
    {"systemCreateNetworkPipe", (void*)systemCreateNetworkPipe},
    {"systemOpenPipe", (void*)systemOpenPipe},
    {"systemWritePipe", (void*)systemWritePipe},
    {"systemReadPipe", (void*)systemReadPipe},
    {"systemNetSend", (void*)systemNetSend},
    {"systemNetRcv", (void*)systemNetRcv},
    {"systemDiskSectorViewer", (void*)systemDiskSectorViewer},
    {"systemBlockViewer", (void*)systemBlockViewer},
    {"systemKernelLogViewer", (void*)systemKernelLogViewer},
    {"waitForEnterOrQuit", (void*)waitForEnterOrQuit},
    {"strConcat", (void*)strConcat},
    {"removeExtension", (void*)removeExtension},
    {"subString", (void*)subString},
    {"isSpace", (void*)isSpace},
    {"skipWhitespace", (void*)skipWhitespace},
    {"toLower", (void*)toLower},
    {"isDigit", (void*)isDigit},
    {"isAlpha", (void*)isAlpha},
    {"systemChangeDirectory", (void*)systemChangeDirectory},
    {"systemMoveFile", (void*)systemMoveFile},
    {"rand", (void*)rand},
    {"scanf", (void*)scanf},
    {"print", (void*)printf},
    {"systemGetInodeStructureOfFile", (void*)systemGetInodeStructureOfFile},
    {"systemChangeFileMode", (void*)systemChangeFileMode},
    {"stringToOctal", (void*)stringToOctal},
    {"hasOwnerExecute", (void*)hasOwnerExecute},
    {"hasGroupExecute", (void*)hasGroupExecute},
    {"hasOtherExecute", (void*)hasOtherExecute},
    {"printString", (void*)printString},
    {"clearScreen", (void*)clearScreen},
    {"printCharacter", (void*)printCharacter},
    {"printHexNumber", (void*)printHexNumber},
    {NULL, NULL}
};

const int lib_jump_table_size __attribute__((section(".jumptable"))) = sizeof(lib_jump_table) / sizeof(JumpTableEntry) - 1;