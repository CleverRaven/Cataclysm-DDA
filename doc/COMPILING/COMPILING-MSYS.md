# Compilation guide for 64-bit Windows (using MSYS2)

This guide contains instructions for compiling Cataclysm-DDA on Windows under MSYS2. **PLEASE NOTE:** These instructions *are not intended* to produce a redistributable copy of CDDA. Please download the official builds from the website or [cross-compile from Linux](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/COMPILING/COMPILING.md#cross-compile-to-windows-from-linux) if that is your intention.

These instructions were written using 64-bit Windows 7 and the 64-bit version of MSYS2; the steps should be the same for other versions of Windows.

## Prerequisites:

* Windows 7, 8, 8.1, or 10
* NTFS partition with ~10 Gb free space (~2 Gb for MSYS2 installation, ~3 Gb for repository and ~5 Gb for ccache)
* 64-bit version of MSYS2

**Note:** Windows XP is unsupported!

## Installation:

1. Go to the [MSYS2 homepage](http://www.msys2.org/) and download the installer.

2. Run the installer. It is suggested that you install to a dev-specific folder (C:\dev\msys64\ or similar), but it's not strictly necessary.

3. After installation, run MSYS2 64bit now.

## Configuration:

1. Update the package database and core system packages:

```bash
pacman -Syyu
```

2. MSYS will inform you of a cygheap base mismatch and inform you a forked process died unexpectedly; these errors appear to be due to the nature of `pacman`'s upgrades and *may be safely ignored.* You will be prompted to close the terminal window; do so, then re-start using the MSYS2 MinGW 64-bit menu item.

3. Update remaining packages:

```bash
pacman -Su
```

4. Install packages required for compilation:

```bash
pacman -S git make mingw-w64-x86_64-{astyle,ccache,gcc,libmad,libwebp,pkg-config,SDL2} mingw-w64-x86_64-SDL2_{image,mixer,ttf}
```

5. Close MSYS2.

6. Update path variables in the system-wide profile file (e.g. `C:\dev\msys64\etc\profile`) as following:

- find lines:

```
    MSYS2_PATH="/usr/local/bin:/usr/bin:/bin"
    MANPATH='/usr/local/man:/usr/share/man:/usr/man:/share/man'
    INFOPATH='/usr/local/info:/usr/share/info:/usr/info:/share/info'
```

and

```
    PKG_CONFIG_PATH="/usr/lib/pkgconfig:/usr/share/pkgconfig:/lib/pkgconfig"
```

- and replace them with:

```
    MSYS2_PATH="/usr/local/bin:/usr/bin:/bin:/mingw64/bin"
    MANPATH='/usr/local/man:/usr/share/man:/usr/man:/share/man:/mingw64/share/man'
    INFOPATH='/usr/local/info:/usr/share/info:/usr/info:/share/info:/mingw64/share/man'
```

and

```
    PKG_CONFIG_PATH="/usr/lib/pkgconfig:/usr/share/pkgconfig:/lib/pkgconfig:/mingw64/lib/pkgconfig:/mingw64/share/pkgconfig"
```

## Cloning and compilation:

1. Open MSYS2 and clone the Cataclysm-DDA repository:

```bash
cd /c/dev/
git clone https://github.com/CleverRaven/Cataclysm-DDA.git ./Cataclysm-DDA
```

**Note:** This will download the entire CDDA repository and all of its history (3GB). If you're just testing, you should probably add `--depth=1` (~350MB).

2. Compile with following command line:

```bash
cd Cataclysm-DDA
make -j$((`nproc`+0)) CCACHE=1 RELEASE=1 MSYS2=1 DYNAMIC_LINKING=1 SDL=1 TILES=1 SOUND=1 LOCALIZE=1 LANGUAGES=all LINTJSON=0 ASTYLE=0 RUNTESTS=0
```

You will receive warnings about unterminated character constants; they do not impact the compilation as far as this writer is aware.

**Note**: This will compile a release version with Sound and Tiles support and all localization languages, skipping checks and tests, and using ccache for build acceleration. You can use other switches, but `MSYS2=1`, `DYNAMIC_LINKING=1` and probably `RELEASE=1` are required to compile without issues.

## Running:

1. Run inside MSYS2 from Cataclysm's directory with the following command:

```bash
./cataclysm-tiles
```

**Note:** If you want to run the compiled executable outside of MSYS2, you will also need to update your user or system `PATH` variable with the path to MSYS2's runtime binaries (e.g. `C:\dev\msys64\mingw64\bin`).

## Debugging with Visual Studio Code:

1. Install the debugger gdb via MSYS2:

```bash
pacman -S --needed base-devel mingw-w64-x86_64-toolchain
```

You'll see a repository. The number associated with gdb depends on which packages you already have installed.
Enter the number corresponding to gdb. (if gdb is already installed, it won't appear on the list.)

```
warning: file-5.39-2 is up to date -- skipping
warning: gawk-5.1.0-1 is up to date -- skipping
warning: gdb-9.2-3 is up to date -- skipping
warning: gettext-0.19.8.1-1 is up to date -- skipping
warning: grep-3.0-2 is up to date -- skipping
warning: make-4.3-1 is up to date -- skipping
warning: pacman-5.2.2-13 is up to date -- skipping
warning: perl-5.32.0-2 is up to date -- skipping
warning: sed-4.8-1 is up to date -- skipping
warning: wget-1.21.1-2 is up to date -- skipping
:: There are 46 members in group base-devel:
:: Repository msys
   1) asciidoc  2) autoconf  3) autoconf2.13  4) autogen  5) automake-wrapper
   6) automake1.10  7) automake1.11  8) automake1.12  9) automake1.13
   10) automake1.14  11) automake1.15  12) automake1.16  13) automake1.6
   14) automake1.7  15) automake1.8  16) automake1.9  17) bison  18) btyacc
   19) diffstat  20) diffutils  21) dos2unix  22) flex  23) gettext-devel
   24) gperf  25) groff  26) help2man  27) intltool  28) libtool  29) libunrar
   30) libunrar-devel  31) m4  32) man-db  33) pactoys-git  34) patch
   35) patchutils  36) pkgconf  37) pkgfile  38) quilt  39) reflex  40) scons
   41) swig  42) texinfo  43) texinfo-tex  44) ttyrec  45) unrar  46) xmlto

Enter a selection (default=all):
```

2. Open the Cataclysm-DDA project in VSCode:

- Either via VSCode :
`File -> Open folder`

- Or if you use Github Desktop :
`File -> Options -> Integrations -> External editor` , select Visual Studio Code then `Repository -> Open in Visual Studio Code` (or just press `Ctrl+Shift+A`).

3. Install VSCode extensions:

- You'll need `C/C++ from ms-vscode.cpptools (Microsoft)` and `Shell launcher from Tyriar.shell-launcher (Daniel Immms)`
(Extensions are added with the bottom left button)
![where is extensions button](https://cdn.discordapp.com/attachments/823936842814586880/863898785994506270/unknown.png)
![what C/C++ extension looks like](https://cdn.discordapp.com/attachments/823936842814586880/863898481410965524/unknown.png)
![what Shell launcher extension looks like](https://cdn.discordapp.com/attachments/823936842814586880/863898575255765012/unknown.png)

- Now to configure Shell launcher by editing `settings.json`.
You'll find the file in the `.vscode` folder.
![where to find settings.json](https://cdn.discordapp.com/attachments/823936842814586880/863898325018738688/unknown.png)

Add this (if it throw errors, check the brackets, it has to be included, not just at the end): 

```javascript
"terminal.integrated.profiles.windows": {
    "PowerShell": {
        "source": "PowerShell",
        "icon": "terminal-powershell"
    },
    "Command Prompt": {
        "path": [
            "${env:windir}\\Sysnative\\cmd.exe",
            "${env:windir}\\System32\\cmd.exe"
        ],
        "args": [],
        "icon": "terminal-cmd"
    },
    "Bash": {
        "path": ["D:\\Git\\bin\\bash.exe"]
    },
    "MSYS2": {
        "path": "D:\\MSYS2\\usr\\bin\\bash.exe",
        "label": "MSYS2",
        "args": ["--login", "-i"],
        "env": {
            "MSYSTEM": "MINGW64",
            "CHERE_INVOKING": "1",
            "MSYS2_PATH_TYPE": "inherit"
        }
    }
}
```

Of course, adapt the paths to your needs.

- You can now use the MSYS2 console within VSCode and compile as usual. Multiple console can be used simultaneously. You can add MSYS2 console with the `+` button, bottom right. (don't forget to select the `TERMINAL` tab before)

- Select MSYS2 console as your default console : `+` button, then `Select Default Profile`.
![How to set MSYS2 as default console](https://cdn.discordapp.com/attachments/823936842814586880/863906033394515979/unknown.png)

4. Configure the debugger profile within VSCode:

Edit the `launch.json`file.
You'll find the file in the `.vscode` folder, or alternatively, in the menu `Run -> Open Configurations`.

Paste:

```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(gdb) Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/cataclysm-tiles.exe",
            "args": [],
            "stopAtEntry": true,
            "justMyCode": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "console": "MSYS2",
            "MIMode": "gdb",
            "miDebuggerPath": "D:\\MSYS2\\mingw64\\bin\\gdb.exe",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}
```

Change `miDebuggerPath` to the path of gdb you previously installed.
(`stopAtEntry` can also be set to true.)

It will create a debug profile. The debug button is the 4th, at the left. The name you choose should now appears.
![where is the debug tab](https://cdn.discordapp.com/attachments/823936842814586880/863907995767537684/unknown.png)

5. `Make` debug flags:

First, run a `make clean` within the VSCode MSYS2 console.

Then, when compiling, add `DEBUG_SYMBOLS=1 STRING_ID_DEBUG=1`. If you followed this guide, it looks like this : 

```bash
make -j$((`nproc`+0)) CCACHE=1 RELEASE=1 MSYS2=1 DEBUG_SYMBOLS=1 STRING_ID_DEBUG=1 DYNAMIC_LINKING=1 SDL=1 TILES=1 SOUND=1 LOCALIZE=1 LANGUAGES=all LINTJSON=0 ASTYLE=0 RUNTESTS=0
```

6. (optionnal) Prepare a Task for building in 1 click:

Use the menu `Terminal -> Configure Tasks`, or open `task.json`, in the `.vscode` folder.

```json
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Choose a name",
            "type": "shell",
            "command": "cd github/rep/Cataclysm-DDA;make -j$((`nproc`+0)) CCACHE=1 RELEASE=1 MSYS2=1 DEBUG_SYMBOLS=1 STRING_ID_DEBUG=1 DYNAMIC_LINKING=1 SDL=1 TILES=1 SOUND=1 LOCALIZE=1 LANGUAGES=all LINTJSON=0 ASTYLE=0 RUNTESTS=0",
            "problemMatcher": [],
            "group": {
                "kind": "build",
                "isDefault": true
            }
        }
    ]
}
```

If you have multiple tasks, use the menu `Terminal -> Configure Default Build Task` and select your newly created task as the default build task.
If you want to build and debug in 1 click, add a `preLaunchTask` to `launch.json`.
If you want your task to be conveniently displayed in the... "bottom task bar", install the extension `Tasks from actboy168`. The default build task is then displayed on the very bottom, left.
