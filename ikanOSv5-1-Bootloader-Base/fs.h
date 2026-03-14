// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Text descriptions generated with Grok by xAI.
// 12/2025 with Grok v4.


// ELF header and Program Headers modified from:
// https://refspecs.linuxfoundation.org/elf/gabi4+/ch4.eheader.html
// https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.pheader.html
//
// https://www.nongnu.org/ext2-doc/ext2.html#inode-table
// https://wiki.osdev.org/Ext2#Inode_Type_and_Permissions

#include "constants.h"

/**
 * The ELF Header structure.
 */
struct elfHeader{
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    /**Entry pouint32_t for the initial entry jump. */
    uint32_t e_entry; 
    /** Program header offset. */
    uint32_t e_phoff;    
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    /** Number of program headers. */
    uint16_t e_phnum;    
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

/**
 * Program Header structure.
 */
struct pHeader {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
};

/**
 * The EXT2 Superblock structure.
 */
struct ext2SuperBlock {
  uint32_t sb_total_inodes;
  uint32_t sb_total_blocks;
  uint32_t sb_reserved_blocks;
  uint32_t sb_total_unallocated_blocks;
  uint32_t sb_total_unallocated_inodes;
  uint32_t sb_superblock_block_number;
  uint32_t sb_block_size;
  uint32_t sb_fragment_size;
  uint32_t sb_blocks_per_block_group;
  uint32_t sb_fragements_per_block_group;
  uint32_t sb_inodes_per_block_group;
  uint32_t sb_lastmount_time;
  uint32_t sb_lastwritten_time;
  uint16_t sb_times_mounted_since_consistency_check;
  uint16_t sb_mounts_allows_before_consistency_check;
  uint16_t sb_ext2_signature;
  uint16_t sb_filesystem_state;
  uint16_t sb_error_detection;
  uint16_t sb_minor_version;
  uint32_t sb_last_consistency_check_time;
  uint32_t sb_interval_time;
  uint32_t sb_operating_system_id;
  uint32_t sb_major_version;
  uint16_t sb_userid_for_reserved_blocks;
  uint16_t sb_groupid_for_reserved_blocks;
  uint32_t sb_first_nonreserved_inode;
  uint16_t sb_size_of_inode;
  uint16_t sb_block_group_of_superblock;
  uint32_t sb_optional_features;
  uint32_t sb_required_features;
  uint8_t sb_filesystem_id[16];
  uint8_t sb_volume_name[16];
  uint8_t sb_path_was_last_mounted_to[64];
  uint32_t sb_compression_algorithm;
  uint8_t sb_number_of_block_to_preallocate_for_files;
  uint8_t sb_number_of_block_to_preallocate_for_directories;
  uint16_t sb_not_used;
  uint8_t sb_journal_id[16];
  uint32_t sb_journal_inode;
  uint32_t sb_journal_device;
  uint32_t sb_head_of_orphan_inode_list;
  // The rest of bytes up to 1023 not used  
};

/**
 * The EXT2 Block Group Descriptor structure.
 */
struct blockGroupDescriptor {
  uint32_t bgd_block_address_of_block_usage;
  uint32_t bgd_block_address_of_inode_usage;
  uint32_t bgd_starting_block_of_inode_table;
  uint16_t bgd_number_of_unallocated_blocks_in_group;
  uint16_t bgd_number_of_unallocated_inodes_in_group;
  uint16_t bgd_number_directories_in_group;
  uint8_t bgd_not_used[14];
};

/**
 * The Inode structure.
 */
struct inode {
  uint16_t i_mode;
  uint16_t i_uid;
  uint32_t i_size;
  uint32_t i_atime;
  uint32_t i_ctime;
  uint32_t i_mtime;
  uint32_t i_dtime;
  uint16_t i_gid;
  uint16_t i_links_count;
  uint32_t i_blocks;
  uint32_t i_flags;
  uint32_t i_osd1;
  /** The first 12 blocks are direct blocks, 13 is the first indirect block. */
  uint32_t i_block[15]; 
  uint32_t i_generation;
  uint32_t i_file_acl;
  uint32_t i_dir_acl;
  uint32_t i_faddr;
  uint32_t i_osd2[3];
};

/**
 * The Directory Entry structure.
 */
struct directoryEntry {
  uint32_t directoryInode;
  uint16_t recLength;
  uint8_t nameLength;
  uint8_t fileType;
  uint8_t *fileName;
};

/**
 * The Cache Line Detail structure.
 */
struct cacheLineDetail {
    uint32_t sector;
    uint32_t valid;
    uint32_t accessTime;
};

/**
 * Checks hard disk status and loops again if not ready.
 */
void diskStatusCheck();

/**
 * Reads a 512-byte sector using LBA format and writes it to the destination memory.
 * \param sectorNumber The sector to read in LBA format.
 * \param destinationMemory The pointer to the destination memory to write the sector.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 */
void diskReadSector(uint32_t sectorNumber, uint8_t *destinationMemory, bool cacheActive);

/**
 * Writes 512 bytes of memory to a disk sector. The opposite of diskReadSector(). 
 * \param sectorNumber The sector to write to in LBA format.
 * \param sourceMemory The starting memory address of the 512 bytes to write to the sector.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 */
void diskWriteSector(uint32_t sectorNumber, uint8_t *sourceMemory, bool cacheActive);

/**
 * Reads an EXT2 block number and writes 1024 bytes of the block to the destination memory address.
 * \param blockNumber This is the EXT2 block number, not the disk LBA sector. 
 * \param destinationMemory The pointer to the destination memory to write the block.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 */
void readBlock(uint32_t blockNumber, uint8_t *destinationMemory, bool cacheActive);

/**
 * Writes 1024 bytes of memory to an EXT2 block number. The opposite of readBlock(). 
 * \param blockNumber This is the EXT2 block number, not the disk LBA sector. 
 * \param sourceMemory This is the starting pointing to write 1024 bytes to the EXT2 block.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 */
void writeBlock(uint32_t blockNumber, uint8_t *sourceMemory, bool cacheActive);

/** Finds a free block and returns the block number.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel. 
 */
uint32_t allocateFreeBlock(bool cacheActive);

/** Returns the next available block number without actually allocating it.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
*/
uint32_t readNextAvailableBlock(bool cacheActive);

/** Returns the total blocks used on the file system.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
*/
uint32_t readTotalBlocksUsed(bool cacheActive);

/** Frees a block given a block number.
 * \param blockNumber The block to free.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
*/
void freeBlock(uint32_t blockNumber, bool cacheActive);

/** Frees all blocks associated with an inode.
 * \param inodeStructMemory A pointer to an inode structure for that file.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
*/
void freeAllBlocks(struct inode *inodeStructMemory, bool cacheActive);

/** Deletes a file.
 * \param fileName The file name of the file you want to delete.
 * \param currentPid The pid of the process requesting the delete.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 * \param directoryInode The inode of the directory to look for the file.
 */
void deleteFile(uint8_t *fileName, uint32_t currentPid, bool cacheActive, uint32_t directoryInode);

/** Allocates a free inode and returns the inode number.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
*/
uint32_t allocateInode(bool cacheActive);

/** Returns the next available inode number without actually allocating it.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
*/
uint32_t readNextAvailableInode(bool cacheActive);

/** Deletes the directory entry associated with a file.
 * \param fileName The file name you wish to delete from the directory listing.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 * \param directoryInode Current working directory.
 */
void deleteDirectoryEntry(uint8_t *fileName, bool cacheActive, uint32_t directoryInode);

/** Creates a new file based on an open buffer/file descriptor.
 * \param fileName The name you'd like the new file to be called.
 * \param currentPid The pid of the process requesting the new file.
 * \param fileDescriptor The file descriptor that serves as the basis for the new file's contents.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 * \param directoryInode Current working directory.
 */
void createFile(uint8_t *fileName, uint32_t currentPid, uint32_t fileDescriptor, bool cacheActive, uint32_t directoryInode);

/** Creates an inode entry on the disk.
 * \param inodeEntry The inode associated with the file you wish to write.
 * \param mode The EXT2 mode value for the permissions/file type, etc.
 * \param openFile The pointer to the open file table entry associated with the buffer/file descriptor.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 */
void writeInodeEntry(uint32_t inodeEntry, uint16_t mode, struct globalObjectTableEntry *openFile, bool cacheActive);

/** Given a file name and inode entry, writes the buffer to disk.
 * \param openFile The pointer to the open file table entry associated with the buffer/file descriptor.
 * \param inodeEntry The inode associated with the file.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 */
void writeBufferToDisk(struct globalObjectTableEntry *openFile, uint32_t inodeEntry, bool cacheActive);

/**
 * Checks to see if a file name exists in the current directory of the file system. If found, stores the inode to the destinationMemory location.
 * \param fileName The string value of the file you are looking for.
 * \param destinationMemory Stores the inode of the file at this location if found. This value is typically USER_TEMP_INODE_LOC.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 * \param directoryInode The inode of the directory to look for the file.
 */
bool fsFindFile(uint8_t *fileName, uint8_t *destinationMemory, bool cacheActive, uint32_t directoryInode);

/**
 * Returns the Inode number of a file given its name.
 * \param fileName The string value of the file you are looking for.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 * \param directoryInode Current working directory.
 */
uint32_t returnInodeofFileName(uint8_t *fileName, bool cacheActive, uint32_t directoryInode);

/**
 * Parses an ELF header at a given address and loads the text and data sections to their preferred loading locations based on the ELF header and parsing the program headers.
 * \param elfHeaderLocation The pointer to the ELF header.
 */
void loadElfFile(uint8_t *elfHeaderLocation);

/**
 * Given a pointer to an Inode structure, load all the EXT2 blocks associated with that file to the fileBuffer parameter.
 * \param inodeStructMemory The pointer to the Inode structure. This value is typically USER_TEMP_INODE_LOC.
 * \param fileBuffer The pointer to the location where you want the blocks loaded. This value is typically USER_TEMP_FILE_LOC.
 * \param cacheActive Tell me if the disk read cache is active. This is only active on kernel.
 */
void loadFileFromInodeStruct(uint8_t *inodeStructMemory, uint8_t *fileBuffer, bool cacheActive);

void loadInode(uint32_t inodeNumber, uint8_t* memoryAddress, bool cacheActive);

bool getFilenameFromInode(uint32_t inodeNumber, uint8_t *destinationMemory, bool cacheActive, uint32_t directoryInode);

void moveFile(uint8_t *fileName, uint8_t *sourceDirectory, uint8_t *destinationDirectory, bool cacheActive);

void changeFileMode(uint8_t *fileName, uint16_t newMode, uint32_t currentPid, bool cacheActive, uint32_t directoryInode);

uint32_t getInodeFromPath(uint8_t *path, bool cacheActive);