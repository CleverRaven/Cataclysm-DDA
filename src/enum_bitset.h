#pragma once
#ifndef ENUM_BITSET_H
#define ENUM_BITSET_H

#include <bitset>
#include <type_traits>

#include "enum_traits.h"

template<typename E>
class enum_bitset
{
        static_assert( std::is_enum<E>::value, "the template argument is not an enum." );
        static_assert( has_enum_traits<E>::value,
                       "a specialization of 'enum_traits<E>' template containing 'last' element of the enum must be defined somewhere.  "
                       "The `last` constant must be of the same type as the enum iteslf."
                     );

    public:
        enum_bitset() = default;
        enum_bitset( const enum_bitset & ) = default;
        enum_bitset &operator=( const enum_bitset & ) = default;

        bool operator==( const enum_bitset &rhs ) const noexcept {
            return bits == rhs.bits;
        }

        bool operator!=( const enum_bitset &rhs ) const noexcept {
            return !( *this == rhs );
        }

        enum_bitset &operator&=( const enum_bitset &rhs ) noexcept {
            bits &= rhs.bits;
            return *this;
        }

        enum_bitset &operator|=( const enum_bitset &rhs ) noexcept {
            bits |= rhs.bits;
            return *this;
        }

        bool operator[]( E e ) const {
            return bits[ get_pos( e ) ];
        }

        enum_bitset &operator[]( E e ) {
            return bits[ get_pos( e ) ];
        }

        enum_bitset &set( E e, bool val = true ) {
            bits.set( get_pos( e ), val );
            return *this;
        }

        enum_bitset &reset( E e ) {
            bits.reset( get_pos( e ) );
            return *this;
        }

        enum_bitset &reset() {
            bits.reset();
            return *this;
        }

        bool test( E e ) const {
            return bits.test( get_pos( e ) );
        }

        static constexpr size_t size() noexcept {
            return get_pos( enum_traits<E>::last );
        }

    private:
        static constexpr size_t get_pos( E e ) noexcept {
            return static_cast<size_t>( static_cast<typename std::underlying_type<E>::type>( e ) );
        }

    private:
        std::bitset<enum_bitset<E>::size()> bits;
};

#endif // ENUM_BITSET_H
