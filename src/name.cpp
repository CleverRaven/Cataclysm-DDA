#include "name.h"

#include <map>
#include <string>

#include "cata_utility.h"
#include "json.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"

namespace Name
{
static std::map< nameFlags, std::vector< std::string > > names;

static const std::map< std::string, nameFlags > usage_flags = {
    { "given",     nameIsGivenName },
    { "family",    nameIsFamilyName },
    { "universal", nameIsGivenName | nameIsFamilyName },
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
    auto it = usage_flags.find( usage );
    if( it != usage_flags.end() ) {
        return it->second;
    }
    return static_cast< nameFlags >( 0 );
}

static nameFlags gender_flag( const std::string &gender )
{
    auto it = gender_flags.find( gender );
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
    read_from_file_json( filename, load );
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
    nameFlags baseSearchFlags = is_male ? nameIsMaleName : nameIsFemaleName;
    //One in four chance to pull from the backer list, otherwise generate a name from the parts list
    if( one_in( 4 ) ) {
        return get( baseSearchFlags | nameIsFullName );
    } else {
        //~ used for constructing names. swapping these will put family name first.
        return string_format( pgettext( "Full Name", "%1$s %2$s" ),
                              get( baseSearchFlags | nameIsGivenName ).c_str(),
                              get( baseSearchFlags | nameIsFamilyName ).c_str()
                            );
    }
}

void clear()
{
    names.clear();
}
}

