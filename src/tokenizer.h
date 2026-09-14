#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    LESS,
    HEREDOC,
    GREATER,
    GREATERGREATER,
    SEMICOLON,
    PIPE,
    OR_OR,
    AND_AND,
    AMPERSAND,
    WORD,
    WORD_LITERAL,
} token_types_t;

typedef enum {
    MODE_NORMAL,
    MODE_IN_SINGLE_QUOTE,
    MODE_IN_DOUBLE_QUOTE
} tokenizer_mode_t;

typedef struct {
    token_types_t token_type;
    char *value;
} token_t;

typedef struct {
    tokenizer_mode_t tokenizer_mode;
    token_t *tokens;
    size_t num_tokens;
    size_t capacity;
    const char *line;
    size_t current;
    size_t line_length;
    size_t start;
} tokenizer_state_t;

bool tokenize_line(const char *line, size_t line_length,tokenizer_state_t *state);

#endif
