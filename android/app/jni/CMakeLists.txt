cmake_minimum_required(VERSION 3.22)

# Output libmain.so; SDL3 dlsym's SDL_main from it
# (CataclysmDDA.java::getMainSharedObject).
project(cataclysm-android LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(CATA_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.."
                       ABSOLUTE)

set(TILES ON)
set(USE_SDL3 ON)
set(DYNAMIC_LINKING ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)

# Prefab AARs expose <SDL3/SDL.h> only; CDDA legacy bare <SDL_*.h> matches the
# pkg-config sdl3 desktop layout, so also add each SDL3 subdir to -I.
set(_sdl3_subdir_inc "")
foreach(lib SDL3 SDL3_image SDL3_ttf SDL3_mixer)
  foreach(tgt ${lib}::${lib} ${lib}::${lib}-shared
                  ${lib}::${lib}-Headers ${lib}::Headers)
    if(TARGET ${tgt})
      get_target_property(inc ${tgt} INTERFACE_INCLUDE_DIRECTORIES)
      if(inc)
        foreach(dir IN LISTS inc)
          if(IS_DIRECTORY "${dir}/SDL3")
            list(APPEND _sdl3_subdir_inc "${dir}/SDL3")
          endif()
        endforeach()
      endif()
    endif()
  endforeach()
endforeach()
list(REMOVE_DUPLICATES _sdl3_subdir_inc)

add_subdirectory(${CATA_REPO_ROOT}/src/third-party third-party)

file(GLOB CATA_SOURCES ${CATA_REPO_ROOT}/src/*.cpp
     ${CATA_REPO_ROOT}/src/cata_allocator_c.c)

add_library(main SHARED ${CATA_SOURCES})

target_include_directories(
  main PRIVATE ${CATA_REPO_ROOT}/src ${CATA_REPO_ROOT}/src/third-party
               ${_sdl3_subdir_inc})

target_compile_definitions(
  main
  PRIVATE TILES
          SDL_SOUND
          USE_SDL3
          BACKTRACE
          LOCALIZE
          ZSTD_STATIC_LINKING_ONLY
          ZSTD_DISABLE_ASM)

target_compile_options(main PRIVATE -fsigned-char -Wall -Wextra)

target_link_libraries(
  main
  PRIVATE third-party
          SDL3::SDL3
          SDL3_image::SDL3_image
          SDL3_ttf::SDL3_ttf
          SDL3_mixer::SDL3_mixer
          log
          GLESv2
          GLESv1_CM
          z)
