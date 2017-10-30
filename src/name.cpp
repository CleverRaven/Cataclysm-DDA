#include <map>
#include <string>

#include "name.h"
#include "json.h"
#include "string_formatter.h"
#include "output.h"
#include "translations.h"
#include "rng.h"
#include "cata_utility.h"

static inline nameFlags operator&( nameFlags l, nameFlags r ) {
    return static_cast<nameFlags>( static_cast<unsigned>(l) & static_cast<unsigned>(r) );
}

namespace Name
{
static std::map< nameFlags, std::vector< std::string > > names;

struct str_flag { char const* str; nameFlags f; };

static const str_flag usage_flags[] = {
    { "given",     nameIsGivenName },
    { "family",    nameIsFamilyName },
    { "universal", nameIsGivenName | nameIsFamilyName },
    { "backer",    nameIsFullName },
    { "city",      nameIsTownName },
    { "world",     nameIsWorldName }
};
static const str_flag gender_flags[] = {
    { "male",   nameIsMaleName },
    { "female", nameIsFemaleName },
    { "unisex", nameIsUnisexName }
};

// The loaded name is one of usage with optional gender.
// The combinations used in names files are as follows.
//
// Backer | (Female|Male|Unisex)
// Given  | (Female|Male)        // unisex names are duplicated in each group
// Family | Unisex
// City
// World

static void load_name( JsonObject &jo )
{
    std::string name = jo.get_string( "name" );
    std::string usage = jo.get_string( "usage" );
    nameFlags type;

    for( auto const&i : usage_flags ) {
        if( usage == i.str ) {
            type = i.f;
            break;
        }
    }

    if( jo.has_member( "gender" ) ) {
        std::string gender = jo.get_string( "gender" );
        for( auto const& i : gender_flags ) {
            if( gender == i.str ) {
                type = type | i.f;
                break;
            }
        }
    }

    // Add the name to the appropriate bucket
    names[type].push_back( name );
}

static void load( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject json_name = jsin.get_object();
        load_name( json_name );
    }
}

void load_from_file( std::string const& filename )
{
    read_from_file_json( filename, load );
}

// Get a random name with the specified flag
std::string get( nameFlags searchFlags )
{
    // get name groups for which searchFlag is a subset.
    //
    // i.e. if searchFlag is  [ Male|Family ]
    // it will match any group with those two flags set,
    // such as [ Unisex|Family ] (which is what is in the names.json)
    std::vector< decltype(names.cbegin()) > matching_groups;
    for( auto it = names.cbegin(), end = names.cend(); it != end; ++it ) {
        const nameFlags type = it->first;
        if( ( searchFlags & type ) == searchFlags ) {
            matching_groups.push_back( it );
        }
    }

    if( matching_groups.size() ) {
        // get number of choices
        int nChoices = 0;
        for( auto const& i : matching_groups ) {
            auto const& group = i->second;
            nChoices += group.size();
        }

        // make random selection and return result.
        choice = rng( 0, nChoices - 1 );
        for( auto const& i : matching_groups ) {
            auto const& group = i->second;
            if( choice < ( int ) group.size() ) {
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

