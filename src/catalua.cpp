#include "catalua.h"

#include <memory>

#include "game.h"
#include "player.h"
#include "action.h"
#include "item_factory.h"
#include "item.h"
#include "pldata.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "map.h"
#include "path_info.h"
#include "monstergenerator.h"
#include "messages.h"
#include "debug.h"
#include "translations.h"
#include "line.h"
#include "requirements.h"
#include "weather_gen.h"

#ifdef LUA
#include "ui.h"
#include "mongroup.h"
#include "itype.h"
#include "morale_types.h"
#include "trap.h"
#include "overmap.h"
#include "mtype.h"
#include "field.h"
#include "filesystem.h"
#include "string_input_popup.h"
#include "mutation.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include <type_traits>

#if LUA_VERSION_NUM < 502
#define LUA_OK 0
#endif

using item_stack_iterator = std::list<item>::iterator;
using volume = units::volume;

lua_State *lua_state = nullptr;

// Keep track of the current mod from which we are executing, so that
// we know where to load files from.
std::string lua_file_path = "";

std::stringstream lua_output_stream;
std::stringstream lua_error_stream;

#if LUA_VERSION_NUM < 502
// Compatibility, for before Lua 5.2, which does not have luaL_setfuncs
static void luaL_setfuncs( lua_State * const L, const luaL_Reg arrary[], int const nup )
{
    for( ; arrary->name != nullptr; arrary++ ) {
        lua_pushstring( L, arrary->name );
        // Need to copy the up-values because lua_pushcclosure removes them, they need
        // to be set for each C-function.
        for( int i = 0; i < nup; i++ ) {
            lua_pushvalue( L, -(nup + 1) );
        }
        lua_pushcclosure( L, arrary->func, nup );
        lua_settable( L, -(nup + 3) );
    }
    // Remove up-values as per definition of luaL_setfuncs in 5.2
    lua_pop( L, nup );
}
#endif

void lua_dofile(lua_State *L, const char *path);

// Helper functions for making working with the lua API more straightforward.
// --------------------------------------------------------------------------

// Stores item at the given stack position into the registry.
int luah_store_in_registry(lua_State *L, int stackpos)
{
    lua_pushvalue(L, stackpos);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

// Removes item from registry and pushes on the top of stack.
void luah_remove_from_registry(lua_State *L, int item_index)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, item_index);
    luaL_unref(L, LUA_REGISTRYINDEX, item_index);
}

// Sets the metatable for the element on top of the stack.
void luah_setmetatable(lua_State *L, const char *metatable_name)
{
    // Push the metatable on top of the stack.
    lua_getglobal(L, metatable_name);

    // The element we want to set the metatable for is now below the top.
    lua_setmetatable(L, -2);
}

void luah_setglobal(lua_State *L, const char *name, int index)
{
    lua_pushvalue(L, index);
    lua_setglobal(L, name);
}

/** Safe wrapper to get a Lua string as std::string. Handles nullptr and binary data. */
std::string lua_tostring_wrapper( lua_State* const L, int const stack_position )
{
    size_t length = 0;
    const char* const result = lua_tolstring( L, stack_position, &length );
    if( result == nullptr || length == 0 ) {
        return std::string{};
    }
    return std::string( result, length );
}

// Given a Lua return code and a file that it happened in, print a debugmsg with the error and path.
// Returns true if there was an error, false if there was no error at all.
bool lua_report_error( lua_State *L, int err, const char *path, bool simple = false ) {
    if( err == LUA_OK || err == LUA_ERRRUN ) {
        // No error or error message already shown via traceback function.
        return err != LUA_OK;
    }
    const std::string error = lua_tostring_wrapper( L, -1 );
    switch(err) {
        case LUA_ERRSYNTAX:
            if( !simple ) {
                lua_error_stream << "Lua returned syntax error for "  << path  << std::endl;
            }
            lua_error_stream << error;
            break;
        case LUA_ERRMEM:
            lua_error_stream << "Lua is out of memory";
            break;
        case LUA_ERRFILE:
            if( !simple ) {
                lua_error_stream << "Lua returned file io error for " << path << std::endl;
            }
            lua_error_stream << error;
            break;
        default:
            if( !simple ) {
                lua_error_stream << string_format( "Lua returned unknown error %d for ", err) << path << std::endl;
            }
            lua_error_stream << error;
            break;
    }
    return true;
}

/**
 * Base interface for values, that are copied into Luas own memory (and thereby managed by Lua).
 * The class creates a metatable for the wrapped objects, this is all set up via
 * generate_bindings.lua.
 * Usage: there are two main functions you might need: @ref push and @ref get.
 * - @ref push copies the object into Luas memory and pushes a reference to it on the stack.
 *   It is like @ref lua_pushnumber, only it pushes a whole object.
 * - @ref get reads a value from the stack and returns a reference to it (the memory of the object
 *   is managed by Lua and is kept until the garbage collector frees it).
 *
 * You can expect the following behavior:
 * \code
 * const Foo &myfoo = get_my_foo( ... );
 * LuaValue<Foo>::push( L, myfoo ); // copies myfoo
 * ... // give control back to Lua, wait for a callback from it,
 * Foo &thefoo = LuaValue<Foo>::get( L, 1 ); // get the first argument of the callback
 * thefoo.something(); // do something with it, not that myfoo and thefoo are different objects
 * \endcode
 *
 * @param T is the type of object that should be managed. It must be copy-constructable.
 */
