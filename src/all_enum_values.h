#pragma once
#ifndef CATA_SRC_ALL_ENUM_VALUES_H
#define CATA_SRC_ALL_ENUM_VALUES_H

#include "enum_traits.h"

template<typename E>
constexpr size_t num_enum_values()
{
    return static_cast<size_t>( enum_traits<E>::last );
}

template<typename E, size_t... I>
auto all_enum_values_helper( std::index_sequence<I...> ) ->
const std::array<E, num_enum_values<E>()> &
{
    static constexpr std::array<E, num_enum_values<E>()> result{ { static_cast<E>( I )... } };
    return result;
}

template<typename E>
auto all_enum_values() -> const std::array<E, num_enum_values<E>()> &
{
    return all_enum_values_helper<E>( std::make_index_sequence<num_enum_values<E>()> {} );
}

#endif // CATA_SRC_ALL_ENUM_VALUES_H
