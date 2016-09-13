#include "recipe_dictionary.h"

#include "itype.h"

#include <algorithm>
#include <numeric>

recipe_dictionary recipe_dict;

static recipe null_recipe;
static std::set<const recipe *> null_match;

const recipe &recipe_dictionary::operator[]( const std::string &id ) const
{
    auto iter = recipes.find( id );
    return iter != recipes.end() ? iter->second : null_recipe;
}

const std::set<const recipe *> &recipe_dictionary::in_category( const std::string &cat ) const
{
    auto iter = category.find( cat );
    return iter != category.end() ? iter->second : null_match;
}

const std::set<const recipe *> &recipe_dictionary::of_component( const itype_id &id ) const
{
    auto iter = component.find( id );
    return iter != component.end() ? iter->second : null_match;
}

void recipe_dictionary::finalize()
{
    for( auto it = recipe_dict.recipes.begin(); it != recipe_dict.recipes.end(); ) {
        auto &r = it->second;
        const char *id = it->first.c_str();

        // concatenate requirements
        r.requirements_ = std::accumulate( r.reqs.begin(), r.reqs.end(), requirement_data(),
        []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
            return lhs + ( *rhs.first * rhs.second );
        } );

        // remove blacklisted recipes
        if( r.requirements().is_blacklisted() ) {
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        // remove any invalid recipes...
        if( !item::type_is_defined( r.result ) ) {
            debugmsg( "Recipe %s defines invalid result", id );
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        if( r.charges >= 0 && !item::count_by_charges( r.result ) ) {
            debugmsg( "Recipe %s specified charges but result is not counted by charges", id );
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        if( r.result_mult != 1 && !item::count_by_charges( r.result ) ) {
            debugmsg( "Recipe %s has result_mult but result is not counted by charges", id );
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        if( std::any_of( r.byproducts.begin(),
        r.byproducts.end(), []( const std::pair<itype_id, int> &bp ) {
        return !item::type_is_defined( bp.first );
        } ) ) {
            debugmsg( "Recipe %s defines invalid byproducts", id );
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        if( !r.contained && r.container != "null" ) {
            debugmsg( "Recipe %s defines container but not contained", id );
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        if( !item::type_is_defined( r.container ) ) {
            debugmsg( "Recipe %s specifies unknown container", id );
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        if( ( r.skill_used && !r.skill_used.is_valid() ) ||
            std::any_of( r.required_skills.begin(),
        r.required_skills.end(), []( const std::pair<skill_id, int> &sk ) {
        return !sk.first.is_valid();
        } ) ) {
            debugmsg( "Recipe %s uses invalid skill", id );
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        if( std::any_of( r.booksets.begin(), r.booksets.end(), []( const std::pair<itype_id, int> &bk ) {
        return !item::find_type( bk.first )->book;
        } ) ) {
            debugmsg( "Recipe %s defines invalid book", id );
            it = recipe_dict.recipes.erase( it );
            continue;
        }

        // recipe is valid so perform finalization...
        for( const auto &bk : r.booksets ) {
            islot_book::recipe_with_description_t desc{ &r, bk.second, item::nname( r.result ), false };
            item::find_type( bk.first )->book->recipes.insert( desc );
        }

        if( r.contained && r.container == "null" ) {
            r.container = item::find_type( r.result )->default_container;
        }

        if( r.autolearn ) {
            r.autolearn_requirements = r.required_skills;
            if( r.skill_used ) {
                r.autolearn_requirements[ r.skill_used ] = r.difficulty;
            }
        }

        // add recipe to category and component caches
        recipe_dict.category[r.cat].insert( &r );

        for( const auto &opts : r.requirements().get_components() ) {
            for( const item_comp &comp : opts ) {
                recipe_dict.component[comp.type].insert( &r );
            }
        }

        ++it;
    }
}

void recipe_dictionary::reset()
{
    recipe_dict.component.clear();
    recipe_dict.category.clear();
    recipe_dict.recipes.clear();
}

void recipe_dictionary::delete_if( const std::function<bool( const recipe & )> &pred )
{
    for( auto it = recipes.begin(); it != recipes.end(); ) {
        if( pred( it->second ) ) {
            it = recipe_dict.recipes.erase( it );
        } else {
            ++it;
        }
    }
}
