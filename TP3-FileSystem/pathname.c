
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (fs == NULL || pathname == NULL || pathname[0] != '/') {
        return -1; // solo manejamos paths absolutos
    }

    // Caso especial: root "/"
    if (strcmp(pathname, "/") == 0) {
        return 1; // inode del directorio raíz
    }

    // Copiamos pathname porque strtok lo modifica
    char pathcopy[strlen(pathname) + 1];
    strcpy(pathcopy, pathname);

    // Empezamos desde el inode del root
    int current_inumber = 1;

    // strtok necesita saltar la barra inicial
    char *token = strtok(pathcopy + 1, "/");

    while (token != NULL) {
        int next_inumber = directory_lookup(fs, current_inumber, token);
        if (next_inumber < 0) {
            return -1; // nombre no encontrado
        }
        current_inumber = next_inumber;
        token = strtok(NULL, "/");
    }

    return current_inumber;
}
