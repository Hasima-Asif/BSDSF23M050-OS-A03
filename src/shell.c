#include <stdio.h>
#include <string.h>
#include "shell.h"

void myshell() {
    char command[100];
    while (1) {
        printf("myshell> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;
        if (strcmp(command, "exit") == 0) break;
        execute(command);
    }
}
