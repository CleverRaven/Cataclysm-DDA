#ifndef ITEM_ACTION_H
#define ITEM_ACTION_H

#include <map>
#include <string>
#include "json.h"
#include "item.h"
#include "player.h"

typedef std::string item_action_id;
typedef std::unordered_map< item_action_id, item* > item_action_map;

class item_action
{
    public:
        std::string name;
        std::string id;
        static void load_item_action(JsonObject &jo);
};

// Assigns items to actions they perform on use
// Prefers items with smaller charge_per_use
item_action_map map_actions_to_items( player &p );

#endif
