File Layout
===========

- catalua.cpp - Core of the lua mod, glueing lua to the cataclysm C++ engine.
- catalua.h   - Export of some public lua-related functions, do not use these outside #ifdef LUA
- catalua/catalua.lua - Lua-side initialization of important data structures(metatables for classes etc.)
- catalua/generate_bindings.lua - Custom binding generator for cataclysm, can generate class and function bindings.
- catalua/catabindings.cpp - Output of generate_bindings.lua
- catalua/class_definitions.lua - Definitions of classes and functions that bindings will be generated from
- data/main.lua - Script that will be called on cataclysm startup. You can define functions here and call them in the lua debug interpreter.

Adding new functionality
========================

Generally, adding new functionality to lua is pretty straightforward. You have several options.

Manually defining a new lua function in catalua.cpp
---------------------------------------------------

This is the most straightforward, and also most difficult method. You basically define a new C function that does all the lua stack handling etc. manually, then register it in the lua function table. More information on this can be found at: http://www.lua.org/manual/5.1/manual.html#3

An example of such a function would be:

```c++
// items = game.items_at(x, y)
static int game_items_at(lua_State *L) {
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    std::vector<item>& items = g->m.i_at(x, y);

    lua_createtable(L, items.size(), 0); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    for(int i=0; i < items.size(); i++) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        item** item_userdata = (item**) lua_newuserdata(L, sizeof(item*));
        *item_userdata = &(items[i]);
        luah_setmetatable(L, "item_metatable");
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

static const struct luaL_Reg global_funcs [] = {
    {"register_iuse", game_register_iuse},
    {"items_at", game_items_at}, // Don't forget to register your function in here!
    {NULL, NULL}
};
```

Defining a global function in catalua.cpp and registering it in catalua/class_definitions.lua
---------------------------------------------------------------------------------------------

This method involves a bit more buerocracy, but is much easier to pull off and less prone to errors. Unless you wish to do something that the binding generator can't handle, this method is recommended for functions.

First, define your function however you like in catalua.cpp

```c++
// game.remove_item(x, y, item)
void game_remove_item(int x, int y, item *it) {
    std::vector<item>& items = g->m.i_at(x, y);

    for(int i=0; i<items.size(); i++) {
        if(&(items[i]) == it) {
            items.erase(items.begin() + i);
        }
    }
}
```

Then register your function as a global function in catalua/class_definitions.lua
```lua
global_functions = {
    [...]
    remove_item = {
        cpp_name = "game_remove_item",
        args = {"int", "int", "item"},
        rval = nil
    },
    [...]
}
```

`cpp_name` refers to the function that should be called, this can also be something more complex like `g->add_msg`(works because g is a global). `args` is simply a list of the argument types, most common C++ types should work, as well as any classes that are wrapped by lua. `rval` is similarly the type of the return value, and since C++ has no multi-return, rval is not a list.

Of note is the special argument type `game`. If that type is found, the binding generator will not wrap that parameter, but instead always pass `g` to that argument. So say you have a function foo(game* g, int a), then you should set `args = {"game", "int"}`, and from lua you would simply call foo(5), the game parameter being implicitly set to `g`.


Wrapping class member variables
-------------------------------

Wrapping member variables is simply a matter of adding the relevant entries to the class definition in catalua/class_definitions.lua.

```lua
classes = {
    player = {
        attributes = {
            posx = {
                type = "int",
                writable = false
            },
```

`type` works the same as for global functions, `writable` specifies whether a setter should be generated for this attribute.

Wrapping class member functions
-------------------------------

This process is nearly identical to wrapping global functions.

```lua
classes = {
    player = {
        [...]
        functions = {
            has_disease = {
                args = { "string" },
                rval = "bool"
            },
```

As you can see, the way functions are defined here is identical to global functions. The only difference is that there's an implicit first argument that will always be present, `self`, which enables us to call `player:has_disease("foo")`.


Adding new classes
------------------

To add a new wrapped class, you have to do several things:
- Add a new entry to `classes` in catalua/class_definitions.lua
- Add the relevant metatable to catalua/catalua.lua, e.g. `monster_metatable = generate_metatable("monster", classes.monster)`

Eventually 2) should be automated, but right now it's necessary. Note that the class name should be the exact same in lua as in C++, or otherwise the binding generator will fail. That limitation might be removed at some point.
