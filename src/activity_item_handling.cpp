#include "activity_handlers.h" // IWYU pragma: associated

#include <climits>
#include <algorithm>
#include <list>
#include <vector>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "avatar.h"
#include "construction.h"
#include "clzones.h"
#include "debug.h"
#include "enums.h"
#include "field.h"
#include "fire.h"
#include "game.h"
#include "item.h"
#include "iuse.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "optional.h"
#include "output.h"
#include "pickup.h"
#include "player.h"
#include "player_activity.h"
#include "requirements.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "calendar.h"
#include "character.h"
#include "game_constants.h"
#include "inventory.h"
#include "line.h"
#include "units.h"
#include "type_id.h"
#include "flat_set.h"
#include "int_id.h"
#include "item_location.h"
#include "point.h"
#include "string_id.h"

struct construction_category;

void cancel_aim_processing();

const efftype_id effect_controlled( "controlled" );
const efftype_id effect_pet( "pet" );

const trap_str_id tr_firewood_source( "tr_firewood_source" );
const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

/** Activity-associated item */
struct act_item {
    const item *it;         /// Pointer to the inventory item
    int count;              /// How many items need to be processed
    int consumed_moves;     /// Amount of moves that processing will consume

    act_item( const item *it, int count, int consumed_moves )
        : it( it ),
          count( count ),
          consumed_moves( consumed_moves ) {}
};

// TODO: Deliberately unified with multidrop. Unify further.
using drop_indexes = std::list<std::pair<int, int>>;

static bool same_type( const std::list<item> &items )
{
    return std::all_of( items.begin(), items.end(), [ &items ]( const item & it ) {
        return it.type == items.begin()->type;
    } );
}

static void put_into_vehicle( Character &c, item_drop_reason reason, const std::list<item> &items,
                              vehicle &veh, int part )
{
    if( items.empty() ) {
        return;
    }

    const tripoint where = veh.global_part_pos3( part );
    const std::string ter_name = g->m.name( where );
    int fallen_count = 0;
    int into_vehicle_count = 0;

    for( auto it : items ) { // cant use constant reference here because of the spill_contents()
        if( Pickup::handle_spillable_contents( c, it, g->m ) ) {
            continue;
        }
        if( veh.add_item( part, it ) ) {
            into_vehicle_count += it.count();
        } else {
            if( it.count_by_charges() ) {
                // Maybe we can add a few charges in the trunk and the rest on the ground.
                auto charges_added = veh.add_charges( part, it );
                it.mod_charges( -charges_added );
                into_vehicle_count += charges_added;
            }
            g->m.add_item_or_charges( where, it );
            fallen_count += it.count();
        }
        it.handle_pickup_ownership( c );
    }

    const std::string part_name = veh.part_info( part ).name();

    if( same_type( items ) ) {
        const item &it = items.front();
        const int dropcount = items.size() * it.count();
        const std::string it_name = it.tname( dropcount );

        switch( reason ) {
            case item_drop_reason::deliberate:
                c.add_msg_player_or_npc(
                    ngettext( "You put your %1$s in the %2$s's %3$s.",
                              "You put your %1$s in the %2$s's %3$s.", dropcount ),
                    ngettext( "<npcname> puts their %1$s in the %2$s's %3$s.",
                              "<npcname> puts their %1$s in the %2$s's %3$s.", dropcount ),
                    it_name, veh.name, part_name
                );
                break;
            case item_drop_reason::too_large:
                c.add_msg_if_player(
                    ngettext(
                        "There's no room in your inventory for the %s, so you drop it into the %s's %s.",
                        "There's no room in your inventory for the %s, so you drop them into the %s's %s.",
                        dropcount ),
                    it_name, veh.name, part_name
                );
                break;
            case item_drop_reason::too_heavy:
                c.add_msg_if_player(
                    ngettext( "The %s is too heavy to carry, so you drop it into the %s's %s.",
                              "The %s are too heavy to carry, so you drop them into the %s's %s.", dropcount ),
                    it_name, veh.name, part_name
                );
                break;
            case item_drop_reason::tumbling:
                c.add_msg_if_player(
                    m_bad,
                    ngettext( "Your %s tumbles into the %s's %s.",
                              "Your %s tumble into the %s's %s.", dropcount ),
                    it_name, veh.name, part_name
                );
                break;
        }
    } else {
        switch( reason ) {
            case item_drop_reason::deliberate:
                c.add_msg_player_or_npc(
                    _( "You put several items in the %1$s's %2$s." ),
                    _( "<npcname> puts several items in the %1$s's %2$s." ),
                    veh.name, part_name
                );
                break;
            case item_drop_reason::too_large:
            case item_drop_reason::too_heavy:
            case item_drop_reason::tumbling:
                c.add_msg_if_player(
                    m_bad, _( "Some items tumble into the %1$s's %2$s." ),
                    veh.name, part_name
                );
                break;
        }
    }

    if( fallen_count > 0 ) {
        if( into_vehicle_count > 0 ) {
            c.add_msg_if_player(
                m_warning,
                ngettext( "The %s is full, so something fell to the %s.",
                          "The %s is full, so some items fell to the %s.", fallen_count ),
                part_name, ter_name
            );
        } else {
            c.add_msg_if_player(
                m_warning,
                ngettext( "The %s is full, so it fell to the %s.",
                          "The %s is full, so they fell to the %s.", fallen_count ),
                part_name, ter_name
            );
        }
    }
}

static void pass_to_ownership_handling( item obj, Character &c )
{
    obj.handle_pickup_ownership( c );
}

static void pass_to_ownership_handling( item obj, player *p )
{
    obj.handle_pickup_ownership( *p );
}

