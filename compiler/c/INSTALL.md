Build instructions for the Clownfish compiler
=============================================

Building under UNIX and derivatives or Cygwin
---------------------------------------------

    $ ./configure
    $ make
    $ make test

Use the installation script of the Clownfish runtime to install.

Building under Windows
----------------------

You need MSVC or gcc as C compiler and nmake, mingw32-make, or standard
GNU make as make utility.

Configure under cmd.exe:

    configure.bat

Configure under a POSIX shell like MSYS:

    ./configure

Build with nmake:

    nmake
    nmake test

Build with standard GNU make:

    make
    make test

Build with mingw32-make:

    mingw32-make
    mingw32-make test

Configuration
-------------

    [ environment ] ./configure [ options ] [ -- cflags ]

### Options

- `--enable-coverage`

  Enable code coverage. Create HTML pages with coverage data using
  lcov by running "make coverage".

- `--with-system-cmark`

  The Clownfish compiler is built with a bundled version of libcmark
  by default. Use this option if you want to link against the system
  libcmark.

### Environment variables

- `CC`

  The C compiler.

