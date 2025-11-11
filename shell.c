#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include "shell.h"

#define HISTORY_SIZE 20

char *history[HISTORY_SIZE];
int history_count = 0;

// Add command to history
void add_history_cmd(const char *cmd) {
    if (cmd == NULL || strlen(cmd) == 0) return;

    // Remove oldest if history full
    if (history_count == HISTORY_SIZE) {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++)
            history[i-1] = history[i];
        history_count--;
    }
    history[history_count++] = strdup(cmd);
}

// Display history
void show_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d  %s\n", i+1, history[i]);
    }
}

// Function to handle built-in commands
int handle_builtin(char **args) {
    if (args[0] == NULL)
        return 0;

    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting MyShell...\n");
        exit(0);
    } 
    else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: expected argument\n");
        } else {
            if (chdir(args[1]) != 0)
                perror("cd failed");
        }
        return 1;
    } 
    else if (strcmp(args[0], "help") == 0) {
        printf("MyShell Built-ins:\n");
        printf("  exit   - Quit shell\n");
        printf("  cd     - Change directory\n");
        printf("  help   - List built-in commands\n");
        printf("  jobs   - Job control not yet implemented\n");
        printf("  history - Show last commands\n");
        printf("  !n      - Execute nth command from history\n");
        return 1;
    } 
    else if (strcmp(args[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }
    else if (strcmp(args[0], "history") == 0) {
        show_history();
        return 1;
    }

    return 0;
}

// Parse command into args array
void parse_command(char *command, char **args) {
    int i = 0;
    args[i] = strtok(command, " \t\n");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }
}

// Main shell function
void myshell() {
    char command[1024];
    char *args[64];

    printf("Welcome to MyShell v1.0 (Base) / v2.0 (Built-ins) / v3.0 (History)\n");

    while (1) {
        printf("myshell> ");
        if (!fgets(command, sizeof(command), stdin))
            break;

        // Remove newline
        command[strcspn(command, "\n")] = 0;

        // Skip empty command
        if (strlen(command) == 0) continue;

        // Handle !n history recall
        if (command[0] == '!' && isdigit(command[1])) {
            int idx = atoi(&command[1]) - 1;
            if (idx >= 0 && idx < history_count) {
                printf("Executing: %s\n", history[idx]);
                strcpy(command, history[idx]); // replace command with actual
            } else {
                printf("No such command in history\n");
                continue;
            }
        }

        // Add to history (after handling !n)
        add_history_cmd(command);

        // Parse command
        parse_command(command, args);

        // Check built-ins
        if (handle_builtin(args)) continue;

        // Fork and execute
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                perror("myshell");
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("myshell: fork error");
        } else {
            wait(NULL);
        }
    }
}

