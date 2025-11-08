#ifndef SHELL_H
#define SHELL_H

void myshell(void);
int execute(char **args);
int handle_builtin(char **args);

#endif

