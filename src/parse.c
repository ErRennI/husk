#define _POSIX_C_SOURCE 200809L
#include "parse.h"
#include "tokenizer.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

    if(!value) {
        builder->argv[builder->argc] = NULL;
        return true;
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

static void free_arg_builder(arg_builder_t *builder) {
    for (size_t i = 0; i < builder->argc; i++) {
        free(builder->argv[i]);
    }
    free(builder->argv);
}

void free_ast(ast_node_t *node) {
    if(!node) {
        return;
    }
    if(node->node_type == AST_NODE_COMMAND) {
        for(size_t i = 0; node->argv[i] != NULL; i++) {
            free(node->argv[i]);
            node->argv[i] = NULL;
        }
        free(node->argv);
        free(node);
    } else {
        free_ast(node->left);
        free_ast(node->right);
        free(node);
    }
}

static ast_node_t *parse_command(parser_state_t *p) {
    arg_builder_t arg_builder = {0};

    while(p->pos < p->num_tokens && p->tokens[p->pos].token_type == WORD) {
        if(!add_arg(&arg_builder, p->tokens[p->pos].value)) {
            free_arg_builder(&arg_builder);
            return NULL;
        }
        p->pos++;
    }

    if(arg_builder.argc == 0) {
        free_arg_builder(&arg_builder);
        return NULL;

    }

    if(!add_arg(&arg_builder, NULL)) {
        free_arg_builder(&arg_builder);
        return NULL;
    }

    ast_node_t *node = malloc(sizeof(ast_node_t));

    if(!node) {
        perror("malloc");
        free_arg_builder(&arg_builder);
        return NULL;
    }

    *node = (ast_node_t){
        .node_type = AST_NODE_COMMAND,
        .argv = arg_builder.argv,
        .left = NULL,
        .right = NULL,
    };

    return node;
}

static ast_node_t *parse_pipe(parser_state_t *p) {
    ast_node_t *left = parse_command(p);
    if(!left) {
        return NULL;
    }

    while(p->pos < p->num_tokens && p->tokens[p->pos].token_type == PIPE) {
        p->pos++;

        ast_node_t *right = parse_command(p);
        if(!right) {
            free_ast(left);
            return NULL;
        }

        ast_node_t *new_node = malloc(sizeof(ast_node_t));
        if(!new_node) {
            perror("malloc");
            free_ast(left);
            free_ast(right);
            return NULL;
        }

        *new_node = (ast_node_t){
            .node_type = AST_NODE_PIPE,
            .argv = NULL,
            .left = left,
            .right = right,
        };

        left = new_node;
    }

    return left;
}

static ast_node_t *parse_and_or(parser_state_t *p) {
    ast_node_t *left = parse_pipe(p);
    if(!left) {
        return NULL;
    }

    while(p->pos < p->num_tokens && (p->tokens[p->pos].token_type == AND_AND || p->tokens[p->pos].token_type == OR_OR)) {
        ast_node_type_t node_type = (p->tokens[p->pos].token_type == AND_AND) ? AST_NODE_AND : AST_NODE_OR;
        p->pos++;

        ast_node_t *right = parse_pipe(p);
        if(!right) {
            free_ast(left);
            return NULL;
        }

        ast_node_t *new_node = malloc(sizeof(ast_node_t));
        if(!new_node) {
            perror("malloc");
            free_ast(left);
            free_ast(right);
            return NULL;
        }

        *new_node = (ast_node_t){
            .node_type = node_type,
            .argv = NULL,
            .left = left,
            .right = right
        };

        left = new_node;
    }

    return left;
}

static ast_node_t *parse_sequence(parser_state_t *p) {
    ast_node_t *left = parse_and_or(p);
    if(!left) {
        return NULL;
    }

    while(p->pos < p->num_tokens && p->tokens[p->pos].token_type == SEMICOLON) {
        p->pos++;

        ast_node_t *right = parse_and_or(p);
        if(!right) {
            free_ast(left);
            return NULL;
        }

        ast_node_t *new_node = malloc(sizeof(ast_node_t));
        if(!new_node) {
            perror("malloc");
            free_ast(left);
            free_ast(right);
            return NULL;
        }

        *new_node = (ast_node_t){
            .node_type = AST_NODE_SEQUENCE,
            .argv = NULL,
            .left = left,
            .right = right,
        };

        left = new_node;
    }

    return left;
}

ast_node_t *parse_tokens(const token_t *tokens, size_t num_tokens) {
    parser_state_t p = {
        .tokens = tokens,
        .num_tokens = num_tokens,
        .pos = 0,
    };

    ast_node_t *root = parse_sequence(&p);
    if(root == NULL && p.pos != p.num_tokens) {
        free_ast(root);
        return NULL;
    }

    return root;
}
