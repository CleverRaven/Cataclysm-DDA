#pragma once
#ifndef CATA_SRC_ENUM_CONVERSIONS_H
#define CATA_SRC_ENUM_CONVERSIONS_H

#include <optional>
#include <unordered_map>

#include "debug.h"
#include "enum_traits.h"

namespace io
{
/**
 * @name Enumeration (de)serialization to/from string.
 *
 * @ref enum_to_string converts an enumeration value to a string (which can be written to JSON).
 * The result must be an non-empty string.
 *
 * This function must be implemented somewhere for each enumeration type for
 * which conversion is required.
 *
 * Such enums must also specialize enum_traits to specify their 'last' member.
 *
 * @ref string_to_enum converts the string value back into an enumeration value. The input
 * is expected to be one of the outputs of @ref enum_to_string. If the given string does
 * not match an enumeration, an @ref InvalidEnumString is thrown.
 *
 * @code string_to_enum<E>(enum_to_string<E>(X)) == X @endcode must yield true for all values
 * of the enumeration E.
 */
/*@{*/
class InvalidEnumString : public std::runtime_error
{
    public:
        InvalidEnumString() : std::runtime_error( "invalid enum string" ) { }
        explicit InvalidEnumString( const std::string &msg ) : std::runtime_error( msg ) { }
};

template<typename E>
std::string enum_to_string( E );

template<typename E>
std::unordered_map<std::string, E> build_enum_lookup_map()
{
    static_assert( std::is_enum_v<E>, "E should be an enum type" );
    static_assert( has_enum_traits<E>::value, "enum E needs a specialization of enum_traits" );
    std::unordered_map<std::string, E> result;

    using Int = std::underlying_type_t<E>;
    constexpr Int max = static_cast<Int>( enum_traits<E>::last );

    for( Int i = 0; i < max; ++i ) {
        E e = static_cast<E>( i );
        auto inserted = result.emplace( enum_to_string( e ), e );
        if( !inserted.second ) {
            cata_fatal( "repeated enum string %s (%d and %d)", inserted.first->first,
                        static_cast<Int>( inserted.first->second ), i );
        }
    }

    return result;
}

template<typename E>
const std::unordered_map<std::string, E> &get_enum_lookup_map()
{
    static const std::unordered_map<std::string, E> string_to_enum_map =
        build_enum_lookup_map<E>();
    return string_to_enum_map;
}

// Helper function to do the lookup in an associative container
template<typename C, typename E = typename C::mapped_type>
inline E string_to_enum_look_up( const C &container, const std::string &data )
{
    const auto iter = container.find( data );
    if( iter == container.end() ) {
        throw InvalidEnumString( "Invalid enum string '" + data + "' for '" +
                                 demangle( typeid( E ).name() ) + "'" );
    }
    return iter->second;
}

template<typename E>
E string_to_enum( const std::string &data )
{
    return string_to_enum_look_up( get_enum_lookup_map<E>(), data );
}

// Helper function to do the lookup in an associative container
template<typename C, typename E = typename C::mapped_type>
inline std::optional<E> string_to_enum_look_up_optional( const C &container,
        const std::string &data )
{
    const auto iter = container.find( data );
    if( iter == container.end() ) {
        return std::nullopt;
    }
    return iter->second;
}

template<typename E>
std::optional<E> string_to_enum_optional( const std::string &data )
{
    return string_to_enum_look_up_optional( get_enum_lookup_map<E>(), data );
}

template<typename E>
bool enum_is_valid( const std::string &data )
{
    return get_enum_lookup_map<E>().count( data );
}

/*@}*/
} // namespace io

#endif // CATA_SRC_ENUM_CONVERSIONS_H
