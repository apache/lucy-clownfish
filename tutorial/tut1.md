Clownfish Tutorial
==================

Overview
--------

This tutorial demonstrates Clownfish's object-oriented model and bindings to C
and other "host" languages.

It is assumed that the reader has a strong background in the following:

*   C
*   Object-oriented programming

Some familiarity is required with:

*   Perl
*   Unix shell

If you are not using the `bash` shell, you will need to know how to translate
`bash` commands to your shell of choice.

Getting set up
--------------

This tutorial will work directly from the source directories and will not
require any installation.

Start by obtaining Clownfish from [Apache](http://lucy.apache.org/download).
Unpack the tarball and `cd` into the source directory.

    tar -zxf apache-clownfish-X.Y.Z.tar.gz
    cd apache-clownfish-X.Y.Z

Clownfish comes with a developer script which sets up various environment
variables to facilitate working from an uninstalled source tree.  Run this
script:

    source devel/bin/setup_env.sh

We'll be changing directories a lot in this tutorial, so let's capture the
root source directory to another environment variable.

    export CLOWNFISH=`pwd`

Build and test the Clownfish compiler for C:

    cd $CLOWNFISH/compiler/c
    ./configure
    make -j
    make -j test

Now, build the clownfish runtime for C.

    cd $CLOWNFISH/runtime/c
    ./configure
    make -j
    make -j test

Clownfish is now ready for use with the C portion of the tutorial.

Clownfish C Simple "Hello World"
--------------------------------

Move into the tutorial directory.

    cd $CLOWNFISH/tutorial/hello/c

Clownfish is a C library and a "hello world" application looks basically like
a C "hello world".  In fact, let's start with a plain C `hello` application,
with one twist: who the program will greet will be specified on the command
line:

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

Compile and run:

    cc hello1.c -std=c99 -Wall -o hello
    ./hello world

Now, let's use Clownfish's string handling facilities to assemble the
greeting.

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

In order to bring in Clownfish, our compilation command requires a few more
arguments:

    cc hello2.c -std=c99 -Wall -o hello \
        -I $CLOWNFISH/runtime/c/autogen/include \
        -L $CLOWNFISH/runtime/c \
        -lcfish
    ./hello world

### Clownfish-flavored C: naming conventions ###

All Clownfish symbols have prefixes to avoid collisions in the global
namespace.  Everything in the Clownfish runtime begins with either `cfish_` or
`CFISH_`.

    cfish_String *hello = cfish_Str_newf("hello");
    char *hello_c = CFISH_Str_To_Utf8(hello);
    CFISH_DECREF(hello);

For convenience, defining `CFISH_USE_SHORT_NAMES` generates macro aliases
for working without prefixes when it is certain that there will be no
collisions.

    #define CFISH_USE_SHORT_NAMES
    #include "Clownfish/String.h"

    String *hello = Str_newf("hello");
    char *hello_c = Str_To_Utf8(hello);
    DECREF(hello);

Clownfish supports objects and methods, but since C has no dedicated syntax to
support invocation of instance methods, capitalization is used to
differentiate dynamic method calls from "inert" (ordinary C) functions.

    CFISH_Str_To_Utf8 // dynamic method call
    cfish_Str_newf    // "inert" functions

Clownfish OOP "Hello World"
-------------------------

To run the OOP version of our hello world tutorial with C, run the following
commands:

    cfc --dest=autogen --source=../core --include=$CLOWNFISH/runtime/core
    cc hello3.c autogen/source/*.c ../core/hello/*.c \
        -std=c99 -Wall -o hello \
        -I autogen/include \
        -I $CLOWNFISH/runtime/c/autogen/include \
        -L $CLOWNFISH/runtime/c \
        -lcfish

Cleaning up after C
-------------------

Run the following commands to clean up the C build of Clownfish:

    cd $CLOWNFISH/runtime/c
    make distclean

Host languages
--------------

Clownfish is a "symbiotic object system", designed to work within a "host"
language environment.  There can be multiple unrelated installations on your
system, one for each "host".  Unlike many C libraries, each host has its own
copy -- different hosts do not link to a single canonical shared object.

This facilitates distribution through host language package managers and
uncoordinated upgrades.

Usage with Perl
---------------

To run the OOP tutorial code from a Perl host, make sure that you have cleaned
up after C (or any other host) and then run the following commands:

    cd $CLOWNFISH/compiler/perl
    perl Build.PL
    ./Build test
    cd $CLOWNFISH/runtime/perl
    perl Build.PL
    ./Build test
    cd $CLOWNFISH/tutorial/hello/perl
    perl Build.PL
    ./Build code
    perl -Mblib hello.pl



