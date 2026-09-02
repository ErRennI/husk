#define _POSIX_C_SOURCE 200809L
#include "tokenizer.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

static void reset_tokenizer(tokenizer_state_t *state) {
    for(size_t i = 0; i < state->num_tokens; i++) {
        free(state->tokens[i].value);
    }
    free(state->tokens);
    state->tokens = NULL;

    state->current = 0;
    state->start = 0;
    state->num_tokens = 0;
}

static void add_tokens(tokenizer_state_t *state, token_types_t token_type, const char *value_start, size_t len) {
    char *new_value = strndup(value_start, len);

    if(!new_value) {
        perror("strndup");
        return;
    }

    token_t *tokens = realloc(state->tokens, (state->num_tokens + 1) * sizeof(token_t));

    if(!tokens) {
        free(new_value);
        perror("add_tokens(realloc)");
        return;
    }
    tokens->token_type = token_type;
    tokens->value = new_value;

    state->tokens = tokens;
    state->num_tokens++;
}

void tokenize_line(const char *line, size_t line_length, tokenizer_state_t *state) {
    reset_tokenizer(state);
    state->line = line;
    state->line_length = line_length;

    while(state->current < state->line_length) {
        char c = state->line[state->current];

        if(c == ' ') {
            if(state->start < state->current) {

            }
        }

        else {
            state->current++;
        }
    }
}
