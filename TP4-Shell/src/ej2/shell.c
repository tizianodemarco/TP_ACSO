#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 256

int main() {

    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;

    while (1) 
    {
        if (isatty(STDIN_FILENO)) { 
            printf("Shell> ");
        }
        
        // command_count = 0;

        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        // validar comillas dobles abiertas sin cerrar
        int quote_count = 0;
        for (size_t i = 0; i < strlen(command); i++) {
            if (command[i] == '"') quote_count++;
        }
        if (quote_count % 2 != 0) {
            fprintf(stderr, "Syntax error: unmatched quotes\n");
            continue;  // salta esta línea sin ejecutar
        }

        // validar de errores de sitaxis
        int len = strlen(command);
        if (len == 0) continue;

        // pipe al principio o final
        if (command[0] == '|' || command[len - 1] == '|') {
            fprintf(stderr, "Syntax error\n");
            continue;
        }

        // pipe vacío (como: ls | | wc) o doble pipe (echo hola || wc)
        int last_was_pipe = 0;
        for (int i = 0; i < len; i++) {
            if (command[i] == '|') {
                if (last_was_pipe) {
                    fprintf(stderr, "Syntax error\n");
                    goto continue_loop;
                    continue_loop:
                        continue;
                }
                last_was_pipe = 1;
            } else if (command[i] != ' ' && command[i] != '\t') {
                last_was_pipe = 0;
            }
        }

        void split_commands(char *line, char **commands, int *count);
        split_commands(command, commands, &command_count);

        /* You should start programming from here... */
        for (int i = 0; i < command_count; i++) {
            if (isatty(STDIN_FILENO)) { 
            printf("Command %d: %s\n", i, commands[i]);
            }
        }
        
        // validaciones de sintaxis
        if (command_count == 0 || commands[0][0] == '\0') continue;
        if (command[0] == '|' || command[strlen(command) - 1] == '|') {
            fprintf(stderr, "Syntax error\n");
            command_count = 0;
            continue;
        }

        int syntax_error = 0;
        for (int i = 0; i < command_count; i++) {
            if (strlen(commands[i]) == 0) {
                syntax_error = 1;
                break;
            }
        }
        if (syntax_error) {
            fprintf(stderr, "Syntax error\n");
            command_count = 0;
            continue;
        }

        // detectar comando "exit"
        if (command_count == 1 && strcmp(commands[0], "exit") == 0) {
            break;
        }

        int pipes[2 * (command_count - 1)];
        for (int i = 0; i < command_count - 1; i++) {
            if (pipe(pipes + i * 2) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pids[command_count];

        for (int i = 0; i < command_count; i++) {
            // tokenizar cada comando por espacios
            char *args[256];
            int arg_count = 0;
            void parse_args(char *line, char **args, int *arg_count);
            parse_args(commands[i], args, &arg_count);
            args[arg_count] = NULL;

            if (arg_count >= MAX_ARGS) {
                fprintf(stderr, "Too many arguments\n");
                // no ejecutar nada
                command_count = 0;
                goto end_loop;
                end_loop:
                    command_count = 0;
                    continue;
            }

            pids[i] = fork();
            if (pids[i] < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pids[i] == 0) {
                // redirección de stdin y stdout
                if (i > 0) {
                    dup2(pipes[(i - 1) * 2], STDIN_FILENO);
                }
                if (i < command_count - 1) {
                    dup2(pipes[i * 2 + 1], STDOUT_FILENO);
                }

                // cerramos todos los pipes
                for (int j = 0; j < 2 * (command_count - 1); j++) {
                    close(pipes[j]);
                }

                execvp(args[0], args);
                perror("command not found");
                exit(EXIT_FAILURE);
            }
        }

        // cerramos todos los pipes en el padre
        for (int i = 0; i < 2 * (command_count - 1); i++) {
            close(pipes[i]);
        }

        // esperamos a todos los hijos
        for (int i = 0; i < command_count; i++) {
            waitpid(pids[i], NULL, 0);
        }

        // reset para el siguiente ciclo
        command_count = 0;
    }

    return 0;
}

void parse_args(char *line, char **args, int *arg_count) {
    int i = 0;
    char *p = line;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        if (*p == '"') {
            p++;
            args[i++] = p;
            while (*p && *p != '"') p++;
            if (*p) {
                *p = '\0';
                p++;
            }
        } else {
            args[i++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) {
                *p = '\0';
                p++;
            }
        }
    }
    args[i] = NULL;
    *arg_count = i;
}

void split_commands(char *line, char **commands, int *count) {
    *count = 0;
    char *start = line;
    int in_quotes = 0;

    for (char *p = line; ; ++p) {
        if (*p == '"') {
            in_quotes = !in_quotes;
        } else if (*p == '|' && !in_quotes) {
            *p = '\0';
            commands[(*count)++] = start;
            start = p + 1;
        } else if (*p == '\0') {
            commands[(*count)++] = start;
            break;
        }
    }
}
