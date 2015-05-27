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
    if member_type == "bool" then return "bool"
    elseif member_type == "cstring" then return "const char*"
    elseif member_type == "string" then return "std::string"
    else
        for name, value in pairs(classes) do
            if name == member_type then
                if value.by_value then
                    return "LuaValue<" .. member_type .. ">"
                else
                    return "LuaReference<" .. member_type .. ">"
                end
            end
        end
    end
    
    return member_type
end

-- Returns code to retrieve a lua value from the stack and store it into
-- a C++ variable
function retrieve_lua_value(value_type, stack_position)
    if value_type == "int" or value_type == "float" then
        return "lua_tonumber(L, "..stack_position..");"
    elseif value_type == "bool" then
        return "lua_toboolean(L, "..stack_position..");"
    elseif value_type == "string" or value_type == "cstring" then
        return "lua_tostring_wrapper(L, "..stack_position..");"
    else
        return member_type_to_cpp_type(value_type) .. "::get_proxy(L, " .. stack_position .. ");"
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
    else
        text = text .. member_type_to_cpp_type(value_type) .. "::push(L, " .. in_variable .. ");"
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
function generate_getter(class, member_name, member_type, cpp_name)
    local function_name = class.."_get_"..member_name
    local text = "static int "..function_name.."(lua_State *L) {"..br

    text = text .. tab .. "auto & "..class.."_instance = "..member_type_to_cpp_type(class).."::get(L, 1);"..br

    text = text .. tab .. push_lua_value(class.."_instance."..cpp_name, member_type)..br

    text = text .. tab .. "return 1;  // 1 return value"..br
    text = text .. "}" .. br

    return text
end

-- Generates a setter function for a specific class and member variable.
function generate_setter(class, member_name, member_type, cpp_name)
    local function_name = class.."_set_"..member_name
    
    local text = "static int "..function_name.."(lua_State *L) {"..br

    text = text .. tab .. "auto & "..class.."_instance = "..member_type_to_cpp_type(class).."::get(L, 1);"..br

    text = text .. tab .. "luaL_checktype(L, 2, "..member_type_to_lua_type(member_type)..");"..br
    text = text .. tab .. "auto && value = " .. retrieve_lua_value(member_type, 2) ..br

    text = text .. tab .. class.."_instance."..cpp_name.." = value;"..br

    text = text .. tab .. "return 0;  // 0 return values"..br
    text = text .. "}" .. br

    return text
end

-- Generates a function wrapper fora  global function. "function_to_call" can be any string
-- that works as a "function", including expressions like "g->add_msg"
function generate_global_function_wrapper(function_name, function_to_call, args, rval)
    local text = "static int "..function_name.."(lua_State *L) {"..br

    for i, arg in ipairs(args) do
        text = text .. tab .. "luaL_checktype(L, "..i..", "..member_type_to_lua_type(arg)..");"..br
        text = text .. tab .. "auto && parameter"..i .. " = " .. retrieve_lua_value(arg, i)..br
    end

    text = text .. tab

    if rval then
        text = text .. "auto && rval = "
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
    text = text .. tab .. "auto & "..class.."_instance = "..member_type_to_cpp_type(class).."::get(L, 1);"..br

    local stack_index = 1
    for i, arg in ipairs(args) do
        -- fixme; non hardcoded userdata to class thingy
        if arg ~= "game" then
            text = text .. tab .. "luaL_checktype(L, "..(stack_index+1)..", "..member_type_to_lua_type(arg)..");"..br
            text = text .. tab .. "auto && parameter"..i .. " = " .. retrieve_lua_value(arg, stack_index+1)..br
            stack_index = stack_index + 1
        end
    end

    text = text .. tab

    if rval then
        text = text .. "auto && rval = "
    end

    text = text .. class.. "_instance."..function_to_call .. "("

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

function generate_constructor(class, args)
    local text = "static int "..class.."___call(lua_State *L) {"..br

    local stack_index = 1
    for i, arg in ipairs(args) do
        -- fixme; non hardcoded userdata to class thingy
        if arg ~= "game" then
            text = text .. tab .. "auto && parameter"..i .. " = " .. retrieve_lua_value(arg, stack_index+1)..br
            stack_index = stack_index + 1
        end
    end

    text = text .. tab .. "auto && rval = " .. class .. "("

    for i, arg in ipairs(args) do
        if arg == "game" then
            text = text .. "g"
        else
            text = text .. "parameter"..i
        end
        if next(args, i) then text = text .. ", " end
    end

    text = text .. ");"..br

    text = text .. tab .. push_lua_value("rval", class)..br
    text = text .. tab .. "return 1; // 1 return values"..br
    text = text .. "}"..br

    return text
end

function generate_operator(class, function_name, cppname)
    local text = "static int "..class.."_"..function_name.."(lua_State *L) {"..br

    if classes[class].by_value then
        text = text .. tab .. "auto & lhs = LuaValue<"..class..">::get( L, 1 );"..br
        text = text .. tab .. "auto & rhs = LuaValue<"..class..">::get( L, 2 );"..br
    else
        text = text .. tab .. "auto & lhs = LuaReference<"..class..">::get( L, 1 );"..br
        text = text .. tab .. "auto & rhs = LuaReference<"..class..">::get( L, 2 );"..br
    end

    text = text .. tab .. "bool rval = "

    if classes[class].by_value then
        text = text .. "lhs " .. cppname .. " rhs";
    else
        -- Both objects are pointers, they need to point to the same object to be equal.
        text = text .. "&lhs " .. cppname .. " &rhs";
    end
    text = text .. ";"

    text = text .. tab .. push_lua_value("rval", "bool")..br
    text = text .. tab .. "return 1; // 1 return values"..br
    text = text .. "}"..br

    return text
