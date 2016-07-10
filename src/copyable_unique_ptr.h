#ifndef COPYABLE_UNIQUE_PTR_H
#define COPYABLE_UNIQUE_PTR_H

#include <memory>

template <typename T>
class copyable_unique_ptr : public std::unique_ptr<T>
{
    public:
        copyable_unique_ptr() = default;
        // Move constructor missing parameter name intentionally,
        // Triggers warnings on some compilers.
        copyable_unique_ptr( copyable_unique_ptr && ) = default;

        copyable_unique_ptr( const copyable_unique_ptr<T> &rhs )
            : std::unique_ptr<T>( rhs ? new T( *rhs ) : nullptr ) {}

        copyable_unique_ptr &operator=( const copyable_unique_ptr<T> &rhs ) {
            std::unique_ptr<T>::reset( rhs ? new T( *rhs ) : nullptr );
            return *this;
        }
};

#endif
