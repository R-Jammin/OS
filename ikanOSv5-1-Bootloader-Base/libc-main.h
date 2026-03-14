// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"

/**
 * This is the user heap object.
 * It represents a fixed-size memory block allocated for user processes.
 * The structure includes a process ID to track ownership, the usable data area,
 * and a null terminator for safety with string operations.
 */
struct heapObjectUser
{
    /** 
     * The PID is stored and used as a non-zero indicator to allow the heap allocator 
     * to know if that heap object position is used. This is done for user and kernel 
     * heap objects alike. It helps in managing allocation and deallocation by marking 
     * the object as occupied when the PID is non-zero.
     */
    uint8_t pid; 
    /** 
     * The size of the heap object. If you need more use systemMMap() instead. 
     * This array holds the actual data that the user can utilize, with its size 
     * defined by the constant HEAP_OBJ_USABLE_SIZE to ensure consistent allocation.
     */
    uint8_t heapUseable[HEAP_OBJ_USABLE_SIZE];
    /** 
     * Always left null to accommodate string null bytes. 
     * This field is intentionally set to zero to prevent overflow issues 
     * when the usable area is treated as a string, providing a safety buffer.
     */
    uint8_t nullByte; 
};

/**
 * This is the kernel heap object. 
 * Similar to the user heap object but allocated in kernel space for kernel-level operations.
 * It includes a process ID for tracking, a larger usable data area, and a null terminator.
 */
struct heapObjectKernel
{
    /** 
     * The PID is stored and used as a non-zero indicator to allow the heap allocator 
     * to know if that heap object position is used. This is done for user and kernel 
     * heap objects alike. It ensures proper management of kernel memory allocations.
     */
    uint8_t pid; 
    /** 
     * The size of the heap object. If you need more use systemMMap() instead. 
     * This array provides the space for kernel data, sized by KERNEL_HEAP_OBJ_USABLE_SIZE 
     * to accommodate potentially larger kernel requirements.
     */
    uint8_t heapUseable[KERNEL_HEAP_OBJ_USABLE_SIZE];
    /** 
     * Always left null to accommodate string null bytes. 
     * This null byte acts as a terminator to safely handle string operations 
     * within the kernel heap, preventing buffer overflows.
     */
    uint8_t nullByte; 
};

/**
 * The time structure. This is used in the Unix epoch conversion for convertToUnixTime() and convertFromUnixTime().
 * It represents a specific point in time with fields for year, day, hour, minute, and second,
 * facilitating time calculations and conversions in the system.
 */
struct time
{
    /** 
     * The four digit year. 
     * This field stores the year in Gregorian calendar format, typically starting from 1970 for Unix time compatibility.
     */
    uint32_t year;
    /** 
     * The day of the year. 
     * This is the ordinal day number within the year, ranging from 1 to 365 (or 366 in leap years).
     */
    uint32_t dayOfYear; 
    /** 
     * The hour of the day in the 24-hour clock. 
     * This ranges from 0 to 23, representing the hour in a 24-hour format.
     */
    uint32_t hour;
    /** 
     * The minute. 
     * This field holds the minute of the hour, ranging from 0 to 59.
     */
    uint32_t min;
    /** 
     * The second. 
     * This stores the second of the minute, ranging from 0 to 59.
     */
    uint32_t sec;
};

/** 
 * Force the program into a busy loop for the number of interrupts in the parameter.
 * This function implements a simple delay mechanism by looping until the specified 
 * number of interrupts have occurred, useful for timing operations without precise timers.
 * \param timeToWait The number of interrupts to wait.
 */
void wait(uint32_t timeToWait);

/**
 * Compare two strings and returns a value. 
 * Returns < 0 if firstString < secondString;
 * Returns 0 if firstString == secondString;
 * Returns > 0 if secondString > firstString.
 * This function performs a lexicographical comparison of two null-terminated strings,
 * character by character, until a difference is found or the end is reached.
 * \param firstString The first string.
 * \param secondString Compare this string to the first string.
 */
uint32_t strcmp(uint8_t *firstString, uint8_t *secondString);

/**
 * Returns the length of the string.
 * This function counts the number of characters in a null-terminated string,
 * excluding the null terminator itself, providing the string's length.
 * \param targetString The string to measure.
 */
uint32_t strlen(uint8_t *targetString);

/**
 * Copy a source string to a destination string.
 * This function copies characters from the source to the destination until 
 * the null terminator is reached, assuming the destination has sufficient space.
 * \param destinationString The destination.
 * \param sourceString The source.
 */
void strcpy(uint8_t *destinationString, uint8_t *sourceString);

