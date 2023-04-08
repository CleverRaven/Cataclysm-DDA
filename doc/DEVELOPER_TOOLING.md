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

See the [JSON style guide](JSON_STYLE.md).

## ctags

In addition to the usual means of creating a `tags` file via e.g. [`ctags`](http://ctags.sourceforge.net/), we provide `tools/json_tools/cddatags.py` to augment a `tags` file with locations of definitions taken from CDDA's JSON data.  `cddatags.py` is designed to safely update a tags file containing source code tags, so if you want both types of tags in your `tags` file then you can run `ctags -R . && tools/json_tools/cddatags.py`.  Alternatively, there is a rule in the `Makefile` to do this for you; just run `make ctags` or `make etags`.


## clang-tidy

Cataclysm has a [clang-tidy configuration file](../.clang-tidy) and if you have
`clang-tidy` available you can run it to perform static analysis of the
codebase.  We test with `clang-tidy` from LLVM 12.0.0 with CI, so for the most
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
sudo apt install build-essential cmake clang-12 libclang-12-dev llvm-12 llvm-12-dev llvm-12-tools pip
sudo pip install compiledb lit
test -f /usr/bin/python || sudo ln -s /usr/bin/python3 /usr/bin/python
# The following command invokes clang-tidy exactly like CI does
COMPILER=clang++-12 CLANG=clang++-12 CMAKE=1 CATA_CLANG_TIDY=plugin TILES=1 LOCALIZE=0 ./build-scripts/clang-tidy.sh
```

#### Ubuntu Focal

If you are on Ubuntu Focal then you might be able to get it working the same
way our CI does.  Add the LLVM 12 Focal source [listed
here](https://apt.llvm.org/) to your `sources.list`, install the needed packages (`clang-12
libclang-12-dev llvm-12-dev llvm-12-tools`), and build Cataclysm with CMake,
adding `-DCATA_CLANG_TIDY_PLUGIN=ON`.

#### Other Linux distributions

On other distributions you will probably need to build `clang-tidy` yourself.
* Expect this process to take about 80GB of disk space.
* Check out the `llvm` project, release 12 branch, with e.g.
  `git clone --branch release/12.x --depth 1 https://github.com/llvm/llvm-project.git llvm-12`.
* Enter the newly cloned repo `cd llvm-12`.
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
Checkout the `release/12.x` branch, since that's what our patch was based on.

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
  `cata::optional`.  Have not looked into this in detail, but again worked
  around it with pragmas.

## Python and pyvips on Windows

They are needed to work with `compose.py` and some other tileset infrastructure scripts. See [TILESET.md](/doc/TILESET.md#pyvips)
