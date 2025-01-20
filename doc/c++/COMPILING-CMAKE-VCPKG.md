# Compiling CMAKE VCPKG
# Disclaimer

**WARNING**: CMake build is **NOT** officially supported and should be used for *dev purposes ONLY*.

For the official way to build CataclysmDDA, see:
  * [COMPILING.md](COMPILING.md)

# Contents

1. Prerequisites
2. Configure
3. Build
4. Install
5. Run

# 1 Prerequisites

`cmake` >= 3.20.0<br/>
`vcpkg` from [vcpkg.io](https://vcpkg.io/en/getting-started.html)
`msgfmt` (optional) as part of Git Bash or msys2 in the default install paths.

On Linux (for `libxcrypt`):
`autoconf`
`automake`
`libtool`
`pkg-config`

# 2 Configure

## Presets
The file `CMakePresets.json` contains all the presets.<br/>
They will all build the code into the directory `out/build/<preset>/`.

## vcpkg

If vcpkg is checked out in a location different from `C:\vcpkg`, eg. `C:\dev\vcpkg`, you must do one of the following:
  * append `-DVCPKG_ROOT=C:\dev\vcpkg` (or whatever the path is) to any cmake configure commands
  * set the environment variable `VCPKG_ROOT` to the path to the vcpkg checkout.
  * edit the `VCPKG_ROOT` cache variable in `CMakePresets.json` to the appropriate path

## Visual Studio
The Standard toolbar shows the presets in the _Configuration_ drop-down box.<br/>
From the main menu, select _Project -> Configure Cache_

If you do not have `msgfmt` available, or do not want to include translations with the build, you need to additionally set `DLOCALIZE=OFF`. To do this, go to _Project -> CMake Settings_, scroll to the bottom where `"windows-tiles-sounds-x64-msvc"` is defined, and under `"cacheVariables"` change the value from `"LOCALIZE": "True"` to `"LOCALIZE": "OFF"`.
 * Note: doing this will change the `CMakePresets.json` file which is tracked by source control. Do not commit this change.

## Terminal
Run the command
 * `cmake --list-presets`<br/>
It will show the presets available to you.
The list changes based on the environment you are in.
If empty, the environment is not supported.

Run the command
 * `cmake --preset <preset>`
 
 If you do not have `msgfmt` available, or do not want to include translations with the build, you need to additionally pass `-DLOCALIZE=OFF`.
 
 * `cmake --preset <preset> -DLOCALIZE=OFF`
 
# 3 Build

## Visual Studio
From the Standard toolbar's _Build Preset_ drop-down menu select the build preset.<br/>
From the main menu, select _Build -> Build All_

## Terminal
Run the command
 * `cmake --build --preset <preset>`

# 4 Install

## Visual Studio
From the main menu, select _Build -> Install CataclysmDDA_

## Terminal
Run the command
 * `cmake --install out/build/<preset>/ --config RelWithDebInfo`

# 5 Run

 ## Visual Studio
From the Standard toolbar's _Select Startup Item..._ drop-down menu select `cataclysm-tiles.exe (Install)` <br/>
The _Project Configuration_ drop-down menu will show `RelWithDebInfo`.<br/>
You can now _Start Without Debugging_ (default Ctrl+F5) or _Debug -> Start Debugging_ (default F5).

 ## Terminal
 Run the commands
  * `cd out/install/<preset>/`
  * `cataclysm` or `cataclysm-tiles.exe`
