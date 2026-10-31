#include "auto_eat.h"

#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
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
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const flag_id json_flag_HOT( "HOT" );

static const itype_id itype_water_faucet( "water_faucet" );

static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );

static const vitamin_id vitamin_calcium( "calcium" );
static const vitamin_id vitamin_iron( "iron" );
static const vitamin_id vitamin_vitC( "vitC" );

static const std::vector<vitamin_id> healthy_vitamins{ vitamin_calcium, vitamin_iron, vitamin_vitC };

const std::map<auto_eat_settings::potential_edible::score_position,
      int( auto_eat_settings::* )( const Character &, const auto_eat_settings::potential_edible & ) const>
auto_eat_settings::score_mappings  = {
    {potential_edible::score_position::SPOIL, &auto_eat_settings::spoil_score},
    {potential_edible::score_position::VITAMIN, &auto_eat_settings::vitamin_score},
    {potential_edible::score_position::CALORIE, &auto_eat_settings::calorie_score},
    {potential_edible::score_position::QUENCH, &auto_eat_settings::quench_score},
    {potential_edible::score_position::JOY, &auto_eat_settings::joy_score},
    {potential_edible::score_position::RECIPE, &auto_eat_settings::recipe_score}
};

auto_eat_settings::potential_edible::potential_edible( const recipe *to_craft ) :
    to_craft( to_craft ), copy( to_craft->result() )
{
    if( to_craft->hot_result() ) {
        copy.set_flag( json_flag_HOT );
    }
}

int auto_eat_settings::joy_score( const Character &guy, const potential_edible &food ) const
{
    const int enjoyability = guy.fun_for( food.get_item_copy() ).first;
    return enjoyability * 10;
}

