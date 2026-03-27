#include "crafting_gui_helpers.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_id.h"
#include "character.h"
#include "crafting.h"
#include "display.h"
#include "flag.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "localized_comparator.h"
#include "output.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "flat_set.h"
#include "translations.h"
#include "type_id.h"
#include "uistate.h"

bool cannot_gain_skill_or_prof( const Character &crafter, const recipe &recp )
{
    if( recp.skill_used &&
        static_cast<int>( crafter.get_skill_level( recp.skill_used ) ) <= recp.get_skill_cap() ) {
        return false;
    }
    for( const proficiency_id &prof : recp.used_proficiencies() ) {
        if( !crafter.has_proficiency( prof ) ) {
            return false;
        }
    }
    return true;
}

float availability::get_proficiency_time_maluses() const
{
    if( proficiency_time_maluses < 0 ) {
        proficiency_time_maluses = rec->proficiency_time_maluses( crafter );
    }

    return proficiency_time_maluses;
}

float availability::get_max_proficiency_time_maluses() const
{
    if( max_proficiency_time_maluses < 0 ) {
        max_proficiency_time_maluses = rec->max_proficiency_time_maluses( crafter );
    }

    return max_proficiency_time_maluses;
}

float availability::get_proficiency_skill_maluses() const
{
    if( proficiency_skill_maluses < 0 ) {
        proficiency_skill_maluses = rec->proficiency_skill_maluses( crafter );
    }

    return proficiency_skill_maluses;
}

float availability::get_max_proficiency_skill_maluses() const
{
    if( max_proficiency_skill_maluses < 0 ) {
        max_proficiency_skill_maluses = rec->max_proficiency_skill_maluses( crafter );
    }

    return max_proficiency_skill_maluses;
}

availability::availability( Character &_crafter, const recipe *r, int batch_size,
                            bool camp_crafting, inventory *inventory_override ) :
    crafter( _crafter )
{
    rec = r;
    inv_override = inventory_override;
    const inventory &inv = camp_crafting ? *inv_override : crafter.crafting_inventory();
    auto all_items_filter = r->get_component_filter( recipe_filter_flags::none );
    auto no_rotten_filter = r->get_component_filter( recipe_filter_flags::no_rotten );
    auto no_favorite_filter = r->get_component_filter( recipe_filter_flags::no_favorite );
    const deduped_requirement_data &req = r->deduped_requirements();
    has_all_skills = r->skill_used.is_null() ||
                     crafter.get_skill_level( r->skill_used ) >= r->get_difficulty( crafter );
    crafter_has_primary_skill = r->skill_used.is_null()
                                || crafter.get_knowledge_level( rec->skill_used )
                                >= static_cast<int>( rec->get_difficulty( crafter ) * 0.8f );
    has_proficiencies = r->character_has_required_proficiencies( crafter );
    std::string reason;
    craft_flags flag = camp_crafting ? craft_flags::none : craft_flags::start_only;

    if( crafter.is_npc() && !r->npc_can_craft( reason ) && !camp_crafting ) {
        can_craft = false;
    } else if( r->is_nested() ) {
        can_craft = check_can_craft_nested( _crafter, *r );
    } else {
        can_craft = ( !r->is_practice() || has_all_skills ) && has_proficiencies &&
                    req.can_make_with_inventory( inv, all_items_filter, batch_size, flag );
    }
    would_use_rotten = !req.can_make_with_inventory( inv, no_rotten_filter, batch_size,
                       flag );
    would_use_favorite = !req.can_make_with_inventory( inv, no_favorite_filter, batch_size,
                         flag );
    useless_practice = r->is_practice() && cannot_gain_skill_or_prof( crafter, *r );
    is_nested_category = r->is_nested();
    const requirement_data &simple_req = r->simple_requirements();
    apparently_craftable = ( !r->is_practice() || has_all_skills ) && has_proficiencies &&
                           simple_req.can_make_with_inventory( inv, all_items_filter, batch_size, flag );
    for( const auto &[skill, skill_lvl] : r->required_skills ) {
        if( crafter.get_skill_level( skill ) < skill_lvl ) {
            has_all_skills = false;
            break;
        }
    }
}

nc_color availability::selected_color() const
{
    if( !can_craft && is_nested_category ) {
        return h_blue;
    } else if( !can_craft ) {
        return h_dark_gray;
    } else if( !crafter_has_primary_skill && is_nested_category ) {
        return h_magenta;
    } else if( !crafter_has_primary_skill ) {
        return h_light_red;
    } else if( is_nested_category ) {
        return h_light_blue;
    } else if( would_use_rotten || useless_practice ) {
        return has_all_skills ? h_brown : h_red;
    } else if( would_use_favorite ) {
        return has_all_skills ? h_pink : h_red;
    } else {
        return has_all_skills ? h_white : h_yellow;
    }
}