/**
 * Copies a string and stops when a newline character is detected.
 * This variant of strcpy halts copying upon encountering a newline (0xA),
 * useful for processing lines from text input without including the newline.
 * \param destinationString The destination.
 * \param sourceString The source.
 */
void strcpyRemoveNewline(uint8_t *destinationString, uint8_t *sourceString);

/**
 * Copy bytes from one location to another in memory.
 * This function performs a raw byte copy from source to destination for a specified length,
 * without regard to null terminators, suitable for binary data or non-string buffers.
 * \param destinationMemory The target location in memory.
 * \param sourceMemory The source location in memory.
 * \param numberOfBytes How many bytes to copy from source to target.
 */
void bytecpy(uint8_t *destinationMemory, uint8_t *sourceMemory, uint32_t numberOfBytes);

/**
 * Reverses a string in-place.
 * This function swaps characters from the start and end of the string, moving inward,
 * effectively reversing the order without needing additional memory.
 * \param targetString The string to reverse.
 */
void reverseString(uint8_t *targetString);

/**
 * Computes the power and return the number.
 * This function calculates number raised to the exponent using repeated multiplication,
 * handling only non-negative integer exponents.
 * \param number The base number to raise to a certain exponent.
 * \param exponent The exponent.
 */
uint32_t power(uint32_t number, uint32_t exponent);

/**
 * Convert an integer to an ASCII-equivalent string value of that integer.
 * This function generates digits in reverse order and then reverses the string,
 * handling negative numbers by prepending a minus sign.
 * \param number The integer to convert.
 * \param destinationMemory The target location for the string-converted integer (you may want to call malloc first to allocate space).
 */
void itoa(uint32_t number, uint8_t *destinationMemory);

/**
 * Convert an ASCII string of a number to an integer. The opposite of itoa(). Returns the integer.
 * This function parses the string, skipping whitespace, handling signs,
 * and accumulating the numeric value from digits.
 * \param sourceString The string to convert to an integer.
 */
uint32_t atoi(uint8_t *sourceString);

/**
 * Compute the ceiling of a number given some base. Returns ceiling value.
 * This function performs integer division rounded up, useful for allocation calculations
 * where partial units need to be counted as full.
 * \param number The input number.
 * \param base The base.
 */
uint32_t ceiling(uint32_t number, uint32_t base);

/**
 * Allocate a user heap object.
 * This function searches for an available heap slot marked by zero PID,
 * assigns the current PID, and returns a pointer to the usable area.
 * \param currentPid The current pid. This is needed to show whether or not the heap object is allocated even though all objects in the same PID will have the same value.
 * \param objectSize The size of the object requested. If this requested size is larger than the heap object, a null pointer is returned. Be sure to check for that. If you need a larger object, use systemMMap().
 */
uint8_t *malloc(uint32_t currentPid, uint32_t objectSize);

/**
 * Free a heap object. See malloc().
 * This function zeros out the entire heap object including the PID,
 * marking it as available for future allocations.
 * \param heapObject The pointer to the heap object you want to free.
 */
void free(uint8_t *heapObject);

/**
 * Free all heap objects for a certain PID. Be very careful with this as it can lead to many memory problems, such as use-after-free (UAF) bugs.
 * This function scans the entire user heap and frees any objects owned by the specified PID
 * by zeroing them out, useful during process cleanup.
 * \param currentPid The current PID.
 */
void freeAll(uint32_t currentPid);

/**
 * The kernel equivalent of malloc(). See kFree() also. The object size and parameters are the same as the userspace heap but the objects exist in kernel space and are for kernel use. Returns a pointer in kernel space if successful.
 * This function allocates from the kernel heap pool, assigning the PID for tracking.
 * \param currentPid The pid associated with the object. This is important as you want to free all kernel heap objects associated with a killed/zombie/exited process.
 * \param objectSize The requested size of the heap object. Same as malloc().
 */
uint8_t *kMalloc(uint32_t currentPid, uint32_t objectSize);

/**
 * The kernel equivalent of free(). See kMalloc().
 * This function zeros the kernel heap object to release it back to the pool.
 * \param heapObject The pointer to the heap object you want to free in kernel space.
 */
void kFree(uint8_t *heapObject);

/**
 * Free all kernel heap objects for a certain PID.
 * Similar to freeAll but for kernel heap, scanning and zeroing matching objects.
 * \param currentPid The PID associated with kernel heap objects you want to free.
 */
void kFreeAll(uint32_t currentPid);

/**
 * Generates a simple hash based on an input string. Not cryptographically secure.
 * This function sums the ASCII values of characters in the string for a basic hash.
 * \param messageToHash The input string.
 */
