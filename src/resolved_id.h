#pragma once
#ifndef CATA_SRC_RESOLVED_ID_H
#define CATA_SRC_RESOLVED_ID_H

#include <functional>
#include <string>
#include <type_traits>

#include "string_id.h"
#include "int_id.h"

/**
 * Just like the @ref string_id and @red int_id, this is a wrapper for identifiers. The main difference
 * is that this represents an already resolved ID, valid or not, and as such is fast to access. This
 * requires addresses returned from int_id::obj to be stable.
 *
 * The template parameter T specifies what kind of object it identifies (e.g. a trap type, monster
 * type, ...)
 *
 * See `string_id` for documentation on usage.
 */
template<typename T>
class resolved_id
{
    public:
        /**
         * Default constructor constructs a 0-id. No id value is special to this class, 0 as id
         * is just as normal as any other integer value.
         */
        resolved_id() : resolved_id( 0 ) { }

        /**
         * Explicit constructor to make it stand out in the code, so one can easily search for all
         * places that use it.
         */
        explicit resolved_id( int id ) : resolved_id( int_id<T>( id ) ) { }

        /**
         * Construct a resolved id from the matching int based id.
         */
        // NOLINTNEXTLINE(google-explicit-constructor)
        resolved_id( const int_id<T> &id ) : ptr_( id.is_valid() ? & id.obj() : & invalid_obj() ) {}

        /**
         * Forwarding constructor, forwards any parameter to the string_id
         * constructor to create the resolved id. This allows plain C-strings,
         * and std::strings to be used.
         */
        template<typename S, class =
                 std::enable_if_t< std::is_convertible_v<S, std::string >> >
        explicit resolved_id( S && id ) : resolved_id( string_id<T>( std::forward<S>( id ) ) ) {}


        /**
         * Construct a resolved id from the matching string based id.
         */
        // NOLINTNEXTLINE(google-explicit-constructor)
        resolved_id( const string_id<T> &id ) : ptr_( id.is_valid() ? & id.obj() : & invalid_obj() ) {}

        /**
         * Comparison, only useful when the id is used in std::map or std::set as key.
         */
        bool operator<( const resolved_id &rhs ) const {
            return ptr_ < rhs.ptr_;
        }
        /**
         * The usual comparator, compares the resolved id as usual
         */
        bool operator==( const resolved_id &rhs ) const {
            return ptr_ == rhs.ptr_;
        }
        /**
         * The usual comparator, compares the resolved id as usual
         */
        bool operator!=( const resolved_id &rhs ) const {
            return ptr_ != rhs.ptr_;
        }

        explicit operator bool() const {
            return is_valid();
        }

        const T &obj() const {
            return *ptr_;
        }

        const T &operator*() const {
            return obj();
        }

        const T *operator->() const {
            return ptr_;
        }

        /**
         * Returns whether this id is valid, that means whether it refers to an existing object.
         */
        bool is_valid() const {
            return ptr_ != &invalid_obj();
        }

    private:
        // This must be defined per type in an appropriate location.
        const T &invalid_obj() const;

        const T *ptr_;
};

// Support hashing of resolved ids by forwarding the hash of the ptr.
template<typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<resolved_id<T>> {
    std::size_t operator()( const resolved_id<T> &v ) const noexcept {
        return hash<const T *>()( &v.obj() );
    }
};

#endif // CATA_SRC_RESOLVED_ID_H
