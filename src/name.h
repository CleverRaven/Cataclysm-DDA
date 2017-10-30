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
static inline nameFlags operator|( nameFlags l, nameFlags r )
{
    return static_cast<nameFlags>( static_cast<unsigned>( l ) | static_cast<unsigned>( r ) );
}

namespace Name
{
// load names from json file
void load_from_file( std::string const &filename );

// Return random name according to search flags
std::string get( nameFlags searchFlags );

// Return random name given gender
std::string generate( bool is_male );

void clear();
};

#endif
