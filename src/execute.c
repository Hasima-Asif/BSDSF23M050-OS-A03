#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "shell.h"

// Execute external commands
int execute(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(args[0], args);
        perror("myshell");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process waits
        wait(NULL);
    } else {
        perror("fork failed");
        return -1;
    }
    return 0;
}