static void stash_on_pet( const std::list<item> &items, monster &pet, player *p )
{
    units::volume remaining_volume = pet.inv.empty() ? 0_ml : pet.inv.front().get_storage();
    units::mass remaining_weight = pet.weight_capacity();

    for( const auto &it : pet.inv ) {
        remaining_volume -= it.volume();
        remaining_weight -= it.weight();
    }

    for( auto &it : items ) {
        pet.add_effect( effect_controlled, 5_turns );
        if( it.volume() > remaining_volume ) {
            add_msg( m_bad, _( "%1$s did not fit and fell to the %2$s." ),
                     it.display_name(), g->m.name( pet.pos() ) );
            g->m.add_item_or_charges( pet.pos(), it );
        } else if( it.weight() > remaining_weight ) {
            add_msg( m_bad, _( "%1$s is too heavy and fell to the %2$s." ),
                     it.display_name(), g->m.name( pet.pos() ) );
            g->m.add_item_or_charges( pet.pos(), it );
        } else {
            pet.add_item( it );
            remaining_volume -= it.volume();
            remaining_weight -= it.weight();
        }
        // TODO: if NPCs can have pets or move items onto pets
        pass_to_ownership_handling( it, p );
    }
}

static void drop_on_map( Character &c, item_drop_reason reason, const std::list<item> &items,
                         const tripoint &where )
{
    if( items.empty() ) {
        return;
    }
    const std::string ter_name = g->m.name( where );
    const bool can_move_there = g->m.passable( where );

    if( same_type( items ) ) {
        const item &it = items.front();
        const int dropcount = items.size() * it.count();
        const std::string it_name = it.tname( dropcount );

        switch( reason ) {
            case item_drop_reason::deliberate:
                if( can_move_there ) {
                    c.add_msg_player_or_npc(
                        ngettext( "You drop your %1$s on the %2$s.",
                                  "You drop your %1$s on the %2$s.", dropcount ),
                        ngettext( "<npcname> drops their %1$s on the %2$s.",
                                  "<npcname> drops their %1$s on the %2$s.", dropcount ),
                        it_name, ter_name
                    );
                } else {
                    c.add_msg_player_or_npc(
                        ngettext( "You put your %1$s in the %2$s.",
                                  "You put your %1$s in the %2$s.", dropcount ),
                        ngettext( "<npcname> puts their %1$s in the %2$s.",
                                  "<npcname> puts their %1$s in the %2$s.", dropcount ),
                        it_name, ter_name
                    );
                }
                break;
            case item_drop_reason::too_large:
                c.add_msg_if_player(
                    ngettext( "There's no room in your inventory for the %s, so you drop it.",
                              "There's no room in your inventory for the %s, so you drop them.", dropcount ),
                    it_name
                );
                break;
            case item_drop_reason::too_heavy:
                c.add_msg_if_player(
                    ngettext( "The %s is too heavy to carry, so you drop it.",
                              "The %s is too heavy to carry, so you drop them.", dropcount ),
                    it_name
                );
                break;
            case item_drop_reason::tumbling:
                c.add_msg_if_player(
                    m_bad,
                    ngettext( "Your %1$s tumbles to the %2$s.",
                              "Your %1$s tumble to the %2$s.", dropcount ),
                    it_name, ter_name
                );
                break;
        }
    } else {
        switch( reason ) {
            case item_drop_reason::deliberate:
                if( can_move_there ) {
                    c.add_msg_player_or_npc(
                        _( "You drop several items on the %s." ),
                        _( "<npcname> drops several items on the %s." ),
                        ter_name
                    );
                } else {
                    c.add_msg_player_or_npc(
                        _( "You put several items in the %s." ),
                        _( "<npcname> puts several items in the %s." ),
                        ter_name
                    );
                }
                break;
            case item_drop_reason::too_large:
            case item_drop_reason::too_heavy:
            case item_drop_reason::tumbling:
                c.add_msg_if_player( m_bad, _( "Some items tumble to the %s." ), ter_name );
                break;
        }
    }
    for( auto &it : items ) {
        g->m.add_item_or_charges( where, it );
        pass_to_ownership_handling( it, c );
    }
}

void put_into_vehicle_or_drop( Character &c, item_drop_reason reason, const std::list<item> &items )
{
    return put_into_vehicle_or_drop( c, reason, items, c.pos() );
}

void put_into_vehicle_or_drop( Character &c, item_drop_reason reason, const std::list<item> &items,
                               const tripoint &where, bool force_ground )
{
    const cata::optional<vpart_reference> vp = g->m.veh_at( where ).part_with_feature( "CARGO", false );
    if( vp && !force_ground ) {
        put_into_vehicle( c, reason, items, vp->vehicle(), vp->part_index() );
        return;
    }
    drop_on_map( c, reason, items, where );
}

static drop_indexes convert_to_indexes( const player_activity &act )
{
    drop_indexes res;

    if( act.values.size() % 2 != 0 ) {
        debugmsg( "Drop/stash activity contains an odd number of values." );
        return res;
    }
    for( auto iter = act.values.begin(); iter != act.values.end(); iter += 2 ) {
        res.emplace_back( *iter, *std::next( iter ) );
    }
    return res;
}

static drop_indexes convert_to_indexes( const player &p, const std::list<act_item> &items )
{
    drop_indexes res;

    for( const auto &ait : items ) {
        const int pos = p.get_item_position( ait.it );

        if( pos != INT_MIN && ait.count > 0 ) {
            if( res.empty() || res.back().first != pos ) {
                res.emplace_back( pos, ait.count );
            } else {
                res.back().second += ait.count;
            }
        }
    }
    return res;
}

static std::list<act_item> convert_to_items( const player &p, const drop_indexes &drop,
        int min_pos, int max_pos )
{
    std::list<act_item> res;

    for( const auto &rec : drop ) {
        const auto pos = rec.first;
        const auto count = rec.second;

        if( pos < min_pos || pos > max_pos ) {
            continue;
        } else if( pos >= 0 ) {
            int obtained = 0;
            for( const auto &it : p.inv.const_stack( pos ) ) {
                if( obtained >= count ) {
                    break;
                }
                const int qty = it.count_by_charges() ? std::min<int>( it.charges, count - obtained ) : 1;
                obtained += qty;
                res.emplace_back( &it, qty, 100 ); // TODO: Use a calculated cost
            }
        } else {
            res.emplace_back( &p.i_at( pos ), count, pos == -1 ? 0 : 100 ); // TODO: Use a calculated cost
        }
    }

    return res;
}

