cmake_minimum_required(VERSION 3.1.4)

project(CataclysmDDA)

set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        ${CMAKE_SOURCE_DIR}/CMakeModules)

set(CMAKE_TLS_VERIFY ON)

# Build options
option(TILES "Build graphical tileset version." "OFF")
option(CURSES "Build curses version." "ON")
option(SOUND "Support for in-game sounds & music." "OFF")
option(BACKTRACE "Support for printing stack backtraces on crash" "ON")
option(LIBBACKTRACE "Print backtrace with libbacktrace." "OFF")
option(USE_HOME_DIR "Use user's home directory for save files." "ON")
option(LOCALIZE "Support for language localizations. Also enable UTF support." "ON")
option(LANGUAGES "Compile localization files for specified languages." "")
option(DYNAMIC_LINKING
        "Use dynamic linking. Or use static to remove MinGW dependency instead." "ON")
option(JSON_FORMAT "Build JSON formatter" "OFF")
option(CATA_CCACHE "Try to find and build with ccache" "ON")
option(CATA_CLANG_TIDY_PLUGIN "Build Cata's custom clang-tidy plugin" "OFF")
set(CATA_CLANG_TIDY_INCLUDE_DIR "" CACHE STRING
        "Path to internal clang-tidy headers required for plugin (e.g. ClangTidy.h)")
set(CATA_CHECK_CLANG_TIDY "" CACHE STRING "Path to check_clang_tidy.py for plugin tests")
set(GIT_BINARY "" CACHE STRING "Git binary name or path.")

include(CTest)

message(STATUS "${PROJECT} build environment --")
message(STATUS "Build realm is: ${CMAKE_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_PROCESSOR}")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif ()

add_definitions(-DCMAKE)

include(GetGitRevisionDescription)
git_describe(GIT_VERSION --tags --always --match "[0-9A-Z]*.[0-9A-Z]*")
if (NOT ${GIT_VERSION} MATCHES GIT-NOTFOUND)
    string(REPLACE "-NOTFOUND" "" GIT_VERSION ${GIT_VERSION})
    file(WRITE ${CMAKE_SOURCE_DIR}/src/version.h
            "// NOLINT(cata-header-guard)\n\#define VERSION \"${GIT_VERSION}\"\n")
    message(STATUS "${PROJECT_NAME} build version is: ${GIT_VERSION}")
    add_definitions(-DGIT_VERSION)
endif ()

#OS Check Placeholders. Will be used for BINDIST
if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
    set(_OS_LINUX_ 1)
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES FreeBSD)
    set(_OS_FREEBSD_ 1)
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    set(_OS_DARWIN_ 1)
    add_definitions(-DMACOSX)
    if (${TILES})
        add_definitions(-DOSX_SDL2_LIBS)
    endif ()
endif ()

include(CheckCXXCompilerFlag)

#FIXME: Add dest build choice: m32 for 32 bit or m64 for 64 bit version
#add_definitions("-m32")
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
#SET(CMAKE_SHARED_LIBRARY_C_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS} -m32")
#SET(CMAKE_SHARED_LIBRARY_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CXX_FLAGS} -m32")

