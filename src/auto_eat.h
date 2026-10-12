#pragma once
#ifndef CATA_SRC_AUTO_EAT_H
#define CATA_SRC_AUTO_EAT_H

#include <list>
#include <utility>

class Character;
class item;
class item_location;
class nutrients;
class recipe;
class temp_crafting_inventory;

using item_craft_pair = std::pair<item_location, const recipe *>;

class auto_eat_settings
{
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

        int joy_score( const Character &guy, const item &food ) const;
        // spoilage absolutely needs item_location because of sealed containers
        int spoil_score( const Character &guy, const item_location &food ) const;
        int calorie_score( const nutrients &nuts ) const;
        int quench_score( const Character &guy, const item &food ) const;
        int vitamin_score( const nutrients &nuts ) const;

        int score_food( const Character &guy, const item_location &food ) const;
        // outputs a list of all the food within crafting_inventory
        std::list<item_craft_pair> list_auto_eat_foods( const Character &guy ) const;
        // this loads up foods from available crafts into food_list and inv
        // based on recipe rules
        void load_available_crafting_recipes( std::list<item_craft_pair> &food_list, const Character &guy,
                                              temp_crafting_inventory &inv ) const;
        // this sorts all of the item_craft_pairs based on all of the score() functions, and recipes.
        std::list<item_craft_pair> sort_food_lists( std::list<item_craft_pair> food_list, Character &guy );
    public:
        std::list<item_craft_pair> get_surrounding_available_food( Character &guy );

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
        int score_food( const Character &guy, const item_craft_pair &food ) const;
        int get_meal_size() const {
            return meal_size;
        }
        // compare food against settings which are mutable for player
        // does not consider anything out of Character::will_eat
        bool will_eat( const Character &guy, const item &food ) const;
        // which item is better? considers settings.
        bool comestible_sort_compare( Character &you, const item_craft_pair &lhs,
                                      const item_craft_pair &rhs ) const;
};

#endif // CATA_SRC_AUTO_EAT_H
