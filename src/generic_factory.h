#pragma once
#ifndef CATA_SRC_GENERIC_FACTORY_H
#define CATA_SRC_GENERIC_FACTORY_H

#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cached_options.h"
#include "calendar.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "debug.h"
#include "demangle.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "init.h"
#include "int_id.h"
#include "mod_tracker.h"
#include "string_formatter.h"
#include "string_id.h"
#include "units.h"

class quantity;

template <typename E> class enum_bitset;

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
  throwing an exception (e.g. via `JsonObject::throw_error`).
  if `T::load`, for whatever reason, cannot report failures by throwing an expection and
  instead wishes to defer loading, it may have a boolean return type. Returning false will
  defer loading of this JSON.
  It is preferred that errors are reported and an exception is thrown.

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
void string_id<my_class>::load( const JsonObject &jo ) {
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
    public:
        virtual ~generic_factory() = default;

    private:
        DynamicDataLoader::deferred_json deferred;
        // generation or "modification count" of this factory
        // it's incremented when any changes to the inner id containers occur
        // version value corresponds to the string_id::_version,
        // so incrementing the version here effectively invalidates all cached string_id::_cid
        int64_t  version = 0;

        void inc_version() {
            do {
                version++;
            } while( version == INVALID_VERSION );
        }

    protected:
        std::vector<T> list;
        std::unordered_map<string_id<T>, int_id<T>> map;
        std::unordered_map<std::string, T> abstracts;

        std::string type_name;
        std::string id_member_name;

        bool find_id( const string_id<T> &id, int_id<T> &result ) const {
            if( id._version == version ) {
                result = int_id<T>( id._cid );
                return is_valid( result );
            }
            const auto iter = map.find( id );
            // map lookup happens at most once per string_id instance per generic_factory::version
            // id was not found, explicitly marking it as "invalid"
            if( iter == map.end() ) {
                id.set_cid_version( INVALID_CID, version );
                return false;
            }
            result = iter->second;
            id.set_cid_version( result.to_i(), version );
            return true;
        }

        const T dummy_obj;

    public:
        const bool initialized;
        /**
         * @param type_name A string used in debug messages as the name of `T`,
         * for example "vehicle type".
         * @param id_member_name The name of the JSON member that contains the id(s) of the
         * loaded object(s).
         */
        explicit generic_factory( const std::string &type_name, const std::string &id_member_name = "id" )
            : type_name( type_name ),
              id_member_name( id_member_name ),
              dummy_obj(),
              initialized( true ) {
        }

        // Begin template magic for T::handle_inheritance; if T has handle_inheritance function
        // that accepts the correct args - the inherited object and map of the abstracts then it
        // will be invoked to handle the copy-from, if not the assignment operator is used.
        // At time of writing this is used for vehicle parts (vpart_info class)
        // *INDENT-OFF* astyle turns templates unreadable
        template<typename U>
        using T_has_handle_inheritance_t = decltype(
            std::declval<U>().handle_inheritance(
                std::declval<const T &>(),
                std::declval<const std::unordered_map<std::string, T>&>() ) );

        template<typename U, typename=void>
        struct T_has_handle_inheritance : std::false_type {};
        template<typename U>
        struct T_has_handle_inheritance<U, std::void_t<T_has_handle_inheritance_t<U>>> : std::true_type {};
        template<typename U=T, std::enable_if_t<T_has_handle_inheritance<U>::value>* = nullptr>
        void handle_inheritance_on_T( T &def, const T &copy_from ) {
            def.handle_inheritance( copy_from, abstracts ); // let the function handle it
        }
        template<typename U=T, std::enable_if_t<!T_has_handle_inheritance<U>::value>* = nullptr>
        void handle_inheritance_on_T( T &def, const T &copy_from ) {
            def = copy_from; // just use assignment
        }
        // *INDENT-ON* astyle turns templates unreadable
        // End template magic for T::handle_inheritance

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
        bool handle_inheritance( T &def, const JsonObject &jo, const std::string &src ) {
            static const std::string copy_from_member_name( "copy-from" );
            static const std::string abstract_member_name( "abstract" );
            if( jo.has_string( copy_from_member_name ) ) {
                const std::string source = jo.get_string( copy_from_member_name );
                auto base = map.find( string_id<T>( source ) );

                if( base != map.end() ) {
                    const T &base_obj = obj( base->second );
                    handle_inheritance_on_T( def, base_obj );
                } else {
                    auto ab = abstracts.find( source );

                    if( ab != abstracts.end() ) {
                        const T &base_obj = ab->second;
                        handle_inheritance_on_T( def, base_obj );
                    } else {
                        def.was_loaded = false;
                        deferred.emplace_back( jo, src );
                        jo.allow_omitted_members();
                        return false;
                    }
                }
                def.was_loaded = true;
            }

            if( jo.has_string( abstract_member_name ) ) {
                if( jo.has_string( id_member_name ) ) {
                    jo.throw_error( string_format( "cannot specify both '%s' and '%s'",
                                                   abstract_member_name, id_member_name ) );
                }
                restore_on_out_of_scope restore_check_plural( check_plural );
                check_plural = check_plural_t::none;
                const std::string abstract_id =  jo.get_string( abstract_member_name );
                def.id = string_id<T>( abstract_id );
                def.load( jo, src );
                abstracts[abstract_id] = def;
            }
            return true;
        }

        template<typename LT, typename = std::void_t<>>
        struct load_is_bool : std::false_type { };

        template<typename LT>
        // astyle??
    struct load_is_bool<LT, std::void_t<std::is_same<decltype( std::declval<LT &>().load() ), bool>>> :
        std:: true_type {};

        /**
         * Load an object of type T with the data from the given JSON object.
         *
         * The id of the object is taken from the JSON object. The object data is loaded by
         * calling `T::load(jo)` (either on a new object or on an existing object).
         * See class documentation for intended behavior of that function.
         *
         * @throws JsonError If loading fails for any reason (thrown by `T::load`).
         *
         * The first function is for a load() function that returns a bool. This allows a type
         * to skip being inserted and instead defer loading.
         * The second is for load() functions that do not return a bool. This loads and inserts
         * the object
         */
        template<typename U = T, std::enable_if_t<load_is_bool<U>::value>* = nullptr>
        void load( const JsonObject &jo, const std::string &src ) {
            static const std::string abstract_member_name( "abstract" );

            T def;

            if( !handle_inheritance( def, jo, src ) ) {
                return;
            }
            if( jo.has_string( id_member_name ) ) {
                def.id = string_id<T>( jo.get_string( id_member_name ) );
                mod_tracker::assign_src( def, src );
                if( def.load( jo, src ) ) {
                    insert( def );
                } else {
                    def.was_loaded = false;
                    deferred.emplace_back( jo, src );
                    jo.allow_omitted_members();
                }

            } else if( jo.has_array( id_member_name ) ) {
                for( JsonValue e : jo.get_array( id_member_name ) ) {
                    T def;
                    if( !handle_inheritance( def, jo, src ) ) {
                        break;
                    }
                    def.id = string_id<T>( e );
                    mod_tracker::assign_src( def, src );
                    if( def.load( jo, src ) ) {
                        insert( def );
                    } else {
                        def.was_loaded = false;
                        deferred.emplace_back( jo, src );
                        jo.allow_omitted_members();
                    }
                }

            } else if( !jo.has_string( abstract_member_name ) ) {
                jo.throw_error( string_format( "must specify either '%s' or '%s'",
                                               abstract_member_name, id_member_name ) );
            }
        }
        // astyle???
        template < typename U = T, std::enable_if_t < !load_is_bool<T>::value > * = nullptr >
        void load( const JsonObject &jo, const std::string &src ) {
            static const std::string abstract_member_name( "abstract" );

            T def;

            if( !handle_inheritance( def, jo, src ) ) {
                return;
            }
            if( jo.has_string( id_member_name ) ) {
                def.id = string_id<T>( jo.get_string( id_member_name ) );
                mod_tracker::assign_src( def, src );
                def.load( jo, src );
                insert( def );

            } else if( jo.has_array( id_member_name ) ) {
                for( JsonValue e : jo.get_array( id_member_name ) ) {
                    T def;
                    if( !handle_inheritance( def, jo, src ) ) {
                        break;
                    }
                    def.id = string_id<T>( e );
                    mod_tracker::assign_src( def, src );
                    def.load( jo, src );
                    insert( def );
                }

            } else if( !jo.has_string( abstract_member_name ) ) {
                jo.throw_error( string_format( "must specify either '%s' or '%s'",
                                               abstract_member_name, id_member_name ) );
            }
        }
        /**
         * Add an object to the factory, without loading from JSON.
         * The new object replaces any existing object of the same id.
         * The function returns the actual object reference.
         */
        T &insert( const T &obj ) {
            // this invalidates `_cid` cache for all previously added string_ids,
            // but! it's necessary to invalidate cache for all possibly cached "missed" lookups
            // (lookups for not-yet-inserted elements)
            // in the common scenario there is no loss of performance, as `finalize` will make cache
            // for all ids valid again
            inc_version();
            const auto iter = map.find( obj.id );
            if( iter != map.end() ) {
                mod_tracker::check_duplicate_entries( obj, list[iter->second.to_i()] );
                T &result = list[iter->second.to_i()];
                result = obj;
                result.id.set_cid_version( iter->second.to_i(), version );
                return result;
            }

            const int_id<T> cid( list.size() );
            list.push_back( obj );

            T &result = list.back();
            result.id.set_cid_version( cid.to_i(), version );
            map[result.id] = cid;
            return result;
        }

        /** Finalize all entries (derived classes should chain to this method) */
        virtual void finalize() {
            DynamicDataLoader::get_instance().load_deferred( deferred );
            abstracts.clear();

            inc_version();
            for( size_t i = 0; i < list.size(); i++ ) {
                list[i].id.set_cid_version( static_cast<int>( i ), version );
            }
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
            /* Avoid unvisited member errors when iterating on json */
            for( std::pair<JsonObject, std::string> &deferred_json : deferred ) {
                deferred_json.first.allow_omitted_members();
            }
            deferred.clear();
            list.clear();
            map.clear();
            inc_version();
        }
        /**
         * Returns all the loaded objects. It can be used to iterate over them.
         */
        const std::vector<T> &get_all() const {
            return list;
        }
        /**
         * Returns all the loaded objects. It can be used to iterate over them.
         * Getting modifiable objects should be done sparingly!
         */
        std::vector<T> &get_all_mod() {
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
            return list[id.to_i()];
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
            return list[i_id.to_i()];
        }
        /**
         * Checks whether the factory contains an object with the given id.
         * This function can be used to implement @ref int_id::is_valid().
         */
        bool is_valid( const int_id<T> &id ) const {
            return static_cast<size_t>( id.to_i() ) < list.size();
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
         * When optional flag warn is true, issues a warning if `id` is not found and null_id was returned.
         */
        int_id<T> convert( const string_id<T> &id, const int_id<T> &null_id,
                           const bool warn = true ) const {
            int_id<T> result;
            if( find_id( id, result ) ) {
                return result;
            }
            if( warn ) {
                debugmsg( "invalid %s id \"%s\"", type_name, id.c_str() );
            }
            return null_id;
        }
        /**
         * Converts int_id<T> to string_id<T>. Returns null_id on failure.
         */
        const string_id<T> &convert( const int_id<T> &id ) const {
            return obj( id ).id;
        }
        /**@}*/

        /**
         * Wrapper around generic_factory::version.
         * Allows to have local caches that invalidate when corresponding generic factory invalidates.
         * Note: when created using it's default constructor, Version is guaranteed to be invalid.
        */
        class Version
        {
                friend generic_factory<T>;
            public:
                Version() = default;
            private:
                explicit Version( int64_t version ) : version( version ) {}
                int64_t  version = -1;
            public:
                bool operator==( const Version &rhs ) const {
                    return version == rhs.version;
                }
                bool operator!=( const Version &rhs ) const {
                    return !( rhs == *this );
                }
        };

        // current version of this generic_factory
        Version get_version() {
            return Version( version );
        }

        // checks whether given version is the same as current version of this generic_factory
        bool is_valid( const Version &v ) {
            return v.version == version;
        }
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

The functions are designed to work with the `generic_factory` and therefore support the
`was_loaded` parameter (set by `generic_factory::load`). If that parameter is `true`, it
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
`bool operator()( const JsonObject &jo, const std::string &member_name, T &member, bool was_loaded ) const

(This can be implemented as free function or as operator in a class.)

The parameters are the same as the for the `mandatory` function (see above). The `function shall
return `true` if the loading was done, or `false` if the JSON data did
not contain the requested member. If loading fails because of invalid data (but not missing
data), it should throw.

*/

/** @name Implementation of `mandatory` and `optional`. */
/**@{*/
template<typename MemberType>
inline void mandatory( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                       MemberType &member )
{
    if( !jo.read( name, member ) ) {
        if( !was_loaded ) {
            if( jo.has_member( name ) ) {
                jo.throw_error( str_cat( "object beginning at next line failed to read mandatory member \"", name,
                                         "\"" ) );
            } else {
                jo.throw_error( str_cat( "object beginning at next line missing mandatory member \"", name,
                                         "\"" ) );
            }
        }
    }
}
template<typename MemberType, typename ReaderType>
inline void mandatory( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                       MemberType &member, const ReaderType &reader )
{
    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            if( jo.has_member( name ) ) {
                jo.throw_error( str_cat( "object beginning at next line failed to read mandatory member \"", name,
                                         "\"" ) );
            } else {
                jo.throw_error( str_cat( "object beginning at next line missing mandatory member \"", name,
                                         "\"" ) );
            }
        }
    }
}

