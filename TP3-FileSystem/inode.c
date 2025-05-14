#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (fs == NULL || inp == NULL || inumber <= 0) {
        return -1;
    }
    int inodes_per_block = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int total_inodes = fs->superblock.s_isize * inodes_per_block;
    if (inumber >= total_inodes + 1) {
        return -1;
    }
    int sector = INODE_START_SECTOR + (inumber - 1) / inodes_per_block;
    int offset = (inumber - 1) % inodes_per_block;
    char buffer[DISKIMG_SECTOR_SIZE];
    if (diskimg_readsector(fs->dfd, sector, buffer) == -1) {
        return -1;
    }
    struct inode *inodeBlock = (struct inode *)buffer;
    *inp = inodeBlock[offset];
    return 0;
}    

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {  
    if (fs == NULL || inp == NULL || blockNum < 0) {
        return -1;
    }
    int largeFile = (inp->i_mode & ILARG);
    if (!largeFile) {
        if (blockNum < 8) {
            return inp->i_addr[blockNum] ? inp->i_addr[blockNum] : -1;
        } else {
            return -1;
        }
    } else {
        if (blockNum < 7 * 256) {
            int indirectBlockIndex = blockNum / 256;
            int indirectEntryIndex = blockNum % 256;

            if (inp->i_addr[indirectBlockIndex] == 0) {
                return -1;
            }
            uint16_t indirectBlock[256];
            if (diskimg_readsector(fs->dfd, inp->i_addr[indirectBlockIndex], (char *)indirectBlock) == -1) {
                return -1;
            }
            return indirectBlock[indirectEntryIndex] ? indirectBlock[indirectEntryIndex] : -1;
        } else if (blockNum < 7 * 256 + 256 * 256) {
            int doubleIndirectEntry = (blockNum - 7 * 256) / 256;
            int indirectEntryIndex = (blockNum - 7 * 256) % 256;
            if (inp->i_addr[7] == 0) {
                return -1;
            }
            uint16_t doubleIndirectBlock[256];
            if (diskimg_readsector(fs->dfd, inp->i_addr[7], (char *)doubleIndirectBlock) == -1) {
                return -1;
            }
            if (doubleIndirectBlock[doubleIndirectEntry] == 0) {
                return -1;
            }
            uint16_t indirectBlock[256];
            if (diskimg_readsector(fs->dfd, doubleIndirectBlock[doubleIndirectEntry], (char *)indirectBlock) == -1) {
                return -1;
            }
            return indirectBlock[indirectEntryIndex] ? indirectBlock[indirectEntryIndex] : -1;
        } else {
            return -1;
        }
    }
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}

