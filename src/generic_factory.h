#pragma once
#ifndef GENERIC_FACTORY_H
#define GENERIC_FACTORY_H

#include <algorithm>
#include <bitset>
#include <set>
#include <unordered_map>
#include <vector>

#include "assign.h"
#include "catacharset.h"
#include "debug.h"
#include "enum_bitset.h"
#include "init.h"
#include "int_id.h"
#include "json.h"
#include "output.h"
#include "string_id.h"
#include "translations.h"
#include "units.h"
#include "wcwidth.h"

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

  The type can also have:
  - a 'check()' function (to run `generic_factory::check()` on all objects)

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

`check_consistency` typically goes over all loaded items and checks them somehow.

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
class string_id_reader;

template<typename T>
class generic_factory
{

    public:
        virtual ~generic_factory() = default;

    private:
        DynamicDataLoader::deferred_json deferred;

    protected:
        std::vector<T> list;
        std::unordered_map<string_id<T>, int_id<T>> map;
        std::unordered_map<std::string, T> abstracts;

        std::string type_name;
        std::string id_member_name;
        std::string alias_member_name;

        bool find_id( const string_id<T> &id, int_id<T> &result ) const {
            result = id.get_cid();
            if( is_valid( result ) && list[result].id == id ) {
                return true;
            }
            const auto iter = map.find( id );
            if( iter == map.end() ) {
                return false;
            }
            result = iter->second;
            id.set_cid( result );
            return true;
        }

        void remove_aliases( const string_id<T> &id ) {
            int_id<T> i_id;
            if( !find_id( id, i_id ) ) {
                return;
            }
            auto iter = map.begin();
            const auto end = map.end();
            for( ; iter != end; ) {
                if( iter->second == i_id && iter->first != id ) {
                    map.erase( iter++ );
                } else {
                    ++iter;
                }
            }
        }

        const T dummy_obj;

    public:
        /**
         * @param type_name A string used in debug messages as the name of `T`,
         * for example "vehicle type".
         * @param id_member_name The name of the JSON member that contains the id of the
         * loaded object.
         * @param alias_member_name Alternate names of the JSON member that contains the id of the
         * loaded object.
         */
        generic_factory( const std::string &type_name, const std::string &id_member_name = "id",
                         const std::string &alias_member_name = "" )
            : type_name( type_name ),
              id_member_name( id_member_name ),
              alias_member_name( alias_member_name ),
              dummy_obj() {
        }

