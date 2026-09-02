#define _POSIX_C_SOURCE 200809L
#include "tokenizer.h"
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
            exit(EXIT_SUCCESS);
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

int main(int argc, char **argv) {

    char *line = NULL;
    size_t line_size = 0;
    ssize_t line_length;

    tokenizer_state_t state;
    state.num_tokens = 0;
    state.tokens = NULL;


    while(1) {
        printf("> ");
        fflush(stdout);
        
        line_length = read_line(&line, &line_size);
        tokenize_line(line, line_length, &state);



        if(strcmp(line, "exit") == 0) {
            printf("Exiting");
            break;
        }

    }

    //TOKEN FREE
    free(line);
    return EXIT_SUCCESS;
}