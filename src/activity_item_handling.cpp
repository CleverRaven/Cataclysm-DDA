#include "game.h"
#include "item.h"
#include "player_activity.h"
#include "enums.h"
#include "creature.h"
#include <list>
#include <vector>

bool game::make_drop_activity( enum activity_type act, point target )
{
    std::list<std::pair<int, int> > dropped = multidrop();
    if( dropped.empty() ) {
        return false;
    }
    u.assign_activity( act, 0 );
    u.activity.placement = target;
    for( auto item_pair : dropped ) {
        u.activity.values.push_back( item_pair.first );
        u.activity.values.push_back( item_pair.second );
    }
    return true;
}

static void add_drop_pairs( std::list<item *> &items, std::list<int> &quantities )
{
    while( !items.empty() ) {
        int position = g->u.get_item_position( items.front() );
        g->u.activity.values.push_back( position );
        items.pop_front();
        g->u.activity.values.push_back( quantities.front() );
        quantities.pop_front();
        // Items from the same stack will be adjacent,
        // so when detected roll them up into a single entry.
        // This can only happen with inventory items, not weapon or worn items.
        while( !items.empty() && position == g->u.get_item_position( items.front() ) ) {
            items.pop_front();
            g->u.activity.values.back() += quantities.front();
            quantities.pop_front();
        }
    }
}

static void make_drop_activity( enum activity_type act, point drop_target,
    std::list<item *> &selected_items, std::list<int> &item_quantities,
    std::list<item *> &selected_worn_items, std::list<int> &worn_item_quantities )
{
    g->u.assign_activity( act, 0 );
    g->u.activity.placement = drop_target;
    add_drop_pairs( selected_worn_items, worn_item_quantities);
    add_drop_pairs( selected_items, item_quantities);
}

static point get_item_pointers_from_activity(
    std::list<item *> &selected_items, std::list<int> &item_quantities,
    std::list<item *> &selected_worn_items, std::list<int> &worn_item_quantities )
{
    // Drop activity has indices of items in inventory and quantities of same.
    // Indices and quantities alternate on the list.
    // First iterate over the indices and quantities, retrieving item references.
    for( size_t index = 0; index < g->u.activity.values.size(); index += 2 ) {
        const int position = g->u.activity.values[index];
        const int quantity = g->u.activity.values[index + 1];
        const bool is_worn = position < -1;
        const bool is_weapon = position == -1;
        if( is_worn ) {
            selected_worn_items.push_back(
                &g->u.worn[player::worn_position_to_index(position)] );
            worn_item_quantities.push_back( quantity );
        } else  if( is_weapon ) {
            selected_items.push_back( &g->u.weapon );
            item_quantities.push_back( quantity );
        } else {
            // We MUST have pointers to these items, and this is the only way I can see to get them.
            std::list<item> &stack = (std::list<item> &)g->u.inv.const_stack( position );
            int items_dropped = 0;
            for( auto it = stack.begin(); it != stack.end(); ++it ) {
                selected_items.push_back( &*it );
                item_quantities.push_back( 1 );
                if( ++items_dropped >= quantity ) {
                    break;
                }
            }
        }
    }
    point drop_target = g->u.activity.placement;
    // Now that we have all the data, cancel the activity,
    // if we don't finish it we'll make a new one with the remaining items.
    g->u.cancel_activity();
    return drop_target;
}

enum item_place_type {
    DROP_WORN,
    DROP_NOT_WORN,
    STASH_WORN,
    STASH_NOT_WORN
};

static void stash_on_pet( item *item_to_stash, monster *pet )
{
    item *it = &pet->inv[0];
    it_armor *armor = nullptr;
    if( it ) {
        armor = dynamic_cast<it_armor *>(it->type);
    }
    int max_cap = 0;
    if( armor ) {
        max_cap = armor->storage;
    }
    int max_weight = pet->weight_capacity();

    for (auto &i : pet->inv) {
        max_cap -= i.volume();
        max_weight -= i.weight();
    }

    int vol = item_to_stash->volume();
    int weight = item_to_stash->weight();
    bool too_heavy = max_weight - weight < 0;
    bool too_big = max_cap - vol < 0;

    // Stay still you little...
    pet->add_effect("controlled", 5);

    if( !too_heavy && !too_big ) {
        pet->inv.push_back( *item_to_stash );
    } else {
        g->m.add_item_or_charges( pet->xpos(), pet->ypos(), *item_to_stash, 1);
        if( too_big ) {
            g->u.add_msg_if_player(m_bad, _("%s did not fit and fell to the ground!"),
                                   item_to_stash->display_name().c_str());
        } else {
            g->u.add_msg_if_player(m_bad, _("%s is too heavy and fell to the ground!"),
                                   item_to_stash->display_name().c_str());
        }
    }
}

