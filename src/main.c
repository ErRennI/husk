#define _POSIX_C_SOURCE 200809L
#include "parse.h"
#include "tokenizer.h"
#include "executor.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ssize_t read_line(char **line, size_t *line_size) {
    ssize_t nread;

    nread = getline(line , line_size, stdin);

    if(nread == -1) {
        free(*line);
        *line = NULL;

        if(feof(stdin)) {

        } else {
            perror("Read line");
            exit(EXIT_FAILURE);
        }
    }

    if(nread > 0 && (*line)[nread - 1] == '\n') {
        (*line)[nread - 1] = '\0';
        nread--;
    }

    return nread;
}

int main(void) {

    char *line = NULL;
    size_t line_size = 0;
    ssize_t line_length;

    tokenizer_state_t state = {0};

    while(1) {
        printf("> ");
        fflush(stdout);

        line_length = read_line(&line, &line_size);
        if (line_length == -1) {
            break;
        }

        if(!tokenize_line(line, line_length, &state)) {
            fprintf(stderr, "syntax error: unterminated quote\n");
            continue;
        }

        ast_node_t *ast = parse_tokens(state.tokens, state.num_tokens);
        if(!ast) {
            fprintf(stderr, "syntax error\n");
            continue;
        }

        int exit_code = execute_ast(ast);

        free_ast(ast);
        ast = NULL;
    }

    for (size_t i = 0; i < state.num_tokens; i++) {
        free(state.tokens[i].value);
    }
    free(state.tokens);
    state.tokens = NULL;
    state.num_tokens = 0;
    free(line);
    return EXIT_SUCCESS;
}