template<typename T>
class LuaValue {
private:
    /** Defined by generate_bindings.lua in catabindings.cpp */
    static const char * const METATABLE_NAME;
    /** Defined by generate_bindings.lua in catabindings.cpp */
    static const luaL_Reg FUNCTIONS[];
    /** Defined by generate_bindings.lua in catabindings.cpp */
    using MRMap = std::map<std::string, int(*)(lua_State*)>;
    static const MRMap READ_MEMBERS;
    /** Defined by generate_bindings.lua in catabindings.cpp */
    using MWMap = std::map<std::string, int(*)(lua_State*)>;
    static const MWMap WRITE_MEMBERS;

    /*@{*/
    /**
     * @name Dynamic type conversion for Lua, or so.
     *
     * (Disclaimer: don't feel bad for not understanding this, I don't either.)
     *
     * get_subclass is a generated function, it checks whether the value at stack_index
     * is one of the direct subclasses of T (this check is recursive).
     * If one type matches, it returns a pointer to the object.
     *
     * Normally the function would be defined as `T* get_subclass(...)`, but if T is
     * already a pointer (e.g. Creature*), we end with `Creature** get_subclass`.
     * The problem is that a `monster**` can not be converted to `Creature**` and thereby
     * not be returned via this function (trust me, I tried, one may be able to use a
     * reinterpret_cast, but that is evil).
     * We need a simple pointer (not a pointer-to-pointer).
     *
     * We get the simple pointer by removing the pointer from T via the std thingy, which gives us
     * @ref Type. A reference to that is returned by @ref get.
     *
     * Reading user data from Lua gives a T*, so it must be converted to Type&, which may either
     * be just one dereferencing (if T*==Type*) or two (if T*==Type**).
     * One dereferencing is always done int @ref get, the (conditional) second is done in @ref cast.
     * The two overloads match either a Type* (which dereferences the parameter) or a Type&
     * (which just returns the reference without any changes).
     *
     * Maybe an example will help:
     * For T = monster* the function monster::die(Creature*) needs a Creature as parameter.
     * Try this in Lua: `game.zombie(0):die(player)`
     * Lua will check the parameter to be of type LuaValue<Creature*>, this means checking the
     * metatable (whether it's the very same as the one generated by LuaValue<Creature*>::get_metatable)
     * However, `player` has a different table (from LuaValue<player*>), so Lua would complain
     * about the wrong of the argument.
     * `LuaValue<Creature*>::get_subclass` is hard coded (via the code generator) to
     * check whether the value is actually a `Character*`, or a `monster*` (direct subclasses).
     * If so, a pointer the those objects (converted to `Creature*`) is returned. `cast()` will
     * simply pass that pointer through and the caller can return the `Creature*`.
     *
     * Now assume T = point (and assume tripoint inherit from point, why not?)
     * A function calls for a `point`, the input is a tripoint, so the metatables don't match.
     * `get_subclass` is called and successfully extract a tripoint from the userdata. It
     * returns a pointer to the tripoint (converted to `point*`). The caller needs a point (or
     * a point reference), so the `point*` must be converted back to a reference.
     * This is done by the first `cast()` overload.
     */
    using Type = typename std::remove_pointer<T>::type;
    static Type* get_subclass( lua_State* S, int stack_index );
    template<typename P>
    static Type& cast( P* ptr )
    {
        return *ptr;
    }
    template<typename P>
    static Type& cast( P& ptr )
    {
        return ptr;
    }
    /*@}*/

