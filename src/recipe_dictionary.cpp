#include "recipe_dictionary.h"
#include "crafting.h"

#include <algorithm> //std::remove

using itype_id = std::string; // From itype.h

recipe_dictionary recipe_dict;

void recipe_dictionary::add( recipe *rec )
{
    recipes.push_back( rec );
    add_to_component_lookup( rec );
    by_name[rec->ident] = rec;
    by_index[rec->id] = rec;
    by_category[rec->cat].push_back( rec );
}

void recipe_dictionary::remove( recipe *rec )
{
    recipes.remove( rec );
    remove_from_component_lookup( rec );
    by_name.erase( rec->ident );
    by_index.erase( rec->id );
    // Terse name for category vector since it's repeated so many times.
    auto &cat_vec = by_category[rec->cat];
    cat_vec.erase( std::remove( cat_vec.begin(), cat_vec.end(), rec ), cat_vec.end() );
}

void recipe_dictionary::delete_if( const std::function<bool( recipe & )> &pred )
{
    for( auto iter = recipes.begin(); iter != recipes.end(); ) {
        recipe *const r = *iter;
        // Already moving to the next, so we can erase the recipe without invalidating `iter`.
        ++iter;
        if( pred( *r ) ) {
            remove( r );
            delete r;
        }
    }
}

void recipe_dictionary::add_to_component_lookup( recipe *r )
{
    std::unordered_set<itype_id> counted;
    for( const auto &comp_choices : r->requirements.get_components() ) {
        for( const item_comp &comp : comp_choices ) {
            if( counted.count( comp.type ) ) {
                continue;
            }
            counted.insert( comp.type );
            by_component[comp.type].push_back( r );
        }
    }
}

void recipe_dictionary::remove_from_component_lookup( recipe *r )
{
    for( auto &map_item : by_component ) {
        std::vector<recipe *> &rlist = map_item.second;
        rlist.erase( std::remove( rlist.begin(), rlist.end(), r ), rlist.end() );
    }
}

void recipe_dictionary::clear()
{
    by_component.clear();
    by_name.clear();
    by_index.clear();
    by_category.clear();
    for( auto &recipe : recipes ) {
        delete recipe;
    }
    recipes.clear();
}

const std::vector<recipe *> &recipe_dictionary::in_category( const std::string &cat )
{
    return by_category[cat];
}

const std::vector<recipe *> &recipe_dictionary::of_component( const itype_id &id )
{
    return by_component[id];
}
