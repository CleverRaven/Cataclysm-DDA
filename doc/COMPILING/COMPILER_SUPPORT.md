# Compilers Supported

Our goal with compiler support is to make it as easy as possible for new
contributors to get started with development of the game, while also using the
newest compilers (and thus language standards) that we can.

To that end, we aim to support gcc and clang up to the newest stable versions
and back to those shipping in any supported version of a popular distribution
or relevant development environment, including Ubuntu, Debian, MSYS, and XCode.

Since macOS can be harder to update we have active developers and users on
unsupported versions of macOS we would like to support.  Thus, for XCode we try
to support releases for five years (the same time that an Ubuntu LTS is
supported).

At time of writing:
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
* XCode [five years ago](https://xcodereleases.com/) was 8.0, although
  currently we don't support that far back (see below).

In practice, compiler support is often determined by what is covered in our
automated testing.

At time of writing, the oldest relevant compiler is gcc 7.5, shipped in the
Ubuntu Bionic LTS release.  The default version is 7.3 only on platforms other
than x86, and we deem it unlikely that potential users or developers would be
using an Ubuntu LTS release on such a platform.

The two limiting factors preventing us from using newer C++ features are
currently
* Ubuntu, for which Bionic ceases support in 2023-05.
* XCode, where we would like to require 10.0 to get most C++17 features.  The
  last version before that was 9.4.1, [released June
  2018](https://xcodereleases.com/), so we would like to support it until
  2023-06.

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

Although we try to support XCode for five years, due to an issue on macOS
10.11 (see #49595) we only support macOS 10.12 and above, which effectively
means XCode 8.3+ from June 2017.
