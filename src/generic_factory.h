#ifndef H_GENERIC_FACTORY
#define H_GENERIC_FACTORY

#include "string_id.h"

#include <string>
#include <unordered_map>

#include "debug.h"
#include "json.h"

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

  Those things must be visible from the factory, you may have to add this class as
  friend if necessary.

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
         * The id of the object is taken from the JSON object. If there is currently no object
         * with that id loaded, a new object of type `T` is created. Otherwise the existing
         * object is used.
         *
         * The object data is loaded by calling `T::load(jo)` (either on the new object or on the
         * existing object).
         *
         * @throws JsonError If loading fails for any reason (thrown by `T::load`).
         */
        void load( JsonObject &jo ) {
            const string_id<T> id( jo.get_string( "id" ) );

            const auto iter = data.find( id );
            const bool exists = iter != data.end();

            if( !exists ) {
                T obj;

                obj.id = id;
                obj.load( jo );

                data[id] = std::move( obj );
            } else {
                T &obj = iter->second;

                obj.load( jo );
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

#endif