uint32_t stringHash(uint8_t *messagetoHash);

/**
 * Plays a sound at a certain frequency for a certain duration.
 * This function uses system calls to generate audio output via the PC speaker.
 * \param frequency Frequency in Hz.
 * \param duration Duration in number of interrupts.
 */
void makeSound(uint32_t frequency, uint32_t duration);

/**
 * The LibC wrapper for the SYS_UPTIME sysCall(). It displays the system uptime.
 * This function invokes the kernel to print the elapsed time since boot.
 */
void systemUptime();

/**
 * The LibC wrapper for the SYS_EXIT sysCall(). This terminates the current process and returns to the parent process.
 * This function signals the kernel to end the process with the given status.
 * \param exitCode The exit code for this process.
 */
void systemExit(uint32_t exitCode);

/**
 * The LibC wrapper for the SYS_FREE sysCall(). This frees all data structures and memory associated with killed/zombie processes.
 * This function cleans up resources from terminated processes.
 */
void systemFree();

/**
 * The LibC wrapper for the SYS_MMAP sysCall(). This function will return a pointer (if successful) to a new and available page of user space memory somewhere in the address space of the process. Uses first-fit algorithm.
 * This function requests a memory mapping from the kernel for dynamic allocation.
 */
uint8_t *systemMMap(uint8_t* requestedPage);

/**
 * The LibC wrapper for the SYS_SWITCH_TASK_TO_PARENT sysCall(). This will switch to the parent, putting the child to sleep without terminating the child process.
 * This function yields control back to the parent process.
 */
void systemSwitchToParent();

/**
 * The LibC wrapper for the SYS_PS sysCall(). This will display the informational screen about processes, objects, memory, etc.
 * This function shows process status similar to the 'ps' command.
 */
void systemShowProcesses();

/**
 * The LibC wrapper for the SYS_DIR sysCall(). This will display the contents of the current directory.
 * This function lists files and directories in the current working directory.
 */
void systemListDirectory();

/**
 * The LibC wrapper for the SYS_TOGGLE_SCHEDULER sysCall(). This will turn on the scheduler/disbatch functionality if it is off, or it will turn it off if it is on.
 * This function enables or disables the task scheduler.
 */
void systemSchedulerToggle();

/**
 * The Libc wrapper for the SYS_EXEC sysCall(). This will take a file name from the file system and a requested run priority and launch it.
 * This function starts a new process from an executable file, setting up stdio as specified.
 * \param fileName The string of a file name on the file system.
 * \param requestedRunPriority An 8-bit value to assign as the priority of the process. If left blank, the priority will be zero.
 * \param requestedStdIn The file descriptor from the parent requested for the child STDIN, if zero, it defaults.
 * \param requestedStdOut The file descriptor from the parent requested for the child STDOUT, if zero, it defaults.
 * \param requestedStdErr The file descriptor from the parent requested for the child STDERR, if zero, it defaults.
 */
void systemExec(uint8_t *fileName, uint32_t requestedRunPriority, uint32_t requestedStdIn, uint32_t requestedStdOut, uint32_t requestedStdErr);

/**
 * The LibC wrapper for the SYS_KILL sysCall(). This will kill a process. You cannot kill yourself and you cannot kill PID=1.
 * This function terminates a specified process.
 * \param pidToKill The PID to kill.
 */
void systemKill(uint8_t *pidToKill);

/**
 * The LibC wrapper for the SYS_SWITCH_TASK sysCall(). This will switch to a certain process and put the current process to sleep.
 * This function context switches to another process.
 * \param pidToSwitchTo The PID to switch to.
 */
void systemTaskSwitch(uint8_t *pidToSwitchTo);

/**
 * The LibC wrapper for the SYS_MEM_DUMP sysCall(). Takes the memory location in decimal and displays it.
 * This function dumps memory contents starting from the given address.
 * \param memoryLocation Takes a memory location in decimal.
 */
void systemShowMemory(uint8_t *memoryLocation);

/**
 * The LibC wrapper for the SYS_SHOW_OPEN_FILES sysCall(). This will display the open files for the current process to the screen, along with their permissions.
 * This function lists open file descriptors and their details.
 * \param startDisplayAtRow The row to start printing the open files
 */
void systemShowOpenFiles(uint32_t startDisplayAtRow);

/**
 * The LibC wrapper for the SYS_CREATE sysCall(). This will create a new file on the file system based on the contents of a file descriptor buffer.
 * This function creates a file using data from an open FD.
 */
void systemCreateFile(uint8_t *fileName, uint32_t fileDescriptor);