// Prepares items for dropping by reordering them so that the drop
// cost is minimal and "dependent" items get taken off first.
// Implements the "backpack" logic.
static std::list<act_item> reorder_for_dropping( const player &p, const drop_indexes &drop )
{
    auto res  = convert_to_items( p, drop, -1, -1 );
    auto inv  = convert_to_items( p, drop, 0, INT_MAX );
    auto worn = convert_to_items( p, drop, INT_MIN, -2 );

    // Sort inventory items by volume in ascending order
    inv.sort( []( const act_item & first, const act_item & second ) {
        return first.it->volume() < second.it->volume();
    } );
    // Add missing dependent worn items (if any).
    for( const auto &wait : worn ) {
        for( const auto dit : p.get_dependent_worn_items( *wait.it ) ) {
            const auto iter = std::find_if( worn.begin(), worn.end(),
            [ dit ]( const act_item & ait ) {
                return ait.it == dit;
            } );

            if( iter == worn.end() ) {
                worn.emplace_front( dit, dit->count(), 100 ); // TODO: Use a calculated cost
            }
        }
    }
    // Sort worn items by storage in descending order, but dependent items always go first.
    worn.sort( []( const act_item & first, const act_item & second ) {
        return first.it->is_worn_only_with( *second.it )
               || ( first.it->get_storage() > second.it->get_storage()
                    && !second.it->is_worn_only_with( *first.it ) );
    } );

    units::volume storage_loss = 0_ml;                     // Cumulatively increases
    units::volume remaining_storage = p.volume_capacity(); // Cumulatively decreases

    while( !worn.empty() && !inv.empty() ) {
        storage_loss += worn.front().it->get_storage();
        remaining_storage -= p.volume_capacity_reduced_by( storage_loss );
        units::volume inventory_item_volume = inv.front().it->volume();
        if( remaining_storage < inventory_item_volume ) {
            break; // Does not fit
        }

        while( !inv.empty() && remaining_storage >= inventory_item_volume ) {
            remaining_storage -= inventory_item_volume;

            res.push_back( inv.front() );
            res.back().consumed_moves = 0; // Free of charge

            inv.pop_front();
        }

        res.push_back( worn.front() );
        worn.pop_front();
    }
    // Now insert everything that remains
    std::copy( inv.begin(), inv.end(), std::back_inserter( res ) );
    std::copy( worn.begin(), worn.end(), std::back_inserter( res ) );

    return res;
}

// TODO: Display costs in the multidrop menu
static void debug_drop_list( const std::list<act_item> &list )
{
    if( !debug_mode ) {
        return;
    }

    std::string res( "Items ordered to drop:\n" );
    for( const auto &ait : list ) {
        res += string_format( "Drop %d %s for %d moves\n",
                              ait.count, ait.it->display_name( ait.count ), ait.consumed_moves );
    }
    popup( res, PF_GET_KEY );
}

static std::list<item> obtain_activity_items( player_activity &act, player &p )
{
    std::list<item> res;

    auto items = reorder_for_dropping( p, convert_to_indexes( act ) );

    debug_drop_list( items );

    while( !items.empty() && ( p.is_npc() || p.moves > 0 || items.front().consumed_moves == 0 ) ) {
        const auto &ait = items.front();

        p.mod_moves( -ait.consumed_moves );

        if( p.is_worn( *ait.it ) ) {
            p.takeoff( *ait.it, &res );
        } else if( ait.it->count_by_charges() ) {
            res.push_back( p.reduce_charges( const_cast<item *>( ait.it ), ait.count ) );
        } else {
            res.push_back( p.i_rem( ait.it ) );
        }

        items.pop_front();
    }
    // Avoid tumbling to the ground. Unload cleanly.
    const units::volume excessive_volume = p.volume_carried() - p.volume_capacity();
    if( excessive_volume > 0_ml ) {
        const auto excess = p.inv.remove_randomly_by_volume( excessive_volume );
        res.insert( res.begin(), excess.begin(), excess.end() );
    }
    // Load anything that remains (if any) into the activity
    act.values.clear();
    if( !items.empty() ) {
        for( const auto &drop : convert_to_indexes( p, items ) ) {
            act.values.push_back( drop.first );
            act.values.push_back( drop.second );
        }
    }
    // And cancel if its empty. If its not, we modified in place and we will continue
    // to resolve the drop next turn. This is different from the pickup logic which
    // creates a brand new activity every turn and cancels the old activity
    if( act.values.empty() ) {
        p.cancel_activity();
    }

    return res;
}

void activity_handlers::drop_do_turn( player_activity *act, player *p )
{
    const tripoint pos = act->placement + p->pos();

    bool force_ground = false;
    for( auto &it : act->str_values ) {
        if( it == "force_ground" ) {
            force_ground = true;
            break;
        }
    }

    put_into_vehicle_or_drop( *p, item_drop_reason::deliberate, obtain_activity_items( *act, *p ),
                              pos, force_ground );
}

void activity_on_turn_wear( player_activity &act, player &p )
{
    // ACT_WEAR has item_location targets, and int quatities
    while( p.moves > 0 && !act.targets.empty() ) {
        item_location target = std::move( act.targets.back() );
        int quantity = act.values.back();
        act.targets.pop_back();
        act.values.pop_back();

        if( !target ) {
            debugmsg( "Lost target item of ACT_WEAR" );
            continue;
        }

        // Make copies so the original remains untouched if wearing fails
        item newit = *target;
        item leftovers = newit;

        // Handle charges, quantity == 0 means move all
        if( quantity != 0 && newit.count_by_charges() ) {
            leftovers.charges = newit.charges - quantity;
            if( leftovers.charges > 0 ) {
                newit.charges = quantity;
            }
        } else {
            leftovers.charges = 0;
        }

        if( p.wear_item( newit ) ) {
            // If we wore up a whole stack, remove the original item
            // Otherwise, replace the item with the leftovers
            if( leftovers.charges > 0 ) {
                *target = std::move( leftovers );
            } else {
                target.remove_item();
            }
        }
    }

    // If there are no items left we are done
    if( act.targets.empty() ) {
        p.cancel_activity();
    }
}

