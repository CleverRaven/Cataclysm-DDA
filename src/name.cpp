#include "name.h"

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "enum_traits.h"
#include "json.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"

namespace Name
{
names_map &get_names()
{
    static names_map names;
    return names;
}

static const std::map< std::string, nameFlags > usage_flags = {
    { "given",     nameFlags::IsGivenName },
    { "family",    nameFlags::IsFamilyName },
    { "universal", nameFlags::IsGivenName | nameFlags::IsFamilyName },
    { "nick",      nameFlags::IsNickName },
    { "backer",    nameFlags::IsFullName },
    { "city",      nameFlags::IsTownName },
    { "world",     nameFlags::IsWorldName }
};

static const std::map< std::string, nameFlags > gender_flags {
    { "male",   nameFlags::IsMaleName },
    { "female", nameFlags::IsFemaleName },
    { "unisex", nameFlags::IsUnisexName }
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
static void load( const JsonArray &names_json )
{
    for( JsonObject jo : names_json ) {
        // get flags of name.
        const nameFlags type =
            usage_flag( jo.get_string( "usage" ) )
            | gender_flag( jo.get_string( "gender", "" ) );

        // find group type and add name(s) to group
        if( jo.has_array( "name" ) ) {
            for( const std::string n : jo.get_array( "name" ) ) {
                get_names()[type].push_back( n );
            }
        } else {
            get_names()[type].push_back( jo.get_string( "name" ) );
        }
    }
}

void load_from_file( const cata_path &filename )
{
    read_from_file_json( filename, []( const JsonArray & jsin ) {
        load( jsin );
    } );
}

// get name groups for which searchFlag is a subset.
//
// i.e. if searchFlag is  [ Male|Family ]
// it will match any group with those two flags set, such as [ Unisex|Family ]
using names_vec = std::vector < decltype( get_names().cbegin() ) >;
static names_vec get_matching_groups( nameFlags searchFlags )
{
    names_vec matching_groups;
    for( auto it = get_names().cbegin(), end = get_names().cend(); it != end; ++it ) {
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
    if( !matching_groups.empty() ) {
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
    const nameFlags baseSearchFlags = is_male ? nameFlags::IsMaleName : nameFlags::IsFemaleName;
    //One in twenty chance to pull from the backer list, otherwise generate a name from the parts list
    if( one_in( 20 ) ) {
        return get( baseSearchFlags | nameFlags::IsFullName );
    } else {
        //~ Used for constructing full name: %1$s is `given name`, %2$s is `family name`
        translation full_name_format = to_translation( "Full Name", "%1$s %2$s" );
        // Chance to add a nickname to full name
        if( one_in( 10 ) ) {
            //~ Used for constructing full name with nickname: %1$s is `given name`, %2$s is `family name`, %3$s is `nickname`
            full_name_format = to_translation( "Full Name", "%1$s '%3$s' %2$s" );
        }
        return string_format( full_name_format,
                              get( baseSearchFlags | nameFlags::IsGivenName ).c_str(),
                              get( baseSearchFlags | nameFlags::IsFamilyName ).c_str(),
                              get( nameFlags::IsNickName ).c_str()
                            );
    }
}

void clear()
{
    get_names().clear();
}
} // namespace Name