    static int gc( lua_State* const L )
    {
        T *object = static_cast<T*>( lua_touserdata( L, 1 ) );
        object->T::~T();
        lua_pop( L, 1 );
        return 0;
    }
    /**
     * Wrapper for the Lua __index entry in the metatable of the userdata.
     * It queries the actual metatable in case the call goes to a function (and does not request
     * and actual class member) and returns that function (if found).
     * If there is no function of the requested name, it looks up the name in @ref READ_MEMBERS,
     * if it's there, it calls the function that the entry refers to (which acts as a getter).
     * Finally it returns nil, which is what Lua would have used anyway.
     */
    static int index( lua_State * const L )
    {
        // -2 is the userdata, -1 is the key (funtion to call)
        const char * const key = lua_tostring( L, -1 );
        if( key == nullptr ) {
            luaL_error( L, "Invalid input to __index: key is not a string." );
        }
        if( luaL_getmetafield( L, -2, key ) != 0 ) {
            // There is an entry of that name, return it.
            lua_remove( L, -3 ); // remove userdata
            lua_remove( L, -2 ); // remove key
            // -1 is now the things we have gotten from luaL_getmetafield, return it.
            return 1;
        }
        const auto iter = READ_MEMBERS.find( key );
        if( iter == READ_MEMBERS.end() ) {
            // No such member or function
            lua_pushnil( L );
            return 1;
        }
        lua_remove( L, -1 ); // remove key
        // userdata is still there (now on -1, where it is expected by the getter)
        return iter->second( L );
    }
    /**
     * Wrapper for the Lua __newindex entry in the metatable of the userdata.
     * It looks up the name of the requested member in @ref WRITE_MEMBERS and (if found),
     * calls the function that the entry refers to (which acts as a setter).
     */
    static int newindex( lua_State * const L )
    {
        // -3 is the userdata, -2 is the key (name of the member), -1 is the value
        const char * const key = lua_tostring( L, -2 );
        if( key == nullptr ) {
            luaL_error( L, "Invalid input to __newindex: key is not a string." );
        }
        const auto iter = WRITE_MEMBERS.find( key );
        if( iter == WRITE_MEMBERS.end() ) {
            luaL_error( L, "Unknown attribute" );
        }
        lua_remove( L, -2 ); // key, userdata is still there, but now on -2, and the value is on -1
        return iter->second( L );
    }
    /**
     * This loads the metatable (and adds the available functions) and pushes it on the stack.
     */
    static void get_metatable( lua_State* const L )
    {
        // Create table (if it does not already exist), pushes it on the stack.
        // If the table already exists, we have already filled it, so we can return
        // without doing it again.
        if( luaL_newmetatable( L, METATABLE_NAME ) == 0 ) {
            return;
        }
        // Push the metatable itself, the stack now contains two pointers to the same metatable
        lua_pushvalue( L, -1 );
        // Set the metatable of the new metatable (-2 on the stack) to be itself. Pretty meta, hu?
        // It also pops one value from the stack.
        lua_setmetatable( L, -2 );
        // Now set the actual functions of the metatable.
        luaL_setfuncs( L, &FUNCTIONS[0], 0 );

        // Push function pointer
        lua_pushcfunction( L, &gc );
        // -1 would be the function pointer, -2 is the metatable, the function pointer is popped
        lua_setfield( L, -2, "__gc" );
        lua_pushcfunction( L, &index );
        lua_setfield( L, -2, "__index" );
        lua_pushcfunction( L, &newindex );
        lua_setfield( L, -2, "__newindex" );
    }
    /**
     * Checks the metatable that of value at stack_index against the metatable of this
     * object (matching type T). Returns the stack in the same state as it was when called.
     */
    static bool has_matching_metatable( lua_State* const L, int const stack_index )
    {
        if( lua_getmetatable( L, stack_index ) == 0 ) {
            // value does not have a metatable, can not be valid at all.
            return false;
        }
        get_metatable( L );
        const bool is_correct_metatable = lua_rawequal( L, -1, -2 );
        lua_remove( L, -1 );
        lua_remove( L, -1 );
        return is_correct_metatable;
    }

public:
    static void load_metatable( lua_State* const L, const char * const global_name )
    {
        // Create the metatable for the first time (or just retrieve it)
        get_metatable( L );
        if( global_name == nullptr ) {
            // remove the table from stack, setglobal does this in the other branch,
            // make it here manually to leave the stack in the same state.
            lua_remove( L, -1 );
        } else {
            lua_setglobal( L, global_name );
        }
    }
    template<typename ...Args>
    static void push( lua_State* const L, Args &&... args )
    {
        // Push user data,
        T* value_in_lua = static_cast<T*>( lua_newuserdata( L, sizeof( T ) ) );
        // Push metatable,
        get_metatable( L );
        // -1 would the the metatable, -2 is the uservalue, the table is popped
        lua_setmetatable( L, -2 );
        // This is where the copy happens:
        new (value_in_lua) T( std::forward<Args>( args )... );
    }
    static int push_reg( lua_State* const L, const T& value )
    {
        push( L, value );
        return luah_store_in_registry( L, -1 );
    }
    static Type& get( lua_State* const L, int const stack_index )
    {
        luaL_checktype( L, stack_index, LUA_TUSERDATA );
        T* user_data = static_cast<T*>( lua_touserdata( L, stack_index ) );
        if( user_data == nullptr ) {
            // luaL_error does not return at all.
            luaL_error( L, "First argument to function is not a class" );
        }
        if( has_matching_metatable( L, stack_index ) ) {
            return cast( *user_data );
        }
        Type* const subobject = get_subclass( L, stack_index );
        if( subobject == nullptr ) {
            // luaL_argerror does not return at all.
            luaL_argerror( L, stack_index, METATABLE_NAME );
        }
        return *subobject;
    }
    /** Checks whether the value at stack_index is of the type T. If so, @ref get can be used to get it. */
    static bool has( lua_State* const L, int const stack_index )
    {
        if( !lua_isuserdata( L, stack_index ) ) {
            return false;
        }
        if( has_matching_metatable( L, stack_index ) ) {
            return true;
        }
        return get_subclass( L, stack_index ) != nullptr;
    }
    /** Raises a Lua error if the type of the value at stack_index is not compatible with T. */
    static void check( lua_State* const L, int const stack_index )
    {
        luaL_checktype( L, stack_index, LUA_TUSERDATA );
        if( !has( L, stack_index ) ) {
            // METATABLE_NAME is used here as the name of the type we expect.
            luaL_argerror( L, stack_index, METATABLE_NAME );
        }
    }
};

