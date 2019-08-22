## Astyle

Automatic formatting is performed by astyle.  If you have make and astyle
installed then this can be done with `make astyle`.

On Windows, there is an astyle extension for Visual Studio.

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
