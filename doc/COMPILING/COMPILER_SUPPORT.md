# Compilers Supported

Our goal with compiler support is to make it as easy as possible for new
contributors to get started with development of the game, while also using the
newest compilers (and thus language standards) that we can.

To that end, we aim to support gcc and clang up to the newest stable versions
and back to those shipping in any supported version of a popular distribution
or relevant development environment, including Ubuntu, Debian, MSYS, and XCode.

In practice, compiler support is often determined by what is covered in our
automated testing.

At time of writing, the oldest relevant compiler is gcc 5.3, shipped in the
Ubuntu Xenial LTS release.  Ubuntu is likely to remain the limiting factor, so
the set of supported compilers should be reviewed when Xenial ceases support in
2021-05.

## GCC

We support and test gcc from version 5.3.

## Clang

We support and test Clang from version 3.8.

## Mingw and Mingw-w64

We use Mingw for cross-compilation of Windows versions on Linux.  gcc 5.4 is
currently used both in the tests and for the Windows release binaries.

## MSYS2

MSYS2 is [a way to build the project](../COMPILING-MSYS.md) on Windows. It
currently offers gcc at versions 7 or higher.

MSYS also provides clang.  We don't currently support using clang here, but
work to that end is welcome.

## Visual Studio

We also support [Visual Studio](../COMPILING-VS-VCPKG.md) 2015 Update 3 and above.