/*
 * Template vodoo:
 * The compiler will construct the appropriate one of these based on if the
 * type can support the operations being done.
 * So, it defaults to the false_type, but if it can use the *= operator
 * against a float OR defines member function handle_proportional (not both!),
 * it then supports proportional, and the handle_proportional
 * template that isn't just a dummy is constructed.
 * Similarly, if it can use a += operator against it's own type, the non-dummy
 * handle_relative template is constructed.
 */
template<typename T, typename = std::void_t<>>
struct supports_proportional : std::false_type { };

template<typename T>
struct supports_proportional<T, std::void_t<decltype( std::declval<T &>() *= std::declval<float>() )
>> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct supports_proportional_handler : std::false_type {};

template<typename T>
struct supports_proportional_handler<T, std::void_t<decltype( &T::handle_proportional )
>> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct supports_relative : std::false_type { };

template<typename T>
struct supports_relative < T, std::void_t < decltype( std::declval<T &>() += std::declval<T &>() )
>> : std::true_type {};

// Explicitly specialize these templates for a couple types
// So the compiler does not attempt to use a template that it should not
template<>
struct supports_proportional<bool> : std::false_type {};

template<>
struct supports_relative<bool> : std::false_type {};

template<>
struct supports_relative<std::string> : std::false_type {};

