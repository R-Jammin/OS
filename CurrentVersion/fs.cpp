// Copyright (c) 2023-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Functions were written by Dan O'Malley and Grok. They are noted below.
// 12/2025 with Grok v4.


#include "screen.h"
#include "fs.h"
#include "libc-main.h"
#include "constants.h"
#include "x86.h"
#include "vm.h"
#include "file.h"

uint32_t cacheLRUtime = 0;


void diskReadSector(uint32_t sectorNumber, uint8_t *destinationMemory, bool cacheActive)
{
    
    if (!cacheActive)
    {
        // ASSIGNMENT 2 TO DO
        //step 1
    diskStatusCheck();

  //step2
    outputIOPort(PRIMARY_ATA_SECTOR_COUNT_REGISTER, 1);

    //step 3 and 4
    outputIOPort(PRIMARY_ATA_SECTOR_LOWBYTE_NUMBER, (uint8_t)(sectorNumber & 0xFF));          // Low byte
    outputIOPort(PRIMARY_ATA_SECTOR_MIDBYTE_NUMBER,  (uint8_t)((sectorNumber >> 8) & 0xFF));   // Mid byte
    outputIOPort(PRIMARY_ATA_SECTOR_HIGHBYTE_NUMBER, (uint8_t)((sectorNumber >> 16) & 0xFF));  // High byte

  //5 with shift
    outputIOPort(PRIMARY_ATA_DRIVE_HEADER_REGISTER, 0xE0 | ((sectorNumber >> 24) & 0x0F));

 //read step 6
    outputIOPort(PRIMARY_ATA_COMMAND_STATUS_REGISTER, ATA_READ);

//7
    diskStatusCheck();

   //8
    ioPortWordToMem((uint16_t)PRIMARY_ATA_DATA_REGISTER, destinationMemory, SECTOR_SIZE / 2U);



    }
    else
    {        
        // LRU cache extension by Grok.
        // 12/2025 with Grok v4.

        struct cacheLineDetail *CacheLineDetail = (struct cacheLineDetail *)DISK_READ_CACHE_LOC;
        uint8_t *cacheData = (uint8_t *)DISK_READ_CACHE_DATA;
        uint32_t cacheHits = *(uint32_t *)KERNEL_CACHE_HITS;
        uint32_t cacheMisses = *(uint32_t *)KERNEL_CACHE_MISSES;
        uint8_t *tempBuffer = 0;
        uint32_t cacheLine;

        // Check for cache hit
        for (cacheLine = 0; cacheLine < CACHE_SIZE; cacheLine++) 
        {
            if (CacheLineDetail[cacheLine].valid == 1 && CacheLineDetail[cacheLine].sector == sectorNumber) 
            {
                // Record the cache hit
                *(uint32_t*)KERNEL_CACHE_HITS = ++cacheHits;
                tempBuffer = cacheData + (cacheLine * SECTOR_SIZE);
                memoryCopy(tempBuffer, destinationMemory, ceiling(SECTOR_SIZE, 2));
                CacheLineDetail[cacheLine].accessTime = ++cacheLRUtime;
                return;
            }
        }

        // Record the cache miss
        *(uint32_t*)KERNEL_CACHE_MISSES = ++cacheMisses;
        
        // Find an invalid slot
        cacheLine = CACHE_SIZE;
        for (uint32_t i = 0; i < CACHE_SIZE; i++) 
        {
            if (CacheLineDetail[i].valid == 0) 
            {
                cacheLine = i;
                break;
            }
        }

        // If no invalid slot, evict LRU (lowest accessTime among valid entries)
        if (cacheLine == CACHE_SIZE) 
        {
            uint32_t minimumTime = 0xFFFFFFFF; 
            cacheLine = 0;
            for (uint32_t i = 0; i < CACHE_SIZE; i++) 
            {
                if (CacheLineDetail[i].valid == 1 && CacheLineDetail[i].accessTime < minimumTime) 
                {
                    minimumTime = CacheLineDetail[i].accessTime;
                    cacheLine = i;
                }
            }
        }

        // Compute the cache pointer to use to load data not in cache
        tempBuffer = (uint8_t *)(cacheData + (cacheLine * SECTOR_SIZE));

        // Load with false since we want this to bypass this code or else it is circular
        diskReadSector(sectorNumber, tempBuffer, false);

        // Update cache info since I just loaded a new cache line with data
        CacheLineDetail[cacheLine].sector = sectorNumber;
        CacheLineDetail[cacheLine].valid = 1;
        CacheLineDetail[cacheLine].accessTime = ++cacheLRUtime;

        // Copy to destination now that the cache is loaded
        memoryCopy(tempBuffer, destinationMemory, ceiling(SECTOR_SIZE, 2));

    }
}


void diskStatusCheck()
{
    // Dan O'Malley
    
    // checks disk status and loops if not ready
    while ( ((inputIOPort(PRIMARY_ATA_COMMAND_STATUS_REGISTER) & 0xC0) != 0x40) ) {}   
}


