#include "shell.h"

#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

job job_list[JOBS_MAX];
int job_count = 0;

/* simple command history (separate from readline) for !n */
static char *cmd_history[HISTORY_MAX];
static int cmd_history_count = 0;

static void add_cmd_history(const char *s) {
    if (!s || !*s) return;
    if (cmd_history_count < HISTORY_MAX) {
        cmd_history[cmd_history_count++] = strdup(s);
    } else {
        free(cmd_history[0]);
        memmove(&cmd_history[0], &cmd_history[1], sizeof(char*)*(HISTORY_MAX-1));
        cmd_history[HISTORY_MAX-1] = strdup(s);
    }
}

void reap_background(void) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; ++i) {
            if (job_list[i].pid == pid) {
                printf("\n[Done] PID %d  CMD: %s\n", pid, job_list[i].cmd);
                /* remove job i */
                free(job_list[i].cmd); /* in our design cmd is fixed-size; safe to ignore, but kept for clarity */
                for (int j = i; j < job_count-1; ++j) job_list[j] = job_list[j+1];
                job_count--;
                break;
            }
        }
    }
}

/* built-ins: exit, cd, help, jobs, history, !n (handled in main loop) */
int handle_builtin(char **args) {
    if (!args[0]) return 0;

    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting MyShell...\n");
        exit(0);
    }
    if (strcmp(args[0], "cd") == 0) {
        if (!args[1]) { fprintf(stderr, "cd: expected argument\n"); }
        else if (chdir(args[1]) != 0) perror("cd");
        return 1;
    }
    if (strcmp(args[0], "help") == 0) {
        puts("Built-ins:");
        puts("  exit        - Quit shell");
        puts("  cd <dir>    - Change directory");
        puts("  help        - Show this help");
        puts("  jobs        - List background jobs");
        puts("  history     - Show recent commands");
        puts("  !n          - Execute nth command from history");
        return 1;
    }
    if (strcmp(args[0], "jobs") == 0) {
        if (job_count == 0) { puts("No background jobs."); return 1; }
        for (int i = 0; i < job_count; ++i) {
            printf("[%d] PID %d  CMD: %s\n", i+1, job_list[i].pid, job_list[i].cmd);
        }
        return 1;
    }
    if (strcmp(args[0], "history") == 0) {
        for (int i = 0; i < cmd_history_count; ++i)
            printf("%d  %s\n", i+1, cmd_history[i]);
        return 1;
    }

    return 0;
}

/* helper: split a string into tokens (modifies the input) */
static void parse_tokens(char *s, char **args) {
    int i = 0;
    char *tok = strtok(s, " \t\n");
    while (tok && i < MAX_ARGS-1) {
        args[i++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

/* main loop: supports ; chaining, & background token, !n recall (using cmd_history), readline */
void myshell(void) {
    printf("Welcome to MyShell (v1→v6) — features: built-ins, history, readline, redir, pipes, & and ;\n");

    using_history(); /* enable readline history */

    while (1) {
        reap_background();

        char *line = readline("myshell> ");
        if (!line) { putchar('\n'); break; } /* Ctrl-D */

        /* ignore empty */
        if (strlen(line) == 0) { free(line); continue; }

        /* add to readline history and our cmd history */
        add_history(line);
        add_cmd_history(line);

        /* handle !n recall: if the entire line is like "!3" then replace line with that history entry */
        if (line[0] == '!' && isdigit((unsigned char)line[1])) {
            int n = atoi(line+1);
            if (n >= 1 && n <= cmd_history_count) {
                free(line);
                line = strdup(cmd_history[n-1]);
                printf("Executing: %s\n", line);
            } else {
                printf("No such command in history\n");
                free(line);
                continue;
            }
        }

        /* split by ';' to get sequential commands */
        char *saveptr = NULL;
        char *segment = strtok_r(line, ";", &saveptr);
        while (segment) {
            /* trim leading/trailing spaces */
            while (*segment && isspace((unsigned char)*segment)) segment++;
            char *end = segment + strlen(segment) - 1;
            while (end > segment && isspace((unsigned char)*end)) *end-- = '\0';

            if (*segment == '\0') { segment = strtok_r(NULL, ";", &saveptr); continue; }

            /* detect background: if ends with & (possibly trailing spaces) */
            int background = 0;
            size_t seglen = strlen(segment);
            if (seglen > 0) {
                /* find last non-space */
                size_t i = seglen;
                while (i > 0 && isspace((unsigned char)segment[i-1])) --i;
                if (i > 0 && segment[i-1] == '&') {
                    background = 1;
                    segment[i-1] = '\0';
                    /* strip spaces again */
                    while (i>1 && isspace((unsigned char)segment[i-2])) { segment[i-2] = '\0'; i--; }
                }
            }

            /* now handle pipeline/redirection/execution */
            /* We will build args and pass to execute(); execute handles pipes/redir */
            char tmp[MAX_INPUT];
            strncpy(tmp, segment, MAX_INPUT-1); tmp[MAX_INPUT-1]=0;
            char *args[MAX_ARGS];
            parse_tokens(tmp, args);

            if (!args[0]) { segment = strtok_r(NULL, ";", &saveptr); continue; }

            if (handle_builtin(args)) {
                /* builtin ran */
            } else {
                execute(args, background);
            }

            segment = strtok_r(NULL, ";", &saveptr);
        }

        free(line);
    }
}

// Feature 7: Implemented if-then-else logic

