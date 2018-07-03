#include "recipe_dictionary.h"

#include "itype.h"
#include "generic_factory.h"
#include "item_factory.h"
#include "item.h"
#include "init.h"
#include "cata_utility.h"
#include "crafting.h"
#include "skill.h"

#include <algorithm>
#include <numeric>

recipe_dictionary recipe_dict;

namespace
{

void delete_if( std::map<recipe_id, recipe> &data,
                const std::function<bool( const recipe & )> &pred )
{
    for( auto it = data.begin(); it != data.end(); ) {
        if( pred( it->second ) ) {
            it = data.erase( it );
        } else {
            ++it;
        }
    }
}

}

static recipe null_recipe;
static std::set<const recipe *> null_match;

static DynamicDataLoader::deferred_json deferred;

template<>
const recipe &string_id<recipe>::obj() const
{
    const auto iter = recipe_dict.recipes.find( *this );
    if( iter != recipe_dict.recipes.end() ) {
        return iter->second;
    }
    if( *this != NULL_ID() ) {
        debugmsg( "invalid recipe id \"%s\"", c_str() );
    }
    return null_recipe;
}

template<>
bool string_id<recipe>::is_valid() const
{
    return recipe_dict.recipes.find( *this ) != recipe_dict.recipes.end();
}

const recipe &recipe_dictionary::get_uncraft( const itype_id &id )
{
    auto iter = recipe_dict.uncraft.find( recipe_id( id ) );
    return iter != recipe_dict.uncraft.end() ? iter->second : null_recipe;
}

// searches for left-anchored partial match in the relevant recipe requirements set
template <class group>
bool search_reqs( group gp, const std::string &txt )
{
    return std::any_of( gp.begin(), gp.end(), [&]( const typename group::value_type & opts ) {
        return std::any_of( opts.begin(),
        opts.end(), [&]( const typename group::value_type::value_type & e ) {
            return lcmatch( e.to_string(), txt );
        } );
    } );
}
// template specialization to make component searches easier
template<>
bool search_reqs( std::vector<std::vector<item_comp> >  gp,
                  const std::string &txt )
{
    return std::any_of( gp.begin(), gp.end(), [&]( const std::vector<item_comp> &opts ) {
        return std::any_of( opts.begin(), opts.end(), [&]( const item_comp & ic ) {
            return lcmatch( item::nname( ic.type ), txt );
        } );
    } );
}

std::vector<const recipe *> recipe_subset::search( const std::string &txt,
        const search_type key ) const
{
    std::vector<const recipe *> res;

    std::copy_if( recipes.begin(), recipes.end(), std::back_inserter( res ), [&]( const recipe * r ) {
        switch( key ) {
            case search_type::name:
                return lcmatch( r->result_name(), txt );

            case search_type::skill:
                return lcmatch( r->required_skills_string(), txt ) || lcmatch( r->skill_used->name(), txt );

            case search_type::component:
                return search_reqs( r->requirements().get_components(), txt );

            case search_type::tool:
                return search_reqs( r->requirements().get_tools(), txt );

            case search_type::quality:
                return search_reqs( r->requirements().get_qualities(), txt );

            case search_type::quality_result: {
                const auto &quals = item::find_type( r->result() )->qualities;
                return std::any_of( quals.begin(), quals.end(), [&]( const std::pair<quality_id, int> &e ) {
                    return lcmatch( e.first->name, txt );
                } );
            }

            default:
                return false;
        }
    } );

    return res;
}

bool recipe_subset::empty_category( const std::string &cat,
                                    const std::string &subcat ) const
{
    auto iter = category.find( cat );
    if( iter != category.end() ) {
        if( subcat.empty() ) {
            return false;
        } else {
            for( auto &e : iter->second ) {
                if( e->subcategory == subcat ) {
                    return false;
                }
            }
        }
    }
    return true;
}

std::vector<const recipe *> recipe_subset::in_category( const std::string &cat,
        const std::string &subcat ) const
{
    std::vector<const recipe *> res;
    auto iter = category.find( cat );
    if( iter != category.end() ) {
        if( subcat.empty() ) {
            res.insert( res.begin(), iter->second.begin(), iter->second.end() );
        } else {
            std::copy_if( iter->second.begin(), iter->second.end(),
            std::back_inserter( res ), [&subcat]( const recipe * e ) {
                return e->subcategory == subcat;
            } );
        }
    }
    return res;
}

const std::set<const recipe *> &recipe_subset::of_component( const itype_id &id ) const
{
    auto iter = component.find( id );
    return iter != component.end() ? iter->second : null_match;
}

void recipe_dictionary::load_recipe( JsonObject &jo, const std::string &src )
{
    load( jo, src, recipe_dict.recipes );
}

void recipe_dictionary::load_uncraft( JsonObject &jo, const std::string &src )
{
    load( jo, src, recipe_dict.uncraft );
}

