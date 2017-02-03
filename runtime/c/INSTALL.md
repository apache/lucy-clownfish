Build instructions for the Clownfish C library
==============================================

Building under UNIX and derivatives or Cygwin
---------------------------------------------

    ./configure
    make
    make test
    ./install.sh --prefix [ install prefix ]

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

Install under cmd.exe:

    install.bat --prefix [ install prefix ]

Install under a POSIX shell like MSYS:

    ./install.sh --prefix [ install prefix ]

Configuration
-------------

    [ environment ] ./configure [ options ] [ -- cflags ]

### Options

- `--enable-coverage`

  Enable code coverage. Create HTML pages with coverage data using
  lcov by running "make coverage".

- `--disable-threads`

  Disable thread support.

### Environment variables

- `CC`

  The C compiler.

- `TARGET_CC`

  The target compiler when cross-compiling. `CC` can be set in addition
  to specify the host compiler.

