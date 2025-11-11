#include "shell.h"

job job_list[JOBS_MAX];
int job_count = 0;
shell_var *var_list = NULL;

char history_list[HISTORY_MAX][MAX_INPUT];
int history_count = 0;

// -------------------- Job Handling --------------------
void add_job(pid_t pid, char *cmd) {
    if (job_count < JOBS_MAX) {
        job_list[job_count].pid = pid;
        strncpy(job_list[job_count].cmd, cmd, 255);
        job_list[job_count].active = 1;
        printf("[Background] PID: %d\n", pid);
        job_count++;
    }
}

void reap_background() {
    int status;
    for (int i = 0; i < job_count; i++) {
        if (job_list[i].active) {
            pid_t ret = waitpid(job_list[i].pid, &status, WNOHANG);
            if (ret > 0) job_list[i].active = 0;
        }
    }
}

// -------------------- Variable Handling --------------------
void set_variable(char *assignment) {
    char *eq = strchr(assignment, '=');
    if (!eq) return;
    *eq = '\0';
    char *name = assignment;
    char *value = eq + 1;

    shell_var *var = var_list;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            free(var->value);
            var->value = strdup(value);
            return;
        }
        var = var->next;
    }

    var = malloc(sizeof(shell_var));
    var->name = strdup(name);
    var->value = strdup(value);
    var->next = var_list;
    var_list = var;
}

char* get_variable(const char *name) {
    shell_var *var = var_list;
    while (var) {
        if (strcmp(var->name, name) == 0) return var->value;
        var = var->next;
    }
    return NULL;
}

void expand_variables(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (args[i][0] == '$') {
            char *val = get_variable(args[i] + 1);
            if (val) args[i] = val;
        }
    }
}

void print_variables() {
    shell_var *var = var_list;
    while (var) {
        printf("%s=%s\n", var->name, var->value);
        var = var->next;
    }
}

// -------------------- Built-ins --------------------
int handle_builtin(char **args) {
    if (!args[0]) return 0;

    if (strcmp(args[0], "cd") == 0) {
        chdir(args[1] ? args[1] : getenv("HOME"));
        return 1;
    } else if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "help") == 0) {
        printf("MyShell Built-ins:\n  exit  cd  help  jobs  history  set  !n\n");
        return 1;
    } else if (strcmp(args[0], "jobs") == 0) {
        for (int i = 0; i < job_count; i++)
            if (job_list[i].active)
                printf("[%d] PID %d  CMD: %s\n", i + 1, job_list[i].pid, job_list[i].cmd);
        return 1;
    } else if (strcmp(args[0], "history") == 0) {
        for (int i = 0; i < history_count; i++)
            printf("%d  %s\n", i + 1, history_list[i]);
        return 1;
    } else if (strcmp(args[0], "set") == 0) {
        print_variables();
        return 1;
    }
    // Variable assignment
    if (strchr(args[0], '=') != NULL) {
        set_variable(args[0]);
        return 1;
    }
    return 0;
}

// -------------------- Input Parsing --------------------
int parse_input(char *input, char **args) {
    int i = 0;
    char *token = strtok(input, " \t\n");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return i;
}

// -------------------- Main Shell Loop --------------------
void myshell() {
    char input[MAX_INPUT];
    char *args[MAX_INPUT];

    while (1) {
        reap_background();
        printf("myshell> ");
        fflush(stdout);

        if (!fgets(input, MAX_INPUT, stdin)) break;
        if (strlen(input) <= 1) continue;

        strncpy(history_list[history_count++ % HISTORY_MAX], input, MAX_INPUT);

        parse_input(input, args);
        expand_variables(args);

        if (!handle_builtin(args)) {
            execute(args, 0);
        }
    }
}

