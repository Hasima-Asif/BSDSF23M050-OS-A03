#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "shell.h"

// Built-in commands handler
int handle_builtin(char **arglist) {
    if (arglist[0] == NULL) return 0;

    if (strcmp(arglist[0], "exit") == 0) {
        exit(0);  // closes the shell
    } else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(arglist[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    } else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  exit\n");
        printf("  cd <dir>\n");
        printf("  help\n");
        printf("  jobs\n");
        return 1;
    } else if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    return 0;  // not handled
}

// Main shell loop
void myshell() {
    char line[1024];
    char *args[64];

    while (1) {
        printf("myshell> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        line[strcspn(line, "\n")] = 0; // remove newline

        // Tokenize input
        int i = 0;
        char *token = strtok(line, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Handle built-in commands first
        if (handle_builtin(args)) continue;

        // Execute non-built-ins
        if (args[0] != NULL) {
            execute(line);
        }
    }
}

