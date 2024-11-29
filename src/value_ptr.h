#pragma once
#ifndef CATA_SRC_VALUE_PTR_H
#define CATA_SRC_VALUE_PTR_H

#include <memory>

class JsonOut;
class JsonValue;

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
        value_ptr( value_ptr && ) noexcept = default;
        // NOLINTNEXTLINE(google-explicit-constructor)
        value_ptr( std::nullptr_t ) {}
        explicit value_ptr( T *value ) : std::unique_ptr<T>( value ) {}
        value_ptr( const value_ptr<T> &other ) :
            std::unique_ptr<T>( other ? new T( *other ) : nullptr ) {}
        value_ptr &operator=( value_ptr<T> other ) noexcept {
            std::unique_ptr<T>::operator=( std::move( other ) );
            return *this;
        }

        template<typename Stream = JsonOut>
        void serialize( Stream &jsout ) const {
            if( this->get() ) {
                this->get()->serialize( jsout );
            } else {
                jsout.write_null();
            }
        }
        template<typename Value = JsonValue, std::enable_if_t<std::is_same_v<std::decay_t<Value>, JsonValue>>* = nullptr>
        void deserialize( const Value &jsin ) {
            if( jsin.test_null() ) {
                this->reset();
            } else {
                this->reset( new T() );
                this->get()->deserialize( jsin );
            }
        }
};

template <class T, class... Args>
value_ptr<T> make_value( Args &&...args )
{
    return value_ptr<T>( new T( std::forward<Args>( args )... ) );
}

template <class T>
bool value_ptr_equals( const value_ptr<T> &lhs, const value_ptr<T> &rhs )
{
    return ( !lhs && !rhs ) || ( lhs && rhs && *lhs == *rhs );
}

/**
 * This class is essentially a copyable unique pointer, like value_ptr, except
 * it hides the fact it is a unique_ptr. It is intended for helping make types
 * noexcept movable by moving non-noexcept-movable types to the heap.
 *
 * Contractually speaking, should never be empty. However for performance reasons
 * in eg. item, moved-from heap<> objects do not automatically reset with a new
 * copy of the wrapped data.
 */
template <class T>
struct heap {
    private:
        std::unique_ptr<T> heaped_;

    public:
        template<typename ...Args>
        // NOLINTNEXTLINE(google-explicit-constructor)
        heap( Args &&...args ) : heaped_{ new T{ std::forward<Args>( args )... } } {}

        // Unlike value_ptr, moves actually move and leave the moved-from heap empty.
        // Using it again will cause a segfault without resetting it first.
        heap( heap && ) noexcept = default;
        heap &operator=( heap &&other ) noexcept = default;

        // Like value_ptr, copies copy the rhs and leave it intact.
        heap( heap const &rhs ) {
            *this = rhs;
        }
        heap &operator=( heap const &rhs ) {
            // This should always be true, however, sanity says do the right thing.
            if( rhs.heaped_ ) {
                heaped_.reset( new T{ *rhs.heaped_ } );
            } else {
                heaped_.reset( new T{} );
            }
            return *this;
        }

        heap &operator=( T const &t ) {
            heaped_.reset( new T{ t } );
            return *this;
        }

        heap &operator=( T &&t ) {
            heaped_.reset( new T{ std::move( t ) } );
            return *this;
        }

        // Implicit conversion functions
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator T &() & { // *NOPAD*
            return *heaped_;
        }
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator T const &() const & { // *NOPAD*
            return *heaped_;
        }
        // Intentionally move construct a value T to avoid binding a ref to a temporary.
        // NOLINTNEXTLINE(google-explicit-constructor)
        operator T() && { // *NOPAD*
            return std::move( *heaped_ );
        }

        // The one weird one: since this is ultimately backed on the heap,
        // allow operator* to get a reference to the thing.
        // We don't allow exposing it as a pointer directly, but some places need
        // to peel back the heap<> wrapper because otherwise template type deduction
        // might break.
        T &operator*() & { // *NOPAD*
            return *heaped_;
        }
        T const &operator*() const & { // *NOPAD*
            return *heaped_;
        }
        // Intentionally move construct a value T to avoid binding a ref to a temporary.
        T operator*() && { // *NOPAD*
            return std::move( *heaped_ );
        }

    private:
        // Helper for proxy functions.
        T &val() {
            return *heaped_;
        }
        T const &val() const {
            return *heaped_;
        }
    public:

        // Various conditionally defined proxy functions for common types like containers.
#pragma push_macro("PROXY")
#define PROXY(func) \
    template<typename ...Us> \
    auto func( Us&& ...us ) -> decltype( val().func( std::forward<Us>( us )... ) ) { \
        return val().func( std::forward<Us>( us )... ); \
    }
#pragma push_macro("PROXY_CONST")
#define PROXY_CONST(func) \
    template<typename ...Us> \
    auto func( Us&& ...us ) const -> decltype( val().func( std::forward<Us>( us )... ) ) { \
        return val().func( std::forward<Us>( us )... ); \
    }

        // Comparison operators.
        auto operator==( heap const &rhs ) const -> decltype( val() == val() ) {
            const T *lhsp = heaped_.get();
            const T *rhsp = rhs.heaped_.get();

            return rhsp && lhsp && *rhsp == *lhsp;
        }

        auto operator!=( heap const &rhs ) const -> decltype( val() != val() ) {
            const T *lhsp = heaped_.get();
            const T *rhsp = rhs.heaped_.get();

            return rhsp && lhsp && *rhsp != *lhsp;
        }


        // Tests & sets
        PROXY_CONST( empty )
        PROXY_CONST( count )
        PROXY_CONST( size )
        PROXY( clear )

        // Iterators
        PROXY( begin )
        PROXY_CONST( begin )
        PROXY( end )
        PROXY_CONST( end )

        // Accessors
        template<typename U>
        auto operator[]( U &&u ) -> decltype( val()[std::forward<U>( u )] ) {
            return val()[std::forward<U>( u )];
        }

        PROXY( find )
        PROXY_CONST( find )

        PROXY( erase )
        PROXY( insert )
        PROXY( emplace )

        // Json* support.
        template<typename Stream = JsonOut>
        void serialize( Stream &jsout ) const {
            jsout.write( val() );
        }

        template<typename Value = JsonValue, std::enable_if_t<std::is_same_v<std::decay_t<Value>, JsonValue>>* = nullptr>
        void deserialize( const Value &jsin ) {
            jsin.read( val() );
        }
#pragma pop_macro("PROXY")
#pragma pop_macro("PROXY_CONST")
};

} // namespace cata

#endif // CATA_SRC_VALUE_PTR_H
