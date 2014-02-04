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
OPTION(SDL "Use SDL curses emulation instead of terminal support." OFF)
OPTION(TILES "Enable tileset support." OFF)

# Note: The CMake documentation says it's better to list the actual source
#       files here, as otherwise cmake won't know to rerun when a new file
#       is added. However, currently the Makefile also just scans for source
#       files, so this approach should be "good enough".
FILE(GLOB CataclysmDDA_SRCS *.cpp)
FILE(GLOB CataclysmDDA_HDRS *.h)

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

IF(SDL OR TILES)
    # CMake ships with a few helpful include's to find common libraries
    # like SDL
    Include(FindSDL)
    Include(FindSDL_ttf)
    
    Find_Package(SDL REQUIRED)
    Find_Package(SDL_ttf REQUIRED)
    
    TARGET_LINK_LIBRARIES(cataclysm ${SDL_LIBRARY} ${SDLTTF_LIBRARY})
    
    IF(TILES)
        # Install GFX directory
        ADD_CUSTOM_COMMAND(
            TARGET cataclysm PRE_BUILD 
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/gfx $<TARGET_FILE_DIR:cataclysm>/gfx
        )
    
        Include(FindSDL_image)
        TARGET_LINK_LIBRARIES(cataclysm ${SDLIMAGE_LIBRARY})
        ADD_DEFINITIONS(-DSDLTILES -DTILES)
    ELSE()
        ADD_DEFINITIONS(-DTILES)
    ENDIF()
    
ELSEIF(MSVC OR MINGW)
    # On windows our default isn't curses, but rather GDI
    TARGET_LINK_LIBRARIES(cataclysm gdi32 intl iconv winmm)
    add_definitions(-Wl,-stack,12000000,-subsystem,windows)
ELSE()
    TARGET_LINK_LIBRARIES(cataclysm curses)
ENDIF()
