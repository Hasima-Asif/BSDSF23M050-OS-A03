#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#define MAX_INPUT 1024
#define HISTORY_MAX 100
#define JOBS_MAX 50
#define MAX_CMDS 10

typedef struct job {
    pid_t pid;
    char cmd[256];
    int active;
} job;

extern job job_list[JOBS_MAX];
extern int job_count;

// Variables linked list
typedef struct shell_var {
    char *name;
    char *value;
    struct shell_var *next;
} shell_var;

extern shell_var *var_list;

// -------------------- Shell Functions --------------------
void myshell();
int parse_input(char *input, char **args);
int handle_builtin(char **args);
int execute(char **args, int background);
int execute_if_block(char *condition, char *then_block, char *else_block);
int execute_command(char **args, int background);
void add_job(pid_t pid, char *cmd);
void reap_background();
void set_variable(char *assignment);
char* get_variable(const char *name);
void expand_variables(char **args);
void print_variables();

#endif

