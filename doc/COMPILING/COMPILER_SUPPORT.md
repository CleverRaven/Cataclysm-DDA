# Compilers Supported

Our goal with compiler support is to make it as easy as possible for new
contributors to get started with development of the game, while also using the
newest compilers (and thus language standards) that we can.

To that end, we aim to support gcc and clang up to the newest stable versions
and back to those shipping in any supported version of a popular distribution
or relevant development environment, including Ubuntu, Debian, MSYS, and XCode.

At the time of writing:
* Bionic is the oldest Ubuntu LTS, and [defaults to g++ 7.3 or
  7.5](https://packages.ubuntu.com/bionic/g++), depending on the platform, and
  [clang 6.0](https://packages.ubuntu.com/bionic/clang).
* Debian stable is Buster, and [defaults to g++
  8.3](https://packages.debian.org/buster/g++).
* Oldest [supported Fedora](https://fedoraproject.org/wiki/Releases) is 33,
  which uses [gcc
  10.2](https://fedora.pkgs.org/33/fedora-x86_64/gcc-10.2.1-3.fc33.x86_64.rpm.html).
* MSYS [offers gcc 10.2](https://packages.msys2.org/base).
* Code::Blocks [offers g++
  8.1](https://www.codeblocks.org/downloads/binaries/).

In practice, compiler support is often determined by what is covered in our
automated testing.

At time of writing, the oldest relevant compiler is gcc 7.5, shipped in the
Ubuntu Bionic LTS release.  The default version is 7.3 only on platforms other
than x86, and we deem it unlikely that potential users or developers would be
using an Ubuntu LTS release on such a platform.

Ubuntu is likely to remain the limiting factor, so
the set of supported compilers should be reviewed when Bionic ceases support in
2023-05.

## GCC

We support and test gcc from version 7.5.

## Clang

We support and test Clang from version 6.0.

## Mingw and Mingw-w64

We use Mingw for cross-compilation of Windows versions on Linux.  gcc 9.3 is
currently used both in the tests and for the Windows release binaries.

## MSYS2

MSYS2 is [a way to build the project](COMPILING-MSYS.md) on Windows. It
currently offers gcc at versions 7 or higher.

MSYS also provides clang.  We don't currently support using clang here, but
work to that end is welcome.

## Visual Studio

We also support [Visual Studio](COMPILING-VS-VCPKG.md) 2015 Update 3 and above.

## XCode

The minimum supported version is 10.2, which is also what we test.
