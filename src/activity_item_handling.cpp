#include "game.h"
#include "item.h"
#include "player_activity.h"
#include "enums.h"
#include "creature.h"
#include "pickup.h"
#include <list>
#include <vector>

bool game::make_drop_activity( enum activity_type act, point target )
{
    std::list<std::pair<int, int> > dropped = multidrop();
    if( dropped.empty() ) {
        return false;
    }
    u.assign_activity( act, 0 );
    u.activity.placement = point( target.x - u.posx(), target.y - u.posy() );
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
    std::list<item *> &selected_worn_items, std::list<int> &worn_item_quantities,
    bool ignoring_interruptions )
{
    g->u.assign_activity( act, 0 );
    // This one is only ever called to re-insert the activity into the activity queue.
    // It's already relative, so no need to adjust it.
    g->u.activity.placement = drop_target;
    g->u.activity.ignore_trivial = ignoring_interruptions;
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
            for( auto &elem : stack ) {
                selected_items.push_back( &elem );
                if( elem.count_by_charges() ) {
                    const int qty_to_drop = std::min( quantity - items_dropped, (int)elem.charges );
                    item_quantities.push_back( qty_to_drop );
                    items_dropped += qty_to_drop;
                } else {
                    item_quantities.push_back( 1 );
                    items_dropped++;
                }
                if( items_dropped >= quantity ) {
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
    int max_cap = it->get_storage();
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
        g->m.add_item_or_charges( pet->posx(), pet->posy(), *item_to_stash, 1);
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
    // Make the relative coordinates absolute.
    drop_target.x += g->u.posx();
    drop_target.y += g->u.posy();
    if( type == DROP_WORN || type == STASH_WORN ) {
        // TODO: Add the logic where dropping a worn container drops a number of contents as well.
        // Stash previous volume and compare it to volume after taking off each article of clothing.
        taken_off = g->u.takeoff( selected_worn_items.front(), false, &dropped_worn_items );
        // Whether it succeeds or fails, we're done processing it.
        selected_worn_items.pop_front();
        worn_item_quantities.pop_front();
        if( taken_off ) {
            // Move cost for taking off worn item.
            g->u.moves -= 250;
        } else {
            // If we failed to take off the item, bail out.
            return;
        }
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

    bool ignoring_interruptions = g->u.activity.ignore_trivial;
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
                        selected_worn_items, worn_item_quantities, ignoring_interruptions );
}

void activity_on_turn_drop()
{
    activity_on_turn_drop_or_stash( ACT_DROP );
}

void activity_on_turn_stash()
{
    activity_on_turn_drop_or_stash( ACT_STASH );
}

void activity_on_turn_pickup()
{
    // Pickup activity has source square, bool indicating source type,
    // indices of items on map, and quantities of same.
    bool from_vehicle = g->u.activity.values.front();
    point pickup_target = g->u.activity.placement;
    // Auto_resume implies autopickup.
    bool autopickup = g->u.activity.auto_resume;
    std::list<int> indices;
    std::list<int> quantities;

    if( !from_vehicle &&
        g->m.i_at(pickup_target.x + g->u.posx(), pickup_target.y + g->u.posy()).size() <= 0 ) {
        g->u.cancel_activity();
        return;
    }
    // Note i = 1, skipping first element.
    for( size_t i = 1; i < g->u.activity.values.size(); i += 2 ) {
        indices.push_back( g->u.activity.values[i] );
        quantities.push_back( g->u.activity.values[ i + 1 ] );
    }
    g->u.cancel_activity();

    Pickup::do_pickup( pickup_target, from_vehicle, indices, quantities, autopickup );

    // If there are items left, we ran out of moves, so make a new activity with the remainder.
    if( !indices.empty() ) {
        g->u.assign_activity( ACT_PICKUP, 0 );
        g->u.activity.placement = pickup_target;
        g->u.activity.auto_resume = autopickup;
        g->u.activity.values.push_back( from_vehicle );
        while( !indices.empty() ) {
            g->u.activity.values.push_back( indices.front() );
            indices.pop_front();
            g->u.activity.values.push_back( quantities.front() );
            quantities.pop_front();
        }
    }
}

// I'd love to have this not duplicate so much code from Pickup::pick_one_up(),
// but I don't see a clean way to do that.
static void move_items( point source, point destination,
                        std::list<int> &indices, std::list<int> &quantities )
{
    source.x += g->u.posx();
    source.y += g->u.posy();
    destination.x += g->u.posx();
    destination.y += g->u.posy();
    int veh_root_part = -1;
    vehicle *veh = g->m.veh_at( source.x, source.y, veh_root_part );
    int cargo_part = -1;
    if( veh != nullptr ) {
        cargo_part = veh->part_with_feature( veh_root_part, "CARGO", false );
    }

    std::vector<item> dropped_items;
    std::vector<item> dropped_worn;
    while( g->u.moves > 0 && !indices.empty() ) {
        int index = indices.back();
        int quantity = quantities.back();
        indices.pop_back();
        quantities.pop_back();

        item *temp_item = nullptr;
        if( cargo_part >= 0 ) {
            temp_item = g->m.item_from( veh, cargo_part, index );
        } else {
            temp_item = g->m.item_from( source, index );
        }

        if( temp_item == nullptr ) {
            continue; // No such item.
        }
        item leftovers = *temp_item;

        if( quantity != 0 ) {
            // Reinserting leftovers happens after item removal to avoid stacking issues.
            int leftover_charges = temp_item->charges - quantity;
            if (leftover_charges > 0) {
                leftovers.charges = leftover_charges;
                temp_item->charges = quantity;
            }
        }

        // Check that we can pick it up.
        if( !temp_item->made_of(LIQUID) ) {
            // Is it too bulky? We'll have to use our hands, then.
            if( !g->u.can_pickVolume(temp_item->volume()) && g->u.is_armed() ) {
                g->u.moves -= 20; // Pretend to be unwielding our gun.
            }

            // Is it too heavy? It'll take a while...
            if( !g->u.can_pickWeight(temp_item->weight(), true) ) {
                int overweight = temp_item->weight() - (g->u.weight_capacity() - g->u.weight_carried());

                // ...like one move cost per 100 grams over your leftover carry capacity.
                g->u.moves -= int(overweight / 100);
            }

            // Drop it first since we're going to delete the original.
            dropped_items.push_back( *temp_item );
            g->drop( dropped_items, dropped_worn, 0, destination.x, destination.y );

            // Remove from map.
            if( veh != nullptr && cargo_part >= 0 ) {
                veh->remove_item( cargo_part, index );
            } else {
                g->m.i_rem( source.x, source.y, index );
            }
            g->u.moves -= 100;

        }

        // If we didn't pick up a whole stack, put the remainder back where it came from.
        if( quantity != 0 ) {
            bool to_map = veh != nullptr;
            if( !to_map ) {
                to_map = !veh->add_item( cargo_part, leftovers );
            }
            if( to_map ){
                g->m.add_item_or_charges( source.x, source.y, leftovers );
            }
        }

        dropped_items.clear();
    }
}

void activity_on_turn_move_items()
{
    // Move activity has source square, target square,
    // indices of items on map, and quantities of same.
    point source = g->u.activity.placement;
    point destination = point( g->u.activity.values[0], g->u.activity.values[1] );
    std::list<int> indices;
    std::list<int> quantities;
    // Note i = 2, skipping first few elements.
    for( size_t i = 2; i < g->u.activity.values.size(); i += 2 ) {
        indices.push_back( g->u.activity.values[i] );
        quantities.push_back( g->u.activity.values[ i + 1 ] );
    }
    // Nuke the current activity, leaving the backlog alone.
    g->u.activity = player_activity();

    move_items( source, destination, indices, quantities );

    if( !indices.empty() ) {
        g->u.assign_activity( ACT_MOVE_ITEMS, 0 );
        g->u.activity.placement = source;
        g->u.activity.values.push_back( destination.x );
        g->u.activity.values.push_back( destination.y );
        while( !indices.empty() ) {
            g->u.activity.values.push_back( indices.front() );
            indices.pop_front();
            g->u.activity.values.push_back( quantities.front() );
            quantities.pop_front();
        }
    }
}
