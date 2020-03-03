#include "overmap_biome.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

#include "generic_factory.h"
#include "omdata.h"
#include "overmap.h"
#include "rng.h"
#include "debug.h"
#include "json.h"

#include "string_id.h"
#include "int_id.h"

namespace
{

generic_factory<overmap_biome> biomes( "overmap biome" );
} // namespace

template<>
bool string_id<overmap_biome>::is_valid() const
{
    return biomes.is_valid( *this );
}

template<>
const overmap_biome &string_id<overmap_biome>::obj() const
{
    return biomes.obj( *this );
}

void overmap_biome::check() const
{
    //TODO: Probably what we need to do.
}

void overmap_biome::finalize()
{
    name = id.str();
    overmap_biomes_map.insert( std::make_pair( name, *this ) );
}

void overmap_biome::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "weight", weight );
}

void overmap_biomes::load( const JsonObject &jo, const std::string &src )
{
    biomes.load( jo, src );
}

void overmap_biomes::check_consistency()
{
    biomes.check();
}

void overmap_biomes::reset()
{
    biomes.reset();
}

void overmap_biomes::finalize()
{
    biomes.finalize();
    for( const overmap_biome &elem : biomes.get_all() ) {
        const_cast<overmap_biome &>( elem ).finalize(); // This cast is ugly, but safe.
    }
}
