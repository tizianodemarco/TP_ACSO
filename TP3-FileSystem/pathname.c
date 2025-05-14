
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ROOT_INUMBER 1      // Inodo correspondiente al directorio raíz
#define DIR_NAME_LEN 14     // Longitud máxima del nombre de un directorio o archivo

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (fs == NULL || pathname == NULL || pathname[0] != '/') {
        return -1;  // Error en el pathname
    }
   if (strcmp(pathname, "/") == 0) {
        return ROOT_INUMBER; // Si el pathname es solo "/" se devuelve el inodo raíz
    }
    int current_inumber = ROOT_INUMBER;
    const char *start = pathname + 1;
    char component[DIR_NAME_LEN + 1];

    while (*start != '\0') {
        const char *end = strchr(start, '/');
        if (end == NULL) {
            end = start + strlen(start);    // Si no hay '/', tomamos hasta el final del string
        }
        size_t length = end - start;
        if (length == 0 || length > DIR_NAME_LEN) {
            return -1;  // Error por nombre vacío o demasiado largo
        }
        strncpy(component, start, length);
        component[length] = '\0';

        struct direntv6 entry;
        if (directory_findname(fs, component, current_inumber, &entry) != 0) {
            return -1;  // No se encontró el nombre
        }
        current_inumber = entry.d_inumber;
        start = (*end == '/') ? end + 1 : end;
    }
    return current_inumber;
}