/**
 * This is special wrapper (an extension) for references to objects which are not stored in Lua,
 * but are kept in the memory managed by C++. This class only stores and retrieves the pointers,
 * you have to make sure those pointers stay valid.
 *
 * Example (an @ref itype is loaded when a world is loaded and stays valid until the game ends):
 * \code
 * itype *it = type::find_type( "water" );
 * LuaReference<itype>::push( L, it ); // copies the pointer it
 * ... // give control back to Lua, wait for a callback from it,
 * itype &it = LuaValue<itype*>::get( L, 1 ); // get the first argument of the callback
 * assert(it.id == "water");
 * \endcode
 *
 * This class extends LuaValue by some pointer specific behavior:
 * - @ref push is overloaded to accept a reference to T (which will be converted to a pointer
 *   and stored). Additionally, if the pointer passed to @ref push is nullptr, nil will be pushed
 *   (this obviously does not work for references).
 *   \code
 *   Foo *x = ...;
 *   LuaReference<Foo>::push( L, x );
 *   LuaReference<Foo>::push( L, *x ); // both push calls do exactly the same.
 *   \endcode
 *   push is also overloaded to accept const and non-const references / pointers. The templated
 *   third parameter there makes sure that this is only done when T is not const. Otherwise we
 *   would end up with 2 identical push functions, both taking a const references.
 * - @ref get returns a proxy object. It contains the pointer to T. It will automatically convert
 *   to a reference / a pointer to T:
 *   \code
 *   void f_ptr( itype* );
 *   void f_ref( itype& );
 *   auto proxy = LuaReference<itype>::get( L, 1 );
 *   f_ptr( proxy ); // proxy converts to itype*
 *   f_ref( proxy ); // proxy converts to itype&
 *   itype *it = proxy;
 *   \endcode
 *   If you only need a reference (e.g. to call member functions or access members), use
 *   @ref LuaValue<T*>::get instead:
 *   \code
 *   itype &it = LuaValue<itype*>::get( L, 1 );
 *   std::string name = it.nname();
 *   \endcode
 */
template<typename T>
class LuaReference : private LuaValue<T*> {
public:
    template<typename U = T>
    static void push( lua_State* const L, T* const value, typename std::enable_if<!std::is_const<U>::value>::value_type* = nullptr )
    {
        if( value == nullptr ) {
            lua_pushnil( L );
            return;
        }
        LuaValue<T*>::push( L, value );
    }
    // HACK: because Lua does not known what const is.
    static void push( lua_State* const L, const T* const value )
    {
        if( value == nullptr ) {
            lua_pushnil( L );
            return;
        }
        LuaValue<T*>::push( L, const_cast<T*>( value ) );
    }
    template<typename U = T>
    static void push( lua_State* const L, T& value, typename std::enable_if<!std::is_const<U>::value>::value_type* = nullptr )
    {
        LuaValue<T*>::push( L, &value );
    }
    // HACK: because Lua does not known what const is.
    static void push( lua_State* const L, const T& value )
    {
        LuaValue<T*>::push( L, const_cast<T*>( &value ) );
    }
    static int push_reg( lua_State* const L, T* const value )
    {
        push( L, value );
        return luah_store_in_registry( L, -1 );
    }
    static int push_reg( lua_State* const L, T& value )
    {
        return LuaValue<T*>::push_reg( L, &value );
    }
    /** A proxy object that allows to convert the reference to a pointer on-demand. The proxy object can
     * be used as argument to functions that expect either a pointer and to functions expecting a
     * reference. */
    struct proxy {
        T *ref;
        operator T*() { return ref; }
        operator T&() { return *ref; }
        T* operator &() { return ref; }
    };
    /** Same as calling @ref get, but returns a @ref proxy containing the reference. */
    static proxy get( lua_State* const L, int const stack_position ) {
        return proxy{ &LuaValue<T*>::get( L, stack_position ) };
    }
    using LuaValue<T*>::has;
    using LuaValue<T*>::check;
};

/**
 * This is the basic type-checking interface for the Lua bindings generator.
 * Instead of "if type is string, call lua_isstring, if it's int, call lua_isnumber, ...", the
 * generator can just call "LuaType<"..type..">::has".
 * The C++ classes do the actual separation based on the type through the template parameter.
 *
 * Each implementation contains function like the LuaValue has:
 * - @ref has checks whether the object at given stack index is of the requested type.
 * - @ref check calls @ref has and issues a Lua error if the types is not as requested.
 * - @ref get returns the value at given stack_index. This is like @ref LuaValue::get.
 *   If you need to store the value, use \code auto && val = LuaType<X>::get(...); \endcode
 * - @ref push puts the value on the stack, like @ref LuaValue::push
 */
template<typename T>
struct LuaType;

