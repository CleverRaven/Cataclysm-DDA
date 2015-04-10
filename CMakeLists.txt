# TODO:
#
# Move binary after compilation
# Support for special builds (json checking, tests, other stuff)

cmake_minimum_required(VERSION 2.8)

PROJECT(CataclysmDDA)

SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake
)

# Build options
option(LUA          "Lua support (required for some mods)"      "OFF")
option(TILES        "Support for graphical tilesets"            "OFF")
option(SOUND        "Support for in-game sounds & music"        "OFF")
option(DEBUG        "Enable debug. Use it for development or bug reporting purposes." "OFF")
option(ZLEVELS      "Enable experimental z-levels"              "OFF")
option(USE_HOME_DIR "Use user's home directory for save files." "OFF")
option(LOCALIZE     "Support for language localizations"        "OFF")


MESSAGE("\n * Cataclysm: Dark Days Ahead is a roguelike set in a post-apocalyptic world.")
MESSAGE("   _________            __                   .__                                ")
MESSAGE("   \\_   ___ \\ _____   _/  |_ _____     ____  |  |   ___.__   ______  _____      ")
MESSAGE("   /    \\  \\/ \\__  \\  \\   __\\\\__  \\  _/ ___\\ |  |  <   |  | /  ___/ /     \\     ")
MESSAGE("   \\     \\____ / __ \\_ |  |   / __ \\_\\  \\___ |  |__ \\___  | \\___ \\ |  Y Y  \\    ")
MESSAGE("    \\______  /\(____  / |__|  \(____  / \\___  >|____/ / ____|/____  >|__|_|  /    ")
MESSAGE("           \\/      \\/             \\/      \\/        \\/          \\/       \\/     ")
MESSAGE("                               --= Dark Days Ahead =--") 
MESSAGE("\n * http://en.cataclysmdda.com/\n")

MESSAGE(STATUS "${PROJECT} build environment -- \n")

MESSAGE(STATUS "Build realm is   : ${CMAKE_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_PROCESSOR}\n")

IF (${CMAKE_SYSTEM_NAME} MATCHES Linux)
	SET(_OS_LINUX_ 1)
ENDIF (${CMAKE_SYSTEM_NAME} MATCHES Linux)

IF (${CMAKE_SYSTEM_NAME} MATCHES FreeBSD)
	SET(_OS_FREEBSD_ 1)
ENDIF (${CMAKE_SYSTEM_NAME} MATCHES FreeBSD)

IF( ${CMAKE_SYSTEM_NAME} MATCHES Darwin )
	SET(_OS_DARWIN_ 1)
	SET(LOCALIZE OFF)
	MESSAGE(STATUS "Disable internationalization on Darwin as it is not supported")
ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES Darwin )

IF( ${CMAKE_SYSTEM_NAME} MATCHES Windows )
	SET(_OS_WINDOWS_ 1)
	SET(LOCALIZE OFF)
	MESSAGE(STATUS "Disable internationalization on Windows as it is not supported")
ENDIF( ${CMAKE_SYSTEM_NAME} MATCHES Windows )

IF (${CMAKE_SYSTEM_NAME} MATCHES Linux OR ${CMAKE_SYSTEM_NAME} MATCHES FreeBSD)
    IF( NOT DATA_PREFIX )
        SET( DATA_PREFIX ${CMAKE_INSTALL_PREFIX}/share/cataclysm-dda/ )
    ENDIF( NOT DATA_PREFIX )
    IF( NOT LOCALE_DIR )
        SET( LOCALE_DIR ${CMAKE_INSTALL_PREFIX}/share/cataclysm-dda/locale )
    ENDIF( NOT LOCALE_DIR )
    IF( NOT BIN_PREFIX )
        SET( BIN_PREFIX ${CMAKE_INSTALL_PREFIX}/bin )
    ENDIF( NOT BIN_PREFIX )
    IF( NOT DESKTOP_ENTRY_PATH )
        SET( DESKTOP_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/applications )
    ENDIF( NOT DESKTOP_ENTRY_PATH )
    IF( NOT PIXMAPS_ENTRY_PATH )
        SET( PIXMAPS_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor )
    ENDIF( NOT PIXMAPS_ENTRY_PATH )
    IF( NOT PIXMAPS_UNITY_ENTRY_PATH )
        SET( PIXMAPS_UNITY_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/icons/ubuntu-mono-dark )
    ENDIF( NOT PIXMAPS_UNITY_ENTRY_PATH )
    IF( NOT MANPAGE_ENTRY_PATH )
        SET( MANPAGE_ENTRY_PATH ${CMAKE_INSTALL_PREFIX}/share/man )
    ENDIF( NOT MANPAGE_ENTRY_PATH )
ENDIF (${CMAKE_SYSTEM_NAME} MATCHES Linux OR ${CMAKE_SYSTEM_NAME} MATCHES FreeBSD)

MESSAGE(STATUS "${PROJECT} build options --\n")

IF ( LUA )
	SET (LUA_BINARY "lua")
ENDIF ( LUA )

IF( DEVELOP )
	MESSAGE("\n")
	MESSAGE(STATUS "Build ${PROJECT} in development mode (DEVELOP=ON) --\n")
	ADD_DEFINITIONS(-std=c++11 -g -Wall -Wextra)
