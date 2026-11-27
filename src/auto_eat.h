#pragma once
#ifndef CATA_SRC_AUTO_EAT_H
#define CATA_SRC_AUTO_EAT_H

#include <stdint.h>
#include <climits>
#include <list>
#include <map>
#include <unordered_set>
#include <vector>

#include "coords_fwd.h"
#include "debug.h"
#include "item.h"
#include "item_location.h"
#include "type_id.h"
#include "units_fwd.h"

class Character;
class map;
class recipe;
class temp_crafting_inventory;

class auto_eat_settings
{
    public:
        // for tests we expose the individual scoring functions.
        friend class auto_eat_test;
        class potential_edible
        {
            public:
                enum score_position : int8_t {
                    SPOIL,
                    VITAMIN,
                    CALORIE,
                    QUENCH,
                    JOY,
                    RECIPE,
                    LAST
                };
            private:
                const recipe *to_craft = nullptr;
                item_location edible;
                // note: this is a snapshot of the item when the constructor is called.
                item copy;
                std::vector<int> scores{INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN};
                bool needs_update( const enum score_position pos ) const {
                    return pos == score_position::QUENCH || scores[pos] == INT_MIN;
                }
            public:
                potential_edible() = default;
                explicit potential_edible( const recipe *to_craft );
                explicit potential_edible( const item_location &edible ) : edible( edible ), copy( *edible ) {}
                // always returns an item. could be a craft or from an item_location.
                const item &get_item_copy() const {
                    return copy;
                }
                // need an item_location for recipe stuff;
                // it could either be a real item, or a recipe item.
                // that means this item_location is not safe to use for general purpose
                item_location fake_loc() const;
                item_location loc() const {
                    if( edible.valid() ) {
                        return edible;
                    } else {
                        debugmsg( "attempted to get invalid potential_edible item_location" );
                        return item_location();
                    }
                }
                bool is_recipe() const {
                    return !!to_craft;
                }
                const recipe *get_recipe() const {
                    return to_craft;
                }
                bool valid() const {
                    if( !copy.is_null() && !copy.is_comestible() ) {
                        return false;
                    }
                    return edible.valid() || to_craft;
                }
                void load_scores( const std::vector<int> &n_scores ) {
                    scores = n_scores;
                }
                void update_score( const enum score_position pos, const int val ) {
                    scores[pos] = val;
                }
                // returns total count of items; recipe batching and charges
                int total_count() const;
                // gives scores based on recipe result amounts & charges

                int total_quench() const;
                int total_kcal( const Character &guy ) const;
                units::volume satiety( const Character &guy ) const;
                // satiety of entire recipe output or stack of items
                units::volume total_satiety( const Character &guy ) const;
                std::map<vitamin_id, int> total_vitamins( const Character &guy ) const;

                int score_food( const Character &guy, const auto_eat_settings &settings );
                int score() const;
        };
    private:
        // the minimum kcal value to consider an item food
        int min_kcal_for_food = 50;
        // the minimum quench value for a drink
        int min_quench = 15;
        // minimum fun value for consumption
        int min_fun = -5;
        // will drink past necessary quench values to eat calories
        bool over_quench = true;
        // the amount of kcal a "meal" consists of when eating multiple items
        int meal_size = 1000;

        bool avoid_crafting = false;

        // for auto eat zone settings
        bool avoid_food = false;
        bool avoid_drink = false;
        // this is a rolling thirst value to queue up quench values for food
        // remember to reset it after use!
        int thirst_modifier = 0;

        int joy_score( const Character &guy, const potential_edible &food ) const;
        // spoilage absolutely needs item_location because of sealed containers
        int spoil_score( const Character &guy, const potential_edible &edible ) const;
        int calorie_score( const Character &guy, const potential_edible &food ) const;
        int quench_score( const Character &guy, const potential_edible &food ) const;
        int vitamin_score( const Character &guy, const potential_edible &food ) const;
        int recipe_score( const Character &, const potential_edible &food ) const;
        // outputs a list of all the food within inventory and radius on map. radius = -1 skips map items
        std::list<potential_edible> list_auto_eat_foods( const Character &guy, int map_radius = 1 ) const;
        // this loads up foods from available crafts into food_list and inv
        // based on recipe rules
        void load_available_crafting_recipes( std::list<potential_edible> &food_list, const Character &guy,
                                              temp_crafting_inventory &inv ) const;
        // this sorts all of the food based on all of the score() functions, and recipes.
        std::list<potential_edible> sort_food_lists( std::list<potential_edible> food_list,
                Character &guy );

        int get_score_from_map( potential_edible::score_position pos, const Character &guy,
                                const potential_edible &food ) const;
        static const std::map<potential_edible::score_position,
               int( auto_eat_settings::* )( const Character &, const potential_edible & ) const>
               score_mappings;
    public:
        std::list<potential_edible> get_surrounding_available_food( Character &guy );
        item_location best_food_carried( Character &guy );
        // finds best food in a set of points.
        item_location best_food_here( Character &guy, map &here,
                                      const std::unordered_set<tripoint_abs_ms> &pts, bool skip_map_pts_if_veh ) const;

        void add_thirst( int thirst );
        bool get_avoid_drink() const {
            return avoid_drink;
        }
        void set_avoid_drink( bool avoid = true ) {
            avoid_drink = avoid;
        }
        bool get_avoid_food() const {
            return avoid_food;
        }
        void set_avoid_food( bool avoid = true ) {
            avoid_food = avoid;
        }
        // clears all temporary values
        void clear();
        int score_food( const Character &guy, const item_location &food ) const;
        int score_food( const Character &guy, potential_edible &food ) const;
        int get_meal_size() const {
            return meal_size;
        }
        // compare food against settings which are mutable for player
        // does not consider anything out of Character::will_eat
        bool will_eat( const Character &guy, const item &food ) const;
        // which item is better? considers settings.
        bool comestible_sort_compare( Character &you, potential_edible &lhs,
                                      potential_edible &rhs ) const;
};

#endif // CATA_SRC_AUTO_EAT_H
