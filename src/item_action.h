#ifndef ITEM_ACTION_H
#define ITEM_ACTION_H

#include <map>
#include <string>
#include <vector>
#include "json.h"
#include "item.h"
#include "player.h"

class item_action;

typedef std::string item_action_id;
typedef std::map< item_action_id, item* > item_action_map;
typedef std::map< item_action_id, item_action > action_map;

class item_action
{
    public:
        std::string name;
        item_action_id id;
};

class item_action_generator
{
private:
    action_map item_actions;
public:
    static item_action_generator &generator()
    {
        static item_action_generator generator;

        return generator;
    }

    item_action_generator();
    ~item_action_generator();

    // Assigns items to actions they perform on use
    // Prefers items with smaller charge_per_use
    // "pseudos" are extra items not in player's inventory that should still be mapped
    item_action_map map_actions_to_items( player &p ) const;
    item_action_map map_actions_to_items( player &p, const std::vector<item*> &pseudos ) const;

    // Returns (translated) name of action
    std::string get_action_name( const item_action_id &id ) const;
    // As above, except for use_function. Supports iuse_actor and cpp functions
    // The functions needs its item_action json entry or this will debugmsg
    std::string get_action_name( const iuse_actor *actor ) const;

    const item_action &get_action( const item_action_id &id ) const;

    const action_map &get_item_action_map() const
    {
        return item_actions;
    }

    void load_item_action(JsonObject &jo);
};



#endif
