#pragma once
#ifndef PIMPL_H
#define PIMPL_H

#include <memory>
#include <type_traits>

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
        template<typename ...Args, typename = typename std::enable_if<std::is_constructible<T, Args...>::value>::type>
        explicit pimpl( Args && ... args ) : std::unique_ptr<T>( new T( std::forward<Args>( args )... ) ) { }

        explicit pimpl( const pimpl<T> &rhs ) : std::unique_ptr<T>( new T( *rhs ) ) { }
        explicit pimpl( pimpl<T> &&rhs ) : std::unique_ptr<T>( new T( std::move( *rhs ) ) ) { }

        pimpl<T> &operator=( const pimpl<T> &rhs ) {
            operator*() = *rhs;
            return *this;
        }
        pimpl<T> &operator=( pimpl<T> &&rhs ) {
            operator*() = std::move( *rhs );
            return *this;
        }

        bool operator==( const pimpl<T> &rhs ) const {
            return operator*() == *rhs;
        }

        using std::unique_ptr<T>::operator->;
        using std::unique_ptr<T>::operator*;

        /// Forwards the stream to `T::deserialize`.
        template<typename JsonStream>
        void deserialize( JsonStream &stream ) {
            operator*().deserialize( stream );
        }
        /// Forwards the stream to `T::serialize`.
        template<typename JsonStream>
        void serialize( JsonStream &stream ) const {
            operator*().serialize( stream );
        }
};

#endif
