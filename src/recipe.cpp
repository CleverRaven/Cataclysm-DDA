#include "recipe.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <numeric>
#include <sstream>

#include "assign.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "construction.h"
#include "debug.h"
#include "enum_traits.h"
#include "flat_set.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "mapgen_functions.h"
#include "npc.h"
#include "optional.h"
#include "output.h"
#include "proficiency.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"
#include "uistate.h"
#include "units.h"
#include "value_ptr.h"

static const itype_id itype_hotplate( "hotplate" );

extern bool test_mode;

recipe::recipe() : skill_used( skill_id::NULL_ID() ) {}

time_duration recipe::batch_duration( const Character &guy, int batch, float multiplier,
                                      size_t assistants ) const
{
    return time_duration::from_turns( batch_time( guy, batch, multiplier, assistants ) / 100 );
}

static bool helpers_have_proficiencies( const Character &guy, const proficiency_id &prof )
{
    std::vector<npc *> helpers = guy.get_crafting_helpers();
    for( npc *helper : helpers ) {
        if( helper->has_proficiency( prof ) ) {
            return true;
        }
    }
    return false;
}

time_duration recipe::time_to_craft( const Character &guy, recipe_time_flag flags ) const
{
    return time_duration::from_seconds( time_to_craft_moves( guy, flags ) / 100 );
}

int recipe::time_to_craft_moves( const Character &guy, recipe_time_flag flags ) const
{
    if( flags == recipe_time_flag::ignore_proficiencies ) {
        return time;
    }
    int ret = time;
    for( const recipe_proficiency &prof : proficiencies ) {
        if( !prof.required ) {
            if( !guy.has_proficiency( prof.id ) &&
                !helpers_have_proficiencies( guy, prof.id ) ) {
                ret *= prof.time_multiplier;
            }
        }
    }
    return ret;
}

int recipe::batch_time( const Character &guy, int batch, float multiplier, size_t assistants ) const
{
    // 1.0f is full speed
    // 0.33f is 1/3 speed
    if( multiplier == 0.0f ) {
        // If an item isn't craftable in the dark, show the time to complete as if you could craft it
        multiplier = 1.0f;
    }

    const float local_time = static_cast<float>( time_to_craft_moves( guy ) ) / multiplier;

    // if recipe does not benefit from batching and we have no assistants, don't do unnecessary additional calculations
    if( batch_rscale == 0.0 && assistants == 0 ) {
        return static_cast<int>( local_time ) * batch;
    }

    float total_time = 0.0f;
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

    return static_cast<int>( total_time );
}

bool recipe::has_flag( const std::string &flag_name ) const
{
    return flags.count( flag_name );
}

