#include "recipe.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>

#include "assign.h"
#include "cached_options.h"
#include "calendar.h"
#include "cartesian_product.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "enum_traits.h"
#include "effect_on_condition.h"
#include "flag.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "itype.h"
#include "json.h"
#include "mapgen_functions.h"
#include "npc.h"
#include "output.h"
#include "proficiency.h"
#include "recipe_dictionary.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_id_utils.h"
#include "translations.h"
#include "type_id.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"

static const itype_id itype_atomic_coffeepot( "atomic_coffeepot" );
static const itype_id itype_hotplate( "hotplate" );
static const itype_id itype_null( "null" );

static const std::string flag_FULL_MAGAZINE( "FULL_MAGAZINE" );

recipe::recipe() : skill_used( skill_id::NULL_ID() ) {}

int recipe::get_difficulty( const Character &crafter ) const
{
    if( is_practice() && skill_used ) {
        return clamp( static_cast<int>( crafter.get_all_skills().get_skill_level( skill_used ) ),
                      practice_data->min_difficulty, practice_data->max_difficulty );
    } else {
        return difficulty;
    }
}

int recipe::get_skill_cap() const
{
    if( is_practice() ) {
        return practice_data->skill_limit - 1;
    } else {
        return difficulty * 1.25;
    }
}

time_duration recipe::batch_duration( const Character &guy, int batch, float multiplier,
                                      size_t assistants ) const
{
    return time_duration::from_turns( batch_time( guy, batch, multiplier, assistants ) / 100 );
}

static bool helpers_have_proficiencies( const Character &guy, const proficiency_id &prof )
{
    for( Character *helper : guy.get_crafting_group() ) {
        if( helper->has_proficiency( prof ) ) {
            return true;
        }
    }
    return false;
}

time_duration recipe::time_to_craft( const Character &guy, recipe_time_flag flags ) const
{
    return time_duration::from_moves( time_to_craft_moves( guy, flags ) );
}

int64_t recipe::time_to_craft_moves( const Character &guy, recipe_time_flag flags ) const
{
    if( flags == recipe_time_flag::ignore_proficiencies ) {
        return time;
    }
    return time * proficiency_time_maluses( guy );
}

int64_t recipe::batch_time( const Character &guy, int batch, float multiplier,
                            size_t assistants ) const
{
    // 1.0f is full speed
    // 0.33f is 1/3 speed
    if( multiplier == 0.0f ) {
        // If an item isn't craftable in the dark, show the time to complete as if you could craft it
        multiplier = 1.0f;
    }

    const double local_time = static_cast<double>( time_to_craft_moves( guy ) ) / multiplier;

    // if recipe does not benefit from batching and we have no assistants, don't do unnecessary additional calculations
    if( batch_rscale == 0.0 && assistants == 0 ) {
        return static_cast<int64_t>( local_time ) * batch;
    }

    double total_time = 0.0;
    // if recipe does not benefit from batching but we do have assistants, skip calculating the batching scale factor
    if( batch_rscale == 0.0f ) {
        total_time = local_time * batch;
    } else {
        // recipe benefits from batching, so batching scale factor needs to be calculated
        // At batch_rsize, incremental time increase is 99.5% of batch_rscale
        const double scale = batch_rsize / 6.0f;
        for( int x = 0; x < batch; x++ ) {
            // scaled logistic function output
            const double logf = ( 2.0 / ( 1.0 + std::exp( -( x / scale ) ) ) ) - 1.0;
            total_time += local_time * ( 1.0 - ( batch_rscale * logf ) );
        }
    }

    //Assistants can decrease the time for production but never less than that of one unit
    if( assistants == 1 ) {
        total_time = total_time * .75;
    } else if( assistants >= 2 ) {
        total_time = total_time * .60;
    }
    if( total_time < local_time ) {
        total_time = local_time;
    }

    return static_cast<int64_t>( total_time );
}

bool recipe::has_flag( const std::string &flag_name ) const
{
    return flags.count( flag_name );
}