void diskWriteSector(uint32_t sectorNumber, uint8_t *sourceMemory, bool cacheActive)
{
    // Do not remove this section when completing assignment 2
    if (cacheActive)
    {
        // flush entire cache on write.
        fillMemory(DISK_READ_CACHE_LOC, 0x0, PAGE_SIZE);
        fillMemory(DISK_READ_CACHE_DATA, 0x0, CACHE_SIZE * BLOCK_SIZE);
        
    }
     
    
    // ASSIGNMENT 2 TO DO
    diskStatusCheck();


    outputIOPort(PRIMARY_ATA_SECTOR_COUNT_REGISTER, 1);

    outputIOPort(PRIMARY_ATA_SECTOR_LOWBYTE_NUMBER, (uint8_t)(sectorNumber & 0xFF));          // Low byte
    outputIOPort(PRIMARY_ATA_SECTOR_MIDBYTE_NUMBER,  (uint8_t)((sectorNumber >> 8) & 0xFF));   // Mid byte
    outputIOPort(PRIMARY_ATA_SECTOR_HIGHBYTE_NUMBER, (uint8_t)((sectorNumber >> 16) & 0xFF));  // High byte

    outputIOPort(PRIMARY_ATA_DRIVE_HEADER_REGISTER, 0xE0 | ((sectorNumber >> 24) & 0x0F));

    outputIOPort(PRIMARY_ATA_COMMAND_STATUS_REGISTER, ATA_WRITE);

    
    diskStatusCheck();

    memToIoPortWord((uint16_t)PRIMARY_ATA_DATA_REGISTER, sourceMemory, SECTOR_SIZE / 2U);
    

}

void readBlock(uint32_t blockNumber, uint8_t *destinationMemory, bool cacheActive)
{
    // Dan O'Malley
    
    uint32_t sectorStart = (blockNumber * (BLOCK_SIZE / SECTOR_SIZE)) + EXT2_SECTOR_START;

    for (uint32_t x = 0; x < BLOCK_SIZE / SECTOR_SIZE; x++)
    {
        diskReadSector(sectorStart + x, destinationMemory + (SECTOR_SIZE * x), cacheActive);
    }

}

void writeBlock(uint32_t blockNumber, uint8_t *sourceMemory, bool cacheActive)
{
    // Dan O'Malley
    
    uint32_t sectorStart = (blockNumber * (BLOCK_SIZE / SECTOR_SIZE)) + EXT2_SECTOR_START;

    for (uint32_t x = 0; x < BLOCK_SIZE / SECTOR_SIZE; x++)
    {
        diskWriteSector(sectorStart + x, sourceMemory + (SECTOR_SIZE * x), cacheActive);
    }

}

uint32_t allocateFreeBlock(bool cacheActive)
{
    // Initial version by Dan O'Malley. Extended with Grok.
    // 12/2025 with Grok v4.
    
    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    readBlock(BlockGroupDescriptor->bgd_block_address_of_block_usage, (uint8_t *)EXT2_BLOCK_USAGE_MAP, cacheActive);

    uint8_t *bitmap = (uint8_t *)EXT2_BLOCK_USAGE_MAP;

    for (uint32_t byte_idx = 0; byte_idx < 0x7D0; byte_idx++)
    {
        uint8_t byte = bitmap[byte_idx];
        if (byte != 0xff)
        {
            for (uint32_t bit = 0; bit < 8; bit++)
            {
                if ((byte & (1 << bit)) == 0)
                {
                    uint32_t block = byte_idx * 8 + bit + 1;
                    bitmap[byte_idx] |= (1 << bit);
                    writeBlock(BlockGroupDescriptor->bgd_block_address_of_block_usage, (uint8_t *)EXT2_BLOCK_USAGE_MAP, cacheActive);
                    return block;
                }
            }
        }
    }
    // No free block found
    return 0;
}

uint32_t readNextAvailableBlock(bool cacheActive)
{
    // Dan O'Malley
    
    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    readBlock(BlockGroupDescriptor->bgd_block_address_of_block_usage, (uint8_t *)EXT2_BLOCK_USAGE_MAP, cacheActive);

    uint32_t lastUsedBlock = 0;
    uint32_t blockNumber = 0;

    while (blockNumber < 0x7D0) // Max byte of block bit flag for this 32 MB file system (16000/8)
    {
        if (*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)1) { lastUsedBlock++; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)3)) { lastUsedBlock=lastUsedBlock+2; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)7)) { lastUsedBlock=lastUsedBlock+3; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)15)) { lastUsedBlock=lastUsedBlock+4; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)31)) { lastUsedBlock=lastUsedBlock+5; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)63)) { lastUsedBlock=lastUsedBlock+6; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)127)) { lastUsedBlock=lastUsedBlock+7; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)255)) { lastUsedBlock=lastUsedBlock+8;}
        else if (*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)0) { break; }
        blockNumber++;
    }

    lastUsedBlock++;

    return (uint32_t)lastUsedBlock;
}

uint32_t readTotalBlocksUsed(bool cacheActive)
{
    // Dan O'Malley
    
    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    readBlock(BlockGroupDescriptor->bgd_block_address_of_block_usage, (uint8_t *)EXT2_BLOCK_USAGE_MAP, cacheActive);

    uint32_t blocksInUse = 0;
    uint32_t blockNumber = 0;

    while (blockNumber < 0x7D0) // Max byte of block bit flag for this 32 MB file system (16000/8)
    {
        if (*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)1) { blocksInUse++;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)3)) { blocksInUse=blocksInUse+2;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)7)) { blocksInUse=blocksInUse+3;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)15)) { blocksInUse=blocksInUse+4;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)31)) { blocksInUse=blocksInUse+5;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)63)) { blocksInUse=blocksInUse+6;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)127)) { blocksInUse=blocksInUse+7;}
        else if (((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockNumber) == (uint8_t)255)) { blocksInUse=blocksInUse+8;}
        blockNumber++;
    }

    return (uint32_t)blocksInUse;
}

