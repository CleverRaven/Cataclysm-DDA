# Compilers Supported

| Compiler                                             | Oldest Version |
| :---                                                 | ---: |
| [GCC](https://gcc.gnu.org)                           | [9.3](https://gcc.gnu.org/onlinedocs/gcc-9.3.0/gcc/) |
| [clang](https://clang.llvm.org)                      | [10.0](https://releases.llvm.org/10.0.0/docs/index.html) |
| [MinGW-w64](https://www.mingw-w64.org)               | [UCRT 14.2.0](https://www.mingw-w64.org/downloads/)  |
| [Visual Studio](https://visualstudio.microsoft.com/) | [2019](COMPILING-VS-VCPKG.md) |
| [XCode](https://developer.apple.com/xcode)           | 10.1 <br/> [macOS 10.13](https://en.wikipedia.org/wiki/MacOS_High_Sierra) |

Our goal with compiler support is to make it as easy as possible for new
contributors to get started with development of the game, while also using the
newest compilers (and thus language standards) that we can.

To that end, we aim to support gcc and clang up to the newest stable versions
and back to those shipping in any supported version of a popular distribution
or relevant development environment, including Ubuntu, Debian, MSYS, and XCode.

In practice, compiler support is often determined by what is covered in our
automated testing.

At the time of writing:
* Bionic is about to end general support, so we aim to support the next oldest
  Ubuntu LTS (Focal).  Focal [defaults to g++
  9.3](https://packages.ubuntu.com/focal/g++) and [clang
  10](https://packages.ubuntu.com/focal/clang).
* Debian stable is Bullseye, and [defaults to g++
  10.2](https://packages.debian.org/bullseye/g++).
* Oldest [supported Fedora](https://fedoraproject.org/wiki/Releases) is 36,
  which uses [gcc
  12.0](https://fedora.pkgs.org/36/fedora-x86_64/gcc-12.0.1-0.16.fc36.x86_64.rpm.html).
* MSYS [offers gcc 12.2](https://packages.msys2.org/base).
* macOS 10.13+ has 96.0% [market
  share](https://gs.statcounter.com/os-version-market-share/macos/desktop/worldwide)[^1]
  and that corresponds to [XCode 10.1](https://xcodereleases.com/).

[^1]: [Limit reported macOS release to 10.15 series](https://bugs.webkit.org/show_bug.cgi?id=216593)

With gcc 9.3, clang 10, and XCode 10.1 we can get all the C++17 language
features and [most but not all of the C++17 library
features](https://en.cppreference.com/w/cpp/compiler_support/17).  The
following C++17 features are not supported widely enough for us to use:

* Parallel algorithms and execution policies.
* Hardware interference size.
* File system library (note, we already have a backported version of the
  filesystem library bundled with CDDA, so that can be used instead).
* Polymorphic memory resources.
* Mathematical special functions.
* Elementary string conversions for floating point.
* Array support in `std::shared_ptr` and `weak_ptr`.

Some of these are not even supported in the latest XCode so we cannot expect to
use them for many years.

The limiting factor preventing us from using newer C++ features is primarily
XCode, where we would probably want version 13 before moving to C++20.

## Mingw and Mingw-w64

We use Mingw for cross-compilation of Windows versions on Linux.
It is currently used both in the tests and for the Windows release binaries.

## MSYS2

MSYS2 is [a way to build the project](COMPILING-MSYS.md) on Windows. It
currently (2024) offers gcc at versions 7 or higher.

MSYS also provides clang.  We don't currently support using clang here, but
work to that end is welcome.

## XCode

Since macOS can be harder to update we have active developers and users on
unsupported versions of macOS we would like to support.  To support a reasonable
number of users we aim to support at least 95% of users by macOS market share.

At time of writing, the oldest relevant compiler is XCode 10.1, the latest
supported on macOS 10.13, which is based on LLVM 6.

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
5% in 2021-07, at which point we (following the above guidelines) allowed
ourselves to drop support for 10.11.
