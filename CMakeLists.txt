# TODO:
#
# Move binary after compilation
# Support for special builds (json checking, tests, other stuff)

cmake_minimum_required(VERSION 2.8)
project(CataclysmDDA)
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake
)

# Build options
option(LUA      "Lua support (required for some mods)"  OFF)
option(TILES    "Support for graphical tilesets"        OFF)
option(SOUND    "Support for in-game sounds & music"    OFF)

# Disable internationalization on Windows and OS X as it is not supported
# without external libraries
if (WIN32 OR APPLE)
    option(LOCALIZE "Support for language localizations" OFF)
else()
    option(LOCALIZE "Support for language localizations" ON)
endif()

# Build settings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra -Werror -Os -DRELEASE")

# Force out-of-source build
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR
    "This project requires an out of source build. Remove the file 'CMakeCache.txt' found in this directory before continuing, create a separate build directory and run 'cmake [options] <srcs>' from there."
  )
endif()

if(TILES)
    # Find SDL, SDL_ttf & SDL_image for graphical install
    find_package(SDL2)
    if(NOT SDL2_FOUND)
        message(FATAL_ERROR
            "This project requires SDL2 to be installed to be compiled in graphical mode. Please install the SDL2 development libraries, or try compiling without the -DTILES=1 for a text only compilation."
        )
    endif()

    find_package(SDL2_ttf)
    if(NOT SDL2_TTF_FOUND)
        message(FATAL_ERROR
            "This project requires SDL2_ttf to be installed to be compiled in graphical mode. Please install the SDL2_ttf development libraries, or try compiling without the -DTILES=1 for a text only compilation."
        )
    endif()

    find_package(SDL2_image)
    if(NOT SDL2_IMAGE_FOUND)
        message(FATAL_ERROR
            "This project requires SDL2_image to be installed to be compiled in graphical mode. Please install the SDL2_image development libraries, or try compiling without the -DTILES=1 for a text only compilation."
        )
    endif()

    add_definitions(-DTILES -DSDLTILES)
else()
    # Find the ncurses library for a text based compile
    set(CURSES_NEED_NCURSES TRUE)
    find_package(Curses)
    if(NOT CURSES_FOUND)
        message(FATAL_ERROR
            "This project requires ncurses to be installed to be compiled in text only mode. Please install the ncurses development libraries, or try compiling with the -DTILES=1 for a graphical compilation."
        )
    endif()
endif()

if(SOUND)
    # You need TILES to be able to use SOUND
    if(NOT TILES)
        message(FATAL_ERROR
            "You must enable graphical support with -DTILES=1 to be able to enable sound support."
        )
    endif()

    # Sound requires SDL_mixer library
    find_package(SDL2_mixer)
    if(NOT SDL2_MIXER_FOUND)
        message(FATAL_ERROR
            "You need the SDL2_mixer development library to be able to compile with sound enabled."
        )
    endif()

    add_definitions(-DSDL_SOUND)
endif()

if(LUA)
    find_package(Lua)
    if(NOT LUA_FOUND)
        message(FATAL_ERROR
            "You need the Lua development library to be able to compile with Lua support."
        )
    endif()

    add_definitions(-DLUA)
endif()

if(LOCALIZE)
    add_definitions(-DLOCALIZE)
endif()

add_subdirectory(src)