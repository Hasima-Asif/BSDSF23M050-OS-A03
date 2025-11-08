#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"

char *history[HISTORY_SIZE];
int history_count = 0;

// ---------------- Parse command ----------------
void parse_command(char *command, char **args) {
    int i = 0;
    args[i] = strtok(command, " \n\t");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \n\t");
    }
}

// ---------------- Built-in commands ----------------
int handle_builtin(char **args) {
    if (args[0] == NULL) return 0;

    if (strcmp(args[0], "exit") == 0) {
        for (int i = 0; i < history_count; i++)
            free(history[i]);
        printf("Exiting MyShell...\n");
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: expected argument\n");
        } else if (chdir(args[1]) != 0) {
            perror("cd failed");
        }
        return 1;
    } else if (strcmp(args[0], "help") == 0) {
        printf("MyShell Built-ins:\n");
        printf("  exit   - Quit shell\n");
        printf("  cd     - Change directory\n");
        printf("  help   - List built-in commands\n");
        printf("  jobs   - Job control not yet implemented\n");
        printf("  history - Show last %d commands\n", HISTORY_SIZE);
        printf("  !n     - Execute nth command from history\n");
        return 1;
    } else if (strcmp(args[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    } else if (strcmp(args[0], "history") == 0) {
        for (int i = 0; i < history_count; i++)
            printf("%d  %s\n", i + 1, history[i]);
        return 1;
    } else if (args[0][0] == '!' && strlen(args[0]) > 1) {
        int index = atoi(&args[0][1]);
        if (index < 1 || index > history_count) {
            printf("No such command in history\n");
        } else {
            printf("Executing: %s\n", history[index - 1]);
            char temp_command[1024];
            strncpy(temp_command, history[index - 1], sizeof(temp_command));
            temp_command[sizeof(temp_command) - 1] = '\0';
            parse_command(temp_command, args);
            if (!handle_builtin(args))
                execute(args);
        }
        return 1;
    }

    return 0;
}

// ---------------- Main shell loop ----------------
void myshell() {
    char command[1024];
    char *args[64];

    printf("Welcome to MyShell v3.0 with History\n");

    while (1) {
        printf("myshell> ");
        if (!fgets(command, sizeof(command), stdin)) {
            printf("\n");
            break;
        }

        command[strcspn(command, "\n")] = 0;
        if (strlen(command) == 0) continue;

        // Store in history
        if (history_count < HISTORY_SIZE) {
            history[history_count++] = strdup(command);
        } else {
            free(history[0]);
            for (int i = 1; i < HISTORY_SIZE; i++)
                history[i - 1] = history[i];
            history[HISTORY_SIZE - 1] = strdup(command);
        }

        // Parse arguments
        parse_command(command, args);

        // Built-in commands
        if (handle_builtin(args)) continue;

        // Execute external commands
        execute(args);
    }
}

