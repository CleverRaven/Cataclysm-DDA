-- Tool to automatically generate bindings for lua.
-- The bindings are generated as C++ and lua files, which are
-- then included into the cataclysm source.

-- Conventions:
-- The variable holding the name of a class is named "class_name"
-- The variable holding the data (declarations etc.) of a class is "class"
-- Example code: for class_name, class in pairs(classes) do ...

-- The generated helper C++ functions use this naming system:
-- Getter: "get_" .. class_name .. "_" .. member_name
-- Setter: "set_" .. class_name .. "_" .. member_name
-- Member function: "func_" .. class_name .. "_" .. member_name
-- Operators: "op_" .. class_name .. "_" .. operator_id
-- Constructors: "new_" .. class_name
-- Global functions get a "global_" prefix.
-- This allows a data member "foo", as well as a function member "get_foo(...)".
-- They would get "get_class_foo", "set_class_foo" and "func_class_get_foo" wrappers.

local br = "\n"
local tab = "    "

-- Helper function for sorted iteration over tables
local function sorted_pairs(t)
    local keys = {}
    for key in pairs(t) do
        table.insert(keys, key)
    end
    table.sort(keys)

    return function()
        local key = table.remove(keys, 1)
        return key, t[key]
    end
end

-- Generic helpers to generate C++ source code chunks for use in our lua binding.
---------------------------------------------------------------------------------

-- Convert a given type such as "string" to the corresponding C++ wrapper class,
-- e.g. `LuaType<std::string>`. The wrapper class has various static functions:
-- `get` to get a value of that type from Lua stack.
-- `push` to push a value of that type to Lua stack.
-- `check` and `has` to check for a value of that type on the stack.
-- See catalua.h for their implementation.
function member_type_to_cpp_type(member_type)
    if member_type == "bool" then return "LuaType<bool>"
    elseif member_type == "cstring" then return "LuaType<const char*>"
    elseif member_type == "string" then return "LuaType<std::string>"
    elseif member_type == "int" then return "LuaType<int>"
    elseif member_type == "float" then return "LuaType<float>"
    else
        for class_name, class in pairs(classes) do
            if class_name == member_type then
                if class.by_value then
                    return "LuaValue<" .. member_type .. ">"
                elseif class.by_value_and_reference then
                    return "LuaValueOrReference<" .. member_type .. ">"
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
-- variable, named "instance". Only use for classes (not enums/primitives).
function load_instance(class_name)
    if not classes[class_name] then
        error("'"..class_name.."' is not defined in class_definitions.lua")
    end

    return class_name .. "& instance = " .. retrieve_lua_value(class_name, 1) .. ";"
end

-- Returns a full statement that checks whether the given stack item has the given value.
-- The statement does not return if the check fails (long jump back into the Lua error handling).
function check_lua_value(value_type, stack_position)
    return member_type_to_cpp_type(value_type).."::check(L, " .. stack_position .. ");"
end

-- Returns an expression that evaluates to `true` if the stack has an object of the given type
-- at the given position.
function has_lua_value(value_type, stack_position)
    return member_type_to_cpp_type(value_type) .. "::has(L, " .. stack_position .. ")"
end

-- Returns code to retrieve a lua value from the stack and store it into
-- a C++ variable
function retrieve_lua_value(value_type, stack_position)
    return member_type_to_cpp_type(value_type) .. "::get(L, " .. stack_position .. ")"
end

-- Returns code to take a C++ variable of the given type and push a lua version
-- of it onto the stack.
function push_lua_value(in_variable, value_type)
    local wrapper
    if value_type:sub(-1) == "&" then
        -- A reference is to be pushed. Copying the referred to object may not be allowed  (it may
        -- be a reference to a global game object).
        local t = value_type:sub(1, -2)
        if classes[t] then
            if classes[t].by_value_and_reference then
                -- special case becaus member_type_to_cpp_type would return LuaValueOrReference,
                -- which does not have a push function.
                wrapper = "LuaReference<" .. t .. ">"
            else
                wrapper = member_type_to_cpp_type(t)
            end
        else
            wrapper = member_type_to_cpp_type(t)
        end
    elseif classes[value_type] then
        -- Not a native Lua type, but it's not a reference, so we *have* to copy it (the value would
        -- go out of scope otherwise). Copy semantic means using LuaValue.
        wrapper = "LuaValue<" .. value_type .. ">"
    else
        -- Either an undefined type or a native Lua type, both is handled in member_type_to_cpp_type
        wrapper = member_type_to_cpp_type(value_type)
    end

    return wrapper .. "::push(L, " .. in_variable .. ");"
