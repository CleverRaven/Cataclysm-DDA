# WARNING: Only linux is currently supported by this cmake build file.

# This file is used to build cataclysm with CMake.
#
# To use it, install cmake, then:
#
# cd cataclysm
# mkdir build
# cd build
# cmake -G "Unix Makefiles" -DLOCALIZE=ON -DSDL=OFF ..
# make
#
# This will build and install cataclysm into the build directory.
#
# "Unix Makefiles" is a generator, you can get a list of supported generators
# on your system by running "cmake" without any arguments.
#
# -DOPTION_NAME=ON/OFF enables/disables OPTION_NAME

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)

PROJECT(CataclysmDDA)

OPTION(LOCALIZE "Toggle gettext localization/translation." ON)
OPTION(SDL "Use SDL curses emulation instead of terminal support. Also enables tiles support." OFF)
OPTION(SOUND "Use SDL mixer to play music and sounds." ON)
OPTION(LUA "Enable lua scripting support." ON)

# Note: The CMake documentation says it's better to list the actual source
#       files here, as otherwise cmake won't know to rerun when a new file
#       is added. However, currently the Makefile also just scans for source
#       files, so this approach should be "good enough".
FILE(GLOB CataclysmDDA_SRCS src/*.cpp)
FILE(GLOB CataclysmDDA_HDRS src/*.h)

ADD_EXECUTABLE(cataclysm ${CataclysmDDA_SRCS} ${CataclysmDDA_HDRS})

# Custom command that will be executed whenever the "cataclysm" target is built.
# Copy all the relevant game data into the build directory.
ADD_CUSTOM_COMMAND(
    TARGET cataclysm PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:cataclysm>/data
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/LICENSE.txt $<TARGET_FILE_DIR:cataclysm> # Don't forget the license :)
)

IF(MINGW)
    add_definitions("-D_WINDOWS -D_MINGW -D_WIN32 -DWIN32 -D__MINGW__")
ENDIF()

IF(LOCALIZE)
    add_definitions(-DLOCALIZE)
    #TARGET_LINK_LIBRARIES(cataclysm
    #    intl
    #)
ENDIF()

# TODO: windows rc stuff

IF(MSVC OR MINGW)
    add_definitions(-Wl,-stack,12000000,-subsystem,windows)
ENDIF()

Include(FindPkgConfig)
IF(LUA)
    ADD_DEFINITIONS(-DLUA)
    PKG_SEARCH_MODULE(LUA51 REQUIRED lua5.1)

    if(LUA51_FOUND)
        ADD_DEFINITIONS(-I${LUA51_INCLUDE_DIRS})
        target_link_libraries(cataclysm ${LUA51_LIBRARIES})
    endif()
ENDIF()

IF(SDL)
    # Look for SDL2 using pkgconfig
    PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)

    # Harcode these for now since they don't seem to be included in pkg-config
    # CMake is bound to get SDL2 finders eventually.
    #PKG_SEARCH_MODULE(SDL2_TTF REQUIRED sdl2_ttf)
    #PKG_SEARCH_MODULE(SDL2_IMAGE REQUIRED sdl2_image)
    SET(SDL2_TTF_LIBRARIES -lSDL2_ttf)
    SET(SDL2_IMAGE_LIBRARIES -lSDL2_image)

    TARGET_LINK_LIBRARIES(cataclysm ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
    include_directories(${SDL2_INCLUDE_DIRS})

    # Install GFX directory
    ADD_CUSTOM_COMMAND(
        TARGET cataclysm PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/gfx $<TARGET_FILE_DIR:cataclysm>/gfx
    )

    ADD_DEFINITIONS(-DSDLTILES -DTILES)

    IF(SOUND)
        SET(SDL2_MIXER_LIBRARIES -lSDL2_mixer)
        TARGET_LINK_LIBRARIES(cataclysm ${SDL2_MIXER_LIBRARIES})
    ADD_DEFINITIONS(-DSDL_SOUND)
    ENDIF()
ELSEIF(MSVC OR MINGW)
    # On windows our default isn't curses, but rather GDI
    TARGET_LINK_LIBRARIES(cataclysm gdi32 intl iconv winmm)
ELSE()
    TARGET_LINK_LIBRARIES(cataclysm curses)
ENDIF()
