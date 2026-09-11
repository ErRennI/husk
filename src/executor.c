#define _POSIX_C_SOURCE 200809L
#include "parse.h"
#include "executor.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>


static int execute_command(const ast_node_t *node) {
    pid_t pid = fork();
    int status;

    if(pid == 0) {
        if(execvp(node->argv[0], node->argv) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        waitpid(pid, &status, 0);

        if(WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    } else {
        perror("fork");
        return -1;
    }

    return -1;
}


int execute_ast(const ast_node_t *node) {
    if(!node) {
        return -1;
    }

    switch (node->node_type) {
        case AST_NODE_COMMAND:
            return execute_command(node);
        case AST_NODE_SEQUENCE:
            // Not yet
            return -1;
        case AST_NODE_AND:
            // Not yet
            return -1;
        case AST_NODE_OR:
            // Not yet
            return -1;
        case AST_NODE_PIPE:
            // Not yet
            return -1;
        default:
            return -1;
    }
}