void activity_handlers::washing_finish( player_activity *act, player *p )
{
    auto items = reorder_for_dropping( *p, convert_to_indexes( *act ) );

    // Check again that we have enough water and soap incase the amount in our inventory changed somehow
    // Consume the water and soap
    units::volume total_volume = 0_ml;

    for( const act_item &filthy_item : items ) {
        total_volume += filthy_item.it->volume();
    }
    washing_requirements required = washing_requirements_for_volume( total_volume );

    const auto is_liquid_crafting_component = []( const item & it ) {
        return is_crafting_component( it ) && ( !it.count_by_charges() || it.made_of( LIQUID ) ||
                                                it.contents_made_of( LIQUID ) );
    };
    const inventory &crafting_inv = p->crafting_inventory();
    if( !crafting_inv.has_charges( "water", required.water, is_liquid_crafting_component ) &&
        !crafting_inv.has_charges( "water_clean", required.water, is_liquid_crafting_component ) ) {
        p->add_msg_if_player( _( "You need %1$i charges of water or clean water to wash these items." ),
                              required.water );
        act->set_to_null();
        return;
    } else if( !crafting_inv.has_charges( "soap", required.cleanser ) &&
               !crafting_inv.has_charges( "detergent", required.cleanser ) ) {
        p->add_msg_if_player( _( "You need %1$i charges of cleansing agent to wash these items." ),
                              required.cleanser );
        act->set_to_null();
        return;
    }

    for( const auto &ait : items ) {
        item *filthy_item = const_cast<item *>( ait.it );
        filthy_item->item_tags.erase( "FILTHY" );
        p->on_worn_item_washed( *filthy_item );
    }

    std::vector<item_comp> comps;
    comps.push_back( item_comp( "water", required.water ) );
    comps.push_back( item_comp( "water_clean", required.water ) );
    p->consume_items( comps, 1, is_liquid_crafting_component );

    std::vector<item_comp> comps1;
    comps1.push_back( item_comp( "soap", required.cleanser ) );
    comps1.push_back( item_comp( "detergent", required.cleanser ) );
    p->consume_items( comps1 );

    p->add_msg_if_player( m_good, _( "You washed your items." ) );

    // Make sure newly washed components show up as available if player attempts to craft immediately
    p->invalidate_crafting_inventory();

    act->set_to_null();
}

void activity_handlers::stash_do_turn( player_activity *act, player *p )
{
    const tripoint pos = act->placement + p->pos();

    monster *pet = g->critter_at<monster>( pos );
    if( pet != nullptr && pet->has_effect( effect_pet ) ) {
        stash_on_pet( obtain_activity_items( *act, *p ), *pet, p );
    } else {
        p->add_msg_if_player( _( "The pet has moved somewhere else." ) );
        p->cancel_activity();
    }
}

void activity_on_turn_pickup()
{
    // ACT_PICKUP has item_locations of target items and quantities of the same.

    // If we don't have target items bail out
    if( g->u.activity.targets.empty() ) {
        g->u.cancel_activity();
        return;
    }

    // Auto_resume implies autopickup.
    const bool autopickup = g->u.activity.auto_resume;

    // False indicates that the player canceled pickup when met with some prompt
    const bool keep_going = Pickup::do_pickup( g->u.activity.targets, g->u.activity.values,
                            autopickup );

    // If there are items left we ran out of moves, so continue the activity
    // Otherwise, we are done.
    if( !keep_going || g->u.activity.targets.empty() ) {
        g->u.cancel_activity();
    }

    // TODO: Move this to advanced inventory instead of hacking it in here
    if( !keep_going ) {
        cancel_aim_processing();
    }
}

// I'd love to have this not duplicate so much code from Pickup::pick_one_up(),
// but I don't see a clean way to do that.
static void move_items( player &p, const tripoint &relative_dest, bool to_vehicle,
                        std::vector<item_location> &targets, std::vector<int> &quantities )
{
    const tripoint dest = relative_dest + p.pos();

    while( p.moves > 0 && !targets.empty() ) {
        item_location target = std::move( targets.back() );
        int quantity = quantities.back();
        targets.pop_back();
        quantities.pop_back();

        if( !target ) {
            debugmsg( "Lost target item of ACT_MOVE_ITEMS" );
            continue;
        }

        // Don't need to make a copy here since movement can't be canceled
        item &leftovers = *target;
        // Make a copy to be put in the destination location
        item newit = leftovers;

        // Handle charges, quantity == 0 means move all
        if( quantity != 0 && newit.count_by_charges() ) {
            newit.charges = std::min( newit.charges, quantity );
            leftovers.charges -= quantity;
        } else {
            leftovers.charges = 0;
        }

        // Check that we can pick it up.
        if( !newit.made_of_from_type( LIQUID ) ) {
            // This is for hauling across zlevels, remove when going up and down stairs
            // is no longer teleportation
            const tripoint src = target.position();
            int distance = src.z == dest.z ? std::max( rl_dist( src, dest ), 1 ) : 1;
            p.mod_moves( -Pickup::cost_to_move_item( p, newit ) * distance );
            if( to_vehicle ) {
                put_into_vehicle_or_drop( p, item_drop_reason::deliberate, { newit }, dest );
            } else {
                drop_on_map( p, item_drop_reason::deliberate, { newit }, dest );
            }
            // If we picked up a whole stack, remove the leftover item
            if( leftovers.charges <= 0 ) {
                target.remove_item();
            }
        }
    }
}

/*      values explanation
 *      0: items to a vehicle?
 *      1: amount 0 <-+
 *      2: amount 1   |
 *      n: ^----------+
 *
 *      targets correspond to amounts
 */
void activity_on_turn_move_items( player_activity &act, player &p )
{
    // Drop activity if we don't know destination coordinates or have target items.
    if( act.coords.empty() || act.targets.empty() ) {
        act.set_to_null();
        return;
    }

    // Move activity has source square, target square,
    // item_locations of targets, and quantities of same.
    const tripoint relative_dest = act.coords.front();
    const bool to_vehicle = act.values.front();

    // *puts on 3d glasses from 90s cereal box*
    move_items( p, relative_dest, to_vehicle, act.targets, act.values );

    if( act.targets.empty() ) {
        // Nuke the current activity, leaving the backlog alone.
        act.set_to_null();
    }
}

