#include "character_mutations.h"

#include "character.h"
#include "debug.h"
#include "json.h"

void character_mutations::serialize( JsonOut &json ) const
{
    json.member( "mutations", my_mutations );
    json.member( "traits", my_traits );
}

void character_mutations::load_cache_data( Character &guy )
{
    for( auto it = my_mutations.begin(); it != my_mutations.end(); ) {
        const auto &mid = it->first;
        if( mid.is_valid() ) {
            guy.on_mutation_gain( mid );
            cached_mutations.push_back( &mid.obj() );
            ++it;
        } else {
            debugmsg( "character %s has invalid mutation %s, it will be ignored", guy.name, mid.c_str() );
            my_mutations.erase( it++ );
        }
    }
}

character_mutations::trait_data &character_mutations::get_trait_data( const trait_id &mut )
{
    return my_mutations[mut];
}

character_mutations::trait_data character_mutations::get_trait_data( const trait_id &mut ) const
{
    const auto iter = my_mutations.find( mut );
    if( iter == my_mutations.cend() ) {
        return trait_data();
    } else {
        return iter->second;
    }
}

int character_mutations::get_cat_level( const std::string &category ) const
{
    const auto iter = mutation_category_level.find( category );
    if( iter == mutation_category_level.cend() ) {
        return 0;
    } else {
        return iter->second;
    }
}
