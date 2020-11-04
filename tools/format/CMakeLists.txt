include(ExternalProject)

add_executable(
    json_formatter
    format.cpp
    format_main.cpp
    ${CMAKE_SOURCE_DIR}/src/json.cpp
    )

ADD_DEFINITIONS(-DCATA_IN_TOOL)

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src )
