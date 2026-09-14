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

typedef struct {
    redirect_t *redirects;
    size_t count;
    size_t capacity;
} redirect_builder_t;

//VAR
static bool append_to_result(char **result, size_t *len, size_t *capacity, const char *text, size_t text_len) {
    if(text_len == 0) {
        return true;
    }

    size_t needed = *len + text_len + 1;
    if(needed > *capacity) {
        size_t new_cap = *capacity ? *capacity * 2 : 16;
        while(new_cap < needed) {
            new_cap *= 2;
        }
        char *tmp = realloc(*result, new_cap);
        if(!tmp) {
            perror("realloc");
            return false;
        }
        *result = tmp;
        *capacity = new_cap;
    }

    memcpy(*result + *len, text, text_len);
    *len += text_len;
    (*result)[*len] = '\0';
    return true;
}

static bool is_var_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static char *expand_word(const char *value) {
    char *result = NULL;
    size_t len = 0;
    size_t capacity = 0;

    size_t i = 0;
    size_t value_len = strlen(value);

    while(i < value_len) {
        if(value[i] == '$') {
            size_t j = i + 1;
            while(j < value_len && is_var_char(value[j])) {
                j++;
            }

            if(j == i + 1) {
                if (!append_to_result(&result, &len, &capacity, "$", 1)) {
                    free(result);
                    return NULL;
                }
                i++;
                continue;
            }

            char *var_name = strndup(&value[i + 1], j - (i + 1));
            if(!var_name) {
                perror("strndup");
                free(result);
                return NULL;
            }

            const char *env_value = getenv(var_name);
            free(var_name);

            if(env_value) {
                if(!append_to_result(&result, &len, &capacity, env_value, strlen(env_value))) {
                    free(result);
                    return NULL;
                }
            }

            i = j;
        } else {
            if(!append_to_result(&result, &len, &capacity, &value[i], 1)) {
                free(result);
                return NULL;
            }
            i++;
        }
    }

    if(!result) {
        result = strdup("");
        if(!result) {
            perror("strdup");
            return NULL;
        }
    }

    return result;
}

static bool add_redirect(redirect_builder_t *builder, redirect_type_t type, const char *filename) {
    if (builder->count >= builder->capacity) {
        size_t new_cap = builder->capacity ? builder->capacity * 2 : 4;
        redirect_t *redirects = realloc(builder->redirects, new_cap * sizeof(redirect_t));
        if (!redirects) {
            perror("realloc");
            return false;
        }
        builder->redirects = redirects;
        builder->capacity = new_cap;
    }

    char *filename_copy = strdup(filename);
    if (!filename_copy) {
        perror("strdup");
        return false;
    }

    builder->redirects[builder->count].type = type;
    builder->redirects[builder->count].filename = filename_copy;
    builder->count++;
    return true;
}

static void free_redirect_builder(redirect_builder_t *builder) {
    for (size_t i = 0; i < builder->count; i++) {
        free(builder->redirects[i].filename);
    }
    free(builder->redirects);
}

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
        for(size_t i = 0; i < node->num_redirects; i++) {
            free(node->redirects[i].filename);
        }
        free(node->redirects);
        free(node->argv);
        free(node);
    } else {
        free_ast(node->left);
        free_ast(node->right);
        free(node);
    }
}

static bool is_command_token(token_types_t type) {
    return type == WORD || type == WORD_LITERAL || type == LESS || type == GREATER || type == GREATERGREATER;
}

static ast_node_t *parse_command(parser_state_t *p) {
    arg_builder_t arg_builder = {0};
    redirect_builder_t redirect_builder = {0};

    while(p->pos < p->num_tokens && is_command_token(p->tokens[p->pos].token_type)) {
        if(p->tokens[p->pos].token_type == WORD) {
            char *expanded = expand_word(p->tokens[p->pos].value);
            if(!expanded) {
                free_arg_builder(&arg_builder);
                free_redirect_builder(&redirect_builder);
                return NULL;
            }
            bool ok = add_arg(&arg_builder, expanded);
            free(expanded);
            if(!ok) {
                free_arg_builder(&arg_builder);
                free_redirect_builder(&redirect_builder);
                return NULL;
            }
        } else if(p->tokens[p->pos].token_type == WORD_LITERAL) {
            if(!add_arg(&arg_builder, p->tokens[p->pos].value)) {
                free_arg_builder(&arg_builder);
                free_redirect_builder(&redirect_builder);
                return NULL;
            }
        } else {
            redirect_type_t type;
            if(p->tokens[p->pos].token_type == LESS) type = REDIRECT_IN;
            else if (p->tokens[p->pos].token_type == GREATER) type = REDIRECT_OUT;
            else type = REDIRECT_APPEND;

            p->pos++;

            if(p->pos >= p->num_tokens || (p->tokens[p->pos].token_type != WORD && p->tokens[p->pos].token_type != WORD_LITERAL)) {
                free_arg_builder(&arg_builder);
                free_redirect_builder(&redirect_builder);
                return NULL;
            }

            char *filename = p->tokens[p->pos].value;
            char *expanded_filename = NULL;

            if(p->tokens[p->pos].token_type == WORD) {
                expanded_filename = expand_word(filename);
                if(!expanded_filename) {
                    free_arg_builder(&arg_builder);
                    free_redirect_builder(&redirect_builder);
                    return NULL;
                }
                filename = expanded_filename;
            }

            bool ok = add_redirect(&redirect_builder, type, filename);
            free(expanded_filename);

            if(!ok) {
                free_arg_builder(&arg_builder);
                free_redirect_builder(&redirect_builder);
                return NULL;
            }

            p->pos++;
            continue;
        }
        p->pos++;
    }

    if(arg_builder.argc == 0) {
        free_arg_builder(&arg_builder);
        free_redirect_builder(&redirect_builder);
        return NULL;

    }

    if(!add_arg(&arg_builder, NULL)) {
        free_arg_builder(&arg_builder);
        free_redirect_builder(&redirect_builder);
        return NULL;
    }

    ast_node_t *node = malloc(sizeof(ast_node_t));

    if(!node) {
        perror("malloc");
        free_arg_builder(&arg_builder);
        free_redirect_builder(&redirect_builder);
        return NULL;
    }

    *node = (ast_node_t){
        .node_type = AST_NODE_COMMAND,
        .argv = arg_builder.argv,
        .redirects = redirect_builder.redirects,
        .num_redirects = redirect_builder.count,
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
