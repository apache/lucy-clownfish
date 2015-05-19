#define HELLO_USE_SHORT_NAMES
#define CFISH_USE_SHORT_NAMES

#include "hello/Dog.h"
#include "Clownfish/Class.h"
#include "Clownfish/String.h"

Dog*
Dog_new(void) {
    Dog *self = (Dog*)Class_Make_Obj(DOG);
    return Dog_init(self);
}

Dog*
Dog_init(Dog *self) {
    return self;
}

String*
Dog_Greet_IMP(Dog *self, String *name) {
    (void)name; // Dogs can't talk.
    return Str_newf("Woof! Woof! Woof!");
}

String*
Dog_Salutation_IMP(Dog *self) {
    return Str_newf("Woof!");
}
