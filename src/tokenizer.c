#define _POSIX_C_SOURCE 200809L
#include "tokenizer.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/*
 * is_special_char returns true if c is a non-letter char and sets the token_type accordingly
 */
static bool is_special_char(char c, token_types_t *token_type) {
    switch (c) {
        case '|':
            *token_type = PIPE;
            break;
        case '<':
            *token_type = LESS;
            break;
        case '>':
            *token_type = GREATER;
            break;
        case ';':
            *token_type = SEMICOLON;
            break;
        //ADD MORE IF NEEDED
        default:
            return false;
    }
    return true;
}

static void reset_tokenizer(tokenizer_state_t *state) {
    for (size_t i = 0; i < state->num_tokens; i++) {
        free(state->tokens[i].value);
    }
    state->num_tokens = 0;
    state->current = 0;
    state->start = 0;
}

static void add_tokens(tokenizer_state_t *state, token_types_t token_type, const char *value_start, size_t len) {
    if(state->num_tokens >= state->capacity) {
        size_t new_cap = state->capacity ? state->capacity * 2 : 8;
        token_t *tokens = realloc(state->tokens, new_cap * sizeof(token_t));
        if(!tokens) {
            perror("realloc");
            return;
        }
        state->tokens = tokens;
        state->capacity = new_cap;
    }
    char *new_value = strndup(value_start, len);
    if(!new_value) {
        perror("strndup");
        return;
    }

    state->tokens[state->num_tokens].token_type = token_type;
    state->tokens[state->num_tokens].value = new_value;
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
                add_tokens(state, WORD, &state->line[state->start], (state->current - state->start));
            }
            state->current++;
            state->start = state->current;
            continue;
        }
        else {
            state->current++;
        }
    }

    if(state->start < state->current) {
        add_tokens(state, WORD, &state->line[state->start], (state->current - state->start));
    }
}
