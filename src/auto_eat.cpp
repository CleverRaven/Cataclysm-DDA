#include "auto_eat.h"

#include <climits>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "output.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "ret_val.h"
#include "stomach.h"
#include "temp_crafting_inventory.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"

static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );

static const vitamin_id vitamin_calcium( "calcium" );
static const vitamin_id vitamin_iron( "iron" );
static const vitamin_id vitamin_vitC( "vitC" );

static const std::vector<vitamin_id> healthy_vitamins{ vitamin_calcium, vitamin_iron, vitamin_vitC };

int auto_eat_settings::joy_score( const Character &guy, const item &food ) const
{
    const int enjoyability = guy.fun_for( food ).first;
    return enjoyability * 10;
}

static time_duration get_comestible_time_left( const item_location &loc )
{
    time_duration time_left = 0_turns;
    const time_duration shelf_life = loc->is_comestible() ? loc->get_comestible()->spoils :
                                     calendar::INDEFINITELY_LONG_DURATION;
    if( shelf_life > 0_turns ) {
        const item &it = *loc;
        const double relative_rot = it.get_relative_rot();
        time_left = shelf_life - shelf_life * relative_rot;

        // Correct for an estimate that exceeds shelf life -- this happens especially with
        // fresh items.
        if( time_left > shelf_life ) {
            time_left = shelf_life;
        }
    }

    return time_left;
}

int auto_eat_settings::spoil_score( const Character &guy, const item_location &food ) const
{
    time_duration time_left = get_comestible_time_left( food );
    if( food.has_parent() && food.parent_pocket()->spoil_multiplier() == 0.0f ) {
        time_left = calendar::INDEFINITELY_LONG_DURATION;
    }
    // if we are here we should technically not have to check the traits
    if( food->rotten() ) {
        if( guy.has_trait( trait_SAPROPHAGE ) || guy.has_trait( trait_SAPROVORE ) ) {
            return 50;
        } else {
            // not int min because there needs to be some sorting
            // but a reasonably low number
            return -15000;
        }
    }

    if( time_left > 4_weeks || time_left == 0_seconds ) {
        return 0;
    } else if( time_left > 1_weeks ) {
        return 5;
    } else if( time_left > 1_days ) {
        return 15;
    }
    // better get this in
    else if( time_left < 5_minutes ) {
        return 500;
    }
    return 50;
}

int auto_eat_settings::calorie_score( const nutrients &nuts ) const
{
    return nuts.kcal() / 10;
}

int auto_eat_settings::quench_score( const Character &guy, const item &food ) const
{
    if( !food.is_comestible() ) {
        return 0;
    }
    const int quench = food.get_comestible()->quench;
    const int thirst = guy.get_instant_thirst() + thirst_modifier;
    float thirst_multiplier = 1.0f;
    // not thirsty. deprioritize quench.
    if( thirst < -40 ) {
        thirst_multiplier = -4.0f;
    } else if( thirst < 0 ) {
        thirst_multiplier = -1.0f;
    } else if( thirst < 20 ) {
        thirst_multiplier = 1.0f;
    } else if( thirst < 40 ) {
        thirst_multiplier = 2.0f;
    } else {
        thirst_multiplier = 4.0f;
    }
    return std::round( quench * thirst_multiplier );
}

int auto_eat_settings::vitamin_score( const nutrients &nuts ) const
{

    float total_score = 0.0f;
    const std::map<vitamin_id, int> vit_map = nuts.vitamins();
    for( const vitamin_id &vit : healthy_vitamins ) {
        const auto vit_iter = vit_map.find( vit );
        if( vit_iter != vit_map.cend() ) {
            total_score += vit_iter->second;
        }
    }
    // 96 units per day make up %dv, we want 1 point per 10% dv
    total_score /= 9.6f;
    return std::round( total_score );
}

int auto_eat_settings::score_food( const Character &guy, const item_location &food ) const
{
    if( !food ) {
        // item_location was invalidated somehow.
        debugmsg( "lost track of item_location when scoring food" );
        return INT_MIN;
    }
    const nutrients nuts = guy.compute_effective_nutrients( *food );
    return spoil_score( guy, food ) + vitamin_score( nuts ) +
           calorie_score( nuts ) + quench_score( guy, *food ) +
           joy_score( guy, *food );
}

int auto_eat_settings::score_food( const Character &guy, const item_craft_pair &food ) const
{
    int recipe_score = 0;
    if( food.second ) {
        recipe_score = -60;
    }
    return recipe_score + score_food( guy, food.first );
}

