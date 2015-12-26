-- Tool to automatically generate bindings for lua.
-- The bindings are generated as C++ and lua files, which are
-- then included into the cataclysm source.

-- Conventions:
-- The variable holding the name of a class is named "class_name"
-- The variable holding the data (declarations etc.) of a class is "class"
-- Example code: for class_name, class in pairs(classes) do ...

local br = "\n"
local tab = "    "

-- Generic helpers to generate C++ source code chunks for use in our lua binding.
---------------------------------------------------------------------------------

-- Convert a given type such as "string" to the corresponding C++
-- type string, e.g. "std::string". For types wrapped in LuaReference/LuaValue/LuaEnum, it
-- returns the wrapped type, e.g. "LuaValue<ter_t>"
function member_type_to_cpp_type(member_type)
    if member_type == "bool" then return "bool"
    elseif member_type == "cstring" then return "const char*"
    elseif member_type == "string" then return "std::string"
    elseif member_type == "int" then return "int"
    elseif member_type == "float" then return "float"
    else
        for class_name, class in pairs(classes) do
            if class_name == member_type then
                if class.by_value then
                    return "LuaValue<" .. member_type .. ">"
                else
                    return "LuaReference<" .. member_type .. ">"
                end
            end
        end
        for enum_name, _ in pairs(enums) do
            if enum_name == member_type then
                return "LuaEnum<" .. member_type .. ">"
            end
        end

        error("'"..member_type.."' is not a build-in type and is not defined in class_definitions.lua")
    end
end

-- Loads an instance of class_name (which must be the first thing on the stack) into a local
-- variable, named "<class_name>_instance". Only use for classes (not enums/primitives).
function load_instance(class_name)
    if not classes[class_name] then
        error("'"..class_name.."' is not defined in class_definitions.lua")
    end

    local instance_name = class_name .. "_instance"
    local wrapper_type = ""
    if classes[class_name].by_value then
        wrapper_type = "LuaValue<" .. class_name .. ">"
    else
        wrapper_type = "LuaValue<" .. class_name .. "*>"
    end
    return class_name .. "& " .. instance_name .. " = " .. wrapper_type .. "::get(L, 1);"
end

-- Returns code to retrieve a lua value from the stack and store it into
-- a C++ variable
function retrieve_lua_value(value_type, stack_position)
    return "LuaType<" .. member_type_to_cpp_type(value_type) .. ">::get(L, " .. stack_position .. ");"
end

-- Returns code to take a C++ variable of the given type and push a lua version
-- of it onto the stack.
function push_lua_value(in_variable, value_type)
    return "LuaType<" .. member_type_to_cpp_type(value_type) .. ">::push(L, " .. in_variable .. ");"
end

-- Generates a getter function for a specific class and member variable.
function generate_getter(class_name, member_name, member_type, cpp_name)
    local function_name = class_name.."_get_"..member_name
    local text = "static int "..function_name.."(lua_State *L) {"..br

    text = text .. tab .. load_instance(class_name)..br

    text = text .. tab .. push_lua_value(class_name.."_instance."..cpp_name, member_type)..br

    text = text .. tab .. "return 1;  // 1 return value"..br
    text = text .. "}" .. br

    return text
end

-- Generates a setter function for a specific class and member variable.
function generate_setter(class_name, member_name, member_type, cpp_name)
    local function_name = class_name.."_set_"..member_name

    local text = "static int "..function_name.."(lua_State *L) {"..br

    text = text .. tab .. load_instance(class_name)..br

    text = text .. tab .. "LuaType<"..member_type_to_cpp_type(member_type)..">::check(L, 2);"..br
    text = text .. tab .. "auto && value = " .. retrieve_lua_value(member_type, 2)..br

    text = text .. tab .. class_name.."_instance."..cpp_name.." = value;"..br

    text = text .. tab .. "return 0;  // 0 return values"..br
    text = text .. "}" .. br

    return text
end