// This checks that all units:: types will support relative and proportional
static_assert( supports_relative<units::energy>::value, "units should support relative" );
static_assert( supports_proportional<units::energy>::value, "units should support proportional" );

static_assert( supports_relative<int>::value, "ints should support relative" );
static_assert( supports_proportional<int>::value, "ints should support proportional" );

static_assert( !supports_relative<bool>::value, "bools should not support relative" );
static_assert( !supports_proportional<bool>::value, "bools should not support proportional" );

// Using string ids with ints doesn't make sense in practice, but it doesn't matter here
// The type that it is templated with does not change its behavior
static_assert( !supports_relative<string_id<int>>::value,
               "string ids should not support relative" );
static_assert( !supports_proportional<string_id<int>>::value,
               "string ids should not support proportional" );

// Using int ids with ints doesn't make sense in practice, but it doesn't matter here
// The type that it is templated with does not change its behavior
static_assert( !supports_relative<int_id<int>>::value,
               "int ids should not support relative" );
static_assert( !supports_proportional<int_id<int>>::value,
               "int ids should not support proportional" );

static_assert( !supports_relative<std::string>::value, "strings should not support relative" );
static_assert( !supports_proportional<std::string>::value,
               "strings should not support proportional" );

// Grab an enum class from debug.h
static_assert( !supports_relative<DebugOutput>::value, "enum classes should not support relative" );
static_assert( !supports_proportional<DebugOutput>::value,
               "enum classes should not support proportional" );

