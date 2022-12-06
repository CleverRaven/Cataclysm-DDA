#[=======================================================================[

windows-tiles-sounds-x64-msvc
-----------------------------

Pre-load script for Microsoft Visual Studio builds.

Used by CMakePresets.json -> "cacheVariables" -> "CMAKE_PROJECT_INCLUDE_BEFORE".

When CMake does not run under VS environment, it sources the VsDevCmd.bat on it own.
It then writes CMakeUserPresets.json -> "buildPresets" -> "environment"

#]=======================================================================]

# Ref https://github.com/actions/virtual-environments/blob/win19/20220515.1/images/win/Windows2019-Readme.md#environment-variables
if (NOT $ENV{VCPKG_INSTALLATION_ROOT} STREQUAL "")
    set(ENV{VCPKG_ROOT} $ENV{VCPKG_INSTALLATION_ROOT})
endif()
# Ref https://vcpkg.io/en/docs/users/config-environment.html#vcpkg_root
if ("$ENV{VCPKG_ROOT}" STREQUAL "" AND WIN32)
    set(ENV{VCPKG_ROOT} $CACHE{VCPKG_ROOT})
endif()

include(${CMAKE_SOURCE_DIR}/build-scripts/VsDevCmd.cmake)

# It's fine to keep @_MSVC_DEVENV@ undefined
set(BUILD_PRESET_NAME "windows-tiles-sounds-x64-msvc")
set(CONFIGURE_PRESET "windows-tiles-sounds-x64-msvc")
configure_file(
    ${CMAKE_SOURCE_DIR}/build-scripts/CMakeUserPresets.json.in
    ${CMAKE_SOURCE_DIR}/CMakeUserPresets.json
    @ONLY
)

# Ninja is provided by Microsoft but not in the Path
if (CMAKE_GENERATOR MATCHES "^Ninja")
    set(CMAKE_MAKE_PROGRAM $ENV{DevEnvDir}\\CommonExtensions\\Microsoft\\CMake\\Ninja\\ninja.exe CACHE PATH "")
endif()
