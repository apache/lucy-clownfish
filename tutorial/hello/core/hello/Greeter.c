#define HELLO_USE_SHORT_NAMES
#define CFISH_USE_SHORT_NAMES
#define C_HELLO_GREETER

#include "hello/Greeter.h"
#include "Clownfish/String.h"

cfish_String*
Greeter_Greet_IMP(Greeter *self, String *name) {
    String *salutation = Greeter_Salutation(self);
    String *greeting = Str_newf("%o, %o!", salutation, name);
    DECREF(salutation);
    return greeting;
}

