# CataclysmDDA Lua bindings
cmake_minimum_required(VERSION 2.8.12)

MESSAGE(STATUS "Searching for Lua library --\n")
FIND_PACKAGE(Lua)
IF(NOT LUA_FOUND)
	MESSAGE(FATAL_ERROR
		"You need the Lua development library to be able to compile with Lua support.\nSee INSTALL file for details and more info\n"
	)
ENDIF(NOT LUA_FOUND)

ADD_DEFINITIONS(-DLUA)

add_custom_target (
	lua_bindings
	ALL
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_CUSTOM_COMMAND (
	TARGET lua_bindings PRE_BUILD
	COMMAND ${LUA_BINARY} generate_bindings.lua
	BYPRODUCTS ${CMAKE_SOURCE_DIR}/src/lua/catabindings.cpp
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/lua
)
