#ifndef H_GENERIC_FACTORY
#define H_GENERIC_FACTORY

#include "string_id.h"

#include <string>
#include <unordered_map>
#include <bitset>
#include <map>
#include <set>
#include <string>
#include <cassert>
#include <vector>

#include "debug.h"
#include "json.h"
#include "color.h"
#include "translations.h"

/**
A generic class to store objects identified by a `string_id`.

The class handles loading (including overriding / replacing existing objects) and
querying for specific objects. The class is designed to work with @ref string_id and
can be by it to implement its interface.

----

@tparam T The type of the managed objects. The type must have:
  - a default constructor,
  - a `load( JsonObject & )` function,
  - an `id` member of type `string_id<T>`,
  - a `was_loaded` member of type `bool`, which must have the value `false` before
    the first call to `load`.

  Those things must be visible from the factory, you may have to add this class as
  friend if necessary.

  `T::load` should load all the members of `T`, except `id` and `was_loaded` (they are
  set by the `generic_factory` before calling `load`). Failures should be reported by
  trowing an exception (e.g. via `JsonObject::throw_error`).

----

Usage:

- Create an instance, it can be static, or packed into another object.
- Implement `string_id::load` as simply forwarding to `generic_factory::load`.
  Register `string_id::load` in the @ref DynamicDataLoader (init.cpp) to be called when
  an object of the matching type is encountered.
- Similar: implement and register `string_id::reset` and let it call `generic_factory::reset`.

The functions can contain more code:
- `load` typically does nothing special beside forwarding to `generic_factory`.
- `reset` removes the loaded objects. It usually needs to remove the additional data that was set
  up in `finalize`. It must call `generic_factory::reset`.

Optional: implement the other functions used by the DynamicDataLoader: `finalize`,
`check_consistency`. There is no implementation of them in the generic factory.

`check_consistency` typically goes over all loaded items (@ref generic_factory::all) and checks
them somehow.

`finalize` typically populates some other data (e.g. some cache) or sets up connection between
loaded objects of different type.

A sample implementation looks like this:
\code
class my_class { ... }

namespace {
generic_factory<my_class> my_class_factory( "my class" );
std::map<...> some_cache;
}

template<>
void string_id<my_class>::load( JsonObject &jo ) {
    my_class_factory.load( jo );
}

template<>
void string_id<my_class>::reset() {
    some_cache.clear();
    my_class_factory.reset();
}

template<>
void string_id<my_class>::finalize() {
    for( auto &ptr : my_class_factory.all() )
        // populate a cache just as an example
        some_cache.insert( ... );
    }
}

// Implementation of the string_id functions:
template<>
const my_class &string_id<my_class>::obj() const
{
    return my_class_factory.obj( *this );
}
// ... more functions of string_id, similar to the above.

\endcode
*/
template<typename T>
class generic_factory
{
    protected:
        std::unordered_map< string_id<T>, T> data;

        std::string type_name;

        void load_override( const string_id<T> &id, JsonObject &jo ) {
            T obj;
            obj.id = id;
            obj.load( jo );
            obj.was_loaded = true;
            data[id] = std::move( obj );
        }