if (NOT DYNAMIC_LINKING)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a;.dll.a")
    set(BUILD_SHARED_LIBRARIES OFF)
    check_cxx_compiler_flag (-static HAVE_STATIC_FLAG)
    if (HAVE_STATIC_FLAG)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif ()
    # Workaround for cmake link library guesser
    set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic
    set(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
    set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
    set(CMAKE_SHARED_LIBRARY_CXX_FLAGS)
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
else ()
    if (MINGW AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Avoid depending on MinGW runtime DLLs
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif ()
endif ()

# System specific actions
if (${CMAKE_SYSTEM_NAME} MATCHES Linux OR ${CMAKE_SYSTEM_NAME} MATCHES FreeBSD
    OR ${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    if (NOT DATA_PREFIX)
        set(DATA_PREFIX ${CMAKE_INSTALL_PREFIX}/share/cataclysm-dda)
    endif ()
    if (NOT LOCALE_DIR)
        set(LOCALE_DIR ${CMAKE_INSTALL_PREFIX}/share/locale)
    endif ()
    if (NOT BIN_PREFIX)
        set(BIN_PREFIX ${CMAKE_INSTALL_PREFIX}/bin)
    endif ()
    if (NOT DESKTOP_ENTRY_PATH)
        set(DESKTOP_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/applications)
    endif ()
    if (NOT PIXMAPS_ENTRY_PATH)
        set(PIXMAPS_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor)
    endif ()
    if (NOT PIXMAPS_UNITY_ENTRY_PATH)
        set(PIXMAPS_UNITY_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/icons/ubuntu-mono-dark)
    endif ()
    if (NOT MANPAGE_ENTRY_PATH)
        set(MANPAGE_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/man)
    endif ()
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES Windows)
    if (NOT DATA_PREFIX)
        set(DATA_PREFIX ${CMAKE_INSTALL_PREFIX})
    endif ()
    if (NOT LOCALE_DIR)
        set(LOCALE_DIR ${CMAKE_INSTALL_PREFIX})
    endif ()
    if (NOT BIN_PREFIX)
        set(BIN_PREFIX ${CMAKE_INSTALL_PREFIX})
    endif ()
endif ()

message(STATUS "${PROJECT_NAME} build options --")

# Preset variables
if (NOT LANGUAGES)
    set(LANGUAGES de es_AR es_ES fr it_IT ja ko pt_BR ru zh_CN zh_TW)
endif ()

if (GIT_BINARY)
    set(GIT_EXECUTABLE ${GIT_BINARY})
else ()
    find_package(Git)
    if (NOT GIT_FOUND)
        message(WARNING
            "Git binary not found. Build version will be set to NULL. \
             Install Git package or use -DGIT_BINARY to set path to git binary.")
    endif ()
endif ()

# Can't compile curses and tiles build's at same time
if (TILES)
    set(CURSES OFF)
endif ()

# Set build types and display info
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(RELEASE 0)
    message(STATUS "Build ${PROJECT} in development mode --")
    message(STATUS "Binaries will be located in: " ${CMAKE_SOURCE_DIR})
    set(CMAKE_VERBOSE_MAKEFILE ON)
    # Since CataclysmDDA does not respect PREFIX for development builds
    # and has funny path handlers, we should create resulting Binaries
    # in the source directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR} CACHE PATH
            "Single Directory for all Executables.")
    set(BIN_PREFIX ${CMAKE_SOURCE_DIR})
else ()
    set(RELEASE 1)
    message(STATUS "CMAKE_INSTALL_PREFIX          : ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "BIN_PREFIX                    : ${BIN_PREFIX}")
    message(STATUS "DATA_PREFIX                   : ${DATA_PREFIX}")
    if (LOCALIZE)
        message(STATUS "LOCALE_PATH                   : ${LOCALE_DIR}")
    endif ()
    message(STATUS "DESKTOP_ENTRY_PATH            : ${DESKTOP_ENTRY_PATH}")
    message(STATUS "PIXMAPS_ENTRY_PATH            : ${PIXMAPS_ENTRY_PATH}")
    message(STATUS "PIXMAPS_UNITY_ENTRY_PATH      : ${PIXMAPS_UNITY_ENTRY_PATH}")
    message(STATUS "MANPAGE_ENTRY_PATH            : ${MANPAGE_ENTRY_PATH}")
    add_definitions(-DRELEASE)
    # Use CMAKE_INSTALL_PREFIX as storage of data,gfx, etc.. Useful only on *nix OS.
    if (CMAKE_INSTALL_PREFIX AND NOT WIN32)
        add_definitions(-DPREFIX=${CMAKE_INSTALL_PREFIX})
        add_definitions(-DDATA_DIR_PREFIX)
    endif ()
endif ()

message(STATUS "GIT_BINARY                    : ${GIT_EXECUTABLE}")
message(STATUS "DYNAMIC_LINKING               : ${DYNAMIC_LINKING}")
message(STATUS "TILES                         : ${TILES}")
message(STATUS "CURSES                        : ${CURSES}")
message(STATUS "SOUND                         : ${SOUND}")
message(STATUS "BACKTRACE                     : ${BACKTRACE}")
message(STATUS "LOCALIZE                      : ${LOCALIZE}")
message(STATUS "USE_HOME_DIR                  : ${USE_HOME_DIR}")
message(STATUS "LANGUAGES                     : ${LANGUAGES}")
message(STATUS "See doc/COMPILING/COMPILING-CMAKE.md for details and more info --")

if (MSVC)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-D_AMD64_)
    else ()
        add_definitions(-D_X86_)
    endif ()
else (MSVC)
    set(CATA_WARNINGS
            "-Werror -Wall -Wextra \
             -Wformat-signedness \
             -Wlogical-op \
             -Wmissing-declarations \
             -Wmissing-noreturn \
             -Wnon-virtual-dtor \
             -Wold-style-cast \
             -Woverloaded-virtual \
             -Wpedantic \
             -Wsuggest-override \
             -Wunused-macros \
             -Wzero-as-null-pointer-constant \
             -Wno-unknown-warning-option")
    if (NOT ${CMAKE_SYSTEM_NAME} MATCHES Windows)
        set(CATA_WARNINGS "${CATA_WARNINGS} -Wredundant-decls")
    endif ()
    set(CATA_OTHER_FLAGS "${CATA_OTHER_FLAGS} -fsigned-char")
    # Compact the whitespace in the warning string
    string(REGEX REPLACE "[\t ]+" " " CATA_WARNINGS "${CATA_WARNINGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CATA_WARNINGS} ${CATA_OTHER_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "-Og -g")
endif ()

set(CMAKE_CXX_STANDARD 14)

# Force out-of-source build
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR
            "This project requires an out of source build. \
             Remove the file 'CMakeCache.txt' found in this directory before continuing; \
             create a separate build directory and run 'cmake [options] <srcs>' from there. \
             See doc/COMPILING/COMPILING-CMAKE.md for details and more info.")
endif ()

#set(THREADS_USE_PTHREADS_WIN32 True)
set(CMAKE_THREAD_PREFER_PTHREAD True)
find_package(Threads REQUIRED)

# Check for build types and libraries
if (TILES)
    # Find SDL, SDL_ttf & SDL_image for graphical install
    message(STATUS "Searching for SDL2 library --")
    find_package(SDL2)
    if (NOT SDL2_FOUND)
        message(FATAL_ERROR
                "This project requires SDL2 to be installed to compile in graphical mode.  \
                 Please install the SDL2 development libraries, \
                 or try compiling without -DTILES=1 for a text-only compilation. \
                 See doc/COMPILING/COMPILING-CMAKE.md for details and more info.")
    endif ()

    if (NOT DYNAMIC_LINKING)
        # SDL, SDL_Image, SDL_ttf deps are required for static build
        message(STATUS "Searching for SDL deps libraries --")
        find_package(Freetype REQUIRED)
        find_package(PNG REQUIRED)
        find_package(JPEG REQUIRED)
        find_package(ZLIB REQUIRED)
        find_package(BZip2 REQUIRED)
    endif ()

    message(STATUS "Searching for SDL2_TTF library --")
    find_package(SDL2_ttf)
    if (NOT SDL2_TTF_FOUND)
        message(FATAL_ERROR
                "This project requires SDL2_ttf to be installed to compile in graphical mode. \
                 Please install the SDL2_ttf development libraries, \
                 or try compiling without -DTILES=1 for a text-only compilation. \
                 See doc/COMPILING/COMPILING-CMAKE.md for details and more info.")
    endif ()

    message(STATUS "Searching for SDL2_image library --")
    find_package(SDL2_image)
    if (NOT SDL2_IMAGE_FOUND)
        message(FATAL_ERROR
                "This project requires SDL2_image to be installed to compile in graphical mode. \
                 Please install the SDL2_image development libraries, \
                 or try compiling without -DTILES=1 for a text-only compilation. \
                 See doc/COMPILING/COMPILING-CMAKE.md for details and more info.")
    endif ()
    add_definitions(-DTILES)
endif ()

if (CURSES)
    # Find the ncurses library for a text based compile
    message(STATUS "Searching for Curses library --")
    set(CURSES_NEED_NCURSES TRUE)
    set(CURSES_NEED_WIDE TRUE)
    find_package(Curses)
    if (NOT CURSES_FOUND)
        message(FATAL_ERROR
                "This project requires ncurses to be installed to be compiled in text-only mode. \
                 Please install the ncurses development libraries, \
                 or try compiling with -DTILES=1 for a graphical compilation. \
                 See doc/COMPILING/COMPILING-CMAKE.md for details and more info")
    endif ()
endif ()

if (SOUND)
    # You need TILES to be able to use SOUND
    if (NOT TILES)
        message(FATAL_ERROR
                "You must enable graphical support with -DTILES=1 \
                 to be able to enable sound support. \
                 See doc/COMPILING/COMPILING-CMAKE.md for details and more info.")
    endif ()

    # Sound requires SDL_mixer library
    message(STATUS "Searching for SDL2_mixer library --")
    find_package(SDL2_mixer)
    if (NOT SDL2_MIXER_FOUND)
        message(FATAL_ERROR
                "You need the SDL2_mixer development library \
                 to be able to compile with sound enabled. \
                 See doc/COMPILING/COMPILING-CMAKE.md for details and more info.")
    endif ()
endif ()

if (BACKTRACE)
    add_definitions(-DBACKTRACE)
    if (LIBBACKTRACE)
        add_definitions(-DLIBBACKTRACE)
    endif (LIBBACKTRACE)
endif (BACKTRACE)

# Ok. Now create build and install recipes
if (LOCALIZE)
    add_subdirectory(lang)
    add_definitions(-DLOCALIZE)
endif ()

if (USE_HOME_DIR)
    add_definitions(-DUSE_HOME_DIR)
endif ()

add_subdirectory(src)
add_subdirectory(data)
if (NOT MSVC)
    add_subdirectory(src/chkjson)
endif()
add_subdirectory(tests)
if (JSON_FORMAT)
    add_subdirectory(tools/format)
endif()
if (CATA_CLANG_TIDY_PLUGIN)
    add_subdirectory(tools/clang-tidy-plugin)
endif()

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

add_custom_target(uninstall
        "${CMAKE_COMMAND}"
        -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND AND CATA_CCACHE)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif ()