bool auto_eat_settings::will_eat( const Character &guy, const item &food ) const
{
    if( guy.fun_for( food ).first < min_fun ) {
        // not good eatings.
        return false;
    }

    const bool is_food = food.get_comestible()->comesttype == "FOOD";

    if( is_food && !avoid_food && guy.compute_effective_nutrients( food ).kcal() < min_kcal_for_food ) {
        // not filling enough
        return false;
    }
    if( !is_food && !avoid_drink ) {
        const int quench = food.get_comestible()->quench;
        // not quenching enough
        return quench >= min_quench &&
               // -20 is "Slaked"
               ( over_quench || guy.get_instant_thirst() + quench + thirst_modifier < -20 );
    }
    return true;
}

bool auto_eat_settings::comestible_sort_compare( Character &you, const item_craft_pair &lhs,
        const item_craft_pair &rhs ) const
{
    return score_food( you, lhs ) > score_food( you, rhs );
}

std::list<item_craft_pair> auto_eat_settings::list_auto_eat_foods( const Character &guy ) const
{
    std::list<item_craft_pair> food_list;
    temp_crafting_inventory inv;
    inv.form_from_map( guy.pos_bub(), 1, &guy );
    const auto visit = [&guy, &food_list]( const item_location & food ) {
        // maybe crafting inventory wasn't invalidated or something
        // but crafting_inventory item_location isn't a real item
        if( food.where() == item_location::type::crafting_inventory ) {
            return VisitResponse::SKIP;
        }
        if( food->is_food() && guy.will_auto_eat( *food ).success() ) {
            food_list.emplace_back( food, nullptr );
        }
        return VisitResponse::NEXT;
    };
    inv.visit_items( visit );
    guy.visit_items( visit );
    return food_list;
}

std::list<item_craft_pair> auto_eat_settings::sort_food_lists( std::list<item_craft_pair> food_list,
        Character &guy )
{
    std::list<item_craft_pair> to_eat_list;
    int cal_count = 0;
    const int cal_meal = get_meal_size();
    units::volume total_eaten_volume = 0_ml;
    // sort the list every time since quench is a rolling value
    while( cal_count < cal_meal && !food_list.empty() ) {
        food_list.sort( [&]( const item_craft_pair & a, const item_craft_pair & b ) {
            return comestible_sort_compare( guy, a, b );
        } );
        const item_craft_pair &pair = food_list.front();
        if( !pair.first ) {
            debugmsg( "item_location invalidated in auto_eat sort_food_lists" );
            food_list.pop_front();
            continue;
        }
        cal_count += guy.compute_effective_nutrients( *pair.first ).kcal();
        {
            // not currently a function for this. scoped for variables
            const auto vol_pair = guy.masticated_volume( *pair.first );
            const double ratio = guy.compute_effective_food_volume_ratio( *pair.first );
            total_eaten_volume += vol_pair.first + ( vol_pair.second * ratio );
        }
        to_eat_list.push_back( pair );
        add_thirst( -pair.first->get_comestible()->quench );
        food_list.pop_front();
        if( guy.stomach.would_be_full_with( guy, total_eaten_volume, guy.has_calorie_deficit() ) ) {
            // we're already full, no matter the calories we're trying to eat
            break;
        }
    }
    clear();
    return to_eat_list;
}

std::list<item_craft_pair> auto_eat_settings::get_surrounding_available_food( Character &guy )
{
    std::list<item_craft_pair> food_list = list_auto_eat_foods( guy );
    // needed to be declared here; needs to be in scope for sort_food_lists
    temp_crafting_inventory inv;
    if( !avoid_crafting ) {
        inv = guy.crafting_inventory();
        load_available_crafting_recipes( food_list, guy, inv );
    }
    guy.invalidate_crafting_inventory();
    if( food_list.empty() ) {
        popup( _( "You don't have anything you want to eat." ) );
        return food_list;
    }
    return sort_food_lists( food_list, guy );
}

void auto_eat_settings::load_available_crafting_recipes( std::list<item_craft_pair> &food_list,
        const Character &guy, temp_crafting_inventory &inv ) const
{
    recipe_subset &recipes = guy.get_group_available_recipes( &inv );
    for( const recipe *rec : recipes ) {
        item res( rec->result() );
        res.set_owner( guy );
        if( res.is_food() && guy.will_eat( res ).success() && will_eat( guy, res ) &&
            guy.can_start_craft( rec, recipe_filter_flags{ recipe_filter_flags::no_rotten } ) ) {
            std::vector<item> recipe_results = rec->create_results();
            // we're gonna skip multi results as too complicated for this algorithm for now.
            if( recipe_results.size() == 1 && !recipe_results.front().is_null() ) {
                item_location loc( inv, &inv.add_item_copy( recipe_results.front() ) );
                food_list.emplace_back( loc, rec );
            }
        }
    }
}

void auto_eat_settings::add_thirst( const int thirst )
{
    thirst_modifier += thirst;
}

void auto_eat_settings::clear()
{
    thirst_modifier = 0;
}
