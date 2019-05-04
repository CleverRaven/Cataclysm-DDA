#include "name.h"

#include <stddef.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "json.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "path_info.h"

namespace Name
{
static std::map< nameFlags, std::vector< std::string > > names;

static const std::map< std::string, nameFlags > usage_flags = {
    { "given",     nameIsGivenName },
    { "family",    nameIsFamilyName },
    { "universal", nameIsGivenName | nameIsFamilyName },
    { "nick",      nameIsNickName },
    { "backer",    nameIsFullName },
    { "city",      nameIsTownName },
    { "world",     nameIsWorldName }
};

static const std::map< std::string, nameFlags > gender_flags {
    { "male",   nameIsMaleName },
    { "female", nameIsFemaleName },
    { "unisex", nameIsUnisexName }
};

static nameFlags usage_flag( const std::string &usage )
{
    const auto it = usage_flags.find( usage );
    if( it != usage_flags.end() ) {
        return it->second;
    }
    return static_cast< nameFlags >( 0 );
}

static nameFlags gender_flag( const std::string &gender )
{
    const auto it = gender_flags.find( gender );
    if( it != gender_flags.end() ) {
        return it->second;
    }
    return static_cast< nameFlags >( 0 );
}

// The loaded name is one of usage with optional gender.
// The combinations used in names files are as follows.
//
// Backer | (Female|Male|Unisex)
// Given  | (Female|Male)        // unisex names are duplicated in each group
// Family | Unisex
// Nick
// City
// World
static void load( JsonIn &jsin )
{
    jsin.start_array();

    while( !jsin.end_array() ) {
        JsonObject jo = jsin.get_object();

        // get flags of name.
        const nameFlags type =
            usage_flag( jo.get_string( "usage" ) )
            | gender_flag( jo.get_string( "gender", "" ) );

        // find group type and add name to group
        names[type].push_back( jo.get_string( "name" ) );
    }
}

void load_from_file( const std::string &filename )
{
    // TODO: Forever is data/names/en.json {Stupid Bug}
    read_from_file_json( filename, load );

    RandomName *randomName = new RandomName();
    randomName->getRandomName(NAME_IS_MALE_NAME);
    randomName->toString();
}

// get name groups for which searchFlag is a subset.
//
// i.e. if searchFlag is  [ Male|Family ]
// it will match any group with those two flags set, such as [ Unisex|Family ]
using names_vec = std::vector< decltype( names.cbegin() ) >;

static names_vec get_matching_groups( nameFlags searchFlags )
{
    names_vec matching_groups;
    for( auto it = names.cbegin(), end = names.cend(); it != end; ++it ) {
        const nameFlags type = it->first;
        if( ( searchFlags & type ) == searchFlags ) {
            matching_groups.push_back( it );
        }
    }
    return matching_groups;
}

// Get a random name with the specified flag
std::string get( nameFlags searchFlags )
{
    auto matching_groups = get_matching_groups( searchFlags );
    if( ! matching_groups.empty() ) {
        // get number of choices
        size_t nChoices = 0;
        for( const auto &i : matching_groups ) {
            const auto &group = i->second;
            nChoices += group.size();
        }

        // make random selection and return result.
        size_t choice = rng( 0, nChoices - 1 );
        for( const auto &i : matching_groups ) {
            const auto &group = i->second;
            if( choice < group.size() ) {
                return group[choice];
            }
            choice -= group.size();
        }
    }
    // BUG, no matching name found.
    return std::string( _( "Tom" ) );
}

std::string generate( bool is_male )
{
    const nameFlags baseSearchFlags = is_male ? nameIsMaleName : nameIsFemaleName;
    //One in twenty chance to pull from the backer list, otherwise generate a name from the parts list
    if( one_in( 20 ) ) {
        return get( baseSearchFlags | nameIsFullName );
    } else {
        //~ Used for constructing full name: %1$s is `family name`, %2$s is `given name`
        std::string full_name_format = "%1$s %2$s";
        //One in three chance to add a nickname to full name
        if( one_in( 3 ) ) {
            //~ Used for constructing full name with nickname: %1$s is `family name`, %2$s is `given name`, %3$s is `nickname`
            full_name_format = "%1$s '%3$s' %2$s";
        }
        return string_format( pgettext( "Full Name", full_name_format.c_str() ),
                              get( baseSearchFlags | nameIsGivenName ).c_str(),
                              get( baseSearchFlags | nameIsFamilyName ).c_str(),
                              get( nameIsNickName ).c_str()
                            );
    }
}

void clear()
{
    names.clear();
}
}

