cmake_minimum_required(VERSION 3.20)

# Don't clang-tidy third party code
set(CMAKE_CXX_CLANG_TIDY "")

# Root third-party library which will export all individual ones.
add_library(
        third-party
        INTERFACE)

# Flatbuffers
file(GLOB FLATBUFFERS_HEADERS flatbuffers/**/*.h)
file(GLOB FLATBUFFERS_SOURCES flatbuffers/*.cpp)

add_library(
        flatbuffers
        STATIC
        ${FLATBUFFERS_HEADERS}
        ${FLATBUFFERS_SOURCES})

target_include_directories(
        flatbuffers
        SYSTEM
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

# For cata_flatbuffers_assert.h
target_include_directories(
        flatbuffers
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(
        third-party
        INTERFACE
        flatbuffers
        )

# GHC filesystem
file(GLOB GHC_HEADERS ghc/*.hpp)

add_library(
        ghc-filesystem
        INTERFACE)

target_sources(
        ghc-filesystem
        INTERFACE
        ${GHC_HEADERS})

target_include_directories(
        ghc-filesystem
        SYSTEM
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
        third-party
        INTERFACE
        ghc-filesystem
        )

# ImGUI
if (TILES)
        add_library(
                imgui
                STATIC
                imgui/imconfig.h
                imgui/imgui.cpp
                imgui/imgui.h
                imgui/imgui_demo.cpp
                imgui/imgui_draw.cpp
                imgui/imgui_freetype.cpp
                imgui/imgui_freetype.h
                imgui/imgui_impl_sdl2.cpp
                imgui/imgui_impl_sdl2.h
                imgui/imgui_impl_sdlrenderer2.cpp
                imgui/imgui_impl_sdlrenderer2.h
                imgui/imgui_internal.h
                imgui/imgui_stdlib.cpp
                imgui/imgui_stdlib.h
                imgui/imgui_tables.cpp
                imgui/imgui_widgets.cpp
                imgui/imstb_rectpack.h
                imgui/imstb_textedit.h
                imgui/imstb_truetype.h
                )

        target_include_directories(
                imgui
                SYSTEM
                PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}
                )

        target_include_directories(
                imgui
                SYSTEM
                PRIVATE
                ${SDL2_INCLUDE_DIR}/..
                /usr/include/freetype2
                )

        target_compile_options(
                imgui
                PRIVATE
                -w
                )

        if (NOT DYNAMIC_LINKING)
            target_link_libraries(imgui PUBLIC
                SDL2::SDL2-static
            )
        else()
            target_link_libraries(imgui PUBLIC
                SDL2::SDL2
                freetype
            )
        endif ()


        target_link_libraries(
                third-party
                INTERFACE
                imgui
                )
endif ()

# ImTUI
if (CURSES)
        add_library(
                imtui
                STATIC
                imgui/imconfig.h
                imgui/imgui.cpp
                imgui/imgui.h
                imgui/imgui_demo.cpp
                imgui/imgui_draw.cpp
                imgui/imgui_internal.h
                imgui/imgui_stdlib.cpp
                imgui/imgui_stdlib.h
                imgui/imgui_tables.cpp
                imgui/imgui_widgets.cpp
                imtui/imtui-impl-ncurses.cpp
                imtui/imtui-impl-ncurses.h
                imtui/imtui-impl-text.cpp
                imtui/imtui-impl-text.h
                imtui/imtui.h
                )

        target_include_directories(
                imtui
                SYSTEM
                PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}
        )
        target_compile_options(
                imtui
                PRIVATE
                -w
                )

        target_compile_definitions(
                imtui
                PUBLIC
                IMTUI
                $<$<BOOL:${MSYS2}>:NCURSES_INTERNALS>
        )

        target_link_libraries(
                third-party
                INTERFACE
                imtui
                )

        if (TARGET unofficial::pdcurses::pdcurses)
            target_link_libraries(
                    imtui
                    PUBLIC
                    unofficial::pdcurses::pdcurses
            )
            target_compile_definitions(
                imtui
                PUBLIC
                PDC_WIDE
                NCURSES_MOUSE_VERSION
            )
        elseif(TARGET pdcurses)
            target_link_libraries(imtui PUBLIC pdcurses)
        else() # FindCurses.cmake does not create targets
            target_include_directories(
                    imtui
                    SYSTEM
                    PUBLIC
                    ${CURSES_INCLUDE_DIR}
                    ${NCURSES_INCLUDE_DIRS}
                    ${CURSES_INCLUDE_DIR}/ncurses$<$<BOOL:${CURSES_NEED_WIDE}>:w>
            )
        endif()
endif ()
