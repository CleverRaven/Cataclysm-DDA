-- Tool to automatically generate bindings for lua.
-- The bindings are generated as C++ and lua files, which are
-- then included into the cataclysm source.


local br = "\n"
local tab = "    "

-- Generic helpers to generate C++ source code chunks for use in our lua binding.
---------------------------------------------------------------------------------

-- Convert a given type such as "int", "bool" etc to a lua type, such
-- as LUA_TNUMBER etc, needed for typechecking mostly.
function member_type_to_lua_type(member_type)
    if member_type == "int" or member_type == "float" then
        return "LUA_TNUMBER"
    elseif member_type == "string" or member_type == "cstring" then
        return "LUA_TSTRING"
    elseif member_type == "bool" then
        return "LUA_TBOOLEAN"
    else
        for name, value in pairs(classes) do
            if name == member_type then
                return "LUA_TUSERDATA"
            end
        end
    end
end

-- Convert a given type such as "string" to the corresponding C++
-- type string, e.g. "std::string"
function member_type_to_cpp_type(member_type)
    if member_type == "bool" then return "int"
    elseif member_type == "cstring" then return "const char*"
    elseif member_type == "string" then return "std::string"
    else
        for name, value in pairs(classes) do
            if name == member_type then
                return member_type.."*"
            end
        end
    end
    
    return member_type
end

-- Returns code to retrieve a lua value from the stack and store it into
-- a C++ variable
function retrieve_lua_value(out_variable, value_type, stack_position)
    local cpp_value_type = member_type_to_cpp_type(value_type)
    if value_type == "int" or value_type == "float" then
        return cpp_value_type .. " "..out_variable.." = ("..cpp_value_type..") lua_tonumber(L, "..stack_position..");"
    elseif value_type == "bool" then
        return cpp_value_type .. " "..out_variable.." = ("..cpp_value_type..") lua_toboolean(L, "..stack_position..");"
    elseif value_type == "string" or value_type == "cstring" then
        return cpp_value_type .. " "..out_variable.." = ("..cpp_value_type..") lua_tostring(L, "..stack_position..");"
    elseif member_type_to_lua_type(value_type) == "LUA_TUSERDATA" then
        return cpp_value_type .. " "..out_variable.." = ("..cpp_value_type..") lua_touserdata(L, "..stack_position..");"
    end
end

-- Returns code to take a C++ variable of the given type and push a lua version
-- of it onto the stack.
function push_lua_value(in_variable, value_type)
    local text = ""
    if value_type == "int" or value_type == "float" then
        text = text .. "lua_pushnumber(L, "..in_variable..");"
    elseif value_type == "cstring" then
        text = text .. "lua_pushstring(L, "..in_variable..");"
    elseif value_type == "string" then
        text = text .. "lua_pushstring(L, "..in_variable..".c_str());"
    elseif value_type == "bool" then
        text = text .. "lua_pushboolean(L, "..in_variable..");"
    elseif member_type_to_lua_type(value_type) == "LUA_TUSERDATA" then
        text = text .. "if(" .. in_variable .. " == NULL) { lua_pushnil(L); } else {"
        text = text .. value_type .. "** userdata_" .. in_variable .. " = ("..value_type.."**) lua_newuserdata(L, sizeof("..value_type.."*));"
        text = text .. "*userdata_" .. in_variable .. " = ("..value_type.."*)"..in_variable..";"
        text = text .. 'luah_setmetatable(L, "'..value_type..'_metatable");}'
    end
    
    return text
end

