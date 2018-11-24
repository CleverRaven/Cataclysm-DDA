# Compilers Supported

We are compiling with -std=c++11 as you can see from the Makefile.
This regrettably restricts which compilers will compile the project, while at the same time allowing the use of a plethora of features introduced by C++11.

## GCC

GCC is the preferred compiler for building on Linux.
In order to fully support the C++11 standard, we are using GCC version 4.8.2.

## Clang

We use Clang 3.5.2 for our experimental and release builds.
Clang similarly fully supports C++11 as of version 3.1, but you may be able to get by with an earlier version.

## MinGW

MinGW version 4.9.0 is currently building the project, but has a few rough edges, such as lack of support for std::to_string().  As such we aren't using to_string(), since MinGW is a popular option for building on Windows.  Input on the earliest version that will successfully compile the project is welcome.

## MinGW-w64

This is the preferred compiler for building on Windows, and is the compiler we use to cross-compile for Windows experimental and release builds.
MinGW-w64 4.8.2 is currently building the project.  Input on the earliest version that will successfully compile the project is welcome.

## MSYS2

MSYS2 is an alternate way to build the project on Windows. It has a great package manager called Pacman which was ported from Arch Linux. It's as close to a Linux system as a (native) Windows can get (Arch in particular) and allows for simple updating of all the installed packages. You need the latest version of some packages in order for the provided instructions to work properly. To be specific, you will need mingw-w64-x86_64-SDL2_mixer >= 2.0.0-6 if you want sound support.

## Visual Studio

MSVC 14 can build Cataclysm and we have project files for it in msvc140/.  We strongly suspect that MSVC 13 and earlier are incapable of building Cataclysm and we've removed their project files.  If we can support MSVC 13, feel free to restore the project files.
