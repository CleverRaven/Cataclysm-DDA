# Disclaimer

**WARNING**: CMake build is **NOT** official and should be used for *dev purposes ONLY*.

For the official way to build CataclysmDDA, see:
  * [COMPILING.md](COMPILING.md)


# Contents

1. Prerequisites

# Prerequisites

`cmake` => 3.20.0

## Visual Studio

`C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`

# Presets

Presets will build into `out/build/<preset>/`

Run the command `cmake --list-presets` to show the presets available to you.
The list changes based on the environment you are in.


# Configure

Run the command
 * `cmake --preset <preset>`

# Build

Run the command
 * `cmake --build --preset <preset>`

# Install

Run the command
 * `cmake --install out/build/<preset>/`

 # Run

 Run the commands
  * `cd out/install/<preset>/`
  * `cataclysm` or `cataclysm-tiles.exe`
