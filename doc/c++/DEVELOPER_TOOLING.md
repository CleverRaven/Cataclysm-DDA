# Developer Tooling

## Pre-commit hook

If you have all the relevant tools installed, you can have git automatically
check the style of code and json by adding these commands to your git
pre-commit hook (typically at `.git/hooks/pre-commit`):

```BASH
git diff --cached --name-only -z HEAD | grep -z 'data/.*\.json' | \
    xargs -r -0 -L 1 ./tools/format/json_formatter.[ce]* || exit 1

make astyle-check || exit 1
```

More details below on how to make these work and other ways to invoke these tools.

## Code style (astyle)

Automatic formatting of source code is performed by [Artistic Style](http://astyle.sourceforge.net/).

If you have both `make` and `astyle` installed then this can be done with:

```BASH
make astyle
```

If you have only `astyle` then use:

```BASH
astyle --options=.astylerc --recursive src/*.cpp,*.h tests/*.cpp,*.h
```

On Windows, there is an [AStyle extension for Visual Studio 2019](https://github.com/lukamicoder/astyle-extension) with an unmerged update for [Visual Studio 2022](https://github.com/lukamicoder/astyle-extension/pull/21).

#### Instruction:

1. Install aforementioned extension to Visual Studio IDE.

2. Go to `Tools` - `Options` - `AStyle Formatter` - `General`.

3. Import `https://raw.githubusercontent.com/CleverRaven/Cataclysm-DDA/master/msvc-full-features/AStyleExtension-Cataclysm-DDA.cfg` on `Export/Import` tab using `Import` button:

![image](https://user-images.githubusercontent.com/16213433/54817923-1d85c200-4ca9-11e9-95ac-e1f84394429b.png)

4. After import is successful you can see imported rules on `C/C++` tab:

![image](https://user-images.githubusercontent.com/16213433/54817974-427a3500-4ca9-11e9-8179-84b19cc25c0f.png)

5. Close `Options` menu, open file to be astyled and use `Format Document (Astyle)` or `Format Selection (Astyle)` commands from `Edit` - `Advanced` menu.

![image](https://user-images.githubusercontent.com/16213433/54818041-68073e80-4ca9-11e9-8e1f-a1996fd4ee75.png)

*Note:* You can also configure keybindings for aforementioned commands in `Tools` - `Options` - `Environment` - `Keybindings` menu:

![image](https://user-images.githubusercontent.com/16213433/54818153-aac91680-4ca9-11e9-80e6-51e243b2b33b.png)

## JSON style

See the [JSON style guide](../JSON/JSON_STYLE.md).

## ctags

In addition to the usual means of creating a `tags` file via e.g. [`ctags`](http://ctags.sourceforge.net/), we provide `tools/json_tools/cddatags.py` to augment a `tags` file with locations of definitions taken from CDDA's JSON data.  `cddatags.py` is designed to safely update a tags file containing source code tags, so if you want both types of tags in your `tags` file then you can run `ctags -R . && tools/json_tools/cddatags.py`.  Alternatively, there is a rule in the `Makefile` to do this for you; just run `make ctags` or `make etags`.


## clang-tidy

Cataclysm has a [clang-tidy configuration file](../.clang-tidy) and if you have
`clang-tidy` available you can run it to perform static analysis of the
codebase.  We test with `clang-tidy` from LLVM 18.0.0 with CI, so for the most
consistent results, you might want to use that version.

To run it, you have a few options.

* `clang-tidy` ships with a wrapper script `run-clang-tidy.py`.

* Use CMake's built-in support by adding `-DCMAKE_CXX_CLANG_TIDY=clang-tidy`
  or similar, pointing it to your chosen clang-tidy version.

* To run `clang-tidy` directly try something like
```sh
grep '"file": "' build/compile_commands.json | \
    sed "s+.*$PWD/++;s+\"$++" | \
    egrep '.' | \
    xargs -P 9 -n 1 clang-tidy -quiet
```
To focus on a subset of files add their names into the `egrep` regex in the
middle of the command-line.

### Custom clang-tidy plugin

We have written our own clang-tidy checks in a custom plugin.  Unfortunately,
`clang-tidy` as distributed by LLVM doesn't support plugins, so making this
work requires some extra steps.

#### Extreme tl;dr for Ubuntu Focal (including WSL)
The following set of commands should take you from zero to running clang-tidy equivalent to the CI job. This will lint all sources in a random order.
```sh
sudo apt install build-essential cmake clang-18 libclang-18-dev llvm-18 llvm-18-dev llvm-18-tools pip
sudo pip install compiledb lit
test -f /usr/bin/python || sudo ln -s /usr/bin/python3 /usr/bin/python
# The following command invokes clang-tidy exactly like CI does
COMPILER=clang++-18 CLANG=clang++-18 CMAKE=1 CATA_CLANG_TIDY=plugin TILES=1 LOCALIZE=0 ./build-scripts/clang-tidy-build.sh
COMPILER=clang++-18 CLANG=clang++-18 CMAKE=1 CATA_CLANG_TIDY=plugin TILES=1 LOCALIZE=0 ./build-scripts/clang-tidy-run.sh
```

#### Ubuntu Focal

If you are on Ubuntu Focal then you might be able to get it working the same
way our CI does.  Add the LLVM 18 Focal source [listed
here](https://apt.llvm.org/) to your `sources.list`, install the needed packages (`clang-18
libclang-18-dev llvm-18-dev llvm-18-tools`), and build Cataclysm with CMake,
adding `-DCATA_CLANG_TIDY_PLUGIN=ON`.

[apt.llvm.org](apt.llvm.org) also has an automated install script.

#### Other Linux distributions

On other distributions you will probably need to build `clang-tidy` yourself.
* Expect this process to take about 80GB of disk space.
* Check out the `llvm` project, release 18 branch, with e.g.
  `git clone --branch release/18.x --depth 1 https://github.com/llvm/llvm-project.git llvm-18`.
* Enter the newly cloned repo `cd llvm-18`.
* Patch in plugin support for `clang-tidy` using [this
  patch](https://github.com/jbytheway/clang-tidy-plugin-support/blob/master/plugin-support.patch).
  `curl https://raw.githubusercontent.com/jbytheway/clang-tidy-plugin-support/master/plugin-support.patch | patch -p1`
* Configure LLVM using CMake, including the
  `-DCMAKE_EXE_LINKER_FLAGS="-rdynamic"` option.  Some additional options below
  are simply to reduce the amount of stuff that gets built.  These might nee to
  be adjusted to your situation (e.g. if you're on another architecture then
  choose that target instead of X86).
  ```sh
  mkdir build
  cd build
  cmake \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_EXE_LINKER_FLAGS="-rdynamic" \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;compiler-rt' \
    ../llvm
  ```
* Build LLVM `make -j$(nproc)`.  This can take a long time (maybe 6 core-hours
  or more).
* Add the `build/bin` directory to your path so that `clang-tidy` and
  `FileCheck` are found from there.

Then you can use your locally built `clang-tidy` to compile Cataclysm.  You'll
need to use the CMake version of the Cataclysm build rather than the `Makefile`
build.  Add the following CMake options:
```sh
-DCATA_CLANG_TIDY_PLUGIN=ON
-DCATA_CLANG_TIDY_INCLUDE_DIR="$llvm_dir/clang-tools-extra/clang-tidy"
-DCATA_CHECK_CLANG_TIDY="$llvm_dir/clang-tools-extra/test/clang-tidy/check_clang_tidy.py"
```
where `$llvm_dir` is the location of your LLVM source directory.

To run `clang-tidy` with this plugin enabled add the
`'-plugins=$build_dir/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so'` option
to your `clang-tidy` command line.

If you wish to run the tests for the custom clang-tidy plugin you will also
need `lit`.  This will be built as part of LLVM, or you can install it via
`pip` or your local package manager if you prefer.

Then, assuming `build` is your Cataclysm build directory, you can run the tests
with
```sh
lit -v build/tools/clang-tidy-plugin/test
```

#### Windows

If you just want to run the default clang-tidy checks, and skip the cata-specific ones, the easiest way is probably to install Clang Power Tools visual studio extension (in the `Extensions` > `Manage extensions..` menu).

Otherwise it is probably faster and easier to install WSL and follow the steps described above in [Extreme tl;dr for Ubuntu Focal (including WSL)](<#extreme-tldr-for-ubuntu-focal-including-wsl>). However building on windows is still possible.

You will need:
- windows powershell console
- Git
- CMake
- Python
- A working C++ compiler that can be run from the windows console, such as MSBuild (aka "Visual Studio Build Tools"; it's part of Visual Studio, but can also be obtained without VS from [this link](https://visualstudio.microsoft.com/downloads/?q=build+tools) (scroll down)) or standalone clang or gcc
  - notably, you *can't* use the one provided by msys2/mingw for this (or, well, you can, but you are on your own then)
  - (optionall) specifically clang compiler. Can often be downloaded from llvm github https://github.com/llvm/llvm-project/releases . If you don't have one, you can build clang from source with the instructions below
- Ninja build system
- (for running tests) GNU core utilities or an msys2 install (specifically we just need the `diff` tool).
- vcpkg

There are compromises, and you can make do with a different set of tools, but these are what this doc will use.

The rough outline of the process is:
- Obtain a working set of LLVM libraries
- Build clang-tidy with custom checks
- (optional) Test that the checks work
- Run the custom clang-tidy

##### Build LLVM

First of all you would need a working LLVM. You *might* be able to [download it](https://github.com/llvm/llvm-project/releases) from llvm-project github (look for file named something like `clang+llvm-19.1.5-x86_64-pc-windows-msvc.tar.xz`), and it *might* work, but there is no guarantee. I suggest you try it, and skip to the next section. If it works for you - great! You just saved yourself several hours of building it from source, if not - read on.

Ideally you should try to match the llvm version you download with the one that we build the CI against, as different versions yield slightly different results. And also there is no stability promise so that the checks that compile against llvm-17 might stop doing so in llvm-20.
At the time of writing we are building against llvm-17, if you are reading this in the future, check [the CI definition](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/.github/workflows/clang-tidy.yml) to know what's current.


The instruction for building LLVM can be found at https://clang.llvm.org/get_started.html, but duplicating here with some emphasis.

Clone the LLVM repo from https://github.com/llvm/llvm-project.git and checkout the appropriate branch.
The LLVM repo is pretty big so it's recommended to fetch just the commit you want to build at:

```ps1
git clone --depth=1 --branch=release/17.x https://github.com/llvm/llvm-project.git
```

The first step to build the code is to run CMake to generate the makefile.
On the root dir of LLVM, run the following script.
Make sure CMake and python are on the path.

```ps1
# for visual studio
mkdir -p build
cd build
cmake `
    -G "Visual Studio 16 2019" `
    -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;lld' `
    -DLLVM_TARGETS_TO_BUILD='X86' `
    ../llvm

# for non-visual studio
mkdir -p build
cd build
cmake `
    -G "Ninja" `
    -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra;lld' `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    -DLLVM_TARGETS_TO_BUILD='X86' `
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON `
    -DLLVM_HOST_TRIPLE=x86_64 `
    ../llvm
```

If you're using a non-msvc compiler you either need its containing folder on your `PATH`, specify said containing folder in `-DCMAKE_PROGRAM_PATH=<your/path/here>` or specify the full path to the binary in `-DCMAKE_C_COMPILER` as the error message should suggest. Same goes for ninja and python.


Now that we have the build files, we can actually build it by running:

```ps1
# for Visual Studio / MSBuild
cmake --build . --target=clang-tidy --target=clangTidyMain --target=FileCheck   --config=RelWithDebInfo
# for ninja
cmake --build . --target=clang-tidy --target=clangTidyMain --target=FileCheck   --parallel 4
```

If you don't have clang already, add a `--target=clang --target=llvm-rc --target=lld --target=llvm-objdump` to the command line above.
Remove `--target=FileCheck` if you don't intend to test custom checks.
LLVM is a behemoth so it will take several hours to finish while using 100% of your CPU, even when building only just clang-tidy. Consider stepping out and doing something productive while it's running. (On my machine it took 2h30m to build clang-tidy itself, and 2h to build the other tools afterwards).

Now, if you wanted to make things super smooth going forward, you could build all the targets (not just the select few) and then bundle the built llvm to some nice place via
```ps1
cmake --build . --config=RelWithDebInfo # no target specification
cmake --install --prefix=<some-path-here> .
```
That *might* even only add an hour or so to the total time? Your call whether it's worth it.


##### Build clang-tidy with custom checks

After building clang-tidy as a library from the LLVM source, the next step is to
build clang-tidy as an executable, with the custom checks from the CDDA source.

We will need to run a cmake with lots of arguments, and it gets unwieldy on the command line. So create `CMakeUserPresets.json` file at the root of cataclysm project with the following contents, replacing the paths in `<>` as appropriate:

```jsonc
{
  "version": 2,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 20,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "cata-clang-tidy",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/out/build/${presetName}/",
      "environment":{
        "MY_LLVM_INSTALL_DIR": "<C:/path/to/installed-llvm>",
        "VCPKG_ROOT": "<C:/path/to/your/vcpkg>"
      },
      "cacheVariables": {
        "CATA_CLANG_TIDY_INCLUDE_DIR": "<C:/path/to/llvm-source>/clang-tools-extra/",
        "CMAKE_PROGRAM_PATH": "<see-below>",


        "LLVM_DIR": "$env{MY_LLVM_INSTALL_DIR}/lib/cmake/llvm",
        "Clang_DIR": "$env{MY_LLVM_INSTALL_DIR}/lib/cmake/clang",
        "CMAKE_BUILD_TYPE": "RelWithDebInfo",
        "CATA_CHECK_CLANG_TIDY": "python ${sourceDir}/tools/clang-tidy-plugin/test/check_clang_tidy.py -clang-tidy-binary=${sourceDir}/out/build/${presetName}/tools/clang-tidy-plugin/CataAnalyzer.exe",
        "CMAKE_EXPORT_COMPILE_COMMANDS": true,
        "CATA_CLANG_TIDY_EXECUTABLE": true,

        "VCPKG_MANIFEST_DIR": "${sourceDir}/msvc-full-features",
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
        "TILES": true, "SOUND": true, "BACKTRACE": true,
        "LOCALIZE": true,
        "GETTEXT_MSGFMT_BINARY": "cat"
      }
    }
  ]
}
```

If you downloaded the packaged llvm archive, or `install`ed it yourself, then `<C:/path/to/installed-llvm>` would point to that. Otherwise it would be `<C:/path/to/llvm-source>/build`.
If you have a packaged version, you can omit `"Clang_DIR"` and  `"CATA_CLANG_TIDY_INCLUDE_DIR"`
If you don't intend to run tests in the next step - skip `"CATA_CHECK_CLANG_TIDY"`.
Adjust `"TILES"`, `"SOUND"`, `"BACKTRACE"` and `"LOCALIZE"` as you would for building the game normally, although it is strongly suggested to keep `"LOCALIZE": true` to avoid excessive translation-related false-positives.
`CMAKE_PROGRAM_PATH` should contain the list of directories that hold compiler binaries that are not already in your PATH env variable. I.e. if you have a packaged llvm, set `CMAKE_PROGRAM_PATH` to `<C:/path/to/installed-llvm>/bin/`; otherwise set it to `<C:/path/to/llvm-source>/build/RelWithDebInfo/bin/`. You can add Ninja and python to this list (semicolon-separated).


Run cmake to generate build files under `.\out\build\cata-clang-tidy\`:
```ps1
cmake --preset=cata-clang-tidy
```
(If you adjust the configuration, and then rerun the command but the changes don't seem to be picked up, add a `--fresh` argument)

And then build it
```ps1
cmake --build  .\out\build\cata-clang-tidy\ --target=CataAnalyzer
```

Finally, confirm that you do, in fact, have the cata-specific checks:
```ps1
> .\out\build\cata-clang-tidy\tools\clang-tidy-plugin\CataAnalyzer.exe '--checks=-*,cata-*' --list-checks
Enabled checks:
    cata-almost-never-auto
    cata-assert
    cata-avoid-alternative-tokens
    cata-combine-locals-into-point
    cata-determinism
    cata-header-guard
    cata-json-translation-input
    cata-large-inline-function
    cata-large-stack-object
    cata-no-long
    cata-no-static-translation
    cata-ot-match
    cata-point-initialization
    cata-redundant-parentheses
    cata-serialize
    cata-simplify-point-constructors
    cata-static-declarations
    cata-static-initialization-order
    cata-static-int_id-constants
    cata-static-string_id-constants
    cata-test-filename
    cata-tests-must-restore-global-state
    cata-text-style
    cata-translate-string-literal
    cata-translations-in-debug-messages
    cata-u8-path
    cata-unit-overflow
    cata-unsequenced-calls
    cata-unused-statics
    cata-use-localized-sorting
    cata-use-mdarray
    cata-use-named-point-constants
    cata-use-point-apis
    cata-use-point-arithmetic
    cata-use-string_view
    cata-utf8-no-to-lower-to-upper
    cata-xy
```

##### (Optional) Test custom checks work

LLVM folks seem to never excercise their test driver in windows console, so it has been broken since 2022 and needs a small patch to work.

```ps1
cd <path-to-llvm-source>
git apply -v --ignore-whitespace <path-to-cata-source>\tools\clang-tidy-plugin\lit-win-console-llvm-17.patch
```

You will need to have `FileCheck` (distributed with LLVM or self-built in the above step) and `diff` (either downloaded as part of [GnuWin32 tools](https://getgnuwin32.sourceforge.net/), or if you have msys2 installed then you can find it in `<path-to-msys2>\usr\bin\`) in your PATH:
```ps1
$env:PATH += ';<C:\path\to\installed-llvm>\bin\;<C:\msys2>\usr\bin'
```
Note the leading `;`

Once you have that, run the test suite
```ps1
cd <path-to-cata-source>
python <path-to-llvm-source>\llvm\utils\lit\lit.py -v .\out\build\cata-clang-tidy\tools\clang-tidy-plugin\test
```

#### Run custom-built clang-tidy on your code

Now you should have a clang-tidy binary at  `.\out\build\cata-clang-tidy\tools\clang-tidy-plugin\CataAnalyzer.exe` (which you can move to a more convenient place)
and (as long as you passed `CMAKE_EXPORT_COMPILE_COMMANDS`) a compilation database at `.\out\build\cata-clang-tidy\compile_commands.json`

You can now run clang-tidy on a file of your choosing as
```ps1
CataAnalyzer.exe -p .\out\build\cata-clang-tidy\ --header-filter='nothing'  src\ascii_art.cpp
```


#### Windows MinGW-w64

(This section appears to be out of date, but is preserved for historical reasons, in case someone wishes to revive it)

##### Build LLVM

It is probably faster and easier to install WSL and follow the steps described above in [Extreme tl;dr for Ubuntu Focal (including WSL)](<#extreme-tldr-for-ubuntu-focal-including-wsl>).

To build LLVM natively on Windows, you'll first need to get some tools installed.
- Cmake
- Python 3
- MinGW-w64 (other compilers may or may not work. Clang itself does not seem to be
building LLVM on Windows correctly.)
- A shell environment

After the tools are installed, a patch still needs to be applied before building
LLVM, since `clang-tidy` as distributed by LLVM doesn't support plugins.

First, clone the LLVM repo from, for example, [the official github repo](https://github.com/llvm/llvm-project.git).
Checkout the `release/18.x` branch, since that's what our patch was based on.

On Windows, in addition to applying `plugin-support.patch` mentioned in the previous section, you
should also apply
[`clang-tidy-scripts.patch`](https://github.com/jbytheway/clang-tidy-plugin-support/blob/master/clang-tidy-scripts.patch)
so you can run the lit test with the custom clang-tidy executable and let
clang-tidy apply suggestions automatically.

After the patch is applied, you can then build the LLVM code. Unfortunately, it
seems that clang itself cannot correctly compile the LLVM code on Windows (gives
some sort of relocation error). Luckily, MinGW-w64 can be used instead to compile
the code.

The first step to build the code is to run CMake to generate the makefile. On
the root dir of LLVM, run the following script (substitute values inside `<>`
with the actual paths). Make sure CMake, python, and MinGW-w64 are on the path.

```sh
mkdir -p build
cd build
cmake \
    -DCMAKE_MAKE_PROGRAM="<mingw-w64-root>/bin/mingw32-make" \
    -G "MSYS Makefiles" \
    -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra' \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DLLVM_TARGETS_TO_BUILD='X86' \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    ../llvm
```

The next step is to call `make` to actually build clang-tidy as a library.
When using MinGW-w64 to build, you should call `mingw32-make` instead.
Also, because `FileCheck` is not shipped with Windows, you'll also need to build
it yourself using LLVM sources by adding the `FileCheck` target to the make command.

```sh
mkdir -p build
cd build
mingw32-make -j4 clang-tidy clangTidyMain FileCheck
```

Here `clang-tidy` is only added to trigger the building of several targets that
are needed to build our custom clang-tidy executable later.

##### Build clang-tidy with custom checks

After building clang-tidy as a library from the LLVM source, the next step is to
build clang-tidy as an executable, with the custom checks from the CDDA source.

In this step, the following tools are required.
- Python 3
- CMake
- MinGW-w64
- FileCheck (built from the LLVM source)
- A shell environment

You also need to install yaml for python 3 to work. Download the `.whl` installer
corresponding to your python version from [here](https://pyyaml.org/wiki/PyYAML)
and execute the following command inside the `<python3_root>/Scripts` directory
```sh
pip install path/to/your/downloaded/file.whl
```

The next step is to run CMake to generate the compilation database. The compilation
database contains compiler flags that clang-tidy uses to check the source files.

Make sure Python 3, CMake, MinGW-w64, and FileCheck are on the path.
Note that two `bin` directories of MinGW-w64 should be on the path: `<mingw-w64-root>/bin`,
and `<mingw-w64-root>/x86_64-w64-mingw32/bin`. FileCheck's path is `<llvm-source-root>/build/bin`,
if you built it with the instructions in the previous section.

Then add the following CMake options to generate the compilation database
(substitute values inside `<>` with the actual paths) and build the CDDA source
and the custom clang-tidy executable with `mingw32-make`. In this tutorial we
run CMake and `mingw32-make` in the `build` subdirectory. Note that `DCATA_CLANG_TIDY_EXECUTABLE`
is defined instead of `DCATA_CLANG_TIDY_PLUGIN`.

```sh
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
-DLLVM_DIR="<llvm-source-root>/build/lib/cmake/llvm"
-DClang_DIR="<llvm-source-root>/build/lib/cmake/clang"
-DLLVM_INCLUDE_DIRS="<llvm-source-root>/llvm/include"
-DCLANG_INCLUDE_DIRS="<llvm-source-root>/clang/include"
-DCATA_CLANG_TIDY_EXECUTABLE=ON
-DCATA_CLANG_TIDY_INCLUDE_DIR="<llvm-source-root>/clang-tools-extra/clang-tidy"
-DCATA_CHECK_CLANG_TIDY="<llvm-source-root>/clang-tools-extra/test/clang-tidy/check_clang_tidy.py -clang-tidy=<cdda-source-root>/build/tools/clang-tidy-plugin/CataAnalyzer.exe"
```

Next, change the directory back to the source root and run `tools/fix-compilation-database.py`
with Python 3 to fix some errors in the compilation database. Then the compilation
database should be usable by clang-tidy.

If you want to check if the custom checks are working correctly, run the following
script.

```sh
python3 <llvm-source-root>/llvm/utils/lit/lit.py -v build/tools/clang-tidy-plugin/test
```

Finally, use the following command to run clang-tidy with the custom checks.
In the following command, the first line of "-extra-arg"s are used to tell
clang-tidy to mimic g++ behavior, and the second line of "-extra-arg"s are
used to tell clang-tidy to use clang's x86intrin.h instead of g++'s, so as
to avoid compiler errors.

```sh
python3 <llvm-source-root>/clang-tools-extra/clang-tidy/tool/run-clang-tidy.py \
    -clang-tidy-binary=build/tools/clang-tidy-plugin/CataAnalyzer.exe \
    -p=build "\.cpp$" \
    -extra-arg=-target -extra-arg=x86_64-pc-windows-gnu -extra-arg=-pthread -extra-arg=-DSDL_DISABLE_ANALYZE_MACROS \
    -extra-arg=-isystem -extra-arg=<llvm-source-root>/clang/lib/Headers
```

You can also add `-fix-errors` to apply fixes reported by the checks or
`-checks="-*,xxx,yyy"` to specify the checks you would like to run.

## include-what-you-use

[include-what-you-use](https://github.com/include-what-you-use/include-what-you-use)
(IWYU) is a project intended to optimize includes.  It will calculate the
required headers and add and remove includes as appropriate.

Running IWYU on this codebase revealed some issues.  You will need a version of IWYU
where the following PR has been merged (which has not yet happened at time of
writing, but with luck might make it into the clang-10 release of IWYU):

* https://github.com/include-what-you-use/include-what-you-use/pull/775

Once you have IWYU built, build the codebase using CMake, with
`CMAKE_EXPORT_COMPILE_COMMANDS=ON` on to create a compilation database
(Look for `compile_commands.json` in the build dir to see whether that worked).

Then run:

```
iwyu_tool.py -p $CMAKE_BUILD_DIR/compile_commands.json -- -Xiwyu --mapping_file=$PWD/tools/iwyu/cata.imp | fix_includes.py --nosafe_headers --reorder
```

IWYU will sometimes add C-style library headers which clang-tidy doesn't like,
so you might need to run clang-tidy (as described above) and then re-run IWYU a
second time.

There are mapping files in `tools/iwyu` intended to help IWYU pick the right
headers.  Mostly they should be fairly obvious, but the SDL mappings might
warrant further explanation.  We want to force most SDL includes to go via
`sdl_wrappers.h`, because that handles the platform-dependence issues (the
include paths are different on Windows).  There are a couple of exceptions
(`SDL_version.h` and `SDL_mixer.h`).  The former is because `main.cpp` can't
include all SDL headers, because they `#define WinMain`.  All the mappings in
`sdl.imp` are designed to make this happen.

We have to use IWYU pragmas in some situations.  Some of the reasons are:

* IWYU has a concept of [associated
  headers](https://github.com/include-what-you-use/include-what-you-use/blob/master/docs/IWYUPragmas.md#iwyu-pragma-associated),
  where each cpp file can have some number of such headers.  The cpp file is
  expected to define the things declared in those headers.  In Cata, the
  mapping between headers and cpp files is not nearly so simple, so there are
  files with multiple associated headers, and files with none.  Headers that
  are not the associated header of any cpp file will not get their includes
  updated, which could lead to broken builds, so ideally all headers would be
  associated to some cpp file.  You can use the following command to get a list
  of headers which are not currently associated to any cpp file (requires GNU
  sed):

```
diff <(ls src/*.h | sed 's!.*/!!') <(for i in src/*.cpp; do echo $i; sed -n '/^#include/{p; :loop n; p; /^$/q; b loop}' $i; done | grep 'e "' | grep -o '"[^"]*"' | sort -u | tr -d '"')
```

* Due to a [clang bug](https://bugs.llvm.org/show_bug.cgi?id=20666), uses in
  template arguments to explicit instantiations are not counted, which leads to
  some need for `IWYU pragma: keep`.

* Due to
  [these](https://github.com/include-what-you-use/include-what-you-use/blob/4909f206b46809775e9b5381f852eda62cbf4bf7/iwyu.cc#L1617)
  [missing](https://github.com/include-what-you-use/include-what-you-use/blob/4909f206b46809775e9b5381f852eda62cbf4bf7/iwyu.cc#L1629)
  features of IWYU, it does not count uses in template arguments to return
  types, which leads to other requirements for `IWYU pragma: keep`.

* IWYU seems to have particular trouble with types used in maps and
  `cata::optional` (NOTE: cata::optional replaced with std::optional around the C++17 migration).
  Have not looked into this in detail, but again worked around it with pragmas.

## Python and pyvips on Windows

They are needed to work with `compose.py` and some other tileset infrastructure scripts. See [TILESET.md](/doc/TILESET.md#pyvips)
