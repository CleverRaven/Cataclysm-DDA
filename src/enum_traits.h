#pragma once
#ifndef CATA_SRC_ENUM_TRAITS_H
#define CATA_SRC_ENUM_TRAITS_H

#include <type_traits>

// enum_traits is a template you can specialize for your enum types.  It serves
// two purposes:
// * Picking out the special "last" enumerator if you have one.  This is used
//   by various generic code to iterate over all the enumerators.  Most notably
//   it enables io::string_to_enum and thereby string-based json serialization.
// * Specifying that your enum is a flag enum, and therefore that you want
//   bitwise operators to work for it.  This saves everyone from implementing
//   those operators independently.
//
// Usage examples:
//
// For a 'regular' enum
//
// enum ordinal {
//   first,
//   second,
//   third,
//   last
// };
//
// template<>
// struct enum_traits<ordinal> {
//   static constexpr ordinal last = ordinal::last;
// };
//
// For a flag enum
//
// enum my_flags {
//   option_one = 1 << 0,
//   option_two = 1 << 1,
// };
//
// template<>
// struct enum_traits<ordinal> {
//   static constexpr bool is_flag_enum = true;
// };

template<typename E>
struct enum_traits;

namespace enum_traits_detail
{

template<typename E>
using last_type = std::decay_t<decltype( enum_traits<E>::last )>;

} // namespace enum_traits_detail

template<typename E, typename U = E>
struct has_enum_traits : std::false_type {};

template<typename E>
struct has_enum_traits<E, enum_traits_detail::last_type<E>> : std::true_type {};

template<typename E, typename B = std::true_type>
struct is_flag_enum : std::false_type {};

template<typename E>
struct is_flag_enum<E, std::integral_constant<bool, enum_traits<E>::is_flag_enum>> :
            std::true_type {};

// Annoyingly, you cannot overload conversions for enums, and we want
// operator& to be convertible to bool, so we have it return a helper struct
// which can be treated as a bool or an enum value.
template<typename E>
struct enum_test_result {
    E value;

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator E() const {
        return value;
    }
    bool operator!() const {
        using I = std::underlying_type_t<E>;
        return !static_cast<I>( value );
    }
    explicit operator bool() const {
        return !!*this;
    }
};

template<typename E, typename = std::enable_if_t<is_flag_enum<E>::value>>
inline enum_test_result<E> operator&( E l, E r )
{
    using I = std::underlying_type_t<E>;
    return { static_cast<E>( static_cast<I>( l ) & static_cast<I>( r ) ) };
}

template<typename E, typename = std::enable_if_t<is_flag_enum<E>::value>>
inline E operator|( E l, E r )
{
    using I = std::underlying_type_t<E>;
    return static_cast<E>( static_cast<I>( l ) | static_cast<I>( r ) );
}

template<typename E, typename = std::enable_if_t<is_flag_enum<E>::value>>
inline E & operator&=( E &l, E r )
{
    return l = l & r;
}

template<typename E, typename = std::enable_if_t<is_flag_enum<E>::value>>
inline E & operator|=( E &l, E r )
{
    return l = l | r;
}

template<typename E, typename = std::enable_if_t<is_flag_enum<E>::value>>
inline bool operator!( E e )
{
    using I = std::underlying_type_t<E>;
    return !static_cast<I>( e );
}

#endif // CATA_SRC_ENUM_TRAITS_H
