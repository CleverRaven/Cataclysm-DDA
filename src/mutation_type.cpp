#include "mutation.h" // IWYU pragma: associated

#include "flexbuffer_json.h"

struct mutation_type {
    std::string id;
};

static std::map<std::string, mutation_type> mutation_types;

void load_mutation_type( const JsonObject &jsobj )
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
    for( const mutation_branch &it : mutation_branch::get_all() ) {
        if( it.types.find( id ) != it.types.end() ) {
            ret.push_back( it.id );
        }
    }
    return ret;
}

std::vector<trait_and_var> mutations_var_in_type( const std::string &id )
{
    std::vector<trait_and_var> ret;
    for( const mutation_branch &it : mutation_branch::get_all() ) {
        if( it.types.find( id ) != it.types.end() ) {
            if( it.variants.empty() ) {
                ret.emplace_back( it.id, "" );
                continue;
            }
            for( const std::pair<const std::string, mutation_variant> &var : it.variants ) {
                ret.emplace_back( it.id, var.second.id );
            }
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
