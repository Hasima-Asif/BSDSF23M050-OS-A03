#include "shell.h"

// -------------------- Execute a single command --------------------
int execute_command(char **args, int background) {
    pid_t pid = fork();

    if (pid == 0) {
        // Input/output redirection
        for (int i = 0; args[i]; i++) {
            if (strcmp(args[i], "<") == 0) {
                int fd = open(args[i + 1], O_RDONLY);
                if (fd < 0) { perror("open"); exit(1); }
                dup2(fd, STDIN_FILENO); close(fd); args[i] = NULL;
            } else if (strcmp(args[i], ">") == 0) {
                int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open"); exit(1); }
                dup2(fd, STDOUT_FILENO); close(fd); args[i] = NULL;
            }
        }

        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        int status = 0;
        if (!background) waitpid(pid, &status, 0);
        else add_job(pid, args[0]);
        return status;
    } else { perror("fork"); return -1; }
}

// -------------------- Split pipeline --------------------
static int split_pipeline(char **args, char ***segments) {
    int count = 0;
    segments[count++] = args;
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL;
            segments[count++] = &args[i + 1];
        }
    }
    segments[count] = NULL;
    return count;
}

// -------------------- Execute with pipes --------------------
int execute(char **args, int background) {
    if (!args || !args[0]) return 0;

    int pipe_count = 0;
    for (int i = 0; args[i]; i++)
        if (strcmp(args[i], "|") == 0) pipe_count++;

    if (pipe_count == 0) return execute_command(args, background);

    int pipes[pipe_count][2];
    char *segments[MAX_CMDS];
    char **seg_ptrs[MAX_CMDS];
    int segs = split_pipeline(args, seg_ptrs);

    for (int i = 0; i < pipe_count; i++)
        if (pipe(pipes[i]) < 0) { perror("pipe"); return -1; }

    for (int i = 0; i <= pipe_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) dup2(pipes[i - 1][0], STDIN_FILENO);
            if (i < pipe_count) dup2(pipes[i][1], STDOUT_FILENO);
            for (int j = 0; j < pipe_count; j++) { close(pipes[j][0]); close(pipes[j][1]); }
            execvp(seg_ptrs[i][0], seg_ptrs[i]);
            perror("execvp"); exit(1);
        } else if (pid < 0) { perror("fork"); return -1; }
    }

    for (int i = 0; i < pipe_count; i++) { close(pipes[i][0]); close(pipes[i][1]); }
    for (int i = 0; i <= pipe_count; i++) wait(NULL);

    return 0;
}

// -------------------- Feature 7: if-then-else --------------------
int execute_if_block(char *condition, char *then_block, char *else_block) {
    char *cond_args[MAX_INPUT]; int idx = 0;
    char *token = strtok(condition, " ");
    while (token && idx < MAX_INPUT - 1) { cond_args[idx++] = token; token = strtok(NULL, " "); }
    cond_args[idx] = NULL;

    pid_t pid = fork();
    if (pid == 0) { execvp(cond_args[0], cond_args); perror("execvp"); exit(1); }

    int status; waitpid(pid, &status, 0);
    int exit_code = WEXITSTATUS(status);

    if (exit_code == 0 && then_block) {
        char *then_args[MAX_INPUT]; int i = 0;
        token = strtok(then_block, " ");
        while (token && i < MAX_INPUT - 1) { then_args[i++] = token; token = strtok(NULL, " "); }
        then_args[i] = NULL;
        execute(then_args, 0);
    } else if (exit_code != 0 && else_block) {
        char *else_args[MAX_INPUT]; int j = 0;
        token = strtok(else_block, " ");
        while (token && j < MAX_INPUT - 1) { else_args[j++] = token; token = strtok(NULL, " "); }
        else_args[j] = NULL;
        execute(else_args, 0);
    }

    return exit_code;
}

