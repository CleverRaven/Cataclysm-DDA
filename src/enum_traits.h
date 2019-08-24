#pragma once
#ifndef CATA_ENUM_TRAITS_H
#define CATA_ENUM_TRAITS_H

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

#endif // CATA_ENUM_TRAITS_H