nc_color availability::color( bool ignore_missing_skills ) const
{
    if( !can_craft && is_nested_category ) {
        return c_blue;
    } else if( !can_craft ) {
        return c_dark_gray;
    } else if( !crafter_has_primary_skill && is_nested_category ) {
        return c_magenta;
    } else if( !crafter_has_primary_skill ) {
        return c_light_red;
    } else if( is_nested_category ) {
        return c_light_blue;
    } else if( would_use_rotten || useless_practice ) {
        return has_all_skills || ignore_missing_skills ? c_brown : c_red;
    } else if( would_use_favorite ) {
        return has_all_skills ? c_pink : c_red;
    } else {
        return has_all_skills || ignore_missing_skills ? c_white : c_yellow;
    }
}

bool availability::check_can_craft_nested( Character &_crafter, const recipe &r )
{
    for( const recipe_id &nested_r : r.nested_category_data ) {
        if( availability( _crafter, &nested_r.obj() ).can_craft ) {
            return true;
        }
    }
    return false;
}

craft_confirm_result can_start_craft(
    const recipe &rec,
    const availability &avail,
    const Character &crafter )
{
    if( !avail.can_craft || !avail.crafter_has_primary_skill ) {
        return craft_confirm_result::cannot_craft;
    }
    if( crafter.lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
        return craft_confirm_result::too_dark;
    }
    return craft_confirm_result::ok;
}

bool recipe_sort_compare(
    const recipe *a, const recipe *b,
    const availability &avail_a, const availability &avail_b,
    const Character &crafter,
    bool a_read, bool b_read,
    bool unread_first )
{
    if( unread_first ) {
        if( a_read != b_read ) {
            return !a_read;
        }
    }
    if( avail_a.can_craft != avail_b.can_craft ) {
        return avail_a.can_craft;
    }
    if( b->difficulty != a->difficulty ) {
        return b->difficulty < a->difficulty;
    }
    const std::string a_name = a->result_name();
    const std::string b_name = b->result_name();
    if( a_name != b_name ) {
        return localized_compare( a_name, b_name );
    }
    return b->time_to_craft( crafter ) < a->time_to_craft( crafter );
}

static std::string craft_success_chance_string( const recipe &recp, const Character &guy )
{
    float chance = 100.f * ( 1.f - guy.recipe_success_chance( recp ) );
    std::string color;
    if( chance > 75 ) {
        color = "yellow";
    } else if( chance > 50 ) {
        color = "light_gray";
    } else if( chance > 25 ) {
        color = "green";
    } else {
        color = "cyan";
    }

    return string_format( _( "Minor Failure Chance: <color_%s>%2.2f%%</color>" ), color, chance );
}

static std::string cata_fail_chance_string( const recipe &recp, const Character &guy )
{
    float chance = 100.f * guy.item_destruction_chance( recp );
    std::string color;
    if( chance > 50 ) {
        color = "i_red";
    } else if( chance > 20 ) {
        color = "red";
    } else if( chance > 5 ) {
        color = "yellow";
    } else {
        color = "light_gray";
    }

    return string_format( _( "Catastrophic Failure Chance: <color_%s>%2.2f%%</color>" ), color,
                          chance );
}

