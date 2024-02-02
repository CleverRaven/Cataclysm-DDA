list(APPEND CMAKE_MODULE_PATH 
    ${CMAKE_SOURCE_DIR}/CMakeModules)
include(GetGitRevisionDescription)

git_describe(GIT_VERSION --tags --always --match "[0-9A-Z]*.[0-9A-Z]*")

if(EXISTS ${GIT_EXECUTABLE})
    execute_process(COMMAND ${GIT_EXECUTABLE} diff --quiet
        RESULT_VARIABLE DIRTY_FLAG
    )

    if(DIRTY_FLAG)
        string(APPEND GIT_VERSION "-dirty")
    endif()
endif()

message(NOTICE ${GIT_VERSION})

if("${GIT_VERSION}" MATCHES "GIT-NOTFOUND")
    return()
endif()

string(REPLACE "-NOTFOUND" "" GIT_VERSION ${GIT_VERSION})

file(TOUCH ${CMAKE_SOURCE_DIR}/src/version.h)
file(READ  ${CMAKE_SOURCE_DIR}/src/version.h VERSION_H)
string(REGEX MATCH "#define VERSION \"(.+)\"" VERSION_H "${VERSION_H}")

if(NOT GIT_VERSION STREQUAL VERSION_H)
    file(WRITE ${CMAKE_SOURCE_DIR}/src/version.h
            "// NOLINT(cata-header-guard)\n\#define VERSION \"${GIT_VERSION}\"\n")
endif()

# get_git_head_revision() does not work with worktrees in Windows
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    OUTPUT_VARIABLE _sha1
    OUTPUT_STRIP_TRAILING_WHITESPACE)
string(TIMESTAMP _timestamp %Y-%m-%d-%H%M)
file(WRITE ${CMAKE_SOURCE_DIR}/VERSION.txt "\
build type: Release\n\
build number: ${_timestamp}\n\
commit sha: ${_sha1}\n\
commit url: https://github.com/CleverRaven/Cataclysm-DDA/commit/${_sha1}"
)

