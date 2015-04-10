set(CATACLYSM_DDA_LUA_SOURCES
    ${CMAKE_SOURCE_DIR}/lua/class_definitions.lua
    ${CMAKE_SOURCE_DIR}/lua/autoexec.lua
)

add_custom_target (
	tests
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command (
	TARGET tests
	PRE_BUILD
	COMMAND ${LUA_BINARY} lua/json_verifier.lua
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


IF(RELEASE)
	install(FILES ${CATACLYSM_DDA_LUA_SOURCES} DESTINATION ${DATA_PREFIX}/lua)
ENDIF(RELEASE)