// Grab a normal enum from there too
static_assert( !supports_relative<DebugLevel>::value, "enums should not support relative" );
static_assert( !supports_proportional<DebugLevel>::value, "enums should not support relative" );

// Dummy template:
// Warn if it's trying to use proportional where it cannot, but otherwise just
// return.
template < typename MemberType, std::enable_if_t < !supports_proportional<MemberType>::value &&
           !supports_proportional_handler<MemberType>::value > * = nullptr >
inline bool handle_proportional( const JsonObject &jo, const std::string_view name, MemberType & )
{
    if( jo.has_object( "proportional" ) ) {
        JsonObject proportional = jo.get_object( "proportional" );
        proportional.allow_omitted_members();
        if( proportional.has_member( name ) ) {
            debugmsg( "Member %s of type %s does not support proportional", name,
                      demangle( typeid( MemberType ).name() ) );
        }
    }
    return false;
}

// Real template:
// Copy-from makes it so the thing we're inheriting from is used to construct
// this, so member will contain the value of the thing we inherit from
// So, check if there is a proportional entry, check if it's got a valid value
// and if it does, multiply the member by it.
template<typename MemberType, std::enable_if_t<supports_proportional<MemberType>::value>* = nullptr>
inline bool handle_proportional( const JsonObject &jo, const std::string_view name,
                                 MemberType &member )
{
    if( jo.has_object( "proportional" ) ) {
        JsonObject proportional = jo.get_object( "proportional" );
        proportional.allow_omitted_members();
        // We need to check this here, otherwise we get problems with unvisited members
        if( !proportional.has_member( name ) ) {
            return false;
        }
        if( proportional.has_float( name ) ) {
            double scalar = proportional.get_float( name );
            if( scalar <= 0 || scalar == 1 ) {
                debugmsg( "Invalid scalar %g for %s", scalar, name );
                return false;
            }
            member *= scalar;
            return true;
        } else {
            jo.throw_error_at( name, str_cat( "Invalid scalar for ", name ) );
        }
    }
    return false;
}

