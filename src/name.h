#pragma once
#ifndef NAME_H
#define NAME_H

#include <string>

enum nameFlags {
    nameIsMaleName   = 1 << 0,
    nameIsFemaleName = 1 << 1,
    nameIsUnisexName = nameIsMaleName | nameIsFemaleName,
    nameIsGivenName  = 1 << 2,
    nameIsFamilyName = 1 << 3,
    nameIsTownName   = 1 << 4,
    nameIsFullName   = 1 << 5,
    nameIsWorldName  = 1 << 6
};

namespace Name
{
/// Load names from given json file to use for generation
void load_from_file( std::string const &filename );

/// Return a random name given search flags
std::string get( nameFlags searchFlags );

/// Return a random full name given gender
std::string generate( bool is_male );

/// Clear names used for generation
void clear();
};

inline nameFlags operator|( nameFlags l, nameFlags r )
{
    return static_cast<nameFlags>( static_cast<unsigned>( l ) | static_cast<unsigned>( r ) );
}

inline nameFlags operator&( nameFlags l, nameFlags r )
{
    return static_cast<nameFlags>( static_cast<unsigned>( l ) & static_cast<unsigned>( r ) );
}

#endif
