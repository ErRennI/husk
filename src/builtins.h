#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdbool.h>

bool is_builtin(const char *name);
int execute_builtin(char **argv);

#endif
