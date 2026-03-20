#include "crafting_gui_helpers.h"

#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character_id.h"
#include "character.h"
#include "crafting.h"
#include "display.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "localized_comparator.h"
#include "output.h"
#include "recipe.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"

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
