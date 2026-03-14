// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"
/**
 * The file parameter structure. This is used to pass file-related information in a sysCall().
 */
struct fileParameter
{
    /** The requested permissions. Can be RDWRITE, RDONLY, etc. */
    uint32_t requestedPermissions;
    /** The requested run priority. This is an 8-bit value. If left blank, the value will be zero. */
    uint32_t requestedRunPriority; 
    /** The file descriptor associated with the file. This is needed when creating a new file on the file system. Blank otherwise. */
    uint32_t fileDescriptor;
    /** The requested size of a new, empty file. The size is in pages. */
    uint32_t requestedSizeInPages;
    /** The requested FD to place in STDIN during SYS_EXEC. */
    uint32_t requestedStdInFd;
    /** The requested FD to place in STDOUT during SYS_EXEC. */
    uint32_t requestedStdOutFd;
    /** The requested FD to place in STDERR during SYS_EXEC. */
    uint32_t requestedStdErrFd;
    /** The source directory inode for file moves */
    uint8_t *sourceDirectory;
    /** The destination directory inode for file moves */
    uint8_t *destinationDirectory;
    /** The requested mode for this file (during a chmod) */
    uint16_t requestedFileMode;
    /** The length of the filename string. */
    uint32_t fileNameLength; 
    /** The name of the file. */
    uint8_t *fileName;
};

/**
 * The global object table entry. This is used to track open objects in the kernel.
 */
struct globalObjectTableEntry
{
    /** The PID that opened this object. */
    uint32_t openedByPid; 
    /** The inode of the open object. */
    uint32_t inode;
    /** Is the open object a buffer, file, pipe, socket, ? */
    uint32_t type;
    /** The source IP address. */
    uint32_t sourceIPAddress;
    /** The destination IP address. */
    uint32_t destinationIPAddress; 
    /** The source port. */
    uint16_t sourcePort;
    /** The destination port. */
    uint16_t destinationPort;
    /** What is the size of the buffer, file, pipe, etc */
    uint32_t size;
    /** A pointer to the user space buffer where the object begins. */
    uint8_t *userspaceBuffer; 
    /** The number of contiguous pages used for the object buffer, starting at the userspaceBuffer address. */
    uint32_t numberOfPagesForBuffer; 
    /** A pointer to the kernel space buffer, if any. */
    uint8_t *kernelSpaceBuffer; 
    /** The name. */
    uint8_t *name; 
    /** The current byte offset into the buffer, used for modification and the editor. */
    uint32_t readOffset;
    /** The current byte offset into the buffer, used for modification and the editor. */
    uint32_t writeOffset;
    /** Is this file locked for writing (i.e., opened with RDWRITE permissions). The value is the PID that has it locked */
    uint32_t lockedForWriting; 
    /** This tracks how many packets have been sent via this socket */
    uint32_t packetsTransmitted; 
    /** This tracks how many packets have been received via this socket */
    uint32_t packetsReceived; 
    /** This tracks how many refernces to this object */
    uint32_t referenceCount; 
};

struct openBufferTable
{
    uint8_t *buffers[MAX_FILE_DESCRIPTORS];
    uint32_t bufferSize[MAX_FILE_DESCRIPTORS];
};

/**
 * Creates the open file table data structure at a particular address in kernel space.
 * \param openFileTable The pointer to the memory location to start building the open file table data structure.
 */
void createOpenFileTable(uint8_t *openFileTable);

/**
 * Creates an entry in the open file table data structure. Returns an GlobalObjectTableEntry pointer to the entry.
 * \param openFileTable The pointer to the beginning of the open file table data structure.
 * \param openedByPid The PID of the process that opened the file.
 * \param inode The inode of the open file.
 * \param type The type. Is it buffer, file, pipe, socket, etc.
 * \param sourceIPAddress The source IP address of the socket.
 * \param destinationIPAddress The destination IP address of the socket.
 * \param sourcePort The source port of the socket.
 * \param destinationPort The destination port of the socket.
 * \param size The size of the pipe or socket.
 * \param userspaceBuffer The pointer to the beginning of the contiguous buffer holding the file information.
 * \param numberOfPagesForBuffer The number of contiguous pages that form the buffer.
 * \param kernelSpaceBuffer The pointer to the beginning of the contiguous buffer in kernel space.
 * \param name The name..
 * \param readOffset The current offset into the file, used in the editor.
 * \param writeOffset The current offset into the file, used in the editor.
 * \param lockedForWriting The indicator as to who, if anyone has it locked. 0 = no lock, !=0 is the PID who has it locked.
 */
globalObjectTableEntry *insertGlobalObjectTableEntry(uint8_t *openFileTable, uint32_t openedByPid, uint32_t inode, uint32_t type, uint32_t sourceIPAddress, uint32_t destinationIPAddress, uint16_t sourcePort, uint16_t destinationPort, uint32_t size, uint8_t *userspaceBuffer, uint32_t numberOfPagesForBuffer, uint8_t *kernelSpaceBuffer,  uint8_t *name, uint32_t readOffset, uint32_t writeOffset, uint32_t lockedForWriting);

/**
 * Returns the total number of open files in the open file table data structure.
 * \param openFileTable The pointer to the open file table data structure.
 */
uint32_t totalOpenFiles(uint8_t *openFileTable);

/**
 * Closes all files for a particular PID in the open file table data structure.
 * \param openFileTable The pointer to the open file table data structure.
 * \param pid The PID associated with closure of the files.
 */
void closeAllFiles(uint8_t *openFileTable, uint32_t pid);

/**
 * For a particular inode, return a bool as to whether it can be locked (RDWRITE) or not.
 * \param openFileTable The pointer to the open file table data structure.
 * \param inode The inode of the file in question.
 */
bool fileAvailableToBeLocked(uint8_t *openFileTable, uint32_t inode);

/**
 * Lock a file for RDWRITE access in the open file table data structure.
 * \param openFileTable The pointer to the open file table data structure. 
 * \param pid The PID that is locking the file.
 * \param inode The inode of the file being locked.
 */
bool lockFile(uint8_t *openFileTable, uint32_t pid, uint32_t inode);