void freeBlock(uint32_t blockNumber, bool cacheActive)
{
    // Initial version by Dan O'Malley. Extended with Grok.
    // 12/2025 with Grok v4.
    
    uint32_t blockGroupByte = blockNumber / 8;
    uint32_t blockGroupBit = blockNumber % 8;
    uint32_t valueToWrite = 0;

    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    readBlock(BlockGroupDescriptor->bgd_block_address_of_block_usage, (uint8_t *)EXT2_BLOCK_USAGE_MAP, cacheActive);

    if (blockGroupBit == (uint8_t)1) { valueToWrite = ((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockGroupByte)) -1;}
    else if (blockGroupBit == (uint8_t)2) { valueToWrite = ((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockGroupByte)) -2;}
    else if (blockGroupBit == (uint8_t)3) { valueToWrite = ((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockGroupByte)) -4;}
    else if (blockGroupBit == (uint8_t)4) { valueToWrite = ((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockGroupByte)) -8;}
    else if (blockGroupBit == (uint8_t)5) { valueToWrite = ((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockGroupByte)) -16;}
    else if (blockGroupBit == (uint8_t)6) { valueToWrite = ((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockGroupByte)) -32;}
    else if (blockGroupBit == (uint8_t)7) { valueToWrite = ((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockGroupByte)) -64;}
    else if (blockGroupBit == (uint8_t)0) { valueToWrite = ((uint8_t)*(uint8_t*)(EXT2_BLOCK_USAGE_MAP + blockGroupByte)) -128;}

    *(uint8_t *)(EXT2_BLOCK_USAGE_MAP + blockGroupByte) = (uint8_t)valueToWrite;

    writeBlock(BlockGroupDescriptor->bgd_block_address_of_block_usage, (uint8_t *)EXT2_BLOCK_USAGE_MAP, cacheActive);   
}

void freeAllBlocks(struct inode *inodeStructMemory, bool cacheActive)
{
    // Dan O'Malley
    
    for (uint32_t x = 0; x < EXT2_NUMBER_OF_DIRECT_BLOCKS; x++)
    { 
        if (x < EXT2_NUMBER_OF_DIRECT_BLOCKS && inodeStructMemory->i_block[x] != 0)
        {
            freeBlock(inodeStructMemory->i_block[x], cacheActive);
        }       
    }
    if (inodeStructMemory->i_block[EXT2_FIRST_INDIRECT_BLOCK] && inodeStructMemory->i_block[EXT2_FIRST_INDIRECT_BLOCK] != 0)
    {
        readBlock(inodeStructMemory->i_block[EXT2_FIRST_INDIRECT_BLOCK], EXT2_INDIRECT_BLOCK, cacheActive);
        uint32_t *indirectBlock = (uint32_t *)EXT2_INDIRECT_BLOCK;

        for (uint32_t y = 0; y < EXT2_BLOCKS_PER_INDIRECT_BLOCK; y++)
        {
            if (indirectBlock[y] !=0)
            {
                freeBlock(indirectBlock[y], cacheActive);
            }

        }
             
    }
}

void deleteFile(uint8_t *fileName, uint32_t currentPid, bool cacheActive, uint32_t directoryInode)
{
    // Dan O'Malley
    
    uint8_t *inodePage = requestAvailablePage(currentPid, PG_USER_PRESENT_RW);
    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    
    fsFindFile(fileName, inodePage, cacheActive, directoryInode);

    if (!fsFindFile(fileName, inodePage, cacheActive, directoryInode))
    {
        // File not found
        return;
    }
    //freeAllBlocks((struct inode *)inodePage, cacheActive);

    // Load all inodes of a directory up to max number of files per directory. This requires 16KB of memory.
    for (uint32_t blocksOfInodes=0; blocksOfInodes < (MAX_FILES_PER_DIRECTORY / INODES_PER_BLOCK); blocksOfInodes++)
    {
        readBlock(BlockGroupDescriptor->bgd_starting_block_of_inode_table + blocksOfInodes, (uint8_t *)(uint32_t)EXT2_TEMP_INODE_STRUCTS + (BLOCK_SIZE * blocksOfInodes), cacheActive);
    }

    // Zero out the inode
    fillMemory((EXT2_TEMP_INODE_STRUCTS + ((returnInodeofFileName(fileName, cacheActive, directoryInode)-1) * INODE_SIZE)), 0x0, INODE_SIZE);

    for (uint32_t blocksOfInodes=0; blocksOfInodes < (MAX_FILES_PER_DIRECTORY / INODES_PER_BLOCK); blocksOfInodes++)
    {
        writeBlock(BlockGroupDescriptor->bgd_starting_block_of_inode_table + blocksOfInodes, (uint8_t *)(uint32_t)EXT2_TEMP_INODE_STRUCTS + (BLOCK_SIZE * blocksOfInodes), cacheActive);
    }

    deleteDirectoryEntry(fileName, cacheActive, directoryInode);
    freePage(currentPid, inodePage);
}

uint32_t allocateInode(bool cacheActive)
{
    // Initial version written by Dan O'Malley. Extended with Grok.
    // 12/2025 with Grok v4.
    
    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    readBlock(BlockGroupDescriptor->bgd_block_address_of_inode_usage, (uint8_t *)EXT2_INODE_USAGE_MAP, cacheActive);

    uint8_t *bitmap = (uint8_t *)EXT2_INODE_USAGE_MAP;

    for (uint32_t byte_idx = 0; byte_idx < 0x3E8; byte_idx++)
    {
        uint8_t byte = bitmap[byte_idx];
        if (byte != 0xff)
        {
            for (uint32_t bit = 0; bit < 8; bit++)
            {
                if ((byte & (1 << bit)) == 0)
                {
                    uint32_t inode = byte_idx * 8 + bit + 1;
                    bitmap[byte_idx] |= (1 << bit);
                    writeBlock(BlockGroupDescriptor->bgd_block_address_of_inode_usage, (uint8_t *)EXT2_INODE_USAGE_MAP, cacheActive);
                    return inode;
                }
            }
        }
    }
    // No free inode found
    return 0;
}

