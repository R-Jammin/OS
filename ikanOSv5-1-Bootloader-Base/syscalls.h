// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"
/**
 * The main system call handler routine that runs in the kernel during each system call interrupt.
 */
void syscallHandler();

/**
 * The kernel routine that executes a sound.
 * \param SoundParameter It takes the sound parameter structure pointer as the argument.
 */
void sysSound(struct soundParameter *SoundParameter);

/** The kernel routine that prints a file descriptor to the screen.
 * \param fileDescriptorToPrint The file descriptor to print to the screen.
 * \param currentPid The pid of the process requesting this action.
 */
void sysWrite(uint32_t fileDescriptorToPrint, uint32_t currentPid);

/** The kernel routine to open a file.
 * \param FileParameter The file parameter structure with the file specifics.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode The current working directory inode.
 */
uint32_t sysOpen(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine to close a file.
 * \param fileDescriptor The file descriptor to close.
 * \param currentPid The pid of the process requesting this action.
 */
void sysClose(uint32_t fileDescriptor, uint32_t currentPid);

/** The kernel routine that prints the current processes to the screen.
 * \param currentPid The pid of the process requesting this action.
 */
void sysPs(uint32_t currentPid);

/** The kernel routine that loads and runs a new program. 
 * \param FileParameter The file parameter structure with the file specifics.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode The working directory inode.
*/
void sysExec(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine that exits the current process.
 * \param currentPid The pid of the process requesting this action.
 * \param exitCode The exit code for this process.
 */
void sysExit(uint32_t currentPid, uint32_t exitCode);

/** The kernel routine that frees memory and frames for killed or zombie processes.
 * \param currentPid The pid of the process requesting this action.
 */
void sysFree(uint32_t currentPid);

/** The kernel routine that kills a process.
 * \param pidToKill The pid to kill.
 */
void sysKill(uint32_t pidToKill);

/** The kernel routine to switch to another process and puts the current process to sleep.
 * \param pidToSwitchTo The pid to switch to.
 * \param currentPid The pid of the process requesting this action.
 */
void sysSwitch(uint32_t pidToSwitchTo, uint32_t currentPid);

/** Display the contents of a memory address to the screen.
 * \param memAddressToDump The memory address to display.
 */
void sysMemDump(uint32_t memAddressToDump);

/** The main system interrupt handler. This function is called each time an interrupt is triggered. */
void systemInterruptHandler();

/** Prints the the uptime to the screen.
 * \param uptimeSeconds The uptime in seconds.
 */
void printSystemUptime(uint32_t uptimeSeconds);

/** The kernel routine wrapper for printSystemUptime() */
void sysUptime();

/** The kernel routine that switches to the parent of the running process. Puts the child to sleep.
 * \param currentPid The pid of the process requesting this action.
 */
void sysSwitchToParent(uint32_t currentPid);

/** The kernel routine that waits one second. This is computed based on number of interrupts per second. */
void sysWait();

/** The kernel routine that waits for one interrupt. The length of this action varies based on the system's interrupts per second. */
void sysWaitOneInterrupt();

/** The kernel routine that prints the directory contents to the screen.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode The current working directory
 */
void sysDirectory(uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine that toggles the schedule. This is done by modifying the kernelConfiguration structure. */
void sysToggleScheduler();

/** The kernel routine that shows the open files for that process.
 * \param startDisplayAtRow The row to start the printing of the open files
 * \param currentPid The pid of the process requesting this action.
 */
void sysShowOpenFiles(uint32_t startDisplayAtRow, uint32_t currentPid);

/** The kernel routine that creates a new file.
 * \param FileParameter The file parameter structure with the file specifics.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode Current working directory.
 */
void sysCreate(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine that deletes a file.
 * \param FileParameter The file parameter structure with the file specifics.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode The current working directory.
 */
void sysDelete(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine that opens a new and empty file descriptor.
 * \param FileParameter The file parameter structure with the file specifics.
 * \param currentPid The pid of the process requesting this action. 
 * \param directoryInode Current working directory.
 */ 
void sysOpenEmpty(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine that creates a pipe.
 * \param FileParameter The file parameter structure with the pipe specifics.
 * \param currentPid The pid of the process requesting this action.
 */
void sysCreatePipe(struct fileParameter *FileParameter, uint32_t currentPid);

/** The kernel routine that opens an existing pipe.
 * \param FileParameter The file parameter structure with the pipe specifics.
 * \param currentPid The pid of the process requesting this action.
 */
uint32_t sysOpenPipe(struct fileParameter *FileParameter, uint32_t currentPid);

/** The kernel routine that reads from a file descriptor.
 * \param fileDescriptorToRead The file descriptor to read from.
 * \param currentPid The pid of the process requesting this action.
 */
void sysRead(uint32_t fileDescriptorToRead, uint32_t currentPid);

/** The kernel routine that displays the global object table entries and open socket details.
 * \param currentPid The pid of the process requesting this action.
 */
void sysShowGlobalObjects(uint32_t currentPid);

/** The kernel routine that creates a network pipe (socket).
 * \param NetworkParameter The network parameter structure with the socket specifics.
 * \param currentPid The pid of the process requesting this action.
 */
void sysCreateNetworkPipe(struct networkParameter *NetworkParameter, uint32_t currentPid);

/** The kernel routine that maps a requested page for the process.
 * \param requestedPage The page to map.
 * \param currentPid The pid of the process requesting this action.
 */
void sysMmap(uint8_t* requestedPage, uint32_t currentPid);

/** The kernel routine that returns the current process ID.
 */
uint32_t sysWhatPidAmI();

/** The kernel routine that disassembles memory at a given address.
 * \param memAddressToDisasm The memory address to disassemble.
 */
void sysDisassemble(uint32_t memAddressToDisasm);

/** The kernel routine that dumps a disk sector.
 * \param sectorToDump The sector number to dump.
 */
void sysDiskSectorDump(uint32_t sectorToDump);

/** The kernel routine that dumps a disk block.
 * \param blockToDump The block number to dump.
 */
void sysDiskBlockDump(uint32_t blockToDump);

/** The kernel routine that dumps an ELF header.
 * \param elfHeaderLocation The location of the ELF header.
 */
void sysElfHeaderDump(uint8_t *elfHeaderLocation);

/** The kernel routine that dumps an inode.
 * \param inodeNumber The inode number to dump.
 * \param currentPid The pid of the process requesting this action.
 */
void sysInodeDump(uint32_t inodeNumber, uint32_t currentPid);

/** The kernel routine that inserts an entry into the kernel log.
 * \param interruptCount The interrupt count at the time of logging.
 * \param pid The pid associated with the log entry.
 * \param type The type of the log entry.
 * \param value A value associated with the log entry.
 * \param description A description for the log entry.
 */
void insertKernelLog(uint32_t interruptCount, uint32_t pid, uint8_t *type, uint32_t value, uint8_t *description);

/** The kernel routine that prints the kernel log.
 * \param currentPid The pid of the process requesting this action.
 */
void printKernelLog(uint32_t currentPid);

/** The kernel routine that changes the current directory.
 * \param FileParameter The file parameter structure with the directory specifics.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode The current working directory inode.
 */
void sysChangeDirectory(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine that moves or renames a file.
 * \param FileParameter The file parameter structure with the move specifics.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode The current working directory inode.
 */
void sysMove(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine that retrieves the latest example of a given type.
 * \param typeToFind The type to search for.
 */
uint32_t getLatestTypeExample(uint8_t* typeToFind);

/** The kernel routine that gets an inode structure for the user.
 * \param FileParameter The file parameter structure with the file specifics.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode The current working directory inode.
 */
void sysGetInodeForUser(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);

/** The kernel routine that changes the mode (permissions) of a file.
 * \param FileParameter The file parameter structure with the mode specifics.
 * \param currentPid The pid of the process requesting this action.
 * \param directoryInode The current working directory inode.
 */
void sysChangeFileMode(struct fileParameter *FileParameter, uint32_t currentPid, uint32_t directoryInode);