//handles proportional for a class/struct with member function handle_proportional
template<typename MemberType, std::enable_if_t<supports_proportional_handler<MemberType>::value>* = nullptr>
inline bool handle_proportional( const JsonObject &jo, const std::string_view name,
                                 MemberType &member )
{
    if( jo.has_object( "proportional" ) ) {
        JsonObject proportional = jo.get_object( "proportional" );
        proportional.allow_omitted_members();
        // We need to check this here, otherwise we get problems with unvisited members
        if( !proportional.has_member( name ) ) {
            return false;
        }
        bool handled = member.handle_proportional( proportional.get_member( name ) );
        if( !handled ) {
            jo.throw_error_at( name, str_cat( "Invalid scalar for ", name ) );
        }
        return handled;
    }
    return false;
}

// Dummy template:
// Warn when trying to use relative when it's not supported, but otherwise,
// return
template < typename MemberType,
           std::enable_if_t < !supports_relative<MemberType>::value > * = nullptr
           >
inline bool handle_relative( const JsonObject &jo, const std::string_view name, MemberType & )
{
    if( jo.has_object( "relative" ) ) {
        JsonObject relative = jo.get_object( "relative" );
        relative.allow_omitted_members();
        if( !relative.has_member( name ) ) {
            return false;
        }
        debugmsg( "Member %s of type %s does not support relative", name,
                  demangle( typeid( MemberType ).name() ) );
    }
    return false;
}

// Real template:
// Copy-from makes it so the thing we're inheriting from is used to construct
// this, so member will contain the value of the thing we inherit from
// So, check if there is a relative entry, then add it to our member
template<typename MemberType, std::enable_if_t<supports_relative<MemberType>::value>* = nullptr>
inline bool handle_relative( const JsonObject &jo, const std::string_view name, MemberType &member )
{
    if( jo.has_object( "relative" ) ) {
        JsonObject relative = jo.get_object( "relative" );
        relative.allow_omitted_members();
        // This needs to happen here, otherwise we get unvisited members
        if( !relative.has_member( name ) ) {
            return false;
        }
        MemberType adder;
        if( relative.read( name, adder ) ) {
            member += adder;
            return true;
        } else {
            jo.throw_error_at( name, str_cat( "Invalid adder for ", name ) );
        }
    }
    return false;
}