std::vector<std::string> recipe_info(
    const recipe &recp,
    const availability &avail,
    Character &guy,
    std::string_view qry_comps,
    const int batch_size,
    const int fold_width,
    const nc_color &color,
    const std::vector<Character *> &crafting_group )
{
    std::ostringstream oss;
    oss << string_format( _( "Crafter: %s\n" ), guy.name_and_maybe_activity() );

    oss << string_format( _( "Primary skill: %s\n" ), recp.primary_skill_string( guy ) );
    if( !avail.crafter_has_primary_skill ) {
        if( recp.is_practice() ) {
            oss << _( "<color_red>Crafter cannot practice this because they "
                      "lack the theoretical knowledge for it.</color>\n" );
        } else {
            oss << _( "<color_red>Crafter cannot craft this because they "
                      "lack the theoretical knowledge to understand the recipe.</color>\n" );
        }
    }

    if( !recp.required_skills.empty() ) {
        oss << string_format( _( "Other skills: %s\n" ), recp.required_skills_string( guy ) );
    }

    const std::string req_profs = recp.required_proficiencies_string( &guy );
    if( !req_profs.empty() ) {
        oss << string_format( _( "Proficiencies Required: %s\n" ), req_profs );
    }
    const std::string used_profs = recp.used_proficiencies_string( &guy );
    if( !used_profs.empty() ) {
        oss << string_format( _( "Proficiencies Used: %s\n" ), used_profs );
    }
    const std::string missing_profs = recp.missing_proficiencies_string( &guy );
    if( !missing_profs.empty() ) {
        oss << string_format( _( "Proficiencies Missing: %s\n" ), missing_profs );
    }

    oss << craft_success_chance_string( recp, guy ) << "\n";
    oss << cata_fail_chance_string( recp, guy ) << "\n";

    if( !recp.is_nested() ) {
        const int expected_turns = guy.expected_time_to_craft( recp, batch_size )
                                   / to_moves<int>( 1_turns );
        oss << string_format( _( "Time to complete: <color_cyan>%s</color>\n" ),
                              to_string( time_duration::from_turns( expected_turns ) ) );

    }

    const std::string batch_savings = recp.batch_savings_string();
    if( !batch_savings.empty() ) {
        oss << string_format( _( "Batch time savings: <color_cyan>%s</color>\n" ), batch_savings );
    }

    oss << string_format( _( "Activity level: <color_cyan>%s</color>\n" ),
                          display::activity_level_str( recp.exertion_level() ) );

    const int makes = recp.makes_amount();
    if( makes > 1 ) {
        oss << string_format( _( "Recipe makes: <color_cyan>%d</color>\n" ), makes );
    }

    oss << string_format( _( "Craftable in the dark?  <color_cyan>%s</color>\n" ),
                          recp.has_flag( "BLIND_EASY" ) ? _( "Easy" ) :
                          recp.has_flag( "BLIND_HARD" ) ? _( "Hard" ) :
                          _( "Impossible" ) );

    const inventory &crafting_inv = avail.inv_override ? *avail.inv_override : guy.crafting_inventory();
    if( recp.result() ) {
        const int nearby_amount = crafting_inv.count_item( recp.result() );
        std::string nearby_string;
        if( nearby_amount == 0 ) {
            nearby_string = "<color_light_gray>0</color>";
        } else if( nearby_amount > 9000 ) {
            // at some point you get too many to count at a glance and just know you have a lot
            nearby_string = _( "<color_red>It's Over 9000!!!</color>" );
        } else {
            nearby_string = string_format( "<color_yellow>%d</color>", nearby_amount );
        }
        oss << string_format( _( "Nearby: %s\n" ), nearby_string );
    }

    const bool can_craft_this = avail.can_craft;
    if( can_craft_this && avail.would_use_rotten ) {
        oss << _( "<color_red>Will use rotten ingredients</color>\n" );
    }
    if( can_craft_this && avail.would_use_favorite ) {
        oss << _( "<color_red>Will use favorited ingredients</color>\n" );
    }
    const bool too_complex = recp.deduped_requirements().is_too_complex();
    if( can_craft_this && too_complex ) {
        oss << _( "Due to the complex overlapping requirements, this "
                  "recipe <color_yellow>may appear to be craftable "
                  "when it is not</color>.\n" );
    }
    std::string reason;
    bool npc_cant = avail.crafter.is_npc() && !recp.npc_can_craft( reason ) && !avail.inv_override ;
    if( !can_craft_this && avail.apparently_craftable && !recp.is_nested() && !npc_cant ) {
        oss << _( "<color_red>Cannot be crafted because the same item is needed "
                  "for multiple components.</color>\n" );
    }

    if( !can_craft_this && npc_cant ) {
        oss << colorize( reason, c_red ) << "\n";
    }

    const bool disp_prof_msg = avail.has_proficiencies && !recp.is_nested();
    const float time_maluses = avail.get_proficiency_time_maluses();
    const float max_time_malus = avail.get_max_proficiency_time_maluses();
    const float skill_maluses = avail.get_proficiency_skill_maluses();
    const float max_skill_malus = avail.get_max_proficiency_skill_maluses();
    if( disp_prof_msg && time_maluses < max_time_malus && skill_maluses < max_skill_malus ) {
        oss << string_format( _( "<color_green>This recipe will be %.2fx faster than normal, "
                                 "and your effective skill will be %.2f levels higher than normal, because of "
                                 "the proficiencies the crafter has.</color>\n" ),
                              max_time_malus / time_maluses, max_skill_malus - skill_maluses );
    } else if( disp_prof_msg && time_maluses < max_time_malus ) {
        oss << string_format( _( "<color_green>This recipe will be %.2fx faster than normal, "
                                 "because of the proficiencies the crafter has.</color>\n" ), max_time_malus / time_maluses );
    } else if( disp_prof_msg && skill_maluses < max_skill_malus ) {
        oss << string_format(
                _( "<color_green>Your effective skill will be %.2f levels higher than normal, "
                   "because of the proficiencies the crafter has.</color>\n" ), max_skill_malus - skill_maluses );
    }
    if( !can_craft_this && !avail.has_proficiencies ) {
        oss << _( "<color_red>Cannot be crafted because the crafter lacks"
                  " the required proficiencies.</color>\n" );
    }

    if( recp.has_byproducts() ) {
        oss << _( "Byproducts:\n" );
        for( const std::pair<const itype_id, int> &bp : recp.get_byproducts() ) {
            const itype *t = item::find_type( bp.first );
            int amount = bp.second * batch_size;
            if( t->count_by_charges() ) {
                oss << string_format( "> %s (%d)\n", t->nname( 1 ), amount );
            } else {
                oss << string_format( "> %d %s\n", amount,
                                      t->nname( static_cast<unsigned int>( amount ) ) );
            }
        }
    }

    std::vector<std::string> result = foldstring( oss.str(), fold_width );

    if( !recp.is_nested() ) {
        const requirement_data &req = recp.simple_requirements();
        const std::vector<std::string> tools = req.get_folded_tools_list(
                fold_width, color, crafting_inv, batch_size );
        const std::vector<std::string> comps = req.get_folded_components_list(
                fold_width, color, crafting_inv, recp.get_component_filter(), batch_size, qry_comps );
        result.insert( result.end(), tools.begin(), tools.end() );
        result.insert( result.end(), comps.begin(), comps.end() );
    }

    oss = std::ostringstream();
    if( !guy.knows_recipe( &recp ) ) {
        oss << _( "Recipe not memorized yet\n" );
        const std::set<itype_id> books_with_recipe = guy.get_books_for_recipe( crafting_inv, &recp );
        if( !books_with_recipe.empty() ) {
            const std::string enumerated_books = enumerate_as_string( books_with_recipe,
            []( const itype_id & type_id ) {
                return colorize( item::nname( type_id ), c_cyan );
            } );
            oss << string_format( _( "Written in: %s\n" ), enumerated_books );
        }
        std::vector<const Character *> knowing_helpers;
        for( const Character *helper : crafting_group ) {
            // guy.getID() != helper->getID(): guy doesn't know the recipe anyway, but this should be faster
            if( guy.getID() != helper->getID() && helper->knows_recipe( &recp ) ) {
                knowing_helpers.push_back( helper );
            }
        }
        if( !knowing_helpers.empty() ) {
            const std::string enumerated_helpers = enumerate_as_string( knowing_helpers,
            []( const Character * helper ) {
                return colorize( helper->is_avatar() ? _( "You" ) : helper->get_name(), c_cyan );
            } );
            oss << string_format( _( "Known by: %s\n" ), enumerated_helpers );
        }
    }
    std::vector<std::string> tmp = foldstring( oss.str(), fold_width );
    result.insert( result.end(), tmp.begin(), tmp.end() );

    return result;
}

