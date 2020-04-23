#include "event_field_transformations.h"

#include <set>

#include "mtype.h"
#include "string_id.h"
#include "type_id.h"

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

const std::unordered_map<std::string, event_field_transformation> event_field_transformations = {
    {
        "species_of_monster",
        { species_of_monster, cata_variant_type::species_id, { cata_variant_type::mtype_id } }
    },
};
