#[=======================================================================[

windows-tiles-sounds-x64
------------------------

Pre-load script for MINGW builds.

Used by CMakePresets.json -> "cacheVariables" -> "CMAKE_PROJECT_INCLUDE_BEFORE".

It then writes CMakeUserPresets.json -> "buildPresets" -> "environment"

#]=======================================================================]

# Ref https://github.com/actions/virtual-environments/blob/win19/20220515.1/images/linux/Ubuntu2204-Readme.md#environment-variables
if (NOT $ENV{VCPKG_INSTALLATION_ROOT} STREQUAL "")
    set(ENV{VCPKG_ROOT} $ENV{VCPKG_INSTALLATION_ROOT})
endif()
# Ref https://vcpkg.io/en/docs/users/config-environment.html#vcpkg_root
if ("$ENV{VCPKG_ROOT}" STREQUAL "")
    set(ENV{VCPKG_ROOT} /usr/local/share/vcpkg)
endif()

# It's fine to keep @_MSVC_DEVENV@ undefined
set(BUILD_PRESET_NAME "windows-tiles-sounds-x64")
set(CONFIGURE_PRESET "windows-tiles-sounds-x64")
configure_file(
    ${CMAKE_SOURCE_DIR}/build-scripts/CMakeUserPresets.json.in
    ${CMAKE_SOURCE_DIR}/CMakeUserPresets.json
    @ONLY
)

if (UNIX)
    set(CMAKE_MAKE_PROGRAM /usr/bin/ninja CACHE PATH "")
endif()
