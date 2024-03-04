add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third-party)

set(MAIN_CPP ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
set(MESSAGES_CPP ${CMAKE_CURRENT_SOURCE_DIR}/messages.cpp)
set(RESOURCE_RC ${CMAKE_CURRENT_SOURCE_DIR}/resource.rc)

if (WIN32)
    set(RSRC_RC_DEP "${CMAKE_SOURCE_DIR}/data/cataicon.ico")
    set(RSRC_RC_DEP "${RSRC_RC_DEP};${CMAKE_SOURCE_DIR}/data/application_manifest.xml")
    set_source_files_properties(
            ${RESOURCE_RC}
            PROPERTIES
            OBJECT_DEPENDS "${RSRC_RC_DEP}")
endif ()

file(GLOB CATACLYSM_DDA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

list(REMOVE_ITEM CATACLYSM_DDA_SOURCES ${MAIN_CPP} ${MESSAGES_CPP})

file(GLOB CATACLYSM_DDA_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# Get GIT version strings
add_custom_target(
        get_version
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/version.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/version.h
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Build tiles version if requested
if (TILES)
    add_library(
            cataclysm-tiles-common OBJECT
            ${CATACLYSM_DDA_SOURCES}
            ${CATACLYSM_DDA_HEADERS})
    target_include_directories(cataclysm-tiles-common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(cataclysm-tiles-common PUBLIC third-party)

    if (WIN32)
        add_definitions(-DUSE_WINMAIN)
        add_executable(
                cataclysm-tiles WIN32
                ${MAIN_CPP}
                ${MESSAGES_CPP}
                ${RESOURCE_RC})
        if(MSYS2)
            set(_KEEP $<$<CONFIG:RelWithDebInfo,Debug>:objcopy
            --only-keep-debug
            $<TARGET_FILE:cataclysm-tiles>
            $<TARGET_FILE:cataclysm-tiles>.debug
            >)
            set(_STRIP $<$<CONFIG:RelWithDebInfo,Debug>:objcopy
            --strip-unneeded
            --add-gnu-debuglink=$<TARGET_FILE:cataclysm-tiles>.debug
            $<TARGET_FILE:cataclysm-tiles>
            >)
            add_custom_command(TARGET cataclysm-tiles POST_BUILD
                COMMAND "${_KEEP}"
                COMMAND "${_STRIP}"
                COMMAND_EXPAND_LISTS
            )
        else()
            # Use only our RT_MANIFEST from resource.rc
            target_link_options(cataclysm-tiles PRIVATE
                "LINKER:/MANIFEST:NO"
            )
        endif()
    else ()
        add_executable(cataclysm-tiles
                ${MAIN_CPP}
                ${MESSAGES_CPP})
    endif ()

    add_dependencies(cataclysm-tiles-common get_version)

    target_link_libraries(cataclysm-tiles PRIVATE cataclysm-tiles-common)
    target_compile_definitions(cataclysm-tiles-common PUBLIC TILES )
    if(NOT "${CMAKE_EXPORT_COMPILE_COMMANDS}")
        target_precompile_headers(cataclysm-tiles-common PUBLIC
            ${CMAKE_SOURCE_DIR}/pch/main-pch.hpp)
    endif ()
    if (LOCALIZE)
        target_include_directories(cataclysm-tiles-common PUBLIC
                ${LIBINTL_INCLUDE_DIR}
                ${ICONV_INCLUDE_DIR})
        target_link_libraries(cataclysm-tiles-common PUBLIC
                ${LIBINTL_LIBRARIES}
                ${ICONV_LIBRARIES})
    endif ()

    if (CMAKE_USE_PTHREADS_INIT)
        target_compile_options(cataclysm-tiles-common PUBLIC "-pthread")
    endif ()

    if (CMAKE_THREAD_LIBS_INIT)
        target_link_libraries(cataclysm-tiles-common PUBLIC ${CMAKE_THREAD_LIBS_INIT})
    endif ()

    if (NOT DYNAMIC_LINKING)
        target_link_libraries(cataclysm-tiles-common PUBLIC
            SDL2::SDL2-static
            SDL2_image::SDL2_image-static
            SDL2_ttf::SDL2_ttf-static
        )
    else()
        target_link_libraries(cataclysm-tiles-common PUBLIC
            SDL2::SDL2
            SDL2_image::SDL2_image
            SDL2_ttf::SDL2_ttf
        )
    endif ()
    if (SOUND)
        if (NOT DYNAMIC_LINKING)
            target_link_libraries(cataclysm-tiles-common PUBLIC
                SDL2_mixer::SDL2_mixer-static
            )
        else()
            target_link_libraries(cataclysm-tiles-common PUBLIC
                SDL2_mixer::SDL2_mixer
            )
        endif()
        add_definitions(-DSDL_SOUND)
    endif ()

    if (WIN32)
        # Global settings for Windows targets (at end)
        target_link_libraries(cataclysm-tiles-common PUBLIC gdi32.lib)
        target_link_libraries(cataclysm-tiles-common PUBLIC winmm.lib)
        target_link_libraries(cataclysm-tiles-common PUBLIC imm32.lib)
        target_link_libraries(cataclysm-tiles-common PUBLIC ole32.lib)
        target_link_libraries(cataclysm-tiles-common PUBLIC oleaut32.lib)
        target_link_libraries(cataclysm-tiles-common PUBLIC version.lib)
        target_link_libraries(cataclysm-tiles-common PUBLIC setupapi.lib)
        if (SOUND)
            target_link_libraries(cataclysm-tiles-common PUBLIC shlwapi.lib)
        endif()
        if (BACKTRACE)
            target_link_libraries(cataclysm-tiles-common PUBLIC dbghelp.lib)
            if (LIBBACKTRACE)
                target_link_libraries(cataclysm-tiles-common PUBLIC backtrace)
            endif ()
        endif ()
    elseif (APPLE)
        target_link_libraries(cataclysm-tiles-common PUBLIC "-framework CoreFoundation")
    endif ()

    if (LIBBACKTRACE)
        target_link_libraries(cataclysm-tiles-common PUBLIC backtrace)
    endif ()

    if (RELEASE)
        install(TARGETS cataclysm-tiles RUNTIME)
    endif ()
endif ()

# Build curses version if requested
if (CURSES)
    add_library(cataclysm-common OBJECT
            ${CATACLYSM_DDA_SOURCES}
            ${CATACLYSM_DDA_HEADERS})
    target_include_directories(cataclysm-common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(cataclysm-common PUBLIC third-party)

    if (WIN32)
        add_executable(cataclysm
                ${MAIN_CPP}
                ${MESSAGES_CPP}
                ${RESOURCE_RC})
    else ()
        add_executable(cataclysm
                ${MAIN_CPP}
                ${MESSAGES_CPP})
    endif ()

    add_dependencies(cataclysm-common get_version)
    target_link_libraries(cataclysm PRIVATE cataclysm-common)

    if (LOCALIZE)
        target_include_directories(cataclysm-common PUBLIC
                ${LIBINTL_INCLUDE_DIR}
                ${ICONV_INCLUDE_DIR})
        target_link_libraries(cataclysm-common PUBLIC
                ${LIBINTL_LIBRARIES}
                ${ICONV_LIBRARIES})
    endif ()

    target_include_directories(cataclysm-common PUBLIC ${CURSES_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(cataclysm-common PUBLIC ${CURSES_LIBRARIES} ${ZLIB_LIBRARIES})

    if (CMAKE_USE_PTHREADS_INIT)
        target_compile_options(cataclysm-common PUBLIC "-pthread")
    endif ()

    if (CMAKE_THREAD_LIBS_INIT)
        target_link_libraries(cataclysm-common PUBLIC ${CMAKE_THREAD_LIBS_INIT})
    endif ()

    if (WIN32)
        # Global settings for Windows targets (at end)
        target_link_libraries(cataclysm-common PUBLIC gdi32.lib)
        target_link_libraries(cataclysm-common PUBLIC winmm.lib)
        target_link_libraries(cataclysm-common PUBLIC imm32.lib)
        target_link_libraries(cataclysm-common PUBLIC ole32.lib)
        target_link_libraries(cataclysm-common PUBLIC oleaut32.lib)
        target_link_libraries(cataclysm-common PUBLIC version.lib)
        if (BACKTRACE)
            target_link_libraries(cataclysm-common PUBLIC dbghelp.lib)
        endif ()
    elseif (APPLE)
        target_link_libraries(cataclysm-common PUBLIC "-framework CoreFoundation")
    endif ()

    if (LIBBACKTRACE)
        target_link_libraries(cataclysm-common PUBLIC backtrace)
    endif ()

    if (RELEASE)
        install(TARGETS cataclysm RUNTIME)
    endif ()
endif ()

if (MINGW AND NOT RELEASE)
    # Try to Install shared libraries for dev builds
    # Note: It is specific to MSYS2 and uses hardcoded versions so
    # probably it will fail if you run it :)

    # GCC-specific libraries and zlib
    find_library(RuntimeLib_GCC_S_DW2_1  "gcc_s_dw2-1")
    find_library(RuntimeLib_STDC_PP_6    "stdc++-6")
    find_library(RuntimeLib_WINPTHREAD_1 "winpthread-1")
    find_library(RuntimeLib_zlib  "zlib1")

    set(RuntimeLib_GCC_ALL
            ${RuntimeLib_GCC_S_DW2_1}
            ${RuntimeLib_STDC_PP_6}
            ${RuntimeLib_WINPTHREAD_1}
            ${RuntimeLib_zlib})
    if (TILES)
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
        find_library(RuntimeLib_hb  "libharfbuzz-0")
        find_library(RuntimeLib_SDL2_TTF  "SDL2_ttf")
        find_library(RuntimeLib_ft  "libfreetype-6")
        find_library(RuntimeLib_glib  "libglib-2.0-0")
        set(RuntimeLib_SDL
                ${RuntimeLib_SDL2}
                ${RuntimeLib_SDL2_IMG}
                ${RuntimeLib_png}
                ${RuntimeLib_jpeg}
                ${RuntimeLib_jbig}
                ${RuntimeLib_tiff}
                ${RuntimeLib_webp}
                ${RuntimeLib_lzma}
                ${RuntimeLib_bz2}
                ${RuntimeLib_hb}
                ${RuntimeLib_SDL2_TTF}
                ${RuntimeLib_ft}
                ${RuntimeLib_glib})
        if (SOUND)
            find_library(RuntimeLib_SDL_SND  "SDL2_mixer")
            find_library(RuntimeLib_flac  "libFLAC-8")
            find_library(RuntimeLib_ogg  "libogg-0")
            find_library(RuntimeLib_flu  "libfluidsynth-1")
            find_library(RuntimeLib_port  "libportaudio-2")
            find_library(RuntimeLib_snd  "libsndfile-1")
            find_library(RuntimeLib_vorb  "libvorbis-0")
            find_library(RuntimeLib_vorb_enc  "libvorbisenc-2")
            find_library(RuntimeLib_vorb_f  "libvorbisfile-3")
            find_library(RuntimeLib_mod  "libmodplug-1")
            find_library(RuntimeLib_mpeg  "smpeg2")
            set(RuntimeLib_SDL_SOUND
                    ${RuntimeLib_SDL_SND}
                    ${RuntimeLib_flac}
                    ${RuntimeLib_ogg}
                    ${RuntimeLib_flu}
                    ${RuntimeLib_port}
                    ${RuntimeLib_snd}
                    ${RuntimeLib_vorb}
                    ${RuntimeLib_vorb_enc}
                    ${RuntimeLib_vorb_f}
                    ${RuntimeLib_mod}
                    ${RuntimeLib_mpeg})
        endif ()
    endif ()
    install(FILES ${RuntimeLib_GCC_ALL}
            ${RuntimeLib_LOCALIZE}
            ${RuntimeLib_SDL}
            ${RuntimeLib_SDL_SOUND}
            TYPE BIN)
endif ()
