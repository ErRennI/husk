#define _POSIX_C_SOURCE 200809L
#include "tokenizer.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

static bool check_double(char next_c, char expected, token_types_t double_type, token_types_t single_type,token_types_t *out_type, size_t *out_len) {
    if(next_c == expected) {
        *out_type = double_type;
        *out_len = 2;
    } else {
        *out_type = single_type;
        *out_len = 1;
    }
    return true;
}

/*
 * is_special_char returns true if c is a non-letter char and sets the token_type accordingly
 */
static bool is_special_char(const tokenizer_state_t *state,char c, token_types_t *token_type, size_t *len) {
    const char next_c = (state->current + 1 < state->line_length) ? state->line[state->current + 1] : '\0';
    switch (c) {
        case '|':
            return check_double(next_c, '|', PIPEPIPE, PIPE, token_type, len);
        case '<':
            return check_double(next_c, '<', HEREDOC, LESS, token_type, len);
        case '>':
            return check_double(next_c, '>', GREATERGREATER, GREATER, token_type, len);
        case ';':
            *token_type = SEMICOLON;
            *len = 1;
            return true;
        case '&':
            return check_double(next_c, '&', AND_AND, AND, token_type, len);
        default:
            return false;
    }
}

static void reset_tokenizer(tokenizer_state_t *state) {
    for (size_t i = 0; i < state->num_tokens; i++) {
        free(state->tokens[i].value);
    }

    state->tokenizer_mode = MODE_NORMAL;
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

static bool handle_normal_mode(tokenizer_state_t *state) {
    char c = state->line[state->current];
    token_types_t special_token_type;
    size_t special_token_len;

    if(c == ' ') {
        if(state->start < state->current) {
            add_tokens(state, WORD, &state->line[state->start], (state->current - state->start));
        }
        state->current++;
        state->start = state->current;
        return true;
    } else if(is_special_char(state, c, &special_token_type, &special_token_len)) {
        if(state->start < state->current) {
            add_tokens(state, WORD, &state->line[state->start], (state->current - state->start));
        }
        add_tokens(state, special_token_type, &state->line[state->current], special_token_len);
        state->current += special_token_len;
        state->start = state->current;
        return true;
    } else if(c == '\'' || c == '\"') {
        state->tokenizer_mode = (c == '\'') ? MODE_IN_SINGLE_QUOTE : MODE_IN_DOUBLE_QUOTE;

        state->current++;
        state->start = state->current;
        return true;
    } else {
        state->current++;
        return true;
    }
}

static bool handle_quote_mode(tokenizer_state_t *state, const char quote_char) {
    char c = state->line[state->current];

    if(c == quote_char) {
        add_tokens(state, WORD, &state->line[state->start], (state->current - state->start));
        state->tokenizer_mode = MODE_NORMAL;
        state->current++;
        state->start = state->current;
    } else {
        state->current++;
    }
    return true;
}

bool tokenize_line(const char *line, size_t line_length, tokenizer_state_t *state) {
    reset_tokenizer(state);
    state->line = line;
    state->line_length = line_length;

    while(state->current < state->line_length) {
        bool ok;
        switch(state->tokenizer_mode) {
            case MODE_NORMAL:
                ok = handle_normal_mode(state);
                break;
            case MODE_IN_SINGLE_QUOTE:
                ok = handle_quote_mode(state, '\'');
                break;
            case MODE_IN_DOUBLE_QUOTE:
                ok = handle_quote_mode(state, '\"');
                break;
        }

        if(!ok) {
            return false;
        }
    }

    if(state->tokenizer_mode != MODE_NORMAL) {
        return false;
    }

    if(state->start < state->current) {
        add_tokens(state, WORD, &state->line[state->start], (state->current - state->start));
    }

    return true;
}
