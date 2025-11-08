#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "shell.h"

// Function to handle built-in commands
int handle_builtin(char **args) {
    if (args[0] == NULL)
        return 0;  // Empty command

    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting MyShell...\n");
        exit(0);
    } 
    else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: expected argument\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd failed");
            }
        }
        return 1;
    } 
    else if (strcmp(args[0], "help") == 0) {
        printf("MyShell Built-ins:\n");
        printf("  exit   - Quit shell\n");
        printf("  cd     - Change directory\n");
        printf("  help   - List built-in commands\n");
        printf("  jobs   - Job control not yet implemented\n");
        return 1;
    } 
    else if (strcmp(args[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    return 0;  // Not a builtin
}

// Function to parse command line into args
void parse_command(char *command, char **args) {
    int i = 0;
    args[i] = strtok(command, " \n\t");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \n\t");
    }
}

// Main shell function
void myshell() {
    char command[1024];
    char *args[64];
    printf("Welcome to MyShell v2.0\n");

    while (1) {
        printf("myshell> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\n");
            break;
        }

        parse_command(command, args);

        // Check if it's a built-in command
        if (handle_builtin(args))
            continue;

        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            execvp(args[0], args);
            perror("myshell");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            wait(NULL);
        } else {
            perror("fork failed");
        }
    }
}