-- Generates a function wrapper for a global function. "function_to_call" can be any string
-- that works as a "function", including expressions like "g->add_msg"
function generate_global_function_wrapper(function_name, function_to_call, args, rval)
    local text = "static int "..function_name.."(lua_State *L) {"..br

    for i, arg in ipairs(args) do
        text = text .. tab .. "LuaType<"..member_type_to_cpp_type(arg)..">::check(L, "..i..");"..br
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

--[[
To allow function overloading, we need to create a function that somehow detects the input
types and calls the matching C++ overload.
First step is to restructure the input: instead of
functions = {
    { name = "f", ... },
    { name = "f", ... },
    { name = "f", ... },
}
we want a decision tree. The leafs of the tree are the rval entries (they can be different for
each overload, but they do not affect the overload resolution).
Example: functions = {
    r = { rval = "int", cpp_name = "f" }
    "string" = {
        r = { rval = "int", cpp_name = "f" }
    },
    "int" = {
        r = { rval = "bool", cpp_name = "f" }
        "int" = {
            r = { rval = "void", cpp_name = "f" }
        },
        "string" = {
            r = { rval = "int", cpp_name = "f" }
        }
    }
}
Means: `int f()` `int f(string)` `bool f(int)` void f(int, int)` `int f(int, string)`
Leafs are marked by the presence of rval entries.
--]]
function generate_overload_tree(classes)
    for class_name, value in pairs(classes) do
        local functions_by_name = {}
        for _, func in ipairs(value.functions) do
            if not func.name then
                print("Every function of " .. class_name .. " needs a name, doesn't it?")
            end
            -- Create the table now. In the next loop below, we can simply assume it already exists
            functions_by_name[func.name] = {}
        end
        -- This creates the mentioned tree: each entry has the key matching the parameter type,
        -- and the final table (the leaf) has a `r` entry.
        for _, func in ipairs(value.functions) do
            local root = functions_by_name[func.name]
            for _, arg in pairs(func.args) do
                if not root[arg] then
                    root[arg] = {}
                end
                root = root[arg]
            end
            root.r = { rval = func.rval, cpp_name = func.cpp_name or func.name }
        end
        value.functions = functions_by_name
    end
end

--[[
This (recursive) function handles function overloading:
- function_name: actual function name (only for showing an error message)
- args: tree of argument (see generate_overload_tree)
- indentation: level of indentation of the source code (only used for nice formatting)
- stack_index: index (1-based, because this is Lua) of the currently handled argument.
- cbc: callback that returns the C++ code that actually calls the C++ function.
  Its parameters:
  - indentation: level of indentation it should use
  - stack_index: number of parameters it can use (C++ variables parameter1, parameter2 and so on)
  - rval: type of the object the C++ function returns (can be nil)
  - cpp_name: name of the C++ function to call.
--]]
function insert_overload_resolution(function_name, args, cbc, indentation, stack_index)
    local ind = string.rep("    ", indentation)
    local text = ""
    -- Number of choices that can be made for function overloading
    local count = 0
    for _ in pairs(args) do count = count + 1 end
    local more = (count ~= 1)
    -- If we can chose several overloads at this point (more=true), we have to add if statements
    -- and have to increase the indentation inside of them.
    -- Otherwise (no choices), we keep everything at the same indentation level.
    -- (e.g. no overload at all, or this parameter is the same for all overloads).
    local nsi = stack_index + 1 -- next stack_index
    local ni = more and (indentation + 1) or indentation -- next indentation
    local mind = more and (ind .. tab) or ind -- more indentation
    local valid_types = "" -- list of acceptable types, for the error message
    for arg_type, more_args in pairs(args) do
        if arg_type == "r" then
            -- handled outside this loop
        else
            if more then
                -- Either check the type here (and continue when it's fine)
                text = text..ind.."if(LuaType<"..member_type_to_cpp_type(arg_type)..">::has(L, "..(nsi)..")) {"..br
            else
                -- or check it here and let Lua bail out.
                text = text..ind.."LuaType<"..member_type_to_cpp_type(arg_type)..">::check(L, "..nsi..");"..br
            end
            text = text..mind.."auto && parameter"..stack_index.." = "..retrieve_lua_value(arg_type, nsi)..br
            text = text..insert_overload_resolution(function_name, more_args, cbc, ni, nsi)
            if more then
                text = text..ind.."}"..br
            end
            valid_types = valid_types.." or "..arg_type
        end
    end
    -- An overload can be called at this level, all required parameters are already extracted.
    if args.r then
        if more then
            text = text .. ind .. "if(lua_gettop(L) == "..stack_index..") {"..br
        end
        -- If we're here, any further arguments will ignored, so raise an error if there are any left over
        text = text..mind.."if(lua_gettop(L) > "..stack_index..") {"..br
        text = text..mind..tab.."return luaL_error(L, \"Too many arguments to "..function_name..", expected only "..stack_index..", got %d\", lua_gettop(L));"..br
        text = text..mind.."}"..br
        text = text .. cbc(ni, stack_index - 1, args.r.rval, args.r.cpp_name)
        if more then
            text = text .. ind .. "}"..br
        end
        valid_types = valid_types .. " or nothing at all"
    end
    -- If more is false, there was no branching (no `if` statement) generated, but a return
    -- statement had already been made, so this error would be unreachable code.
    if more then
        text = text .. ind .. "return luaL_argerror(L, "..stack_index..", \"Unexpected type, expected are "..valid_types:sub(5).."\");"..br
    end
    return text
end

-- Generate a wrapper around a class function(method) that allows us to call a method of a specific
-- C++ instance by calling the method on the corresponding lua wrapper, e.g.
-- monster:name() in lua translates to monster.name() in C++
function generate_class_function_wrapper(class_name, function_name, func, cur_class_name)
    local text = "static int "..class_name.."_"..function_name.."(lua_State *L) {"..br

    -- retrieve the object to call the function on from the stack.
    text = text .. tab .. load_instance(class_name)..br

    local cbc = function(indentation, stack_index, rval, function_to_call)
    local tab = string.rep("    ", indentation)
    text = tab

    if rval then
        text = text .. "auto && rval = "
    end

    if cur_class_name == class_name then
        text = text .. class_name .. "_instance"
    else
        --[[
        If we call a function of the parent class is called, we need to call it through
        a reference to the parent class, otherwise we get into trouble with default parameters:
        Example:
        class A {     void f(int = 0); }
        class B : A { void f(int); }
        This won't work: B b; b.f();
        But this will:   B b; static_cast<A&>(b).f()
        --]]
        text = text .. "static_cast<"..cur_class_name.."&>(" .. class_name .. "_instance)"
    end
    text = text .. "."..function_to_call .. "("

    for i = 1,stack_index do
        text = text .. "parameter"..i
        if i < stack_index then text = text .. ", " end
    end

    text = text .. ");"..br

    if rval then
        text = text .. tab .. push_lua_value("rval", rval)..br
        text = text .. tab .. "return 1; // 1 return values"..br
    else
        text = text .. tab .. "return 0; // 0 return values"..br
    end
    return text
    end

    text = text .. insert_overload_resolution(function_name, func, cbc, 1, 1)

    text = text .. "}"..br

    return text
end

function generate_constructor(class_name, args)
    local text = "static int "..class_name.."___call(lua_State *L) {"..br

    local stack_index = 1
    for i, arg in ipairs(args) do
        text = text .. tab .. "auto && parameter"..i .. " = " .. retrieve_lua_value(arg, stack_index+1)..br
        stack_index = stack_index + 1
    end

    text = text .. tab .. "auto && rval = " .. class_name .. "("

    for i, arg in ipairs(args) do
        text = text .. "parameter"..i
        if next(args, i) then text = text .. ", " end
    end

    text = text .. ");"..br

    text = text .. tab .. push_lua_value("rval", class_name)..br
    text = text .. tab .. "return 1; // 1 return values"..br
    text = text .. "}"..br

    return text
end

function generate_operator(class_name, function_name, cppname)
    local text = "static int "..class_name.."_"..function_name.."(lua_State *L) {"..br

    if classes[class_name].by_value then
        text = text .. tab .. "auto & lhs = LuaValue<"..class_name..">::get( L, 1 );"..br
        text = text .. tab .. "auto & rhs = LuaValue<"..class_name..">::get( L, 2 );"..br
    else
        text = text .. tab .. "auto & lhs = LuaReference<"..class_name..">::get( L, 1 );"..br
        text = text .. tab .. "auto & rhs = LuaReference<"..class_name..">::get( L, 2 );"..br
    end

    text = text .. tab .. "bool rval = "

    if classes[class_name].by_value then
        text = text .. "lhs " .. cppname .. " rhs";
    else
        -- Both objects are pointers, they need to point to the same object to be equal.
        text = text .. "&lhs " .. cppname .. " &rhs";
    end
    text = text .. ";"..br

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

generate_overload_tree(classes)

function generate_accessors(class, class_name)
    -- Generate getters and setters for our player attributes.
    for key, attribute in pairs(class) do
        cpp_output = cpp_output .. generate_getter(class_name, key, attribute.type, attribute.cpp_name or key)
        if attribute.writable then
            cpp_output = cpp_output .. generate_setter(class_name, key, attribute.type, attribute.cpp_name or key)
        end
    end
end


function generate_class_function_wrappers(functions, class_name, cur_class_name)
    for index, func in pairs(functions) do
        cpp_output = cpp_output .. generate_class_function_wrapper(class_name, index, func, cur_class_name)
    end
end

for class_name, class in pairs(classes) do
    while class do
        generate_accessors(class.attributes, class_name)
        class = classes[class.parent]
    end
end

for class_name, class in pairs(classes) do
    local cur_class_name = class_name
    while class do
        generate_class_function_wrappers(class.functions, class_name, cur_class_name)
        if class.new then
            cpp_output = cpp_output .. generate_constructor(class_name, class.new)
        end
        if class.has_equal then
            cpp_output = cpp_output .. generate_operator(class_name, "__eq", "==")
        end
        cur_class_name = class.parent
        class = classes[class.parent]
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
function generate_functions_static(cpp_type, class, class_name)
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const luaL_Reg " .. cpp_type .. "::FUNCTIONS[] = {" .. br
    while class do
        for name, _ in pairs(class.functions) do
            cpp_output = cpp_output .. luaL_Reg(class_name, name)
        end
        if class.new then
            cpp_output = cpp_output .. luaL_Reg(class_name, "__call")
        end
        if class.has_equal then
            cpp_output = cpp_output .. luaL_Reg(class_name, "__eq")
        end
        class = classes[class.parent]
    end
    cpp_output = cpp_output .. tab .. "{NULL, NULL}" .. br -- sentinel to indicate end of array
    cpp_output = cpp_output .. "};" .. br
end
-- Creates the LuaValue<T>::READ_MEMBERS map, containing the getters. Example:
-- const LuaValue<foo>::MRMap LuaValue<foo>::READ_MEMBERS{ { "id", foo_get_id }, ... };
function generate_read_members_static(cpp_type, class, class_name)
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const " .. cpp_type .. "::MRMap " .. cpp_type .. "::READ_MEMBERS{" .. br
    while class do
        for key, attribute in pairs(class.attributes) do
            cpp_output = cpp_output .. tab .. "{\"" .. key .. "\", " .. class_name .. "_get_" .. key .. "}," .. br
        end
        class = classes[class.parent]
    end
    cpp_output = cpp_output .. "};" .. br
end
-- Creates the LuaValue<T>::READ_MEMBERS map, containing the setters. Example:
-- const LuaValue<foo>::MWMap LuaValue<foo>::WRITE_MEMBERS{ { "id", foo_set_id }, ... };
function generate_write_members_static(cpp_type, class, class_name)
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const " .. cpp_type .. "::MWMap " .. cpp_type .. "::WRITE_MEMBERS{" .. br
    while class do
        for key, attribute in pairs(class.attributes) do
            if attribute.writable then
                cpp_output = cpp_output .. tab .. "{\"" .. key .. "\", " .. class_name .. "_set_" .. key .. "}," .. br
            end
        end
        class = classes[class.parent]
    end
    cpp_output = cpp_output .. "};" .. br
end

-- The static constant is always define in LuaValue (LuaReference gets it via inheritance)
-- But LuaReference inherits from LuaValue<T*>!
function wrapper_base_class(class_name)
    -- This must not be LuaReference because it is used for declaring/defining the static members
    -- and those should onloy exist for LuaValue.
    if classes[class_name].by_value then
        return "LuaValue<" .. class_name .. ">"
    else
        return "LuaValue<" .. class_name .. "*>"
    end
end

-- Create the static constant members of LuaValue
for class_name, value in pairs(classes) do
    local cpp_name = wrapper_base_class(class_name)
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const char * const " .. cpp_name .. "::METATABLE_NAME = \"" .. class_name .. "_metatable\";" .. br
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. cpp_name.."::Type *"..cpp_name.."::get_subclass( lua_State* const S, int const i) {"..br
    for child, value in pairs(classes) do
        -- Note: while the function get_subclass resides in LuaValue<T>, this calls into LuaValue or
        -- LuaReference, that way we get a simple pointer. Unconditionally calling LuaValue<T>::get,
        -- would result in returning monster** (LuaValue<T>::get returns a T&, applying `&` gives T*).
        -- Now consider that T is already a pointer for LuaValue<monster*>, so T* would be monster**
        -- And that can not be converted to Creature**!
        -- In the end, this function does not return T*. but std::remove_pointer<T>::type* and that
        -- is basically T* (if T is not a pointer) or T (if T is already a pointer).
        local cpp_child_name = member_type_to_cpp_type(child);
        if value.parent == class_name then
            cpp_output = cpp_output .. tab .. "if("..cpp_child_name.."::has(S, i)) {" .. br
            cpp_output = cpp_output .. tab .. tab .. "return &"..cpp_child_name.."::get( S, i );" .. br
            cpp_output = cpp_output .. tab .. "}" .. br
        end
    end
    cpp_output = cpp_output .. tab .. "(void)S; (void)i;" .. br -- just in case to prevent warnings
    cpp_output = cpp_output .. tab .. "return nullptr;" .. br
    cpp_output = cpp_output .. "}" .. br
    generate_functions_static(cpp_name, value, class_name)
    generate_read_members_static(cpp_name, value, class_name)
    generate_write_members_static(cpp_name, value, class_name)
end

-- Create a function that calls load_metatable on all the registered LuaValue's
cpp_output = cpp_output .. "static void load_metatables(lua_State* const L) {" .. br

for class_name, class in pairs(classes) do
    local cpp_name = wrapper_base_class(class_name)
    -- If the class has a constructor, it should be exposed via a global name (which is the class name)
    if class.new then
        cpp_output = cpp_output .. tab .. cpp_name .. "::load_metatable( L, \"" .. class_name .. "\" );" .. br
    else
        cpp_output = cpp_output .. tab .. cpp_name .. "::load_metatable( L, nullptr );" .. br
    end
end
cpp_output = cpp_output .. "}" .. br

-- Create bindings lists for enums
for enum_name, values in pairs(enums) do
    local cpp_name = "LuaEnum<"..enum_name..">"
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const "..cpp_name.."::EMap "..cpp_name.."::BINDINGS = {"..br
    for _, name in ipairs(values) do
        cpp_output = cpp_output .. tab.."{\""..name.."\", "..name.."},"..br
    end
    cpp_output = cpp_output .. "};" .. br
end

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
