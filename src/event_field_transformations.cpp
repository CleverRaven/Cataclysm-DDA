#include "event_field_transformations.h"

#include "mtype.h"

static std::vector<cata_variant> species_of_monster( const cata_variant &v )
{
    const std::set<species_id> &species = v.get<mtype_id>()->species;
    std::vector<cata_variant> result;
    result.reserve( species.size() );
    for( const species_id &s : species ) {
        result.push_back( cata_variant( s ) );
    }
    return result;
}

const std::unordered_map<std::string, EventFieldTransformation> event_field_transformations = {
    { "species_of_monster", species_of_monster },
};
