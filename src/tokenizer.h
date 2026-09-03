#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
typedef enum {
    LESS,
    GREATER,
    GREATERGREATER,
    HEREDOC,
    SEMICOLON,
    PIPE,
    WORD
} token_types_t;

typedef struct {
    token_types_t token_type;
    char *value;
} token_t;

typedef struct {
    token_t *tokens;
    size_t num_tokens;
    size_t capacity;
    const char *line;
    size_t current;
    size_t line_length;
    size_t start;
} tokenizer_state_t;

void tokenize_line(const char *line, size_t line_length,tokenizer_state_t *state);

#endif