    public:
        /**
         * @param type_name A string used in debug messages as the name of `T`,
         * for example "vehicle type".
         */
        generic_factory( const std::string &type_name )
            : type_name( type_name ) {
        }
        /**
         * Load an object of type T with the data from the given JSON object.
         *
         * The id of the object is taken from the JSON object. The object data is loaded by
         * calling `T::load(jo)` (either on a new object or on an existing object).
         * See class documentation for intended behavior of that function.
         *
         * @throws JsonError If loading fails for any reason (thrown by `T::load`).
         */
        void load( JsonObject &jo ) {
            const string_id<T> id( jo.get_string( "id" ) );
            const auto iter = data.find( id );
            const bool exists = iter != data.end();

            // "create" is the default, so the game catches accidental re-definitions of
            // existing objects.
            const std::string mode = jo.get_string( "edit-mode", "create" );
            if( mode == "override" ) {
                load_override( id, jo );

            } else if( mode == "modify" ) {
                if( !exists ) {
                    jo.throw_error( "missing definition of " + type_name + " \"" + id.str() + "\" to be modified", "id" );
                }
                iter->second.load( jo );

            } else if( mode == "create" ) {
                if( exists ) {
                    jo.throw_error( "duplicated definition of " + type_name + " \"" + id.str() + "\"", "id" );
                }
                load_override( id, jo );

            } else {
                jo.throw_error( "invalid edit mode, must be \"create\", \"modify\" or \"override\"", "edit-mode" );
            }
        }
        /**
         * Add an object to the factory, without loading from JSON.
         * The new object replaces any existing object of the same id.
         */
        void insert( const T &obj ) {
            data[obj.id] = std::move( obj );
        }
        /**
         * Returns the number of loaded objects.
         */
        size_t size() const {
            return data.size();
        }
        /**
         * Removes all loaded objects.
         * Postcondition: `size() == 0`
         */
        void reset() {
            data.clear();
        }
        /**
         * Returns all the loaded objects. It can be used to iterate over them.
         * This returns a reference and is therefor quite fast, but you can also
         * use @ref get_all, which returns a copy of this data with raw pointers.
         * You should prefer `get_all` as it exposes a more stable interface.
         */
        const std::unordered_map<string_id<T>, T> &all_ref() const {
            return data;
        }
        std::vector<const T *> get_all() const {
            std::vector<const T *> result;
            result.reserve( data.size() );

            using pair = typename std::unordered_map<string_id<T>, T>::value_type;
            std::transform( data.begin(), data.end(), back_inserter( result ), []( const pair & p ) {
                return &p.second;
            } );

            return result;
        }
        /**
         * @name `string_id` interface functions
         *
         * The functions here are supposed to be used by the id classes, they have the
         * same behavior as described in the id classes and can be used directly by
         * forwarding the parameters to them and returning their result.
         */
        /**@{*/
        /**
         * Returns the object with the given id.
         * The input id should be valid, otherwise a debug message is issued.
         * This function can be used to implement @ref string_id::obj().
         * Note: If the id was valid, the returned object can be modified (after
         * casting the const away).
         */
        const T &obj( const string_id<T> &sid ) const {
            const auto iter = data.find( sid );
            if( iter == data.end() ) {
                debugmsg( "invalid %s id \"%s\"", type_name.c_str(), sid.c_str() );
                static const T dummy{};
                return dummy;
            }
            return iter->second;
        }
        /**
         * Checks whether the factory contains an object with the given id.
         * This function can be used to implement @ref string_id::is_valid().
         */
        bool is_valid( const string_id<T> &sid ) const {
            return data.count( sid ) > 0;
        }
        /**@}*/
};

/**
@file
Helper for loading from JSON

Loading (inside a `T::load(JsonObject &jo)` function) can be done with two functions
(defined here):
- `mandatory` loads required data and throws an error if the JSON data does not contain
  the required data.
- `optional` is for optional data, it has the same parameters and an additional default
  value that will be used if the JSON data does not contain the requested data. It may
  throw an error if the existing data is not valid (e.g. string instead of requested int).

The functions are designed to work with the `generic_factory` and therefor support the
`was_loaded` parameter (set be `generic_factory::load`). If that parameter is `true`, it
is assumed the object has already been loaded and missing JSON data is simply ignored
(the default value is not applied and no error is thrown upon missing mandatory data).

The parameters are this:
- `JsonObject jo` the object to load from.
- `bool was_loaded` whether the object had already been loaded completely.
- `std::string member_name` the name of the JSON member to load from.
- `T &member` a reference to the C++ object to store the loaded data.
- (for `optional`) a default value of any type that can be assigned to `member`.

Both functions use the native `read` functions of `JsonIn` (see there) to load the value.

Example:
\code
class Dummy {
    bool was_loaded = false;
    int a;
    std::string b;
    void load(JsonObject &jo) {
        mandatory(jo, was_loaded, "a", a);
        optional(jo, was_loaded, "b", b, "default value of b");
    }
};
\endcode

This only works if there is function with the matching type defined in `JsonIn`. For other
types, or if the loaded value needs to be converted (e.g. to `nc_color`), one can use the
reader classes/functions. `mandatory` and `optional` have an overload that requires the same
parameters and an additional reference to such a reader object.

\code
class Dummy2 {
    bool was_loaded = false;
    int b;
    nc_color c;
    void load(JsonObject &jo) {
        mandatory(jo, was_loaded, "b", b); // uses JsonIn::read(int&)
        mandatory(jo, was_loaded, "c", c, color_reader);
    }
};
\endcode

----

Readers must provide the following function:
`bool operator()( JsonObject &jo, const std::string &member_name, T &member, bool was_loaded ) const

(This can be implemented as free function or as operator in a class.)

The parameters are the same as the for the `mandatory` function (see above). The `function shall
return `true` if the loading was done, or `false` if the JSON data did
not contain the requested member. If loading fails because of invalid data (but not missing
data), it should throw.

*/

