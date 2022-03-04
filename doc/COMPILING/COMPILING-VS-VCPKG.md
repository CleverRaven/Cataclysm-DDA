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

3. Install and configure `vcpkg`. If you already have `vcpkg` installed, you need to update it to at least commit `49b67d9cb856424ff69f10e7721aec5299624268` but not newer than `c0bc5e1b0ff27dd8710ee9ca18b708c0a9accce1` and rerun `.\bootstrap-vcpkg.bat` as described:

***WARNING: It is important that, wherever you decide to clone this repo, the path does not include whitespace. That is, `C:/dev/vcpkg` is acceptable, but `C:/dev test/vcpkg` is not.***

In a `cmd.exe` shell:
```cmd
REM cd to the appropriate folder first
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
git checkout 49b67d9cb856424ff69f10e7721aec5299624268
.\bootstrap-vcpkg.bat -disableMetrics
.\vcpkg integrate install
```
In a Git Bash shell, the commands are almost the same except the filesystem path separator is `/` instead of `\`.
```
# cd to the appropriate folder first
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
git checkout 49b67d9cb856424ff69f10e7721aec5299624268
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

    - **Note:** If you are using Visual Studio 2022, the first time you open the solution, it will prompt you to "Retarget Projects". Unless you know what you are doing, hit cancel on this dialog.

3. Open the `Build > Configuration Manager` menu and adjust `Active solution configuration` and `Active solution platform` to match your intended target.

    - The `Release` configuration and `x64` platform together make a good default setting. `Debug` is too slow and should be reserved for breakpoint debugging with code stepping.
    - This will configure Visual Studio to compile the release version, with support for Sound, Tiles, and Localization (note, however, that language files themselves are not automatically compiled; this will be done later).

4. Start the build process by selecting either `Build > Build Solution` or `Build > Build > 1 Cataclysm-vcpkg-static`. The process may take a long period of time, so you'd better prepare a cup of coffee and some books in front of your computer :) The first build of each architecture will also download and install dependencies through vcpkg, which can take an especially long time.

5. If you need localization support, execute the bash script `lang/compile_mo.sh` inside Git Bash GUI just like on a UNIX-like system. This will compile the language files that were not automatically compiled in step 2 above.

Even if you do not need languages other than English, you may still want to execute `lang/compile_mo.sh` to compile the language files if you're planning to run the unit tests, since those rely on the language files existing.

### Debugging

1. Ensure that the Cataclysm project (`Cataclysm-vcpkg-static`) is the selected startup project.

    - Right click the project in the Solution Explorer pane, select `Set as Startup Project`

2. Configure the working directory in the project settings to `$(ProjectDir)..`

    - Right click the project in the Solution Explorer pane, select `Properties`
    - Under `Configuration Properties > Debugging`, change `Working Directory` to `$(ProjectDir)..`
    - **Note:** If you change configurations (eg from `Release` to `Debug`), you will have to repeat this step. You only have to do it once per configuration, the setting will persist.

If you discover that after pressing the debug button in Visual Studio, Cataclysm just exits after launch with return code 1, that is because of the wrong working directory.

When debugging, it is not strictly necessary to use a `Debug` build; `Release` builds run significantly faster, can still be run in the debugger, and most of the time will have most of the information you need.

### Running unit tests

Ensure that the Cataclysm test project (`Cataclysm-test-vcpkg-static`) is the selected startup project, configure the working directory in the project settings to `$(ProjectDir)..`, and then press the debug button (or use the appropriate shortcut, e.g. F5). This will run all of the unit tests.

Additional command line arguments may be configured in the project's command line arguments setting, or if you are using a compatible unit test runner (e.g. Resharper) you can run or debug individual tests from the unit test sessions.
You can also start the test runner library manually from windows console. Run it with `--help` for an overview of the arguments.

### Make a distribution

There is a batch script in `msvc-full-features` folder `distribute.bat`. It will create a sub folder `distribution` and copy all required files(eg. `data/`, `Cataclysm.exe` and dlls) into that folder. Then you can zip it and share the archive on the Internet.