static double get_capacity_fraction( int capacity, int volume )
{
    // fration of capacity the item would occupy
    // fr = 1 is for capacity smaller than is size of item
    // in such case, let's assume player does the trip for full cost with item in hands
    double fr = 1;

    if( capacity > volume ) {
        fr = static_cast<double>( volume ) / capacity;
    }

    return fr;
}

static int move_cost_inv( const item &it, const tripoint &src, const tripoint &dest )
{
    // to prevent potentially ridiculous number
    const int MAX_COST = 500;

    // it seems that pickup cost is flat 100
    // in function pick_one_up, varible moves_taken has initial value of 100
    // and never changes until it is finally used in function
    // remove_from_map_or_vehicle
    const int pickup_cost = 100;

    // drop cost for non-tumbling items (from inventory overload) is also flat 100
    // according to convert_to_items (it does contain todo to use calculated costs)
    const int drop_cost = 100;

    // typical flat ground move cost
    const int mc_per_tile = 100;

    // only free inventory capacity
    const int inventory_capacity = units::to_milliliter( g->u.volume_capacity() -
                                   g->u.volume_carried() );

    const int item_volume = units::to_milliliter( it.volume() );

    const double fr = get_capacity_fraction( inventory_capacity, item_volume );

    // approximation of movement cost between source and destination
    const int move_cost = mc_per_tile * rl_dist( src, dest ) * fr;

    return std::min( pickup_cost + drop_cost + move_cost, MAX_COST );
}

static int move_cost_cart( const item &it, const tripoint &src, const tripoint &dest,
                           const units::volume &capacity )
{
    // to prevent potentially ridiculous number
    const int MAX_COST = 500;

    // cost to move item into the cart
    const int pickup_cost = Pickup::cost_to_move_item( g->u, it );

    // cost to move item out of the cart
    const int drop_cost = pickup_cost;

    // typical flat ground move cost
    const int mc_per_tile = 100;

    // only free cart capacity
    const int cart_capacity = units::to_milliliter( capacity );

    const int item_volume = units::to_milliliter( it.volume() );

    const double fr = get_capacity_fraction( cart_capacity, item_volume );

    // approximation of movement cost between source and destination
    const int move_cost = mc_per_tile * rl_dist( src, dest ) * fr;

    return std::min( pickup_cost + drop_cost + move_cost, MAX_COST );
}

static int move_cost( const item &it, const tripoint &src, const tripoint &dest )
{
    if( g->u.get_grab_type() == OBJECT_VEHICLE ) {
        tripoint cart_position = g->u.pos() + g->u.grab_point;

        if( const cata::optional<vpart_reference> vp = g->m.veh_at(
                    cart_position ).part_with_feature( "CARGO", false ) ) {
            auto veh = vp->vehicle();
            auto vstor = vp->part_index();
            auto capacity = veh.free_volume( vstor );

            return move_cost_cart( it, src, dest, capacity );
        }
    }

    return move_cost_inv( it, src, dest );
}

static void move_item( player &p, item &it, int quantity, const tripoint &src,
                       const tripoint &dest, vehicle *src_veh, int src_part )
{
    item leftovers = it;

    if( quantity != 0 && it.count_by_charges() ) {
        // Reinserting leftovers happens after item removal to avoid stacking issues.
        leftovers.charges = it.charges - quantity;
        if( leftovers.charges > 0 ) {
            it.charges = quantity;
        }
    } else {
        leftovers.charges = 0;
    }

    // Check that we can pick it up.
    if( !it.made_of_from_type( LIQUID ) ) {
        p.mod_moves( -move_cost( it, src, dest ) );
        put_into_vehicle_or_drop( p, item_drop_reason::deliberate, { it }, dest );
        // Remove from map or vehicle.
        if( src_veh ) {
            src_veh->remove_item( src_part, &it );
        } else {
            g->m.i_rem( src, &it );
        }
    }

    // If we didn't pick up a whole stack, put the remainder back where it came from.
    if( leftovers.charges > 0 ) {
        if( src_veh ) {
            if( !src_veh->add_item( src_part, leftovers ) ) {
                debugmsg( "SortLoot: Source vehicle failed to receive leftover charges." );
            }
        } else {
            g->m.add_item_or_charges( src, leftovers );
        }
    }
}

std::vector<tripoint> route_adjacent( const player &p, const tripoint &dest )
{
    auto passable_tiles = std::unordered_set<tripoint>();

    for( const tripoint &tp : g->m.points_in_radius( dest, 1 ) ) {
        if( tp != p.pos() && g->m.passable( tp ) ) {
            passable_tiles.emplace( tp );
        }
    }

    const auto &sorted = get_sorted_tiles_by_distance( p.pos(), passable_tiles );

    const auto &avoid = p.get_path_avoid();
    for( const tripoint &tp : sorted ) {
        auto route = g->m.route( p.pos(), tp, p.get_pathfinding_settings(), avoid );

        if( !route.empty() ) {
            return route;
        }
    }

    return std::vector<tripoint>();
}

static construction check_build_pre( const construction &con )
{
    const std::string pre_con_str = con.pre_terrain;
    construction pre_con = con;
    const std::vector<construction> &list_constructions = get_constructions();
    for( const construction elem : list_constructions ) {
        if( !elem.post_terrain.empty() && elem.post_terrain == pre_con_str &&
            elem.category != string_id<construction_category>( "REPAIR" ) &&
            elem.category != string_id<construction_category>( "REINFORCE" ) ) {
            //we found the construction that could build the required terrain
            pre_con = elem;
            break;
        }
    }
    return pre_con;
}