std::string practice_recipe_description( const recipe &recp,
        const Character &crafter )
{
    std::ostringstream oss;
    oss << recp.get_description( crafter ) << "\n\n";
    if( !recp.practice_data ) {
        return oss.str();
    }
    if( recp.practice_data->min_difficulty != recp.practice_data->max_difficulty ) {
        std::string txt = string_format( _( "Difficulty range: %d to %d" ),
                                         recp.practice_data->min_difficulty, recp.practice_data->max_difficulty );
        oss << txt << "\n";
    }
    if( recp.skill_used ) {
        const int player_skill_level = crafter.get_all_skills().get_skill_level( recp.skill_used );
        if( player_skill_level < recp.practice_data->min_difficulty ) {
            std::string txt = string_format(
                                  _( "The crafter does not possess the minimum <color_cyan>%s</color> skill level required to practice this." ),
                                  recp.skill_used->name() );
            txt = string_format( "<color_red>%s</color>", txt );
            oss << txt << "\n";
        }
        if( recp.practice_data->skill_limit != MAX_SKILL ) {
            std::string txt = string_format(
                                  _( "This practice action will not increase your <color_cyan>%s</color> skill above %d." ),
                                  recp.skill_used->name(), recp.practice_data->skill_limit );
            if( player_skill_level >= recp.practice_data->skill_limit ) {
                txt = string_format( "<color_brown>%s</color>", txt );
            }
            oss << txt << "\n";
        }
    }
    return oss.str();
}

static bool query_is_yes( std::string_view query )
{
    const std::string_view subquery = query.substr( 2 );

    return subquery == "yes" || subquery == "y" || subquery == "1" ||
           subquery == "true" || subquery == "t" || subquery == "on" ||
           subquery == _( "yes" );
}

