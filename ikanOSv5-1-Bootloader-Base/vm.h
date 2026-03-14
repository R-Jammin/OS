// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"

/**
 * The task structure. This holds all of the process-specific information to successfully context switch.
 */
struct task {
    uint32_t pid;
    uint32_t ppid;
    uint32_t uid;
    uint32_t state;
    uint32_t exitCode;
    uint32_t pgd;
    uint32_t stack;
    uint32_t priority;
    uint32_t nice;
    uint32_t runtime;
    uint32_t sleepTime;
    uint32_t recentRuntime;
    uint32_t waitChannel;
    uint32_t signal;
    uint32_t physMemStart;
    uint32_t nextAvailableFileDescriptor;
    uint32_t eip;              
    uint32_t eflags;         
    uint32_t eax;              
    uint32_t ecx;           
    uint32_t edx;            
    uint32_t ebx;             
    uint32_t esp;              
    uint32_t ebp;          
    uint32_t esi;            
    uint32_t edi;        
    uint16_t es;                
    uint16_t cs;                
    uint16_t ss;              
    uint16_t ds;             
    uint16_t fs;               
    uint16_t gs;             
    uint32_t ldt;            
    uint16_t trap;              
    uint16_t iomap_base;   
    uint32_t currentDirectoryInode;    
	void *fileDescriptor[MAX_FILE_DESCRIPTORS];
    uint8_t *binaryName;
    // This struct is 192 bytes
};

/** The structure of the semaphore. */
struct semaphore
{
    uint32_t pid;
    uint8_t *memoryLocationToLock;
    uint32_t currentValue;
    uint32_t maxValue;
};

/** Fills memory with a certain byte value.
 * \param memLocation The pointer to start the fill.
 * \param byteToFill The byte value used to fill.
 * \param numberOfBytes The number of bytes to fill starting at memLocation.
 */
void fillMemory(uint8_t *memLocation, uint8_t byteToFill, uint32_t numberOfBytes);

/** Creates the page directory and single page table structures for a particular pid.
 * \param pid The pid associated with the page directory and table you'd like to create.
 */
void initializePageTables(uint32_t pid);

/** Performs a context switch to the given pid.
 * \param pid The pid you'd like to switch to.
 */
void contextSwitch(uint32_t pid);

/** Creates the task structure values for a new process.
 * \param ppid The parent pid of the new process.
 * \param state The state value of the new process.
 * \param stack The stack location for the new process.
 * \param binaryName The string value of the process name.
 * \param priority The running priority of the new process.
 * \param directoryInode The current working directory.
 * \param requestedStdIn The GOT pointer you want for STDIN when initializing.
 * \param requestedStdOut The GOT pointer you want for STDOUT when initializing.
 * \param requestedStdErr The GOT pointer you want for STDIN when initializing.
 */
uint32_t initializeTask(uint32_t ppid, uint16_t state, uint32_t stack, uint8_t *binaryName, uint32_t priority, uint32_t directoryInode, uint32_t requestedStdIn, uint32_t requestedStdOut, uint32_t requestedStdErr);

/** Updates a pid's state.
 * \param pid The pid you'd like to update.
 * \param state The new state value.
 */
void updateTaskState(uint32_t pid, uint16_t state);

/** Requests allocation of a particular page in a pid's address space. Returns 0 if unsuccessful. Returns 1 if successful. 
 * \param pid The pid associated with the process space you are interested in.
 * \param pageMemoryLocation The virtual address you are interested in allocating.
 * \param perms The requested permissions of the new page.
*/
uint32_t requestSpecificPage(uint32_t pid, uint8_t *pageMemoryLocation, uint8_t perms);

/** Requests allocation of any available page in a pid's address space. Returns pointer to the allocated page if successful. 
 * \param pid The pid associated with the process space you are interested in.
 * \param perms The requested permissions of the new page.
*/
uint8_t *requestAvailablePage(uint32_t pid, uint8_t perms);

/** Finds an open buffer using a first-fit algorithm for the number of pages requested in the pid's address space.
 * \param pid The pid you are interested in.
 * \param numberOfPages The number of contiguous pages you are looking for.
 * \param perms The requested permissions of the new pages.
 */
uint8_t *findBuffer(uint32_t pid, uint32_t numberOfPages, uint8_t perms);

/** Frees a particular page in a pid's address space.
 * \param pid The pid you are interested in.
 * \param pageToFree The virtual address of the page you want to free.
 */
void freePage(uint32_t pid, uint8_t *pageToFree);

/** Used to acquire mutual exclusivity to a data structure.
 * \param currentPid The pid requesting the action.
 * \param memoryLocation The memory location you want to secure, stored in KERNEL_SEMAPHORE_TABLE
 */
bool acquireLock(uint32_t currentPid, uint8_t *memoryLocation);

/** Used to release mutual exclusivity to a data structure. This assumes a semaphore has already been created for the data structure you wish to control. 
 * \param currentPid The pid requesting the action.
 * \param memoryLocation The memory location you want to release, stored in KERNEL_SEMAPHORE_TABLE
 */
bool releaseLock(uint32_t currentPid, uint8_t *memoryLocation);

/** Creates a semaphore to protect some memory location.
 * \param currentPid The pid requesting the action.
 * \param memoryLocation The memory address you want to protect by creating a semaphore.
 * \param currentValue The initial/current value of the semaphore.
 * \param maxValue The max value of the semaphore. If you want mutual exclusivity, set this to 1.
 */
bool createSemaphore(uint32_t currentPid, uint8_t *memoryLocation, uint32_t currentValue, uint32_t maxValue);