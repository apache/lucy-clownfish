// hello1.c
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s NAME\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("Hello, %s!\n", argv[1]);
    return 0;
}

