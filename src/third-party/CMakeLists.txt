cmake_minimum_required(VERSION 3.20)

# Don't clang-tidy third party code
set(CMAKE_CXX_CLANG_TIDY "")

# Root third-party library which will export all individual ones.
add_library(
        third-party
        INTERFACE)

set(THIRD_PARTY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/third-party)

# Flatbuffers
file(GLOB FLATBUFFERS_HEADERS ${THIRD_PARTY_SOURCE_DIR}/flatbuffers/**/*.h)
file(GLOB FLATBUFFERS_SOURCES ${THIRD_PARTY_SOURCE_DIR}/flatbuffers/*.cpp)

add_library(
        flatbuffers
        STATIC
        ${FLATBUFFERS_HEADERS}
        ${FLATBUFFERS_SOURCES})

target_include_directories(
        flatbuffers
        SYSTEM
        PUBLIC
        ${THIRD_PARTY_SOURCE_DIR})

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
file(GLOB GHC_HEADERS ${THIRD_PARTY_SOURCE_DIR}/ghc/*.hpp)

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
        ${THIRD_PARTY_SOURCE_DIR})

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
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imconfig.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_internal.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imstb_rectpack.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imstb_textedit.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imstb_truetype.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_demo.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_draw.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_tables.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_widgets.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_impl_sdl2.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_impl_sdl2.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_impl_sdlrenderer2.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_impl_sdlrenderer2.h
                )

        target_include_directories(
                imgui
                SYSTEM
                PUBLIC
                ${THIRD_PARTY_SOURCE_DIR}
                )

        target_include_directories(
                imgui
                SYSTEM
                PRIVATE
                ${SDL2_INCLUDE_DIR}/..
                )

        target_compile_options(
                imgui
                PRIVATE
                -w
                )

        target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_KEYIO)

        if (NOT DYNAMIC_LINKING)
            target_link_libraries(imgui PUBLIC
                SDL2::SDL2-static
            )
        else()
            target_link_libraries(imgui PUBLIC
                SDL2::SDL2
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
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imconfig.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_internal.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imstb_rectpack.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imstb_textedit.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imstb_truetype.h
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_demo.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_draw.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_tables.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui_widgets.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imgui/imgui.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imtui/imtui-impl-ncurses.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imtui/imtui-impl-ncurses.h
                ${THIRD_PARTY_SOURCE_DIR}/imtui/imtui-impl-text.cpp
                ${THIRD_PARTY_SOURCE_DIR}/imtui/imtui-impl-text.h
                ${THIRD_PARTY_SOURCE_DIR}/imtui/imtui.h
                )

        target_include_directories(
                imtui
                SYSTEM
                PUBLIC
                ${THIRD_PARTY_SOURCE_DIR}
                )

        target_compile_options(
                imtui
                PRIVATE
                -w
                )

        target_link_libraries(
                third-party
                INTERFACE
                imtui
                )
endif ()
