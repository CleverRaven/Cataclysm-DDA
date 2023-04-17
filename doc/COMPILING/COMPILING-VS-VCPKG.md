# Compilation guide for Windows (using Visual Studio and vcpkg)

This guide contains steps required to allow compilation of Cataclysm-DDA on Windows using Visual Studio and vcpkg.

Steps from current guide were tested on Windows 10 (64 bit), Visual Studio 2019 (64 bit) and vcpkg, but should as well work with slight modifications for other versions of Windows and Visual Studio.

## Prerequisites:

* Computer with modern Windows operating system installed (Windows 10, Windows 8.1 or Windows 7);
* NTFS partition with ~15 Gb free space (~10 Gb for Visual Studio, ~1 Gb for vcpkg installation, ~3 Gb for repository and ~1 Gb for build cache);
* Git for Windows (installer can be downloaded from [Git homepage](https://git-scm.com/));
* Visual Studio 2019 (or 2015 Visual Studio Update 3 and above);
  * **Note**: If you are using Visual Studio 2022, you must install the Visual Studio 2019 compilers to work around a vcpkg bug. In the Visual Studio Installer, select the 'Individual components' tab and search for / select the component that looks like 'MSVC v142 - VS 2019 C++ x64/x86 Build Tools'. See https://github.com/microsoft/vcpkg/issues/22287.
* Latest version of vcpkg (see instructions on [vcpkg homepage](https://github.com/Microsoft/vcpkg)).

**Note:** Windows XP is unsupported!

## Installation and configuration:

1. Install `Visual Studio` (installer can be downloaded from [Visual Studio homepage](https://visualstudio.microsoft.com/)).

    - Select the "Desktop development with C++" and "Game development with C++" workloads.

2. Install `Git for Windows` (installer can be downloaded from [Git homepage](https://git-scm.com/)).

3. Install and configure `vcpkg`. If you already have `vcpkg` installed, you should update it to at least commit `5b1214315250939257ef5d62ecdcbca18cf4fb1c` (the most recent tested good revision) and rerun `.\bootstrap-vcpkg.bat` as described:

***WARNING: It is important that, wherever you decide to clone this repo, the path does not include whitespace. That is, `C:/dev/vcpkg` is acceptable, but `C:/dev test/vcpkg` is not.***

In a `cmd.exe` shell:
```cmd
REM cd to the appropriate folder first
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat -disableMetrics
.\vcpkg integrate install
```
In a Git Bash shell, the commands are almost the same except the filesystem path separator is `/` instead of `\`.
```
# cd to the appropriate folder first
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat -disableMetrics
./vcpkg.exe integrate install
```

## Cloning and compilation:

1. Clone Cataclysm-DDA repository with following command line:

**Note:** This will download the entire CDDA repository; about three gigs of data. If you're just testing you should probably add `--depth=1`.

```cmd
git clone https://github.com/CleverRaven/Cataclysm-DDA.git
cd Cataclysm-DDA
```

2. Open the provided solution (`msvc-full-features\Cataclysm-vcpkg-static.sln`) in `Visual Studio`.

3. Open the `Build > Configuration Manager` menu and adjust `Active solution configuration` and `Active solution platform` to match your intended target.

    - The `Release` configuration and `x64` platform together make a good default setting. `Debug` is too slow and should be reserved for breakpoint debugging with code stepping.
    - This will configure Visual Studio to compile the release version, with support for Sound, Tiles, and Localization (note, however, that language files themselves are not automatically compiled; this will be done later).

4. Start the build process by selecting either `Build > Build Solution` or `Build > Build > 1 Cataclysm-vcpkg-static`. The process may take a long period of time, so you'd better prepare a cup of coffee and some books in front of your computer :) The first build of each architecture will also download and install dependencies through vcpkg, which can take an especially long time.

5. If you need localization support, execute the bash script `lang/compile_mo.sh` inside Git Bash GUI just like on a UNIX-like system. This will compile the language files that were not automatically compiled in step 2 above.

Even if you do not need languages other than English, you may still want to execute `lang/compile_mo.sh` to compile the language files if you're planning to run the unit tests, since those rely on the language files existing.

### Running and debugging Cataclysm

1. Ensure that the Cataclysm project (`Cataclysm-vcpkg-static`) is the selected startup project.

    - By default it should be already. If the project name is **Bold** in the Solution Explorer pane, then it is already set.
    - Otherwise, right click the project in the Solution Explorer pane, select `Set as Startup Project`.

2. Run or debug Cataclysm

    - To debug with the debugger attached, press the 'Local Windows Debugger' button at the top of the window with a solid green arrow on it, or press F5 which is the default shortcut, or go to the Debug menu and select 'Start Debugging'.
    - To run without a debugger, press the empty green arrow next to the 'Local Windows Debugger', or the default shortcut ctrl-F5, or go to the Debug menu and select 'Start Without Debugging'.

When debugging, it is not strictly necessary to use a `Debug` build; `Release` builds run significantly faster, can still be run in the debugger, and most of the time will have most of the information you need.

### Running unit tests

1. Ensure that the Cataclysm test project (`Cataclysm-test-vcpkg-static`) is the selected startup project.

    - This is done the same way as you do it for Cataclysm, except for the test project.

2. Configure any extra command line arguments for the tests.

    - Under `Configuration Properties > Debugging`, change `Command Arguments` to the needed arguments.
    - `--wait-for-keypress exit` can be helpful by keeping the test window open at the end until you press Enter.

3. Run the tests

    - The same ways you run Cataclysm can be used to run the unit tests, assuming you've set the test project as the startup project.

Additional command line arguments may be configured in the project's command line arguments setting, or if you are using a compatible unit test runner (e.g. Resharper) you can run or debug individual tests from the unit test sessions.
You can also start the test runner library manually from Windows console. Run it with `--help` for an overview of the arguments.

It is recommended you run the unit tests in a Release configuration. Debug builds of the unit tests generally run intolerably slowly, but can raise signal that Release builds will not catch, like invalid iterators or improper STL usage.

### Make a distribution

There is a batch script in `msvc-full-features` folder `distribute.bat`. It will create a sub folder `distribution` and copy all required files(eg. `data/`, `Cataclysm.exe` and dlls) into that folder. Then you can zip it and share the archive on the Internet.

### ccache integration

It is possible to use ccache with Visual Studio and gain the same benefits as other platforms.

1. Download the "Windows x86_64 (binary release)" of ccache from https://ccache.dev/download.html.

2. Extract the contents of the zip file somewhere convenient but not on $PATH.

    - For example, if Cataclysm is checked out at `C:/dev/Cataclysm-DDA/`, then extract the folder and move the contents to `C:/dev/ccache/`. Verify the binary exists at `C:/dev/ccache/ccache.exe`.

3. Create a copy of `ccache.exe` in the same folder, called `cl.exe`.

    - If you use the LLVM toolchain ("clang-cl.exe") when building, make another copy of `ccache.exe` called `clang-cl.exe`.

4. Create a file called `Directory.Build.props` at the root of the Cataclysm-DDA folder with the following contents. The value of `CDDA_CCACHE_PATH` should be the folder where you put `ccache.exe`. Assuming this path is `C:\dev\ccache\` (note: `\` vs `/` matters, you need to use `\` here):

```
<Project>
  <PropertyGroup>
    <CDDA_USE_CCACHE>true</CDDA_USE_CCACHE>
    <CDDA_CCACHE_PATH>C:\dev\ccache\</CDDA_CCACHE_PATH>
  </PropertyGroup>
</Project>
```

5. ccache should now just work when building with Release modes in Visual Studio. Debug builds do not work because of the size of CDDA and limitations in the msvc toolchain. However, Debug builds are almost intolerably slow anyway so this limitation is not something we are going to fix right now.
