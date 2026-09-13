#define _POSIX_C_SOURCE 200809L
#include "parse.h"
#include "executor.h"
#include "builtins.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

static bool g_should_exit = false;
static int g_exit_code = 0;

bool shell_should_exit(void) {
    return g_should_exit;
}

int shell_exit_code(void) {
    return g_exit_code;
}

void shell_request_exit(int code) {
    g_should_exit = true;
    g_exit_code = code;
}

static int execute_command(const ast_node_t *node) {

    if (is_builtin(node->argv[0])) {
        return execute_builtin(node->argv);
    }

    pid_t pid = fork();
    int status;

    if(pid == 0) {
        signal(SIGINT, SIG_DFL);
        for(size_t i = 0; i < node->num_redirects; i++) {
            if(node->redirects[i].type == REDIRECT_IN) {
                int fd = open(node->redirects[i].filename, O_RDONLY);
                if(fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            } else if(node->redirects[i].type == REDIRECT_OUT) {
                int fd = open(node->redirects[i].filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            } else if(node->redirects[i].type == REDIRECT_APPEND) {
                int fd = open(node->redirects[i].filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if(fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        }

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

static int execute_sequence(const ast_node_t *node) {
    execute_ast(node->left);
    return execute_ast(node->right);
}

// If left node succeed execute rigth
static int execute_and(const ast_node_t *node) {
    int left_status = execute_ast(node->left);
    if(left_status == 0) {
        return execute_ast(node->right);
    } else {
        return left_status;
    }
}

static int execute_or(const ast_node_t *node) {
    int left_status = execute_ast(node->left);
    if(left_status != 0) {
        return execute_ast(node->right);
    } else {
        return left_status;
    }
}

static int execute_pipe(const ast_node_t *node) {
    int fd[2];

    if(pipe(fd) == -1) {
        perror("pipe");
        return -1;
    }

    pid_t pid1 = fork();
    if(pid1 == -1) {
        perror("fork");
        return -1;
    }
    if(pid1 == 0) {
        signal(SIGINT, SIG_DFL);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        exit(execute_ast(node->left));
    }

    pid_t pid2 = fork();
    if(pid2 == -1) {
        perror("fork");
        return -1;
    }

    if(pid2 == 0) {
        signal(SIGINT, SIG_DFL);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        exit(execute_ast(node->right));
    }

    close(fd[0]);
    close(fd[1]);

    int status1,status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    if(WIFEXITED(status2)) {
        return WEXITSTATUS(status2);
    } else {
        return -1;
    }
}


int execute_ast(const ast_node_t *node) {
    if(!node) {
        return -1;
    }

    switch (node->node_type) {
        case AST_NODE_COMMAND:
            return execute_command(node);
        case AST_NODE_SEQUENCE:
            return execute_sequence(node);
        case AST_NODE_AND:
            return execute_and(node);
        case AST_NODE_OR:
            return execute_or(node);
        case AST_NODE_PIPE:
            return execute_pipe(node);
        default:
            return -1;
    }
}
