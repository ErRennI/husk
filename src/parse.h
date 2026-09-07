#ifndef PARSE_H
#define PARSE_H

typedef enum {
    AST_NODE_COMMAND,
    AST_NODE_PIPE,
    AST_NODE_AND,
    AST_NODE_OR,
    AST_NODE_SEQUENCE
} ast_node_type_t;

typedef struct ast_node {
    ast_node_type_t node_type;

    char **argv;
    //rediraction
    struct ast_node *left;
    struct ast_node *right;
} ast_node_t;

#endif
