#include "overmap_location.h"

#include <map>
#include <set>
#include <string>
#include <utility>

#include "debug.h"
#include "generic_factory.h"
#include "json.h"
#include "omdata.h"
#include "rng.h"

namespace
{

generic_factory<overmap_location> locations( "overmap location" );

} // namespace

template<>
bool string_id<overmap_location>::is_valid() const
{
    return locations.is_valid( *this );
}

template<>
const overmap_location &string_id<overmap_location>::obj() const
{
    return locations.obj( *this );
}

bool overmap_location::test( const int_id<oter_t> &oter ) const
{
    return terrains.count( oter->get_type_id() );
}

oter_type_id overmap_location::get_random_terrain() const
{
    return random_entry( terrains );
}

void overmap_location::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "flags", flags );
    optional( jo, was_loaded, "terrains", terrains );
    if( flags.empty() && terrains.empty() ) {
        jo.throw_error( "At least one flag or terrain must be specified." );
    }
}

const cata::flat_set<oter_type_str_id> &overmap_location::get_all_terrains() const
{
    return terrains;
}

void overmap_location::check() const
{
    for( const auto &element : terrains ) {
        if( !element.is_valid() ) {
            debugmsg( "In overmap location \"%s\", terrain \"%s\" is invalid.", id.c_str(), element.c_str() );
        }
    }
}

void overmap_location::finalize()
{
    const std::unordered_map<std::string, oter_flags> &oter_flags_map =
        io::get_enum_lookup_map<oter_flags>();
    for( const std::string &elem : flags ) {
        auto it = oter_flags_map.find( elem );
        if( it == oter_flags_map.end() ) {
            continue;
        }
        oter_flags check_flag = it->second;
        for( const oter_t &ter_elem : overmap_terrains::get_all() ) {
            if( ter_elem.has_flag( check_flag ) ) {
                terrains.insert( ter_elem.get_type_id() );
            }
        }
    }
}

void overmap_locations::load( const JsonObject &jo, const std::string &src )
{
    locations.load( jo, src );
}

void overmap_locations::check_consistency()
{
    locations.check();
}

void overmap_locations::reset()
{
    locations.reset();
}

void overmap_locations::finalize()
{
    locations.finalize();
    for( const overmap_location &elem : locations.get_all() ) {
        const_cast<overmap_location &>( elem ).finalize(); // This cast is ugly, but safe.
    }
}
