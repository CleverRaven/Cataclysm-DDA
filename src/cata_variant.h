#pragma once
#ifndef CATA_VARIANT_H
#define CATA_VARIANT_H

#include <cassert>
#include <utility>

#include "debug.h"
#include "type_id.h"

using itype_id = std::string;

// cata_variant is a variant-like type that stores a variety of different cata
// types.  All types are stored by converting them to a string.

enum class cata_variant_type : int {
    itype_id,
    mtype_id,
    num_types, // last
};

namespace cata_variant_detail
{

std::string to_string( cata_variant_type );
cata_variant_type from_string( const std::string & );

template<cata_variant_type Type>
struct convert;

template<typename T, size_t... I>
constexpr cata_variant_type type_for_impl( std::index_sequence<I...> )
{
    constexpr size_t num_types = static_cast<size_t>( cata_variant_type::num_types );
    constexpr std::array<bool, num_types> matches = {{
            std::is_same<T, typename convert<static_cast<cata_variant_type>( I )>::type>::value...
        }
    };
    for( size_t i = 0; i < num_types; ++i ) {
        if( matches[i] ) {
            return static_cast<cata_variant_type>( i );
        }
    }
    // No match
    return cata_variant_type::num_types;
}

template<typename T>
constexpr cata_variant_type type_for()
{
    constexpr size_t num_types = static_cast<size_t>( cata_variant_type::num_types );
    return type_for_impl<T>( std::make_index_sequence<num_types> {} );
}

template<>
struct convert<cata_variant_type::itype_id> {
    using type = itype_id;
    static std::string to_string( const itype_id &v ) {
        return v;
    }
    static itype_id from_string( const std::string &v ) {
        return v;
    }
};

template<>
struct convert<cata_variant_type::mtype_id> {
    using type = mtype_id;
    static std::string to_string( const mtype_id &v ) {
        return v.str();
    }
    static mtype_id from_string( const std::string &v ) {
        return mtype_id( v );
    }
};

} // namespace cata_variant_detail

class cata_variant
{
    public:
        template<cata_variant_type Type, typename Value>
        static cata_variant make( Value &&value ) {
            return cata_variant(
                       Type, cata_variant_detail::convert<Type>::to_string(
                           std::forward<Value>( value ) ) );
        }

        cata_variant_type type() const {
            return type_;
        }

        template<cata_variant_type Type>
        auto get() const -> typename cata_variant_detail::convert<Type>::type {
            if( type_ != Type ) {
                debugmsg( "Tried to extract type %s from cata_variant which contained %s",
                          cata_variant_detail::to_string( Type ),
                          cata_variant_detail::to_string( type_ ) );
                return {};
            }
            return cata_variant_detail::convert<Type>::from_string( value_ );
        }

        template<typename T>
        T get() const {
            return get<cata_variant_detail::type_for<T>()>();
        }

        const std::string &get_string() const {
            return value_;
        }

    private:
        explicit cata_variant( cata_variant_type t, std::string &&v )
            : type_( t )
            , value_( std::move( v ) )
        {}

        cata_variant_type type_;
        std::string value_;
};

#endif // CATA_VARIANT_H
