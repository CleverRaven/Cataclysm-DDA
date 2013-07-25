-- Tool to automatically generate bindings for lua.
-- The bindings are generated as C++ and lua files, which are
-- then included into the cataclysm source.


local br = "\n"
local tab = "    "

function member_type_to_lua_type(member_type)
    if member_type == "int" or member_type == "float" then
        return "LUA_TNUMBER"
    elseif member_type == "string" or member_type == "cstring" then
        return "LUA_TSTRING"
    elseif member_type == "bool" then
        return "LUA_TBOOLEAN"
    end
end

function member_type_to_cpp_type(member_type)
    if member_type == "bool" then return "int"
    elseif member_type == "cstring" then return "const char*"
    elseif member_type == "string" then return "std::string"
    end
    
    return member_type
end

function retrieve_lua_value(out_variable, value_type, stack_position)
    local cpp_value_type = member_type_to_cpp_type(value_type)
    if value_type == "int" or value_type == "float" then
        return cpp_value_type .. " "..out_variable.." = ("..cpp_value_type..") lua_tonumber(L, "..stack_position..");"
    elseif value_type == "bool" then
        return cpp_value_type .. " "..out_variable.." = ("..cpp_value_type..") lua_toboolean(L, "..stack_position..");"
    elseif value_type == "string" or value_type == "cstring" then
        return cpp_value_type .. " "..out_variable.." = ("..cpp_value_type..") lua_tostring(L, "..stack_position..");"
    end
end

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
    end
    return text
end

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

function generate_global_function_wrapper(function_name, function_to_call, args, rval)
    local text = "static int "..function_name.."(lua_State *L) {"..br

    for i, arg in ipairs(args) do
        text = text .. tab .. retrieve_lua_value("parameter"..i, arg, i)..br
    end

    text = text .. tab

    if rval then
        text = text .. member_type_to_cpp_type(rval) .. " rval = "
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

function generate_class_function_wrapper(class, function_name, function_to_call, args, rval)
    local text = "static int "..class.."_"..function_name.."(lua_State *L) {"..br

    -- retrieve the object to call the function on from the stack.
    text = text .. tab .. class .. "** "..class.."_instance = ("..class.."**) lua_touserdata(L, 1);\n"..br

    text = text .. tab .. "if(!"..class.."_instance) {"..br
    text = text .. tab .. tab .. 'return luaL_error(L, "First argument to '..function_name..' is not a '..class..'");'..br
    text = text .. tab .. "}" .. br

    local stack_index = 1
    for i, arg in ipairs(args) do
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

-- Generate our C++ source file with all getters and setters.
local cpp_output = "// This file was automatically generated by catalua/generate_bindings.lua"..br

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

for key, metatable in pairs(__metatables) do
    generate_accessors(metatable.attributes, key)
end

function generate_class_function_wrappers(functions, class)
    for name, func in pairs(functions) do
        cpp_output = cpp_output .. generate_class_function_wrapper(class, name, name, func.args, func.rval)
    end
end

generate_class_function_wrappers(__player_metatable.functions, "player")
generate_class_function_wrappers(__item_metatable.functions, "item")

for name, func in pairs(global_functions) do
    cpp_output = cpp_output .. generate_global_function_wrapper(name, func.cpp_name, func.args, func.rval)
end

-- Create a lua registry with our getters and setters.
cpp_output = cpp_output .. "static const struct luaL_Reg gamelib [] = {"..br

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

for key, metatable in pairs(__metatables) do
    generate_registry(metatable, key)
end

for name, func in pairs(global_functions) do
    cpp_output = cpp_output .. tab .. '{"'..name..'", '..name..'},'..br
end

cpp_output = cpp_output .. tab .. "{NULL, NULL}"..br.."};"..br

function writeFile(path,data)
   local file = io.open(path,"wb")
   file:write(data)
   file:close()
end

writeFile("catabindings.cpp", cpp_output)
