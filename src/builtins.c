#define _POSIX_C_SOURCE 200809L
#include "builtins.h"
#include "executor.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static int builtin_pwd(char **argv) {
    (void)argv;
    char buf[PATH_MAX];
    if(getcwd(buf, sizeof(buf)) == NULL) {
        perror("getcwd");
        return -1;
    }
    printf("%s\n", buf);
    return 0;
}

static int builtin_cd(char **argv) {
    const char *target;
    if(argv[1]) {
        target = argv[1];
    } else {
        target = getenv("HOME");
        if (!target) {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
    }

    if(chdir(target) == -1) {
        perror("chdir");
        return -1;
    }
    return 0;
}

static int builtin_exit(char **argv) {
    int code = 0;
    if (argv[1]) {
        code = atoi(argv[1]);
    }
    shell_request_exit(code);
    return code;
}

bool is_builtin(const char *name) {
    return strcmp(name, "cd") == 0 || strcmp(name, "exit") == 0 || strcmp(name, "pwd") == 0;
}

int execute_builtin(char **argv) {
    if (strcmp(argv[0], "cd") == 0) {
        return builtin_cd(argv);
    }
    if (strcmp(argv[0], "exit") == 0) {
        return builtin_exit(argv);
    }
    if (strcmp(argv[0], "pwd") == 0) {
        return builtin_pwd(argv);
    }
    return -1;
}
