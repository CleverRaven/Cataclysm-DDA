#include "recipe_dictionary.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>

#include "cata_algo.h"
#include "cata_utility.h"
#include "init.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "output.h"
#include "skill.h"
#include "uistate.h"
#include "debug.h"
#include "json.h"
#include "optional.h"
#include "player.h"
#include "requirements.h"
#include "units.h"
#include "string_id.h"

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

} // namespace

static recipe null_recipe;
static std::set<const recipe *> null_match;

static DynamicDataLoader::deferred_json deferred;

template<>
const recipe &string_id<recipe>::obj() const
{
    const auto iter = recipe_dict.recipes.find( *this );
    if( iter != recipe_dict.recipes.end() ) {
        if( iter->second.obsolete ) {
            return null_recipe;
        }
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

std::vector<const recipe *> recipe_subset::favorite() const
{
    std::vector<const recipe *> res;

    std::copy_if( recipes.begin(), recipes.end(), std::back_inserter( res ), [&]( const recipe * r ) {
        if( !*r || r->obsolete ) {
            return false;
        }
        return uistate.favorite_recipes.find( r->ident() ) != uistate.favorite_recipes.end();
    } );

    return res;
}

std::vector<const recipe *> recipe_subset::hidden() const
{
    std::vector<const recipe *> res;

    std::copy_if( recipes.begin(), recipes.end(), std::back_inserter( res ), [&]( const recipe * r ) {
        if( !*r || r->obsolete ) {
            return false;
        }
        return uistate.hidden_recipes.find( r->ident() ) != uistate.hidden_recipes.end();
    } );

    return res;
}

std::vector<const recipe *> recipe_subset::recent() const
{
    std::vector<const recipe *> res;

    for( auto rec_id = uistate.recent_recipes.rbegin(); rec_id != uistate.recent_recipes.rend();
         ++rec_id ) {
        std::find_if( recipes.begin(), recipes.end(), [&rec_id, &res]( const recipe * r ) {
            if( !*r || *rec_id != r->ident() || r->obsolete ) {
                return false;
            }

            res.push_back( r );
            return true;
        } );
    }

    return res;
}
std::vector<const recipe *> recipe_subset::search( const std::string &txt,
        const search_type key ) const
{
    std::vector<const recipe *> res;

    std::copy_if( recipes.begin(), recipes.end(), std::back_inserter( res ), [&]( const recipe * r ) {
        if( !*r || r->obsolete ) {
            return false;
        }
        switch( key ) {
            case search_type::name:
                return lcmatch( r->result_name(), txt );

            case search_type::skill:
                return lcmatch( r->required_skills_string( nullptr ), txt ) ||
                       lcmatch( r->skill_used->name(), txt );

            case search_type::primary_skill:
                return lcmatch( r->skill_used->name(), txt );

            case search_type::component:
                return search_reqs( r->simple_requirements().get_components(), txt );

            case search_type::tool:
                return search_reqs( r->simple_requirements().get_tools(), txt );

            case search_type::quality:
                return search_reqs( r->simple_requirements().get_qualities(), txt );

            case search_type::quality_result: {
                const auto &quals = item::find_type( r->result() )->qualities;
                return std::any_of( quals.begin(), quals.end(), [&]( const std::pair<quality_id, int> &e ) {
                    return lcmatch( e.first->name, txt );
                } );
            }

            case search_type::description_result: {
                const item result = r->create_result();
                return lcmatch( remove_color_tags( result.info( true ) ), txt );
            }

            default:
                return false;
        }
    } );

    return res;
}

recipe_subset::recipe_subset( const recipe_subset &src, const std::vector<const recipe *> &recipes )
{
    for( const auto elem : recipes ) {
        include( elem, src.get_custom_difficulty( elem ) );
    }
}

recipe_subset recipe_subset::reduce( const std::string &txt, const search_type key ) const
{
    return recipe_subset( *this, search( txt, key ) );
}
recipe_subset recipe_subset::intersection( const recipe_subset &subset ) const
{
    std::vector<const recipe *> intersection_result;
    std::set_intersection( this->begin(), this->end(), subset.begin(), subset.end(),
                           std::back_inserter( intersection_result ) );
    return recipe_subset( *this, intersection_result );
}
recipe_subset recipe_subset::difference( const recipe_subset &subset ) const
{
    std::vector<const recipe *> difference_result;
    std::set_difference( this->begin(), this->end(), subset.begin(), subset.end(),
                         std::back_inserter( difference_result ) );
    return recipe_subset( *this, difference_result );
}

std::vector<const recipe *> recipe_subset::search_result( const itype_id &item ) const
{
    std::vector<const recipe *> res;

    std::copy_if( recipes.begin(), recipes.end(), std::back_inserter( res ), [&]( const recipe * r ) {
        if( r->obsolete ) {
            return false;
        }
        return item == r->result() ||
               ( r->has_byproducts() && r->byproducts.find( item ) != r->byproducts.end() );
    } );

    return res;
}

bool recipe_subset::empty_category( const std::string &cat, const std::string &subcat ) const
{
    if( subcat == "CSC_*_FAVORITE" ) {
        return uistate.favorite_recipes.empty();
    } else if( subcat == "CSC_*_RECENT" ) {
        return uistate.recent_recipes.empty();
    } else if( subcat == "CSC_*_HIDDEN" ) {
        return uistate.hidden_recipes.empty();
    }

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
            std::copy_if( iter->second.begin(), iter->second.end(),
            std::back_inserter( res ), [&]( const recipe * e ) {
                return !e->obsolete;
            } );
        } else {
            std::copy_if( iter->second.begin(), iter->second.end(),
            std::back_inserter( res ), [&subcat]( const recipe * e ) {
                if( e->obsolete ) {
                    return false;
                }
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

void recipe_dictionary::load_recipe( const JsonObject &jo, const std::string &src )
{
    load( jo, src, recipe_dict.recipes );
}

void recipe_dictionary::load_uncraft( const JsonObject &jo, const std::string &src )
{
    load( jo, src, recipe_dict.uncraft );
}

recipe &recipe_dictionary::load( const JsonObject &jo, const std::string &src,
                                 std::map<recipe_id, recipe> &out )
{
    recipe r;

    // defer entries dependent upon as-yet unparsed definitions
    if( jo.has_string( "copy-from" ) ) {
        auto base = recipe_id( jo.get_string( "copy-from" ) );
        if( !out.count( base ) ) {
            deferred.emplace_back( jo.str(), src );
            return null_recipe;
        }
        r = out[ base ];
    }

    r.load( jo, src );

    return out[ r.ident() ] = std::move( r );
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

bool recipe_dictionary::is_item_on_loop( const itype_id &i ) const
{
    return items_on_loops.count( i );
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

        if( elem.obsolete ) {
            return false;
        }

        const std::string error = elem.get_consistency_error();

        if( !error.empty() ) {
            debugmsg( "Recipe %s %s.", elem.ident().c_str(), error.c_str() );
        }

        return !error.empty();
    } );
}

void recipe_dictionary::find_items_on_loops()
{
    // Check for infinite recipe loops in food (which are problematic for
    // nutrient calculations).
    //
    // Start by building a directed graph of itypes to potential components of
    // those itypes.
    items_on_loops.clear();
    std::unordered_map<itype_id, std::vector<itype_id>> potential_components_of;
    for( const itype *i : item_controller->all() ) {
        if( !i->comestible || i->item_tags.count( "NUTRIENT_OVERRIDE" ) ) {
            continue;
        }
        std::vector<itype_id> &potential_components = potential_components_of[i->get_id()];
        for( const recipe_id &rec : i->recipes ) {
            const requirement_data requirements = rec->simple_requirements();
            const requirement_data::alter_item_comp_vector &component_requirements =
                requirements.get_components();

            for( const std::vector<item_comp> &component_options : component_requirements ) {
                for( const item_comp &component_option : component_options ) {
                    potential_components.push_back( component_option.type );
                }
            }
        }
    }

    // Now check that graph for loops
    std::vector<std::vector<itype_id>> loops = cata::find_cycles( potential_components_of );
    for( const std::vector<itype_id> &loop : loops ) {
        std::string error_message =
            "loop in comestible recipes detected: " + loop.back();
        for( const itype_id &i : loop ) {
            error_message += " -> " + i;
            items_on_loops.insert( i );
        }
        error_message += ".  Such loops can be broken by either removing or altering "
                         "recipes or marking one of the items involved with the NUTRIENT_OVERRIDE "
                         "flag";
        debugmsg( error_message );
    }
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

        if( r.obsolete ) {
            continue;
        }

        for( const auto &bk : r.booksets ) {
            const itype *booktype = item::find_type( bk.first );
            int req = bk.second > 0 ? bk.second : std::max( booktype->book->req, r.difficulty );
            islot_book::recipe_with_description_t desc{ &r, req, r.result_name(), false };
            const_cast<islot_book &>( *booktype->book ).recipes.insert( desc );
        }

        // if reversible and no specific uncraft recipe exists use this recipe
        if( r.is_reversible() && !recipe_dict.uncraft.count( recipe_id( r.result() ) ) ) {
            recipe_dict.uncraft[ recipe_id( r.result() ) ] = r;
        }
    }

    // add pseudo uncrafting recipes
    for( const itype *e : item_controller->all() ) {
        const itype_id id = e->get_id();
        const recipe_id rid = recipe_id( id );

        // books that don't already have an uncrafting recipe
        if( e->book && !recipe_dict.uncraft.count( rid ) && e->volume > 0_ml ) {
            int pages = e->volume / 12.5_ml;
            auto &bk = recipe_dict.uncraft[rid];
            bk.ident_ = rid;
            bk.result_ = id;
            bk.reversible = true;
            bk.requirements_ = *requirement_id( "uncraft_book" ) * pages;
            // TODO: allow specifying time in requirement_data
            bk.time = pages * 10;
        }
    }

    // Cache auto-learn recipes and blueprints
    for( const auto &e : recipe_dict.recipes ) {
        if( e.second.autolearn ) {
            recipe_dict.autolearn.insert( &e.second );
        }
        if( e.second.is_blueprint() ) {
            recipe_dict.blueprints.insert( &e.second );
        }
    }

    recipe_dict.find_items_on_loops();
}

void recipe_dictionary::reset()
{
    recipe_dict.blueprints.clear();
    recipe_dict.autolearn.clear();
    recipe_dict.recipes.clear();
    recipe_dict.uncraft.clear();
    recipe_dict.items_on_loops.clear();
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
        for( const auto &opts : r->simple_requirements().get_components() ) {
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