template<>
struct LuaType<int> {
    static bool has( lua_State* const L, int const stack_index )
    {
        return lua_isnumber( L, stack_index );
    }
    static void check( lua_State* const L, int const stack_index )
    {
        luaL_checktype( L, stack_index, LUA_TNUMBER );
    }
    static int get( lua_State* const L, int const stack_index )
    {
        return lua_tonumber( L, stack_index );
    }
    static void push( lua_State* const L, int const value )
    {
        lua_pushnumber( L, value );
    }
};
template<>
struct LuaType<bool> {
    static bool has( lua_State* const L, int const stack_index )
    {
        return lua_isboolean( L, stack_index );
    }
    static void check( lua_State* const L, int const stack_index )
    {
        luaL_checktype( L, stack_index, LUA_TBOOLEAN );
    }
    static bool get( lua_State* const L, int const stack_index )
    {
        return lua_toboolean( L, stack_index );
    }
    static void push( lua_State* const L, bool const value )
    {
        lua_pushboolean( L, value );
    }
};
template<>
struct LuaType<std::string> {
    static bool has( lua_State* const L, int const stack_index )
    {
        return lua_isstring( L, stack_index );
    }
    static void check( lua_State* const L, int const stack_index )
    {
        luaL_checktype( L, stack_index, LUA_TSTRING );
    }
    static std::string get( lua_State* const L, int const stack_index )
    {
        return lua_tostring_wrapper( L, stack_index );
    }
    static void push( lua_State* const L, const std::string &value )
    {
        lua_pushlstring( L, value.c_str(), value.length() );
    }
    // For better performance: if the input is a c-string, forward it as such without wrapping
    // it into a std::string first.
    static void push( lua_State* const L, const char *value )
    {
        lua_pushstring( L, value );
    }
};
template<>
struct LuaType<float> : public LuaType<int> { // inherit checking because it's all the same to Lua
    static float get( lua_State* const L, int const stack_index )
    {
        return lua_tonumber( L, stack_index );
    }
    static void push( lua_State* const L, float const value )
    {
        lua_pushnumber( L, value );
    }
};
template<typename T>
struct LuaType<LuaValue<T>> : public LuaValue<T> {
};
template<typename T>
struct LuaType<LuaReference<T>> : public LuaReference<T> {
};

/** This basically transforms a string (therefor inheriting from LuaType<string>) into a C++
 * enumeration value. It simply contains a table of string-to-enum-values. */
template<typename E>
class LuaEnum : private LuaType<std::string> {
private:
    using Parent = LuaType<std::string>;
    /** Defined by generate_bindings.lua in catabindings.cpp */
    using EMap = std::map<std::string, E>;
    static const EMap BINDINGS;
    static E from_string( const std::string &value )
    {
        const auto iter = BINDINGS.find( value );
        if( iter == BINDINGS.end() ) {
            // This point shall not be reached. Always call this with valid input.
            return BINDINGS.begin()->second;
        }
        return iter->second;
    }
    static const std::string &to_string( E const value )
    {
        for( auto & e : BINDINGS ) {
            if( e.second == value ) {
                return e.first;
            }
        }
        // This point shall not be reached. Always call this with valid input.
        return BINDINGS.begin()->first;
    }
    static bool has( const std::string &value )
    {
        return BINDINGS.count( value ) > 0;
    }
    static int index( lua_State * const L )
    {
        // -1 is the key (funtion to call)
        const char * const key = lua_tostring( L, -1 );
        if( key == nullptr ) {
            luaL_error( L, "Invalid input to __index: key is not a string." );
        }
        const auto iter = BINDINGS.find( key );
        if( iter == BINDINGS.end() ) {
            return luaL_error( L, "Invalid enum value." );
        }
        lua_remove( L, -1 ); // remove key
        // Push the enum as string, it will be converted back to the enum later. This way, it can
        // be specified both ways in Lua code: either as string or via an entry here.
        lua_pushlstring( L, iter->first.c_str(), iter->first.length() );
        return 1;
    }
public:
    static bool has( lua_State* const L, int const stack_index )
    {
        return Parent::has( L, stack_index ) && has( Parent::get( L, stack_index ) );
    }
    static void check( lua_State* const L, int const stack_index )
    {
        Parent::check( L, stack_index );
        if( !has( Parent::get( L, stack_index ) ) ) {
            luaL_argerror( L, stack_index, "invalid value for enum" );
        }
    }
    static E get( lua_State* const L, int const stack_index )
    {
        return from_string( Parent::get( L, stack_index ) );
    }
    static void push( lua_State* const L, E const value )
    {
        Parent::push( L, to_string( value ) );
    }
    /** Export the enum values as entries of a global metatable */
    static void export_global( lua_State* const L, const char *global_name )
    {
        lua_createtable( L, 0, 1 ); // +1
        lua_pushvalue( L, -1 ); // + 1
        // Set the new table to have itself as metatable
        lua_setmetatable( L, -2 ); // -1
        // Setup the __index entry, which will translate the entry to a enum value
        lua_pushcfunction( L, &index ); // +1
        lua_setfield( L, -2, "__index" ); // -1
        // And register as a global value
        lua_setglobal( L, global_name ); // -1
    }
};
template<typename E>
struct LuaType<LuaEnum<E>> : public LuaEnum<E> {
};

