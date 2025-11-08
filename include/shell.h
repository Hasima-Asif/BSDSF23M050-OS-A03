#ifndef SHELL_H
#define SHELL_H

#define HISTORY_SIZE 20

void myshell();
int execute(char **args);
int handle_builtin(char **args);

#endif

