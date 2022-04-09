# Compilers Supported

Our goal with compiler support is to make it as easy as possible for new
contributors to get started with development of the game, while also using the
newest compilers (and thus language standards) that we can.

To that end, we aim to support gcc and clang up to the newest stable versions
and back to those shipping in any supported version of a popular distribution
or relevant development environment, including Ubuntu, Debian, MSYS, and XCode.

Since macOS can be harder to update we have active developers and users on
unsupported versions of macOS we would like to support.  Newer macOS cannot
compile for older macOS, so to support a reasonable number of users we aim to
support at least 95% of users by macOS market share.

At the time of writing:
* Bionic is the oldest Ubuntu LTS, and [defaults to g++ 7.3 or
  7.5](https://packages.ubuntu.com/bionic/g++), depending on the platform, and
  [clang 6.0](https://packages.ubuntu.com/bionic/clang).
* Debian stable is Buster, and [defaults to g++
  8.3](https://packages.debian.org/buster/g++).
* Oldest [supported Fedora](https://fedoraproject.org/wiki/Releases) is 34,
  which uses [gcc
  11.0](https://fedora.pkgs.org/34/fedora-x86_64/gcc-11.0.1-0.3.fc34.x86_64.rpm.html).
* MSYS [offers gcc 10.2](https://packages.msys2.org/base).
* Code::Blocks [offers g++
  8.1](https://www.codeblocks.org/downloads/binaries/).
* macOS 10.12+ has 95.3% [market
  share](https://gs.statcounter.com/os-version-market-share/macos/desktop/worldwide)
  and that corresponds to [XCode 8.3](https://xcodereleases.com/).

In practice, compiler support is often determined by what is covered in our
automated testing.

At time of writing, the oldest relevant compiler is gcc 7.5, shipped in the
Ubuntu Bionic LTS release.  The default version is 7.3 only on platforms other
than x86, and we deem it unlikely that potential users or developers would be
using an Ubuntu LTS release on such a platform.

The limiting factor preventing us from using newer C++ features is currently
XCode, where we would like to require 10.0 to get [most C++17
features](https://en.cppreference.com/w/cpp/compiler_support/17).  That
[requires macOS 10.13](https://xcodereleases.com/) so we need to wait until
macOS versions up to 10.12 drop below 5% market share (currently 7.4%).

To monitor macOS market share we have a helper script in
tools/macos-market-share.py.  Download the CSV-formatted data from
[statcounter](https://gs.statcounter.com/os-version-market-share/macos/desktop/worldwide)
and process it with that script to get a summary of cumulative market share as
it varies across time.  For example, this output:

```
2021-05 :: 10.11:  8.2  10.12: 11.0  10.13: 18.3  10.14: 27.0  10.15: 98.1
2021-06 :: 10.11:  6.6  10.12:  9.3  10.13: 16.3  10.14: 24.6  10.15: 99.0
2021-07 :: 10.11:  4.7  10.12:  7.4  10.13: 14.2  10.14: 22.1  10.15: 99.3
```

shows that cumulative market share for versions up to 10.11 first dropped below
5% in 2021-07, at which point we can (following the above guidelines) allow
ourselves to drop support for 10.11.

## GCC

We support and test gcc from version 7.5.

## Clang

We support and test Clang from version 6.0.

## Mingw and Mingw-w64

We use Mingw for cross-compilation of Windows versions on Linux.  gcc 11.2 is
currently used both in the tests and for the Windows release binaries.

## MSYS2

MSYS2 is [a way to build the project](COMPILING-MSYS.md) on Windows. It
currently offers gcc at versions 7 or higher.

MSYS also provides clang.  We don't currently support using clang here, but
work to that end is welcome.

## Visual Studio

We also support [Visual Studio](COMPILING-VS-VCPKG.md) 2015 Update 3 and above.

## XCode

We support macOS 10.12 and above, which effectively means XCode 8.3+.