recipe_subset filter_recipes( const recipe_subset &available_recipes,
                              std::string_view qry, const Character &crafter,
                              const std::function<void( size_t, size_t )> &progress_callback )
{
    size_t qry_begin = 0;
    size_t qry_end = 0;
    recipe_subset filtered_recipes = available_recipes;
    do {
        // Find next ','
        qry_end = qry.find_first_of( ',', qry_begin );

        std::string qry_filter_str = trim( qry.substr( qry_begin, qry_end - qry_begin ) );
        // Process filter
        if( qry_filter_str.size() > 2 && qry_filter_str[1] == ':' ) {
            switch( qry_filter_str[0] ) {
                case 't':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::tool, progress_callback );
                    break;

                case 'c':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::component, progress_callback );
                    break;

                case 's':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::skill, progress_callback );
                    break;

                case 'p':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::primary_skill, progress_callback );
                    break;

                case 'Q':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::quality, progress_callback );
                    break;

                case 'q':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::quality_result, progress_callback );
                    break;

                case 'L':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::length, progress_callback );
                    break;

                case 'V':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::volume, progress_callback );
                    break;

                case 'M':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::mass, progress_callback );
                    break;

                case 'v':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::covers, progress_callback );
                    break;

                case 'e':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::layer, progress_callback );
                    break;

                case 'd':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::description_result, progress_callback );
                    break;

                case 'm': {
                    // get_learned_recipes lists NO nested_recipes
                    const recipe_subset &learned = crafter.get_learned_recipes();
                    recipe_subset temp_subset;
                    if( query_is_yes( qry_filter_str ) ) {
                        temp_subset = available_recipes.intersection( learned );
                    } else {
                        // nested_recipes cannot be learned so don't show them
                        temp_subset = available_recipes.difference( learned )
                                      .difference( recipe_dict.all_nested() );
                    }
                    filtered_recipes = filtered_recipes.intersection( temp_subset );
                    break;
                }

                case 'P':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::proficiency, progress_callback );
                    break;

                case 'b':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ), crafter,
                                       recipe_subset::search_type::book, progress_callback );
                    break;

                case 'l':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::difficulty, progress_callback );
                    break;

                case 'r': {
                    // TODO: this replaces filtered_recipes instead of intersecting,
                    // which breaks comma-chaining when r: appears after other filters.
                    // Prior filters in the chain are lost. Fix in a separate PR.
                    recipe_subset result;
                    for( const itype *e : item_controller->all() ) {
                        if( lcmatch( e->nname( 1 ), qry_filter_str.substr( 2 ) ) ) {
                            result.include( recipe_subset( available_recipes,
                                                           available_recipes.recipes_that_produce( e->get_id() ) ) );
                        }
                    }
                    filtered_recipes = result;

                    break;
                }

                case 'a':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::activity_level, progress_callback );
                    break;
                default:
                    break;
            }
        } else if( qry_filter_str.size() > 1 && qry_filter_str[0] == '-' ) {
            filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 1 ),
                               recipe_subset::search_type::exclude_name, progress_callback );
        } else {
            filtered_recipes = filtered_recipes.reduce( qry_filter_str,
                               recipe_subset::search_type::name,
                               std::function<void( size_t, size_t )> {} );
        }

        qry_begin = qry_end + 1;
    } while( qry_end != std::string::npos );
    return filtered_recipes;
}

const std::vector<SearchPrefix> prefixes = {
    //~ Example result description search term
    { 'q', to_translation( "metal sawing" ), to_translation( "<color_cyan>quality</color> of resulting item" ) },
    { 'd', to_translation( "reach attack" ), to_translation( "<color_cyan>full description</color> of resulting item (slow)" ) },
    { 'c', to_translation( "plank" ), to_translation( "<color_cyan>component</color> required to craft" ) },
    { 'p', to_translation( "tailoring" ), to_translation( "<color_cyan>primary skill</color> used to craft" ) },
    { 's', to_translation( "food handling" ), to_translation( "<color_cyan>any skill</color> used to craft" ) },
    { 'Q', to_translation( "fine bolt turning" ), to_translation( "<color_cyan>quality</color> required to craft" ) },
    { 't', to_translation( "soldering iron" ), to_translation( "<color_cyan>tool</color> required to craft" ) },
    { 'm', to_translation( "yes" ), to_translation( "recipe <color_cyan>memorized</color> (or not)" ) },
    { 'P', to_translation( "Blacksmithing" ), to_translation( "<color_cyan>proficiency</color> used to craft" ) },
    { 'b', to_translation( "chemistry textbook" ), to_translation( "<color_cyan>source</color> of the recipe" ) },
    { 'l', to_translation( "5" ), to_translation( "<color_cyan>difficulty</color> of the recipe as a number or range" ) },
    { 'r', to_translation( "buttermilk" ), to_translation( "recipe's (<color_cyan>by</color>)<color_cyan>products</color>" ) },
    { 'L', to_translation( "122 cm" ), to_translation( "result can contain item of <color_cyan>length</color>" ) },
    { 'V', to_translation( "450 ml" ), to_translation( "result can contain item of <color_cyan>volume</color>" ) },
    { 'M', to_translation( "250 kg" ), to_translation( "result can contain item of <color_cyan>mass</color>" ) },
    { 'v', to_translation( "head" ), to_translation( "<color_cyan>body part</color> the result covers" ) },
    { 'e', to_translation( "close to skin" ), to_translation( "<color_cyan>layer</color> the result covers" ) },
    { 'a', to_translation( "brisk" ), to_translation( "recipe's <color_cyan>activity level</color>" ) }
};