void activity_on_turn_blueprint_move( player_activity &, player &p )
{
    zone_manager &mgr = zone_manager::get_manager();

    const tripoint abspos = g->m.getabs( p.pos() );
    const std::unordered_set<tripoint> &src_set = mgr.get_near(
                zone_type_id( "CONSTRUCTION_BLUEPRINT" ), abspos );

    const std::vector<tripoint> &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );
    const activity_id act_multiple_construction = activity_id( "ACT_BLUEPRINT_CONSTRUCTION" );

    // Nuke the current activity, leaving the backlog alone
    p.activity = player_activity();

    // sort source tiles by distance
    for( const tripoint &src : src_sorted ) {
        const tripoint &src_loc = g->m.getlocal( src );
        // check if somebodies already started it
        partial_con *nc = g->m.partial_con_at( src_loc );
        if( nc ) {
            continue;
        }

        if( !g->m.inbounds( src_loc ) ) {
            if( !g->m.inbounds( p.pos() ) ) {
                // p is implicitly an NPC that has been moved off the map, so reset the activity
                // and unload them
                p.assign_activity( act_multiple_construction );
                p.set_moves( 0 );
                g->reload_npcs();
                return;
            }
            const std::vector<tripoint> route = route_adjacent( p, src_loc );
            if( route.empty() ) {
                // can't get there, can't do anything, skip it
                continue;
            }
            p.set_destination( route, player_activity( act_multiple_construction ) );
            return;
        }
        // dont go there if it's dangerous.
        bool dangerous_field = false;
        for( const std::pair<const field_type_id, field_entry> &e : g->m.field_at( src_loc ) ) {
            if( p.is_dangerous_field( e.second ) ) {
                dangerous_field = true;
                break;
            }
        }
        if( dangerous_field ) {
            continue;
        }
        // work out if we can build it before we move there.
        const std::vector<zone_data> &zones = mgr.get_zones( zone_type_id( "CONSTRUCTION_BLUEPRINT" ),
                                              g->m.getabs( src_loc ) );
        construction built_chosen;
        const inventory pre_inv = p.crafting_inventory( src_loc, PICKUP_RANGE - 1 );
        // PICKUP_RANGE -1 because we will be adjacent to the spot when arriving.
        bool found_any_pre = false;

        for( const zone_data &zone : zones ) {
            const blueprint_options options = dynamic_cast<const blueprint_options &>( zone.get_options() );
            const int index = options.get_index();
            const std::vector<construction> &list_constructions = get_constructions();
            const construction &built = list_constructions[index];
            // maybe it's already built?
            if( !built.post_terrain.empty() ) {
                if( built.post_is_furniture ) {
                    furn_id f = furn_id( built.post_terrain );
                    if( g->m.furn( src_loc ) == f ) {
                        break;
                    }
                } else {
                    ter_id t = ter_id( built.post_terrain );
                    if( g->m.ter( src_loc ) == t ) {
                        break;
                    }
                }
            }
            if( can_construct( built, src_loc ) && player_can_build( p, pre_inv, built ) ) {
                found_any_pre = true;
                built_chosen = list_constructions[index];
                break;
            } else {
                // cant build it
                // maybe we can build the pre-requisite instead
                // see if the reason is because of pre-terrain requirement
                bool place_okay = true;
                if( !built.pre_terrain.empty() ) {
                    if( built.pre_is_furniture ) {
                        furn_id f = furn_id( built.pre_terrain );
                        place_okay &= g->m.furn( src_loc ) == f;
                    } else {
                        ter_id t = ter_id( built.pre_terrain );
                        place_okay &= g->m.ter( src_loc ) == t;
                    }
                }
                if( !place_okay ) {
                    built_chosen = check_build_pre( built );
                    // We only got here because the original choice cant be constructed
                    // Check again, if we still have the same construction, itll fail again.
                    if( can_construct( built_chosen, src_loc ) && player_can_build( p, pre_inv, built_chosen ) ) {
                        found_any_pre = true;
                        break;
                    }
                }
                continue;
            }
        }
        if( !found_any_pre ) {
            continue;
        }
        bool adjacent = false;
        for( const tripoint &elem : g->m.points_in_radius( src_loc, 1 ) ) {
            if( p.pos() == elem ) {
                adjacent = true;
                break;
            }
        }
        if( !adjacent ) {
            std::vector<tripoint> route = route_adjacent( p, src_loc );

            // check if we found path to source / adjacent tile
            if( route.empty() ) {
                add_msg( m_info, _( "%s can't reach the source tile to construct." ),
                         p.disp_name() );
                return;
            }

            // set the destination and restart activity after player arrives there
            // we don't need to check for safe mode,
            // activity will be restarted only if
            // player arrives on destination tile
            p.set_destination( route, player_activity( act_multiple_construction ) );
            return;
        }
        // if it's too dark to construct there
        const bool enough_light = p.fine_detail_vision_mod() <= 4;
        if( !enough_light ) {
            p.add_msg_if_player( m_info, _( "It is too dark to construct anything." ) );
            return;
        }
        // check if can do the construction now we are actually there
        const std::vector<zone_data> &post_zones = mgr.get_zones( zone_type_id( "CONSTRUCTION_BLUEPRINT" ),
                g->m.getabs( src_loc ) );
        construction post_built_chosen;
        p.invalidate_crafting_inventory();
        const inventory &total_inv = p.crafting_inventory();
        bool found_any = false;

        for( const zone_data &zone : post_zones ) {
            const blueprint_options options = dynamic_cast<const blueprint_options &>( zone.get_options() );
            const int index = options.get_index();
            const std::vector<construction> &list_constructions = get_constructions();
            const construction &built = list_constructions[index];
            // maybe it's already built?
            if( !built.post_terrain.empty() ) {
                if( built.post_is_furniture ) {
                    furn_id f = furn_id( built.post_terrain );
                    if( g->m.furn( src_loc ) == f ) {
                        break;
                    }
                } else {
                    ter_id t = ter_id( built.post_terrain );
                    if( g->m.ter( src_loc ) == t ) {
                        break;
                    }
                }
            }
            if( can_construct( built, src_loc ) && player_can_build( p, total_inv, built ) ) {
                found_any = true;
                post_built_chosen = list_constructions[index];
                break;
            } else {
                // cant build it
                // maybe we can build the pre-requisite instead
                // see if the reason is because of pre-terrain requirement
                bool place_okay = true;
                if( !built.pre_terrain.empty() ) {
                    if( built.pre_is_furniture ) {
                        furn_id f = furn_id( built.pre_terrain );
                        place_okay &= g->m.furn( src_loc ) == f;
                    } else {
                        ter_id t = ter_id( built.pre_terrain );
                        place_okay &= g->m.ter( src_loc ) == t;
                    }
                }
                if( !place_okay ) {
                    post_built_chosen = check_build_pre( built );
                    if( can_construct( post_built_chosen, src_loc ) &&
                        player_can_build( p, total_inv, post_built_chosen ) ) {
                        found_any = true;
                        break;
                    }
                }
                continue;
            }
        }
        if( !found_any ) {
            continue;
        }
        std::list<item> used;
        // create the partial construction struct
        partial_con pc;
        pc.id = built_chosen.id;
        pc.counter = 0;
        // Set the trap that has the examine function
        if( g->m.tr_at( src_loc ).loadid == tr_null ) {
            g->m.trap_set( src_loc, tr_unfinished_construction );
        }
        // Use up the components
        for( const std::vector<item_comp> &it : built_chosen.requirements->get_components() ) {
            std::list<item> tmp = p.consume_items( it, 1, is_crafting_component );
            used.splice( used.end(), tmp );
        }
        pc.components = used;
        g->m.partial_con_set( src_loc, pc );
        for( const std::vector<tool_comp> &it : built_chosen.requirements->get_tools() ) {
            p.consume_tools( it );
        }
        p.backlog.push_front( act_multiple_construction );
        p.assign_activity( activity_id( "ACT_BUILD" ) );
        p.activity.placement = g->m.getabs( src_loc );
        return;
    }
    if( p.moves <= 0 ) {
        // Restart activity and break from cycle.
        p.assign_activity( act_multiple_construction );
        return;
    }

    // If we got here without restarting the activity, it means we're done.
    if( p.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &p );
        guy->current_activity.clear();
        guy->revert_after_activity();
    }
}

