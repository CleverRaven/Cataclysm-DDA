#pragma once
#ifndef CATA_SRC_PIMPL_H
#define CATA_SRC_PIMPL_H

#include <memory>
#include <type_traits>

class JsonOut;
class JsonValue;

template<typename T>
class pimpl;
template<typename ...T>
class is_pimpl_helper : public std::false_type
{
};
template<typename T>
class is_pimpl_helper<pimpl<T>> : public std::true_type
{
};
template<typename T>
class is_pimpl : public is_pimpl_helper<std::decay_t<T>>
{
};
/**
 * This is a wrapper for implementing the pointer-to-implementation technique,
 * see for example http://en.cppreference.com/w/cpp/language/pimpl
 *
 * It's a container for one element of type @p T, which is *always* present
 * (one can not create a @ref pimpl instance without a contained @ref T instance).
 *
 * The contained @ref T can be access like a pointer via @ref operator-> and @ref operator.
 *
 * The constructor forwards all its arguments to the constructor of @ref T.
 */
template <typename T>
class pimpl : private std::unique_ptr<T>
{
    public:
        // Original second template arguments was `std::enable_if<std::is_constructible<T, Args>::value>`,
        // but this caused errors in few compilers when `T` was not a complete type.
        // The new argument serves the same purpose: this constructor should *not* be available when the
        // argument is a `pimpl` itself (the other copy constructors should be used instead).
        explicit pimpl() : std::unique_ptr<T>( new T() ) { }
        template < typename P, typename ...Args,
                   typename = std::enable_if_t < !is_pimpl<P>::value > >
        explicit pimpl( P && head, Args &&
                        ... args ) : std::unique_ptr<T>( new T( std::forward<P>( head ), std::forward<Args>( args )... ) ) { }

        pimpl( const pimpl<T> &rhs ) : std::unique_ptr<T>( new T( *rhs ) ) { }
        pimpl( pimpl<T> &&rhs ) noexcept : std::unique_ptr<T>( new T( std::move( *rhs ) ) ) { }

        pimpl<T> &operator=( const pimpl<T> &rhs ) {
            operator*() = *rhs;
            return *this;
        }
        pimpl<T> &operator=( pimpl<T> &&rhs ) noexcept {
            operator*() = std::move( *rhs );
            return *this;
        }

        bool operator==( const pimpl<T> &rhs ) const {
            return operator*() == *rhs;
        }

        using std::unique_ptr<T>::operator->;
        using std::unique_ptr<T>::operator*;

        /// Forwards the stream to `T::deserialize`.
        void deserialize( const JsonValue &stream ) {
            operator*().deserialize( stream );
        }
        /// Forwards the stream to `T::serialize`.
        void serialize( JsonOut &stream ) const {
            operator*().serialize( stream );
        }
};

#endif // CATA_SRC_PIMPL_H