end

-- Generates a getter function for a specific class and member variable.
function generate_getter(class_name, member_name, member_type, cpp_name)
    local function_name = "get_" .. class_name .. "_" .. member_name
    local text = "static int "..function_name.."(lua_State *L) {"..br

    text = text .. tab .. load_instance(class_name)..br

    -- adding the "&" to the type, so push_lua_value knows it's a reference.
    text = text .. tab .. push_lua_value("instance."..cpp_name, member_type .. "&")..br

    text = text .. tab .. "return 1;  // 1 return value"..br
    text = text .. "}" .. br

    return text
end

-- Generates a setter function for a specific class and member variable.
function generate_setter(class_name, member_name, member_type, cpp_name)
    local function_name = "set_" .. class_name .. "_" .. member_name

    local text = "static int "..function_name.."(lua_State *L) {"..br

    text = text .. tab .. load_instance(class_name)..br

    text = text .. tab .. check_lua_value(member_type, 2)..";"..br
    text = text .. tab .. "instance."..cpp_name.." = " .. retrieve_lua_value(member_type, 2)..";"..br

    text = text .. tab .. "return 0;  // 0 return values"..br
    text = text .. "}" .. br

    return text
end

-- Generates a function wrapper for a global function. "function_to_call" can be any string
-- that works as a "function", including expressions like "g->add_msg"
function generate_global_function_wrapper(function_name, function_to_call, args, rval)
    local text = "static int global_"..function_name.."(lua_State *L) {"..br

    for i, arg in ipairs(args) do
        text = text .. tab .. check_lua_value(arg, i)..br
        -- Needs to be auto, can be a proxy object, a real reference, or a POD.
        -- This will be resolved when the functin is called. The C++ compiler will convert
        -- the auto to the expected parameter type.
        text = text .. tab .. "auto && parameter"..i .. " = " .. retrieve_lua_value(arg, i)..";"..br
    end

    local func_invoc = function_to_call .. "("
    for i, arg in ipairs(args) do
        func_invoc = func_invoc .. "parameter"..i
        if next(args, i) then func_invoc = func_invoc .. ", " end
    end
    func_invoc = func_invoc .. ")"

    if rval then
        text = text .. tab .. push_lua_value(func_invoc, rval)..br
        text = text .. tab .. "return 1; // 1 return values"..br
    else
        text = text .. tab .. func_invoc .. ";"..br
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

        if value.new then
            local new_root = {}
            for _, func in ipairs(value.new) do
                local root = new_root
                for _, arg in pairs(func) do
                    if not root[arg] then
                        root[arg] = {}
                    end
                    root = root[arg]
                end
                root.r = { rval = nil, cpp_name = class_name .. "::" .. class_name }
            end
            value.new = new_root
        end
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
    for arg_type, more_args in sorted_pairs(args) do
        if arg_type == "r" then
            -- handled outside this loop
        else
            if more then
                -- Either check the type here (and continue when it's fine)
                text = text..ind.."if("..has_lua_value(arg_type, nsi)..") {"..br
            else
                -- or check it here and let Lua bail out.
                text = text..ind..check_lua_value(arg_type, nsi)..br
            end
            -- Needs to be auto, can be a proxy object, a real reference, or a POD.
            -- This will be resolved when the functin is called. The C++ compiler will convert
            -- the auto to the expected parameter type.
            text = text..mind.."auto && parameter"..stack_index.." = "..retrieve_lua_value(arg_type, nsi)..";"..br
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
    local text = "static int func_" .. class_name .. "_" .. function_name .. "(lua_State *L) {"..br

    -- retrieve the object to call the function on from the stack.
    text = text .. tab .. load_instance(class_name)..br

    local cbc = function(indentation, stack_index, rval, function_to_call)
        local tab = string.rep("    ", indentation)

        local func_invoc
        if cur_class_name == class_name then
            func_invoc = "instance"
        else
            --[[
            If we call a function of the parent class, we need to call it through
            a reference to the parent class, otherwise we get into trouble with default parameters:
            Example:
            class A {     void f(int = 0); }
            class B : A { void f(int); }
            This won't work: B b; b.f();
            But this will:   B b; static_cast<A&>(b).f()
            --]]
            func_invoc = "static_cast<"..cur_class_name.."&>(instance)"
        end
        func_invoc = func_invoc .. "."..function_to_call .. "("

        for i = 1,stack_index do
            func_invoc = func_invoc .. "parameter"..i
            if i < stack_index then func_invoc = func_invoc .. ", " end
        end
        func_invoc = func_invoc .. ")"

        local text
        if rval then
            text = tab .. push_lua_value(func_invoc, rval)..br
            text = text .. tab .. "return 1; // 1 return values"..br
        else
            text = tab .. func_invoc .. ";"..br
            text = text .. tab .. "return 0; // 0 return values"..br
        end
        return text
    end

    text = text .. insert_overload_resolution(function_name, func, cbc, 1, 1)

    text = text .. "}"..br

    return text
end

function generate_constructor(class_name, args)
    local text = "static int new_" .. class_name .. "(lua_State *L) {"..br

    local cbc = function(indentation, stack_index, rval, function_to_call)
        local tab = string.rep("    ", indentation)

        -- Push is always done on a value, never on a pointer/reference, therefor don't use
        -- `push_lua_value` (which uses member_type_to_cpp_type to get either LuaValue or LuaReference).
        local text = tab .. "LuaValue<" .. class_name .. ">::push(L"

        for i = 1,stack_index do
            text = text .. ", parameter"..i
        end

        text = text .. ");"..br
        text = text .. tab .. "return 1; // 1 return values"..br
        return text
    end

    text = text .. insert_overload_resolution(class_name .. "::" .. class_name, args, cbc, 1, 1)

    text = text .. "}"..br

    return text
end

function generate_operator(class_name, operator_id, cppname)
    local text = "static int op_" .. class_name .. "_" .. operator_id .. "(lua_State *L) {"..br

    text = text .. tab .. "const " .. class_name .. " &lhs = " .. retrieve_lua_value(class_name, 1) .. ";"..br
    text = text .. tab .. "const " .. class_name .. " &rhs = " .. retrieve_lua_value(class_name, 2) .. ";"..br

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
    for key, attribute in sorted_pairs(class) do
        cpp_output = cpp_output .. generate_getter(class_name, key, attribute.type, attribute.cpp_name or key)
        if attribute.writable then
            cpp_output = cpp_output .. generate_setter(class_name, key, attribute.type, attribute.cpp_name or key)
        end
    end
end

function generate_class_function_wrappers(functions, class_name, cur_class_name)
    for index, func in sorted_pairs(functions) do
        cpp_output = cpp_output .. generate_class_function_wrapper(class_name, index, func, cur_class_name)
    end
end

for class_name, class in sorted_pairs(classes) do
    while class do
        generate_accessors(class.attributes, class_name)
        class = classes[class.parent]
    end
end

for class_name, class in sorted_pairs(classes) do
    local cur_class_name = class_name
    while class do
        generate_class_function_wrappers(class.functions, class_name, cur_class_name)
        if class.new then
            cpp_output = cpp_output .. generate_constructor(class_name, class.new)
        end
        if class.has_equal then
            cpp_output = cpp_output .. generate_operator(class_name, "eq", "==")
        end
        cur_class_name = class.parent
        class = classes[class.parent]
    end
end

for name, func in sorted_pairs(global_functions) do
    cpp_output = cpp_output .. generate_global_function_wrapper(name, func.cpp_name, func.args, func.rval)
end

-- luaL_Reg is the name of the struct in C which this creates and returns.
function luaL_Reg(cpp_name, lua_name)
    return tab .. '{"' .. lua_name .. '", ' .. cpp_name .. '},' .. br
end
-- Creates the LuaValue<T>::FUNCTIONS array, containing all the public functions of the class.
function generate_functions_static(cpp_type, class, class_name)
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const luaL_Reg " .. cpp_type .. "::FUNCTIONS[] = {" .. br
    while class do
        for name, _ in sorted_pairs(class.functions) do
            cpp_output = cpp_output .. luaL_Reg("func_" .. class_name .. "_" .. name, name)
        end
        if class.new then
            cpp_output = cpp_output .. luaL_Reg("new_" .. class_name, "__call")
        end
        if class.has_equal then
            cpp_output = cpp_output .. luaL_Reg("op_" .. class_name .. "_eq", "__eq")
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
        for key, attribute in sorted_pairs(class.attributes) do
            local function_name = "get_" .. class_name .. "_" .. key
            cpp_output = cpp_output .. tab .. "{\"" .. key .. "\", " .. function_name .. "}," .. br
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
        for key, attribute in sorted_pairs(class.attributes) do
            if attribute.writable then
                local function_name = "set_" .. class_name .. "_" .. key
                cpp_output = cpp_output .. tab .. "{\"" .. key .. "\", " .. function_name .. "}," .. br
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

function generate_LuaValue_constants(class_name, class, by_value_and_reference)
    local cpp_name = ""
    local metatable_name = ""
    if by_value_and_reference then
        -- A different metatable name, to allow the C++ wrappers to detect what the
        -- object from Lua refers to. The wrappers can thereby be used by both types
        -- at the same type. In other words: the created static functions `get_foo_member`
        -- can be called on a value that was pushed to Lua via `Lua<foo>::push` (and is a value),
        -- and values pushed via `LuaReference<foo>::push` (which is a pointer).
        metatable_name = "value_of_" .. class_name .. "_metatable"
        cpp_name = "LuaValue<" .. class_name .. ">"
    else
        metatable_name = class_name .. "_metatable"
        cpp_name = wrapper_base_class(class_name)
    end
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. "const char * const " .. cpp_name .. "::METATABLE_NAME = \"" .. metatable_name .. "\";" .. br
    cpp_output = cpp_output .. "template<>" .. br
    cpp_output = cpp_output .. cpp_name.."::Type *"..cpp_name.."::get_subclass( lua_State* const S, const int i) {"..br
    for child, class in sorted_pairs(classes) do
        -- Note: while the function get_subclass resides in LuaValue<T>, this calls into LuaValue or
        -- LuaReference, that way we get a simple pointer. Unconditionally calling LuaValue<T>::get,
        -- would result in returning monster** (LuaValue<T>::get returns a T&, applying `&` gives T*).
        -- Now consider that T is already a pointer for LuaValue<monster*>, so T* would be monster**
        -- And that can not be converted to Creature**!
        -- In the end, this function does not return T*. but std::remove_pointer<T>::type* and that
        -- is basically T* (if T is not a pointer) or T (if T is already a pointer).
        local cpp_child_name = member_type_to_cpp_type(child);
        if class.parent == class_name then
            cpp_output = cpp_output .. tab .. "if("..cpp_child_name.."::has(S, i)) {" .. br
            cpp_output = cpp_output .. tab .. tab .. "return &"..cpp_child_name.."::get( S, i );" .. br
            cpp_output = cpp_output .. tab .. "}" .. br
        end
    end
    cpp_output = cpp_output .. tab .. "(void)S; (void)i;" .. br -- just in case to prevent warnings
    cpp_output = cpp_output .. tab .. "return nullptr;" .. br
    cpp_output = cpp_output .. "}" .. br
    generate_functions_static(cpp_name, class, class_name)
    generate_read_members_static(cpp_name, class, class_name)
    generate_write_members_static(cpp_name, class, class_name)
end

-- Create the static constant members of LuaValue
for class_name, class in sorted_pairs(classes) do
    generate_LuaValue_constants(class_name, class, false)
    if class.by_value_and_reference then
        if class.by_value then
            error("by_value_and_reference only works with classes that have `by_value = false`")
        end
        generate_LuaValue_constants(class_name, class, true)
    end
end

-- Create a function that calls load_metatable on all the registered LuaValue's
cpp_output = cpp_output .. "static void load_metatables(lua_State* const L) {" .. br

for class_name, class in sorted_pairs(classes) do
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
for enum_name, values in sorted_pairs(enums) do
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

for name, func in sorted_pairs(global_functions) do
    cpp_output = cpp_output .. tab .. '{"'..name..'", global_'..name..'},'..br
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