void activity_on_turn_move_loot( player_activity &, player &p )
{
    const activity_id act_move_loot = activity_id( "ACT_MOVE_LOOT" );
    auto &mgr = zone_manager::get_manager();
    if( g->m.check_vehicle_zones( g->get_levz() ) ) {
        mgr.cache_vzones();
    }
    const auto abspos = g->m.getabs( p.pos() );
    const auto &src_set = mgr.get_near( zone_type_id( "LOOT_UNSORTED" ), abspos );
    vehicle *src_veh, *dest_veh;
    int src_part, dest_part;

    // Nuke the current activity, leaving the backlog alone.
    p.activity = player_activity();

    // sort source tiles by distance
    const auto &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );

    if( !mgr.is_sorting() ) {
        mgr.start_sort( src_sorted );
    }

    for( auto &src : src_sorted ) {
        const auto &src_loc = g->m.getlocal( src );
        if( !g->m.inbounds( src_loc ) ) {
            if( !g->m.inbounds( p.pos() ) ) {
                // p is implicitly an NPC that has been moved off the map, so reset the activity
                // and unload them
                p.assign_activity( act_move_loot );
                p.set_moves( 0 );
                g->reload_npcs();
                mgr.end_sort();
                return;
            }
            std::vector<tripoint> route;
            route = g->m.route( p.pos(), src_loc, p.get_pathfinding_settings(),
                                p.get_path_avoid() );
            if( route.empty() ) {
                // can't get there, can't do anything, skip it
                continue;
            }
            p.set_destination( route, player_activity( act_move_loot ) );
            mgr.end_sort();
            return;
        }

        bool is_adjacent_or_closer = square_dist( p.pos(), src_loc ) <= 1;

        // skip tiles in IGNORE zone and tiles on fire
        // (to prevent taking out wood off the lit brazier)
        // and inaccessible furniture, like filled charcoal kiln
        if( mgr.has( zone_type_id( "LOOT_IGNORE" ), src ) ||
            g->m.get_field( src_loc, fd_fire ) != nullptr ||
            !g->m.can_put_items_ter_furn( src_loc ) ) {
            continue;
        }

        // the boolean in this pair being true indicates the item is from a vehicle storage space
        auto items = std::vector<std::pair<item *, bool>>();

        //Check source for cargo part
        //map_stack and vehicle_stack are different types but inherit from item_stack
        // TODO: use one for loop
        if( const cata::optional<vpart_reference> vp = g->m.veh_at( src_loc ).part_with_feature( "CARGO",
                false ) ) {
            src_veh = &vp->vehicle();
            src_part = vp->part_index();
            for( auto &it : src_veh->get_items( src_part ) ) {
                items.push_back( std::make_pair( &it, true ) );
            }
        } else {
            src_veh = nullptr;
            src_part = -1;
        }
        for( auto &it : g->m.i_at( src_loc ) ) {
            items.push_back( std::make_pair( &it, false ) );
        }
        //Skip items that have already been processed
        for( auto it = items.begin() + mgr.get_num_processed( src ); it < items.end(); it++ ) {

            mgr.increment_num_processed( src );

            const auto thisitem = it->first;

            if( thisitem->made_of_from_type( LIQUID ) ) { // skip unpickable liquid
                continue;
            }

            // Only if it's from a vehicle do we use the vehicle source location information.
            vehicle *this_veh = it->second ? src_veh : nullptr;
            const int this_part = it->second ? src_part : -1;

            const auto id = mgr.get_near_zone_type_for_item( *thisitem, abspos );

            // checks whether the item is already on correct loot zone or not
            // if it is, we can skip such item, if not we move the item to correct pile
            // think empty bag on food pile, after you ate the content
            if( !mgr.has( id, src ) ) {
                const auto &dest_set = mgr.get_near( id, abspos, 60, thisitem );

                for( auto &dest : dest_set ) {
                    const auto &dest_loc = g->m.getlocal( dest );

                    //Check destination for cargo part
                    if( const cata::optional<vpart_reference> vp = g->m.veh_at( dest_loc ).part_with_feature( "CARGO",
                            false ) ) {
                        dest_veh = &vp->vehicle();
                        dest_part = vp->part_index();
                    } else {
                        dest_veh = nullptr;
                        dest_part = -1;
                    }

                    // skip tiles with inaccessible furniture, like filled charcoal kiln
                    if( !g->m.can_put_items_ter_furn( dest_loc ) ) {
                        continue;
                    }

                    units::volume free_space;
                    // if there's a vehicle with space do not check the tile beneath
                    if( dest_veh ) {
                        free_space = dest_veh->free_volume( dest_part );
                    } else {
                        free_space = g->m.free_volume( dest_loc );
                    }
                    // check free space at destination
                    if( free_space >= thisitem->volume() ) {
                        // before we move any item, check if player is at or
                        // adjacent to the loot source tile
                        if( !is_adjacent_or_closer ) {
                            std::vector<tripoint> route;
                            bool adjacent = false;

                            // get either direct route or route to nearest adjacent tile if
                            // source tile is impassable
                            if( g->m.passable( src_loc ) ) {
                                route = g->m.route( p.pos(), src_loc, p.get_pathfinding_settings(),
                                                    p.get_path_avoid() );
                            } else {
                                // immpassable source tile (locker etc.),
                                // get route to nerest adjacent tile instead
                                route = route_adjacent( p, src_loc );
                                adjacent = true;
                            }

                            // check if we found path to source / adjacent tile
                            if( route.empty() ) {
                                add_msg( m_info, _( "%s can't reach the source tile. Try to sort out loot without a cart." ),
                                         p.disp_name() );
                                mgr.end_sort();
                                return;
                            }

                            // shorten the route to adjacent tile, if necessary
                            if( !adjacent ) {
                                route.pop_back();
                            }

                            // set the destination and restart activity after player arrives there
                            // we don't need to check for safe mode,
                            // activity will be restarted only if
                            // player arrives on destination tile
                            p.set_destination( route, player_activity( act_move_loot ) );
                            mgr.end_sort();
                            return;
                        }
                        move_item( p, *thisitem, thisitem->count(), src_loc, dest_loc, this_veh, this_part );

                        // moved item away from source so decrement
                        mgr.decrement_num_processed( src );

                        break;
                    }
                }
                if( p.moves <= 0 ) {
                    // Restart activity and break from cycle.
                    p.assign_activity( act_move_loot );
                    mgr.end_sort();
                    return;
                }
            }
        }
    }

    // If we got here without restarting the activity, it means we're done
    add_msg( m_info, string_format( _( "%s sorted out every item possible." ), p.disp_name() ) );
    if( p.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &p );
        guy->current_activity.clear();
    }
    mgr.end_sort();
}