uint32_t readNextAvailableInode(bool cacheActive)
{
    // Dan O'Malley
    
    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    readBlock(BlockGroupDescriptor->bgd_block_address_of_inode_usage, (uint8_t *)EXT2_INODE_USAGE_MAP, cacheActive);

    uint32_t lastUsedInode = 0;
    uint32_t inodeNumber = 0;

    while (inodeNumber < 0x3E8) // Max byte of inode bit flag for this 32 MB file system (8000 inodes/8)
    {
        if (*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)1) { lastUsedInode++; break; }
        else if (((uint8_t)*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)3)) { lastUsedInode=lastUsedInode+2; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)7)) { lastUsedInode=lastUsedInode+3; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)15)) { lastUsedInode=lastUsedInode+4; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)31)) { lastUsedInode=lastUsedInode+5; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)63)) { lastUsedInode=lastUsedInode+6; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)127)) { lastUsedInode=lastUsedInode+7; break;}
        else if (((uint8_t)*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)255)) { lastUsedInode=lastUsedInode+8; }
        else if (*(uint8_t*)(EXT2_INODE_USAGE_MAP + inodeNumber) == (uint8_t)0) { break; }
        inodeNumber++;
    }

    lastUsedInode++;

    return (uint32_t )lastUsedInode;
}

void deleteDirectoryEntry(uint8_t *fileName, bool cacheActive, uint32_t directoryInode)
{
    // Initial version written by Dan O'Malley. Extended with Grok.
    // 12/2025 with Grok v4.
    
    uint32_t inodeToRemove = returnInodeofFileName(fileName, cacheActive, directoryInode);
    if (inodeToRemove == 0) return;
   
    fillMemory((uint8_t *)KERNEL_WORKING_DIR_TEMP_INODE_LOC, 0x0, KERNEL_WORKING_DIR_TEMP_INODE_LOC_SIZE);
    fillMemory((uint8_t *)KERNEL_WORKING_DIR, 0x0, KERNEL_WORKING_DIR_SIZE);

    loadInode(directoryInode, KERNEL_WORKING_DIR_TEMP_INODE_LOC, cacheActive);
    loadFileFromInodeStruct(KERNEL_WORKING_DIR_TEMP_INODE_LOC, KERNEL_WORKING_DIR, cacheActive);

    struct inode *Inode = (struct inode*)(KERNEL_WORKING_DIR_TEMP_INODE_LOC);

    uint32_t pos = 0;
    struct directoryEntry *prev = NULL;
    while (pos < BLOCK_SIZE)
    {
        struct directoryEntry *dir = (directoryEntry*)(KERNEL_WORKING_DIR + pos);
        uint16_t rec = dir->recLength;
        if (rec == 0) break;
        if (dir->directoryInode == inodeToRemove)
        {
            if (prev)
            {
                prev->recLength += rec;
                fillMemory((uint8_t*)dir, 0x0, rec);
            }
            else
            {
                fillMemory((uint8_t*)dir, 0x0, rec);
            }
            
            // I only write the first block, if directories require more than one block,
            // I will have to add more writes here.
            writeBlock(Inode->i_block[0], (uint8_t *)KERNEL_WORKING_DIR, cacheActive);
            return;
        }
        if (dir->directoryInode != 0)
        {
            prev = dir;
        }
        pos += rec;
    }
}

void createFile(uint8_t *fileName, uint32_t currentPid, uint32_t fileDescriptor, bool cacheActive, uint32_t directoryInode)
{
    // Initial version written by Dan O'Malley but extended with bug fix by Grok.
    // 12/2025 with Grok v4.
    
    uint32_t taskStructLocation = PROCESS_TABLE_LOC + (TASK_STRUCT_SIZE * (currentPid - 1));
    struct task *Task = (struct task*)taskStructLocation;

    fillMemory((uint8_t *)KERNEL_WORKING_DIR_TEMP_INODE_LOC, 0x0, KERNEL_WORKING_DIR_TEMP_INODE_LOC_SIZE);
    fillMemory((uint8_t *)KERNEL_WORKING_DIR, 0x0, KERNEL_WORKING_DIR_SIZE);

    loadInode(directoryInode, KERNEL_WORKING_DIR_TEMP_INODE_LOC, cacheActive);
    loadFileFromInodeStruct(KERNEL_WORKING_DIR_TEMP_INODE_LOC, KERNEL_WORKING_DIR, cacheActive);

    struct inode *Inode = (struct inode*)(KERNEL_WORKING_DIR_TEMP_INODE_LOC);

    uint32_t pos = 0;
    struct directoryEntry *last_entry = NULL;
    uint32_t last_pos = 0;
    while (pos < BLOCK_SIZE)
    {
        struct directoryEntry *dir = (directoryEntry*)(KERNEL_WORKING_DIR + pos);
        uint16_t rec = dir->recLength;
        if (rec == 0) break;
        if (dir->directoryInode != 0)
        {
            last_entry = dir;
            last_pos = pos;
        }
        pos += rec;
    }

    if (last_entry == NULL)
    {
        return; // empty dir, need to handle . ..
    }

    uint16_t old_rec = last_entry->recLength;
    uint8_t old_name_len = last_entry->nameLength;
    uint16_t min_old = ((old_name_len + 8 + 3) / 4) * 4;

    uint8_t new_name_len = (uint8_t)strlen(fileName);
    uint16_t min_new = ((new_name_len + 8 + 3) / 4) * 4;

    if (old_rec < min_old + min_new)
    {
        return; // not enough, need extend dir
    }

    last_entry->recLength = min_old;

    struct directoryEntry *new_entry = (directoryEntry*)(KERNEL_WORKING_DIR + last_pos + min_old);
    new_entry->directoryInode = allocateInode(cacheActive);
    bytecpy((uint8_t *)(new_entry) + 8, fileName, new_name_len);
    new_entry->fileType = (uint8_t)1;
    new_entry->nameLength = new_name_len;
    new_entry->recLength = old_rec - min_old;

    // Pad the name with zeros if necessary
    uint32_t pad_len = min_new - 8 - new_name_len;
    if (pad_len > 0) {
        fillMemory((uint8_t *)(new_entry) + 8 + new_name_len, 0, pad_len);
    }

    writeInodeEntry(new_entry->directoryInode, 0x81b6, (struct globalObjectTableEntry *)Task->fileDescriptor[fileDescriptor], cacheActive);
    
    // I only write the first block, if directories require more than one block,
    // I will have to add more writes here.
    writeBlock(Inode->i_block[0], (uint8_t *)KERNEL_WORKING_DIR, cacheActive);
}

