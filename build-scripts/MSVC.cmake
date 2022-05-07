if (NOT $ENV{DevEnvDir})
 set(ENV{DevEnvDir} "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE")
endif()
set(CMAKE_MAKE_PROGRAM $ENV{DevEnvDir}/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe CACHE PATH "")
set(CMAKE_CXX_FLAGS_INIT "\
/MP /utf-8 /bigobj /permissive- /sdl- /FC /Gd /GS- /Gy /GF \
/wd4068 /wd4146 /wd4819 /wd6237 /wd6319 /wd26444 /wd26451 /wd26495 /WX- /W1 \
/TP /Zc:forScope /Zc:inline /Zc:wchar_t"
)
# /MP     # cl.exe build with multiple processes
# /utf-8  # set source and execution character sets to UTF-8
# /bigobj # increase # of sections in object files
# /permissive- # to allow alternative operators ("and", "or", "not")
# /sdl-   # disable additional security checks
# /FC     # full path in compiler messages
# /Gd     # __cdecl
# /GS-    # disable buffer security checks
# /Gy     # Enable Function-Level Linking
# /GF     # Eliminate Duplicate Strings
# /wd4068 # unknown pragma
# /wd4146 # negate unsigned
# /wd4819 # codepage?
# /wd6237 # short-circuit eval
# /wd6319 # a, b: unused a
# /wd26444 # unnamed objects
# /wd26451 # overflow
# /wd26495 # uninitialized member
# /WX- # (do not) Treat Warnings as Errors
# /W1  # Warning Level
# /TP  # every file is a C++ file
# /Zc:forScope  # Force Conformance in for Loop Scope
# /Zc:inline    # Remove unreferenced COMDAT
# /Zc:wchar_t   # wchar_t Is Native Type
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT
"/Oi"
)
# /Ob1 # Inline Function Expansion (1 = only when marked as such)
# /Oi  # Generate Intrinsic Functions
add_compile_definitions(
    _SCL_SECURE_NO_WARNINGS
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    LOCALIZE
    USE_VCPKG
)
add_link_options(
    /OPT:REF
    /OPT:ICF
    /LTCG:OFF
    /MANIFEST:NO
)
# TODO use those two in debug builds only
# /OPT:REF  # remove unreferenced COMDATs
# /OPT:ICF  # folds identical COMDATs


set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR}/msvc-full-features)
include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
