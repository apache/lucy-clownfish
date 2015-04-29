// hello2.c
#include <stdio.h>
#include <stdlib.h>

#include "Clownfish/String.h"

int
main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s NAME\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Bootstrap the Clownfish runtime.
    cfish_bootstrap_parcel();

    // Create a Clownfish String using the `newf` constructor, which takes
    // a format string and arguments.
    cfish_String *greeting = cfish_Str_newf("Hello, %s\n!", argv[1]);

    // Call String's `To_Utf8` method to export null-terminated UTF-8.
    char *greeting_c = CFISH_Str_To_Utf8(greeting);
    printf("%s", greeting_c);

    // Clean up and exit.
    CFISH_DECREF(greeting);
    free(greeting_c);
    return 0;
}