/**
 * The LibC wrapper for the SYS_DELETE sysCall(). This will delete a file on the file system based on file name.
 * This function removes a file from the filesystem.
 */
void systemDeleteFile(uint8_t *fileName);

/**
 * The LibC wrapper for the SYS_OPEN sysCall(). This will open a filename on the file system with the requested permissions.
 * This function opens a file and returns its descriptor.
 * \param fileName The string value of the filename on the file system.
 * \param requestPermissions The permissions are constants in constants.h. They are typically RDWRITE for read/write or RDONLY for read only.
 */
uint32_t systemOpenFile(uint8_t *fileName, uint32_t requestedPermissions);

/**
 * The LibC wrapper for the SYS_OPEN_EMPTY sysCall(). This will open a buffer of the requested size in pages. Used for created brand new text files.
 * This function allocates an empty buffer as a file.
 * \param fileName The string value of the filename for the open file table.
 * \param requestSizeInPages The size of the buffer in pages.
 */
void systemOpenEmptyFile(uint8_t *fileName, uint32_t requestedSizeInPages);

/**
 * The LibC wrapper for the SYS_CLOSE sysCall(). This will close a file given a file descriptor number.
 * This function closes an open file descriptor.
 * \param fileDescriptor The file descriptor to close.
 */
void systemCloseFile(uint32_t fileDescriptor);

/**
 * Converts octal permissions in EXT2 to RWX string.
 * This function translates permission bits to human-readable format.
 * \param permissions The raw byte octal permissions.
 */
uint8_t *octalTranslation(uint8_t permissions);

/**
 * Converts directory entry types (e.g., file, directories, etc) to their string value.
 * This function maps type codes to descriptive strings.
 * \param type The raw type of the directory entry.
 */
uint8_t *directoryEntryTypeTranslation(uint8_t type);

/**
 * Counts the number of active heap objects. Returns the count.
 * This function iterates over the heap to count occupied slots.
 * \param heapLoc The pointer to the heap location.
 */
uint32_t countHeapObjects(uint8_t *heapLoc);

/** 
 * Returns the Unix Epoch value (seconds since 1970) given a time struct.
 * This function computes seconds from the time components, accounting for leap years.
 * \param Time The time struct.
 */
uint32_t convertToUnixTime(struct time* Time);

/**
 * Given the Unix Epoch value (seconds since 1970), return the time struct.
 * This function breaks down seconds into year, day, hour, etc.
 * \param unixTime Seconds since 1970.
 */
void convertFromUnixTime(uint32_t unixTime, struct time *Time);

/**
 * Creates a named pipe for IPC.
 * This function sets up a FIFO for process communication.
 */
void systemCreatePipe(uint8_t *pipeName);

/**
 * Opens a named pipe and returns its FD.
 * This function connects to an existing pipe.
 */
uint32_t systemOpenPipe(uint8_t *pipeName);

/**
 * Writes data to a pipe via FD.
 * This function sends data through the pipe.
 */
void systemWritePipe(uint8_t *fileDescriptor);

/**
 * Reads data from a pipe via FD.
 * This function receives data from the pipe.
 */
void systemReadPipe(uint8_t *fileDescriptor);

/**
 * Sends a network packet with IP and port info.
 * This function transmits data over the network.
 */
void systemNetSend(uint32_t sourceIPAddress, uint32_t destinationIPAddress, uint16_t sourcePort, uint16_t destinationPort, uint8_t *packetPayload);

/**
 * Receives network data.
 * This function listens for incoming packets.
 */
void systemNetRcv();

/**
 * Displays global system objects.
 * This function shows shared resources or objects.
 */
void systemShowGlobalObjects();

/**
 * Converts number to hex string.
 * This function generates hex digits in reverse and reverses them.
 */
void itoaHex(uint32_t number, uint8_t *destinationMemory);

/**
 * Converts hex string to integer.
 * This function parses hex digits, handling 0x prefix.
 */
uint32_t hextoi(uint8_t *hexString);

/**
 * Converts IP integer to dotted string.
 * This function extracts octets and formats them.
 */
void itoIPAddressString(uint32_t ipAddress, uint8_t *destinationMemory);

/**
 * Converts dotted IP string to integer.
 * This function parses octets and combines them.
 */
uint32_t ipAddressTointeger(uint8_t *ipAddress);

/**
 * Counts active kernel heap objects.
 * This function scans kernel heap for occupied slots.
 */
uint32_t countKernelHeapObjects(uint8_t *heapLoc);

/**
 * Creates a network socket with addresses and ports.
 * This function sets up a network connection pipe.
 */
