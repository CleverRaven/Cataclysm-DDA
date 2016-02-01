# JSON check utility
cmake_minimum_required(VERSION 2.8.12)

SET (CHKJSON_SOURCES
	${CMAKE_SOURCE_DIR}/src/json.cpp
	${CMAKE_SOURCE_DIR}/src/chkjson/chkjson.cpp
)

SET (CHKJSON_HEADERS
	${CMAKE_SOURCE_DIR}/src/json.h
)

# test chain
add_custom_target (
	check_json
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command (
	TARGET check_json
	PRE_BUILD
	COMMAND chkjson
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/chkjson )

# Add the actual executable
IF(WIN32)
	ADD_EXECUTABLE(chkjson WIN32 EXCLUDE_FROM_ALL ${CHKJSON_SOURCES} ${CHKJSON_HEADERS})
ELSE(WIN32)
	ADD_EXECUTABLE(chkjson EXCLUDE_FROM_ALL ${CHKJSON_SOURCES} ${CHKJSON_HEADERS})
ENDIF(WIN32)
