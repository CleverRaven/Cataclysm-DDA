cmake_minimum_required(VERSION 3.20)

# Don't clang-tidy third party code
set(CMAKE_CXX_CLANG_TIDY "")

# Root third-party library which will export all individual ones.
add_library(
        third-party
        INTERFACE)

set(THIRD_PARTY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/third-party)

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
        flatbuffers
        ghc-filesystem)
