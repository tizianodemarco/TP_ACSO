#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{	
	int start, status, pid, n;
	int val;

	if (argc != 4){ printf("Uso: anillo <n> <c> <s> \n"); exit(0);}
    
    /* Parsing of arguments */
  	n = atoi(argv[1]); 
	val = atoi(argv[2]);
	start = atoi(argv[3]);
    printf("Se crearán %i procesos, se enviará el caracter %i desde proceso %i \n", n, val, start);
    
   	int pipes[n][2];  	// pipes para conectar cada proceso con el siguiente
	int parent_pipe[2];	// pipe para volver al padre 

	for (int i = 0; i < n; i++) {	// crear todos los pipes antes del fork
		if (pipe(pipes[i]) == -1) {
			perror("pipe");
			exit(1);
		}
	}

	if (pipe(parent_pipe) == -1) {
        perror("pipe padre");
        exit(1);
    }

    for (int i = 0; i < n; i++) {	// crear los procesos hijos
        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            // Cerrar todos los pipes que no usa este proceso
            for (int j = 0; j < n; j++) {
                if (j != i) close(pipes[j][0]);
                if (j != (i + 1) % n) close(pipes[j][1]);
            }

            close(parent_pipe[0]);
            if (!(i == (start + n - 1) % n)) {
                close(parent_pipe[1]);
            }

            int value;
            read(pipes[i][0], &value, sizeof(int));
            close(pipes[i][0]); // cerrar después de leer
            value++;

            if (i == (start + n - 1) % n) {
                write(parent_pipe[1], &value, sizeof(int));
                close(parent_pipe[1]);
            } else {
                write(pipes[(i + 1) % n][1], &value, sizeof(int));
                close(pipes[(i + 1) % n][1]);
            }

            exit(0);
        }
    }

    write(pipes[start][1], &val, sizeof(int));     // proceso padre: Envía el valor inicial al proceso `start`
    close(pipes[start][1]);

    for (int i = 0; i < n; i++) {	// cierra todos los pipes que no va a usar
        close(pipes[i][0]);
        if (i != start) close(pipes[i][1]);
    }

    close(parent_pipe[1]);

    read(parent_pipe[0], &val, sizeof(int));  // lee el valor final
    close(parent_pipe[0]);

    printf("Valor final recibido en el proceso padre: %d\n", val);

    for (int i = 0; i < n; i++) {
        wait(&status);
    }

    return 0;
}
