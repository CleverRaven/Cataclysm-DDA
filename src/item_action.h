#pragma once
#ifndef CATA_SRC_ITEM_ACTION_H
#define CATA_SRC_ITEM_ACTION_H

#include <map>
#include <string>
#include <vector>

#include "translations.h"

class item_action;
class player;
class item;
class JsonObject;

using item_action_id = std::string;
using item_action_map = std::map< item_action_id, item * >;
using action_map = std::map< item_action_id, item_action >;

class item_action
{
    public:
        translation name;
        item_action_id id;
};

class item_action_generator
{
    private:
        action_map item_actions;
    public:
        static item_action_generator &generator() {
            static item_action_generator generator;

            return generator;
        }

        item_action_generator();
        ~item_action_generator();

        // Assigns items to actions they perform on use
        // Prefers items with smaller charge_per_use
        // "pseudos" are extra items not in player's inventory that should still be mapped
        item_action_map map_actions_to_items( player &p ) const;
        item_action_map map_actions_to_items( player &p, const std::vector<item *> &pseudos ) const;

        // Returns (translated) name of action
        std::string get_action_name( const item_action_id &id ) const;

        bool action_exists( const item_action_id &id ) const;
        const item_action &get_action( const item_action_id &id ) const;

        const action_map &get_item_action_map() const {
            return item_actions;
        }

        void load_item_action( const JsonObject &jo );
        void check_consistency() const;
};

#endif // CATA_SRC_ITEM_ACTION_H
