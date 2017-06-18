#include <map>
#include <sstream>

#include "json.h"
#include "name.h"
#include "output.h"
#include "translations.h"
#include "rng.h"
#include "cata_utility.h"

NameGenerator::NameGenerator()
{

}

void NameGenerator::clear_names()
{
    names.clear();
}

void NameGenerator::load_name( JsonObject &jo )
{
    std::string name = jo.get_string( "name" );
    std::string usage = jo.get_string( "usage" );
    uint32_t type = 0;

    if( usage == "given" ) {
        type |= nameIsGivenName;
    } else if( usage == "family" ) {
        type |= nameIsFamilyName;
    } else if( usage == "universal" ) {
        type |= nameIsGivenName | nameIsFamilyName;
    } else if( usage == "backer" ) {
        type |= nameIsFullName;
    } else if( usage == "city" ) {
        type |= nameIsTownName;
    } else if( usage == "world" ) {
        type |= nameIsWorldName;
    }

    // Gender is optional
    if( jo.has_member( "gender" ) ) {
        std::string gender = jo.get_string( "gender" );

        if( gender == "male" ) {
            type |= nameIsMaleName;
        } else if( gender == "female" ) {
            type |= nameIsFemaleName;
        } else if( gender == "unisex" ) {
            type |= nameIsUnisexName;
        }
    }

    // Add the name to the appropriate bucket
    names[type].push_back( Name( name, type ) );
}

// Find all name types satisfying the search flags.
// There can be more than one, i.e. if searchFlags == nameIsUnisexName
// this function will return [ nameIsMaleName, nameIsFemaleName ]
std::vector<uint32_t> NameGenerator::uint32_tsFromFlags( uint32_t searchFlags ) const
{
    std::vector<uint32_t> types;

    for( std::map< uint32_t, std::vector<Name> >::const_iterator it = names.begin();
         it != names.end(); ++it ) {
        const uint32_t type = it->first;
        if( ( searchFlags & type ) == searchFlags ) {
            types.push_back( type );
        }
    }

    return types;
}

// Get a random name with the specified flag
std::string NameGenerator::getName( uint32_t searchFlags )
{
    const std::vector<uint32_t> types = uint32_tsFromFlags( searchFlags );
    int nTypes = types.size();

    if( nTypes == 0 ) {
        // BUG, no matching name type found.
        return std::string( _( "Tom" ) );
    }

    // Get number of matching names
    int nNames = 0;
    for( uint32_t type : types ) {
        nNames += names[type].size();
    }

    // Choose a random name
    int choice = rng( 0, nNames - 1 );
    for( uint32_t type : types ) {
        std::vector<Name> &theseNames = names[type];
        if( choice < int( theseNames.size() ) ) {
            return theseNames[choice].value();
        }
        choice -= theseNames.size();
    }

    // BUG, no matching name found.
    return std::string( _( "Tom" ) );
}

std::string NameGenerator::generateName( bool male )
{
    uint32_t baseSearchFlags = male ? nameIsMaleName : nameIsFemaleName;
    //One in four chance to pull from the backer list, otherwise generate a name from the parts list
    if( one_in( 4 ) ) {
        return getName( baseSearchFlags | nameIsFullName );
    } else {
        //~ used for constructing names. swapping these will put family name first.
        return string_format( pgettext( "Full Name", "%1$s %2$s" ),
                              getName( baseSearchFlags | nameIsGivenName ).c_str(),
                              getName( baseSearchFlags | nameIsFamilyName ).c_str()
                            );
    }
}

NameGenerator &Name::generator()
{
    return NameGenerator::generator();
}

std::string Name::generate( bool male )
{
    return NameGenerator::generator().generateName( male );
}

std::string Name::get( uint32_t searchFlags )
{
    return NameGenerator::generator().getName( searchFlags );
}

Name::Name()
{
    _value = _( "Tom" );
    _type = 15;
}

Name::Name( std::string name, uint32_t type )
{
    _value = name;
    _type = type;
}

void NameGenerator::load( JsonIn &jsin )
{
    jsin.start_array();
    while( !jsin.end_array() ) {
        JsonObject json_name = jsin.get_object();
        load_name( json_name );
    }
}

void load_names_from_file( const std::string &filename )
{
    using namespace std::placeholders;
    read_from_file_json( filename, std::bind( &NameGenerator::load, &NameGenerator::generator(), _1 ) );
}

