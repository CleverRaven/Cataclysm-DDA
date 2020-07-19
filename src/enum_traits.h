#pragma once
#ifndef CATA_SRC_ENUM_TRAITS_H
#define CATA_SRC_ENUM_TRAITS_H

#include <type_traits>

template<typename E>
struct enum_traits;

namespace enum_traits_detail
{

template<typename E>
using last_type = typename std::decay<decltype( enum_traits<E>::last )>::type;

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
