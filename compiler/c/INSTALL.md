Build instructions for the Clownfish compiler
=============================================

Building under UNIX and derivatives or Cygwin
---------------------------------------------

    ./configure --prefix [ install prefix ]
    make
    make test
    make install

Building under Windows
----------------------

You need MSVC or gcc as C compiler and nmake, mingw32-make, or standard
GNU make as make utility. When building under cmd.exe, configure with:

    configure.bat --prefix [ install prefix ]

Configuration
-------------

    [ environment ] ./configure [ options ] [ -- cflags ]

### Options

    --enable-coverage

Enable code coverage. Create HTML pages with coverage data using
lcov by running "make coverage".

    --with-system-cmark

The Clownfish compiler is built with a bundled version of libcmark
by default. Use this option if you want to link against the system
libcmark.

    --prefix
    --bindir
    --datarootdir
    --datadir
    --libdir
    --mandir

Installation directories following the GNU Autoconf convention.

### Environment variables

    CC

The C compiler.

