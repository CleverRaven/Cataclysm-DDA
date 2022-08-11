#pragma once
#ifndef CATA_SRC_NAME_H
#define CATA_SRC_NAME_H

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

template <typename E> struct enum_traits;

/// @brief types of proper noun tables
enum class nameFlags : int {
    /// Masculine first names, also used for Unisex
    IsMaleName   = 1 << 0,
    /// Feminine first names, also used for Unisex
    IsFemaleName = 1 << 1,
    IsUnisexName = IsMaleName | IsFemaleName,
    IsGivenName  = 1 << 2,
    IsFamilyName = 1 << 3,
    IsNickName   = 1 << 4,
    /// Names of cities and towns in game
    IsTownName   = 1 << 5,
    /// Full names of crowdfunding donors
    IsFullName   = 1 << 6,
    /// Name belongs to list for world config saves. Seen in start menu.
    IsWorldName  = 1 << 7
};

template<>
struct enum_traits<nameFlags> {
    static constexpr bool is_flag_enum = true;
};

namespace Name
{
using names_map = std::map< nameFlags, std::vector< std::string > >;
names_map &get_names();
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
