# Apache Clownfish symbiotic object system

Apache Clownfish is a "symbiotic" object system for C which is designed to
pair with a "host" dynamic language environment, facilitating the development
of high performance host language extensions.

## Features

* Designed to support multiple host languages. Currently, only Perl is
  supported.
* Support for stand-alone C libraries and executables.
* Subclassing and method overriding from the host language.
* Support for host language idioms like named parameters or default argument
  values.
* Highly performant object system with lazy creation of host language objects.
* Runtime with classes for commonly used data structures like strings, dynamic
  arrays and hash tables.
* Guaranteed ABI stability when adding or reordering methods or instance
  variables.
* Modularity.
* Introspection.
* Documentation generator.

## Planned features

* Support for more host languages.
* Support for interfaces.

## Overview

Clownfish consists of two parts, the Clownfish compiler "CFC" and the
Clownfish runtime. CFC is a code generator that creates C header files, host
language bindings, initialization code and documentation from a set of
Clownfish header files. The generated code is compiled with other project
code and linked with the Clownfish runtime.

Clownfish header files have a `.cfh` extensions and define classes used within
the Clownfish object system. The object system is implemented in C and
supports single inheritance and virtual method dispatch. CFC creates a C
header file from each Clownfish header containing the C interface to Clownfish
objects. Functions and methods of objects are implemented in normal C source
files. Beyond the C level, CFC can generate host language bindings to make
Clownfish objects accessible from other programming languages. The compiler
also creates class documentation in various formats from comments contained in
Clownfish header files and standalone Markdown files.

The Clownfish runtime provides:

* The [](cfish.Obj) class which is the root of the class hierarchy.
* Core data types like [strings](cfish.String),
  [dynamic arrays](cfish.Vector), and [hashes](cfish.Hash).
* The [](cfish.Class) metaclass.
* Some helper classes.

## Documentation

* [Working with Clownfish classes](ClassIntro)
* [Building Clownfish projects in C environments](BuildingProjects)
* [Writing Clownfish classes](WritingClasses)

