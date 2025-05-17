# ImGuiFileDialog

## Build Matrix

|  | Win | Linux | MacOs |
|  :---: |  :---: |  :---: |  :---: |
| Msvc | [![Win Msvc](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Win_Msvc.yml/badge.svg)](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Win_Msvc.yml)  |  |   |
| Gcc |  | [![Linux Gcc](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Linux_Gcc.yml/badge.svg)](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Linux_Gcc.yml) | [![Osx Gcc](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Osx_Gcc.yml/badge.svg)](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Osx_Gcc.yml) |
| Clang |  | [![Linux Clang](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Linux_Clang.yml/badge.svg)](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Linux_Clang.yml)  | [![Osx Clang](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Osx_Clang.yml/badge.svg)](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Osx_Clang.yml)  |
| Leak Sanitizer | | [![Leak Sanitizer](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Leak.yml/badge.svg)](https://github.com/aiekick/ImGuiFileDialog/actions/workflows/Leak.yml) | |

## Purpose

ImGuiFileDialog is a file selection dialog built for (and using only) [Dear ImGui](https://github.com/ocornut/imgui).

My primary goal was to have a custom pane with widgets according to file extension. This was not possible using other
solutions.

## Possible Dialog Customization

![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/DemoApp/doc/demo_dialog.png)

## ImGui Supported Version

> [!NOTE]
> ImGuiFileDialog follow the master and docking branch of ImGui.
> Currently : [![Wrapped Dear ImGui Version](https://img.shields.io/badge/Dear%20ImGui%20Version-1.90.5-blue.svg)](https://github.com/ocornut/imgui)

### Documentation :

> [!NOTE]
> for a complete explanation and howto about ImGuiFileDialog Api, 
> you can check the [**Documentation**](https://github.com/aiekick/ImGuiFileDialog/blob/master/Documentation.md)

## Structure

* The library is in [Master branch](https://github.com/aiekick/ImGuiFileDialog/tree/master)
* A demo app can be found in the [DemoApp branch](https://github.com/aiekick/ImGuiFileDialog/tree/DemoApp)

This library is designed to be dropped into your source code rather than compiled separately.

From your project directory:

```
mkdir lib    <or 3rdparty, or externals, etc.>
cd lib
git clone https://github.com/aiekick/ImGuiFileDialog.git
git checkout master
```

These commands create a `lib` directory where you can store any third-party dependencies used in your project, downloads
the ImGuiFileDialog git repository and checks out the Lib_Only branch where the actual library code is located.

Add `lib/ImGuiFileDialog/ImGuiFileDialog.cpp` to your build system and include
`lib/ImGuiFileDialog/ImGuiFileDialog.h` in your source code. ImGuiFileLib will compile with and be included directly in
your executable file.

If, for example, your project uses cmake, look for a line like `add_executable(my_project_name main.cpp)`
and change it to `add_executable(my_project_name lib/ImGuiFileDialog/ImGuiFileDialog.cpp main.cpp)`. This tells the
compiler where to find the source code declared in `ImGuiFileDialog.h` which you included in your own source code.

## Requirements:

You must also, of course, have added [Dear ImGui](https://github.com/ocornut/imgui) to your project for this to work at
all.

ImguiFileDialog is agnostic about the filesystem api you can use.
It provides a IFileSystem you can override for your needs.

By default you can use dirent or std::filesystem. you have also a demo of uisng boos filesystem api in the DemoApp branch

Android Requirements : Api 21 mini

## Features

- C Api (succesfully tested with CimGui)
- Separate system for call and display
	- Can have many function calls with different parameters for one display function, for example
- Can create a custom pane with any widgets via function binding
	- This pane can block the validation of the dialog
	- Can also display different things according to current filter and UserDatas
- Advanced file style for file/dir/link coloring / icons / font
    - predefined form or user custom form by lambda function (the lambda mode is not available for the C API)
- Multi-selection (ctrl/shift + click) :
	- 0 => Infinite
	- 1 => One file (default)
	- n => n files
- Compatible with MacOs, Linux, Windows, Emscripten, Android
- Supports modal or standard dialog types
- Select files or directories
- Filter groups and custom filter names
- can ignore filter Case for file searching
- Keyboard navigation (arrows, backspace, enter)
- Exploring by entering characters (case insensitive)
- Custom places (bookmarks, system devices, whatever you want)
- Directory manual entry (right click on any path element)
- Optional 'Confirm to Overwrite" dialog if file exists
- Thumbnails Display (agnostic way for compatibility with any backend, sucessfully tested with OpenGl and Vulkan)
- The dialog can be embedded in another user frame than the standard or modal dialog
- Can tune validation buttons (placements, widths, inversion)
- Can quick select a parrallel directory of a path, in the path composer (when you clikc on a / you have a popup)
- regex support for filters, collection of filters and filestyle (the regex is recognized when between (( and )) in a filter)
- multi layer extentions like : .a.b.c .json.cpp .vcxproj.filters etc..
- advanced behavior regarding asterisk based filter. like : .* .*.* .vcx.* .*.filters .vcs*.filt.* etc.. (internally regex is used)
- result modes GetFilePathName, GetFileName and GetSelection (overwrite file ext, keep file, add ext if no user ext exist)
- you can use your own FileSystem Api
    - by default Api Dirent and std::filesystem are defined
	- you can override GetDrieveList for specify by ex on android other fs, like local and SDCards

### WARNINGS :
- the nav system keyboard behavior is not working as expected, so maybe full of bug for ImGuiFileDialog
 
### Simple Usage :

```cpp
void drawGui() { 
  // open Dialog Simple
  if (ImGui::Button("Open File Dialog")) {
    IGFD::FileDialogConfig config;
	config.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".cpp,.h,.hpp", config);
  }
  // display
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
    if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
      std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
      std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
      // action
    }
    
    // close
    ImGuiFileDialog::Instance()->Close();
  }
}
```

![alt text](https://github.com/aiekick/ImGuiFileDialog/blob/DemoApp/doc/dlg_simple.gif)

### How to build the sample app

The sample app is [here in master branch](https://github.com/aiekick/ImGuiFileDialog/tree/master)

You need to use CMake. For the 3 Os (Win, Linux, MacOs), the CMake usage is exactly the same,

    Choose a build directory. (called here my_build_directory for instance) and
    Choose a Build Mode : "Release" / "MinSizeRel" / "RelWithDebInfo" / "Debug" (called here BuildMode for instance)
    Run CMake in console : (the first for generate cmake build files, the second for build the binary)

cmake -B my_build_directory -DCMAKE_BUILD_TYPE=BuildMode
cmake --build my_build_directory --config BuildMode

Some CMake version need Build mode define via the directive CMAKE_BUILD_TYPE or via --Config when we launch the build. This is why i put the boths possibilities

By the way you need before, to make sure, you have needed dependencies.

### On Windows :

You need to have the opengl library installed

### On Linux :

You need many lib : (X11, xrandr, xinerama, xcursor, mesa)

If you are on debian you can run :

sudo apt-get update 
sudo apt-get install libgl1-mesa-dev libx11-dev libxi-dev libxrandr-dev libxinerama-dev libxcursor-dev

### On MacOs :

you need many lib : opengl and cocoa framework

</blockquote></details>

## That's all folks :-)

You can check by example in this repo with the file CustomImGuiFileDialogConfig.h :
- this trick was used to have custom icon font instead of labels for buttons or messages titles
- you can also use your custom imgui button, the button call stamp must be same by the way :)

The Custom Icon Font (in CustomFont.cpp and CustomFont.h) was made with ImGuiFontStudio (https://github.com/aiekick/ImGuiFontStudio) i wrote for that :)
ImGuiFontStudio is also using ImGuiFileDialog.$

![Alt](https://repobeats.axiom.co/api/embed/22a6eef207d0bce7c03519d94f55100973b451ca.svg "Repobeats analytics image")
