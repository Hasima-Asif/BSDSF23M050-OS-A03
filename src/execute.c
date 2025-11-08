#include <stdio.h>
#include <stdlib.h>
#include "shell.h"

void execute(char *command) {
    int ret = system(command);
    if (ret == -1) printf("Command execution failed\n");
}