--[[
        text = text .. indentation .. value_type .. "** userdata" .. parameter_index .. " = ("..value_type.."**) lua_newuserdata(L, sizeof("..value_type.."*));"
        text = text .. indentation .. "*userdata" .. parameter_index .. " = "..in_variable..";"
        text = text .. indentation .. value_type .. "parameter_in_registry_" .. parameter_index .. " = luah_store_in_registry(L, -1);"..br
        text = text .. indentation .. 'luah_setmetatable(L, "'..value_type..'_metatable");'..br
function cleanup_lua_parameter(value_type, parameter_index, indentation)
    local text = ""
    if member_type_to_lua_type(value_type) == "LUA_TUSERDATA" then
        text = text .. indentation .. "luah_remove_from_registry(L, parameter_in_registry_" .. parameter_index .. ");"..br
        text = text .. indentation .. 'luah_setmetatable(L, "outdated_metatable");'..br
    end
    return text
end
]]

-- Generates a getter function for a specific class and member variable.
function generate_getter(class, member_name, member_type)
    local function_name = class.."_get_"..member_name
    local text = "static int "..function_name.."(lua_State *L) {"..br

    text = text .. tab .. class .. "** "..class.."_instance = ("..class.."**) lua_touserdata(L, 1);\n"..br

    text = text .. tab .. "if(!"..class.."_instance) {"..br
    text = text .. tab .. tab .. 'return luaL_error(L, "First argument to '..function_name..' is not a '..class..'");'..br
    text = text .. tab .. "}" .. br

    text = text .. tab .. push_lua_value("(*"..class.."_instance"..")->"..member_name, member_type)..br

    text = text .. tab .. "return 1;  // 1 return value"..br
    text = text .. "}" .. br

    return text
end

-- Generates a setter function for a specific class and member variable.
function generate_setter(class, member_name, member_type)
    local function_name = class.."_set_"..member_name
    
    local text = "static int "..function_name.."(lua_State *L) {"..br

    text = text .. tab .. class .. "** "..class.."_instance = ("..class.."**) lua_touserdata(L, 1);\n"..br

    text = text .. tab .. "if(!"..class.."_instance) {"..br
    text = text .. tab .. tab .. 'return luaL_error(L, "First argument to '..function_name..' is not a '..class..'");'..br
    text = text .. tab .. "}" .. br

    text = text .. tab .. "luaL_checktype(L, 2, "..member_type_to_lua_type(member_type)..");"..br

    text = text .. tab .. retrieve_lua_value("value", member_type, 2) ..br

    text = text .. tab .. "(*"..class.."_instance)->"..member_name.." = value;"..br

    text = text .. tab .. "return 0;  // 0 return values"..br
    text = text .. "}" .. br

    return text
end

-- Generates a function wrapper fora  global function. "function_to_call" can be any string
-- that works as a "function", including expressions like "g->add_msg"
function generate_global_function_wrapper(function_name, function_to_call, args, rval)
    local text = "static int "..function_name.."(lua_State *L) {"..br

    for i, arg in ipairs(args) do
        text = text .. tab .. retrieve_lua_value("parameter"..i, arg, i)..br
    end

    text = text .. tab

    if rval then
        text = text .. member_type_to_cpp_type(rval) .. " rval = (" .. member_type_to_cpp_type(rval) .. ")"
    end

    text = text .. function_to_call .. "("

    for i, arg in ipairs(args) do
        text = text .. "parameter"..i
        if next(args, i) then text = text .. ", " end
    end

    text = text .. ");"..br

    if rval then
        text = text .. tab .. push_lua_value("rval", rval)..br
        text = text .. tab .. "return 1; // 1 return values"..br
    else
        text = text .. tab .. "return 0; // 0 return values"..br
    end
    text = text .. "}"..br

    return text
end

