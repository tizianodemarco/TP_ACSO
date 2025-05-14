#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    if (fs == NULL || buf == NULL || inumber <= 0 || blockNum < 0) {
        return -1; // Error en los parámetros
    }
    struct inode inode;
    if (inode_iget(fs, inumber, &inode) == -1) {
        return -1;  // Error al obtener el inodo
    }
    if ((inode.i_mode & IALLOC) == 0) {
        return -1;  // El inodo no está asignado
    }
    int sector = inode_indexlookup(fs, &inode, blockNum);
    if (sector == -1) {
        return -1;  // Error al buscar el sector
    }
    if (diskimg_readsector(fs->dfd, sector, buf) == -1) {
        return -1;  // Error al leer el sector
    }
    int filesize = inode_getsize(&inode);
    int start = blockNum * DISKIMG_SECTOR_SIZE;
    // Si el bloque esta completamente dentro del archivo, devuelve el tamaño del bloque
    if (filesize > start + DISKIMG_SECTOR_SIZE) {
        return DISKIMG_SECTOR_SIZE;
    // Si es el último bloque parcial, devuelve solo los bytes válidos
    } else if (filesize > start) {
        return filesize - start;
    // Si el bloque no está dentro del archivo, no hay nada que leer
    } else {
        return 0;
    }
}