static cata::optional<tripoint> find_best_fire(
    const std::vector<tripoint> &from, const tripoint &center )
{
    cata::optional<tripoint> best_fire;
    time_duration best_fire_age = 1_days;
    for( const tripoint &pt : from ) {
        field_entry *fire = g->m.get_field( pt, fd_fire );
        if( fire == nullptr || fire->get_field_intensity() > 1 ||
            !g->m.clear_path( center, pt, PICKUP_RANGE, 1, 100 ) ) {
            continue;
        }
        time_duration fire_age = fire->get_field_age();
        // Refuel only the best fueled fire (if it needs it)
        if( fire_age < best_fire_age ) {
            best_fire = pt;
            best_fire_age = fire_age;
        }
        // If a contained fire exists, ignore any other fires
        if( g->m.has_flag_furn( TFLAG_FIRE_CONTAINER, pt ) ) {
            return pt;
        }
    }

    return best_fire;
}

void try_fuel_fire( player_activity &act, player &p, const bool starting_fire )
{
    const tripoint pos = p.pos();
    auto adjacent = closest_tripoints_first( PICKUP_RANGE, pos );
    adjacent.erase( adjacent.begin() );

    cata::optional<tripoint> best_fire = starting_fire ? act.placement : find_best_fire( adjacent,
                                         pos );

    if( !best_fire || !g->m.accessible_items( *best_fire ) ) {
        return;
    }

    const auto refuel_spot = std::find_if( adjacent.begin(), adjacent.end(),
    [pos]( const tripoint & pt ) {
        // Hacky - firewood spot is a trap and it's ID-checked
        // TODO: Something cleaner than ID-checking a trap
        return g->m.tr_at( pt ).id == tr_firewood_source && g->m.has_items( pt ) &&
               g->m.accessible_items( pt ) && g->m.clear_path( pos, pt, PICKUP_RANGE, 1, 100 );
    } );
    if( refuel_spot == adjacent.end() ) {
        return;
    }

    // Special case: fire containers allow burning logs, so use them as fuel iif fire is contained
    bool contained = g->m.has_flag_furn( TFLAG_FIRE_CONTAINER, *best_fire );
    fire_data fd( 1, contained );
    time_duration fire_age = g->m.get_field_age( *best_fire, fd_fire );

    // Maybe TODO: - refueling in the rain could use more fuel
    // First, simulate expected burn per turn, to see if we need more fuel
    map_stack fuel_on_fire = g->m.i_at( *best_fire );
    for( item &it : fuel_on_fire ) {
        it.simulate_burn( fd );
        // Unconstrained fires grow below -50_minutes age
        if( !contained && fire_age < -40_minutes && fd.fuel_produced > 1.0f && !it.made_of( LIQUID ) ) {
            // Too much - we don't want a firestorm!
            // Move item back to refueling pile
            // Note: this handles messages (they're the generic "you drop x")
            drop_on_map( p, item_drop_reason::deliberate, { it }, *refuel_spot );

            const int distance = std::max( rl_dist( *best_fire, *refuel_spot ), 1 );
            p.mod_moves( -Pickup::cost_to_move_item( p, it ) * distance );

            g->m.i_rem( *best_fire, &it );
            return;
        }
    }

    // Enough to sustain the fire
    // TODO: It's not enough in the rain
    if( !starting_fire && ( fd.fuel_produced >= 1.0f || fire_age < 10_minutes ) ) {
        return;
    }

    // We need to move fuel from stash to fire
    map_stack potential_fuel = g->m.i_at( *refuel_spot );
    for( item &it : potential_fuel ) {
        if( it.made_of( LIQUID ) ) {
            continue;
        }

        float last_fuel = fd.fuel_produced;
        it.simulate_burn( fd );
        if( fd.fuel_produced > last_fuel ) {
            // Note: this handles messages (they're the generic "you drop x")
            drop_on_map( p, item_drop_reason::deliberate, { it }, *best_fire );

            const int distance = std::max( rl_dist( *refuel_spot, *best_fire ), 1 );
            p.mod_moves( -Pickup::cost_to_move_item( p, it ) * distance );

            g->m.i_rem( *refuel_spot, &it );
            return;
        }
    }
}
