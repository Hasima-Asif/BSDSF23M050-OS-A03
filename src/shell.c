#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"

#define HISTORY_SIZE 20

// Built-in commands handler
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
        HIST_ENTRY **hist_list = history_list();
        if (hist_list) {
            for (int i = 0; hist_list[i]; i++)
                printf("%d  %s\n", i + 1, hist_list[i]->line);
        }
        return 1;
    }

    return 0;
}

// Parse command line into args array
void parse_command(char *command, char **args) {
    int i = 0;
    args[i] = strtok(command, " \t\n");
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }
}

// Main shell loop
void myshell() {
    char *line;
    char *args[128];

    printf("Welcome to MyShell v1.0 (Base) / v2.0 (Built-ins) / v3.0 (History) / v5.0 (I/O & Pipes)\n");

    using_history(); // Enable readline history

    while (1) {
        line = readline("myshell> ");
        if (!line) { printf("\n"); break; }

        if (strlen(line) > 0)
            add_history(line); // Add to history

        // Handle !n history execution
        if (line[0] == '!' && strlen(line) > 1) {
            int cmd_num = atoi(&line[1]);
            HIST_ENTRY **hist_list = history_list();
            if (hist_list && cmd_num > 0 && hist_list[cmd_num - 1])
                line = strdup(hist_list[cmd_num - 1]->line);
            else {
                printf("No such command in history\n");
                free(line);
                continue;
            }
        }

        parse_command(line, args);

        if (handle_builtin(args)) {
            free(line);
            continue;
        }

        if (args[0] != NULL)
            execute(args); // execute with I/O redirection and pipes

        free(line);
    }
}