/**
 * Wrapper class to access objects in Lua that are stored as either a pointer or a value.
 * Technically, this class could inherit from both `LuaValue<T>` and `LuaReference<T>`,
 * but that would basically the same code anyway.
 * It behaves like a LuaValue if there is a value on the stack, and like LuaReference is there
 * is a reference on the stack. Functions behave like the functions in a `LuaType`.
 * Note that it does not have a push function because it can not know whether to push a reference
 * or a value (copy). The caller must decide this and must use `LuaValue` or `LuaReference`.
 */
template<typename T>
class LuaValueOrReference {
    public:
        using proxy = typename LuaReference<T>::proxy;
        static proxy get( lua_State* const L, int const stack_index ) {
            if( LuaValue<T>::has( L, stack_index ) ) {
                return proxy{ &LuaValue<T>::get( L, stack_index ) };
            }
            return LuaReference<T>::get( L, stack_index );
        }
        static void check( lua_State* const L, int const stack_index ) {
            if( LuaValue<T>::has( L, stack_index ) ) {
                return;
            }
            LuaValue<T*>::check( L, stack_index );
        }
        static bool has( lua_State* const L, int const stack_index ) {
            return LuaValue<T>::has( L, stack_index ) || LuaValue<T*>::has( L, stack_index );
        }
};

void update_globals(lua_State *L)
{
    LuaReference<player>::push( L, g->u );
    luah_setglobal( L, "player", -1 );

    LuaReference<map>::push( L, g->m );
    luah_setglobal( L, "map", -1 );

    LuaReference<game>::push( L, g );
    luah_setglobal( L, "g", -1 );
}

class lua_iuse_wrapper : public iuse_actor {
private:
    int lua_function;
public:
    lua_iuse_wrapper( const int f, const std::string &type ) : iuse_actor( type ), lua_function( f ) {}
    ~lua_iuse_wrapper() override = default;
    long use( player *, item *it, bool a, const tripoint &pos ) const override {
        // We'll be using lua_state a lot!
        lua_State * const L = lua_state;

        // If it's a lua function, the arguments have to be wrapped in
        // lua userdata's and passed on the lua stack.
        // We will now call the function f(player, item, active)

        update_globals( L );

        // Push the lua function on top of the stack
        lua_rawgeti( L, LUA_REGISTRYINDEX, lua_function );

        // TODO: also pass the player object, because of NPCs and all
        //       I guess

        // Push the item on top of the stack.
        const int item_in_registry = LuaReference<item>::push_reg( L, it );
        // Push the "active" parameter on top of the stack.
        lua_pushboolean( L, a );
        // Push the location of the item.
        const int tripoint_in_registry = LuaValue<tripoint>::push_reg( L, pos );

        // Call the iuse function
        int err = lua_pcall( L, 3, 1, 0 );
        lua_report_error( L, err, "iuse function" );

        // Make sure the now outdated parameters we passed to lua aren't
        // being used anymore by setting a metatable that will error on
        // access.
        luah_remove_from_registry( L, item_in_registry );
        luah_setmetatable( L, "outdated_metatable");
        luah_remove_from_registry( L, tripoint_in_registry );
        luah_setmetatable( L, "outdated_metatable" );

        return lua_tointeger( L, -1 );
    }
    iuse_actor *clone() const override {
        return new lua_iuse_wrapper( *this );
    }

    void load( JsonObject & ) override {}
};

// iuse abstraction to make iuse's both in lua and C++ possible
// ------------------------------------------------------------
void Item_factory::register_iuse_lua(const std::string &name, int lua_function)
{
    if( iuse_function_list.count( name ) > 0 ) {
        DebugLog(D_INFO, D_MAIN) << "lua iuse function " << name << " overrides existing iuse function";
    }
    iuse_function_list[name] = use_function( new lua_iuse_wrapper( lua_function, name ) );
}

// Call the given string directly, used in the lua debug command.
int call_lua(std::string tocall)
{
    lua_State *L = lua_state;

    update_globals(L);
    int err = luaL_dostring(L, tocall.c_str());
    lua_report_error(L, err, tocall.c_str(), true);
    return err;
}

void lua_callback(const char *callback_name)
{
    call_lua(std::string("mod_callback(\"") + std::string(callback_name) + "\")");
}

//
int lua_mapgen(map *m, const oter_id &terrain_type, const mapgendata &, int t, float, const std::string &scr)
{
    if( lua_state == nullptr ) {
        return 0;
    }
    lua_State *L = lua_state;
    LuaReference<map>::push( L, m );
    luah_setglobal(L, "map", -1);

    int err = luaL_loadstring(L, scr.c_str() );
    if( lua_report_error( L, err, scr.c_str() ) ) {
        return err;
    }
    //    int function_index = luaL_ref(L, LUA_REGISTRYINDEX); // todo; make use of this
    //    lua_rawgeti(L, LUA_REGISTRYINDEX, function_index);

    lua_pushstring(L, terrain_type.id().c_str());
    lua_setglobal(L, "tertype");
    lua_pushinteger(L, t);
    lua_setglobal(L, "turn");

    err = lua_pcall(L, 0 , LUA_MULTRET, 0);
    lua_report_error( L, err, scr.c_str() );

    //    luah_remove_from_registry(L, function_index); // todo: make use of this

    return err;
}

