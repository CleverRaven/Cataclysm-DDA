#pragma once
#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <cassert>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>

namespace cata
{

class bad_optional_access : public std::logic_error
{
    public:
        bad_optional_access() : logic_error( "cata::optional: no value contained" ) { }
};

struct nullopt_t {
    explicit constexpr nullopt_t( int ) {}
};
static constexpr nullopt_t nullopt{ 0 };

struct in_place_t {
    explicit in_place_t() = default;
};
static constexpr in_place_t in_place{ };

template<typename T>
class optional
{
    private:
        using StoredType = typename std::remove_const<T>::type;
        union {
            char dummy;
            StoredType data;
        };
        bool full;

        T &get() {
            assert( full );
            return data;
        }
        const T &get() const {
            assert( full );
            return data;
        }

        template<typename... Args>
        void construct( Args &&... args ) {
            assert( !full );
            new( &data )StoredType( std::forward<Args>( args )... );
            full = true;
        }
        void destruct() {
            data.~StoredType();
        }

    public:
        constexpr optional() noexcept : dummy(), full( false ) { }
        constexpr optional( const nullopt_t ) noexcept : dummy(), full( false ) { }

        optional( const optional &other ) : full( false ) {
            if( other.full ) {
                construct( other.get() );
            }
        }
        optional( optional &&other ) : full( false ) {
            if( other.full ) {
                construct( std::move( other.get() ) );
            }
        }
        template<typename... Args>
        explicit optional( in_place_t, Args &&... args ) : data( std::forward<Args>( args )... ),
            full( true ) { }

        template<typename U, typename... Args>
        explicit optional( in_place_t, std::initializer_list<U> ilist,
                           Args &&... args ) : data( ilist,
                                       std::forward<Args>( args )... ), full( true ) { }

        template < typename U = T,
                   typename std::enable_if <
                       !std::is_same<optional<T>, typename std::decay<U>::type>::value &&
                       std::is_constructible < T, U && >::value &&
                       std::is_convertible < U &&, T >::value, bool >::type = true >
        optional( U && t )
            : optional( in_place, std::forward<U>( t ) ) { }

        template < typename U = T,
                   typename std::enable_if <
                       !std::is_same<optional<T>, std::decay<U>>::value &&
                       std::is_constructible < T, U && >::value &&
                       !std::is_convertible < U &&, T >::value, bool >::type = false >
        explicit optional( U && t )
            : optional( in_place, std::forward<U>( t ) ) { }

        ~optional() {
            reset();
        }

        constexpr const T *operator->() const {
            return &get();
        }
        T *operator->() {
            return &get();
        }
        constexpr const T &operator*() const {
            return get();
        }
        T &operator*() {
            return get();
        }

        constexpr explicit operator bool() const noexcept {
            return full;
        }
        constexpr bool has_value() const noexcept {
            return full;
        }

        T &value() {
            if( !full ) {
                throw bad_optional_access();
            }
            return get();
        }
        const T &value() const {
            if( !full ) {
                throw bad_optional_access();
            }
            return get();
        }

        template<typename O>
        T value_or( O &&other ) const {
            return full ? get() : static_cast<T>( other );
        }

        template<class... Args>
        T &emplace( Args &&... args ) {
            reset();
            construct( std::forward<Args>( args )... );
            return get();
        }
        template<class U, class... Args>
        T &emplace( std::initializer_list<U> ilist, Args &&... args ) {
            reset();
            construct( ilist, std::forward<Args>( args )... );
            return get();
        }

        void reset() noexcept {
            if( full ) {
                full = false;
                destruct();
            }
        }

        optional &operator=( nullopt_t ) noexcept {
            reset();
            return *this;
        }
        optional &operator=( const optional &other ) {
            if( full && other.full ) {
                get() = other.get();
            } else if( full ) {
                reset();
            } else if( other.full ) {
                construct( other.get() );
            }
            return *this;
        }
        optional &operator=( optional &&other ) {
            if( full && other.full ) {
                get() = std::move( other.get() );
            } else if( full ) {
                reset();
            } else if( other.full ) {
                construct( std::move( other.get() ) );
            }
            return *this;
        }
        template < class U = T,
                   typename std::enable_if <
                       !std::is_same<optional<T>, typename std::decay<U>::type>::value &&
                       std::is_constructible < T, U && >::value &&
                       std::is_convertible < U &&, T >::value, bool >::type = true >
        optional & operator=( U && value ) {
            if( full ) {
                get() =  std::forward<U>( value );
            } else {
                construct( std::forward<U>( value ) );
            }
            return *this;
        }
        template<class U>
        optional &operator=( const optional<U> &other ) {
            if( full && other.full ) {
                get() = other.get();
            } else if( full ) {
                reset();
            } else if( other.full ) {
                construct( other.get() );
            }
            return *this;
        }
        template<class U>
        optional &operator=( optional<U> &&other ) {
            if( full && other.full ) {
                get() = std::move( other.get() );
            } else if( full ) {
                reset();
            } else if( other.full ) {
                construct( std::move( other.get() ) );
            }
            return *this;
        }

        void swap( optional &other ) {
            using std::swap;

            if( full && other.full ) {
                swap( get(), other.get() );
            } else if( other.full() ) {
                construct( std::move( other.get() ) );
                other.destruct();
            } else if( full ) {
                other.construct( std::move( get() ) );
                destruct();
            }
        }
};

} // namespace cata

#endif
