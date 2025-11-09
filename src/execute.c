#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>   // Needed for strcmp
#include "shell.h"

// Execute command with I/O redirection and pipes
int execute(char **args) {
    int i;
    int input_redir = -1, output_redir = -1, pipe_pos = -1;

    // Detect <, >, |
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) input_redir = i;
        else if (strcmp(args[i], ">") == 0) output_redir = i;
        else if (strcmp(args[i], "|") == 0) pipe_pos = i;
    }

    // Handle simple pipe
    if (pipe_pos != -1) {
        args[pipe_pos] = NULL;
        int fd[2];
        if (pipe(fd) == -1) { perror("pipe"); return 1; }

        pid_t pid1 = fork();
        if (pid1 == 0) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            execvp(args[0], args);
            perror("myshell"); exit(EXIT_FAILURE);
        }

        pid_t pid2 = fork();
        if (pid2 == 0) {
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            execvp(args[pipe_pos + 1], &args[pipe_pos + 1]);
            perror("myshell"); exit(EXIT_FAILURE);
        }

        close(fd[0]); close(fd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        return 1;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process

        // Input redirection
        if (input_redir != -1) {
            int fd_in = open(args[input_redir + 1], O_RDONLY);
            if (fd_in < 0) { perror("open input"); exit(EXIT_FAILURE); }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
            args[input_redir] = NULL;
        }

        // Output redirection
        if (output_redir != -1) {
            int fd_out = open(args[output_redir + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) { perror("open output"); exit(EXIT_FAILURE); }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
            args[output_redir] = NULL;
        }

        execvp(args[0], args);
        perror("myshell");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        wait(NULL);
    } else {
        perror("fork failed");
    }

    return 1;
}

