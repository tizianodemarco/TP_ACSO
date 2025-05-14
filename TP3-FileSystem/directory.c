#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name,
		int dirinumber, struct direntv6 *dirEnt) {
      if (fs == NULL || name == NULL || dirEnt == NULL || dirinumber <= 0) {
        return -1;  // error en los parámetros
    }
    size_t namelen = strlen(name);
    if (namelen > sizeof(dirEnt->d_name)) {
        return -1;  // valida el largo del nombre
    }
    // Obtener el inode del directorio
    struct inode dir_inode;
    if (inode_iget(fs, dirinumber, &dir_inode) == -1) {
        return -1;  // error al obtener el inode del directorio
    }
    // Asegurarse de que el inode sea un directorio
    if ((dir_inode.i_mode & IFMT) != IFDIR) {
        return -1;  // no es un directorio
    }
    // Leer los bloques del directorio
    int size = inode_getsize(&dir_inode);
    int num_blocks = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    for (size_t i = 0; i < (size_t)num_blocks; i++) {
        // Leer el bloque de datos del directorio
        char buf[DISKIMG_SECTOR_SIZE];
        if (file_getblock(fs, dirinumber, i, buf) == -1) {
            return -1;  // error al leer el bloque del directorio
        }
        // Buscar el nombre en las entradas del directorio
        for (size_t j = 0; j < DISKIMG_SECTOR_SIZE / sizeof(struct direntv6); j++) {
            struct direntv6 *entry = (struct direntv6 *)(buf + j * sizeof(struct direntv6));

            // Comprobar si la entrada es válida y si el nombre coincide
            if (entry->d_inumber != 0 && 
                strncmp(entry->d_name, name, sizeof(entry->d_name)) == 0 &&
                namelen == strnlen(entry->d_name, sizeof(entry->d_name))) {
                *dirEnt = *entry;  // Copiar la entrada encontrada
                return 0;  // Encontrado
            }
        }
    }
    return -1;  // No encontrado
}

int directory_lookup(struct unixfilesystem *fs, int dirinumber, const char *name) {
  struct direntv6 dir_entry;
  int result = directory_findname(fs, name, dirinumber, &dir_entry);
  if (result == 0) {
      return dir_entry.d_inumber;  // Devuelve el inumber de la entrada encontrada
  }
  return -1;  // No encontrado
}
