# Compilers Supported

We are compiling with -std=c++11 as you can see from the Makefile.
This regretably restricts which compilers will compile the project, while at the same time allowing the use of a plethora of features introduced by c++11.

## GCC

GCC is the preferred compiler for building on Linux.
Technically GCC doesn't "fully" support c++11 until version 4.8.1, but we aren't currently using any language features past 4.6 or so.  That is liable to change in the future.
There was one use of constructor delegation in the code which I've replaced with an init method.
At some point we are going to want to switch that back once enough people are using gcc 4.7, or some other killer feature comes along that makes us declare a hard dependency on 4.7+.

## Clang

We use Clang for our experimental and release builds.
Clang similarly fully supports c++11 as of version 3.1, but you may be able to get by with an earlier version.

## MinGW

MinGW version 4.9.0 is currently building the project, but has a few rough edges, such as lack of support for std::to_string().  As such we aren't using to_string(), since MinGW is a popular option for building on Windows.  Input on the earliest version that will successfully compile the project is welcome.

## MinGW-w64

This is the preferred compiler for building on Windows, and is the compiler we use to cross-compile for Windows experimental and release builds.
MinGW-w64 is currently building the project.  Input on the earliest version that will successfully compile the project is welcome.

## Visual Studio (BROKEN!)

The project is currently unable to compile under any version of Visual Studio.  When we made the switch to c++11, Visual Studio was not able to compile the code, and no one on the project has been willing to put in the time necessary to sort out the issues.  We welcome non-invasive code changes that restore compilation on VS (probably limited to VS 2013), but be aware that VS 2013 does not support various c++11 features we are likely to use in the future.  Foremost among them is UTF-8 literals and constexpr.