void writeInodeEntry(uint32_t inodeEntry, uint16_t mode, struct globalObjectTableEntry *openFile, bool cacheActive)
{
    // Dan O'Malley
    
    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    uint32_t blocksOfInodes = 0;

    while(blocksOfInodes < (MAX_FILES_PER_DIRECTORY / INODES_PER_BLOCK))
    {
        readBlock(BlockGroupDescriptor->bgd_starting_block_of_inode_table + blocksOfInodes, (uint8_t *)(int)EXT2_TEMP_INODE_STRUCTS + (BLOCK_SIZE * blocksOfInodes), cacheActive);
        blocksOfInodes++;
    }

    struct inode *Inode = (struct inode*)(EXT2_TEMP_INODE_STRUCTS + (INODE_SIZE * (inodeEntry - 1)));

    Inode->i_mode = mode;
    // Doesn't seem to write the correct size here, though if I hard-code it with a size
    // it does write. 
    Inode->i_blocks = ceiling(openFile->size, BLOCK_SIZE);

    writeBufferToDisk(openFile, inodeEntry, cacheActive);

    blocksOfInodes = 0;

    while(blocksOfInodes < (MAX_FILES_PER_DIRECTORY / INODES_PER_BLOCK))
    {
        writeBlock(BlockGroupDescriptor->bgd_starting_block_of_inode_table + blocksOfInodes, (uint8_t *)(int)EXT2_TEMP_INODE_STRUCTS + (BLOCK_SIZE * blocksOfInodes), cacheActive);
        blocksOfInodes++;
    }

}

void writeBufferToDisk(struct globalObjectTableEntry *openFile, uint32_t inodeEntry, bool cacheActive)
{
    // Dan O'Malley
    
    struct inode *Inode = (struct inode*)(EXT2_TEMP_INODE_STRUCTS + (INODE_SIZE * (inodeEntry - 1)));

    fillMemory((uint8_t *)EXT2_INDIRECT_BLOCK_TMP_LOC, 0x0, BLOCK_SIZE);

    uint32_t blockArrayDirect[13];
    uint32_t *blockArraySinglyIndirect = (uint32_t *)EXT2_INDIRECT_BLOCK_TMP_LOC;
    uint32_t totalBlocksNeeded = ceiling((openFile->numberOfPagesForBuffer * PAGE_SIZE), BLOCK_SIZE);
    uint32_t currentDirectBlock = 0;
    uint32_t currentIndirectBlock = 0;

    while (currentDirectBlock <= 12 && (currentDirectBlock < totalBlocksNeeded))
    {
        if (currentDirectBlock < 12)
        {
            blockArrayDirect[currentDirectBlock] = allocateFreeBlock(cacheActive);
            writeBlock(blockArrayDirect[currentDirectBlock], (uint8_t *)(openFile->userspaceBuffer + (currentDirectBlock * BLOCK_SIZE)), cacheActive);
            Inode->i_block[currentDirectBlock] = blockArrayDirect[currentDirectBlock];
            currentDirectBlock++;
        }
        else if (currentDirectBlock == 12)
        {
            blockArrayDirect[12] = allocateFreeBlock(cacheActive); //write the indirect block
            Inode->i_block[12] = blockArrayDirect[12];
            currentDirectBlock++;
        }
    }

    if (totalBlocksNeeded > 12)
    {

        for (currentIndirectBlock=0; (currentIndirectBlock + currentDirectBlock) < totalBlocksNeeded; currentIndirectBlock++)
        {
            blockArraySinglyIndirect[currentIndirectBlock] = allocateFreeBlock(cacheActive);
            writeBlock(blockArraySinglyIndirect[currentIndirectBlock], (uint8_t *)(openFile->userspaceBuffer + ((currentIndirectBlock + currentDirectBlock) * BLOCK_SIZE)), cacheActive);
        }

        // Write the indirect block to disk
        writeBlock(Inode->i_block[12], (uint8_t *)EXT2_INDIRECT_BLOCK_TMP_LOC, cacheActive);
    }

    Inode->i_size = (currentDirectBlock + currentIndirectBlock) * BLOCK_SIZE;
}