        /**
        * Perform JSON inheritance handling for `T def` and returns true if JsonObject is real.
        *
        * If the object contains a "copy-from" member the corresponding abstract gets copied if found.
        *    If abstract is not found, object is added to deferred.
        * If the object is abstract, it is loaded via `T::load` and added to `abstracts`
        *
        * @return true if `jo` is loaded and false if loading is deferred.
        * @throws JsonError If `jo` is both abstract and real. (contains "abstract" and "id" members)
        */
        bool handle_inheritance( T &def, JsonObject &jo, const std::string &src ) {
            static const std::string copy_from( "copy-from" );
            if( jo.has_string( copy_from ) ) {
                const std::string source = jo.get_string( copy_from );
                auto base = map.find( string_id<T>( source ) );

                if( base != map.end() ) {
                    def = obj( base->second );
                } else {
                    auto ab = abstracts.find( source );

                    if( ab != abstracts.end() ) {
                        def = ab->second;
                    } else {
                        def.was_loaded = false;
                        deferred.emplace_back( jo.str(), src );
                        return false;
                    }
                }
                def.was_loaded = true;
            }

            if( jo.has_string( "abstract" ) ) {
                if( jo.has_string( "id" ) ) {
                    jo.throw_error( "cannot specify both 'abstract' and 'id'" );
                }
                def.load( jo, src );
                abstracts[jo.get_string( "abstract" )] = def;
            }
            return true;
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
        void load( JsonObject &jo, const std::string &src ) {
            bool strict = src == "dda";

            T def;

            if( !handle_inheritance( def, jo, src ) ) {
                return;
            }
            if( jo.has_string( id_member_name ) ) {
                def.id = string_id<T>( jo.get_string( id_member_name ) );
                def.load( jo, src );
                insert( def );

                if( !alias_member_name.empty() && jo.has_member( alias_member_name ) ) {
                    std::set<string_id<T>> aliases;
                    assign( jo, alias_member_name, aliases, strict );

                    const int_id<T> ref = map[def.id];
                    for( const auto &e : aliases ) {
                        map[e] = ref;
                    }
                }

            } else if( !jo.has_string( "abstract" ) ) {
                jo.throw_error( "must specify either id or abstract" );
            }
        }
        /**
         * Add an object to the factory, without loading from JSON.
         * The new object replaces any existing object of the same id.
         * The function returns the actual object reference.
         */
        T &insert( const T &obj ) {
            const auto iter = map.find( obj.id );
            if( iter != map.end() ) {
                T &result = list[iter->second];
                result = obj;
                result.id.set_cid( iter->second );
                return result;
            }

            const int_id<T> cid( list.size() );
            list.push_back( obj );

            T &result = list.back();
            result.id.set_cid( cid );
            map[result.id] = cid;
            return result;
        }

        /** Finalize all entries (derived classes should chain to this method) */
        virtual void finalize() {
            DynamicDataLoader::get_instance().load_deferred( deferred );
            abstracts.clear();
        }

        /**
         * Checks loaded/inserted objects for consistency
         */
        void check() const {
            for( const T &obj : list ) {
                obj.check();
            }
        }
        /**
         * Returns the number of loaded objects.
         */
        size_t size() const {
            return list.size();
        }
        /**
         * Returns whether factory is empty.
         */
        bool empty() const {
            return list.empty();
        }
        /**
         * Removes all loaded objects.
         * Postcondition: `size() == 0`
         */
        void reset() {
            list.clear();
            map.clear();
        }
        /**
         * Returns all the loaded objects. It can be used to iterate over them.
         */
        const std::vector<T> &get_all() const {
            return list;
        }
        /**
         * @name `string_id/int_id` interface functions
         *
         * The functions here are supposed to be used by the id classes, they have the
         * same behavior as described in the id classes and can be used directly by
         * forwarding the parameters to them and returning their result.
         */
        /**@{*/
        /**
         * Returns the object with the given id.
         * The input id should be valid, otherwise a debug message is issued.
         * This function can be used to implement @ref int_id::obj().
         * Note: If the id was valid, the returned object can be modified (after
         * casting the const away).
         */
        const T &obj( const int_id<T> &id ) const {
            if( !is_valid( id ) ) {
                debugmsg( "invalid %s id \"%d\"", type_name, id.to_i() );
                return dummy_obj;
            }
            return list[id];
        }
        /**
         * Returns the object with the given id.
         * The input id should be valid, otherwise a debug message is issued.
         * This function can be used to implement @ref string_id::obj().
         * Note: If the id was valid, the returned object can be modified (after
         * casting the const away).
         */
        const T &obj( const string_id<T> &id ) const {
            int_id<T> i_id;
            if( !find_id( id, i_id ) ) {
                debugmsg( "invalid %s id \"%s\"", type_name, id.c_str() );
                return dummy_obj;
            }
            return list[i_id];
        }
        /**
         * Checks whether the factory contains an object with the given id.
         * This function can be used to implement @ref int_id::is_valid().
         */
        bool is_valid( const int_id<T> &id ) const {
            return static_cast<size_t>( id ) < list.size();
        }
        /**
         * Checks whether the factory contains an object with the given id.
         * This function can be used to implement @ref string_id::is_valid().
         */
        bool is_valid( const string_id<T> &id ) const {
            int_id<T> dummy;
            return find_id( id, dummy );
        }
        /**
         * Converts string_id<T> to int_id<T>. Returns null_id on failure.
         */
        int_id<T> convert( const string_id<T> &id, const int_id<T> &null_id ) const {
            int_id<T> result;
            if( find_id( id, result ) ) {
                return result;
            }
            debugmsg( "invalid %s id \"%s\"", type_name, id.c_str() );
            return null_id;
        }
        /**
         * Converts int_id<T> to string_id<T>. Returns null_id on failure.
         */
        const string_id<T> &convert( const int_id<T> &id ) const {
            return obj( id ).id;
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
    }
};
\endcode

Both versions of `optional` have yet another overload that does not require an explicit default
value, a default initialized object of the member type will be used instead.

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
            if( jo.has_member( name ) ) {
                jo.throw_error( "failed to read mandatory member \"" + name + "\"" );
            } else {
                jo.throw_error( "missing mandatory member \"" + name + "\"" );
            }
        }
    }
}
template<typename MemberType, typename ReaderType>
inline void mandatory( JsonObject &jo, const bool was_loaded, const std::string &name,
                       MemberType &member, const ReaderType &reader )
{
    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            if( jo.has_member( name ) ) {
                jo.throw_error( "failed to read mandatory member \"" + name + "\"" );
            } else {
                jo.throw_error( "missing mandatory member \"" + name + "\"" );
            }
        }
    }
}

