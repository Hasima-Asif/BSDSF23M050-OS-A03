#include "shell.h"

#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

extern job job_list[JOBS_MAX];
extern int job_count;

/* split args into segments separated by '|' — returns number of segments,
   and fills segments array with pointers into a local copy (caller must free copy) */
static int split_pipeline(char **args, char ***segments_out, char **args_copy) {
    /* create a flat string copy and recreate argv tokens in it */
    int total = 0;
    /* build a joined command from args for easier splitting */
    size_t bufflen = 0;
    for (int i = 0; args[i]; ++i) bufflen += strlen(args[i]) + 1;
    *args_copy = malloc(bufflen + 1);
    (*args_copy)[0] = '\0';
    for (int i = 0; args[i]; ++i) {
        strcat(*args_copy, args[i]);
        if (args[i+1]) strcat(*args_copy, " ");
    }

    /* temporary array of pointers */
    char *tokens[MAX_ARGS];
    int ti = 0;
    char *tok = strtok(*args_copy, " ");
    while (tok && ti < MAX_ARGS-1) {
        tokens[ti++] = tok;
        tok = strtok(NULL, " ");
    }
    tokens[ti] = NULL;

    /* now locate pipe positions and build segments: point segments to tokens */
    char **segments = malloc(sizeof(char*) * (MAX_ARGS));
    int seg = 0;
    segments[seg++] = tokens[0];
    for (int i = 0; tokens[i]; ++i) {
        if (strcmp(tokens[i], "|") == 0) {
            tokens[i] = NULL;
            if (tokens[i+1]) segments[seg++] = tokens[i+1];
        }
    }
    segments[seg] = NULL;
    *segments_out = segments;
    return seg;
}

/* For a given argv (null-terminated), detect < and > and return infile/outfile, and
   remove those tokens (by setting them to NULL). Returns 0 on success. */
static void detect_redirection_in_args(char **argv, char **infile, char **outfile, int *append) {
    *infile = NULL; *outfile = NULL; *append = 0;
    for (int i = 0; argv[i]; ++i) {
        if (strcmp(argv[i], "<") == 0 && argv[i+1]) {
            *infile = argv[i+1];
            argv[i] = NULL;
        } else if (strcmp(argv[i], ">") == 0 && argv[i+1]) {
            *outfile = argv[i+1];
            *append = 0;
            argv[i] = NULL;
        } else if (strcmp(argv[i], ">>") == 0 && argv[i+1]) {
            *outfile = argv[i+1];
            *append = 1;
            argv[i] = NULL;
        }
    }
}

