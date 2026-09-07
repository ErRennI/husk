#define _POSIX_C_SOURCE 200809L
#include "parse.h"
#include "tokenizer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const token_t *tokens;
    size_t num_tokens;
    size_t pos;
} parser_state_t;

typedef struct {
    char **argv;
    size_t argc;
    size_t capacity;
} arg_builder_t;

static bool add_arg(arg_builder_t *builder, const char *value) {
    if(builder->argc >= builder->capacity) {
        size_t new_cap = builder->capacity ? builder->capacity * 2 : 8;
        char **args = realloc(builder->argv, (new_cap * sizeof(char *)));

        if(!args) {
            perror("realloc");
            return false;
        }
        builder->argv = args;
        builder->capacity = new_cap;
    }
    char *temp_val = strdup(value);
    if(!temp_val) {
        perror("strdup");
        return false;
    }

    builder->argv[builder->argc] = temp_val;
    builder->argc++;
    return true;
}

static ast_node_t *parse_command(parser_state_t *p) {
    arg_builder_t arg_builder = {0};

    while(p->pos < p->num_tokens && p->tokens[p->pos].token_type == WORD) {




    }

}
