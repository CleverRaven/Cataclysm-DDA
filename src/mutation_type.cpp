#include "mutation.h" // IWYU pragma: associated

#include "json.h"

struct mutation_type {
    std::string id;
};

std::map<std::string, mutation_type> mutation_types;

void load_mutation_type( JsonObject &jsobj )
{
    mutation_type new_type;
    new_type.id = jsobj.get_string( "id" );

    mutation_types[new_type.id] = new_type;
}

bool mutation_type_exists( const std::string &id )
{
    return mutation_types.find( id ) != mutation_types.end();
}

std::vector<trait_id> get_mutations_in_type( const std::string &id )
{
    std::vector<trait_id> ret;
    for( auto it : mutation_branch::get_all() ) {
        if( it.second.types.find( id ) != it.second.types.end() ) {
            ret.push_back( it.first );
        }
    }
    return ret;
}

std::vector<trait_id> get_mutations_in_types( const std::set<std::string> &ids )
{
    std::vector<trait_id> ret;
    for( const std::string &it : ids ) {
        std::vector<trait_id> this_id = get_mutations_in_type( it );
        ret.insert( ret.end(), this_id.begin(), this_id.end() );
    }
    return ret;
}