recipe &recipe_dictionary::load( JsonObject &jo, const std::string &src,
                                 std::map<recipe_id, recipe> &dest )
{
    recipe r;

    // defer entries dependent upon as-yet unparsed definitions
    if( jo.has_string( "copy-from" ) ) {
        auto base = recipe_id( jo.get_string( "copy-from" ) );
        if( !dest.count( base ) ) {
            deferred.emplace_back( jo.str(), src );
            return null_recipe;
        }
        r = dest[ base ];
    }

    r.load( jo, src );

    dest[ r.ident() ] = std::move( r );

    return dest[ r.ident() ];
}

size_t recipe_dictionary::size() const
{
    return recipes.size();
}

std::map<recipe_id, recipe>::const_iterator recipe_dictionary::begin() const
{
    return recipes.begin();
}

std::map<recipe_id, recipe>::const_iterator recipe_dictionary::end() const
{
    return recipes.end();
}

void recipe_dictionary::finalize_internal( std::map<recipe_id, recipe> &obj )
{
    for( auto &elem : obj ) {
        elem.second.finalize();
    }
    // remove any blacklisted or invalid recipes...
    delete_if( []( const recipe & elem ) {
        if( elem.is_blacklisted() ) {
            return true;
        }

        const std::string error = elem.get_consistency_error();

        if( !error.empty() ) {
            debugmsg( "Recipe %s %s.", elem.ident().c_str(), error.c_str() );
        }

        return !error.empty();
    } );
}

void recipe_dictionary::finalize()
{
    DynamicDataLoader::get_instance().load_deferred( deferred );

    // remove abstract recipes
    delete_if( []( const recipe & element ) {
        return element.abstract;
    } );

    finalize_internal( recipe_dict.recipes );
    finalize_internal( recipe_dict.uncraft );

    for( auto &e : recipe_dict.recipes ) {
        auto &r = e.second;

        for( const auto &bk : r.booksets ) {
            const itype *booktype = item::find_type( bk.first );
            int req = bk.second > 0 ? bk.second : std::max( booktype->book->req, r.difficulty );
            islot_book::recipe_with_description_t desc{ &r, req, r.result_name(), false };
            const_cast<islot_book &>( *booktype->book ).recipes.insert( desc );
        }

        // if reversible and no specific uncraft recipe exists use this recipe
        if( r.reversible && !recipe_dict.uncraft.count( recipe_id( r.result() ) ) ) {
            recipe_dict.uncraft[ recipe_id( r.result() ) ] = r;
        }
    }

    // add pseudo uncrafting recipes
    for( const itype *e : item_controller->all() ) {
        const itype_id id = e->get_id();
        const recipe_id rid = recipe_id( id );

        // books that don't already have an uncrafting recipe
        if( e->book && !recipe_dict.uncraft.count( rid ) && e->volume > 0 ) {
            int pages = e->volume / units::from_milliliter( 12.5 );
            auto &bk = recipe_dict.uncraft[rid];
            bk.ident_ = rid;
            bk.result_ = id;
            bk.reversible = true;
            bk.requirements_ = *requirement_id( "uncraft_book" ) * pages;
            bk.time = pages * 10; // @todo: allow specifying time in requirement_data
        }
    }

    // Cache auto-learn recipes
    for( const auto &e : recipe_dict.recipes ) {
        if( e.second.autolearn ) {
            recipe_dict.autolearn.insert( &e.second );
        }
    }
}

void recipe_dictionary::reset()
{
    recipe_dict.autolearn.clear();
    recipe_dict.recipes.clear();
    recipe_dict.uncraft.clear();
}

void recipe_dictionary::delete_if( const std::function<bool( const recipe & )> &pred )
{
    ::delete_if( recipe_dict.recipes, pred );
    ::delete_if( recipe_dict.uncraft, pred );
}

void recipe_subset::include( const recipe *r, int custom_difficulty )
{
    if( custom_difficulty < 0 ) {
        custom_difficulty = r->difficulty;
    }
    // We always prefer lower difficulty for the subset, but we save it only if it's not default
    if( recipes.count( r ) > 0 ) {
        const auto iter = difficulties.find( r );
        // See if we need to lower the difficulty of the existing recipe
        if( iter != difficulties.end() && custom_difficulty < iter->second ) {
            if( custom_difficulty != r->difficulty ) {
                iter->second = custom_difficulty; // Added again with lower difficulty
            } else {
                difficulties.erase( iter ); // No need to keep the default difficulty. Free some memory
            }
        } else if( custom_difficulty < r->difficulty ) {
            difficulties[r] = custom_difficulty; // Added again with lower difficulty
        }
    } else {
        // add recipe to category and component caches
        for( const auto &opts : r->requirements().get_components() ) {
            for( const item_comp &comp : opts ) {
                component[comp.type].insert( r );
            }
        }
        category[r->category].insert( r );
        // Set the difficulty is it's not the default
        if( custom_difficulty != r->difficulty ) {
            difficulties[r] = custom_difficulty;
        }
        // insert the recipe
        recipes.insert( r );
    }
}

void recipe_subset::include( const recipe_subset &subset )
{
    for( const auto &elem : subset ) {
        include( elem, subset.get_custom_difficulty( elem ) );
    }
}

int recipe_subset::get_custom_difficulty( const recipe *r ) const
{
    const auto iter = difficulties.find( r );
    if( iter != difficulties.end() ) {
        return iter->second;
    }
    return r->difficulty;
}