template<typename MemberType>
inline void optional( JsonObject &jo, const bool was_loaded, const std::string &name,
                      MemberType &member )
{
    if( !jo.read( name, member ) ) {
        if( !was_loaded ) {
            member = MemberType();
        }
    }
}
/*
Template trickery, not for the faint of heart. It is required because there are two functions
with 5 parameters. The first 4 are always the same: JsonObject, bool, member name, member reference.
The last one is different: in one case it's the default value, in the other case it's the reader
and there is no explicit default value there.
The enable_if stuff assumes that a `MemberType` can not be constructed from a `ReaderType`, in other
words: `MemberType foo( ReaderType(...) );` does not work. This is what `is_constructible` checks.
If the 5. parameter can be used to construct a `MemberType`, it is assumed to be the default value,
otherwise it is assumed to be the reader.
*/
template<typename MemberType, typename DefaultType = MemberType,
         typename = typename std::enable_if<std::is_constructible<MemberType, const DefaultType &>::value>::type>
inline void optional( JsonObject &jo, const bool was_loaded, const std::string &name,
                      MemberType &member, const DefaultType &default_value )
{
    if( !jo.read( name, member ) ) {
        if( !was_loaded ) {
            member = default_value;
        }
    }
}
template < typename MemberType, typename ReaderType, typename DefaultType = MemberType,
           typename = typename std::enable_if <
               !std::is_constructible<MemberType, const ReaderType &>::value >::type >
