this is a bit poorly structured, but leaving for posterity.
I just really wanted to avoid WSL. And I also failed to make this work in msys. So Powershell+MSVS here we go.

# step 1: build clang. I chose clang-19 because that's the latest at the moment, but this is up to you.
YOU CANNOT USE PRE-BUILT CLANG downloadable from their github. It was compiled on different machine with different symbols. You'll be sad if you try.
You can't skip this step.

Generic docs are on the website: https://clang.llvm.org/get_started.html
Slightly modified for my own needs:

```ps1
git clone --depth=1 --branch=llvmorg-19.1.6 https://github.com/llvm/llvm-project.git  llvm-19
# cmake was getting stuck running git update-index for some reason. Running it manually fixed the issue, but it might be placebo
cd llvm-19 ; git update-index --refresh ; cd ..  
mkdir -p build-19-msvc
cd build-19-msvc
cmake '-DLLVM_ENABLE_PROJECTS=clang;lld' -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD=X86 ..\llvm-19\llvm\
cmake --build . --config=Release --target=lld --target=clang -j4
```
Two things of note: 
- `-DLLVM_TARGETS_TO_BUILD=X86` is neccessary to make the build take less than eternity
- `lld` is somehow a separate project and is somehow neccessary for the clang to work [without extra difficulties]

# step2: build iwuy.
Surprisingly straightforward actually. Official docs: https://github.com/include-what-you-use/include-what-you-use

```ps1
git clone https://github.com/include-what-you-use/include-what-you-use.git
cd include-what-you-use
git checkout clang_19
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=H:\PATH\TO\LLVM\build-19-msvc\ -DCMAKE_BUILD_TYPE=Release ..
```

# step 3 grab your compilation database
whichever method you like, there are plenty. Here's what I used: Add a `CMakeUserPresets.json` to root of cata repo with the following content:

```json
{
  "version": 4,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 20,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "my-clang",
      "generator": "Ninja",
      "environment": {
        "VCPKG_ROOT": "H:/soft/vcpkg",
        "NINJA": "H:/soft/ninja/ninja.exe",
        "CLANG_DIR": "H:/work/github/llvm/build-19-msvc/Release/bin"   
      },
      "cacheVariables": {
        "DCMAKE_EXPORT_COMPILE_COMMANDS": true,
        "CMAKE_MAKE_PROGRAM": "$env{NINJA}",
        "CMAKE_C_COMPILER": "$env{CLANG_DIR}/clang.exe",
        "CMAKE_CXX_COMPILER": "$env{CLANG_DIR}/clang++.exe",
        "CMAKE_RC_COMPILER": "$env{CLANG_DIR}/llvm-rc.exe",
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",

        "TILES": true,
        "SOUND": true,
        "BACKTRACE": true
      }
    }
  ]
}
```
(adjust the vars in the `environment` section as appropriate)
Then do 
```ps1
mkdir -p .\build\
cd build
cmake --preset=my-clang ..
cd ..
```

# step 4. Running iwyu
First we need to add iwyu to path (this is actually required)
```ps1
$env:PATH+=";PATH\TO\include-what-you-use\build\bin\Release"
```
then actually run it.

```ps1
mkdir -p out
H:\work\github\include-what-you-use\iwyu_tool.py --jobs=4 -p .\build\ src/ -- -Xiwyu --cxx17ns -Xiwyu --mapping_file=../tools/iwyu/cata.imp  > out/iwyu-out.txt
```

Important to note:
1) `include-what-you-use.exe` needs to be on the path, but `iwyu_tool.py` doesn't
2) the path to just-built compilation database is passed via `-p`
3) the path to mappting file is relative to the `-p`, not to the current dir
4) the `--cxx17ns` flags are chosen for no particular reason. Check out `include-what-you-use.exe --help` to see other options
5) `--jobs=4` is self-explanatory
6) this runs on all files in `src` *recursively* (so, it includes `src/third-party`). Consider using wildcards (`src/*.cpp`) or some script shenaningans to work around that.

If you look in the output files, you might notice that they have some newlines compare to what you'd see in the terminal. I have no idea how to fix it, but luckily it does not affect the IWYU tooling ability to apply the fixes.

# step 5 apply the fixes
The expected way to apply the fixes is to run
```ps1
python H:\work\github\include-what-you-use\fix_includes.py  .\src\achievement.cpp < out/iwyu-files-all.txt
```
however if you are in powershell, you'll see that this does not work, for two and a half reasons:
- the `command < file` is linux thing that pwsh doesn't do, you need to instead `Get-Content file | command`
- fix_includes.py expects absolute paths because that is what iwyu_tool output. This is seemingly different from linux (and less pleasant at that).
- the file names as you see in the `iwyu` output are not the same as `fix_includes` expects them to be. The former have *forward* slashes (`/`), the latter has *backwards* slashes (/`). So it just skips them. We need to do cursed string manipulation. So there goes

```ps1
Get-Content out/iwyu-iwyu-out.txt | python H:\work\github\include-what-you-use\fix_includes.py --reorder --dry_run (Get-Item src\*).FullName.Replace('\', '/')
```
(remove `--dry-run` when satisfied)
- or, perhaps much easier, select the files you want via `--only_re` flag. For example 
```ps1
Get-Content out/iwyu-iwyu-out.txt | python H:\work\github\include-what-you-use\fix_includes.py --reorder --dry_run --only_re '.*/src/ac.*.cpp'
```

# step 6
Hooray! Now just check that the changes make sense, update the mappings (in this directory) (if neccessary) (docs link: https://github.com/include-what-you-use/include-what-you-use/blob/master/docs/IWYUMappings.md), slap some pragmas, make sure it compliles and all that.

