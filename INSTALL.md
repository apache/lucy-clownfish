Build Instructions
==================

Apache Clownfish consists of two parts, the Clownfish compiler "CFC" and the
Clownfish runtime.

Apache Clownfish runtime
------------------------

To build the Clownfish runtime, chdir into the `runtime/$LANG` subdirectory,
where `$LANG` is your programming language of choice, and follow the
instructions in the `INSTALL` document there.

The Clownfish runtime build requires the Clownfish compiler.  Some host
language bindings will build it first automaticaly, while others require
manual installation of the compiler as a prerequisite.

Apache Clownfish compiler
-------------------------

To build only the Clownfish compiler, chdir into the `compiler/$LANG`
subdirectory, where `$LANG` is your programming language of choice, and follow
the instructions in the `INSTALL` document there.

