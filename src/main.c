#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

    char *line = NULL;
    size_t line_size = 0;
    ssize_t nread;



    while(1) {
        printf("> ");
        nread = getline(&line , &line_size, stdin);

        if(nread == -1) {
            free(line);
            return EXIT_FAILURE;
        }

        if(strcmp(line, "exit\n") == 0) {
            printf("Exitting");
            break;
        }

    }

    free(line);
    return EXIT_SUCCESS;
}