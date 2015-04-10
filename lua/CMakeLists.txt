# LUA files + test
cmake_minimum_required(VERSION 2.8.12)

SET(CATACLYSM_DDA_LUA_SOURCES
    ${CMAKE_SOURCE_DIR}/lua/class_definitions.lua
    ${CMAKE_SOURCE_DIR}/lua/autoexec.lua
)

# test chain
add_custom_target (
	check_lua
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command (
	TARGET check_lua
	PRE_BUILD
	COMMAND ${LUA_BINARY} lua/json_verifier.lua
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

IF(RELEASE)
	install(FILES ${CATACLYSM_DDA_LUA_SOURCES} DESTINATION ${DATA_PREFIX}/lua)
ENDIF(RELEASE)
