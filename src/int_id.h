#pragma once
#ifndef INT_ID_H
#define INT_ID_H

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
        typedef int_id<T> This;

        /**
         * Explicit constructor to make it stand out in the code, so one can easily search for all
         * places that use it.
         */
        explicit int_id( const int id )
            : _id( id ) {
        }
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
        operator int() const {
            return _id;
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
    std::size_t operator()( const int_id<T> &v ) const {
        return hash<int>()( v.to_i() );
    }
};
}

#endif
