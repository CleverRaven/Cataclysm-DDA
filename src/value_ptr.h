#pragma once
#ifndef CATA_VALUE_PTR_H
#define CATA_VALUE_PTR_H

#include <memory>

namespace cata
{
/**
 * This class is essentially a copyable unique pointer. Its purpose is to allow
 * for sparse storage of data without wasting memory in classes such as itype.
 */
template <class T>
class value_ptr : public std::unique_ptr<T>
{
    public:
        value_ptr() = default;
        value_ptr( T *value ) : std::unique_ptr<T>( value ) {}
        value_ptr( const value_ptr<T> &other ) :
            std::unique_ptr<T>( other ? new T( *other ) : nullptr ) {}
        value_ptr &operator=( value_ptr<T> other ) {
            std::unique_ptr<T>::reset( other ? new T( *other ) : nullptr );
            return *this;
        }
};

template <class T, class... Args>
value_ptr<T> make_value( Args &&...args )
{
    return value_ptr<T>( new T( std::forward<Args>( args )... ) );
}

} // namespace cata

#endif // CATA_VALUE_PTR_H
