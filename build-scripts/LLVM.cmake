#[=======================================================================[

LLVM
----

Toolchain file for LLVM CLANG on Windows

Used by CMakePresets.json -> "toolchainFile".

Using clang-cl.exe so we can copy MSVC.cmake flags over here.

Refer to MSVC.cmake documentation

#]=======================================================================]

set(CMAKE_C_COMPILER   "${LLVM_ROOT}/bin/clang-cl.exe")
set(CMAKE_CXX_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_RC_COMPILER  "${LLVM_ROOT}/bin/llvm-rc.exe")
set(CMAKE_MT "${LLVM_ROOT}/bin/llvm-mt.exe")
set(CMAKE_OBJDUMP "${LLVM_ROOT}/bin/llvm-objdump.exe")

set(CMAKE_CXX_FLAGS_INIT "\
/utf-8 /bigobj /permissive- /sdl- /FC /Gd /GS- /Gy /GF \
/wd4068 /wd4146 /wd4661 /wd4819 /wd6237 /wd6319 /wd26444 /wd26451 /wd26495 /WX- /W1 \
/Zc:forScope /Zc:inline /Zc:wchar_t"
)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT
"/Oi"
)

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
    /INCREMENTAL:NO
    /DYNAMICBASE
    /NXCOMPAT
    "$<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>"
    "$<$<CONFIG:RelWithDebInfo>:/NODEFAULTLIB:LIBCMTD>"
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
 
# Where is vcpkg.json ?
set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR}/msvc-full-features)

set(VCPKG_ROOT "" CACHE PATH "Path to VCPKG installation")
if (NOT $ENV{VCPKG_ROOT} STREQUAL "")
    include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
elseif(NOT $CACHE{VCPKG_ROOT} STREQUAL "")
    include($CACHE{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
endif()
