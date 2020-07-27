#pragma once
#ifndef CATA_SRC_NAME_H
#define CATA_SRC_NAME_H

#include <string>

#include "enum_traits.h"

enum class nameFlags : int {
    IsMaleName   = 1 << 0,
    IsFemaleName = 1 << 1,
    IsUnisexName = IsMaleName | IsFemaleName,
    IsGivenName  = 1 << 2,
    IsFamilyName = 1 << 3,
    IsNickName   = 1 << 4,
    IsTownName   = 1 << 5,
    IsFullName   = 1 << 6,
    IsWorldName  = 1 << 7
};

template<>
struct enum_traits<nameFlags> {
    static constexpr bool is_flag_enum = true;
};

namespace Name
{
/// Load names from given json file to use for generation
void load_from_file( const std::string &filename );

/// Return a random name given search flags
std::string get( nameFlags searchFlags );

/// Return a random full name given gender
std::string generate( bool is_male );

/// Clear names used for generation
void clear();
} // namespace Name

#endif // CATA_SRC_NAME_H
