cmake_minimum_required(VERSION 3.20)

set(CHKJSON_SOURCES
        ${CMAKE_SOURCE_DIR}/src/json.cpp
        ${CMAKE_SOURCE_DIR}/src/chkjson/chkjson.cpp)

set(CHKJSON_HEADERS
        ${CMAKE_SOURCE_DIR}/src/json.h)

# test chain
add_custom_target(
        check_json
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_command(
        TARGET check_json
        PRE_BUILD
        COMMAND chkjson
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include_directories(
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/chkjson)

# Since we're just cherrypicking enough stuff to make it build.
include_directories(
        SYSTEM
        ${CMAKE_SOURCE_DIR}/src/third-party)

# Add the actual executable
if (WIN32)
    add_executable(chkjson WIN32 EXCLUDE_FROM_ALL ${CHKJSON_SOURCES} ${CHKJSON_HEADERS})
else ()
    add_executable(chkjson EXCLUDE_FROM_ALL ${CHKJSON_SOURCES} ${CHKJSON_HEADERS})
endif ()