-- Generate a wrapper around a class function(method) that allows us to call a method of a specific
-- C++ instance by calling the method on the corresponding lua wrapper, e.g.
-- monster:name() in lua translates to monster.name() in C++
function generate_class_function_wrapper(class, function_name, function_to_call, args, rval)
    local text = "static int "..class.."_"..function_name.."(lua_State *L) {"..br

    -- retrieve the object to call the function on from the stack.
    text = text .. tab .. class .. "** "..class.."_instance = ("..class.."**) lua_touserdata(L, 1);\n"..br

    text = text .. tab .. "if(!"..class.."_instance) {"..br
    text = text .. tab .. tab .. 'return luaL_error(L, "First argument to '..function_name..' is not a '..class..'. Did you use foo.bar() instead of foo:bar()?");'..br
    text = text .. tab .. "}" .. br

    local stack_index = 1
    for i, arg in ipairs(args) do
        -- fixme; non hardcoded userdata to class thingy
        if arg ~= "game" then
            text = text .. tab .. retrieve_lua_value("parameter"..i, arg, stack_index+1)..br
            stack_index = stack_index + 1
        end
    end

    text = text .. tab

    if rval then
        text = text .. member_type_to_cpp_type(rval) .. " rval = "
    end

    text = text .. "(*" .. class.. "_instance)->"..function_to_call .. "("

    for i, arg in ipairs(args) do
        if arg == "game" then
            text = text .. "g"
        else
            text = text .. "parameter"..i
        end
        if next(args, i) then text = text .. ", " end
    end

    text = text .. ");"..br

    if rval then
        text = text .. tab .. push_lua_value("rval", rval)..br
        text = text .. tab .. "return 1; // 1 return values"..br
    else
        text = text .. tab .. "return 0; // 0 return values"..br
    end
    text = text .. "}"..br

    return text
end

-- Generate our C++ source file with all wrappers for accessing variables and functions from lua.
-------------------------------------------------------------------------------------------------
local cpp_output = "// This file was automatically generated by lua/generate_bindings.lua"..br

local lua_output = ""

dofile "class_definitions.lua"

function generate_accessors(class, name)
    -- Generate getters and setters for our player attributes.
    for key, attribute in pairs(class) do
        cpp_output = cpp_output .. generate_getter(name, key, attribute.type)
        if attribute.writable then
            cpp_output = cpp_output .. generate_setter(name, key, attribute.type)
        end
    end
end


function generate_class_function_wrappers(functions, class)
    for name, func in pairs(functions) do
        if func.cpp_name == nil then
            cpp_output = cpp_output .. generate_class_function_wrapper(class, name, name, func.args, func.rval)
        else
            cpp_output = cpp_output .. generate_class_function_wrapper(class, name, func.cpp_name, func.args, func.rval)
        end
    end
end

for name, value in pairs(classes) do
    generate_accessors(value.attributes, name)
end

for name, value in pairs(classes) do
    generate_class_function_wrappers(value.functions, name)
end

for name, func in pairs(global_functions) do
    cpp_output = cpp_output .. generate_global_function_wrapper(name, func.cpp_name, func.args, func.rval)
end

-- Create a lua registry with our getters and setters.
cpp_output = cpp_output .. "static const struct luaL_Reg gamelib [] = {"..br

-- Now that the wrapper functions are implemented, we need to make them accessible to lua.
-- For this, the lua "registry" is used, which maps strings to C functions.
function generate_registry(class, name)
    for key, attribute in pairs(class.attributes) do
        local getter_name = name.."_get_"..key
        local setter_name = name.."_set_"..key
        cpp_output = cpp_output .. tab .. '{"'..getter_name..'", '..getter_name..'},'..br
        if attribute.writable then
            cpp_output = cpp_output .. tab .. '{"'..setter_name..'", '..setter_name..'},'..br
        end
    end

    for key, func in pairs(class.functions) do
        local func_name = name.."_"..key
        cpp_output = cpp_output .. tab .. '{"'..func_name..'", '..func_name..'},'..br
    end
end

for name, value in pairs(classes) do
    generate_registry(value, name)
end

for name, func in pairs(global_functions) do
    cpp_output = cpp_output .. tab .. '{"'..name..'", '..name..'},'..br
end

cpp_output = cpp_output .. tab .. "{NULL, NULL}"..br.."};"..br

-- We generated our binding, now write it to a .cpp file for inclusion in
-- the main source code.
function writeFile(path,data)
   local file = io.open(path,"wb")
   file:write(data)
   file:close()
end

writeFile("catabindings.cpp", cpp_output)