// No template magic here, yay!
template<typename MemberType>
inline void optional( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                      MemberType &member )
{
    if( !jo.read( name, member ) && !handle_proportional( jo, name, member ) &&
        !handle_relative( jo, name, member ) ) {
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
         typename = std::enable_if_t<std::is_constructible_v<MemberType, const DefaultType &>>>
                                     inline void optional( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                                             MemberType &member, const DefaultType &default_value )
{
    if( !jo.read( name, member ) && !handle_proportional( jo, name, member ) &&
        !handle_relative( jo, name, member ) ) {
        if( !was_loaded ) {
            member = default_value;
        }
    }
}
template < typename MemberType, typename ReaderType, typename DefaultType = MemberType,
           typename = std::enable_if_t <
               !std::is_constructible_v<MemberType, const ReaderType &> > >
inline void optional( const JsonObject &jo, const bool was_loaded, const std::string_view name,
                      MemberType &member, const ReaderType &reader )
{
    if( !reader( jo, name, member, was_loaded ) ) {
        if( !was_loaded ) {
            member = MemberType();
        }
    }
}
template<typename MemberType, typename ReaderType, typename DefaultType = MemberType>
inline void optional( const JsonObject &jo, const bool was_loaded, const std::string_view name,
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
bool one_char_symbol_reader( const JsonObject &jo, std::string_view member_name, int &sym,
                             bool );

/**
 * Reads a UTF-8 string (or int as legacy fallback) and stores Unicode codepoint of it in `symbol`.
 * Throws if the inputs width is more than one console cell wide.
 */
bool unicode_codepoint_from_symbol_reader(
    const JsonObject &jo, std::string_view member_name, uint32_t &member, bool );

//Reads a standard single-float "proportional" entry
float read_proportional_entry( const JsonObject &jo, std::string_view key );

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

template<typename T>
struct handler<std::unordered_set<T>> {
    void clear( std::unordered_set<T> &container ) const {
        container.clear();
    }
    void insert( std::unordered_set<T> &container, const T &data ) const {
        container.insert( data );
    }
    void erase( std::unordered_set<T> &container, const T &data ) const {
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

template<typename Key, typename Val>
struct handler<std::map<Key, Val>> {
    void clear( std::map<Key, Val> &container ) const {
        container.clear();
    }
    void insert( std::map<Key, Val> &container, const std::pair<Key, Val> &data ) const {
        container.emplace( data );
    }
    void erase( std::map<Key, Val> &container, const std::pair<Key, Val> &data ) const {
        const auto iter = container.find( data.first );
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
 *   - `Foo get_next( V ) const`: reads the next value from JSON, converts it into some
 *      type `Foo` and returns it. The returned value is assigned to the loaded member (see reader
 *      interface above), or is inserted into the member (if it's a container). The type `Foo` must
 *      be compatible with those uses (read: it should be the same type). V is a type convertible from
 *      JsonValue.
 *   - (optional) `erase_next( V v, C &container ) const`, the default implementation here
 *      reads a value from JSON via `get_next` and removes the matching value in the container.
 *      The value in the container must match *exactly*. You may override this function to allow
 *      a different matching algorithm, e.g. reading a simple id from JSON and remove entries with
 *      the same id from the container. V is a value convertible from JsonValue.
 */
template<typename Derived>
class generic_typed_reader
{
public:
    template<typename C>
    void insert_values_from( const JsonObject &jo, const std::string_view member_name,
                             C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        if( !jo.has_member( member_name ) ) {
            return;
        }
        JsonValue jv = jo.get_member( member_name );
        // We allow either a single value or an array of values. Note that this will not work
        // correctly if the thing we load from JSON is itself an array.
        if( jv.test_array() ) {
            for( JsonValue jav : jv.get_array() ) {
                derived.insert_next( jav, container );
            }
        } else {
            derived.insert_next( jv, container );
        }
    }

    template<typename C>
    void insert_next( JsonValue &jv, C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        reader_detail::handler<C>().insert( container, derived.get_next( jv ) );
    }

    template<typename C>
    void erase_values_from( const JsonObject &jo, const std::string_view member_name,
                            C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        if( !jo.has_member( member_name ) ) {
            return;
        }
        JsonValue jv = jo.get_member( member_name );
        // Same as for inserting: either an array or a single value, same caveat applies.
        if( jv.test_array() ) {
            for( JsonValue jav : jv.get_array() ) {
                derived.erase_next( jav, container );
            }
        } else {
            derived.erase_next( jv, container );
        }
    }
    template<typename C>
    void erase_next( JsonValue &jv, C &container ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        reader_detail::handler<C>().erase( container, derived.get_next( jv ) );
    }

    /**
     * Implements the reader interface, handles members that are containers of flags.
     * The functions forwards the actual changes to assign(), insert()
     * and erase(), which are specialized for various container types.
     * The `enable_if` is here to prevent the compiler from considering it
     * when called on a simple data member, the other `operator()` will be used.
     */
    template<typename C, std::enable_if_t<reader_detail::handler<C>::is_container, int> = 0>
    bool operator()( const JsonObject &jo, const std::string_view member_name,
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

    /*
     * These two functions are effectively handle_relative but they need to
     * use the reader, so they must be here.
     * proportional does not need these, because it's only reading a float
     * whereas these are reading values of the same type.
     */
    // Type does not support relative
    template < typename C, std::enable_if_t < !reader_detail::handler<C>::is_container,
               int > = 0,
               std::enable_if_t < !supports_relative<C>::value > * = nullptr
               >
    bool do_relative( const JsonObject &jo, const std::string_view name, C & ) const {
        if( jo.has_object( "relative" ) ) {
            JsonObject relative = jo.get_object( "relative" );
            relative.allow_omitted_members();
            if( !relative.has_member( name ) ) {
                return false;
            }
            debugmsg( "Member %s of type %s does not support relative",
                      name, demangle( typeid( C ).name() ) );
        }
        return false;
    }

    // Type supports relative
    template < typename C, std::enable_if_t < !reader_detail::handler<C>::is_container,
               int > = 0, std::enable_if_t<supports_relative<C>::value> * = nullptr >
    bool do_relative( const JsonObject &jo, const std::string_view name, C &member ) const {
        if( jo.has_object( "relative" ) ) {
            JsonObject relative = jo.get_object( "relative" );
            relative.allow_omitted_members();
            const Derived &derived = static_cast<const Derived &>( *this );
            // This needs to happen here, otherwise we get unvisited members
            if( !relative.has_member( name ) ) {
                return false;
            }
            C adder = derived.get_next( relative.get_member( name ) );
            member += adder;
            return true;
        }
        return false;
    }

    template<typename C>
    bool read_normal( const JsonObject &jo, const std::string_view name, C &member ) const {
        if( jo.has_member( name ) ) {
            const Derived &derived = static_cast<const Derived &>( *this );
            member = derived.get_next( jo.get_member( name ) );
            return true;
        }
        return false;
    }

    /**
     * Implements the reader interface, handles a simple data member.
     */
    // was_loaded is ignored here, if the value is not found in JSON, report to
    // the caller, which will take action on their own.
    template < typename C, std::enable_if_t < !reader_detail::handler<C>::is_container,
               int > = 0 >
    bool operator()( const JsonObject &jo, const std::string_view member_name,
                     C &member, bool /*was_loaded*/ ) const {
        const Derived &derived = static_cast<const Derived &>( *this );
        return derived.read_normal( jo, member_name, member ) ||
               handle_proportional( jo, member_name, member ) || //not every reader uses proportional
               derived.do_relative( jo, member_name, member ); //readers can override relative handling
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
    FlagType get_next( std::string &&str ) const {
        return FlagType( std::move( str ) );
    }
};

using string_reader = auto_flags_reader<>;

class volume_reader : public generic_typed_reader<units::volume>
{
    public:
        bool operator()( const JsonObject &jo, const std::string_view member_name,
                         units::volume &member, bool /* was_loaded */ ) const {
            if( !jo.has_member( member_name ) ) {
                return false;
            }
            member = read_from_json_string<units::volume>( jo.get_member( member_name ), units::volume_units );
            return true;
        }
        units::volume get_next( JsonValue &jv ) const {
            return read_from_json_string<units::volume>( jv, units::volume_units );
        }
};

class mass_reader : public generic_typed_reader<units::mass>
{
    public:
        bool operator()( const JsonObject &jo, const std::string_view member_name,
                         units::mass &member, bool /* was_loaded */ ) const {
            if( !jo.has_member( member_name ) ) {
                return false;
            }
            member = read_from_json_string<units::mass>( jo.get_member( member_name ), units::mass_units );
            return true;
        }
        units::mass get_next( JsonValue &jv ) const {
            return read_from_json_string<units::mass>( jv, units::mass_units );
        }
};

class money_reader : public generic_typed_reader<units::money>
{
    public:
        bool operator()( const JsonObject &jo, const std::string_view member_name,
                         units::money &member, bool /* was_loaded */ ) const {
            if( !jo.has_member( member_name ) ) {
                return false;
            }
            member = read_from_json_string<units::money>( jo.get_member( member_name ), units::money_units );
            return true;
        }
        static units::money get_next( JsonValue &jv ) {
            return read_from_json_string<units::money>( jv, units::money_units );
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
    using map_t = std::unordered_map<std::string, T>;

    const map_t &flag_map;
    const std::string flag_type;

public:
    typed_flag_reader( const map_t &flag_map, const std::string_view flag_type )
        : flag_map( flag_map )
        , flag_type( flag_type ) {
    }

    explicit typed_flag_reader( const std::string_view flag_type )
        : flag_map( io::get_enum_lookup_map<T>() )
        , flag_type( flag_type ) {
    }

    T get_next( const JsonValue &jv ) const {
        const std::string flag = jv;
        const auto iter = flag_map.find( flag );

        if( iter == flag_map.cend() ) {
            jv.throw_error( string_format( "invalid %s: \"%s\"", flag_type, flag ) );
        }

        return iter->second;
    }
};

template<typename T>
typed_flag_reader<T> make_flag_reader( const std::unordered_map<std::string, T> &m,
                                       const std::string_view e )
{
    return typed_flag_reader<T>( m, e );
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
    explicit enum_flags_reader( const std::string &flag_type ) : flag_type( flag_type ) {
    }

    E get_next( const JsonValue &jv ) const {
        const std::string flag = jv.get_string();
        try {
            return io::string_to_enum<E>( flag );
        } catch( const io::InvalidEnumString & ) {
            jv.throw_error( string_format( "invalid %s: \"%s\"", flag_type, flag ) );
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
    string_id<T> get_next( std::string &&str ) const {
        return string_id<T>( std::move( str ) );
    }
};

/**
 * Loads std::pair of [K = string_id, V = int/float] values from JSON -- usually into an std::map
 * Accepted formats for elements in an array:
 * 1. A named key/value pair object: "addiction_type": [ { "addiction": "caffeine", "potential": 3 } ]
 * 2. A key/value pair array: "addiction_type": [ [ "caffeine", 3 ] ]
 * 3. A single value: "addiction_type": [ "caffeine" ]
 * A single value can also be provided outside of an array, e.g. "addiction_type": "caffeine"
 * For single values, weights are assigned default_weight
 */
template<typename K, typename V>
class weighted_string_id_reader : public generic_typed_reader<weighted_string_id_reader<K, V>>
{
public:
    V default_weight;
    explicit weighted_string_id_reader( V default_weight ) : default_weight( default_weight ) {};

    std::pair<K, V> get_next( const JsonValue &val ) const {
        if( val.test_object() ) {
            JsonObject inline_pair = val.get_object();
            if( !( inline_pair.size() == 1 || inline_pair.size() == 2 ) ) {
                inline_pair.throw_error( "weighted_string_id_reader failed to read object" );
            }
            K pair_key;
            V pair_val = default_weight;
            for( JsonMember mem : inline_pair ) {
                if( mem.test_string() ) {
                    pair_key = K( std::move( mem.get_string() ) );
                } else if( mem.test_float() ) {
                    pair_val = static_cast<V>( mem.get_float() );
                } else {
                    inline_pair.throw_error( "weighted_string_id_reader found unexpected value in object" );
                }
            }
            return std::pair<K, V>( pair_key, pair_val );
        } else if( val.test_array() ) {
            JsonArray arr = val.get_array();
            if( arr.size() != 2 ) {
                arr.throw_error( "weighted_string_id_reader read array without exactly two entries" );
            }
            return std::pair<K, V>(
                       K( std::move( arr[0].get_string() ) ),
                       static_cast<V>( arr[1].get_float() ) );
        } else {
            if( val.test_string() ) {
                return std::pair<K, V>(
                           K( std::move( val.get_string() ) ), default_weight );
            }
            val.throw_error( "weighted_string_id_reader provided with invalid string_id" );
        }
    }
};

template<typename T>
static T bound_check( T low, T high, const JsonValue &jv, T read_value )
{
    if( read_value < low ) {
        jv.throw_error( string_format( "value below bound_reader's defined low bound" ) );
        return low;
    } else if( read_value > high ) {
        jv.throw_error( string_format( "value above bound_reader's defined high bound" ) );
        return high;
    }
    return read_value;
}

template<typename T, typename = std::void_t<>>
struct supports_units : std::false_type {};

template<typename T>
struct supports_units < T, std::void_t < decltype( std::is_base_of<quantity, T>() )
                        >> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct supports_time : std::false_type {};

template<typename T>
struct supports_time < T, std::void_t < decltype( std::is_base_of<time_duration, T>() )
                                        >> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct supports_primitives : std::false_type {};

template<typename T>
struct supports_primitives < T, std::void_t < decltype( std::is_arithmetic_v<T> )
                             >> : std::true_type {};

/**
 * Throws an error if a single read value is outside those bounds,
 * then clamps that read value between low and, if included, high.
 *
 * bound_reader should not be used alone -- use an extending class
 *
 * TO-DO: handle proportional bounds?
 */
template <typename T>
class bound_reader : public generic_typed_reader<bound_reader<T>>
{
public:
    T low;
    T high;

    bound_reader() = default;
    //override relative handling from generic_typed_reader: check bounds for (original + relative) value
    bool do_relative( const JsonObject &jo, const std::string_view name, T &member ) const {
        if( jo.has_object( "relative" ) ) {
            JsonObject relative = jo.get_object( "relative" );
            relative.allow_omitted_members();
            // This needs to happen here, otherwise we get unvisited members
            if( !relative.has_member( name ) ) {
                return false;
            }
            JsonValue jv = relative.get_member( name );
            T adder = read( jv );
            member += adder;
            //note: passes copy of parameter and then assigns the result to the parameter
            member = bound_check( low, high, jv, member );
            return true;
        }
        return false;
    }

    //reads value as usual for type (i.e. without bound check)
    T read( const JsonValue &jv ) const {
        T read_member;
        if( !jv.read( read_member ) ) {
            jv.throw_error( "bound_reader failed to read value" );
        }
        return read_member;
    }

    T get_next( const JsonValue &jv ) const {
        return bound_check<T>( low, high, jv, read( jv ) );
    }
};

//bound_reader for primitive numbers
template<typename T, std::enable_if_t<supports_primitives<T>::value >* = nullptr >
class numeric_bound_reader : public bound_reader<T>
{
public:
    explicit numeric_bound_reader( T low = std::numeric_limits<T>::lowest(),
                                   T high = std::numeric_limits<T>::max() ) {
        bound_reader<T>::low = low;
        bound_reader<T>::high = high;
    };
};

//bound_reader for units:: namespace
template<typename T, std::enable_if_t<supports_units<T>::value >* = nullptr >
class units_bound_reader : public bound_reader<T>
{
public:
    explicit units_bound_reader( T low = T::min(), T high = T::max() ) {
        bound_reader<T>::low = low;
        bound_reader<T>::high = high;
    };
};

//bound_reader for time_duration, which is not a units::
template <typename T = time_duration, std::enable_if_t<supports_time<T>::value >* = nullptr >
class time_bound_reader : public bound_reader<T>
{
public:
    explicit time_bound_reader( T low = 0_seconds,
                                T high = calendar::INDEFINITELY_LONG_DURATION ) {
        bound_reader<T>::low = low;
        bound_reader<T>::high = high;
    };
};

/**
 * Only for external use in legacy code where migrating to `class translation`
 * is impractical. For new code load with `class translation` instead.
 */
class text_style_check_reader : public generic_typed_reader<text_style_check_reader>
{
    public:
        enum class allow_object : int {
            no,
            yes,
        };

        explicit text_style_check_reader( allow_object object_allowed = allow_object::yes );

        std::string get_next( const JsonValue &jv ) const;

    private:
        allow_object object_allowed;
};

#endif // CATA_SRC_GENERIC_FACTORY_H