void recipe::load( const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    abstract = jo.has_string( "abstract" );

    const std::string type = jo.get_string( "type" );
    if( jo.has_member( "result_eocs" ) ) {
        result_eocs.clear();
        for( JsonValue jv : jo.get_array( "result_eocs" ) ) {
            result_eocs.push_back( effect_on_conditions::load_inline_eoc( jv, src ) );
        }
    }
    if( abstract ) {
        ident_ = recipe_id( jo.get_string( "abstract" ) );
    } else if( type == "practice" ) {
        ident_ = recipe_id( jo.get_string( "id" ) );
        if( jo.has_member( "result" ) ) {
            jo.throw_error_at( "result", "Practice recipes should not have result (use byproducts)" );
        }
        if( jo.has_member( "difficulty" ) ) {
            jo.throw_error_at( "difficulty",
                               "Practice recipes should not have difficulty (use practice_data)" );
        }
    } else if( type == "nested_category" ) {
        ident_ = recipe_id( jo.get_string( "id" ) );
        if( jo.has_member( "result" ) ) {
            jo.throw_error_at( "result", "nested category should not have result" );
        }
        if( jo.has_member( "autolearn" ) ) {
            jo.throw_error_at( "autolearn",
                               "nested category should not have an autolearn, they are only displayed if one of their recipes should be." );
        }
        // nested recipes are never learned directly
        never_learn = true;
    } else {
        if( !jo.read( "result", result_, true ) && !result_ ) {
            if( result_eocs.empty() ) {
                jo.throw_error( "Recipe missing result" );
            } else {
                mandatory( jo, false, "name", name_ );
                ident_ = recipe_id( jo.get_string( "id" ) );
            }
        } else {
            ident_ = recipe_id( result_.str() );
        }
    }

    if( type == "recipe" ) {
        optional( jo, was_loaded, "variant", variant_ );
        if( !variant_.empty() && !abstract ) {
            ident_ = recipe_id( ident_.str() + "_" + variant_ );
        }
        if( jo.has_string( "id_suffix" ) ) {
            if( abstract ) {
                jo.throw_error_at( "id_suffix", "abstract recipe cannot specify id_suffix" );
            }
            ident_ = recipe_id( ident_.str() + "_" + jo.get_string( "id_suffix" ) );
        }
    }

    if( jo.has_bool( "obsolete" ) ) {
        assign( jo, "obsolete", obsolete );
    }

    // If it's an obsolete recipe, we don't need any more data, skip loading
    if( obsolete ) {
        return;
    }

    if( jo.has_string( "time" ) ) {
        time = to_moves<int>( read_from_json_string<time_duration>( jo.get_member( "time" ),
                              time_duration::units ) );
    }
    assign( jo, "difficulty", difficulty, strict, 0, MAX_SKILL );
    assign( jo, "flags", flags );

    // automatically set contained if we specify as container
    assign( jo, "contained", contained, strict );
    contained |= assign( jo, "container", container, strict );
    optional( jo, false, "container_variant", container_variant );
    assign( jo, "sealed", sealed, strict );

    if( jo.has_array( "batch_time_factors" ) ) {
        JsonArray batch = jo.get_array( "batch_time_factors" );
        batch_rscale = batch.get_int( 0 ) / 100.0;
        batch_rsize  = batch.get_int( 1 );
    }

    assign( jo, "charges", charges );
    assign( jo, "result_mult", result_mult );

    assign( jo, "skill_used", skill_used, strict );

    if( jo.has_member( "skills_required" ) ) {
        JsonArray sk = jo.get_array( "skills_required" );
        required_skills.clear();

        if( sk.empty() ) {
            // clear all requirements

        } else if( sk.has_array( 0 ) ) {
            // multiple requirements
            for( JsonArray arr : sk ) {
                required_skills[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }

        } else {
            // single requirement
            required_skills[skill_id( sk.get_string( 0 ) )] = sk.get_int( 1 );
        }
    }

    jo.read( "proficiencies", proficiencies );

    // simplified autolearn sets requirements equal to required skills at finalization
    if( jo.has_bool( "autolearn" ) ) {
        assign( jo, "autolearn", autolearn );

    } else if( jo.has_array( "autolearn" ) ) {
        autolearn = true;
        for( JsonArray arr : jo.get_array( "autolearn" ) ) {
            autolearn_requirements[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
        }
    }

    // Mandatory: This recipe's exertion level
    mandatory( jo, was_loaded, "activity_level", exertion_str );
    // Remove after 0.H
    if( exertion_str == "fake" ) {
        debugmsg( "Depreciated activity level \"fake\" found in recipe %s from source %s. Setting activity level to MODERATE_EXERCISE.",
                  ident_.c_str(), src );
        exertion_str = "MODERATE_EXERCISE";
    }
    const auto it = activity_levels_map.find( exertion_str );
    if( it == activity_levels_map.end() ) {
        jo.throw_error_at(
            "activity_level", string_format( "Invalid activity level %s", exertion_str ) );
    }
    exertion = it->second;

    // Never let the player have a debug or NPC recipe
    if( jo.has_bool( "never_learn" ) ) {
        assign( jo, "never_learn", never_learn );
    }

    if( jo.has_member( "decomp_learn" ) ) {
        learn_by_disassembly.clear();

        if( jo.has_int( "decomp_learn" ) ) {
            if( !skill_used ) {
                jo.throw_error( "decomp_learn specified with no skill_used" );
            }
            assign( jo, "decomp_learn", learn_by_disassembly[skill_used] );

        } else if( jo.has_array( "decomp_learn" ) ) {
            for( JsonArray arr : jo.get_array( "decomp_learn" ) ) {
                learn_by_disassembly[skill_id( arr.get_string( 0 ) )] = arr.get_int( 1 );
            }
        }
    }

    if( jo.has_member( "book_learn" ) ) {
        booksets.clear();
        if( jo.has_array( "book_learn" ) ) {
            for( JsonArray arr : jo.get_array( "book_learn" ) ) {
                booksets.emplace( itype_id( arr.get_string( 0 ) ), book_recipe_data{ arr.size() > 1 ? arr.get_int( 1 ) : -1 } );
            }
        } else {
            mandatory( jo, false, "book_learn", booksets );
        }
    }

    if( jo.has_member( "delete_flags" ) ) {
        flags_to_delete = jo.get_tags<flag_id>( "delete_flags" );
    }

    // recipes not specifying any external requirements inherit from their parent recipe (if any)
    if( jo.has_string( "using" ) ) {
        reqs_external = { { requirement_id( jo.get_string( "using" ) ), 1 } };

    } else if( jo.has_array( "using" ) ) {
        reqs_external.clear();
        for( JsonArray cur : jo.get_array( "using" ) ) {
            reqs_external.emplace_back( requirement_id( cur.get_string( 0 ) ), cur.get_int( 1 ) );
        }
    }

    // inline requirements are always replaced (cannot be inherited)
    reqs_internal.clear();

    // These cannot be inherited
    bp_autocalc = false;
    check_blueprint_needs = false;
    bp_build_reqs.reset();

    if( type == "recipe" ) {

        mandatory( jo, was_loaded, "category", category );
        mandatory( jo, was_loaded, "subcategory", subcategory );
        assign( jo, "description", description, strict );

        if( jo.has_bool( "reversible" ) ) {
            assign( jo, "reversible", reversible, strict );
        } else if( jo.has_object( "reversible" ) ) {
            reversible = true;
            // Convert duration to time in moves
            uncraft_time = to_moves<int>( read_from_json_string<time_duration>
                                          ( jo.get_object( "reversible" ).get_member( "time" ), time_duration::units ) );
        }

        if( jo.has_member( "byproducts" ) ) {
            if( this->reversible ) {
                jo.throw_error( "Recipe cannot be reversible and have byproducts" );
            }
            byproducts.clear();
            for( JsonArray arr : jo.get_array( "byproducts" ) ) {
                itype_id byproduct( arr.get_string( 0 ) );
                byproducts[ byproduct ] += arr.size() == 2 ? arr.get_int( 1 ) : 1;
            }
        }
        if( jo.has_member( "byproduct_group" ) ) {
            if( this->reversible ) {
                jo.throw_error( "Recipe cannot be reversible and have byproducts" );
            }
            byproduct_group = item_group::load_item_group( jo.get_member( "byproduct_group" ),
                              "collection", "byproducts of recipe " + ident_.str() );
        }
        assign( jo, "construction_blueprint", blueprint );
        if( !blueprint.is_empty() ) {
            assign( jo, "blueprint_name", bp_name );
            assign( jo, "blueprint_parameter_names", bp_parameter_names );
            bp_resources.clear();
            for( const std::string resource : jo.get_array( "blueprint_resources" ) ) {
                bp_resources.emplace_back( resource );
            }
            for( JsonObject provide : jo.get_array( "blueprint_provides" ) ) {
                bp_provides.emplace_back( provide.get_string( "id" ),
                                          provide.get_int( "amount", 1 ) );
            }
            // all blueprints provide themselves with needing it written in JSON
            bp_provides.emplace_back( result_.str(), 1 );
            for( JsonObject require : jo.get_array( "blueprint_requires" ) ) {
                bp_requires.emplace_back( require.get_string( "id" ),
                                          require.get_int( "amount", 1 ) );
            }
            // all blueprints exclude themselves with needing it written in JSON
            bp_excludes.emplace_back( result_.str(), 1 );
            for( JsonObject exclude : jo.get_array( "blueprint_excludes" ) ) {
                bp_excludes.emplace_back( exclude.get_string( "id" ),
                                          exclude.get_int( "amount", 1 ) );
            }
            check_blueprint_needs = jo.get_bool( "check_blueprint_needs", true );
            bool has_needs = jo.has_member( "blueprint_needs" );
            if( has_needs && !bp_parameter_names.empty() ) {
                debugmsg( "blueprint %s has parameters but also provides explicit needs; "
                          "these are incompatible", result_.str() );
            }
            if( bp_parameter_names.empty() ) {
                bp_build_reqs = cata::make_value<parameterized_build_reqs>();
                build_reqs &default_reqs =
                    bp_build_reqs->reqs_by_parameters.emplace().first->second;
                const JsonObject jneeds = jo.get_object( "blueprint_needs" );
                if( jneeds.has_member( "time" ) ) {
                    default_reqs.time =
                        to_moves<int>( read_from_json_string<time_duration>(
                                           jneeds.get_member( "time" ), time_duration::units ) );
                }
                if( jneeds.has_member( "skills" ) ) {
                    std::vector<std::pair<skill_id, int>> blueprint_skills;
                    jneeds.read( "skills", blueprint_skills );
                    default_reqs.skills = { blueprint_skills.begin(), blueprint_skills.end() };
                }
                if( jneeds.has_member( "inline" ) ) {
                    const requirement_id req_id( "inline_blueprint_" + type + "_" + ident_.str() );
                    requirement_data::load_requirement( jneeds.get_object( "inline" ), req_id );
                    default_reqs.raw_reqs.emplace( req_id, 1 );
                }
            }

            if( check_blueprint_needs && !has_needs ) {
                bp_autocalc = true;
            }
        }
    } else if( type == "practice" ) {
        mandatory( jo, false, "name", name_ );
        mandatory( jo, was_loaded, "category", category );
        mandatory( jo, was_loaded, "subcategory", subcategory );
        assign( jo, "description", description, strict );
        mandatory( jo, was_loaded, "practice_data", practice_data );

        if( jo.has_member( "byproducts" ) ) {
            byproducts.clear();
            for( JsonArray arr : jo.get_array( "byproducts" ) ) {
                itype_id byproduct( arr.get_string( 0 ) );
                byproducts[ byproduct ] += arr.size() == 2 ? arr.get_int( 1 ) : 1;
            }
        }
        if( jo.has_member( "byproduct_group" ) ) {
            if( this->reversible ) {
                jo.throw_error( "Recipe cannot be reversible and have byproducts" );
            }
            byproduct_group = item_group::load_item_group( jo.get_member( "byproduct_group" ),
                              "collection", "byproducts of recipe " + ident_.str() );
        }
    } else if( type == "uncraft" ) {
        reversible = true;
    } else if( type == "nested_category" ) {
        mandatory( jo, false, "name", name_ );
        mandatory( jo, was_loaded, "category", category );
        mandatory( jo, was_loaded, "subcategory", subcategory );
        assign( jo, "description", description, strict );
        mandatory( jo, was_loaded, "nested_category_data", nested_category_data );

    } else {
        jo.throw_error_at( "type", "unknown recipe type" );
    }

    const requirement_id req_id( "inline_" + type + "_" + ident_.str() );
    requirement_data::load_requirement( jo, req_id );
    reqs_internal.emplace_back( req_id, 1 );
}

static cata::value_ptr<parameterized_build_reqs> calculate_all_blueprint_reqs(
    const update_mapgen_id &id,
    const std::map<std::string, std::map<std::string, translation>> &bp_params )
{
    cata::value_ptr<parameterized_build_reqs> result = cata::make_value<parameterized_build_reqs>();
    const std::vector<std::unique_ptr<update_mapgen_function_json>> &funcs = id->funcs();
    if( funcs.size() != 1 ) {
        debugmsg( "update_mapgen %s used for blueprint, but has %zu versions, where it should have exactly one",
                  funcs.size() );
        return result;
    }

    // We gather the mapgen params from the mapgen function
    const update_mapgen_function_json &json_func = *funcs.front();
    const mapgen_parameters &mapgen_params = json_func.get_parameters();

    std::map<std::string, std::vector<std::string>> mapgen_param_names_and_possible_values;
    for( const std::pair<const std::string, mapgen_parameter> &mapgen_param : mapgen_params.map ) {
        mapgen_param_names_and_possible_values.emplace(
            mapgen_param.first, mapgen_param.second.all_possible_values( mapgen_params ) );
    }

    std::vector<std::string> mapgen_param_names;
    mapgen_param_names.reserve( mapgen_param_names_and_possible_values.size() );
    for( const std::pair<const std::string, std::vector<std::string>> &name_and_values :
         mapgen_param_names_and_possible_values ) {
        mapgen_param_names.push_back( name_and_values.first );
    }
    cata_assert( std::is_sorted( mapgen_param_names.begin(), mapgen_param_names.end() ) );

    // Then we gather the blueprint params from the recipe
    std::vector<std::string> bp_param_names;
    bp_param_names.reserve( bp_params.size() );
    for( const auto &bp_param : bp_params ) {
        bp_param_names.push_back( bp_param.first );
    }
    // NOLINTNEXTLINE(cata-use-localized-sorting)
    cata_assert( std::is_sorted( bp_param_names.begin(), bp_param_names.end() ) );

    // And check they match
    if( mapgen_param_names != bp_param_names ) {
        debugmsg( "parameter names listed in blueprint do not match those from corresponding "
                  "update_mapgen id %s.\nBlueprint had %s.\nupdate_mapgen had %s.",
                  id.str(),
                  enumerate_as_string( bp_param_names ),
                  enumerate_as_string( mapgen_param_names ) );
        return result;
    }

    // Furthermore, we check that, for each parameter, the set of possible
    // values in the mapgen matches the set of translated ids in the blueprint
    for( const auto &bp_param : bp_params ) {
        const std::string &name = bp_param.first;

        std::vector<std::string> &mapgen_values = mapgen_param_names_and_possible_values[name];
        // NOLINTNEXTLINE(cata-use-localized-sorting)
        std::sort( mapgen_values.begin(), mapgen_values.end() );

        std::vector<std::string> bp_values;
        bp_values.reserve( bp_param.second.size() );
        for( const std::pair<const std::string, translation> &p : bp_param.second ) {
            bp_values.push_back( p.first );
        }
        cata_assert( std::is_sorted( bp_values.begin(), bp_values.end() ) );

        if( mapgen_values != bp_values ) {
            debugmsg( "parameter values for parameter %s listed in blueprint do not match those "
                      "from corresponding update_mapgen id %s.\nBlueprint had %s.\n"
                      "update_mapgen had %s.",
                      name,
                      id.str(),
                      enumerate_as_string( bp_values ),
                      enumerate_as_string( mapgen_values ) );
            return result;
        }
    }

    // Each inner vector is a list of (param_name, param_value) pairs where
    // every param_name is the same, and each param_value is the set of
    // possible values for that param_name.  Once these lists are assembled we
    // will need to iterate over their cartesian product to get all the
    // possible options for parameter choices.
    std::vector<std::vector<std::pair<std::string, cata_variant>>> option_lists;

    for( const std::string &param_name : bp_param_names ) {
        option_lists.emplace_back();
        const mapgen_parameter &mapgen_param = mapgen_params.map.at( param_name );
        cata_variant_type param_type = mapgen_param.type();

        for( const std::pair<const std::string, translation> &p : bp_params.at( param_name ) ) {
            cata_variant param_value =
                cata_variant::from_string( param_type, std::string( p.first ) );
            option_lists.back().emplace_back( param_name, param_value );
        }
    }

    for( const std::vector<std::pair<std::string, cata_variant>> &chosen_params :
         cata::cartesian_product( option_lists ) ) {
        result->reqs_by_parameters.emplace(
            chosen_params,
            get_build_reqs_for_furn_ter_ids(
                get_changed_ids_from_update( id, mapgen_arguments{ chosen_params } ) ) );
    }

    return result;
}

void recipe::finalize()
{
    if( bp_autocalc ) {
        bp_build_reqs = calculate_all_blueprint_reqs( blueprint, bp_parameter_names );
    } else if( test_mode && check_blueprint_needs ) {
        check_blueprint_requirements();
    }

    if( !blueprint.is_empty() ) {
        incorporate_build_reqs();
    } else {
        // concatenate both external and inline requirements
        add_requirements( reqs_external );
        add_requirements( reqs_internal );

        reqs_external.clear();
        reqs_internal.clear();

        deduped_requirements_ = deduped_requirement_data( requirements_, ident() );
    }

    if( contained && container.is_null() ) {
        container = item::find_type( result_ )->default_container.value_or( itype_null );
        if( item::find_type( result_ )->default_container_variant.has_value() ) {
            container_variant = item::find_type( result_ )->default_container_variant.value();
        }
    }

    std::set<proficiency_id> required;
    std::set<proficiency_id> used;
    for( recipe_proficiency &rpof : proficiencies ) {
        if( !rpof.id.is_valid() ) {
            debugmsg( "proficiency %s does not exist in recipe %s", rpof.id.str(), ident_.str() );
        }

        if( rpof.required && rpof.time_multiplier != 0.0f ) {
            debugmsg( "proficiencies in recipes cannot be both required and provide a malus in %s",
                      rpof.id.str(), ident_.str() );
        }
        if( required.count( rpof.id ) || used.count( rpof.id ) ) {
            debugmsg( "proficiency %s listed twice recipe %s", rpof.id.str(),
                      ident_.str() );
        }

        if( rpof.time_multiplier < 1.0f && rpof.id->default_time_multiplier() < 1.0f ) {
            debugmsg( "proficiency %s provides a time bonus for not being known in recipe %s.  Time multiplier: %s Default multiplier: %s",
                      rpof.id.str(), ident_.str(), rpof.time_multiplier, rpof.id->default_time_multiplier() );
        }

        if( rpof.time_multiplier == 0.0f ) {
            rpof.time_multiplier = rpof.id->default_time_multiplier();
        }

        if( !rpof._skill_penalty_assigned ) {
            rpof.skill_penalty = rpof.id->default_skill_penalty();
        }

        if( rpof.skill_penalty < 0.f && rpof.id->default_skill_penalty() < 0.f ) {
            debugmsg( "proficiency %s provides a skill bonus for not being known in recipe %s skill penalty: %g default multiplier: %g",
                      rpof.id.str(), ident_.str(), rpof.skill_penalty, rpof.id->default_skill_penalty() );
        }

        // Now that we've done the error checking, log that a proficiency with this id is used
        if( rpof.required ) {
            required.insert( rpof.id );
        } else {
            used.insert( rpof.id );
        }
    }

    if( autolearn && autolearn_requirements.empty() ) {
        autolearn_requirements = required_skills;
        if( skill_used ) {
            autolearn_requirements[ skill_used ] = difficulty;
        }
    }
}

void recipe::add_requirements( const std::vector<std::pair<requirement_id, int>> &reqs )
{
    requirements_ = std::accumulate( reqs.begin(), reqs.end(), requirements_ );
}

std::string recipe::get_consistency_error() const
{
    if( category == "CC_BUILDING" ) {
        if( is_blueprint() || oter_str_id( result_.c_str() ).is_valid() ) {
            return std::string();
        }
        return "defines invalid result";
    }

    if( !item::type_is_defined( result_ ) ) {
        return "defines invalid result";
    }

    const auto is_invalid_bp = []( const std::pair<itype_id, int> &elem ) {
        return !item::type_is_defined( elem.first );
    };

    if( std::any_of( byproducts.begin(), byproducts.end(), is_invalid_bp ) ) {
        return "defines invalid byproducts";
    }

    if( !contained && !container.is_null() ) {
        return "defines container but not contained";
    }

    if( !item::type_is_defined( container ) ) {
        return "specifies unknown container";
    }

    const auto is_invalid_skill = []( const std::pair<skill_id, int> &elem ) {
        return !elem.first.is_valid();
    };

    if( ( skill_used && !skill_used.is_valid() ) ||
        std::any_of( required_skills.begin(), required_skills.end(), is_invalid_skill ) ) {
        return "uses invalid skill";
    }

    const auto is_invalid_book = []( const std::pair<itype_id, book_recipe_data> &elem ) {
        return !item::find_type( elem.first )->book;
    };

    if( std::any_of( booksets.begin(), booksets.end(), is_invalid_book ) ) {
        return "defines invalid book";
    }

    return std::string();
}

static void set_new_comps( item &newit, int amount, item_components *used, bool is_food,
                           bool is_cooked )
{
    if( is_food ) {
        newit.components = *used;
        newit.recipe_charges = amount;
    } else {
        newit.components = used->split( amount, 0, is_cooked );
    }
}

std::vector<item> recipe::create_result( bool set_components, bool is_food,
        item_components *used ) const
{
    item newit( result_, calendar::turn, item::default_charges_tag{} );

    if( !variant().empty() ) {
        newit.set_itype_variant( variant() );
    }

    if( newit.has_flag( flag_VARSIZE ) ) {
        newit.set_flag( flag_FIT );
    }

    // Newly-crafted items are perfect by default. Inspect their materials to see if they shouldn't be
    if( used ) {
        newit.inherit_flags( *used, *this );
    }

    for( const flag_id &flag : flags_to_delete ) {
        newit.unset_flag( flag );
    }

    // If the recipe has a `FULL_MAGAZINE` flag, fill it with ammo
    if( newit.is_magazine() && has_flag( flag_FULL_MAGAZINE ) ) {
        newit.ammo_set( newit.ammo_default(),
                        newit.ammo_capacity( item::find_type( newit.ammo_default() )->ammo->type ) );
    }

    int amount = charges ? *charges : newit.count();

    bool is_cooked = hot_result() || removes_raw();
    if( set_components ) {
        set_new_comps( newit, amount, used, is_food, is_cooked );
    }

    if( contained ) {
        newit = newit.in_container( container, amount, sealed, container_variant );
        return { newit };
    } else if( newit.count_by_charges() ) {
        newit.charges = amount;
        return { newit };
    } else {
        std::vector<item> items;
        for( int i = 0; i < amount; i++ ) {
            if( set_components ) {
                set_new_comps( newit, amount, used, is_food, is_cooked );
            }
            items.push_back( newit );
        }
        return items;
    }
}

std::vector<item> recipe::create_results( int batch, item_components *used ) const
{
    std::vector<item> items;

    for( int i = 0; i < batch; i++ ) {
        item_components batch_comps;
        item temp( result_ );
        bool is_uncraftable = recipe_dictionary::get_uncraft( result_ ) && !result_->count_by_charges() &&
                              is_reversible();
        bool is_food_no_override = temp.is_food() && !temp.has_flag( flag_NUTRIENT_OVERRIDE );
        bool set_components = used && ( is_uncraftable || is_food_no_override );
        bool is_cooked = hot_result() || removes_raw();
        if( set_components ) {
            batch_comps = used->split( batch, i, is_cooked );
        }
        for( int j = 0; j < result_mult; j++ ) {
            item_components mult_comps = batch_comps.split( result_mult, j, is_cooked );
            std::vector<item> newits = create_result( set_components, temp.is_food(), &mult_comps );

            if( !result_->count_by_charges() ) {
                items.reserve( items.size() + newits.size() );
                items.insert( items.end(), newits.begin(), newits.end() );
            } else {
                for( const item &it : newits ) {
                    // try to combine batch results for liquid handling
                    auto found = std::find_if( items.begin(), items.end(), [it]( const item & rhs ) {
                        return it.can_combine( rhs );
                    } );
                    if( found != items.end() ) {
                        found->combine( it );
                    } else {
                        items.emplace_back( it );
                    }
                }
            }
        }
    }

    return items;
}

static void create_byproducts_legacy( const std::map<itype_id, int> &bplist,
                                      std::vector<item> &bps_out, int batch )
{
    for( const auto &e : bplist ) {
        item obj( e.first, calendar::turn, item::default_charges_tag{} );
        if( obj.has_flag( flag_VARSIZE ) ) {
            obj.set_flag( flag_FIT );
        }
        if( obj.count_by_charges() ) {
            obj.charges *= e.second * batch;
            bps_out.push_back( obj );
        } else {
            if( !obj.craft_has_charges() ) {
                obj.charges = 0;
            }
            for( int i = 0; i < e.second * batch; ++i ) {
                bps_out.push_back( obj );
            }
        }
    }
}

static void create_byproducts_group( const item_group_id &bplist, std::vector<item> &bps_out,
                                     int batch )
{
    std::vector<item> ret = item_group::items_from( bplist, calendar::turn );
    for( item &it : ret ) {
        if( it.count_by_charges() ) {
            it.charges *= batch;
        } else {
            if( !it.craft_has_charges() ) {
                it.charges = 0;
            }
            for( int i = 1; i < batch; ++i ) {
                bps_out.push_back( it );
            }
        }
    }
    bps_out.insert( bps_out.end(), ret.begin(), ret.end() );
}

std::vector<item> recipe::create_byproducts( int batch ) const
{
    std::vector<item> bps;
    if( !byproducts.empty() ) {
        create_byproducts_legacy( byproducts, bps, batch );
    }
    if( !!byproduct_group ) {
        create_byproducts_group( *byproduct_group, bps, batch );
    }
    return bps;
}

std::map<itype_id, int> recipe::get_byproducts() const
{
    std::map<itype_id, int> ret;
    if( !byproducts.empty() ) {
        for( const std::pair<const itype_id, int> &byproduct : byproducts ) {
            int count = byproduct.first->count_by_charges() ? byproduct.first->charges_default() : 1;
            ret.emplace( byproduct.first, byproduct.second * count );
        }
    }
    if( !!byproduct_group ) {
        std::vector<item> tmp = item_group::items_from( *byproduct_group );
        for( const item &i : tmp ) {
            if( i.count_by_charges() ) {
                ret.emplace( i.typeId(), i.charges );
            } else {
                ret.emplace( i.typeId(), 1 );
            }
        }
    }
    return ret;
}

bool recipe::has_byproducts() const
{
    return !byproducts.empty() || !!byproduct_group;
}

bool recipe::in_byproducts( const itype_id &it ) const
{
    return ( !byproducts.empty() && byproducts.find( it ) != byproducts.end() ) ||
           ( !!byproduct_group && item_group::group_contains_item( *byproduct_group, it ) );
}

std::string recipe::required_proficiencies_string( const Character *c ) const
{
    std::vector<proficiency_id> required_profs;

    for( const recipe_proficiency &rec : proficiencies ) {
        if( rec.required ) {
            required_profs.push_back( rec.id );
        }
    }
    std::string required = enumerate_as_string( required_profs.begin(),
    required_profs.end(), [&]( const proficiency_id & id ) {
        nc_color color;
        if( c != nullptr && c->has_proficiency( id ) ) {
            color = c_green;
        } else if( c != nullptr && helpers_have_proficiencies( *c, id ) ) {
            color = c_yellow;
        } else {
            color = c_red;
        }
        return colorize( id->name(), color );
    } );

    return required;
}

struct prof_penalty {
    proficiency_id id;
    float time_mult;
    float skill_penalty;
    bool mitigated = false;
};

static std::string profstring( const prof_penalty &prof,
                               std::string &color,
                               const std::string &name_color = "cyan" )
{
    std::string mitigated_str;
    if( prof.mitigated ) {
        mitigated_str = _( " (Mitigated)" );
    }

    if( prof.time_mult == 1.0f ) {
        return string_format( _( "<color_%s>%s</color> (<color_%s>%.2f\u00a0skill bonus</color>%s)" ),
                              name_color, prof.id->name(), color, prof.skill_penalty, mitigated_str );
    } else if( prof.skill_penalty == 0.0f ) {
        return string_format( _( "<color_%s>%s</color> (<color_%s>%.2fx\u00a0time</color>%s)" ),
                              name_color, prof.id->name(), color, prof.time_mult, mitigated_str );
    }

    return string_format(
               _( "<color_%s>%s</color> (<color_%s>%.2fx\u00a0time, %.2f\u00a0skill bonus</color>%s)" ),
               name_color, prof.id->name(), color, prof.time_mult, prof.skill_penalty, mitigated_str );
}

std::string recipe::used_proficiencies_string( const Character *c ) const
{
    if( c == nullptr ) {
        return { };
    }
    std::vector<prof_penalty> used_profs;

    for( const recipe_proficiency &rec : proficiencies ) {
        if( !rec.required ) {
            if( c->has_proficiency( rec.id ) || helpers_have_proficiencies( *c, rec.id ) )  {
                used_profs.push_back( { rec.id, rec.time_multiplier, rec.skill_penalty} );
            }
        }
    }

    std::string color = "light_green";
    std::string used = enumerate_as_string( used_profs.begin(),
    used_profs.end(), [&]( const prof_penalty & prof ) {
        return profstring( prof, color );
    } );

    return used;
}

std::string recipe::recipe_proficiencies_string() const
{
    std::vector<proficiency_id> profs;

    profs.reserve( proficiencies.size() );
    for( const recipe_proficiency &rec : proficiencies ) {
        profs.push_back( rec.id );
    }
    std::string list = enumerate_as_string( profs.begin(),
    profs.end(), [&]( const proficiency_id & id ) {
        return id->name();
    } );

    return list;
}

std::vector<proficiency_id> recipe::required_proficiencies() const
{
    std::vector<proficiency_id> ret;
    for( const recipe_proficiency &rec : proficiencies ) {
        if( rec.required ) {
            ret.emplace_back( rec.id );
        }
    }
    return ret;
}

bool recipe::character_has_required_proficiencies( const Character &c ) const
{
    for( const proficiency_id &id : required_proficiencies() ) {
        if( !c.has_proficiency( id ) && !helpers_have_proficiencies( c, id ) ) {
            return false;
        }
    }
    return true;
}

std::vector<proficiency_id> recipe::used_proficiencies() const
{
    std::vector<proficiency_id> ret;
    for( const recipe_proficiency &rec : proficiencies ) {
        if( !rec.required ) {
            ret.emplace_back( rec.id );
        }
    }
    return ret;
}

static float get_aided_proficiency_level( const Character &crafter, const proficiency_id &prof )
{
    float max_prof = crafter.get_proficiency_practice( prof );
    for( const Character *helper : crafter.get_crafting_group() ) {
        max_prof = std::max( max_prof, helper->get_proficiency_practice( prof ) );
    }
    return max_prof;
}

static float proficiency_time_malus( const Character &crafter, const recipe_proficiency &prof )
{
    if( !crafter.has_proficiency( prof.id )
        && !helpers_have_proficiencies( crafter, prof.id )
        && prof.time_multiplier > 1.0f
      ) {
        double malus = prof.time_multiplier - 1.0;
        malus *= 1.0 - crafter.crafting_inventory().get_book_proficiency_bonuses().time_factor( prof.id );
        double pl = get_aided_proficiency_level( crafter, prof.id );
        // Sigmoid function that mitigates 100% of the time malus as pl approaches 1.0
        // but has little effect at pl < 0.5. See #49198
        malus *= 1.0 - std::pow( 0.5 - 0.5 * std::cos( pl * M_PI ), 2 );
        return static_cast<float>( 1.0 + malus );
    }
    return 1.0f;
}

float recipe::proficiency_time_maluses( const Character &crafter ) const
{
    float total_malus = 1.0f;
    for( const recipe_proficiency &prof : proficiencies ) {
        total_malus *= proficiency_time_malus( crafter, prof );
    }
    return total_malus;
}

float recipe::max_proficiency_time_maluses( const Character & ) const
{
    float total_malus = 1.0f;
    for( const recipe_proficiency &prof : proficiencies ) {
        total_malus *= prof.time_multiplier;
    }
    return total_malus;
}

static float proficiency_skill_malus( const Character &crafter, const recipe_proficiency &prof )
{
    if( !crafter.has_proficiency( prof.id )
        && !helpers_have_proficiencies( crafter, prof.id )
        && prof.skill_penalty > 0.f
      ) {
        double malus =  prof.skill_penalty;
        malus *= 1.0 - crafter.crafting_inventory().get_book_proficiency_bonuses().fail_factor( prof.id );
        double pl = get_aided_proficiency_level( crafter, prof.id );
        // The failure malus is not completely eliminated until the proficiency is mastered.
        // Most of the mitigation happens at higher pl. See #49198
        malus *= 1.0 - ( 0.75 * std::pow( pl, 3 ) );
        return static_cast<float>( malus );
    }
    return 0.0f;
}

float recipe::proficiency_skill_maluses( const Character &crafter ) const
{
    float total_malus = 0.f;
    for( const recipe_proficiency &prof : proficiencies ) {
        total_malus += proficiency_skill_malus( crafter, prof );
    }
    return total_malus;
}

float recipe::max_proficiency_skill_maluses( const Character & ) const
{
    float total_malus = 0.f;
    for( const recipe_proficiency &prof : proficiencies ) {
        total_malus += prof.skill_penalty;
    }
    return total_malus;
}

std::string recipe::missing_proficiencies_string( const Character *crafter ) const
{
    if( crafter == nullptr ) {
        return { };
    }
    std::vector<prof_penalty> missing_profs;

    const book_proficiency_bonuses book_bonuses =
        crafter->crafting_inventory().get_book_proficiency_bonuses();
    for( const recipe_proficiency &prof : proficiencies ) {
        if( !prof.required ) {
            if( !( crafter->has_proficiency( prof.id ) || helpers_have_proficiencies( *crafter, prof.id ) ) ) {
                prof_penalty pen = { prof.id,
                                     proficiency_time_malus( *crafter, prof ),
                                     proficiency_skill_malus( *crafter, prof )
                                   };
                pen.mitigated = book_bonuses.time_factor( pen.id ) != 0.0f ||
                                book_bonuses.fail_factor( pen.id ) != 0.0f;
                missing_profs.push_back( pen );
            }
        }
    }

    std::string color = "dark_gray";
    std::string missing = enumerate_as_string( missing_profs.begin(),
    missing_profs.end(), [&]( const prof_penalty & prof ) {
        return profstring( prof, color, crafter->has_prof_prereqs( prof.id ) ? "cyan" : "red" );
    } );

    return missing;
}

float recipe::exertion_level() const
{
    return exertion;
}

// Format a vector of std::pair<skill_id, int> for the crafting menu.
// skill colored green, yellow or red according to character skill
// with the skill level (player / difficulty)
static std::string required_skills_as_string( const std::vector<std::pair<skill_id, int>> &skills,
        const Character &c )
{
    if( skills.empty() ) {
        return _( "<color_cyan>none</color>" );
    }
    return enumerate_as_string( skills,
    [&]( const std::pair<skill_id, int> &skill ) {
        const int player_skill = c.get_skill_level( skill.first );
        std::string difficulty_color;
        if( skill.second <= player_skill ) {
            difficulty_color = "green";
        } else if( static_cast<int>( skill.second * 0.8 ) <= player_skill ) {
            difficulty_color = "yellow";
        } else {
            difficulty_color = "red";
        }
        return string_format( "<color_cyan>%s</color> <color_%s>(%d/%d)</color>", skill.first->name(),
                              difficulty_color, player_skill, skill.second );
    } );
}

// Format a vector of std::pair<skill_id, int> for the basecamp bulletin board.
// skill colored white with difficulty in parenthesis.
static std::string required_skills_as_string( const std::vector<std::pair<skill_id, int>> &skills )
{
    if( skills.empty() ) {
        return _( "<color_cyan>none</color>" );
    }
    return enumerate_as_string( skills,
    [&]( const std::pair<skill_id, int> &skill ) {
        return string_format( "<color_white>%s (%d)</color>", skill.first->name(), skill.second );
    } );
}

std::string recipe::primary_skill_string( const Character &c ) const
{
    std::vector<std::pair<skill_id, int>> skillList;

    if( skill_used ) {
        skillList.emplace_back( skill_used, get_difficulty( c ) );
    }

    return required_skills_as_string( skillList, c );
}

std::string recipe::required_skills_string( const Character &c ) const
{
    return required_skills_as_string( sorted_lex( required_skills ), c );
}

std::string recipe::required_all_skills_string( const std::map<skill_id, int> &skills ) const
{
    std::vector<std::pair<skill_id, int>> skillList = sorted_lex( skills );
    // There is primary skill used, add it to the front
    if( skill_used ) {
        skillList.insert( skillList.begin(), std::pair<skill_id, int>( skill_used, difficulty ) );
    }
    return required_skills_as_string( skillList );
}

std::string recipe::batch_savings_string() const
{
    return ( batch_rsize != 0 ) ?
           string_format( _( "%d%% at >%d units" ), static_cast<int>( batch_rscale * 100 ), batch_rsize )
           : _( "none" );
}

std::string recipe::result_name( const bool decorated ) const
{
    std::string name;
    if( !name_.empty() ) {
        name = name_.translated();
    } else if( !variant().empty() ) {
        auto iter_var = std::find_if( result_->variants.begin(), result_->variants.end(),
        [this]( const itype_variant_data & itvar ) {
            return itvar.id == variant();
        } );
        if( iter_var != result_->variants.end() ) {
            name = iter_var->alt_name.translated();
        }
    } else {
        name = item::nname( result_ );
    }
    if( decorated &&
        uistate.favorite_recipes.find( this->ident() ) != uistate.favorite_recipes.end() ) {
        name = "* " + name;
    }
    return name;
}

bool recipe::will_be_blacklisted() const
{
    if( requirements_.is_blacklisted() ) {
        return true;
    }

    auto any_is_blacklisted = []( const std::vector<std::pair<requirement_id, int>> &reqs ) {
        auto req_is_blacklisted = []( const std::pair<requirement_id, int> &req ) {
            return req.first->is_blacklisted();
        };

        return std::any_of( reqs.begin(), reqs.end(), req_is_blacklisted );
    };

    return any_is_blacklisted( reqs_internal ) || any_is_blacklisted( reqs_external );
}

std::function<bool( const item & )> recipe::get_component_filter(
    const recipe_filter_flags flags ) const
{
    const item result( result_ );

    // Disallow crafting of non-perishables with rotten components
    // Make an exception for items with the ALLOW_ROTTEN flag such as seeds
    const bool recipe_forbids_rotten =
        result.is_food() && !result.goes_bad() && !has_flag( "ALLOW_ROTTEN" );
    const bool flags_forbid_rotten =
        static_cast<bool>( flags & recipe_filter_flags::no_rotten );
    const bool flags_forbid_favorites =
        static_cast<bool>( flags & recipe_filter_flags::no_favorite );
    std::function<bool( const item & )> rotten_filter = return_true<item>;
    if( recipe_forbids_rotten || flags_forbid_rotten ) {
        rotten_filter = []( const item & component ) {
            return !component.rotten();
        };
    }

    // Disallow crafting using favorited items as components
    std::function<bool( const item & )> favorite_filter = return_true<item>;
    if( flags_forbid_favorites ) {
        favorite_filter = []( const item & component ) {
            return !component.is_favorite;
        };
    }

    // If the result is made hot, we can allow frozen components.
    // EDIBLE_FROZEN components ( e.g. flour, chocolate ) are allowed as well
    // Otherwise forbid them
    std::function<bool( const item & )> frozen_filter = return_true<item>;
    if( result.has_temperature() && !hot_result() ) {
        frozen_filter = []( const item & component ) {
            return !component.has_flag( flag_FROZEN ) || component.has_flag( flag_EDIBLE_FROZEN );
        };
    }

    // Disallow usage of non-full magazines as components
    // This is primarily used to require a fully charged battery, but works for any magazine.
    std::function<bool( const item & )> magazine_filter = return_true<item>;
    if( has_flag( "NEED_FULL_MAGAZINE" ) ) {
        magazine_filter = []( const item & component ) {
            if( component.ammo_remaining() == 0 ) {
                return false;
            }
            return !component.is_magazine() ||
                   ( component.ammo_remaining() >= component.ammo_capacity( component.ammo_data()->ammo->type ) );
        };
    }

    return [ rotten_filter, favorite_filter, frozen_filter,
                   magazine_filter ]( const item & component ) {
        return is_crafting_component( component ) &&
               rotten_filter( component ) &&
               favorite_filter( component ) &&
               frozen_filter( component ) &&
               magazine_filter( component );
    };
}

bool recipe::npc_can_craft( std::string &reason ) const
{
    if( is_practice() ) {
        reason = _( "Ordering NPC to practice is not implemented yet." );
        return false;
    }
    if( result()->phase != phase_id::SOLID ) {
        reason = _( "Ordering NPC to craft non-solid item is not implemented yet." );
        return false;
    }
    for( const auto& [bp, _] : get_byproducts() ) {
        if( bp->phase != phase_id::SOLID ) {
            reason = _( "Ordering NPC to craft non-solid item is not implemented yet." );
            return false;
        }
    }
    return true;
}

bool recipe::is_practice() const
{
    return practice_data.has_value();
}

bool recipe::is_nested() const
{
    return !nested_category_data.empty();
}

bool recipe::is_blueprint() const
{
    return !blueprint.is_empty();
}

const update_mapgen_id &recipe::get_blueprint() const
{
    return blueprint;
}

const translation &recipe::blueprint_name() const
{
    return bp_name;
}

const translation &recipe::blueprint_parameter_ui_string(
    const std::string &param_name, const cata_variant &arg_value ) const
{
    auto param = bp_parameter_names.find( param_name );
    if( param == bp_parameter_names.end() ) {
        debugmsg( "Parameter name %s missing from bp_parameter_names in %s",
                  param_name, ident().str() );
        static const translation error = to_translation( "[bad param name]" );
        return error;
    }

    const TranslationMap &translations = param->second;
    auto arg = translations.find( arg_value.get_string() );
    if( arg == translations.end() ) {
        debugmsg( "Argument value %s missing from bp_parameter_names[\"%s\"] in %s",
                  arg_value.get_string(), param_name, ident().str() );
        static const translation error = to_translation( "[bad argument value]" );
        return error;
    }

    return arg->second;
}

const std::vector<itype_id> &recipe::blueprint_resources() const
{
    return bp_resources;
}

const std::vector<std::pair<std::string, int>> &recipe::blueprint_provides() const
{
    return bp_provides;
}

const std::vector<std::pair<std::string, int>>  &recipe::blueprint_requires() const
{
    return bp_requires;
}

const std::vector<std::pair<std::string, int>>  &recipe::blueprint_excludes() const
{
    return bp_excludes;
}

const parameterized_build_reqs &recipe::blueprint_build_reqs() const
{
    if( !bp_build_reqs ) {
        cata_fatal( "Accessing absent blueprint_build_reqs in recipe %s", ident().str() );
    }
    return *bp_build_reqs;
}

void recipe::check_blueprint_requirements()
{
    build_reqs total_reqs =
        get_build_reqs_for_furn_ter_ids( get_changed_ids_from_update( blueprint, {} ) );
    if( bp_build_reqs->reqs_by_parameters.size() != 1 ) {
        debugmsg( "Cannot check blueprint reqs for blueprints with parameters" );
        return;
    }
    const std::pair<const mapgen_arguments, build_reqs> &no_param_reqs =
        *bp_build_reqs->reqs_by_parameters.begin();
    if( !no_param_reqs.first.map.empty() ) {
        debugmsg( "Cannot check blueprint reqs for blueprints with parameters" );
        return;
    }
    const build_reqs &reqs = no_param_reqs.second;
    requirement_data req_data_blueprint( reqs.raw_reqs );
    requirement_data req_data_calc( total_reqs.raw_reqs );
    // do not consolidate req_data_blueprint: it actually changes the meaning of the requirement.
    // instead we enforce specifying the exact consolidated requirement.
    req_data_calc.consolidate();
    if( reqs.time != total_reqs.time || reqs.skills != total_reqs.skills
        || !req_data_blueprint.has_same_requirements_as( req_data_calc ) ) {
        std::ostringstream os;
        JsonOut jsout( os, /*pretty_print=*/true );

        jsout.start_object();

        jsout.member( "time" );
        if( total_reqs.time % 100 == 0 ) {
            dump_to_json_string( time_duration::from_turns( total_reqs.time / 100 ),
                                 jsout, time_duration::units );
        } else {
            // cannot precisely represent the value using time_duration format,
            // write integer instead.
            jsout.write( total_reqs.time );
        }

        jsout.member( "skills" );
        jsout.start_array( /*wrap=*/!total_reqs.skills.empty() );
        for( const std::pair<const skill_id, int> &p : total_reqs.skills ) {
            jsout.start_array();
            jsout.write( p.first );
            jsout.write( p.second );
            jsout.end_array();
        }
        jsout.end_array();

        jsout.member( "inline" );
        req_data_calc.dump( jsout );

        jsout.end_object();

        debugmsg( "Specified blueprint requirements of %1$s does not match calculated requirements.\n"
                  "Make one of the following changes to resolve this issue:\n"
                  "- Specify \"check_blueprint_needs\": false to disable the check.\n"
                  "- Remove \"blueprint_needs\" from the recipe json to have the requirements be autocalculated.\n"
                  "- Update \"blueprint_needs\" to the following value (you can use tools/update_blueprint_needs.py):\n"
                  // mark it for the auto-update python script
                  "~~~ auto-update-blueprint: %1$s\n"
                  "%2$s\n"
                  "~~~ end-auto-update",
                  ident_.str(), os.str() );
    }
}

bool recipe::removes_raw() const
{
    item result( result_ );
    return result.is_comestible() && !result.has_flag( flag_RAW );
}

bool recipe::hot_result() const
{
    // Check if the recipe tools make this food item hot upon making it.
    // We don't actually know which specific tool the player used/will use here, but
    // we're checking for a class of tools; because of the way requirements
    // processing works, the "surface_heat" id gets nuked into an actual
    // list of tools, see data/json/recipes/cooking_tools.json.
    //
    // Currently it's checking for a hotplate because that's a
    // suitable item in both the "surface_heat" and "water_boiling_heat"
    // tools, and it's usually the first item in a list of tools so if this
    // does get heated we'll find it right away.
    //
    // Atomic coffee is an outlier in that it is a hot drink that cannot be crafted
    // with any of the usual tools except the atomic coffee maker, which is why
    // the check includes this tool in addition to the hotplate.
    //
    // TODO: Make this less of a hack
    if( item( result_ ).has_temperature() ) {
        const requirement_data::alter_tool_comp_vector &tool_lists = simple_requirements().get_tools();
        for( const std::vector<tool_comp> &tools : tool_lists ) {
            for( const tool_comp &t : tools ) {
                if( ( t.type == itype_hotplate ) || ( t.type == itype_atomic_coffeepot ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

int recipe::makes_amount() const
{
    int makes = 1;
    if( charges.has_value() ) {
        makes = charges.value();
    } else if( item::count_by_charges( result_ ) ) {
        makes = item::find_type( result_ )->charges_default();
    }
    return makes * result_mult;
}

void recipe::incorporate_build_reqs()
{
    if( !bp_build_reqs ) {
        bp_build_reqs = cata::make_value<parameterized_build_reqs>();
    }

    for( std::pair<const mapgen_arguments, build_reqs> &p : bp_build_reqs->reqs_by_parameters ) {
        build_reqs &reqs = p.second;
        reqs.time += time;

        for( const std::pair<const skill_id, int> &p : required_skills ) {
            int &val = reqs.skills[p.first];
            val = std::max( val, p.second );
        }

        reqs.consolidate( reqs_internal, reqs_external );
    }
}

void recipe_proficiency::deserialize( const JsonObject &jo )
{
    load( jo );
}

void recipe_proficiency::load( const JsonObject &jo )
{
    jo.read( "proficiency", id );
    jo.read( "required", required );
    jo.read( "time_multiplier", time_multiplier );
    _skill_penalty_assigned = jo.read( "skill_penalty", skill_penalty );
    jo.read( "learning_time_multiplier", learning_time_mult );
    jo.read( "max_experience", max_experience );

    // TODO: Remove at some point
    if( jo.has_number( "fail_multiplier" ) ) {
        debugmsg( "Proficiency %s in a recipe uses 'fail_multiplier' instead of 'skill_penalty'",
                  id.c_str() );
        jo.read( "fail_multiplier", skill_penalty );
        skill_penalty -= 1;
    }
}

void book_recipe_data::deserialize( const JsonObject &jo )
{
    load( jo );
}

void book_recipe_data::load( const JsonObject &jo )
{
    jo.read( "skill_level", skill_req );
    jo.read( "recipe_name", alt_name );
    jo.read( "hidden", hidden );
}

void practice_recipe_data::deserialize( const JsonObject &jo )
{
    load( jo );
}

void practice_recipe_data::load( const JsonObject &jo )
{
    jo.read( "min_difficulty", min_difficulty );
    if( !jo.read( "max_difficulty", max_difficulty ) ) {
        max_difficulty = MAX_SKILL - 1;
    }
    if( !jo.read( "skill_limit", skill_limit ) ) {
        skill_limit = MAX_SKILL;
    }
}
