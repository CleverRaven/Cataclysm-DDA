#pragma once
#ifndef CATA_SRC_INT_ID_H
#define CATA_SRC_INT_ID_H

#include <bitset>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_set>

template<typename T>
class string_id;

/**
 * Just like the @ref string_id, this is a wrapper for int based identifiers.
 * The template parameter T specifies what kind of object it identifies (e.g. a trap type, monster
 * type, ...)
 * See `string_id` for documentation on usage.
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
        template < typename S, typename std::enable_if_t < !std::is_same_v<S, T>, int > = 0 >
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
        // NOLINTNEXTLINE(google-explicit-constructor)
        int_id( const string_id<T> &id );

        /**
         * Forwarding constructor, forwards any parameter to the string_id
         * constructor to create the int id. This allows plain C-strings,
         * and std::strings to be used.
         */
        template<typename S, class =
                 std::enable_if_t< std::is_convertible_v<S, std::string >> >
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

// Fast set implementation for sequential int IDs. Only meant to be used with implementations based
// on generic_factory, i.e. non-negative IDs that are sequential from 0.
template <typename T, std::size_t kFastSize>
class int_id_set
{
    public:
        template <typename... Ts>
        void emplace( Ts &&... ts ) {
            insert( int_id<T>( std::forward<Ts>( ts )... ) );
        }

        void insert( int_id<T> id ) {
            const std::size_t i = static_cast<std::size_t>( id.to_i() );
            if( i < fast_set_.size() ) {
                fast_set_[i] = true;
            } else {
                slow_set_.insert( id );
            }
        }

        void erase( int_id<T> id ) {
            const std::size_t i = static_cast<std::size_t>( id.to_i() );
            if( i < fast_set_.size() ) {
                fast_set_[i] = false;
            } else {
                slow_set_.erase( id );
            }
        }

        bool contains( int_id<T> id ) const {
            const std::size_t i = static_cast<std::size_t>( id.to_i() );
            if( i < fast_set_.size() ) {
                return fast_set_[i];
            } else {
                return slow_set_.count( id );
            }
        }

        void clear() {
            fast_set_.reset();
            slow_set_.clear();
        }

    private:
        std::bitset<kFastSize> fast_set_;
        std::unordered_set<int_id<T>> slow_set_;
};

// Support hashing of int based ids by forwarding the hash of the int.
template<typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp)
struct std::hash<int_id<T>> {
    std::size_t operator()( const int_id<T> &v ) const noexcept {
        return hash<int>()( v.to_i() );
    }
};

#endif // CATA_SRC_INT_ID_H
