# Disclaimer

**WARNING**: CMake build is **NOT** officially supported and should be used for *dev purposes ONLY*.

For the official way to build CataclysmDDA, see:
  * [COMPILING.md](COMPILING.md)


# Contents

1. Prerequisites

# Prerequisites

`cmake` => 3.20.0<br/>
`vcpkg` from [vcpkg.io](https://vcpkg.io/en/getting-started.html)

## Visual Studio

`C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`

## vcpkg

If different from `C:\vcpkg`, define the installed path via `VCPKG_ROOT` env, add `-DVCPKG_ROOT=path` to `cmake` command, or edit `CMakePresets.json`'s `VCPKG_ROOT` cache variable.

# Presets

Presets will build into `out/build/<preset>/`

Run the command `cmake --list-presets` to show the presets available to you.
The list changes based on the environment you are in.


# Configure

Run the command
 * `cmake --preset <preset>`

# Build

Run the command
 * `cmake --build --preset <preset> --config RelWithDebInfo`

# Install

Run the command
 * `cmake --install out/build/<preset>/ --config RelWithDebInfo`

 # Run

 Run the commands
  * `cd out/install/<preset>/`
  * `cataclysm` or `cataclysm-tiles.exe`
