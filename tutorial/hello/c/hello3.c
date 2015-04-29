#include <stdio.h>
#include <stdlib.h>

#define HELLO_USE_SHORT_NAMES
#define CFISH_USE_SHORT_NAMES
#include "hello/Greeter.h"
#include "hello/Person.h"
#include "hello/Dog.h"
#include "Clownfish/Class.h"
#include "Clownfish/String.h"

static Greeter*
S_new_martian(void);

int
main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s NAME\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Bootstrap the "hello" parcel, which also bootstraps the "clownfish"
    // parcel it depends on.
    hello_bootstrap_parcel();

    // Create a Person, a Dog, a Martian, and a _target_ which might be
    // "world".
    Person  *person  = Person_new();
    Dog     *dog     = Dog_new();
    Greeter *martian = S_new_martian();
    String  *target  = Str_newf("%s", argv[1]);

    // Have everyone greet the target.
    String *person_hello  = Person_Greet(person, target);
    String *dog_hello     = Dog_Greet(dog, target);
    String *martian_hello = Greeter_Greet(martian, target);
    char *person_hello_c  = Str_To_Utf8(person_hello);
    char *dog_hello_c     = Str_To_Utf8(dog_hello);
    char *martian_hello_c = Str_To_Utf8(martian_hello);
    printf("A Person says \"%s\"\n", person_hello_c);
    printf("A Dog says \"%s\"\n", dog_hello_c);
    printf("A Martian says \"%s\"\n", martian_hello_c);

    // Clean up and exit.
    free(martian_hello_c);
    free(dog_hello_c);
    free(person_hello_c);
    DECREF(martian_hello);
    DECREF(dog_hello);
    DECREF(person_hello);
    DECREF(target);
    DECREF(martian);
    DECREF(dog);
    DECREF(person);
    return 0;
}

static String*
S_Martian_Salutation_IMP(Greeter *self) {
    return Str_newf("We come in peace");
}

static Greeter*
S_new_martian() {
    // Create a dynamic subclass and override `Salutation`.
    String *class_name = Str_newf("Martian");
    Class *klass = Class_singleton(class_name, GREETER);
    Class_Override(klass, (cfish_method_t)S_Martian_Salutation_IMP,
                   HELLO_Greeter_Salutation_OFFSET);

    // Create a new Martian.
    return (Greeter*)Class_Make_Obj(klass);
}

