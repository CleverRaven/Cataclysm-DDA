#[=======================================================================[

MSVC
----

Toolchain file for Microsoft Visual C++.

Used by CMakePresets.json -> "toolchainFile".

CMake defaults seen in generators:
CMAKE_CXX_FLAGS=/DWIN32 /D_WINDOWS /EHsc
CMAKE_CXX_FLAGS_DEBUG=/Zi /Ob0 /Od /RTC1
CMAKE_CXX_FLAGS_RELWITHDEBINFO=/Zi /O2 /Ob1 /DNDEBUG
CMAKE_EXE_LINKER_FLAGS=/machine:x64
CMAKE_EXE_LINKER_FLAGS_DEBUG=/debug /INCREMENTAL
CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=/debug /INCREMENTAL

Use the following for Dr. Memory:
/Zi or /ZI
/DEBUG:FULL
/Ob0    disable inlining
/Oy-    don't omit frame pointers
Remove /RTC1

#]=======================================================================]

# Path has changed, so this configure run will find cl.exe
set(CMAKE_C_COMPILER   cl.exe)
set(CMAKE_CXX_COMPILER ${CMAKE_C_COMPILER})

# C++ flags used by all builds
add_compile_options(
    /MP    # cl.exe build with multiple processes
    /utf-8 # set source and execution character sets to UTF-8
    /bigobj # increase # of sections in object files
    /permissive- # enforce more standards compliant behavior
    /sdl-  # disable additional security checks
    /FC    # full path in compiler messages
    /Gd    # __cdecl
    /GS-   #  disable buffer security checks
    /Gy    #  enable function-level linking
    /GF    #  eliminate duplicate strings
    /wd4068 # unknown pargam
    /wd4146 # negate unsigned
    /wd4661 # explicit template undefined
    /wd4819 # codepage?
    /wd6237 # short-circuit eval
    /wd6319 # a, b: unused a
    /wd26444 # unnamed objects
    /wd26451 # overflow
    /wd26495 # uninitialized mamber
    /WX-     # do not tread warnings as errors
    /W1      # warning level
    /TP      # every source file is a C++ file
    /Zc:forScope # force conformace in for loop scope
    /Zc:inline   # remove unreferenced COMDAT
    /Zc:wchar_t  # wchar_t is native type
    $<$<CONFIG:RelWithDebInfo>:/Oi> # inline function expansions (1 = only when marked as such)
)

add_compile_definitions(
    _SCL_SECURE_NO_WARNINGS
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    LOCALIZE
    USE_VCPKG
)


# Linker flags used by all builds:
add_link_options(
    /OPT:REF # remove unreferenced COMDATs
    /OPT:ICF # force identical COMDATs
    /LTCG:OFF
    /INCREMENTAL:NO
    /DYNAMICBAS  # does this app realy need ASLR ?
    /NXCOMPAT    # same as above
    "$<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>"
    "$<$<CONFIG:RelWithDebInfo>:/NODEFAULTLIB:LIBCMTD>"
)
# Note: no need to force /TLBID:1 because is default

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Where is vcpkg.json ?
set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR}/msvc-full-features)

set(VCPKG_ROOT "" CACHE PATH "Path to VCPKG installation")
if (NOT $ENV{VCPKG_ROOT} STREQUAL "")
    include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
elseif(NOT $CACHE{VCPKG_ROOT} STREQUAL "")
    include($CACHE{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
endif()