/** @name Implementation of `mandatory` and `optional`. */
/**@{*/
template<typename MemberType>
inline void mandatory( JsonObject &jo, const bool was_loaded, const std::string &name,
                       MemberType &member )
{
    if( !jo.read( name, member ) ) {
        if( !was_loaded ) {
            jo.throw_error( "missing mandatory member \"" + name + "\"" );
        }
    }
}
template<typename MemberType, typename ReaderType>
inline void mandatory( JsonObject &jo, const bool was_loaded, const std::string &name,
                       MemberType &member, const ReaderType &reader )
{
    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            jo.throw_error( "missing mandatory member \"" + name + "\"" );
        }
    }
}
template<typename MemberType, typename DefaultType = MemberType>
inline void optional( JsonObject &jo, const bool was_loaded, const std::string &name,
                      MemberType &member, const DefaultType &default_value )
{
    if( !jo.read( name, member ) ) {
        if( !was_loaded ) {
            member = default_value;
        }
    }
}
template<typename MemberType, typename ReaderType, typename DefaultType = MemberType>
inline void optional( JsonObject &jo, const bool was_loaded, const std::string &name,
                      MemberType &member, const ReaderType &reader, const DefaultType &default_value )
{
    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            member = default_value;
        }
    }
}
/**@}*/


/**
 * Reads a string from JSON and (if not empty) applies the translation function to it.
 */
inline bool translated_string_reader( JsonObject &jo, const std::string &member_name,
                                      std::string &member, bool )
{
    if( !jo.read( member_name, member ) ) {
        return false;
    }
    if( !member.empty() ) {
        member = _( member.c_str() );
    }
    return true;
}

/**
 * Reads a string and stores the first byte of it in `sym`. Throws if the input contains more
 * or less than one byte.
 */
inline bool one_char_symbol_reader( JsonObject &jo, const std::string &member_name, long &sym,
                                    bool )
{
    std::string sym_as_string;
    if( !jo.read( member_name, sym_as_string ) ) {
        return false;
    }
    if( sym_as_string.size() != 1 ) {
        jo.throw_error( member_name + " must be exactly one ASCII character", member_name );
    }
    sym = sym_as_string.front();
    return true;
}

/**
 * Base class for reading generic objects based on a single string.
 * It can load members being certain containers or being a single value.
 * @ref convert needs to be implemented to translate the JSON string to the flag type.
 *
 * - If the object is new (`was_loaded` is `false`), only the given JSON member is read
 *   and assigned, overriding any existing content of it.
 * - If the object is not new and the member exists, it is read and assigned as well.
 * - If the object is not new and the member does not exists, two further members are examined:
 *   entries from `"add:" + member_name` are added to the set and entries from `"remove:" + member_name`
 *   are removed. This only works if the member is actually a container, not just a single value.
 *
 * Example:
 * The JSON `{ "f": ["a","b","c"] }` would be loaded as the set `{"a","b","c"}`.
 * Loading the set again from the JSON `{ "remove:f": ["c","x"], "add:f": ["h"] }` would add the
 * "h" flag and removes the "c" and the "x" flag, resulting in `{"a","b","h"}`.
 *
 * @tparam F The type of the loaded object, e.g. a flag enum or a string_id or just a string.
 *           This type is the result of the conversion form the JSON string and this type is
 *           used when interacting with the member that is to be loaded.
 */
template<typename F>
class generic_typed_reader
{
    public:
        using FlagType = F;
        virtual ~generic_typed_reader() = default;
        /**
         * Does the conversion from string (as read from JSON) to the generic type.
         * `jo` and `member_name` should only be used to throw an exception.
         * The string that should be converted is already loaded from JSON: `data`.
         */
        virtual FlagType convert( const std::string &data, JsonObject &jo,
                                  const std::string &member_name ) const = 0;

        /**
         * Loads the set from JSON, similar to `JsonObject::get_tags`, but returns
         * a properly typed set.
         */
        std::set<FlagType> get_tags( JsonObject &jo, const std::string &member_name ) const {
            std::set<FlagType> result;
            for( auto && data : jo.get_tags( member_name ) ) {
                result.insert( convert( data, jo, member_name ) );
            }
            return result;
        }

        /**
         * Implements the reader interface, handles members that are containers of flags.
         * The functions forwards the actual changes to @ref assign, @ref insert
         * and @ref erase, which are specialized for various container types.
         * The `enable_if` is here to prevent the compiler from considering it
         * when called on a simple data member, the other `operator()` will be used.
         */
        template < typename C, typename = typename std::enable_if <
                       !std::is_same<FlagType, C>::value >::type >
        bool operator()( JsonObject &jo, const std::string &member_name,
                         C &container, bool was_loaded ) const {
            if( jo.has_member( member_name ) ) {
                assign( container, get_tags( jo, member_name ) );
                return true;
            } else if( !was_loaded ) {
                return false;
            } else {
                for( auto && data : get_tags( jo, "remove:" + member_name ) ) {
                    erase( container, data );
                }
                for( auto && data : get_tags( jo, "add:" + member_name ) ) {
                    insert( container, data );
                }
                return true;
            }
        }