// Custom functions that are to be wrapped from lua.
// -------------------------------------------------
static std::unique_ptr<uimenu> uimenu_instance;
uimenu *create_uimenu()
{
    uimenu_instance = std::unique_ptr<uimenu>(new uimenu());
    return uimenu_instance.get();
}

const ter_t &get_terrain_type(int id)
{
    return ter_id( id ).obj();
}

static calendar &get_calendar_turn_wrapper() {
    return calendar::turn;
}

static std::string string_input_popup_wrapper(std::string title, int width, std::string desc) {
    return string_input_popup().title(title).width(width).description(desc).query_string();
}

/** Create a new monster of the given type. */
monster *create_monster( const mtype_id &mon_type, const tripoint &p )
{
    monster new_monster( mon_type, p );
    if(!g->add_zombie(new_monster)) {
        return NULL;
    } else {
        return &(g->zombie(g->mon_at( p )));
    }
}

// Manually implemented lua functions
//
// Most lua functions are generated by src/lua/generate_bindings.lua,
// these generated functions can be found in src/lua/catabindings.cpp

/*
 This function is commented out until I find a way to get a list of all
 currently loaded monsters >_>

// monster_list = game.get_monsters()
static int game_get_monsters(lua_State *L) {
    lua_createtable(L, g->_z.size(), 0); // Preallocate enough space for all our monsters.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < g->_z.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing monsters
        // 2 - k, index at which the next monster will be inserted
        // 3 - v, next monster to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        monster** monster_userdata = (monster**) lua_newuserdata(L, sizeof(monster*));
        *monster_userdata = &(g->_z[i]);
        luah_setmetatable(L, "monster_metatable");
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}
*/

static void popup_wrapper(const std::string &text) {
    popup( "%s", text.c_str() );
}

static void add_msg_wrapper(const std::string &text) {
    add_msg( "%s", text.c_str() );
}

// items = game.items_at(x, y)
static int game_items_at(lua_State *L)
{
    int x = lua_tointeger(L, 1);
    int y = lua_tointeger(L, 2);

    auto items = g->m.i_at(x, y);
    lua_createtable(L, items.size(), 0); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    int i = 0;
    for( auto &an_item : items ) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i++ + 1);
        item **item_userdata = (item **) lua_newuserdata(L, sizeof(item *));
        *item_userdata = &an_item;
        // TODO: update using LuaReference<item>
        luah_setmetatable(L, "item_metatable");
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// item_groups = game.get_item_groups()
static int game_get_item_groups(lua_State *L)
{
    std::vector<std::string> items = item_controller->get_all_group_names();

    lua_createtable(L, items.size(), 0); // Preallocate enough space for all our items.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < items.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing item
        // 2 - k, index at which the next item will be inserted
        // 3 - v, next item to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        lua_pushstring(L, items[i].c_str());
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// monster_types = game.get_monster_types()
static int game_get_monster_types(lua_State *L)
{
    const auto mtypes = MonsterGenerator::generator().get_all_mtypes();

    lua_createtable(L, mtypes.size(), 0); // Preallocate enough space for all our monster types.

    // Iterate over the monster list and insert each monster into our returned table.
    for( size_t i = 0; i < mtypes.size(); ++i ) {
        // The stack will look like this:
        // 1 - t, table containing id
        // 2 - k, index at which the next id will be inserted
        // 3 - v, next id to insert
        //
        // lua_rawset then does t[k] = v and pops v and k from the stack

        lua_pushnumber(L, i + 1);
        LuaValue<mtype_id>::push( L, mtypes[i].id );
        lua_rawset(L, -3);
    }

    return 1; // 1 return values
}

// x, y = choose_adjacent(query_string, x, y)
static int game_choose_adjacent(lua_State *L)
{
    const std::string parameter1 = lua_tostring_wrapper( L, 1 );
    int parameter2 = (int) lua_tonumber(L, 2);
    int parameter3 = (int) lua_tonumber(L, 3);
    bool success = (bool) choose_adjacent(parameter1, parameter2, parameter3);
    if(success) {
        // parameter2 and parameter3 were updated by the call
        lua_pushnumber(L, parameter2);
        lua_pushnumber(L, parameter3);
        return 2; // 2 return values
    } else {
        return 0; // 0 return values
    }
}

// game.register_iuse(string, function_object)
static int game_register_iuse(lua_State *L)
{
    // Make sure the first argument is a string.
    const char *name = luaL_checkstring(L, 1);
    if(!name) {
        return luaL_error(L, "First argument to game.register_iuse is not a string.");
    }

    // Make sure the second argument is a function
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // function_object is at the top of the stack, so we can just pop
    // it with luaL_ref
    int function_index = luaL_ref(L, LUA_REGISTRYINDEX);

    // Now register function_object with our iuse's
    item_controller->register_iuse_lua(name, function_index);

    return 0; // 0 return values
}

#include "lua/catabindings.cpp"

// Load the main file of a mod
void lua_loadmod(std::string base_path, std::string main_file_name)
{
    std::string full_path = base_path + "/" + main_file_name;
    if( file_exist( full_path ) ) {
        lua_file_path = base_path;
        lua_dofile( lua_state, full_path.c_str() );
        lua_file_path = "";
    }
    // debugmsg("Loading from %s", full_path.c_str());
}

// Custom error handler
static int traceback(lua_State *L)
{
    // Get the error message
    const std::string error = lua_tostring_wrapper( L, -1 );

    // Get the lua stack trace
#if LUA_VERSION_NUM < 502
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    lua_getfield(L, -1, "traceback");
#else
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);
#endif
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 2);
    lua_call(L, 2, 1);

    const std::string stacktrace = lua_tostring_wrapper( L, -1 );

    // Print a debug message.
    debugmsg("Error in lua module: %s", error.c_str());

    // Print the stack trace to our debug log.
    DebugLog( D_ERROR, DC_ALL ) << stacktrace;
    return 1;
}

