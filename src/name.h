#pragma once
#ifndef NAME_H
#define NAME_H

#include <string>
#include <map>
#include <vector>

enum nameFlags {
    nameIsMaleName   = 1 << 0,
    nameIsFemaleName = 1 << 1,
    nameIsUnisexName = nameIsMaleName | nameIsFemaleName,
    nameIsGivenName  = 1 << 2,
    nameIsFamilyName = 1 << 3,
    nameIsNickName   = 1 << 4,
    nameIsTownName   = 1 << 5,
    nameIsFullName   = 1 << 6,
    nameIsWorldName  = 1 << 7
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
}

inline nameFlags operator|( nameFlags l, nameFlags r )
{
    return static_cast<nameFlags>( static_cast<unsigned>( l ) | static_cast<unsigned>( r ) );
}

inline nameFlags operator&( nameFlags l, nameFlags r )
{
    return static_cast<nameFlags>( static_cast<unsigned>( l ) & static_cast<unsigned>( r ) );
}

enum NameFlags
{
    NAME_IS_MALE_NAME,
    NAME_IS_FEMALE_NAME,
    NAME_IS_UNISEX_NAME,
    NAME_IS_GIVEN_NAME,
    NAME_IS_FAMILY_NAME,
    NAME_IS_NICK_NAME,
    NAME_IS_TOWN_NAME,
    NAME_IS_WORLD_NAME
};

class RandomName
{

private:

    std::vector<std::string> namesMale;
    std::vector<std::string> namesFemale;
    std::vector<std::string> namesUnisex;
    std::vector<std::string> namesGivenMale;
    std::vector<std::string> namesGivenFemale;
    std::vector<std::string> namesFamily;
    std::vector<std::string> namesNick;
    std::vector<std::string> namesTown;
    std::vector<std::string> namesWorld;

    void loadNames( );

public:

    RandomName();

    std::string getRandomName( NameFlags flag );

    void toString();
};

#endif