ELSE ( DEVELOP )
	MESSAGE(STATUS "CMAKE_INSTALL_PREFIX          : ${CMAKE_INSTALL_PREFIX}")
	MESSAGE(STATUS "BIN_PREFIX                    : ${BIN_PREFIX}")
	MESSAGE(STATUS "DATA_PREFIX                   : ${DATA_PREFIX}")
	MESSAGE(STATUS "DESKTOP_ENTRY_PATH            : ${DESKTOP_ENTRY_PATH}")
	MESSAGE(STATUS "PIXMAPS_ENTRY_PATH            : ${PIXMAPS_ENTRY_PATH}")
	MESSAGE(STATUS "PIXMAPS_UNITY_ENTRY_PATH      : ${PIXMAPS_UNITY_ENTRY_PATH}")
	MESSAGE(STATUS "MANPAGE_ENTRY_PATH            : ${MANPAGE_ENTRY_PATH}\n")

	MESSAGE(STATUS "LUA                           : ${LUA}")
IF ( LUA )
	MESSAGE(STATUS "LUA_BINARY                    : ${LUA_BINARY}")
ENDIF ( LUA )
	MESSAGE(STATUS "TILES                         : ${TILES}")
	MESSAGE(STATUS "SOUND                         : ${SOUND}")
	MESSAGE(STATUS "DEBUG                         : ${DEBUG}")
	MESSAGE(STATUS "LOCALIZE                      : ${LOCALIZE}")
	MESSAGE(STATUS "ZLEVELS                       : ${ZLEVELS}")
	MESSAGE(STATUS "USE_HOME_DIR                  : ${USE_HOME_DIR}\n")
	ADD_DEFINITIONS(-std=c++11 -Wall -Wextra -Werror -Os -DRELEASE)
ENDIF ( DEVELOP )

MESSAGE(STATUS "See INSTALL file for details and more info --\n")


# Force out-of-source build
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	MESSAGE(FATAL_ERROR
		"This project requires an out of source build. Remove the file 'CMakeCache.txt' found in this directory before continuing, create a separate build directory and run 'cmake [options] <srcs>' from there.\nSee INSTALL file for details and more info\n"
	)
ENDIF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

IF(TILES)
	# Find SDL, SDL_ttf & SDL_image for graphical install
	MESSAGE(STATUS "Searching for SDL2 library --\n")
	FIND_PACKAGE(SDL2)
	IF(NOT SDL2_FOUND)
        	MESSAGE(FATAL_ERROR
            		"This project requires SDL2 to be installed to be compiled in graphical mode. Please install the SDL2 development libraries, or try compiling without the -DTILES=1 for a text only compilation.\nSee INSTALL file for details and more info\n"
		)
	ENDIF(NOT SDL2_FOUND)

	MESSAGE(STATUS "Searching for SDL2_TTF library --\n")
	FIND_PACKAGE(SDL2_ttf)
		IF(NOT SDL2_TTF_FOUND)
		MESSAGE(FATAL_ERROR
			"This project requires SDL2_ttf to be installed to be compiled in graphical mode. Please install the SDL2_ttf development libraries, or try compiling without the -DTILES=1 for a text only compilation.\nSee INSTALL file for details and moreinfo\n"
		)
	ENDIF(NOT SDL2_TTF_FOUND)

	MESSAGE(STATUS "Searching for SDL2_image library --\n")
	FIND_PACKAGE(SDL2_image)
	IF(NOT SDL2_IMAGE_FOUND)
        	MESSAGE(FATAL_ERROR
			"This project requires SDL2_image to be installed to be compiled in graphical mode. Please install the SDL2_image development libraries, or try compiling without the -DTILES=1 for a text only compilation.\nSee INSTALL file for details and more info\n"
		)
	ENDIF(NOT SDL2_IMAGE_FOUND)

	ADD_DEFINITIONS(-DTILES -DSDLTILES)
ELSE(TILES)
	# Find the ncurses library for a text based compile
	MESSAGE(STATUS "Searching for Curses library --\n")
	FIND_PACKAGE(Curses)
	IF(NOT CURSES_FOUND)
		MESSAGE(FATAL_ERROR
			"This project requires ncurses to be installed to be compiled in text only mode. Please install the ncurses development libraries, or try compiling with the -DTILES=1 for a graphical compilation.\nSee INSTALL file for details and more info\n"
		)
	ENDIF(NOT CURSES_FOUND)
	SET(CURSES_NEED_NCURSES TRUE)
ENDIF(TILES)

IF(SOUND)
	# You need TILES to be able to use SOUND
	IF(NOT TILES)
		MESSAGE(FATAL_ERROR
			"You must enable graphical support with -DTILES=1 to be able to enable sound support.\nSee INSTALL file for details and more info\n"
		)
	ENDIF(NOT TILES)

	# Sound requires SDL_mixer library
	MESSAGE(STATUS "Searching for SDL2_mixer library --\n")
	FIND_PACKAGE(SDL2_mixer)
	IF(NOT SDL2_MIXER_FOUND)
		MESSAGE(FATAL_ERROR
			"You need the SDL2_mixer development library to be able to compile with sound enabled.\nSee INSTALL file for details and more info\n"
		)
	ENDIF(NOT SDL2_MIXER_FOUND)

	ADD_DEFINITIONS(-DSDL_SOUND)
ENDIF(SOUND)

IF(LOCALIZE)
	ADD_DEFINITIONS(-DLOCALIZE)
ENDIF(LOCALIZE)

IF(ZLEVELS)
	ADD_DEFINITIONS(-DZLEVELS)
ENDIF(ZLEVELS)

IF(USE_HOME_DIR)
	ADD_DEFINITIONS(-DUSE_HOME_DIR)
ENDIF(USE_HOME_DIR)

add_subdirectory(src)
add_subdirectory(data)
IF(LUA)
	add_subdirectory(lua)
	add_subdirectory(src/lua)
ENDIF(LUA)
add_subdirectory(src/chkjson)