void recipe::load( const JsonObject &jo, const std::string &src )
{
    bool strict = src == "dda";

    abstract = jo.has_string( "abstract" );

    if( abstract ) {
        ident_ = recipe_id( jo.get_string( "abstract" ) );
    } else {
        jo.read( "result", result_, true );
        ident_ = recipe_id( result_.str() );
    }

    if( jo.has_bool( "obsolete" ) ) {
        assign( jo, "obsolete", obsolete );
    }

    if( jo.has_int( "time" ) ) {
        // so we can specify moves that is not a multiple of 100
        time = jo.get_int( "time" );
    } else if( jo.has_string( "time" ) ) {
        time = to_moves<int>( read_from_json_string<time_duration>( *jo.get_raw( "time" ),
                              time_duration::units ) );
    }
    assign( jo, "difficulty", difficulty, strict, 0, MAX_SKILL );
    assign( jo, "flags", flags );

    // automatically set contained if we specify as container
    assign( jo, "contained", contained, strict );
    contained |= assign( jo, "container", container, strict );
    assign( jo, "sealed", sealed, strict );

    if( jo.has_array( "batch_time_factors" ) ) {
        auto batch = jo.get_array( "batch_time_factors" );
        batch_rscale = batch.get_int( 0 ) / 100.0;
        batch_rsize  = batch.get_int( 1 );
    }

    assign( jo, "charges", charges );
    assign( jo, "result_mult", result_mult );

    assign( jo, "skill_used", skill_used, strict );

    if( jo.has_member( "skills_required" ) ) {
        auto sk = jo.get_array( "skills_required" );
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
        flags_to_delete = jo.get_tags( "delete_flags" );
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

    const std::string type = jo.get_string( "type" );

    // inline requirements are always replaced (cannot be inherited)
    reqs_internal.clear();

    // These cannot be inherited
    check_blueprint_needs = false;
    has_blueprint_needs = false;
    blueprint_reqs.clear();

    if( type == "recipe" ) {
        if( jo.has_string( "id_suffix" ) ) {
            if( abstract ) {
                jo.throw_error( "abstract recipe cannot specify id_suffix", "id_suffix" );
            }
            ident_ = recipe_id( ident_.str() + "_" + jo.get_string( "id_suffix" ) );
        }

        assign( jo, "category", category, strict );
        assign( jo, "subcategory", subcategory, strict );
        assign( jo, "description", description, strict );
        assign( jo, "reversible", reversible, strict );

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
        assign( jo, "construction_blueprint", blueprint );
        if( !blueprint.empty() ) {
            assign( jo, "blueprint_name", bp_name );
            bp_resources.clear();
            for( const std::string resource : jo.get_array( "blueprint_resources" ) ) {
                bp_resources.emplace_back( resource );
            }
            for( JsonObject provide : jo.get_array( "blueprint_provides" ) ) {
                bp_provides.emplace_back( std::make_pair( provide.get_string( "id" ),
                                          provide.get_int( "amount", 1 ) ) );
            }
            // all blueprints provide themselves with needing it written in JSON
            bp_provides.emplace_back( std::make_pair( result_.str(), 1 ) );
            for( JsonObject require : jo.get_array( "blueprint_requires" ) ) {
                bp_requires.emplace_back( std::make_pair( require.get_string( "id" ),
                                          require.get_int( "amount", 1 ) ) );
            }
            // all blueprints exclude themselves with needing it written in JSON
            bp_excludes.emplace_back( std::make_pair( result_.str(), 1 ) );
            for( JsonObject exclude : jo.get_array( "blueprint_excludes" ) ) {
                bp_excludes.emplace_back( std::make_pair( exclude.get_string( "id" ),
                                          exclude.get_int( "amount", 1 ) ) );
            }
            if( jo.has_member( "blueprint_needs" ) ) {
                has_blueprint_needs = true;
                const JsonObject jneeds = jo.get_object( "blueprint_needs" );
                if( jneeds.has_member( "time" ) ) {
                    if( jneeds.has_int( "time" ) ) {
                        // so we can specify moves that is not a multiple of 100
                        blueprint_reqs.time = jneeds.get_int( "time" );
                    } else {
                        blueprint_reqs.time = to_moves<int>(
                                                  read_from_json_string<time_duration>(
                                                      *jneeds.get_raw( "time" ), time_duration::units ) );
                    }
                    time += blueprint_reqs.time;
                }
                if( jneeds.has_member( "skills" ) ) {
                    std::vector<std::pair<skill_id, int>> blueprint_skills;
                    jneeds.read( "skills", blueprint_skills );
                    blueprint_reqs.skills = { blueprint_skills.begin(), blueprint_skills.end() };
                    for( const std::pair<const skill_id, int> &p : blueprint_reqs.skills ) {
                        int &val = required_skills[p.first];
                        val = std::max( val, p.second );
                    }
                }
                if( jneeds.has_member( "inline" ) ) {
                    const requirement_id req_id( "inline_blueprint_" + type + "_" + ident_.str() );
                    requirement_data::load_requirement( jneeds.get_object( "inline" ), req_id );
                    blueprint_reqs.reqs.emplace( req_id, 1 );
                    reqs_internal.emplace_back( req_id, 1 );
                }
            }
            check_blueprint_needs = jo.get_bool( "check_blueprint_needs", true );
        }
    } else if( type == "uncraft" ) {
        reversible = true;
    } else {
        jo.throw_error( "unknown recipe type", "type" );
    }

    const requirement_id req_id( "inline_" + type + "_" + ident_.str() );
    requirement_data::load_requirement( jo, req_id );
    reqs_internal.emplace_back( req_id, 1 );
}

void recipe::finalize()
{
    if( test_mode && check_blueprint_needs ) {
        check_blueprint_requirements();
    }

    // concatenate both external and inline requirements
    add_requirements( reqs_external );
    add_requirements( reqs_internal );

    reqs_external.clear();
    reqs_internal.clear();

    if( has_blueprint_needs ) {
        blueprint_reqs.clear();
    }

    deduped_requirements_ = deduped_requirement_data( requirements_, ident() );

    if( contained && container.is_null() ) {
        container = item::find_type( result_ )->default_container.value_or( "null" );
    }

    std::set<proficiency_id> required;
    std::set<proficiency_id> used;
    for( const recipe_proficiency &rpof : proficiencies ) {
        if( !rpof.id.is_valid() ) {
            debugmsg( "proficiency %s does not exist in recipe %s", rpof.id.str(), ident_.str() );
        }

        if( rpof.required && rpof.time_multiplier != 1.0f ) {
            debugmsg( "proficiencies in recipes cannot be both required and provide a malus in %s",
                      rpof.id.str(), ident_.str() );
        }
        if( required.count( rpof.id ) || used.count( rpof.id ) ) {
            debugmsg( "proficiency %s listed twice recipe %s", rpof.id.str(),
                      ident_.str() );
        }

        if( rpof.time_multiplier < 1.0f ) {
            debugmsg( "proficiency %s provides a bonus for not being known in recipe %s", rpof.id.str(),
                      ident_.str() );
        }
        if( rpof.fail_multiplier < 1.0f ) {
            debugmsg( "proficiency %s provides a bonus for not being known in recipe %s", rpof.id.str(),
                      ident_.str() );
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

    if( charges && !item::count_by_charges( result_ ) ) {
        return "specifies charges but result is not counted by charges";
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

item recipe::create_result() const
{
    item newit( result_, calendar::turn, item::default_charges_tag{} );
    if( charges ) {
        newit.charges = *charges;
    }

    if( !newit.craft_has_charges() ) {
        newit.charges = 0;
    } else if( result_mult != 1 ) {
        // TODO: Make it work for charge-less items
        newit.charges *= result_mult;
    }

    if( contained ) {
        if( newit.count_by_charges() ) {
            newit = newit.in_container( container, newit.charges, sealed );
        } else {
            newit = newit.in_container( container, item::INFINITE_CHARGES, sealed );
        }
    }

    return newit;
}

std::vector<item> recipe::create_results( int batch ) const
{
    std::vector<item> items;

    const bool by_charges = item::count_by_charges( result_ );
    if( contained || !by_charges ) {
        // by_charges items get their charges multiplied in create_result
        const int num_results = by_charges ? batch : batch * result_mult;
        for( int i = 0; i < num_results; i++ ) {
            item newit = create_result();
            items.push_back( newit );
        }
    } else {
        item newit = create_result();
        newit.charges *= batch;
        items.push_back( newit );
    }

    return items;
}

std::vector<item> recipe::create_byproducts( int batch ) const
{
    std::vector<item> bps;
    for( const auto &e : byproducts ) {
        item obj( e.first, calendar::turn, item::default_charges_tag{} );
        if( obj.has_flag( "VARSIZE" ) ) {
            obj.item_tags.insert( "FIT" );
        }

        if( obj.count_by_charges() ) {
            obj.charges *= e.second * batch;
            bps.push_back( obj );

        } else {
            if( !obj.craft_has_charges() ) {
                obj.charges = 0;
            }
            for( int i = 0; i < e.second * batch; ++i ) {
                bps.push_back( obj );
            }
        }
    }
    return bps;
}

bool recipe::has_byproducts() const
{
    return !byproducts.empty();
}

std::string recipe::required_proficiencies_string( const Character &c ) const
{
    std::vector<proficiency_id> required_profs;
    std::vector<std::pair<proficiency_id, float>> used_profs;

    for( const recipe_proficiency &rec : proficiencies ) {
        if( rec.required ) {
            required_profs.push_back( rec.id );
        } else {
            used_profs.push_back( { rec.id, rec.time_multiplier } );
        }
    }
    std::string required = enumerate_as_string( required_profs.begin(),
    required_profs.end(), [&]( const proficiency_id & id ) {
        nc_color color;
        if( c.has_proficiency( id ) ) {
            color = c_green;
        } else if( helpers_have_proficiencies( c, id ) ) {
            color = c_yellow;
        } else {
            color = c_red;
        }
        return colorize( id->name(), color );
    } );

    std::string used = enumerate_as_string( used_profs.begin(),
    used_profs.end(), [&]( const std::pair<proficiency_id, float> &pair ) {
        std::string color;
        if( c.has_proficiency( pair.first ) ||
            helpers_have_proficiencies( c, pair.first ) ) {
            color = "white";
        } else {
            color = "yellow";
        }
        return string_format( "<color_cyan>%s</color> <color_%s>%gx</color>", pair.first->name(), color,
                              pair.second );
    } );
    used = string_format( _( "Proficiencies Used: %s" ), used );

    return string_format( "%s\n%s", required, used );
}

std::set<proficiency_id> recipe::required_proficiencies() const
{
    std::set<proficiency_id> ret;
    for( const recipe_proficiency &rec : proficiencies ) {
        if( rec.required ) {
            ret.insert( rec.id );
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

std::set<proficiency_id> recipe::assist_proficiencies() const
{
    std::set<proficiency_id> ret;
    for( const recipe_proficiency &rec : proficiencies ) {
        if( !rec.required ) {
            ret.insert( rec.id );
        }
    }
    return ret;
}

float recipe::proficiency_maluses( const Character &guy ) const
{
    float malus = 1.0f;
    for( const recipe_proficiency &prof : proficiencies ) {
        if( !guy.has_proficiency( prof.id ) &&
            !helpers_have_proficiencies( guy, prof.id ) ) {
            malus *= prof.time_multiplier;
        }
    }
    return malus;
}

// Format a std::pair<skill_id, int> for the crafting menu.
// skill colored green (or yellow if beyond characters skill)
// optionally with the skill level (player / difficulty)
template<typename Iter>
std::string required_skills_as_string( Iter first, Iter last, const Character *c,
                                       const bool print_skill_level )
{
    if( first == last ) {
        return _( "<color_cyan>none</color>" );
    }

    return enumerate_as_string( first, last,
    [&]( const std::pair<skill_id, int> &skill ) {
        const int player_skill = c ? c->get_skill_level( skill.first ) : 0;
        std::string difficulty_color = skill.second > player_skill ? "yellow" : "green";
        std::string skill_level_string = print_skill_level ? "" : ( std::to_string( player_skill ) + "/" );
        skill_level_string += std::to_string( skill.second );
        return string_format( "<color_cyan>%s</color> <color_%s>(%s)</color>",
                              skill.first.obj().name(), difficulty_color, skill_level_string );
    } );
}

// Format a std::pair<skill_id, int> for the basecamp bulletin board.
// skill colored white with difficulty in parenthesis.
template<typename Iter>
std::string required_skills_as_string( Iter first, Iter last )
{
    if( first == last ) {
        return _( "<color_cyan>none</color>" );
    }

    return enumerate_as_string( first, last,
    [&]( const std::pair<skill_id, int> &skill ) {
        return string_format( "<color_white>%s (%d)</color>", skill.first.obj().name(),
                              skill.second );
    } );
}

std::string recipe::primary_skill_string( const Character *c, bool print_skill_level ) const
{
    std::vector< std::pair<skill_id, int> > skillList;

    if( !skill_used.is_null() ) {
        skillList.push_back( std::pair<skill_id, int>( skill_used, difficulty ) );
    }

    return required_skills_as_string( skillList.begin(), skillList.end(), c, print_skill_level );
}

std::string recipe::required_skills_string( const Character *c, bool include_primary_skill,
        bool print_skill_level ) const
{
    // There is no primary skill used or it shouldn't be included then we can just use the required_skills directly.
    if( skill_used.is_null() || !include_primary_skill ) {
        return required_skills_as_string( required_skills.begin(), required_skills.end(), c,
                                          print_skill_level );
    }

    std::vector< std::pair<skill_id, int> > skillList;
    skillList.push_back( std::pair<skill_id, int>( skill_used, difficulty ) );
    std::copy( required_skills.begin(), required_skills.end(),
               std::back_inserter<std::vector<std::pair<skill_id, int> > >( skillList ) );

    return required_skills_as_string( skillList.begin(), skillList.end(), c, print_skill_level );
}

std::string recipe::required_all_skills_string() const
{
    // There is no primary skill used, we can just use the required_skills directly.
    if( skill_used.is_null() ) {
        return required_skills_as_string( required_skills.begin(), required_skills.end() );
    }

    std::vector< std::pair<skill_id, int> > skillList;
    skillList.push_back( std::pair<skill_id, int>( skill_used, difficulty ) );
    std::copy( required_skills.begin(), required_skills.end(),
               std::back_inserter<std::vector<std::pair<skill_id, int> > >( skillList ) );

    return required_skills_as_string( skillList.begin(), skillList.end() );
}

std::string recipe::batch_savings_string() const
{
    return ( batch_rsize != 0 ) ?
           string_format( _( "%s%% at >%s units" ), static_cast<int>( batch_rscale * 100 ), batch_rsize )
           : _( "none" );
}

std::string recipe::result_name() const
{
    std::string name = item::nname( result_ );
    if( uistate.favorite_recipes.find( this->ident() ) != uistate.favorite_recipes.end() ) {
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
    const item result = create_result();

    // Disallow crafting of non-perishables with rotten components
    // Make an exception for items with the ALLOW_ROTTEN flag such as seeds
    const bool recipe_forbids_rotten =
        result.is_food() && !result.goes_bad() && !has_flag( "ALLOW_ROTTEN" );
    const bool flags_forbid_rotten =
        static_cast<bool>( flags & recipe_filter_flags::no_rotten );
    std::function<bool( const item & )> rotten_filter = return_true<item>;
    if( recipe_forbids_rotten || flags_forbid_rotten ) {
        rotten_filter = []( const item & component ) {
            return !component.rotten();
        };
    }

    // If the result is made hot, we can allow frozen components.
    // EDIBLE_FROZEN components ( e.g. flour, chocolate ) are allowed as well
    // Otherwise forbid them
    std::function<bool( const item & )> frozen_filter = return_true<item>;
    if( result.is_food() && !hot_result() ) {
        frozen_filter = []( const item & component ) {
            return !component.has_flag( "FROZEN" ) || component.has_flag( "EDIBLE_FROZEN" );
        };
    }

    // Disallow usage of non-full magazines as components
    // This is primarily used to require a fully charged battery, but works for any magazine.
    std::function<bool( const item & )> magazine_filter = return_true<item>;
    if( has_flag( "FULL_MAGAZINE" ) ) {
        magazine_filter = []( const item & component ) {
            if( component.ammo_remaining() == 0 ) {
                return false;
            }
            return !component.is_magazine() ||
                   ( component.ammo_remaining() >= component.ammo_capacity( component.ammo_data()->ammo->type ) );
        };
    }

    return [ rotten_filter, frozen_filter, magazine_filter ]( const item & component ) {
        return is_crafting_component( component ) &&
               rotten_filter( component ) &&
               frozen_filter( component ) &&
               magazine_filter( component );
    };
}

bool recipe::is_blueprint() const
{
    return !blueprint.empty();
}

const std::string &recipe::get_blueprint() const
{
    return blueprint;
}

const translation &recipe::blueprint_name() const
{
    return bp_name;
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

void recipe::check_blueprint_requirements()
{
    build_reqs total_reqs;
    get_build_reqs_for_furn_ter_ids( get_changed_ids_from_update( blueprint ), total_reqs );
    requirement_data req_data_blueprint( blueprint_reqs.reqs );
    requirement_data req_data_calc( total_reqs.reqs );
    // do not consolidate req_data_blueprint: it actually changes the meaning of the requirement.
    // instead we enforce specifying the exact consolidated requirement.
    req_data_calc.consolidate();
    if( blueprint_reqs.time != total_reqs.time || blueprint_reqs.skills != total_reqs.skills
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

        debugmsg( "Specified blueprint requirements of %1$s does not match calculated requirements.  "
                  "Specify \"check_blueprint_needs\": false to disable the check or "
                  "Update \"blueprint_needs\" to the following value (you can use tools/update_blueprint_needs.py):\n"
                  // mark it for the auto-update python script
                  "~~~ auto-update-blueprint: %1$s\n"
                  "%2$s\n"
                  "~~~ end-auto-update",
                  ident_.str(), os.str() );
    }
}

bool recipe::hot_result() const
{
    // Check if the recipe tools make this food item hot upon making it.
    // We don't actually know which specific tool the player used/will use here, but
    // we're checking for a class of tools; because of the way requirements
    // processing works, the "surface_heat" id gets nuked into an actual
    // list of tools, see data/json/recipes/cooking_tools.json.
    //
    // Currently it's only checking for a hotplate because that's a
    // suitable item in both the "surface_heat" and "water_boiling_heat"
    // tools, and it's usually the first item in a list of tools so if this
    // does get heated we'll find it right away.
    //
    // TODO: Make this less of a hack
    if( create_result().is_food() ) {
        const requirement_data::alter_tool_comp_vector &tool_lists = simple_requirements().get_tools();
        for( const std::vector<tool_comp> &tools : tool_lists ) {
            for( const tool_comp &t : tools ) {
                if( t.type == itype_hotplate ) {
                    return true;
                }
            }
        }
    }
    return false;
}

void recipe_proficiency::deserialize( JsonIn &jsin )
{
    load( jsin.get_object() );
}

void recipe_proficiency::load( const JsonObject &jo )
{
    jo.read( "proficiency", id );
    jo.read( "required", required );
    jo.read( "time_multiplier", time_multiplier );
    jo.read( "fail_multiplier", fail_multiplier );
    jo.read( "learning_time_multiplier", learning_time_mult );
    jo.read( "max_experience", max_experience );
}

void book_recipe_data::deserialize( JsonIn &jsin )
{
    load( jsin.get_object() );
}

void book_recipe_data::load( const JsonObject &jo )
{
    jo.read( "skill_level", skill_req );
    jo.read( "recipe_name", alt_name );
    jo.read( "hidden", hidden );
}