/* Build argv array by tokenizing a segment string (which points into args_copy) */
static void build_argv_from_segment(char *seg_str, char **argv) {
    int i = 0;
    char *tok = strtok(seg_str, " \t\n");
    while (tok && i < MAX_ARGS-1) {
        argv[i++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    argv[i] = NULL;
}

/* Execute supports pipelines of arbitrary length (cmd1 | cmd2 | ...),
   with redirection in individual commands, and background flag. */
int execute(char **args, int background) {
    if (!args || !args[0]) return 1;

    /* Create a single string copy of the whole command for easier splitting */
    /* We'll build a joined string similar to shell.c behavior */
    size_t bufsize = 0;
    for (int i = 0; args[i]; ++i) bufsize += strlen(args[i]) + 1;
    char *joined = malloc(bufsize + 1);
    joined[0] = '\0';
    for (int i = 0; args[i]; ++i) {
        strcat(joined, args[i]);
        if (args[i+1]) strcat(joined, " ");
    }

    /* Count pipeline segments by splitting on '|' in a copy */
    /* We'll parse segments by splitting joined on '|' */
    char *segs_buf = strdup(joined);
    int seg_count = 0;
    char *saveptr = NULL;
    char *seg = strtok_r(segs_buf, "|", &saveptr);
    char *segments[64];
    while (seg && seg_count < 64) {
        /* trim leading/trailing spaces */
        while (*seg && isspace((unsigned char)*seg)) seg++;
        char *end = seg + strlen(seg) - 1;
        while (end > seg && isspace((unsigned char)*end)) *end-- = '\0';
        segments[seg_count++] = strdup(seg);
        seg = strtok_r(NULL, "|", &saveptr);
    }

    if (seg_count == 0) { free(joined); free(segs_buf); return 1; }

    /* if only one segment: simple execution with redirection */
    if (seg_count == 1) {
        char *argv[MAX_ARGS];
        build_argv_from_segment(segments[0], argv);

        /* detect redirection */
        char *infile = NULL, *outfile = NULL;
        int append = 0;
        detect_redirection_in_args(argv, &infile, &outfile, &append);

        pid_t pid = fork();
        if (pid == 0) {
            if (infile) {
                int fd = open(infile, O_RDONLY);
                if (fd < 0) { perror("open infile"); exit(EXIT_FAILURE); }
                dup2(fd, STDIN_FILENO); close(fd);
            }
            if (outfile) {
                int fd;
                if (append) fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open outfile"); exit(EXIT_FAILURE); }
                dup2(fd, STDOUT_FILENO); close(fd);
            }
            execvp(argv[0], argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            if (background) {
                if (job_count < JOBS_MAX) {
                    job_list[job_count].pid = pid;
                    strncpy(job_list[job_count].cmd, joined, 255);
                    job_count++;
                }
                printf("[Background] PID %d\n", pid);
            } else {
                waitpid(pid, NULL, 0);
            }
        } else {
            perror("fork");
        }

        /* cleanup */
        free(joined);
        for (int i = 0; i < seg_count; ++i) free(segments[i]);
        free(segs_buf);
        return 1;
    }

    /* Multiple pipeline segments: create N-1 pipes and fork N children */
    int n = seg_count;
    int pipefds[2*(n-1)];
    for (int i = 0; i < n-1; ++i) {
        if (pipe(pipefds + i*2) < 0) { perror("pipe"); return 1; }
    }

    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            /* input redir: if not first, dup read end of previous pipe */
            if (i > 0) {
                dup2(pipefds[(i-1)*2], STDIN_FILENO);
            }
            /* output redir: if not last, dup write end of this pipe */
            if (i < n-1) {
                dup2(pipefds[i*2 + 1], STDOUT_FILENO);
            }

            /* close all pipe fds in child */
            for (int j = 0; j < 2*(n-1); ++j) close(pipefds[j]);

            /* for each segment, detect its own redir tokens */
            char *argv[MAX_ARGS];
            build_argv_from_segment(segments[i], argv);
            char *infile = NULL, *outfile = NULL;
            int append = 0;
            detect_redirection_in_args(argv, &infile, &outfile, &append);

            if (infile) {
                int fd = open(infile, O_RDONLY);
                if (fd < 0) { perror("open infile"); exit(EXIT_FAILURE); }
                dup2(fd, STDIN_FILENO); close(fd);
            }
            if (outfile) {
                int fd;
                if (append) fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror("open outfile"); exit(EXIT_FAILURE); }
                dup2(fd, STDOUT_FILENO); close(fd);
            }

            execvp(argv[0], argv);
            perror("execvp (pipeline)");
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("fork");
            /* close fds and cleanup */
            for (int j = 0; j < 2*(n-1); ++j) close(pipefds[j]);
            for (int j = 0; j < n; ++j) free(segments[j]);
            free(joined); free(segs_buf);
            return 1;
        }
        /* parent continues to next segment */
    }

    /* parent closes all pipe fds */
    for (int j = 0; j < 2*(n-1); ++j) close(pipefds[j]);

    /* wait for children */
    if (background) {
        /* set first child as tracked background job (joined command string) */
        /* Note: we don't have the PIDs of all children easily here, but typical behavior is to report the group leader or the last fork; to keep simple, not tracking each child individually; instead we track nothing or one PID; better tracking can be implemented if needed) */
        /* For simplicity, we won't add to job_list for multi-pipeline background here. */
        printf("[Background pipeline started]\n");
    } else {
        for (int i = 0; i < n; ++i) wait(NULL);
    }

    /* cleanup */
    for (int i = 0; i < n; ++i) free(segments[i]);
    free(joined);
    free(segs_buf);

    return 1;
}

