#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    if (fs == NULL || buf == NULL || inumber <= 0 || blockNum < 0) {
        return -1;
    }
    struct inode inode;
    if (inode_iget(fs, inumber, &inode) == -1) {
        return -1;
    }
    if ((inode.i_mode & IALLOC) == 0) {
        return -1;
    }
    int sector = inode_indexlookup(fs, &inode, blockNum);
    if (sector == -1) {
        return -1;
    }
    if (diskimg_readsector(fs->dfd, sector, buf) == -1) {
        return -1;
    }
    int filesize = inode_getsize(&inode);
    int start = blockNum * DISKIMG_SECTOR_SIZE;
    if (filesize > start + DISKIMG_SECTOR_SIZE) {
        return DISKIMG_SECTOR_SIZE;
    } else if (filesize > start) {
        return filesize - start;
    } else {
        return 0;
    }
}


