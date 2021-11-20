cmake_minimum_required(VERSION 3.1.4)


# Root third-party library which will export all individual ones.
add_library(
        third-party
        INTERFACE)

set(THIRD_PARTY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/third-party)

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
        ghc-filesystem)