void loadElfFile(uint8_t *elfHeaderLocation)
{    
    // ASSIGNMENT 2 TO DO
if (elfHeaderLocation == nullptr) return;//Null check

    
    struct elfHeader *elf = (struct elfHeader *)elfHeaderLocation;
    struct pHeader *programHeaders = (struct pHeader *)(elfHeaderLocation + elf->e_phoff);

    //Iterate through each program header
    for (uint16_t i = 0; i < elf->e_phnum; i++) {
        struct pHeader *ph = &programHeaders[i];

        if (ph->p_type != 1)
            continue;

        // Calculate source and dest
        uint8_t *segmentSrc = elfHeaderLocation + ph->p_offset;
        uint8_t *segmentDst = (uint8_t *)ph->p_vaddr;

        // Convert bytes into words
        uint32_t numWords = ph->p_memsz / 2; 
        if (ph->p_memsz % 2 != 0) //Round up as if for ceiling
            numWords++; 

        //Copy the segment into memory
        memoryCopy((uint8_t *)segmentSrc, (uint8_t *)segmentDst, numWords);
    }
}

void loadFileFromInodeStruct(uint8_t *inodeStructMemory, uint8_t *fileBuffer, bool cacheActive)
{

    // ASSIGNMENT 2 TO DO
     if (inodeStructMemory == nullptr || fileBuffer == nullptr) return;// Null check

    struct inode *inode = (struct inode *)inodeStructMemory;

    
    const int DIRECT_COUNT = 12;
    const int SINGLE_INDIRECT_INDEX = 12;
    const int INDIRECT_ENTRIES = BLOCK_SIZE / sizeof(uint32_t);

    // 1 Parse direct blocks
    for (uint16_t i = 0; i < DIRECT_COUNT; ++i) {
        uint32_t blk = inode->i_block[i];
        if (blk == 0) {
            // no block: zero out the region and advance
            storeValueAtMemLoc(fileBuffer, BLOCK_SIZE);
        } else {
            // read the block directly into the destination buffer
            readBlock(blk, fileBuffer,cacheActive);
        }
        fileBuffer += BLOCK_SIZE;
    }

    // 2 Read first indirect block and copy its pointers into an array
    uint32_t indirect_blk = inode->i_block[SINGLE_INDIRECT_INDEX];
    if (indirect_blk == 0) {
        for (int i = 0; i < INDIRECT_ENTRIES; ++i) {
            storeValueAtMemLoc(fileBuffer, BLOCK_SIZE);
            fileBuffer += BLOCK_SIZE;
        }
        return;
    }

    // BLOCK_SIZE is small 
    uint32_t indirect_entries[INDIRECT_ENTRIES];

    // Read the indirect block 
    readBlock(indirect_blk, (uint8_t *)indirect_entries,cacheActive);

    // 3 Treat each entry as another direct block and loop
    for (int i = 0; i < INDIRECT_ENTRIES; ++i) {
        uint32_t blk = indirect_entries[i];
        if (blk == 0) {
            storeValueAtMemLoc(fileBuffer, BLOCK_SIZE);
        } else {
            readBlock(blk, fileBuffer, cacheActive);
        }
        fileBuffer += BLOCK_SIZE;
    }
}


bool fsFindFile(uint8_t *fileName, uint8_t *destinationMemory, bool cacheActive, uint32_t directoryInode)
{ 
    // Written by Dan O'Malley but adjusted by Grok as there was a bug
    // when adding files to the directory
    // 12/2025 with Grok v4.
    
    fillMemory((uint8_t *)KERNEL_WORKING_DIR_TEMP_INODE_LOC, 0x0, KERNEL_WORKING_DIR_TEMP_INODE_LOC_SIZE);
    fillMemory((uint8_t *)KERNEL_WORKING_DIR, 0x0, KERNEL_WORKING_DIR_SIZE);

    loadInode(directoryInode, KERNEL_WORKING_DIR_TEMP_INODE_LOC, cacheActive);
    loadFileFromInodeStruct(KERNEL_WORKING_DIR_TEMP_INODE_LOC, KERNEL_WORKING_DIR, cacheActive);

    uint32_t pos = 0;
    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    uint32_t name_len = strlen(fileName);

    while (pos < BLOCK_SIZE)
    {
        struct directoryEntry *DirectoryEntry = (directoryEntry*)(KERNEL_WORKING_DIR + pos);
        uint16_t rec_len = DirectoryEntry->recLength;
        if (rec_len == 0) break;
        if (DirectoryEntry->directoryInode != 0 && DirectoryEntry->nameLength == name_len)
        {
            uint8_t *name_start = (uint8_t *)(DirectoryEntry) + 8;
            bool match = true;
            for (uint32_t i = 0; i < name_len; i++) {
                if (name_start[i] != fileName[i]) {
                    match = false;
                    break;
                }
            }
            if (match) 
            {      
                // Load all inodes of a directory up to max number of files per directory. This requires 16KB of memory.
                for (uint32_t blocksOfInodes=0; blocksOfInodes < (MAX_FILES_PER_DIRECTORY / INODES_PER_BLOCK); blocksOfInodes++)
                {
                    readBlock(BlockGroupDescriptor->bgd_starting_block_of_inode_table + blocksOfInodes, (uint8_t *)(uint32_t)EXT2_TEMP_INODE_STRUCTS + (BLOCK_SIZE * blocksOfInodes), cacheActive);
                }
                memoryCopy( (uint8_t *)((uint32_t)EXT2_TEMP_INODE_STRUCTS + ((DirectoryEntry->directoryInode -1) * INODE_SIZE)), destinationMemory, INODE_SIZE);
                
                return true;
            }
        }
        pos += rec_len;
    }
    return false;
}