inline void optional( JsonObject &jo, const bool was_loaded, const std::string &name,
                      MemberType &member, const ReaderType &reader )
{
    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            member = MemberType();
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
 * Reads a string and stores the first byte of it in `sym`. Throws if the input contains more
 * or less than one byte.
 */
inline bool one_char_symbol_reader( JsonObject &jo, const std::string &member_name, int &sym,
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
 * Reads a UTF-8 string (or int as legacy fallback) and stores Unicode codepoint of it in `symbol`.
 * Throws if the inputs width is more than one console cell wide.
 */
inline bool unicode_codepoint_from_symbol_reader( JsonObject &jo, const std::string &member_name,
        uint32_t &member, bool )
{
    int sym_as_int;
    std::string sym_as_string;
    if( !jo.read( member_name, sym_as_string, false ) ) {
        // Legacy fallback to integer `sym`.
        if( !jo.read( member_name, sym_as_int ) ) {
            return false;
        } else {
            sym_as_string = string_from_int( sym_as_int );
        }
    }
    uint32_t sym_as_codepoint = UTF8_getch( sym_as_string );
    if( mk_wcwidth( sym_as_codepoint ) != 1 ) {
        jo.throw_error( member_name + " must be exactly one console cell wide", member_name );
    }
    member = sym_as_codepoint;
    return true;
}

namespace reader_detail
{
template<typename T>
struct handler {
    static constexpr bool is_container = false;
};

template<typename T>
struct handler<std::set<T>> {
    void clear( std::set<T> &container ) const {
        container.clear();
    }
    void insert( std::set<T> &container, const T &data ) const {
        container.insert( data );
    }
    void erase( std::set<T> &container, const T &data ) const {
        container.erase( data );
    }
    static constexpr bool is_container = true;
};

template<size_t N>
struct handler<std::bitset<N>> {
    void clear( std::bitset<N> &container ) const {
        container.reset();
    }
    template<typename T>
    void insert( std::bitset<N> &container, const T &data ) const {
        container.set( data );
    }
    template<typename T>
    void erase( std::bitset<N> &container, const T &data ) const {
        container.reset( data );
    }
    static constexpr bool is_container = true;
};

template<typename E>
struct handler<enum_bitset<E>> {
    void clear( enum_bitset<E> &container ) const {
        container.reset();
    }
    template<typename T>
    void insert( enum_bitset<E> &container, const T &data ) const {
        container.set( data );
    }
    template<typename T>
    void erase( enum_bitset<E> &container, const T &data ) const {
        container.reset( data );
    }
    static constexpr bool is_container = true;
};

template<typename T>
struct handler<std::vector<T>> {
    void clear( std::vector<T> &container ) const {
        container.clear();
    }
    void insert( std::vector<T> &container, const T &data ) const {
        container.push_back( data );
    }
    template<typename E>
    void erase( std::vector<T> &container, const E &data ) const {
        erase_if( container, [&data]( const T & e ) {
            return e == data;
        } );
    }
    template<typename P>
    void erase_if( std::vector<T> &container, const P &predicate ) const {
        const auto iter = std::find_if( container.begin(), container.end(), predicate );
        if( iter != container.end() ) {
            container.erase( iter );
        }
    }
    static constexpr bool is_container = true;
};
} // namespace reader_detail

/**
 * Base class for reading generic objects from JSON.
 * It can load members being certain containers or being a single value.
 * The function get_next() needs to be implemented to read and convert the data from JSON.
 * It uses the curiously recurring template pattern, you have to derive your new class
 * `MyReader` from `generic_typed_reader<MyReader>` and implement `get_next` and
 * optionally `erase_next`.
 * Most function calls here are done on a `Derived`, which means it can "override" them.
 * This even allows changing their signature and return type.
 *
 * - If the object is new (`was_loaded` is `false`), only the given JSON member is read
 *   and assigned, overriding any existing content of it.
 * - If the object is not new and the member exists, it is read and assigned as well.
 * - If the object is not new and the member does not exists, two further members are examined:
 *   entries from `"extend"` are added to the set and entries from `"delete"`
 *   are removed. This only works if the member is actually a container, not just a single value.
 *
 * Example:
 * The JSON `{ "f": ["a","b","c"] }` would be loaded as the set `{"a","b","c"}`.
 * Loading the set again from the JSON `{ "delete": { "f": ["c","x"] }, "extend": { "f": ["h"] } }`
 * would add the "h" flag and removes the "c" and the "x" flag, resulting in `{"a","b","h"}`.
 *
 * @tparam Derived The class that inherits from this. It must implement the following:
 *   - `Foo get_next( JsonIn & ) const`: reads the next value from JSON, converts it into some
 *      type `Foo` and returns it. The returned value is assigned to the loaded member (see reader
 *      interface above), or is inserted into the member (if it's a container). The type `Foo` must
 *      be compatible with those uses (read: it should be the same type).
 *   - (optional) `erase_next( JsonIn &jin, C &container ) const`, the default implementation here
 *      reads a value from JSON via `get_next` and removes the matching value in the container.
 *      The value in the container must match *exactly*. You may override this function to allow
 *      a different matching algorithm, e.g. reading a simple id from JSON and remove entries with
 *      the same id from the container.
 */
template<typename Derived>
class generic_typed_reader
{
    public:
        template<typename C>
        void insert_values_from( JsonObject &jo, const std::string &member_name, C &container ) const {
            const Derived &derived = static_cast<const Derived &>( *this );
            if( !jo.has_member( member_name ) ) {
                return;
            }
            JsonIn &jin = *jo.get_raw( member_name );
            // We allow either a single value or an array of values. Note that this will not work
            // correctly if the thing we load from JSON is itself an array.
            if( jin.test_array() ) {
                jin.start_array();
                while( !jin.end_array() ) {
                    derived.insert_next( jin, container );
                }
            } else {
                derived.insert_next( jin, container );
            }
        }
        template<typename C>
        void insert_next( JsonIn &jin, C &container ) const {
            const Derived &derived = static_cast<const Derived &>( *this );
            reader_detail::handler<C>().insert( container, derived.get_next( jin ) );
        }

        template<typename C>
        void erase_values_from( JsonObject &jo, const std::string &member_name, C &container ) const {
            const Derived &derived = static_cast<const Derived &>( *this );
            if( !jo.has_member( member_name ) ) {
                return;
            }
            JsonIn &jin = *jo.get_raw( member_name );
            // Same as for inserting: either an array or a single value, same caveat applies.
            if( jin.test_array() ) {
                jin.start_array();
                while( !jin.end_array() ) {
                    derived.erase_next( jin, container );
                }
            } else {
                derived.erase_next( jin, container );
            }
        }
        template<typename C>
        void erase_next( JsonIn &jin, C &container ) const {
            const Derived &derived = static_cast<const Derived &>( *this );
            reader_detail::handler<C>().erase( container, derived.get_next( jin ) );
        }

        /**
         * Implements the reader interface, handles members that are containers of flags.
         * The functions forwards the actual changes to assign(), insert()
         * and erase(), which are specialized for various container types.
         * The `enable_if` is here to prevent the compiler from considering it
         * when called on a simple data member, the other `operator()` will be used.
         */
        template<typename C, typename std::enable_if<reader_detail::handler<C>::is_container, int>::type = 0>
        bool operator()( JsonObject &jo, const std::string &member_name,
                         C &container, bool was_loaded ) const {
            const Derived &derived = static_cast<const Derived &>( *this );
            // If you get an error about "incomplete type 'struct reader_detail::handler...",
            // you have to implement a specialization of your container type, so above for
            // existing specializations in namespace reader_detail.
            if( jo.has_member( member_name ) ) {
                reader_detail::handler<C>().clear( container );
                derived.insert_values_from( jo, member_name, container );
                return true;
            } else if( !was_loaded ) {
                return false;
            } else {
                if( jo.has_object( "extend" ) ) {
                    JsonObject tmp = jo.get_object( "extend" );
                    tmp.allow_omitted_members();
                    derived.insert_values_from( tmp, member_name, container );
                }
                if( jo.has_object( "delete" ) ) {
                    JsonObject tmp = jo.get_object( "delete" );
                    tmp.allow_omitted_members();
                    derived.erase_values_from( tmp, member_name, container );
                }
                return true;
            }
        }

        /**
         * Implements the reader interface, handles a simple data member.
         */
        // was_loaded is ignored here, if the value is not found in JSON, report to
        // the caller, which will take action on their own.
        template < typename C, typename std::enable_if < !reader_detail::handler<C>::is_container,
                   int >::type = 0 >
        bool operator()( JsonObject &jo, const std::string &member_name,
                         C &member, bool /*was_loaded*/ ) const {
            const Derived &derived = static_cast<const Derived &>( *this );
            if( !jo.has_member( member_name ) ) {
                return false;
            }
            member = derived.get_next( *jo.get_raw( member_name ) );
            return true;
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
class auto_flags_reader : public generic_typed_reader<auto_flags_reader<FlagType>>
{
    public:
        FlagType get_next( JsonIn &jin ) const {
            return FlagType( jin.get_string() );
        }
};

/**
 * Uses a map (unordered or standard) to convert strings from JSON to some other type
 * (the mapped type of the map: `C::mapped_type`). It works for all mapped types, not just enums.
 *
 * One can use this if the member is `std::set<some_enum>` or `some_enum` and a
 * map `std::map<std::string, some_enum>` with all the value enumeration values exists.
 *
 * The class can be conveniently instantiated for a given map `mapping` using
 * the helper function @ref make_flag_reader (see below).
 * The flag type (@ref flag_type) is used when the input contains invalid flags
 * (a string that is not contained in the map). It should sound something like
 * "my-enum-type".
 */
template<typename T>
class typed_flag_reader : public generic_typed_reader<typed_flag_reader<T>>
{
    private:
        using map_t = std::map<std::string, T>;

    private:
        const map_t &flag_map;
        const std::string flag_type;

    public:
        typed_flag_reader( const map_t &flag_map, const std::string &flag_type )
            : flag_map( flag_map )
            , flag_type( flag_type ) {
        }

        T get_next( JsonIn &jin ) const {
            const std::string flag = jin.get_string();
            const auto iter = flag_map.find( flag );

            if( iter == flag_map.cend() ) {
                jin.seek( jin.tell() );
                jin.error( string_format( "invalid %s: \"%s\"", flag_type, flag ) );
            }

            return iter->second;
        }
};

template<typename T>
typed_flag_reader<T> make_flag_reader( const std::map<std::string, T> &m, const std::string &e )
{
    return typed_flag_reader<T> { m, e };
}

/**
 * Uses @ref io::string_to_enum to convert the string from JSON to a C++ enum.
 */
template<typename E>
class enum_flags_reader : public generic_typed_reader<enum_flags_reader<E>>
{
    private:
        const std::string flag_type;

    public:
        enum_flags_reader( const std::string &flag_type ) : flag_type( flag_type ) {
        }

        E get_next( JsonIn &jin ) const {
            const auto position = jin.tell();
            const std::string flag = jin.get_string();
            try {
                return io::string_to_enum<E>( flag );
            } catch( const io::InvalidEnumString & ) {
                jin.seek( position );
                jin.error( string_format( "invalid %s: \"%s\"", flag_type, flag ) );
                throw; // ^^ throws already
            }
        }
};

/**
 * Loads string_id from JSON
 */
template<typename T>
class string_id_reader : public generic_typed_reader<string_id_reader<T>>
{
    public:
        string_id<T> get_next( JsonIn &jin ) const {
            return string_id<T>( jin.get_string() );
        }
};

/**
 * Reads a volume value from legacy format: JSON contains a integer which represents multiples
 * of `units::legacy_volume_factor` (250 ml).
 */
inline bool legacy_volume_reader( JsonObject &jo, const std::string &member_name,
                                  units::volume &value, bool )
{
    int legacy_value;
    if( !jo.read( member_name, legacy_value ) ) {
        return false;
    }
    value = legacy_value * units::legacy_volume_factor;
    return true;
}

#endif
