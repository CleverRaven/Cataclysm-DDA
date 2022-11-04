include(ExternalProject)

add_executable(
        json_formatter
        format.cpp
        format_main.cpp
        ${CMAKE_SOURCE_DIR}/src/json.cpp)

target_link_libraries(json_formatter flatbuffers)

add_definitions(-DCATA_IN_TOOL)

include_directories(${CMAKE_SOURCE_DIR}/src)