int auto_eat_settings::recipe_score( const Character &, const potential_edible &food ) const
{
    if( food.is_recipe() ) {
        if( avoid_crafting ) {
            // INT_MIN + 1 allows the score to be cached
            return INT_MIN + 1;
        } else {
            return -60;
        }
    } else {
        return 0;
    }
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

item_location auto_eat_settings::potential_edible::fake_loc() const
{
    if( edible.valid() ) {
        return loc();
    } else {
        return item_location( get_player_character(), const_cast<item *>( &copy ) );
    }
}

int auto_eat_settings::potential_edible::score() const
{
    int total = 0;
    for( const int score : scores ) {
        if( !( score == INT_MIN && total < 0 ) ) {
            total += score;
        }
    }
    return total;
}

int auto_eat_settings::spoil_score( const Character &guy, const potential_edible &edible ) const
{
    item_location food = edible.fake_loc();
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

int auto_eat_settings::calorie_score( const Character &guy, const potential_edible &food ) const
{
    const float base_cal = guy.compute_calories_per_effective_volume( food.get_item_copy() ) / 100.0f;
    if( food.is_recipe() ) {
        return std::round( food.total_count() * base_cal );
    } else {
        return std::round( base_cal );
    }
}

int auto_eat_settings::quench_score( const Character &guy, const potential_edible &food ) const
{
    if( !food.get_item_copy().is_comestible() ) {
        return 0;
    }
    const int quench = food.is_recipe() ? food.total_quench() :
                       food.get_item_copy().get_comestible()->quench;
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

int auto_eat_settings::vitamin_score( const Character &guy, const potential_edible &food ) const
{
    float total_score = 0.0f;
    const std::map<vitamin_id, int> vit_map = food.is_recipe() ? food.total_vitamins( guy )
            : guy.compute_effective_nutrients( food.get_item_copy() ).vitamins();
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

int auto_eat_settings::potential_edible::score_food( const Character &guy,
        const auto_eat_settings &settings )
{
    for( int8_t i = 0; i < score_position::LAST; i++ ) {
        const score_position s_p = static_cast<score_position>( i );
        if( needs_update( s_p ) ) {
            update_score( s_p, settings.get_score_from_map( s_p, guy, *this ) );
        }
    }
    return score();
}

int auto_eat_settings::score_food( const Character &guy, const item_location &food ) const
{
    return potential_edible( food ).score_food( guy, *this );
}

int auto_eat_settings::score_food( const Character &guy, potential_edible &food ) const
{
    return food.score_food( guy, *this );
}

int auto_eat_settings::get_score_from_map( potential_edible::score_position pos,
        const Character &guy, const potential_edible &food ) const
{
    const auto func_iter = score_mappings.find( pos );
    if( func_iter == score_mappings.end() ) {
        // pass an invalid location for recipe
        return food.valid() ? 0 : -60;
    } else {
        return ( this->*func_iter->second )( guy, food );
    }
}

bool auto_eat_settings::will_eat( const Character &guy, const item &food ) const
{
    if( !food.is_comestible() ) {
        return false;
    }
    if( guy.fun_for( food ).first < min_fun ) {
        // not good eatings.
        return false;
    }

    const bool is_food = food.get_comestible()->comesttype == "FOOD";

    if( is_food ) {
        if( avoid_food ) {
            return false;
        }
        if( guy.compute_effective_nutrients( food ).kcal() < min_kcal_for_food ) {
            // not filling enough
            return false;
        }
    }
    if( !is_food ) {
        if( !avoid_drink ) {
            const int quench = food.get_comestible()->quench;
            // not quenching enough
            return quench >= min_quench &&
                   // -20 is "Slaked"
                   ( over_quench || guy.get_instant_thirst() + quench + thirst_modifier < -20 );
        } else {
            return false;
        }
    }
    return true;
}

bool auto_eat_settings::comestible_sort_compare( Character &you, potential_edible &lhs,
        potential_edible &rhs ) const
{
    return score_food( you, lhs ) > score_food( you, rhs );
}

std::list<auto_eat_settings::potential_edible> auto_eat_settings::list_auto_eat_foods(
    const Character &guy, int map_radius ) const
{
    std::list<potential_edible> food_list;
    const auto visit = [&]( const item_location & food ) {
        // crafting_inventory item_location isn't a real item
        if( food.where() == item_location::type::crafting_inventory ) {
            return VisitResponse::SKIP;
        }
        if( food->is_food() && guy.will_auto_eat( *food ).success() && will_eat( guy, *food ) ) {
            food_list.emplace_back( food );
        }
        return VisitResponse::NEXT;
    };
    if( map_radius >= 0 ) {
        for( const tripoint_bub_ms &pt : points_in_radius( guy.pos_bub(), map_radius ) ) {
            map_cursor( pt ).visit_items( visit );
        }
    }
    guy.visit_items( visit );
    return food_list;
}

int auto_eat_settings::potential_edible::total_count() const
{
    int charges = 1;
    if( get_item_copy().count_by_charges() ) {
        charges = get_item_copy().charges;
    }
    if( is_recipe() ) {
        charges *= get_recipe()->makes_amount();
    }
    return charges;
}

std::map<vitamin_id, int> auto_eat_settings::potential_edible::total_vitamins(
    const Character &guy ) const
{
    const int charges = total_count();
    std::map<vitamin_id, int> ret = guy.compute_effective_nutrients( get_item_copy() ).vitamins();
    for( auto &[id, amount] : ret ) {
        amount *= charges;
    }
    return ret;
}

int auto_eat_settings::potential_edible::total_kcal( const Character &guy ) const
{
    return total_count() * guy.compute_effective_nutrients( get_item_copy() ).kcal();
}

int auto_eat_settings::potential_edible::total_quench() const
{
    return total_count() * get_item_copy().get_comestible()->quench;
}

units::volume auto_eat_settings::potential_edible::satiety( const Character &guy ) const
{

    // not currently a function for this. scoped for variables
    const auto vol_pair = guy.masticated_volume( get_item_copy() );
    const double ratio = guy.compute_effective_food_volume_ratio( get_item_copy() );
    return vol_pair.first + ( vol_pair.second * ratio );
}

units::volume auto_eat_settings::potential_edible::total_satiety( const Character &guy ) const
{
    return total_count() * satiety( guy );
}

std::list<auto_eat_settings::potential_edible> auto_eat_settings::sort_food_lists(
    std::list<potential_edible> food_list, Character &guy )
{
    std::list<potential_edible> to_eat_list;
    int cal_count = 0;
    const int cal_meal = get_meal_size();
    units::volume total_eaten_volume = 0_ml;
    // sort the list every time since quench is a rolling value
    while( cal_count < cal_meal && !food_list.empty() ) {
        food_list.sort( [&]( potential_edible & a, potential_edible & b ) {
            return comestible_sort_compare( guy, a, b );
        } );
        const potential_edible &pair = food_list.front();
        if( !pair.valid() ) {
            debugmsg( "item_location invalidated in auto_eat sort_food_lists" );
            food_list.pop_front();
            continue;
        }
        cal_count += guy.compute_effective_nutrients( pair.get_item_copy() ).kcal();
        total_eaten_volume += pair.is_recipe() ? pair.total_satiety( guy ) : pair.satiety( guy );
        to_eat_list.push_back( pair );
        add_thirst( pair.is_recipe() ? -pair.total_quench() :
                    -pair.get_item_copy().get_comestible()->quench );
        food_list.pop_front();
        if( guy.stomach.would_be_full_with( guy, total_eaten_volume, guy.has_calorie_deficit() ) ) {
            // we're already full, no matter the calories we're trying to eat
            break;
        }
    }
    clear();
    return to_eat_list;
}

item_location auto_eat_settings::best_food_here( Character &guy, map &here,
        const std::unordered_set<tripoint_abs_ms> &pts, const bool skip_map_pts_if_veh ) const
{
    potential_edible best_food;
    const auto visit = [&]( item_location loc ) -> VisitResponse {
        const auto ret = guy.will_auto_eat( *loc );
        if( ret.value() == edible_rating::NOT_OWNED )
        {
            return VisitResponse::SKIP;
        } else if( !ret.success() )
        {
            return VisitResponse::NEXT;
        }
        potential_edible rhs( loc );
        if( !best_food.valid() || ( rhs.valid() && comestible_sort_compare( guy, rhs, best_food ) ) )
        {
            best_food = rhs;
        }
        return VisitResponse::NEXT;
    };
    for( const tripoint_abs_ms &pt : pts ) {
        const optional_vpart_position vp = here.veh_at( here.get_bub( pt ) );
        if( vp ) {
            // check cargo
            vehicle_cursor( vp->vehicle(), vp->part_index() ).visit_items( visit );
            // checks if we have a faucet we can drink from on the tile
            if( vp.part_with_tool( here, itype_water_faucet ) ) {
                // checks for any installed fluid tanks on the whole car with liquid in them
                for( const vpart_reference &vp_fluid_tank :
                     vp->vehicle().get_avail_parts( "FLUIDTANK" ) ) {
                    item_location i_loc =
                        vp->vehicle().part_base( vp_fluid_tank.part_index() );
                    i_loc.visit_contents( visit );
                }
            }
        }
        if( ( !skip_map_pts_if_veh && vp ) || !vp ) {
            map_cursor( pt ).visit_items( visit );
        }
    }
    if( best_food.valid() ) {
        return best_food.loc();
    } else {
        return item_location();
    }
}

item_location auto_eat_settings::best_food_carried( Character &guy )
{
    std::list<potential_edible> food_list = list_auto_eat_foods( guy, -1 );
    food_list = sort_food_lists( food_list, guy );
    for( potential_edible &food : food_list ) {
        if( food.valid() && !food.is_recipe() ) {
            return food.loc();
        }
    }
    // possible character has no food
    return item_location();
}

std::list<auto_eat_settings::potential_edible> auto_eat_settings::get_surrounding_available_food(
    Character &guy )
{
    std::list<potential_edible> food_list = list_auto_eat_foods( guy );
    if( !avoid_crafting ) {
        temp_crafting_inventory inv = guy.crafting_inventory();
        load_available_crafting_recipes( food_list, guy, inv );
    }
    if( food_list.empty() ) {
        popup( _( "You don't have anything you want to eat." ) );
        return food_list;
    }
    return sort_food_lists( food_list, guy );
}

void auto_eat_settings::load_available_crafting_recipes( std::list<potential_edible> &food_list,
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
                food_list.emplace_back( rec );
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