end

-- Generate our C++ source file with all wrappers for accessing variables and functions from lua.
-------------------------------------------------------------------------------------------------
local cpp_output = "// This file was automatically generated by lua/generate_bindings.lua"..br

local lua_output = ""

dofile "../../lua/class_definitions.lua"

function generate_accessors(class, name)
    -- Generate getters and setters for our player attributes.
    for key, attribute in pairs(class) do
        cpp_output = cpp_output .. generate_getter(name, key, attribute.type, attribute.cpp_name or key)
        if attribute.writable then
            cpp_output = cpp_output .. generate_setter(name, key, attribute.type, attribute.cpp_name or key)
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
    while value do
        generate_accessors(value.attributes, name)
        value = classes[value.parent]
    end
end

for name, value in pairs(classes) do
    while value do
        generate_class_function_wrappers(value.functions, name)
        if value.new then
            cpp_output = cpp_output .. generate_constructor(name, value.new)
        end
        if value.has_equal then
            cpp_output = cpp_output .. generate_operator(name, "__eq", "==")
        end
        value = classes[value.parent]
    end
end

for name, func in pairs(global_functions) do
    cpp_output = cpp_output .. generate_global_function_wrapper(name, func.cpp_name, func.args, func.rval)
end

-- luaL_Reg is the name of the struct in C which this creates and returns.
function luaL_Reg(name, suffix)
    local cpp_name = name .. "_" .. suffix
    local lua_name = suffix
    return tab .. '{"' .. lua_name .. '", ' .. cpp_name .. '},' .. br
end
-- Creates the LuaValue<T>::FUNCTIONS array, containing all the public functions of the class.
function generate_functions_static(cpp_type, class, name)
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const luaL_Reg " .. cpp_type .. "::FUNCTIONS[] = {" .. br
    while class do
        for key, _ in pairs(class.functions) do
            cpp_output = cpp_output .. luaL_Reg(name, key)
        end
        if class.new then
            cpp_output = cpp_output .. luaL_Reg(name, "__call")
        end
        if class.has_equal then
            cpp_output = cpp_output .. luaL_Reg(name, "__eq")
        end
        class = classes[class.parent]
    end
    cpp_output = cpp_output .. tab .. "{NULL, NULL}" .. br -- sentinel to indicate end of array
    cpp_output = cpp_output .. "};" .. br
end
-- Creates the LuaValue<T>::READ_MEMBERS map, containing the getters. Example:
-- const LuaValue<foo>::MRMap LuaValue<foo>::READ_MEMBERS = { { "id", foo_get_id }, ... };
function generate_read_members_static(cpp_type, class, name)
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const " .. cpp_type .. "::MRMap " .. cpp_type .. "::READ_MEMBERS = {" .. br
    while class do
        for key, attribute in pairs(class.attributes) do
            cpp_output = cpp_output .. tab .. "{\"" .. key .. "\", " .. name .. "_get_" .. key .. "}," .. br
        end
        class = classes[class.parent]
    end
    cpp_output = cpp_output .. "};" .. br
end
-- Creates the LuaValue<T>::READ_MEMBERS map, containing the setters. Example:
-- const LuaValue<foo>::MWMap LuaValue<foo>::WRITE_MEMBERS = { { "id", foo_set_id }, ... };
function generate_write_members_static(cpp_type, class, name)
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const " .. cpp_type .. "::MWMap " .. cpp_type .. "::WRITE_MEMBERS = {" .. br
    while class do
        for key, attribute in pairs(class.attributes) do
            if attribute.writable then
                cpp_output = cpp_output .. tab .. "{\"" .. key .. "\", " .. name .. "_set_" .. key .. "}," .. br
            end
        end
        class = classes[class.parent]
    end
    cpp_output = cpp_output .. "};" .. br
end

-- Create the static constant members of LuaValue
for name, value in pairs(classes) do
    cpp_output = cpp_output .. "template<>" .. br
    local cpp_name = ""
    -- The static constant is always define in LuaValue (LuaReference gets it via inheritance)
    -- But LuaReference inherits from LuaValue<T*>!
    if value.by_value then
        cpp_name = "LuaValue<" .. name .. ">"
    else
        cpp_name = "LuaValue<" .. name .. "*>"
    end
    cpp_output = cpp_output .. "const char * const " .. cpp_name .. "::METATABLE_NAME = \"" .. name .. "_metatable\";" .. br
    generate_functions_static(cpp_name, value, name)
    generate_read_members_static(cpp_name, value, name)
    generate_write_members_static(cpp_name, value, name)
end

-- Create a function that calls load_metatable on all the registered LuaValue's
cpp_output = cpp_output .. "static void load_metatables(lua_State* const L) {" .. br

for name, value in pairs(classes) do
    if value.by_value then
        cpp_output = cpp_output .. tab .. "LuaValue<" .. name .. ">::load_metatable( L );" .. br
    else
        cpp_output = cpp_output .. tab .. "LuaValue<" .. name .. "*>::load_metatable( L );" .. br
    end
end
cpp_output = cpp_output .. "}" .. br

-- Create a lua registry with the global functions
cpp_output = cpp_output .. "static const struct luaL_Reg gamelib [] = {"..br

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
