#include "recipe_dictionary.h"

#include "itype.h"
#include "generic_factory.h"
#include "item_factory.h"
#include "init.h"
#include "cata_utility.h"
#include "crafting.h"
#include "skill.h"

#include <algorithm>
#include <numeric>

recipe_dictionary recipe_dict;

static recipe null_recipe;
static std::set<const recipe *> null_match;

static DynamicDataLoader::deferred_json deferred;

const recipe &recipe_dictionary::operator[]( const std::string &id ) const
{
    auto iter = recipes.find( id );
    return iter != recipes.end() ? iter->second : null_recipe;
}

const recipe &recipe_dictionary::get_uncraft( const itype_id &id )
{
    auto iter = recipe_dict.uncraft.find( id );
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

std::vector<const recipe *> recipe_subset::search( const std::string &txt,
        const search_type key ) const
{
    std::vector<const recipe *> res;

    std::copy_if( recipes.begin(), recipes.end(), std::back_inserter( res ), [&]( const recipe * r ) {
        switch( key ) {
            case search_type::name:
                return lcmatch( item::nname( r->result ), txt );

            case search_type::skill:
                return lcmatch( r->required_skills_string(), txt ) || lcmatch( r->skill_used->name(), txt );

            case search_type::component:
                return search_reqs( r->requirements().get_components(), txt );

            case search_type::tool:
                return search_reqs( r->requirements().get_tools(), txt );

            case search_type::quality:
                return search_reqs( r->requirements().get_qualities(), txt );

            case search_type::quality_result: {
                const auto &quals = item::find_type( r->result )->qualities;
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

void recipe_dictionary::load( JsonObject &jo, const std::string &src, bool uncraft )
{
    bool strict = src == "core";

    recipe r;

    // defer entries dependent upon as-yet unparsed definitions
    if( jo.has_string( "copy-from" ) ) {
        auto base = jo.get_string( "copy-from" );
        if( uncraft ) {
            if( !recipe_dict.uncraft.count( base ) ) {
                deferred.emplace_back( jo.str(), src );
                return;
            }
            r = recipe_dict.uncraft[ base ];
        } else {
            if( !recipe_dict.recipes.count( base ) ) {
                deferred.emplace_back( jo.str(), src );
                return;
            }
            r = recipe_dict.recipes[ base ];
        }
    }

    if( jo.has_string( "abstract" ) ) {
        r.ident_ = jo.get_string( "abstract" );
        r.abstract = true;
    } else {
        r.ident_ = r.result = jo.get_string( "result" );
        r.abstract = false;
    }

    if( !uncraft ) {
        if( jo.has_string( "id_suffix" ) ) {
            if( r.abstract ) {
                jo.throw_error( "abstract recipe cannot specify id_suffix", "id_suffix" );
            }
            r.ident_ += "_" + jo.get_string( "id_suffix" );
        }
        assign( jo, "category", r.category, strict );
        assign( jo, "subcategory", r.subcategory, strict );
        assign( jo, "reversible", r.reversible, strict );
    } else {
        r.reversible = true;
    }

    assign( jo, "time", r.time, strict, 0 );
    assign( jo, "difficulty", r.difficulty, strict, 0, MAX_SKILL );
    assign( jo, "flags", r.flags );

    // automatically set contained if we specify as container
    assign( jo, "contained", r.contained, strict );
    r.contained |= assign( jo, "container", r.container, strict );

    if( jo.has_array( "batch_time_factors" ) ) {
        auto batch = jo.get_array( "batch_time_factors" );
        r.batch_rscale = batch.get_int( 0 ) / 100.0;
        r.batch_rsize  = batch.get_int( 1 );
    }

    assign( jo, "charges", r.charges );
    assign( jo, "result_mult", r.result_mult );

    assign( jo, "skill_used", r.skill_used, strict );

    if( jo.has_member( "skills_required" ) ) {
        auto sk = jo.get_array( "skills_required" );
        r.required_skills.clear();

        if( sk.empty() ) {
            // clear all requirements

        } else if( sk.has_array( 0 ) ) {
            // multiple requirements
            while( sk.has_more() ) {
                auto arr = sk.next_array();
                r.required_skills[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }

        } else {
            // single requirement
            r.required_skills[skill_id( sk.get_string( 0 ) )] = sk.get_int( 1 );
        }
    }

    // simplified autolearn sets requirements equal to required skills at finalization
    if( jo.has_bool( "autolearn" ) ) {
        assign( jo, "autolearn", r.autolearn );

    } else if( jo.has_array( "autolearn" ) ) {
        r.autolearn = false;
        auto sk = jo.get_array( "autolearn" );
        while( sk.has_more() ) {
            auto arr = sk.next_array();
            r.autolearn_requirements[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
        }
    }

    if( jo.has_member( "decomp_learn" ) ) {
        r.learn_by_disassembly.clear();

        if( jo.has_int( "decomp_learn" ) ) {
            if( !r.skill_used ) {
                jo.throw_error( "decomp_learn specified with no skill_used" );
            }
            assign( jo, "decomp_learn", r.learn_by_disassembly[r.skill_used] );

        } else if( jo.has_array( "decomp_learn" ) ) {
            auto sk = jo.get_array( "decomp_learn" );
            while( sk.has_more() ) {
                auto arr = sk.next_array();
                r.learn_by_disassembly[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }
        }
    }

    if( !uncraft && jo.has_member( "byproducts" ) ) {
        auto bp = jo.get_array( "byproducts" );
        r.byproducts.clear();
        while( bp.has_more() ) {
            auto arr = bp.next_array();
            r.byproducts[ arr.get_string( 0 ) ] += arr.size() == 2 ? arr.get_int( 1 ) : 1;
        }
    }

    if( jo.has_member( "book_learn" ) ) {
        auto bk = jo.get_array( "book_learn" );
        r.booksets.clear();

        while( bk.has_more() ) {
            auto arr = bk.next_array();
            r.booksets.emplace( arr.get_string( 0 ), arr.get_int( 1 ) );
        }
    }

    // recipes not specifying any external requirements inherit from their parent recipe (if any)
    if( jo.has_string( "using" ) ) {
        r.reqs_external = { { requirement_id( jo.get_string( "using" ) ), 1 } };

    } else if( jo.has_array( "using" ) ) {
        auto arr = jo.get_array( "using" );
        r.reqs_external.clear();

        while( arr.has_more() ) {
            auto cur = arr.next_array();
            r.reqs_external.emplace_back( requirement_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }
    }

    // inline requirements are always replaced (cannot be inherited)
    auto req_id = string_format( "inline_%s_%s", uncraft ? "uncraft" : "recipe", r.ident_.c_str() );
    requirement_data::load_requirement( jo, req_id );
    r.reqs_internal = { { requirement_id( req_id ), 1 } };

    if( uncraft ) {
        recipe_dict.uncraft[ r.ident_ ] = r;
    } else {
        recipe_dict.recipes[ r.ident_ ] = r;
    }
}

size_t recipe_dictionary::size() const
{
    return recipes.size();
}

std::map<std::string, recipe>::const_iterator recipe_dictionary::begin() const
{
    return recipes.begin();
}

std::map<std::string, recipe>::const_iterator recipe_dictionary::end() const
{
    return recipes.end();
}

void recipe_dictionary::finalize_internal( std::map<std::string, recipe> &obj )
{
    for( auto it = obj.begin(); it != obj.end(); ) {
        auto &r = it->second;
        const char *id = it->first.c_str();

        // remove abstract recipes
        if( r.abstract ) {
            it = obj.erase( it );
            continue;
        }

        // concatenate both external and inline requirements
        r.requirements_ = std::accumulate( r.reqs_external.begin(), r.reqs_external.end(), r.requirements_,
        []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
            return lhs + ( *rhs.first * rhs.second );
        } );

        r.requirements_ = std::accumulate( r.reqs_internal.begin(), r.reqs_internal.end(), r.requirements_,
        []( const requirement_data & lhs, const std::pair<requirement_id, int> &rhs ) {
            return lhs + ( *rhs.first * rhs.second );
        } );

        // remove blacklisted recipes
        if( r.requirements().is_blacklisted() ) {
            it = obj.erase( it );
            continue;
        }

        // remove any invalid recipes...
        if( !item::type_is_defined( r.result ) ) {
            debugmsg( "Recipe %s defines invalid result", id );
            it = obj.erase( it );
            continue;
        }

        if( r.charges >= 0 && !item::count_by_charges( r.result ) ) {
            debugmsg( "Recipe %s specified charges but result is not counted by charges", id );
            it = obj.erase( it );
            continue;
        }

        if( std::any_of( r.byproducts.begin(), r.byproducts.end(),
        []( const std::pair<itype_id, int> &bp ) {
        return !item::type_is_defined( bp.first );
        } ) ) {
            debugmsg( "Recipe %s defines invalid byproducts", id );
            it = obj.erase( it );
            continue;
        }

        if( !r.contained && r.container != "null" ) {
            debugmsg( "Recipe %s defines container but not contained", id );
            it = obj.erase( it );
            continue;
        }

        if( !item::type_is_defined( r.container ) ) {
            debugmsg( "Recipe %s specifies unknown container", id );
            it = obj.erase( it );
            continue;
        }

        if( ( r.skill_used && !r.skill_used.is_valid() ) ||
            std::any_of( r.required_skills.begin(),
        r.required_skills.end(), []( const std::pair<skill_id, int> &sk ) {
        return !sk.first.is_valid();
        } ) ) {
            debugmsg( "Recipe %s uses invalid skill", id );
            it = obj.erase( it );
            continue;
        }

        if( std::any_of( r.booksets.begin(), r.booksets.end(), []( const std::pair<itype_id, int> &bk ) {
        return !item::find_type( bk.first )->book;
        } ) ) {
            debugmsg( "Recipe %s defines invalid book", id );
            it = obj.erase( it );
            continue;
        }

        ++it;
    }
}

void recipe_dictionary::finalize()
{
    if( !DynamicDataLoader::get_instance().load_deferred( deferred ) ) {
        debugmsg( "JSON contains circular dependency: discarded %i recipes", deferred.size() );
    }

    finalize_internal( recipe_dict.recipes );
    finalize_internal( recipe_dict.uncraft );

    for( auto &e : recipe_dict.recipes ) {
        auto &r = e.second;

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

        // if reversible and no specific uncraft recipe exists use this recipe
        if( r.reversible && !recipe_dict.uncraft.count( r.result ) ) {
            recipe_dict.uncraft[ r.result ] = r;
        }
    }

    // add pseudo uncrafting recipes
    for( const auto &e : item_controller->get_all_itypes() ) {

        // books that don't alreay have an uncrafting recipe
        if( e.second->book && !recipe_dict.uncraft.count( e.first ) && e.second->volume > 0 ) {
            int pages = e.second->volume / units::from_milliliter( 12.5 );
            auto &bk = recipe_dict.uncraft[ e.first ];
            bk.ident_ = e.first;
            bk.result = e.first;
            bk.reversible = true;
            bk.requirements_ = *requirement_id( "uncraft_book" ) * pages;
            bk.time = pages * 10; // @todo allow specifying time in requirement_data
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
    for( auto it = recipe_dict.recipes.begin(); it != recipe_dict.recipes.end(); ) {
        if( pred( it->second ) ) {
            it = recipe_dict.recipes.erase( it );
        } else {
            ++it;
        }
    }
    for( auto it = recipe_dict.uncraft.begin(); it != recipe_dict.uncraft.end(); ) {
        if( pred( it->second ) ) {
            it = recipe_dict.uncraft.erase( it );
        } else {
            ++it;
        }
    }
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