static void stash_on_pet( std::vector<item> &dropped_items, std::vector<item> &dropped_worn_items,
                          point drop_target )
{
    Creature *critter = g->critter_at( drop_target.x, drop_target.y);
    if( critter == NULL ) {
        return;
    }
    monster *pet = dynamic_cast<monster *>(critter);
    if( pet == NULL || !pet->has_effect("pet") ) {
        return;
    }

    for( auto &item_to_stash : dropped_items ) {
        stash_on_pet( &item_to_stash, pet );
    }
    for( auto &item_to_stash : dropped_worn_items ) {
        stash_on_pet( &item_to_stash, pet );
    }
}

static void place_item_activity( std::list<item *> &selected_items, std::list<int> &item_quantities,
                                 std::list<item *> &selected_worn_items,
                                 std::list<int> &worn_item_quantities,
                                 enum item_place_type type, point drop_target )
{
    std::vector<item> dropped_items;
    std::vector<item> dropped_worn_items;
    int prev_volume = g->u.volume_capacity();
    bool taken_off = false;
    if( type == DROP_WORN || type == STASH_WORN ) {
        // TODO: Add the logic where dropping a worn container drops a number of contents as well.
        // Stash previous volume and compare it to volume after taking off each article of clothing.
        taken_off = g->u.takeoff( selected_worn_items.front(), false, &dropped_worn_items );
        // Whether it succeeds or fails, we're done processing it.
        selected_worn_items.pop_front();
        worn_item_quantities.pop_front();
    } else { // Unworn items.
        if( selected_items.front()->count_by_charges() ) {
            dropped_items.push_back(
                g->u.reduce_charges( selected_items.front(), item_quantities.front() ) );
            selected_items.pop_front();
            item_quantities.pop_front();
        } else {
            dropped_items.push_back( g->u.i_rem( selected_items.front() ) );
            // Process one item at a time.
            if( --item_quantities.front() <= 0 ) {
                selected_items.pop_front();
                item_quantities.pop_front();
            }
        }
    }

    if( type == DROP_WORN || type == STASH_WORN ) {
        if( taken_off ) {
            // Move cost for taking off worn item.
            g->u.moves -= 250;
        } else {
            // If we failed to take off the item, bail out.
            return;
        }
    }

    if( type == DROP_WORN || type == DROP_NOT_WORN ) {
        // Drop handles move cost.
        g->drop( dropped_items, dropped_worn_items, g->u.volume_capacity() - prev_volume,
                 drop_target.x, drop_target.y );
    } else { // Stashing on a pet.
        stash_on_pet( dropped_items, dropped_worn_items, drop_target );
    }
}

static void activity_on_turn_drop_or_stash( enum activity_type act )
{
    // We build a list of item pointers to act as unique identifiers,
    // because the process of dropping the items will invalidate the indexes of the items.
    // We assign worn items to their own list to handle dropping containers with their contents.
    std::list<item *> selected_items;
    std::list<int> item_quantities;
    std::list<item *> selected_worn_items;
    std::list<int> worn_item_quantities;

    point drop_target = get_item_pointers_from_activity( selected_items, item_quantities,
                                                         selected_worn_items, worn_item_quantities );

    // Consume the list as long as we don't run out of moves.
    while( g->u.moves >= 0 && !selected_worn_items.empty() ) {
        place_item_activity( selected_items, item_quantities,
                             selected_worn_items, worn_item_quantities,
                             (act == ACT_DROP) ? DROP_WORN : STASH_WORN, drop_target );
    }
    while( g->u.moves >= 0 && !selected_items.empty() ) {
        place_item_activity( selected_items, item_quantities,
                             selected_worn_items, worn_item_quantities,
                             (act == ACT_DROP ) ? DROP_NOT_WORN : STASH_NOT_WORN, drop_target );
    }
    if( selected_items.empty() && selected_worn_items.empty() ) {
        // Yay we're done, just exit.
        return;
    }
    // If we make it here load anything left into a new activity.
    make_drop_activity( act, drop_target, selected_items, item_quantities,
                        selected_worn_items, worn_item_quantities );
}

void game::activity_on_turn_drop()
{
    activity_on_turn_drop_or_stash( ACT_DROP );
}

void game::activity_on_turn_stash()
{
    activity_on_turn_drop_or_stash( ACT_STASH );
}

void game::activity_on_turn_pickup()
{
    // Pickup activity has source square, indices of items on map, and quantities of same.
}
