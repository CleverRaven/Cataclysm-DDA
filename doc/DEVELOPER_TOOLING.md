## Code style (astyle)

Automatic formatting of source code is performed by [Artistic Style](http://astyle.sourceforge.net/).

If you have both `make` and `astyle` installed then this can be done with:

```BASH
make astyle
```

If you have only `astyle` then use:

```BASH
astyle --options=.astylerc --recursive src/*.cpp,*.h tests/*.cpp,*.h`
```

On Windows, there is an [AStyle extension for Visual Studio](https://github.com/lukamicoder/astyle-extension)

#### Instruction:

1. Install aforementioned extension to Visual Studio IDE.

2. Go to `Tools` - `Options` - `AStyle Formatter` - `General`.

3. Import `https://github.com/CleverRaven/Cataclysm-DDA/blob/master/msvc-full-features/AStyleExtension-Cataclysm-DDA.cfg` on `Export/Import` tab using `Import` button:

![image](https://user-images.githubusercontent.com/16213433/54817923-1d85c200-4ca9-11e9-95ac-e1f84394429b.png)

4. After import is successful you can see imported rules on `C/C++` tab:

![image](https://user-images.githubusercontent.com/16213433/54817974-427a3500-4ca9-11e9-8179-84b19cc25c0f.png)

5. Close `Options` menu, open file to be astyled and use `Format Document (Astyle)` or `Format Selection (Astyle)` commands from `Edit` - `Advanced` menu.

![image](https://user-images.githubusercontent.com/16213433/54818041-68073e80-4ca9-11e9-8e1f-a1996fd4ee75.png)

*Note:* You can also configure keybindings for aforementioned commands in `Tools` - `Options` - `Environment` - `Keybindings` menu:

![image](https://user-images.githubusercontent.com/16213433/54818153-aac91680-4ca9-11e9-80e6-51e243b2b33b.png)

## JSON style

See the [JSON style guide](JSON_STYLE.md).

## clang-tidy

Cataclysm has a [clang-tidy configuration file](../.clang-tidy) and if you have
`clang-tidy` available you can run it to perform static analysis of the
codebase.  We test with `clang-tidy` from LLVM 8.0.1 on Travis, so for the most
consistent results, you might want to use that version.

To run it you have a few options.

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

#### Ubuntu Xenial

If you are on Ubuntu Xenial then you might be able to get it working the same
way Travis does.  Add the LLVM 8 Xenial source [listed
here](https://apt.llvm.org/) to your `sources.list`, install the `clang-8
libclang-8-dev llvm-8-dev llvm-8-tools` packages and build Cataclysm with CMake
adding `-DCATA_CLANG_TIDY_PLUGIN=ON`.

On other distributions you will probably need to build `clang-tidy` yourself.
* Check out the `llvm`, `clang`, and `clang-tools-extra` repositories in the
  required layout (as described for example
  [here](https://quuxplusone.github.io/blog/2018/04/16/building-llvm-from-source/).
* Patch in plugin support for `clang-tidy` using [this
  patch](https://github.com/jbytheway/clang-tidy-plugin-support/blob/master/plugin-support.patch).
* Configure LLVM using CMake, including the
  `-DCMAKE_EXE_LINKER_FLAGS="-rdynamic"` option.
* Add the `build/bin` directory to your path so that `clang-tidy` and
  `FileCheck` are found from there.

Then you can use your locally build `clang-tidy` to compile Cataclysm.  You'll
need to use the CMake version of the Cataclysm build rather than the `Makefile`
build.  Add the following CMake options:
```sh
-DCATA_CLANG_TIDY_PLUGIN=ON
-DCATA_CLANG_TIDY_INCLUDE_DIR="$extra_dir/clang-tidy"
-DCATA_CHECK_CLANG_TIDY="$extra_dir/test/clang-tidy/check_clang_tidy.py"
```
where `$extra_dir` is the location of your `clang-tools-extra` checkout.

To run `clang-tidy` with this plugin enabled add the
`'-plugins=$build_dir/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so'` option
to your `clang-tidy` command line.

If you wish to run the tests for the custom clang-tidy plugin you will also
need `lit`.  This will be built as part of `llvm`, or you can install it via
`pip` or your local package manager if you prefer.

Then, assuming `build` is your Cataclysm build directory, you can run the tests
with
```sh
lit -v build/tools/clang-tidy-plugin/test
```

#### Windows

##### Build LLVM

To build llvm on Windows, you'll first need to get some tools installed.
- Cmake
- Python 3 (Python 2 may not work for building llvm, but it's still required to run
the lit test, which will be discussed in the next section.)
- MinGW-w64 (other compilers may or may not work. Clang itself does not seem to be
building llvm on Windows correctly.)
- A shell environment

After the tools are installed, a patch still needs to be applied before building
llvm, since `clang-tidy` as distributed by LLVM doesn't support plugins.

First, clone the llvm repo from for example [the official github repo](https://github.com/llvm/llvm-project.git).
Checkout the `release/8.x` branch, since that's where our patch was based on.

On windows, instead of applying the patch mentioned in the previous section, you
shoud apply `plugin-support.patch` from [this PR](https://github.com/jbytheway/clang-tidy-plugin-support/pull/1)
instead, if it's not merged yet. This is because the `-rdynamic` option is not
supported on Windows, so clang-tidy needs to be built as a static library instead.
(If you cloned the repo from the official github repo, replace `tools/extra` with
`clang-tools-extra` in the patch before applying it.)

After the patch is applied, you can then build the llvm code. Unfortunately, it
seems that clang itself cannot correctly compile the llvm code on Windows (gives
some sort of relocation error). Luckily, MinGW-w64 can be used instead to compile
the code.

The first step to build the code is to run CMake to generate the makefile. On
the root dir of llvm, run the following script (substitute values inside `<>`
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
it youself using llvm sources by adding the `FileCheck` target to the make command.

```sh
mkdir -p build
cd build
mingw32-make -j4 clang-tidy clangTidyMain FileCheck
```

Here `clang-tidy` is only added to trigger the building of several targets that
are needed to build our custom clang-tidy executable later.

##### Build clang-tidy with custom checks

After building clang-tidy as a library from the llvm source, the next step is to
build clang-tidy as an executable, with the custom checks from the CDDA source.

In this step, the following tools are required.
- Python 2 (used to run the lit test for the custom checks)
- Python 3 (used to run other python scripts)
- CMake
- MinGW-w64
- FileCheck (built from the llvm source)
- A shell environment

You also need to install yaml for python 3 to work. Download the `.whl` installer
corresponding to your python version from [here](https://pyyaml.org/wiki/PyYAML)
and execute the following command inside the `<python3_root>/Scripts` directory
```sh
pip install path/to/your/downloaded/file.whl
```

Currently, the CDDA source is still building the custom checks as a plugin,
which unfortunately is not supported on Windows, so the following patch needs to
be applied before the custom checks can be built as an executable.

```patch
diff --git a/tools/clang-tidy-plugin/CMakeLists.txt b/tools/clang-tidy-plugin/CMakeLists.txt
index 553ef0ebe0..f591bc80d1 100644
--- a/tools/clang-tidy-plugin/CMakeLists.txt
+++ b/tools/clang-tidy-plugin/CMakeLists.txt
@@ -3,8 +3,8 @@ include(ExternalProject)
 find_package(LLVM REQUIRED CONFIG)
 find_package(Clang REQUIRED CONFIG)
 
-add_library(
-    CataAnalyzerPlugin MODULE
+add_executable(
+    CataAnalyzerPlugin
     CataTidyModule.cpp
     JsonTranslationInputCheck.cpp
     NoLongCheck.cpp
@@ -51,6 +51,11 @@ else()
         CataAnalyzerPlugin SYSTEM PRIVATE ${CATA_CLANG_TIDY_INCLUDE_DIR})
 endif()
 
+target_link_libraries(
+    CataAnalyzerPlugin
+    clangTidyMain
+    )
+
 target_compile_definitions(
     CataAnalyzerPlugin PRIVATE ${LLVM_DEFINITIONS})
 
diff --git a/tools/clang-tidy-plugin/test/lit.cfg b/tools/clang-tidy-plugin/test/lit.cfg
index 4ab6e913a7..d1a4418ba6 100644
--- a/tools/clang-tidy-plugin/test/lit.cfg
+++ b/tools/clang-tidy-plugin/test/lit.cfg
@@ -17,11 +17,13 @@ else:
             config.plugin_build_root, 'clang-tidy-plugin-support', 'bin',
             'check_clang_tidy.py')
 
-cata_include = os.path.join( config.cata_source_dir, "src" )
+cata_include = os.path.join( config.cata_source_dir, "./src" )
 
 cata_plugin = os.path.join(
         config.plugin_build_root, 'libCataAnalyzerPlugin.so')
 
+cata_plugin = ''
+
 config.substitutions.append(('%check_clang_tidy', check_clang_tidy))
 config.substitutions.append(('%cata_include', cata_include))
 config.substitutions.append(('%cata_plugin', cata_plugin))
```

The next step is to run CMake to generate the compilation database. The compilation
database contains compiler flags that clang-tidy uses to check the source files.

Make sure Python 2, Python 3, CMake, MinGW-w64, and FileCheck are on the path.
Note that two `bin` directories of MinGW-w64 should be on the path: `<mingw-w64-root>/bin`,
and `<mingw-w64-root>/x86_64-w64-mingw32/bin`. FileCheck's path is `<llvm-source-root>/build/bin`,
if you built it with the instructions in the previous section. Python 2 should
precede Python 3 in the path, otherwise scripts that are intended to run with
Python 2 might not work.

Then add the following CMake options to generate the compilation database
(substitute values inside `<>` with the actual paths), and build the CDDA source
and the custom clang-tidy executable with `mingw32-make`. In this tutorial we
run CMake and `mingw32-make` in the `build` subdirectory.

```sh
-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
-DLLVM_DIR="<llvm-source-root>/build/lib/cmake/llvm"
-DClang_DIR="<llvm-source-root>/build/lib/cmake/clang"
-DLLVM_INCLUDE_DIRS="<llvm-source-root>/llvm/include"
-DCLANG_INCLUDE_DIRS="<llvm-source-root>/clang/include"
-DCATA_CLANG_TIDY_PLUGIN=ON
-DCATA_CLANG_TIDY_INCLUDE_DIR="<llvm-source-root>/clang-tools-extra/clang-tidy"
-DCATA_CHECK_CLANG_TIDY="<llvm-source-root>/clang-tools-extra/test/clang-tidy/check_clang_tidy.py -clang-tidy=<cdda-source-root>/build/tools/clang-tidy-plugin/CataAnalyzerPlugin.exe"
```

Next, change the directory back to the source root, and run `tools/fix-compilation-database.py`
with Python 3 to fix some errors in the compilation database. Then the compilation
database should be usable by clang-tidy.

If you want to check if the custom checks are working correctly, run the following
script. Note that `python` here is the executable from Python 2.

```sh
python <llvm-source-root>/llvm/utils/lit/lit.py -v build/tools/clang-tidy-plugin/test
```

Finally, use the following command to run clang-tidy with the custom checks.
In the following command, the first line of "-extra-arg"s are used to tell
clang-tidy to mimic g++ behavior, and the second line of "-extra-arg"s are
used to tell clang-tidy to use clang's x86intrin.h instead of g++'s, so as
to avoid compiler errors.

```sh
python3 <llvm-source-root>/clang-tools-extra/clang-tidy/tool/run-clang-tidy.py \
    -clang-tidy-binary=build/tools/clang-tidy-plugin/CataAnalyzerPlugin.exe \
    -p=build "\.cpp$" \
    -extra-arg=-target -extra-arg=x86_64-pc-windows-gnu -extra-arg=-pthread -extra-arg=-DSDL_DISABLE_ANALYZE_MACROS \
    -extra-arg=-isystem -extra-arg=<llvm-source-root>/clang/lib/Headers
```

You can also add `-fix-errors` to apply fixes reported by the checks, or
`-checks="-*,xxx,yyy"` to specify the checks you would like to run.

## include-what-you-use

[include-what-you-use](https://github.com/include-what-you-use/include-what-you-use)
(IWYU) is a project intended to optimise includes.  It will calculate the
required headers and add and remove includes as appropriate.

Running on this codebase revealed some issues.  You will need a version of IWYU
where the following PR has been merged (which has not yet happened at time of
writing):

* https://github.com/include-what-you-use/include-what-you-use/pull/681

Once you have IWYU built, build the codebase using cmake, with
`CMAKE_EXPORT_COMPILE_COMMANDS=ON` on to create a compilation database
(Look for `compile_commands.json` in the build dir to see whether that worked).

Then run:

```
iwyu_tool.py -p $CMAKE_BUILD_DIR/compile_commands.json -- -Xiwyu --mapping_file=$PWD/tools/iwyu/cata.imp | fix_includes.py --nosafe_headers
```

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
