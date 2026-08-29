#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_line(char **line, size_t *line_size) {
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
}

int main(int argc, char **argv) {

    char *line = NULL;
    size_t line_size = 0;



    while(1) {
        printf("> ");
        fflush(stdout);
        
        read_line(&line, &line_size);


        if(strcmp(line, "exit\n") == 0) {
            printf("Exiting");
            break;
        }

    }

    free(line);
    return EXIT_SUCCESS;
}