        /**
         * Implements the reader interface, handles a simple data member.
         */
        // was_loaded is ignored here, if the value is not found in JSON, report to
        // the caller, which will take action on their own.
        bool operator()( JsonObject &jo, const std::string &member_name,
                         FlagType &member, bool /*was_loaded*/ ) const {
            if( !jo.has_member( member_name ) ) {
                return false;
            }
            member = convert( jo.get_string( member_name ), jo, member_name );
            return true;
        }

    private:
        /**@{*/
        void insert( std::set<FlagType> &container, const FlagType &data ) const {
            container.insert( data );
        }
        void erase( std::set<FlagType> &container, const FlagType &data ) const {
            container.erase( data );
        }
        void assign( std::set<FlagType> &container, std::set<FlagType> &&entries ) const {
            container = entries;
        }
        /**@}*/

        /**@{*/
        template<size_t N>
        void insert( std::bitset<N> &container, const FlagType &data ) const {
            assert( static_cast<size_t>( data ) < N );
            container.set( data );
        }
        template<size_t N>
        void erase( std::bitset<N> &container, const FlagType &data ) const {
            assert( static_cast<size_t>( data ) < N );
            container.reset( data );
        }
        template<size_t N>
        void assign( std::bitset<N> &container, std::set<FlagType> &&entries ) const {
            container.reset();
            for( auto && data : entries ) {
                container.set( data );
            }
        }
        /**@}*/

        /**@{*/
        void insert( std::vector<FlagType> &container, const FlagType &data ) const {
            container.push_back( data );
        }
        void erase( std::vector<FlagType> &container, const FlagType &data ) const {
            const auto iter = std::find( container.begin(), container.end(), data );
            if( iter != container.end() ) {
                container.erase( iter );
            }
        }
        void assign( std::vector<FlagType> &container, std::set<FlagType> &&entries ) const {
            container = std::vector<FlagType>( entries.begin(), entries.end() );
        }
        /**@}*/
};

/**
 * Converts the input string into a `nc_color`.
 */
class color_reader : public generic_typed_reader<nc_color>
{
    public:
        nc_color convert( const std::string &data, JsonObject &, const std::string & ) const override {
            // TODO: check for valid color name
            return color_from_string( data );
        }
};

/**
 * Converts the JSON string to some type that must be construable from a `std::string`,
 * e.g. @ref string_id.
 * Example:
 * \code
 *   std::set<string_id<Foo>> set;
 *   mandatory( jo, was_loaded, "set", set, auto_flags_reader<string_id<Foo>>{} );
 *   // It also works for containers of simple strings:
 *   std::set<std::string> set2;
 *   mandatory( jo, was_loaded, "set2", set2, auto_flags_reader<>{} );
 * \endcode
 */
template<typename FlagType = std::string>
class auto_flags_reader : public generic_typed_reader<FlagType>
{
    public:
        FlagType convert( const std::string &flag, JsonObject &, const std::string & ) const override {
            return FlagType( flag );
        }
};

/**
 * Uses a map (unordered or standard) to convert strings from JSON to some other type
 * (the mapped type of the map: `C::mapped_type`). It works for all mapped types, not just enums.
 *
 * One can use this if the member is `std::set<some_enum>` or `some_enum` and a
 * map `std::map<std::string, some_enum>` with all the value enumeration values exists.
 *
 * The class can be instantiated for a given map `mapping` like this:
 * `typed_flag_reader<decltype(mapping)> reader{ mapping, "error" };`
 * The error string (@ref error_msg) is used when the input contains invalid flags
 * (a string that is not contained in the map). It should sound something like
 * "invalid my-enum-type".
 */
template<typename C>
class typed_flag_reader : public generic_typed_reader<typename C::mapped_type>
{
    protected:
        const C &flag_map;
        const std::string error_msg;

    public:
        typed_flag_reader( const C &m, const std::string &e )
            : flag_map( m )
            , error_msg( e ) {
        }

        typename C::mapped_type convert( const std::string &flag, JsonObject &jo,
                                         const std::string &member_name ) const override {
            const auto iter = flag_map.find( flag );
            if( iter == flag_map.end() ) {
                jo.throw_error( error_msg + ": \"" + flag + "\"" , member_name );
            }
            return iter->second;
        }
};

/**
 * Uses @ref io::string_to_enum to convert the string from JSON to a C++ enum.
 */
template<typename E>
class enum_flags_reader : public generic_typed_reader<E>
{
    public:
        E convert( const std::string &flag, JsonObject &jo,
                   const std::string &member_name ) const override {
            try {
                return io::string_to_enum<E>( flag );
            } catch( const io::InvalidEnumString & ) {
                jo.throw_error( "invalid enumeration value: \"" + flag + "\"", member_name );
                throw; // ^^ throws already
            }
        }
};

#endif
