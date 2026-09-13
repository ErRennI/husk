#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parse.h"

int execute_ast(const ast_node_t *node);

bool shell_should_exit(void);
int shell_exit_code(void);
void shell_request_exit(int code);

#endif
