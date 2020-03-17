# Cataclysm DDA client
cmake_minimum_required(VERSION 3.1.4)

SET(MAIN_CPP ${CMAKE_SOURCE_DIR}/src/main.cpp)
SET(RESOURCE_RC ${CMAKE_SOURCE_DIR}/src/resource.rc)

FILE(GLOB CATACLYSM_DDA_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

LIST(REMOVE_ITEM CATACLYSM_DDA_SOURCES ${MAIN_CPP})

FILE(GLOB CATACLYSM_DDA_HEADERS
    ${CMAKE_SOURCE_DIR}/src/*.h
)

# Get GIT version strings
ADD_CUSTOM_TARGET(
    get_version
    DEPENDS ${CMAKE_SOURCE_DIR}/src/version.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

ADD_CUSTOM_COMMAND (
    OUTPUT ${CMAKE_SOURCE_DIR}/src/version.h
    COMMAND ${CMAKE_COMMAND}
      -D SRC=${CMAKE_SOURCE_DIR}/src/version.h.in
      -D DST=${CMAKE_SOURCE_DIR}/src/version.h
      -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
      -P ${CMAKE_SOURCE_DIR}/src/version.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Build tiles version if requested
IF(TILES)
    add_library(libcataclysm-tiles STATIC
      ${CATACLYSM_DDA_SOURCES}
      ${CATACLYSM_DDA_HEADERS}
    )
    target_include_directories(libcataclysm-tiles INTERFACE ${CMAKE_SOURCE_DIR}/src)

    IF(WIN32)
      ADD_DEFINITIONS(-DUSE_WINMAIN)
      ADD_EXECUTABLE(cataclysm-tiles WIN32
        ${MAIN_CPP}
        ${RESOURCE_RC}
      )
    ELSE(WIN32)
      ADD_EXECUTABLE(cataclysm-tiles
        ${MAIN_CPP}
      )
    ENDIF(WIN32)

    ADD_DEPENDENCIES(libcataclysm-tiles get_version)

    target_link_libraries(cataclysm-tiles libcataclysm-tiles)
    target_compile_definitions(libcataclysm-tiles PUBLIC TILES )

    IF (LOCALIZE)
      target_include_directories(libcataclysm-tiles PUBLIC
        ${LIBINTL_INCLUDE_DIR}
        ${ICONV_INCLUDE_DIR}
      )
      target_link_libraries(libcataclysm-tiles
        ${LIBINTL_LIBRARIES}
        ${ICONV_LIBRARIES}
      )
    ENDIF (LOCALIZE)

    IF(CMAKE_USE_PTHREADS_INIT)
      target_compile_options(libcataclysm-tiles PUBLIC "-pthread")
    ENDIF(CMAKE_USE_PTHREADS_INIT)

    IF(CMAKE_THREAD_LIBS_INIT)
      target_link_libraries(libcataclysm-tiles ${CMAKE_THREAD_LIBS_INIT})
    ENDIF(CMAKE_THREAD_LIBS_INIT)

    IF (NOT DYNAMIC_LINKING)
      # SDL, SDL_Image, SDL_ttf deps are required for static build
      target_include_directories(libcataclysm-tiles PUBLIC
        ${FREETYPE_INCLUDE_DIRS}
        ${PNG_INCLUDE_DIRS}
        ${JPEG_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIRS}
        ${BZIP2_INCLUDE_DIR}
      )
      target_link_libraries(libcataclysm-tiles
        ${FREETYPE_LIBRARIES}
        ${PNG_LIBRARIES}
        ${JPEG_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${BZIP2_LIBRARIES}
      )
    ENDIF (NOT DYNAMIC_LINKING)
    target_include_directories(libcataclysm-tiles PUBLIC
      ${SDL2_INCLUDE_DIR}
      ${SDL2_IMAGE_INCLUDE_DIRS}
      ${SDL2_TTF_INCLUDE_DIRS}
    )
    target_link_libraries(libcataclysm-tiles
      ${SDL2_LIBRARY}
      ${SDL2_IMAGE_LIBRARIES}
      ${SDL2_TTF_LIBRARIES}
    )

    IF(SOUND)
      target_compile_definitions(libcataclysm-tiles PUBLIC SDL_SOUND )
      target_include_directories(libcataclysm-tiles PUBLIC ${OGGVORBIS_INCLUDE_DIR})
      target_link_libraries(libcataclysm-tiles ${OGG_LIBRARY})
      target_link_libraries(libcataclysm-tiles ${VORBIS_LIBRARY})
      target_link_libraries(libcataclysm-tiles ${VORBISFILE_LIBRARY})
      target_include_directories(libcataclysm-tiles PUBLIC ${SDL2_MIXER_INCLUDE_DIRS})
      target_link_libraries(libcataclysm-tiles ${SDL2_MIXER_LIBRARIES})
    ENDIF(SOUND)

    IF(WIN32)
      # Global settings for Windows targets (at end)
      target_link_libraries(libcataclysm-tiles gdi32.lib)
      target_link_libraries(libcataclysm-tiles winmm.lib)
      target_link_libraries(libcataclysm-tiles imm32.lib)
      target_link_libraries(libcataclysm-tiles ole32.lib)
      target_link_libraries(libcataclysm-tiles oleaut32.lib)
      target_link_libraries(libcataclysm-tiles version.lib)
      IF (BACKTRACE)
        target_link_libraries(libcataclysm-tiles dbghelp.lib)
        IF(LIBBACKTRACE)
          target_link_libraries(libcataclysm-tiles backtrace)
        ENDIF(LIBBACKTRACE)
      ENDIF(BACKTRACE)
    ENDIF(WIN32)

    IF(RELEASE)
      install(TARGETS cataclysm-tiles DESTINATION ${BIN_PREFIX})
    ENDIF(RELEASE)
ENDIF(TILES)

# Build curses version if requested
IF(CURSES)
    add_library(libcataclysm STATIC
      ${CATACLYSM_DDA_SOURCES}
      ${CATACLYSM_DDA_HEADERS}
    )
    target_include_directories(libcataclysm INTERFACE ${CMAKE_SOURCE_DIR}/src)

    IF(WIN32)
      ADD_EXECUTABLE(cataclysm
        ${MAIN_CPP}
        ${RESOURCE_RC}
      )
    ELSE(WIN32)
      ADD_EXECUTABLE(cataclysm
        ${MAIN_CPP}
      )
    ENDIF(WIN32)

    ADD_DEPENDENCIES(libcataclysm get_version)
    target_link_libraries(cataclysm libcataclysm)

    IF (LOCALIZE)
      target_include_directories(libcataclysm PUBLIC
        ${LIBINTL_INCLUDE_DIR}
        ${ICONV_INCLUDE_DIR}
      )
      target_link_libraries(libcataclysm
        ${LIBINTL_LIBRARIES}
        ${ICONV_LIBRARIES}
      )
    ENDIF (LOCALIZE)

    target_include_directories(libcataclysm PUBLIC ${CURSES_INCLUDE_DIR})
    target_link_libraries(libcataclysm ${CURSES_LIBRARIES})

    IF(CMAKE_USE_PTHREADS_INIT)
      target_compile_options(libcataclysm PUBLIC "-pthread")
    ENDIF(CMAKE_USE_PTHREADS_INIT)

    IF(CMAKE_THREAD_LIBS_INIT)
      target_link_libraries(libcataclysm ${CMAKE_THREAD_LIBS_INIT})
    ENDIF(CMAKE_THREAD_LIBS_INIT)

    IF(WIN32)
      # Global settings for Windows targets (at end)
      target_link_libraries(libcataclysm gdi32.lib)
      target_link_libraries(libcataclysm winmm.lib)
      target_link_libraries(libcataclysm imm32.lib)
      target_link_libraries(libcataclysm ole32.lib)
      target_link_libraries(libcataclysm oleaut32.lib)
      target_link_libraries(libcataclysm version.lib)
      IF (BACKTRACE)
        target_link_libraries(libcataclysm dbghelp.lib)
        IF(LIBBACKTRACE)
          target_link_libraries(libcataclysm backtrace)
        ENDIF(LIBBACKTRACE)
      ENDIF(BACKTRACE)
    ENDIF(WIN32)

    IF(RELEASE)
      install(TARGETS cataclysm DESTINATION ${BIN_PREFIX})
    ENDIF(RELEASE)

ENDIF(CURSES)

IF(MINGW AND NOT RELEASE)
    # Try to Install shared libraries for dev builds
    # Note: It is specific to MSYS2 and uses hardcoded versions so
    # probably it will fail if you run it :)

    # GCC-specific libraries
    find_library(RuntimeLib_GCC_S_DW2_1  "gcc_s_dw2-1")
    find_library(RuntimeLib_STDC_PP_6    "stdc++-6")
    find_library(RuntimeLib_WINPTHREAD_1 "winpthread-1")
    SET(RuntimeLib_GCC_ALL
      ${RuntimeLib_GCC_S_DW2_1}
      ${RuntimeLib_STDC_PP_6}
      ${RuntimeLib_WINPTHREAD_1}
    )
    IF (LOCALIZE)
      find_library(RuntimeLib_iconv  "libiconv-2")
      find_library(RuntimeLib_intl  "libintl-8")
      SET(RuntimeLib_LOCALIZE
        ${RuntimeLib_iconv}
        ${RuntimeLib_intl}
      )
    ENDIF (LOCALIZE)
    IF (TILES)
      # SDL2 can have a varius deps. Here you are the MSYS2 ones...
      find_library(RuntimeLib_SDL2  "SDL2")
      find_library(RuntimeLib_SDL2_IMG  "SDL2_image")
      find_library(RuntimeLib_png  "libpng16-16")
      find_library(RuntimeLib_jpeg  "libjpeg-8")
      find_library(RuntimeLib_jbig  "libjbig-0")
      find_library(RuntimeLib_tiff  "libtiff-5")
      find_library(RuntimeLib_webp  "libwebp-5")
      find_library(RuntimeLib_lzma  "liblzma-5")
      find_library(RuntimeLib_bz2  "libbz2-1")
      find_library(RuntimeLib_zlib  "zlib1")
      find_library(RuntimeLib_hb  "libharfbuzz-0")
      find_library(RuntimeLib_SDL2_TTF  "SDL2_ttf")
      find_library(RuntimeLib_ft  "libfreetype-6")
      find_library(RuntimeLib_glib  "libglib-2.0-0")
      SET(RuntimeLib_SDL
        ${RuntimeLib_SDL2}
        ${RuntimeLib_SDL2_IMG}
        ${RuntimeLib_png}
        ${RuntimeLib_jpeg}
        ${RuntimeLib_jbig}
        ${RuntimeLib_tiff}
        ${RuntimeLib_webp}
        ${RuntimeLib_lzma}
        ${RuntimeLib_bz2}
        ${RuntimeLib_zlib}
        ${RuntimeLib_hb}
        ${RuntimeLib_SDL2_TTF}
        ${RuntimeLib_ft}
        ${RuntimeLib_glib}
      )
      IF(SOUND)
        find_library(RuntimeLib_SDL_SND  "SDL2_mixer")
        find_library(RuntimeLib_flak  "libFLAC-8")
        find_library(RuntimeLib_ogg  "libogg-0")
        find_library(RuntimeLib_flu  "libfluidsynth-1")
        find_library(RuntimeLib_port  "libportaudio-2")
        find_library(RuntimeLib_snd  "libsndfile-1")
        find_library(RuntimeLib_vorb  "libvorbis-0")
        find_library(RuntimeLib_vorb_enc  "libvorbisenc-2")
        find_library(RuntimeLib_vorb_f  "libvorbisfile-3")
        find_library(RuntimeLib_mod  "libmodplug-1")
        find_library(RuntimeLib_mpeg  "smpeg2")
        SET(RuntimeLib_SDL_SOUND
          ${RuntimeLib_SDL_SND}
          ${RuntimeLib_flak}
          ${RuntimeLib_ogg}
          ${RuntimeLib_flu}
          ${RuntimeLib_port}
          ${RuntimeLib_snd}
          ${RuntimeLib_vorb}
          ${RuntimeLib_vorb_enc}
          ${RuntimeLib_vorb_f}
          ${RuntimeLib_mod}
          ${RuntimeLib_mpeg}
        )
      ENDIF(SOUND)
    ENDIF (TILES)
    install(FILES ${RuntimeLib_GCC_ALL}
      ${RuntimeLib_LOCALIZE}
      ${RuntimeLib_SDL}
      ${RuntimeLib_SDL_SOUND}
      DESTINATION ${BIN_PREFIX}
    )
ENDIF(MINGW AND NOT RELEASE)

# vim:noet