const translation filter_help_start = to_translation(
        "The default is to search result names.  Some single-character prefixes "
        "can be used with a colon <color_red>:</color> to search in other ways.  Additional filters "
        "are separated by commas <color_red>,</color>.\n"
        "Filtering by difficulty can accept range; "
        "<color_yellow>l</color><color_white>:5~10</color> for all recipes from difficulty 5 to 10.\n"
        "\n\n"
        "<color_white>Examples:</color>\n" );

// --- Recipe result info ---

static void insert_block_separator( std::vector<iteminfo> &info_vec,
                                    const std::string &title, int panel_width )
{
    info_vec.emplace_back( "DESCRIPTION", "--" );
    info_vec.emplace_back( "DESCRIPTION", std::string( center_text_pos( title, 0,
                           panel_width ), ' ' ) +
                           "<bold>" + title + "</bold>" );
    info_vec.emplace_back( "DESCRIPTION", "--" );
}

static void result_item_header( item &dummy_item, int quantity_per_batch,
                                std::vector<iteminfo> &info, const std::string &classification,
                                bool uses_charges, const recipe &rec, int batch_size,
                                const std::string &description = {} )
{
    int total_quantity = quantity_per_batch * batch_size;
    if( uses_charges ) {
        std::string display_name = ( description.empty() ? dummy_item.display_name() : description );
        dummy_item.charges = total_quantity;
        info.emplace_back( "DESCRIPTION",
                           "<bold>" + classification + ": </bold>" + display_name );
        dummy_item.charges /= total_quantity;
    } else {
        std::string display_name = ( description.empty() ? dummy_item.display_name(
                                         total_quantity ) : description );
        info.emplace_back( "DESCRIPTION",
                           "<bold>" + classification + ": </bold>" + display_name +
                           ( total_quantity == 1 ? "" : string_format( " (%d)", total_quantity ) ) );
    }
    if( dummy_item.has_flag( flag_VARSIZE ) &&
        dummy_item.has_flag( flag_FIT ) ) {
        std::vector<std::vector<item_comp> > item_component_reqs =
            rec.simple_requirements().get_components();
        bool has_varsize_components = false;
        for( const std::vector<item_comp> &component_options : item_component_reqs ) {
            for( const item_comp &component : component_options ) {
                const itype *type = item::find_type( component.type );
                if( type->has_flag( flag_VARSIZE ) ) {
                    has_varsize_components = true;
                    break;
                }
            }
            if( has_varsize_components ) {
                break;
            }
        }
        if( has_varsize_components ) {
            info.emplace_back( "DESCRIPTION",
                               _( "<bold>Note:</bold> If crafted from poorly-fitting components, the resulting item may also be poorly-fitted." ) );
        }
    }
}

static void result_item_details( item &dummy_item, int quantity_per_batch,
                                 std::vector<iteminfo> &details_info, const std::string &classification,
                                 bool uses_charges, const recipe &rec, int batch_size,
                                 const std::string &description = {} )
{
    std::vector<iteminfo> temp_info;
    int total_quantity = quantity_per_batch * batch_size;
    result_item_header( dummy_item, quantity_per_batch, details_info, classification, uses_charges,
                        rec, batch_size, description );
    if( uses_charges ) {
        dummy_item.charges *= total_quantity;
        dummy_item.info( true, temp_info );
        dummy_item.charges /= total_quantity;
    } else {
        dummy_item.info( true, temp_info, total_quantity );
    }
    details_info.insert( std::end( details_info ), std::begin( temp_info ), std::end( temp_info ) );
}

static void result_byproducts( const recipe &rec,
                               std::vector<iteminfo> &summary_info, std::vector<iteminfo> &details_info,
                               int batch_size, int panel_width )
{
    const std::string byproduct_string = _( "Byproduct" );

    for( const std::pair<const itype_id, int> &bp : rec.get_byproducts() ) {
        insert_block_separator( details_info, byproduct_string, panel_width );
        item dummy_item = item( bp.first );
        bool uses_charges = dummy_item.count_by_charges();
        result_item_header( dummy_item, bp.second, summary_info, _( "With byproduct" ), uses_charges,
                            rec, batch_size );
        result_item_details( dummy_item, bp.second, details_info, byproduct_string, uses_charges,
                             rec, batch_size );
    }
}

