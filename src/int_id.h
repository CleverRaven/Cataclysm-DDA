#pragma once
#ifndef CATA_SRC_INT_ID_H
#define CATA_SRC_INT_ID_H

#include <functional>
#include <string>
#include <type_traits>

template<typename T>
class string_id;

/**
 * Just like the @ref string_id, this is a wrapper for int based identifiers.
 * The template parameter T specifies what kind of object it identifies (e.g. a trap type, monster
 * type, ...)
 *
 */
template<typename T>
class int_id
{
    public:
        using This = int_id<T>;

        /**
         * Explicit constructor to make it stand out in the code, so one can easily search for all
         * places that use it.
         */
        explicit int_id( const int id )
            : _id( id ) {
        }

        /**
         * Prevent accidental construction from other int ids.
         */
        template < typename S, typename std::enable_if_t < !std::is_same<S, T>::value, int > = 0 >
        int_id( const int_id<S> &id ) = delete;

        /**
         * Default constructor constructs a 0-id. No id value is special to this class, 0 as id
         * is just as normal as any other integer value.
         */
        int_id()
            : _id( 0 ) {
        }
        /**
         * Construct an id from the matching string based id. This may show a debug message if the
         * string id is invalid.
         */
        int_id( const string_id<T> &id );

        /**
         * Forwarding constructor, forwards any parameter to the string_id
         * constructor to create the int id. This allows plain C-strings,
         * and std::strings to be used.
         */
        template<typename S, class =
                 typename std::enable_if< std::is_convertible<S, std::string >::value>::type >
        explicit int_id( S && id ) : int_id( string_id<T>( std::forward<S>( id ) ) ) {}

        /**
         * Comparison, only useful when the id is used in std::map or std::set as key.
         */
        bool operator<( const This &rhs ) const {
            return _id < rhs._id;
        }
        /**
         * The usual comparator, compares the integer id as usual
         */
        bool operator==( const This &rhs ) const {
            return _id == rhs._id;
        }
        /**
         * The usual comparator, compares the integer id as usual
         */
        bool operator!=( const This &rhs ) const {
            return _id != rhs._id;
        }

        /**
         * Returns the identifier as plain int. Use with care, the plain int does not
         * have any information as what type of object it refers to (the T template parameter of
         * the class).
         */
        int to_i() const {
            return _id;
        }
        /**
         * Conversion to int as with the @ref to_i function.
         */
        explicit operator int() const {
            return _id;
        }

        explicit operator bool() const {
            return _id != 0;
        }

        // Those are optional, you need to implement them on your own if you want to use them.
        // If you don't implement them, but use them, you'll get a linker error.
        const string_id<T> &id() const;
        const T &obj() const;

        const T &operator*() const {
            return obj();
        }

        const T *operator->() const {
            return &obj();
        }

        /**
         * Returns whether this id is valid, that means whether it refers to an existing object.
         */
        bool is_valid() const;

    private:
        int _id;
};

// Support hashing of int based ids by forwarding the hash of the int.
namespace std
{
template<typename T>
struct hash< int_id<T> > {
    std::size_t operator()( const int_id<T> &v ) const noexcept {
        return hash<int>()( v.to_i() );
    }
};
} // namespace std

/**
 * A tiny fixed hashtable-based cache that speeds up `int_id::obj()` calls.
 * Exploits the following facts:
 * 1. same ids are usually clustered together (i.e. fields, furn, ter)
 * 2. cache is tiny -> less CPU cache misses
 * 3. if "obj" was stored in the cache, it's guaranteed to be valid
 * Intended to be used locally in situations when `int_id::obj()` is called in a tight loop.
 * Create the cache before the loop and then instead of `id.obj()` call `cache.obj(id)`.
 * @tparam T type of the object of the corresponding int_id
 */
template<typename T>
class int_id_cache
{
    private:
        static constexpr int LOCAL_OBJ_CACHE_SIZE = 16;
        int ids[LOCAL_OBJ_CACHE_SIZE] = { -1 };
        T const *objs[LOCAL_OBJ_CACHE_SIZE];

    public:
        const T &obj( const int_id<T> &id ) {
            int id_i = id.to_i();
            int i = id_i % LOCAL_OBJ_CACHE_SIZE;
            if( ids[i] == id_i ) {
                return *objs[i];
            } else {
                const T &obj = *id;
                objs[i] = &obj;
                ids[i] = id_i;
                return obj;
            }
        }
};

#endif // CATA_SRC_INT_ID_H