RandomName::RandomName( )
{
    loadNames();
}

void RandomName::loadNames( )
{
    Path *path = Path::getInstance();

    std::string pathToFileJSON = path->getPathForValueKey("NAMES_FILE");

    std::ifstream fileJSON;

    fileJSON.open(pathToFileJSON, std::ifstream::in);

    JsonIn *inputJSON = new JsonIn(fileJSON);

    inputJSON->start_array();

    while(!inputJSON->end_array())
    {
        JsonObject objectJSON = inputJSON->get_object();

        if (objectJSON.get_string("usage") == "nick")
        {
            namesNick.push_back(objectJSON.get_string("name"));
        }
        else if (objectJSON.get_string("usage") == "backer")
        {
            if (objectJSON.get_string("gender") == "male")
            {
                namesMale.push_back(objectJSON.get_string("name"));
            }
            else if (objectJSON.get_string("gender") == "female")
            {
                namesFemale.push_back(objectJSON.get_string("name"));
            }
            else
            {
                namesUnisex.push_back(objectJSON.get_string("name"));
            }
        }
        else if (objectJSON.get_string("usage") == "city")
        {
            namesTown.push_back(objectJSON.get_string("name"));
        }
        else if (objectJSON.get_string("usage") == "family")
        {
            namesFamily.push_back(objectJSON.get_string("name"));
        }
        else if (objectJSON.get_string("usage") == "given")
        {
            if (objectJSON.get_string("gender") == "male")
            {
                namesGivenMale.push_back(objectJSON.get_string("name"));
            }
            else
            {
                namesGivenFemale.push_back(objectJSON.get_string("name"));
            }
        }
        else if (objectJSON.get_string("usage") == "world")
        {
            namesWorld.push_back(objectJSON.get_string("name"));
        }
    }

    fileJSON.close();
}

std::string RandomName::getRandomName( NameFlags flag )
{
    //TODO: Generate Number Random

    if (flag == NAME_IS_MALE_NAME)
    {
        short choice = rng(0, namesMale.size());
        return namesMale[choice];
    }
    else if (flag == NAME_IS_FEMALE_NAME)
    {
        short choice = rng(0, namesFemale.size());
        return namesFemale[choice];
    }
    else if (flag == NAME_IS_UNISEX_NAME)
    {
        short choice = rng(0, namesUnisex.size());
        return namesUnisex[choice];
    }
    else if (flag == NAME_IS_GIVEN_NAME)
    {
        short choiceGiven = rng(0, 1);

        if (choiceGiven == 1)
        {
            short choice = rng(0, namesGivenMale.size());
            return namesGivenMale[choice];
        }
        else
        {
            short choice = rng(0, namesGivenFemale.size());
            return namesGivenFemale[choice];
        }

    }
    else if (flag == NAME_IS_FAMILY_NAME)
    {
        short choice = rng(0, namesFamily.size());
        return namesFamily[choice];
    }
    else if (flag == NAME_IS_NICK_NAME)
    {
        short choice = rng(0, namesNick.size());
        return namesNick[choice];
    }
    else if (flag == NAME_IS_TOWN_NAME)
    {
        short choice = rng(0, namesTown.size());
        return namesTown[choice];
    }
    else if (flag == NAME_IS_WORLD_NAME)
    {
        short choice = rng(0, namesWorld.size());
        return namesWorld[choice];
    }
}

void RandomName::toString( )
{
    std::cout << std::endl << namesMale.size() << std::endl;
    std::cout << std::endl << namesFemale.size() << std::endl;
    std::cout << std::endl << namesUnisex.size() << std::endl;
    std::cout << std::endl << namesGivenMale.size() << std::endl;
    std::cout << std::endl << namesGivenFemale.size() << std::endl;
    std::cout << std::endl << namesFamily.size() << std::endl;
    std::cout << std::endl << namesNick.size() << std::endl;
    std::cout << std::endl << namesWorld.size() << std::endl;
}