void systemCreateNetworkPipe(uint8_t *socketName, uint32_t sourceIPAddress, uint32_t destinationIPAddress, uint16_t sourcePort, uint16_t destinationPort);

/**
 * Disassembles code from memory location.
 * This function shows assembly instructions.
 */
void systemShowDisassembly(uint8_t *memoryLocation);

/**
 * Waits for enter or quit key.
 * This function polls keyboard for specific inputs.
 */
uint32_t waitForEnterOrQuit();

/**
 * Views a disk sector.
 * This function displays raw sector data.
 */
void systemDiskSectorViewer(uint32_t diskSectorToView);

/**
 * Views a FS block.
 * This function shows block contents.
 */
void systemBlockViewer(uint32_t blockToView);

/**
 * Concatenates two strings.
 * This function allocates and combines strings.
 */
uint8_t* strConcat(uint8_t* first, uint8_t* second);

/**
 * Removes file extension.
 * This function truncates at last dot.
 */
uint8_t* removeExtension(uint8_t* filename) ;

/**
 * Gets substring removing from right.
 * This function shortens string by count.
 */
uint8_t* subString(uint8_t* original, uint32_t charsToRemoveFromRight);

/**
 * Checks if char is space.
 * This function identifies whitespace chars.
 */
int isSpace(uint8_t character);

/**
 * Skips whitespace in string.
 * This function advances pointer past spaces.
 */
uint8_t *skipWhitespace(uint8_t *stringPointer);

/**
 * Shows ELF header from memory.
 * This function displays ELF file metadata.
 */
void systemShowElfHeader(uint8_t *memoryLocation);

/**
 * Shows inode details.
 * This function prints FS inode info.
 */
void systemShowInodeDetail(uint32_t inodeNumber);

/**
 * Views kernel log.
 * This function displays system logs.
 */
void systemKernelLogViewer();

/**
 * Converts to lowercase.
 * This function lowers uppercase letters.
 */
unsigned char toLower(unsigned char character);

/**
 * Checks if digit.
 * This function verifies 0-9 range.
 */
int isDigit(unsigned char character);

/**
 * Checks if alpha.
 * This function checks a-z A-Z.
 */
int isAlpha(unsigned char character);

/**
 * Changes current directory.
 * This function updates working dir.
 */
void systemChangeDirectory(uint8_t *fileName);

/**
 * Moves file between dirs.
 * This function relocates files.
 */
void systemMoveFile(uint8_t *fileName, uint8_t *sourceDirectory, uint8_t *destinationDirectory);

/** 
 * Function to generate a pseudo-random number using a linear congruential generator.
 * This uses interrupt count as seed for randomness.
 */
uint32_t rand();

/**
 * Formatted print to screen.
 * This supports %d %x %s %c.
 */
void printf(uint32_t color, uint32_t row, uint32_t column, uint8_t *format, ...);

/**
 * Gets ASCII from scan code.
 * This maps keyboard codes to chars.
 */
uint8_t getAsciiFromScanCode(uint8_t scanCode, uint8_t shift_down);

/**
 * Formatted input from keyboard.
 * This supports %d %x %s %c.
 */
uint32_t scanf(uint32_t color, uint32_t row, uint32_t column, uint8_t *format, ...);

/**
 * Gets file inode structure.
 * This retrieves FS metadata.
 */
void systemGetInodeStructureOfFile(uint8_t *fileName);

/**
 * Changes file permissions.
 * This sets mode from string.
 */
void systemChangeFileMode(uint8_t *fileName, uint8_t *requestMode);

/**
 * String mode to octal.
 * This converts -rwxr-xr-x to number.
 */
uint16_t stringToOctal(uint8_t *modeString);

/**
 * Checks owner execute bit.
 * This tests permission flag.
 */
bool hasOwnerExecute(uint16_t mode);

/**
 * Checks group execute bit.
 * This tests permission flag.
 */
bool hasGroupExecute(uint16_t mode);

/**
 * Checks other execute bit.
 * This tests permission flag.
 */
bool hasOtherExecute(uint16_t mode);

/**
 * Prints string to screen.
 * This handles position and color.
 */
void printString(uint32_t color, uint32_t row, uint32_t column, uint8_t *message);

/**
 * Clears screen.
 * This zeros video memory.
 */
void clearScreen();

/**
 * Prints char to screen.
 * This handles special chars.
 */
void printCharacter(uint32_t color, uint32_t row, uint32_t column, uint8_t *message);

/**
 * Prints hex byte.
 * This shows two hex digits.
 */
void printHexNumber(uint32_t color, uint32_t row, uint32_t column, uint8_t number);