item get_recipe_result_item( const recipe &rec, Character &crafter )
{
    item dummy_result = item( rec.result(), calendar::turn, item::default_charges_tag{} );
    if( !rec.variant().empty() ) {
        dummy_result.set_itype_variant( rec.variant() );
    }
    if( dummy_result.has_flag( flag_VARSIZE ) && !dummy_result.has_flag( flag_FIT ) ) {
        item::sizing general_fit = dummy_result.get_sizing( crafter );
        if( general_fit == item::sizing::small_sized_small_char ||
            general_fit == item::sizing::human_sized_human_char ||
            general_fit == item::sizing::big_sized_big_char ||
            general_fit == item::sizing::ignore ) {
            dummy_result.set_flag( flag_FIT );
        }
    }
    if( dummy_result.count_by_charges() ) {
        dummy_result.charges = 1;
    }
    dummy_result.set_var( "recipe_exemplar", rec.ident().str() );
    return dummy_result;
}

std::vector<iteminfo> recipe_result_info( const recipe &rec, Character &crafter,
        int batch_size, int panel_width )
{
    std::vector<iteminfo> info;
    std::vector<iteminfo> details_info;

    item dummy_result = get_recipe_result_item( rec, crafter );
    std::string result_description;
    if( dummy_result.is_null() ) {
        result_description = rec.get_description( crafter );
    }
    bool result_uses_charges = dummy_result.count_by_charges();
    int const makes_amount = rec.makes_amount();
    item dummy_container;

    const std::string result_string = _( "Result" );
    const std::string recipe_output_string = _( "Recipe Outputs" );
    const std::string recipe_result_string = _( "Recipe Result" );
    const std::string container_string = _( "Container" );
    const std::string in_container_string = _( "In sealed container" );
    const std::string container_info_string = _( "Container Information" );

    if( rec.container_id() == itype_id::NULL_ID() && !rec.has_byproducts() ) {
        insert_block_separator( details_info, recipe_result_string, panel_width );
        result_item_details( dummy_result, makes_amount, details_info, result_string,
                             result_uses_charges, rec, batch_size, result_description );
    } else {
        insert_block_separator( info, recipe_output_string, panel_width );
        if( rec.container_id() != itype_id::NULL_ID() ) {
            dummy_container = item( rec.container_id(), calendar::turn, item::default_charges_tag{} );
            result_item_header( dummy_result, makes_amount, info, recipe_result_string,
                                result_uses_charges, rec, batch_size );
            result_item_header( dummy_container, 1, info, in_container_string, false, rec, batch_size );
            insert_block_separator( details_info, recipe_result_string, panel_width );
            result_item_details( dummy_result, makes_amount, details_info, recipe_result_string,
                                 result_uses_charges, rec, batch_size );
            insert_block_separator( details_info, container_info_string, panel_width );
            result_item_details( dummy_container, 1, details_info, container_string, false,
                                 rec, batch_size );
        } else {
            result_item_header( dummy_result, makes_amount, info, recipe_result_string,
                                result_uses_charges, rec, batch_size );
            insert_block_separator( details_info, recipe_result_string, panel_width );
            result_item_details( dummy_result, makes_amount, details_info, recipe_result_string,
                                 result_uses_charges, rec, batch_size );
        }
        if( rec.has_byproducts() ) {
            result_byproducts( rec, info, details_info, batch_size, panel_width );
        }
        info.emplace_back( "DESCRIPTION", "  " );
    }
    info.insert( std::end( info ), std::begin( details_info ), std::end( details_info ) );
    return info;
}

// --- Recipe list pipeline ---

