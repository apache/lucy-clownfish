#define HELLO_USE_SHORT_NAMES
#define CFISH_USE_SHORT_NAMES

#include "hello/Person.h"
#include "Clownfish/Class.h"
#include "Clownfish/String.h"

Person*
Person_new(void) {
    Person *self = (Person*)Class_Make_Obj(PERSON);
    return Person_init(self);
}

Person*
Person_init(Person *self) {
    return self;
}

String*
Person_Salutation_IMP(Person *self) {
    return Str_newf("Hello");
}

