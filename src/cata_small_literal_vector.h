#pragma once
#ifndef CATA_SRC_CATA_SMALL_LITERAL_VECTOR_H
#define CATA_SRC_CATA_SMALL_LITERAL_VECTOR_H

#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "cata_scope_helpers.h"

/**
 * A vector type that is specialized for storing trivial types in contiguous
 * storage with a statically predetermined amount of inline storage.
 *
 * Because this is intended to be *small*, the internal capacity and size variables
 * default to uint8_t for minimal overhead over the inline storage.
 */
template<typename T, size_t kInlineCount = 4, typename SizeT = uint8_t>
struct alignas( T * ) small_literal_vector {
    // To avoid having to invoke constructors and destructors when copying Elements
    // around or inserting new ones, we enforce it is a literal type.
    static_assert( std::is_trivially_destructible_v<T>, "T must be trivially destructible." );
    static_assert( std::is_trivially_copyable_v<T>, "T must be trivially copyable." );
    static_assert( std::is_trivially_default_constructible_v<T>,
                   "T must be trivially default constuctible." );

    small_literal_vector() : heap_( nullptr ), capacity_( kInlineCount ), len_( 0 ) {}
    small_literal_vector( const small_literal_vector &other ) : capacity_( kInlineCount ), len_( 0 ) {
        copy_init( other );
    }
    small_literal_vector &operator=( const small_literal_vector &rhs ) {
        copy_init( rhs );
        return *this;
    }

    small_literal_vector( small_literal_vector &&other ) noexcept {
        steal_init( std::move( other ) );
    }
    small_literal_vector &operator=( small_literal_vector &&rhs ) noexcept {
        steal_init( std::move( rhs ) );
        return *this;
    }

    ~small_literal_vector() {
        if( on_heap() ) {
            free( heap_ );
        }
    }

    T *begin() {
        return data();
    }

    T *back() {
        return data() + size() - 1;
    }

    T *end() {
        return data() + size();
    }

    const T *begin() const {
        return data();
    }

    const T *back() const {
        return data() + size() - 1;
    }

    const T *end() const {
        return data() + size();
    }

    T *data() {
        return on_heap() ? heap_ : inline_;
    }

    const T *data() const {
        return on_heap() ? heap_ : inline_;
    }

    SizeT size() const {
        return len_;
    }

    void push_back( const T &t ) {
        ensure_capacity_for( size() + 1 );
        *end() = t;
        len_ += 1;
    }
    template<typename ...US, std::enable_if_t<std::is_constructible_v<T, US...>> * = 0>
    void push_back( US && ...us ) {
        ensure_capacity_for( size() + 1 );
        *end() = T( std::forward < US && > ( us )... );
        len_ += 1;
    }

    void pop_back() {
        if( len_ == 0 ) {
            throw std::runtime_error( "Popping an empty vector." );
        }
        len_ -= 1;
    }

    // Some nonstandard helpers below.
    void ensure_capacity_for( size_t count ) {
        check_capacity( count );

        if( count <= capacity_ ) {
            return;
        }

        // 1.5x resize factor allows for reuse of earlier allocations at larger sizes.
        // That works best if we tracked explicit capacity, but it's better than nothing.
        // https://stackoverflow.com/a/1100426
        size_t new_capacity = capacity_;
        size_t next_capacity = capacity_ * 1.5;
        // Prevent overflow by testing before multiplying
        while( next_capacity > new_capacity && next_capacity < count ) {
            new_capacity = next_capacity;
            next_capacity *= 1.5;
        }
        if( new_capacity < count ) {
            new_capacity = std::numeric_limits<SizeT>::max();
        }
        reserve_exact( new_capacity );
    }

    void resize( size_t count ) {
        check_capacity( count );
        reserve_exact( count );
        len_ = count;
    }

private:
    void check_capacity( size_t count ) const {
        if( count > std::numeric_limits<SizeT>::max() ) {
            throw std::runtime_error(
                "Attempted to use small_literal_vector for " +
                std::to_string( count ) +
                " elements items when there is only tracking space for " +
                std::to_string( std::numeric_limits<SizeT>::max() ) );
        }
    }

    // 'exact' rounds up to kInlineCount
    void reserve_exact( size_t new_capacity ) {
        T *heap = nullptr;
        if( capacity_ <= kInlineCount ) {
            if( new_capacity <= kInlineCount ) {
                return;
            }
            on_out_of_scope heap_guard( [&heap] { free( heap ); } );
            heap = static_cast<T *>( std::malloc( sizeof( T ) * new_capacity ) );
            std::uninitialized_copy_n( inline_, size(), heap );
            heap_ = heap;
            heap_guard.cancel();
        } else {
            // Don't bother 'unheapifying', but do shrink allocation.
            // GCC incorrectly tags heap_ as possibly uninitialized in the copy_init path.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
            heap = static_cast<T *>( std::realloc( heap_, sizeof( T ) * new_capacity ) );
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
            if( heap ) {
                heap_ = heap;
            } else {
                throw std::runtime_error( "realloc failed." );
            }
        }

        capacity_ = new_capacity;
    }

    void copy_init( const small_literal_vector &other ) {
        if( this != &other ) {
            ensure_capacity_for( other.capacity_ );
            len_ = other.size();
            std::uninitialized_copy_n( other.data(), len_, data() );
        }
    }

    void steal_init( small_literal_vector &&other ) noexcept {
        if( this != &other ) {
            if( other.on_heap() ) {
                // Optimization: steal the heap pointer.
                heap_ = other.heap_;
                capacity_ = other.capacity_;
                other.heap_ = nullptr;
                other.capacity_ = kInlineCount;
            } else {
                // Skip the ensure_capacity_for call cause we know it has to be inline.
                std::uninitialized_copy_n( other.inline_, other.len_, inline_ );
                capacity_ = kInlineCount;
            }
            len_ = other.len_;
            other.len_ = 0;
        }
    }

    bool on_heap() const {
        return capacity_ > kInlineCount;
    }

    // The whole small_literal_vector has alignas(T*) so that the alignment requirements of heap_
    // are never violated, even if you allocate several of these in a row. However we use pragma pack
    // So there are no padding bytes inserted into the union to bring it to a multiple of sizeof(T*)
    // so that capacity_ and len_ can be tightly packed adjacent to it. Their alignments will still
    // be legal because the compiler will insert padding before capacity_ if necessary.
    // We put T *heap_ as the first entry in the union to avoid default initializing the whole inline storage.
#pragma pack(push, 1)
    union alignas( T ) {
        T *heap_;
        // NOLINTNEXTLINE(modernize-avoid-c-arrays)
        T inline_[ kInlineCount ];
    };
#pragma pack(pop)
    // capacity_ > kInlineCount => on heap.
    SizeT capacity_;
    SizeT len_;
};

#endif // CATA_SRC_CATA_SMALL_LITERAL_VECTOR_H