// Load an arbitrary lua file
void lua_dofile(lua_State *L, const char *path)
{
    lua_pushcfunction(L, &traceback);
    int err = luaL_loadfile(L, path);
    if( lua_report_error( L, err, path ) ) {
        return;
    }
    err = lua_pcall(L, 0, LUA_MULTRET, -2);
    lua_report_error( L, err, path );
}

// game.dofile(file)
//
// Method to load files from lua, later should be made "safe" by
// ensuring it's being loaded from a valid path etc.
static int game_dofile(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);

    std::string full_path = lua_file_path + "/" + path;
    lua_dofile(L, full_path.c_str());
    return 0;
}

static int game_myPrint( lua_State *L )
{
    int argc = lua_gettop( L );
    for( int i = argc; i > 0; i-- ) {
        lua_output_stream << lua_tostring_wrapper( L, -i );
    }
    lua_output_stream << std::endl;
    return 0;
}

// Registry containing all the game functions exported to lua.
// -----------------------------------------------------------
static const struct luaL_Reg global_funcs [] = {
    {"register_iuse", game_register_iuse},
    //{"get_monsters", game_get_monsters},
    {"items_at", game_items_at},
    {"choose_adjacent", game_choose_adjacent},
    {"dofile", game_dofile},
    {"get_monster_types", game_get_monster_types},
    {"get_item_groups", game_get_item_groups},
    {NULL, NULL}
};

// Lua initialization.
void game::init_lua()
{
    // This is called on each new-game, the old state (if any) is closed to dispose any data
    // introduced by mods of the previously loaded world.
    if( lua_state != nullptr ) {
        lua_close( lua_state );
    }
    lua_state = luaL_newstate();
    if( lua_state == nullptr ) {
        debugmsg( "Failed to start Lua. Lua scripting won't be available." );
        return;
    }

    luaL_openlibs(lua_state); // Load standard lua libs

    // Load our custom "game" module
#if LUA_VERSION_NUM < 502
    luaL_register(lua_state, "game", gamelib);
    luaL_register(lua_state, "game", global_funcs);
#else
    std::vector<luaL_Reg> lib_funcs;
    for( auto x = gamelib; x->name != nullptr; ++x ) {
        lib_funcs.push_back(*x);
    }
    for( auto x = global_funcs; x->name != nullptr; ++x ) {
        lib_funcs.push_back(*x);
    }
    lib_funcs.push_back( luaL_Reg { NULL, NULL } );
    luaL_newmetatable(lua_state, "game");
    lua_pushvalue(lua_state, -1);
    luaL_setfuncs(lua_state, &lib_funcs.front(), 0);
    lua_setglobal(lua_state, "game");
#endif

    load_metatables( lua_state );
    LuaEnum<body_part>::export_global( lua_state, "body_part" );

    // override default print to our version
    lua_register( lua_state, "print", game_myPrint );

    // Load lua-side metatables etc.
    lua_dofile(lua_state, FILENAMES["class_defslua"].c_str());
    lua_dofile(lua_state, FILENAMES["autoexeclua"].c_str());
}

#endif // #ifdef LUA

use_function::use_function( const use_function &other )
: actor( other.actor ? other.actor->clone() : nullptr )
{
}

use_function &use_function::operator=( iuse_actor * const f )
{
    return operator=( use_function( f ) );
}

use_function &use_function::operator=( const use_function &other )
{
    actor.reset( other.actor ? other.actor->clone() : nullptr );
    return *this;
}

void use_function::dump_info( const item &it, std::vector<iteminfo> &dump ) const
{
    if( actor != nullptr ) {
        actor->info( it, dump );
    }
}

long use_function::call( player *player_instance, item *item_instance, bool active, const tripoint &pos ) const
{
    if( !actor ) {
        if (player_instance != NULL && player_instance->is_player()) {
            add_msg(_("You can't do anything interesting with your %s."), item_instance->tname().c_str());
        }
        return 0;
    }
    return actor->use(player_instance, item_instance, active, pos);
}

#ifndef LUA
/* Empty functions for builds without Lua: */
int lua_monster_move( monster * )
{
    return 0;
}
int call_lua( std::string ) {
    popup( _( "This binary was not compiled with Lua support." ) );
    return 0;
}
// Implemented in mapgen.cpp:
// int lua_mapgen( map *, std::string, mapgendata, int, float, const std::string & )
void lua_callback( const char * )
{
}
void lua_loadmod( std::string, std::string )
{
}
void game::init_lua()
{
}
#endif
