#[=======================================================================[

MINGW
-----

Toolchain file for MINGW

Used by CMakePresets.json -> "toolchainFile".

C++ flags used by all builds:

...

#]=======================================================================]

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER /usr/bin/x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER /usr/bin/x86_64-w64-mingw32-g++-posix)

# Where is vcpkg.json ?
set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR}/msvc-full-features)

# Bring VCPKG in
include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