static void recursively_expand_recipes( std::vector<const recipe *> &current,
                                        std::vector<int> &indent,
                                        std::map<const recipe *, availability> &availability_cache, int i,
                                        Character &crafter, bool unread_recipes_first, bool highlight_unread_recipes,
                                        const recipe_subset &available_recipes, const std::set<recipe_id> &hidden_recipes,
                                        bool camp_crafting, inventory *inventory_override )
{
    std::vector<const recipe *> tmp;
    for( const recipe_id &nested : current[i]->nested_category_data ) {

        if( available_recipes.contains( &nested.obj() )
            && hidden_recipes.find( nested ) == hidden_recipes.end()
          ) {
            tmp.push_back( &nested.obj() );
            indent.insert( indent.begin() + i + 1, indent[i] + 2 );
            if( !availability_cache.count( &nested.obj() ) ) {
                availability_cache.emplace( &nested.obj(),
                                            availability( crafter, &nested.obj(), 1,
                                                    camp_crafting, inventory_override ) );
            }
        }
    }

    const bool want_unread = highlight_unread_recipes && unread_recipes_first;
    std::stable_sort( tmp.begin(), tmp.end(), [
                       &crafter, &availability_cache, want_unread
    ]( const recipe * const a, const recipe * const b ) {
        const bool a_read = !want_unread || uistate.read_recipes.count( a->ident() );
        const bool b_read = !want_unread || uistate.read_recipes.count( b->ident() );
        return recipe_sort_compare( a, b,
                                    availability_cache.at( a ), availability_cache.at( b ),
                                    crafter, a_read, b_read, want_unread );
    } );

    current.insert( current.begin() + i + 1, tmp.begin(), tmp.end() );
}

// TODO: Make this more efficient
static void expand_recipes( std::vector<const recipe *> &current,
                            std::vector<int> &indent,
                            std::map<const recipe *, availability> &availability_cache,
                            Character &crafter, bool unread_recipes_first, bool highlight_unread_recipes,
                            const recipe_subset &available_recipes, const std::set<recipe_id> &hidden_recipes,
                            bool camp_crafting, inventory *inventory_override )
{
    for( size_t i = 0; i < current.size(); ++i ) {
        if( current[i]->is_nested()
            && uistate.expanded_recipes.find( current[i]->ident() ) != uistate.expanded_recipes.end()
          ) {
            recursively_expand_recipes( current, indent, availability_cache, i, crafter,
                                        unread_recipes_first, highlight_unread_recipes, available_recipes,
                                        hidden_recipes, camp_crafting, inventory_override );
        }
    }
}

std::string list_nested( Character &crafter, const recipe *rec,
                         const recipe_subset &available_recipes,
                         int indent_level )
{
    std::string description;
    availability avail( crafter, rec );
    if( rec->is_nested() ) {
        description += colorize( std::string( indent_level,
                                              ' ' ) + rec->result_name() + ":\n", avail.color() );
        for( const recipe_id &r : rec->nested_category_data ) {
            description += list_nested( crafter, &r.obj(), available_recipes, indent_level + 2 );
        }
    } else if( available_recipes.contains( rec ) ) {
        description += colorize( std::string( indent_level,
                                              ' ' ) + rec->result_name() + "\n", avail.color() );
    }

    return description;
}

recipe_list_data build_recipe_list(
    std::vector<const recipe *> picking,
    bool skip_hidden_filter,
    bool skip_sort,
    Character &crafter,
    bool camp_crafting,
    inventory *inventory_override,
    bool highlight_unread,
    bool unread_first,
    std::map<const recipe *, availability> &availability_cache,
    const recipe_subset &available_recipes )
{
    recipe_list_data result;

    if( skip_hidden_filter ) {
        result.entries = std::move( picking );
    } else {
        for( const recipe *r : picking ) {
            if( uistate.hidden_recipes.find( r->ident() ) == uistate.hidden_recipes.end() ) {
                result.entries.push_back( r );
            }
        }
        result.num_hidden = picking.size() - result.entries.size();
    }

    // Cache availability on first display
    for( const recipe *e : result.entries ) {
        if( !availability_cache.count( e ) ) {
            availability_cache.emplace( e,
                                        availability( crafter, e, 1, camp_crafting, inventory_override ) );
        }
    }

    if( !skip_sort ) {
        const bool want_unread = highlight_unread && unread_first;
        std::stable_sort( result.entries.begin(), result.entries.end(), [
                       &crafter, &availability_cache, want_unread
        ]( const recipe * const a, const recipe * const b ) {
            const bool a_read = !want_unread || uistate.read_recipes.count( a->ident() );
            const bool b_read = !want_unread || uistate.read_recipes.count( b->ident() );
            return recipe_sort_compare( a, b,
                                        availability_cache.at( a ), availability_cache.at( b ),
                                        crafter, a_read, b_read, want_unread );
        } );
    }

    // Set up indents and expand nested categories (must happen after sort)
    result.indent.assign( result.entries.size(), 0 );
    expand_recipes( result.entries, result.indent, availability_cache, crafter,
                    unread_first, highlight_unread, available_recipes, uistate.hidden_recipes,
                    camp_crafting, inventory_override );

    // Build the parallel availability vector
    result.available.reserve( result.entries.size() );
    std::transform( result.entries.begin(), result.entries.end(),
    std::back_inserter( result.available ), [&]( const recipe * e ) {
        return availability_cache.at( e );
    } );

    return result;
}
