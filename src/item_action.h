#ifndef ITEM_ACTION_H
#define ITEM_ACTION_H

#include <map>
#include <string>
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
    item_action_map map_actions_to_items( player &p ) const;

    // Returns (translated) name of action
    const std::string &get_action_name( const item_action_id &id ) const;
    // As above, except for use_function. Currently supports only iuse_actor use_functions!
    // Will debugmsg if given a cpp iuse_function
    const std::string &get_action_name( const use_function &fun ) const;

    const item_action &get_action( const item_action_id &id ) const;

    const action_map &get_item_action_map() const
    {
        return item_actions;
    }

    void load_item_action(JsonObject &jo);
};



#endif