uint32_t returnInodeofFileName(uint8_t *fileName, bool cacheActive, uint32_t directoryInode)
{ 
    // This code was written with Grok, an AI by xAI, based on my guidance and specifications.
    // 12/2025 with Grok v4.
    
    fillMemory((uint8_t *)KERNEL_WORKING_DIR_TEMP_INODE_LOC, 0x0, KERNEL_WORKING_DIR_TEMP_INODE_LOC_SIZE);
    fillMemory((uint8_t *)KERNEL_WORKING_DIR, 0x0, KERNEL_WORKING_DIR_SIZE);

    loadInode(directoryInode, KERNEL_WORKING_DIR_TEMP_INODE_LOC, cacheActive);
    loadFileFromInodeStruct(KERNEL_WORKING_DIR_TEMP_INODE_LOC, KERNEL_WORKING_DIR, cacheActive);

    uint32_t pos = 0;
    uint32_t name_len = strlen(fileName);
    while (pos < BLOCK_SIZE)
    {
        struct directoryEntry *DirectoryEntry = (directoryEntry*)(KERNEL_WORKING_DIR + pos);
        uint16_t rec_len = DirectoryEntry->recLength;
        if (rec_len == 0) break;
        if (DirectoryEntry->directoryInode != 0 && DirectoryEntry->nameLength == name_len)
        {
            uint8_t *name_start = (uint8_t *)(DirectoryEntry) + 8;
            bool match = true;
            for (uint32_t i = 0; i < name_len; i++) {
                if (name_start[i] != fileName[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {      
                return DirectoryEntry->directoryInode;
            }
        }
        pos += rec_len;
    }
    return 0;
}

void loadInode(uint32_t inodeNumber, uint8_t* memoryAddress, bool cacheActive)
{
    // This code was written with Grok, an AI by xAI, based on my guidance and specifications.
    // 12/2025 with Grok v4.
    
    struct blockGroupDescriptor *BlockGroupDescriptor = (struct blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    uint32_t inode_index = inodeNumber - 1;
    uint32_t inodes_per_block = BLOCK_SIZE / INODE_SIZE;
    uint32_t block_offset = inode_index / inodes_per_block;
    uint32_t inode_block = BlockGroupDescriptor->bgd_starting_block_of_inode_table + block_offset;
    readBlock(inode_block, KERNEL_TEMP_INODE_LOC, cacheActive);
    uint32_t offset = (inode_index % inodes_per_block) * INODE_SIZE;
    bytecpy(memoryAddress, KERNEL_TEMP_INODE_LOC + offset, INODE_SIZE);
}

bool getFilenameFromInode(uint32_t inodeNumber, uint8_t *destinationMemory, bool cacheActive, uint32_t directoryInode)
{
    // This code was written with Grok, an AI by xAI, based on my guidance and specifications.
    // 12/2025 with Grok v4.
    
    fillMemory((uint8_t *)KERNEL_WORKING_DIR_TEMP_INODE_LOC, 0x0, KERNEL_WORKING_DIR_TEMP_INODE_LOC_SIZE);
    fillMemory((uint8_t *)KERNEL_WORKING_DIR, 0x0, KERNEL_WORKING_DIR_SIZE);

    loadInode(directoryInode, KERNEL_WORKING_DIR_TEMP_INODE_LOC, cacheActive);
    loadFileFromInodeStruct(KERNEL_WORKING_DIR_TEMP_INODE_LOC, KERNEL_WORKING_DIR, cacheActive);

    uint32_t pos = 0;

    while (pos < BLOCK_SIZE)
    {
        struct directoryEntry *DirectoryEntry = (directoryEntry*)(KERNEL_WORKING_DIR + pos);
        uint16_t rec_len = DirectoryEntry->recLength;
        if (rec_len == 0) break;
        if (DirectoryEntry->directoryInode == inodeNumber)
        {
            uint8_t *name_start = (uint8_t *)(DirectoryEntry) + 8;
            bytecpy(destinationMemory, name_start, DirectoryEntry->nameLength);
            destinationMemory[DirectoryEntry->nameLength] = '\0';
            return true;
        }
        pos += rec_len;
    }
    return false;
}

void moveFile(uint8_t *fileName, uint8_t *sourceDirectory, uint8_t *destinationDirectory, bool cacheActive)
{
    // This code was written with Grok, an AI by xAI, based on my guidance and specifications.
    // 12/2025 with Grok v4.
    
    uint32_t fileInode = returnInodeofFileName(fileName, cacheActive, getInodeFromPath(sourceDirectory, cacheActive));
    if (fileInode == 0) return;

    fillMemory((uint8_t *)KERNEL_WORKING_DIR, 0x0, KERNEL_WORKING_DIR_SIZE);

    loadInode(getInodeFromPath(sourceDirectory, cacheActive), KERNEL_WORKING_DIR_TEMP_INODE_LOC, cacheActive);
    loadFileFromInodeStruct(KERNEL_WORKING_DIR_TEMP_INODE_LOC, KERNEL_WORKING_DIR, cacheActive);

    uint32_t pos = 0;
    uint8_t fileType = 0;
    uint32_t name_len = strlen(fileName);

    while (pos < BLOCK_SIZE)
    {
        struct directoryEntry *DirectoryEntry = (directoryEntry*)(KERNEL_WORKING_DIR + pos);
        uint16_t rec_len = DirectoryEntry->recLength;
        if (rec_len == 0) break;
        if (DirectoryEntry->directoryInode == fileInode && DirectoryEntry->nameLength == name_len)
        {
            uint8_t *name_start = (uint8_t *)(DirectoryEntry) + 8;
            bool match = true;
            for (uint32_t i = 0; i < name_len; i++) {
                if (name_start[i] != fileName[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                fileType = DirectoryEntry->fileType;
                break;
            }
        }
        pos += rec_len;
    }

    if (fileType == 0) return;

    fillMemory((uint8_t *)KERNEL_WORKING_DIR, 0x0, KERNEL_WORKING_DIR_SIZE);

    loadInode(getInodeFromPath(destinationDirectory, cacheActive), KERNEL_WORKING_DIR_TEMP_INODE_LOC, cacheActive);
    loadFileFromInodeStruct(KERNEL_WORKING_DIR_TEMP_INODE_LOC, KERNEL_WORKING_DIR, cacheActive);

    uint32_t last_pos = 0;
    pos = 0;
    struct directoryEntry *last_entry = 0;

    while (pos < BLOCK_SIZE)
    {
        struct directoryEntry *entry = (directoryEntry*)(KERNEL_WORKING_DIR + pos);
        if (entry->recLength == 0) break;
        last_pos = pos;
        last_entry = entry;
        pos += entry->recLength;
    }

    uint32_t min_old = 8 + last_entry->nameLength;
    min_old = ((min_old + 3) / 4) * 4;
    uint32_t min_new = 8 + name_len;
    min_new = ((min_new + 3) / 4) * 4;
    uint16_t old_rec = last_entry->recLength;

    if (old_rec < min_old + min_new)
    {
        return;
    }

    last_entry->recLength = min_old;

    struct directoryEntry *new_entry = (directoryEntry*)(KERNEL_WORKING_DIR + last_pos + min_old);
    new_entry->directoryInode = fileInode;
    bytecpy((uint8_t *)(new_entry) + 8, fileName, name_len);
    new_entry->fileType = fileType;
    new_entry->nameLength = name_len;
    new_entry->recLength = old_rec - min_old;

    uint32_t pad_len = min_new - 8 - name_len;
    if (pad_len > 0) {
        fillMemory((uint8_t *)(new_entry) + 8 + name_len, 0, pad_len);
    }

    struct inode *Inode = (struct inode*)KERNEL_WORKING_DIR_TEMP_INODE_LOC;
    writeBlock(Inode->i_block[0], (uint8_t *)KERNEL_WORKING_DIR, cacheActive);

    deleteDirectoryEntry(fileName, cacheActive, getInodeFromPath(sourceDirectory, cacheActive));
}

void changeFileMode(uint8_t *fileName, uint16_t newMode, uint32_t currentPid, bool cacheActive, uint32_t directoryInode)
{
    // This code was written with Grok, an AI by xAI, based on my guidance and specifications.
    // 12/2025 with Grok v4.
    
    uint32_t inodeNumber = returnInodeofFileName(fileName, cacheActive, directoryInode);
    if (inodeNumber == 0)
    {
        return;
    }

    struct blockGroupDescriptor *BlockGroupDescriptor = (blockGroupDescriptor*)(BLOCK_GROUP_DESCRIPTOR_TABLE);
    uint32_t inode_index = inodeNumber - 1;
    uint32_t inodes_per_block = BLOCK_SIZE / INODE_SIZE;
    uint32_t block_offset = inode_index / inodes_per_block;
    uint32_t inode_block = BlockGroupDescriptor->bgd_starting_block_of_inode_table + block_offset;

    uint8_t *inode_block_buffer = EXT2_TEMP_INODE_STRUCTS;
    if (inode_block_buffer == 0)
    {
        return;
    }

    readBlock(inode_block, inode_block_buffer, cacheActive);

    uint32_t offset = (inode_index % inodes_per_block) * INODE_SIZE;
    struct inode *Inode = (struct inode *)(inode_block_buffer + offset);
    Inode->i_mode = (Inode->i_mode & 0xF000) | (newMode & 0x0FFF);

    writeBlock(inode_block, inode_block_buffer, cacheActive);

}

uint32_t getInodeFromPath(uint8_t *path, bool cacheActive) 
{
    // This code was written with Grok, an AI by xAI, based on my guidance and specifications.
    // 12/2025 with Grok v4.
    
    uint32_t currentInode = ROOTDIR_INODE;  // Start from root inode (2)

    if (*path != '/') {
        return 0;  // Assume absolute path starting with '/'
    }
    path++;  // Skip the leading '/'

    while (*path != '\0') {
        uint8_t name[256];  // Buffer for component name (assuming max filename length < 256)
        uint32_t len = 0;

        // Extract the next path component
        uint8_t *componentStart = path;
        while (*path != '\0' && *path != '/') {
            path++;
            len++;
        }

        if (len == 0) {
            // Skip multiple slashes (e.g., "//")
            if (*path == '/') {
                path++;
            }
            continue;
        }

        if (len >= 256) {
            return 0;  // Component too long
        }

        // Copy the component to the name buffer and null-terminate
        bytecpy(name, componentStart, len);
        name[len] = '\0';

        // Find the inode of this component in the current directory
        uint32_t nextInode = returnInodeofFileName(name, cacheActive, currentInode);
        if (nextInode == 0) {
            return 0;  // Not found
        }

        // If there is more path to traverse, check if this is a directory
        if (*path != '\0') {
            uint8_t inodeBuf[INODE_SIZE];
            loadInode(nextInode, inodeBuf, cacheActive);
            struct inode *inode = (struct inode *)inodeBuf;

            // Check if it's a directory (EXT2 directory mode: 0x4000)
            if ((inode->i_mode & 0xF000) != 0x4000) {
                return 0;  // Not a directory
            }
        }

        currentInode = nextInode;

        // Skip the '/' if present
        if (*path == '/') {
            path++;
        }
    }

    return currentInode;
}