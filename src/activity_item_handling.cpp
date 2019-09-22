#include "activity_handlers.h" // IWYU pragma: associated

#include <climits>
#include <algorithm>
#include <list>
#include <iostream>
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
#include "iuse.h"
#include "iexamine.h"
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
#include "flat_set.h"
#include "int_id.h"
#include "item_location.h"
#include "point.h"
#include "string_id.h"

struct construction_category;

void cancel_aim_processing();

const efftype_id effect_controlled( "controlled" );
const efftype_id effect_pet( "pet" );

const zone_type_id zone_source_firewood( "SOURCE_FIREWOOD" );
const zone_type_id z_loot_unsorted( "LOOT_UNSORTED" );

const quality_id LIFT( "LIFT" );

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
    return std::all_of( items.begin(), items.end(), [&items]( const item & it ) {
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

void drop_on_map( Character &c, item_drop_reason reason, const std::list<item> &items,
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
    auto res = convert_to_items( p, drop, -1, -1 );
    auto inv = convert_to_items( p, drop, 0, INT_MAX );
    auto worn = convert_to_items( p, drop, INT_MIN, -2 );

    // Sort inventory items by volume in ascending order
    inv.sort( []( const act_item & first, const act_item & second ) {
        return first.it->volume() < second.it->volume();
    } );
    // Add missing dependent worn items (if any).
    for( const auto &wait : worn ) {
        for( const auto dit : p.get_dependent_worn_items( *wait.it ) ) {
            const auto iter = std::find_if( worn.begin(), worn.end(),
            [dit]( const act_item & ait ) {
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
        if( g->u.get_value( "THIEF_MODE_KEEP" ) != "YES" ) {
            g->u.set_value( "THIEF_MODE", "THIEF_ASK" );
        }
    }

    // TODO: Move this to advanced inventory instead of hacking it in here

    if( !keep_going ) {
        // The user canceled the activity, so we're done
        g->u.cancel_activity();
        // AIM might have more pickup activities pending, also cancel them.
        // TODO: Move this to advanced inventory instead of hacking it in here
        cancel_aim_processing();
    } else if( g->u.activity.targets.empty() ) {
        // The user did not cancel, but there's no item left
        g->u.cancel_activity();
        // But do not cancel AIM processing as it might have more pickup activities
        // pending for other locations.
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
            if( !newit.has_owner() && p.is_player() ) {
                newit.set_owner( p.get_faction() );
            }
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

static void vehicle_activity( player &p, const tripoint src_loc, int vpindex, char type )
{
    vehicle *veh = veh_pointer_or_null( g->m.veh_at( src_loc ) );
    if( !veh ) {
        return;
    }
    int time_to_take = 0;
    if( vpindex >= static_cast<int>( veh->parts.size() ) ) {
        // if parts got removed dduring our work, we cant just carry on removing, we want to repair parts!
        // so just bail out, as we dont know if th enext shifted part is suitable for repair.
        if( type == 'r' ) {
            return;
        } else if( type == 'o' ) {
            vpindex = veh->get_next_shifted_index( vpindex, p );
            if( vpindex == -1 ) {
                return;
            }
        }
    }
    const vpart_info &vp = veh->part_info( vpindex );
    const vehicle_part part = veh->parts[ vpindex ];
    if( type == 'r' ) {
        time_to_take = vp.repair_time( p ) * part.damage() / part.max_damage();
    } else if( type == 'o' ) {
        time_to_take = vp.removal_time( p );
    }
    p.assign_activity( activity_id( "ACT_VEHICLE" ), time_to_take, static_cast<int>( type ) );
    // so , NPCs can remove the last part on a position, then there is no vehicle there anymore,
    // for someone else who stored that position at the start of their activity.
    // so we may need to go looking a bit further afield to find it , at activities end.
    for( const auto pt : veh->get_points( true ) ) {
        p.activity.coord_set.insert( g->m.getabs( pt ) );
    }
    p.activity.values.push_back( g->m.getabs( src_loc ).x );   // values[0]
    p.activity.values.push_back( g->m.getabs( src_loc ).y );   // values[1]
    p.activity.values.push_back( point_zero.x );   // values[2]
    p.activity.values.push_back( point_zero.y );   // values[3]
    p.activity.values.push_back( -point_zero.x );   // values[4]
    p.activity.values.push_back( -point_zero.y );   // values[5]
    p.activity.values.push_back( veh->index_of_part( &veh->parts[vpindex] ) ); // values[6]
    p.activity.str_values.push_back( vp.get_id().str() );
    // this would only be used for refilling tasks
    item_location target;
    p.activity.targets.emplace_back( std::move( target ) );
    p.activity.placement = g->m.getabs( src_loc );
    p.activity_vehicle_part_index = -1;
}

static void move_item( player &p, item &it, int quantity, const tripoint &src,
                       const tripoint &dest, vehicle *src_veh, int src_part,
                       activity_id activity_to_restore = activity_id::NULL_ID() )
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
        if( activity_to_restore == activity_id( "ACT_TIDY_UP" ) ) {
            it.erase_var( "activity_var" );
        } else if( activity_to_restore == activity_id( "ACT_FETCH_REQUIRED" ) ) {
            it.set_var( "activity_var", p.name );
        }
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

static activity_reason_info find_base_construction(
    const std::vector<construction> &list_constructions,
    player &p,
    const inventory &inv,
    const tripoint &loc,
    const cata::optional<size_t> part_con_idx,
    const size_t idx,
    std::set<size_t> &used )
{
    const construction &build = list_constructions[idx];
    //already done?
    const furn_id furn = g->m.furn( loc );
    const ter_id ter = g->m.ter( loc );
    if( !build.post_terrain.empty() ) {
        if( build.post_is_furniture ) {
            if( furn_id( build.post_terrain ) == furn ) {
                return activity_reason_info::build( ALREADY_DONE, false, idx );
            }
        } else {
            if( ter_id( build.post_terrain ) == ter ) {
                return activity_reason_info::build( ALREADY_DONE, false, idx );
            }
        }
    }
    //if theres an apropriate partial construction on the tile, then we can work on it, no need to check inventories.
    const bool has_skill = p.meets_skill_requirements( build );
    if( part_con_idx && *part_con_idx == idx ) {
        if( !has_skill ) {
            return activity_reason_info::build( DONT_HAVE_SKILL, false, idx );
        }
        return activity_reason_info::build( CAN_DO_CONSTRUCTION, true, idx );
    }
    //can build?
    const bool cc = can_construct( build, loc );
    const bool pcb = player_can_build( p, inv, build );
    if( !has_skill ) {
        return activity_reason_info::build( DONT_HAVE_SKILL, false, idx );
    }
    if( cc ) {
        if( pcb ) {
            return activity_reason_info::build( CAN_DO_CONSTRUCTION, true, idx );
        }
        //can't build with current inventory, do not look for pre-req
        return activity_reason_info::build( NO_COMPONENTS, false, idx );
    }

    // there are no pre-requisites.
    // so we need to potentially fetch components
    if( build.pre_terrain.empty() && build.pre_special( loc ) ) {
        return activity_reason_info::build( NO_COMPONENTS, false, idx );
    } else if( !build.pre_special( loc ) ) {
        return activity_reason_info::build( BLOCKING_TILE, false, idx );
    }

    // cant build it
    // maybe we can build the pre-requisite instead
    // see if the reason is because of pre-terrain requirement
    if( !build.pre_terrain.empty() &&
        ( ( build.pre_is_furniture &&
            furn_id( build.pre_terrain ) == furn ) ||
          ( !build.pre_is_furniture &&
            ter_id( build.pre_terrain ) == ter ) ) ) {
        // the pre-req is already built, so the reason is due to lack of tools/components
        return activity_reason_info::build( NO_COMPONENTS, false, idx );
    }

    //we can't immediately build it, looking for pre-req
    used.insert( idx );
    cata::optional<do_activity_reason> reason;
    size_t pre_req_idx = 0;
    //first step: try only constructions with the same description
    //second step: try all constructions
    for( int try_num = 0; try_num < 2; ++try_num ) {
        for( const construction &pre_build : list_constructions ) {
            //skip if already checked this one
            if( pre_build.id == idx || used.find( pre_build.id ) != used.end() ) {
                continue;
            }
            //skip unknown
            if( pre_build.post_terrain.empty() ) {
                continue;
            }
            //skip if it is not a pre-build required or gives the same result
            if( pre_build.post_terrain != build.pre_terrain &&
                pre_build.post_terrain != build.post_terrain ) {
                continue;
            }
            //at first step, try to get building with the same description
            if( try_num == 0 &&
                pre_build.description != build.description ) {
                continue;
            }
            activity_reason_info act_info_pre = find_base_construction( list_constructions,
                                                p, inv, loc, part_con_idx, pre_build.id, used );
            if( act_info_pre.can_do ) {
                return activity_reason_info::build( CAN_DO_PREREQ, true, *act_info_pre.con_idx );
            }
            //find first pre-req failed reason
            if( !reason ) {
                reason = act_info_pre.reason;
                pre_req_idx = *act_info_pre.con_idx;
            }
            if( act_info_pre.reason == ALREADY_DONE ) {
                //pre-req is already here, but we still can't build over it
                reason.reset();
                break;
            }
        }
    }
    //have a partial construction which is not leading to the required construction
    if( part_con_idx ) {
        return activity_reason_info::build( BLOCKING_TILE, false, idx );
    }
    //pre-req failed?
    if( reason ) {
        if( *reason == NO_COMPONENTS ) {
            return activity_reason_info::build( NO_COMPONENTS_PREREQ, false, pre_req_idx );
        }
        return activity_reason_info::build( *reason, false, pre_req_idx );
    }
    if( !pcb ) {
        return activity_reason_info::build( NO_COMPONENTS, false, idx );
    }
    //only cc failed, no pre-req
    return activity_reason_info::build( BLOCKING_TILE, false, idx );
}

static std::string random_string( size_t length )
{
    auto randchar = []() -> char {
        const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = ( sizeof( charset ) - 1 );
        return charset[rand() % max_index];
    };
    std::string str( length, 0 );
    std::generate_n( str.begin(), length, randchar );
    return str;
}

static bool are_requirements_nearby( const std::vector<tripoint> &loot_spots,
                                     const requirement_id &needed_things, player &p, const activity_id activity_to_restore,
                                     bool in_loot_zones )
{
    zone_manager &mgr = zone_manager::get_manager();
    inventory temp_inv;
    units::volume volume_allowed = p.volume_capacity() - p.volume_carried();
    units::mass weight_allowed = p.weight_capacity() - p.weight_carried();
    const bool check_weight = p.backlog.front().id() == activity_id( "ACT_MULTIPLE_FARM" ) ||
                              activity_to_restore == activity_id( "ACT_MULTIPLE_FARM" ) ||
                              p.backlog.front().id() == activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ) ||
                              activity_to_restore == activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ) ||
                              p.backlog.front().id() == activity_id( "ACT_MULTIPLE_BUTCHER" ) ||
                              activity_to_restore == activity_id( "ACT_MULTIPLE_BUTCHER" ) ||
                              p.backlog.front().id() == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ||
                              activity_to_restore == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ||
                              p.backlog.front().id() == activity_id( "ACT_VEHICLE_REPAIR" ) ||
                              activity_to_restore == activity_id( "ACT_VEHICLE_REPAIR" ) ||
                              p.backlog.front().id() == activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ||
                              activity_to_restore == activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ||
                              p.backlog.front().id() == activity_id( "ACT_MULTIPLE_FISH" ) ||
                              activity_to_restore == activity_id( "ACT_MULTIPLE_FISH" );
    for( const tripoint &elem : loot_spots ) {
        // if we are searching for things to fetch, we can skip certain thngs.
        // if, however they are already near the work spot, then the crafting / inventory fucntions will have their own method to use or discount them.
        if( in_loot_zones ) {
            // skip tiles in IGNORE zone and tiles on fire
            // (to prevent taking out wood off the lit brazier)
            // and inaccessible furniture, like filled charcoal kiln
            if( mgr.has( zone_type_id( "LOOT_IGNORE" ), g->m.getabs( elem ) ) ||
                g->m.dangerous_field_at( elem ) ||
                !g->m.can_put_items_ter_furn( elem ) ) {
                continue;
            }
        }
        for( const auto &elem2 : g->m.i_at( elem ) ) {
            if( in_loot_zones && elem2.made_of_from_type( LIQUID ) ) {
                continue;
            }
            if( check_weight ) {
                // this fetch task will need to pick up an item. so check for its weight/volume before setting off.
                if( in_loot_zones && ( elem2.volume() > volume_allowed ||
                                       elem2.weight() > weight_allowed ) ) {
                    continue;
                }
            }
            temp_inv += elem2;
        }
        if( !in_loot_zones ) {
            if( const cata::optional<vpart_reference> vp = g->m.veh_at( elem ).part_with_feature( "CARGO",
                    false ) ) {
                vehicle &src_veh = vp->vehicle();
                int src_part = vp->part_index();
                for( auto &it : src_veh.get_items( src_part ) ) {
                    temp_inv += it;
                }
            }
        }
    }
    for( item *elem : p.inv_dump() ) {
        temp_inv += *elem;
    }
    return needed_things.obj().can_make_with_inventory( temp_inv, is_crafting_component );
}

static bool has_skill_for_vehicle_work( std::map<skill_id, int> required_skills, player &p )
{
    for( const auto &e : required_skills ) {
        bool hasSkill = p.get_skill_level( e.first ) >= e.second;
        if( !hasSkill ) {
            return false;
        }
    }
    return true;
}

static activity_reason_info can_do_activity_there( const activity_id &act, player &p,
        const tripoint &src_loc )
{
    // see activity_handlers.h cant_do_activity_reason enums
    zone_manager &mgr = zone_manager::get_manager();
    std::vector<zone_data> zones;
    if( act == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ||
        act == activity_id( "ACT_VEHICLE_REPAIR" ) ) {
        std::vector<int> already_working_indexes;
        vehicle *veh = veh_pointer_or_null( g->m.veh_at( src_loc ) );
        if( !veh ) {
            return activity_reason_info::fail( NO_ZONE );
        }
        // if the vehicle is moving or player is controlling it.
        if( abs( veh->velocity ) > 100 || veh->player_in_control( g->u ) ) {
            return activity_reason_info::fail( NO_ZONE );
        }
        for( const npc &guy : g->all_npcs() ) {
            if( &guy == &p ) {
                continue;
            }
            // If the NPC has an activity - make sure theyre not duplicating work.
            tripoint guy_work_spot;
            if( guy.has_player_activity() && guy.activity.placement != tripoint_min ) {
                guy_work_spot = g->m.getlocal( guy.activity.placement );
            }
            // If their position or intended position or player position/intended position
            // then discount, dont need to move each other out of the way.
            if( g->m.getlocal( g->u.activity.placement ) == src_loc ||
                guy_work_spot == src_loc || guy.pos() == src_loc || ( p.is_npc() && g->u.pos() == src_loc ) ) {
                return activity_reason_info::fail( ALREADY_WORKING );
            }
            if( guy_work_spot != tripoint_zero ) {
                vehicle *other_veh = veh_pointer_or_null( g->m.veh_at( guy_work_spot ) );
                // working on same vehicle - store the index to check later.
                if( other_veh && other_veh == veh && guy.activity_vehicle_part_index != -1 ) {
                    already_working_indexes.push_back( guy.activity_vehicle_part_index );
                }
            }
            if( g->u.activity_vehicle_part_index != -1 ) {
                already_working_indexes.push_back( g->u.activity_vehicle_part_index );
            }
        }
        if( act == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ) {
            // find out if there is a vehicle part here we can remove.
            std::vector<vehicle_part *> parts = veh->get_parts_at( src_loc, "", part_status_flag::any );
            for( vehicle_part *part_elem : parts ) {
                const vpart_info &vpinfo = part_elem->info();
                int vpindex = veh->index_of_part( part_elem, true );
                // if part is not on this vehicle, or if its attached to another part that needs to be removed first.
                if( vpindex == -1 || !veh->can_unmount( vpindex ) ) {
                    continue;
                }
                // this is the same part that somebody else wants to work on, or already is.
                if( std::find( already_working_indexes.begin(), already_working_indexes.end(),
                               vpindex ) != already_working_indexes.end() ) {
                    continue;
                }
                // dont have skill to remove it
                if( !has_skill_for_vehicle_work( vpinfo.removal_skills, p ) ) {
                    continue;
                }
                item base( vpinfo.item );
                if( base.is_wheel() ) {
                    // no wheel removal yet
                    continue;
                }
                const int max_lift = p.best_nearby_lifting_assist( src_loc );
                const int lvl = ceil( units::quantity<double, units::mass::unit_type>( base.weight() ) /
                                      TOOL_LIFT_FACTOR );
                const bool use_aid = max_lift >= lvl;
                const bool use_str = p.can_lift( base );
                if( !( use_aid || use_str ) ) {
                    continue;
                }
                const auto &reqs = vpinfo.removal_requirements();
                const inventory &inv = p.crafting_inventory();
                const bool can_make = reqs.can_make_with_inventory( inv, is_crafting_component );
                p.set_value( "veh_index_type", vpinfo.name() );
                // temporarily store the intended index, we do this so two NPCs dont try and work on the same part at same time.
                p.activity_vehicle_part_index = vpindex;
                if( !can_make ) {
                    return activity_reason_info::fail( NEEDS_VEH_DECONST );
                } else {
                    return activity_reason_info::ok( NEEDS_VEH_DECONST );
                }
            }
        } else if( act == activity_id( "ACT_VEHICLE_REPAIR" ) ) {
            // find out if there is a vehicle part here we can repair.
            std::vector<vehicle_part *> parts = veh->get_parts_at( src_loc, "", part_status_flag::any );
            for( vehicle_part *part_elem : parts ) {
                const vpart_info &vpinfo = part_elem->info();
                int vpindex = veh->index_of_part( part_elem, true );
                // if part is undamaged or beyond repair - can skip it.
                if( part_elem->is_broken() || part_elem->damage() == 0 ) {
                    continue;
                }
                if( std::find( already_working_indexes.begin(), already_working_indexes.end(),
                               vpindex ) != already_working_indexes.end() ) {
                    continue;
                }
                // dont have skill to repair it
                if( !has_skill_for_vehicle_work( vpinfo.repair_skills, p ) ) {
                    continue;
                }
                const auto &reqs = vpinfo.repair_requirements();
                const inventory &inv = p.crafting_inventory();
                const bool can_make = reqs.can_make_with_inventory( inv, is_crafting_component );
                p.set_value( "veh_index_type", vpinfo.name() );
                // temporarily store the intended index, we do this so two NPCs dont try and work on the same part at same time.
                p.activity_vehicle_part_index = vpindex;
                if( !can_make ) {
                    return activity_reason_info::fail( NEEDS_VEH_REPAIR );
                } else {
                    return activity_reason_info::ok( NEEDS_VEH_REPAIR );
                }
            }
        }
        p.activity_vehicle_part_index = -1;
        return activity_reason_info::fail( NO_ZONE );
    }
    if( act == activity_id( "ACT_MULTIPLE_FISH" ) ) {
        if( !g->m.has_flag( "FISHABLE", src_loc ) ) {
            return activity_reason_info::fail( NO_ZONE );
        }
        std::vector<item *> rod_inv = p.items_with( []( const item & itm ) {
            return itm.has_flag( "FISH_POOR" ) || itm.has_flag( "FISH_GOOD" );
        } );
        if( rod_inv.empty() ) {
            return activity_reason_info::fail( NEEDS_FISHING );
        } else {
            return activity_reason_info::ok( NEEDS_FISHING );
        }
    }
    if( act == activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ) {
        if( g->m.has_flag( "TREE", src_loc ) || g->m.ter( src_loc ) == t_trunk ||
            g->m.ter( src_loc ) == t_stump ) {
            if( p.has_quality( quality_id( "AXE" ) ) ) {
                return activity_reason_info::ok( NEEDS_TREE_CHOPPING );
            } else {
                return activity_reason_info::fail( NEEDS_TREE_CHOPPING );
            }
        } else {
            return activity_reason_info::fail( NO_ZONE );
        }
    }
    if( act == activity_id( "ACT_MULTIPLE_BUTCHER" ) ) {
        std::vector<item> corpses;
        int big_count = 0;
        int small_count = 0;
        for( const auto &i : g->m.i_at( src_loc ) ) {
            // make sure nobody else is working on that corpse right now
            if( i.is_corpse() && !i.has_var( "activity_var" ) ) {
                const mtype corpse = *i.get_mtype();
                if( corpse.size >= MS_MEDIUM ) {
                    big_count += 1;
                } else {
                    small_count += 1;
                }
                corpses.push_back( i );
            }
        }
        bool has_table_nearby = false;
        for( const tripoint &pt : g->m.points_in_radius( src_loc, 2 ) ) {
            if( g->m.has_flag_furn( "FLAT_SURF", pt ) || g->m.has_flag( "FLAT_SURF", pt ) ||
                ( ( g->m.veh_at( pt ) && ( g->m.veh_at( pt )->vehicle().has_part( "KITCHEN" ) ||
                                           g->m.veh_at( pt )->vehicle().has_part( "FLAT_SURF" ) ) ) ) ) {
                has_table_nearby = true;
            }
        }
        bool b_rack_present = false;
        for( const tripoint &pt : g->m.points_in_radius( src_loc, 2 ) ) {
            if( g->m.has_flag_furn( "BUTCHER_EQ", pt ) ) {
                b_rack_present = true;
            }
        }
        if( !corpses.empty() ) {
            if( big_count > 0 && small_count == 0 ) {
                if( !has_table_nearby || !b_rack_present ) {
                    return activity_reason_info::fail( NO_ZONE );
                }
                if( p.has_quality( quality_id( "BUTCHER" ), 1 ) && ( p.has_quality( quality_id( "SAW_W" ) ) ||
                        p.has_quality( quality_id( "SAW_M" ) ) ) ) {
                    return activity_reason_info::ok( NEEDS_BIG_BUTCHERING );
                } else {
                    return activity_reason_info::fail( NEEDS_BIG_BUTCHERING );
                }
            }
            if( ( big_count > 0 && small_count > 0 ) || ( big_count == 0 ) ) {
                // there are small corpses here, so we can ignore any big corpses here for the moment.
                if( p.has_quality( quality_id( "BUTCHER" ), 1 ) ) {
                    return activity_reason_info::ok( NEEDS_BUTCHERING );
                } else {
                    return activity_reason_info::fail( NEEDS_BUTCHERING );
                }
            }
        }
        return activity_reason_info::fail( NO_ZONE );
    }
    if( act == activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ) ) {
        //are there even any logs there?
        for( auto &i : g->m.i_at( src_loc ) ) {
            if( i.typeId() == "log" ) {
                // do we have an axe?
                if( p.has_quality( quality_id( "AXE" ), 1 ) ) {
                    return activity_reason_info::fail( NEEDS_CHOPPING );
                } else {
                    return activity_reason_info::fail( NEEDS_CHOPPING );
                }
            }
        }
        return activity_reason_info::fail( NO_ZONE );
    }
    if( act == activity_id( "ACT_TIDY_UP" ) ) {
        if( mgr.has_near( z_loot_unsorted, g->m.getabs( src_loc ), 60 ) ) {
            return activity_reason_info::ok( CAN_DO_FETCH );
        }
        return activity_reason_info::fail( NO_ZONE );
    }
    if( act == activity_id( "ACT_MULTIPLE_CONSTRUCTION" ) ) {
        const std::vector<construction> &list_constructions = get_constructions();
        zones = mgr.get_zones( zone_type_id( "CONSTRUCTION_BLUEPRINT" ),
                               g->m.getabs( src_loc ) );
        const partial_con *part_con = g->m.partial_con_at( src_loc );
        cata::optional<size_t> part_con_idx;
        if( part_con ) {
            part_con_idx = part_con->id;
        }
        const map_stack stuff_there = g->m.i_at( src_loc );

        // PICKUP_RANGE -1 because we will be adjacent to the spot when arriving.
        const inventory pre_inv = p.crafting_inventory( src_loc, PICKUP_RANGE - 1 );
        for( const zone_data &zone : zones ) {
            const blueprint_options options = dynamic_cast<const blueprint_options &>( zone.get_options() );
            const int index = options.get_index();
            if( !stuff_there.empty() ) {
                return activity_reason_info::build( BLOCKING_TILE, false, static_cast<size_t>( index ) );
            }
            std::set<size_t> used_idx;
            const activity_reason_info act_info = find_base_construction( list_constructions, p, pre_inv,
                                                  src_loc, part_con_idx, static_cast<size_t>( index ), used_idx );
            return act_info;
        }
    } else if( act == activity_id( "ACT_MULTIPLE_FARM" ) ) {
        zones = mgr.get_zones( zone_type_id( "FARM_PLOT" ),
                               g->m.getabs( src_loc ) );
        for( const zone_data &zone : zones ) {
            if( g->m.has_flag_furn( "GROWTH_HARVEST", src_loc ) ) {
                // simple work, pulling up plants, nothing else required.
                return activity_reason_info::ok( NEEDS_HARVESTING );
            } else if( g->m.has_flag( "PLOWABLE", src_loc ) && !g->m.has_furn( src_loc ) ) {
                if( p.has_quality( quality_id( "DIG" ), 1 ) ) {
                    // we have a shovel/hoe already, great
                    return activity_reason_info::ok( NEEDS_TILLING );
                } else {
                    // we need a shovel/hoe
                    return activity_reason_info::fail( NEEDS_TILLING );
                }
            } else if( g->m.has_flag_ter_or_furn( "PLANTABLE", src_loc ) && warm_enough_to_plant( src_loc ) ) {
                if( g->m.has_items( src_loc ) ) {
                    return activity_reason_info::fail( BLOCKING_TILE );
                } else {
                    // do we have the required seed on our person?
                    const auto options = dynamic_cast<const plot_options &>( zone.get_options() );
                    const std::string seed = options.get_seed();
                    std::vector<item *> seed_inv = p.items_with( []( const item & itm ) {
                        return itm.is_seed();
                    } );
                    for( const auto elem : seed_inv ) {
                        if( elem->typeId() == itype_id( seed ) ) {
                            return activity_reason_info::ok( NEEDS_PLANTING );
                        }
                    }
                    // didnt find the seed, but maybe there are overlapping farm zones
                    // and another of the zones is for a seed that we have
                    // so loop again, and return false once all zones done.
                }

            } else {
                // cant plant, till or harvest
                return activity_reason_info::fail( ALREADY_DONE );
            }

        }
        // looped through all zones, and only got here if its plantable, but have no seeds.
        return activity_reason_info::fail( NEEDS_PLANTING );
    } else if( act == activity_id( "ACT_FETCH_REQUIRED" ) ) {
        // we check if its possible to get all the requirements for fetching at two other places.
        // 1. before we even assign the fetch activity and;
        // 2. when we form the src_set to loop through at the beginning of the fetch activity.
        return activity_reason_info::ok( CAN_DO_FETCH );
    }
    // Shouldnt get here because the zones were checked previously. if it does, set enum reason as "no zone"
    return activity_reason_info::fail( NO_ZONE );
}

static std::vector<std::tuple<tripoint, itype_id, int>> requirements_map( player &p )
{
    const requirement_data things_to_fetch = requirement_id( p.backlog.front().str_values[0] ).obj();
    const activity_id activity_to_restore = p.backlog.front().id();
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    requirement_id things_to_fetch_id = things_to_fetch.id();
    std::vector<std::vector<item_comp>> req_comps = things_to_fetch.get_components();
    std::vector<std::vector<tool_comp>> tool_comps = things_to_fetch.get_tools();
    std::vector<std::vector<quality_requirement>> quality_comps = things_to_fetch.get_qualities();
    zone_manager &mgr = zone_manager::get_manager();
    const bool pickup_task = p.backlog.front().id() == activity_id( "ACT_MULTIPLE_FARM" ) ||
                             p.backlog.front().id() == activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ) ||
                             p.backlog.front().id() == activity_id( "ACT_MULTIPLE_BUTCHER" ) ||
                             p.backlog.front().id() == activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ||
                             p.backlog.front().id() == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ||
                             p.backlog.front().id() == activity_id( "ACT_VEHICLE_REPAIR" ) ||
                             p.backlog.front().id() == activity_id( "ACT_MULTIPLE_FISH" );
    // where it is, what it is, how much of it, and how much in total is required of that item.
    std::vector<std::tuple<tripoint, itype_id, int>> requirement_map;
    std::vector<std::tuple<tripoint, itype_id, int>> final_map;
    std::vector<tripoint> loot_spots;
    std::vector<tripoint> already_there_spots;
    std::vector<tripoint> combined_spots;
    std::map<itype_id, int> total_map;
    for( const auto elem : g->m.points_in_radius( g->m.getlocal( p.backlog.front().placement ),
            PICKUP_RANGE - 1 ) ) {
        already_there_spots.push_back( elem );
        combined_spots.push_back( elem );
    }
    for( const tripoint elem : mgr.get_point_set_loot( g->m.getabs( p.pos() ), 60, p.is_npc() ) ) {
        // if there is a loot zone thats already near the work spot, we dont want it to be added twice.
        if( std::find( already_there_spots.begin(), already_there_spots.end(),
                       elem ) != already_there_spots.end() ) {
            // construction tasks dont need the loot spot *and* the already_there/cmbined spots both added.
            // but a farming task will need to go and fetch the tool no matter if its near the work spot.
            // wheras the construction will automaticlaly use whats nearby anyway.
            if( pickup_task ) {
                loot_spots.push_back( elem );
            } else {
                continue;
            }
        } else {
            loot_spots.push_back( elem );
            combined_spots.push_back( elem );
        }
    }
    // if the requirements arent available, then stop.
    if( !are_requirements_nearby( pickup_task ? loot_spots : combined_spots, things_to_fetch_id, p,
                                  activity_to_restore, pickup_task ) ) {
        return requirement_map;
    }
    // if the requirements are already near the work spot and its a construction/crafting task, then no need to fetch anything more.
    if( !pickup_task &&
        are_requirements_nearby( already_there_spots, things_to_fetch_id, p, activity_to_restore,
                                 false ) ) {
        return requirement_map;
    }
    // a vector of every item in every tile that matches any part of the requirements.
    // will be filtered for amounts/charges afterwards.
    for( tripoint point_elem : pickup_task ? loot_spots : combined_spots ) {
        std::map<itype_id, int> temp_map;
        for( auto &stack_elem : g->m.i_at( point_elem ) ) {
            for( std::vector<item_comp> &elem : req_comps ) {
                for( item_comp &comp_elem : elem ) {
                    if( comp_elem.type == stack_elem.typeId() ) {
                        // if its near the work site, we can remove a count from the requirements.
                        // if two "lines" of the requirement have the same component appearing again
                        // that is fine, we will choose which "line" to fulfill later, and the decrement will count towards that then.
                        if( !pickup_task &&
                            std::find( already_there_spots.begin(), already_there_spots.end(),
                                       point_elem ) != already_there_spots.end() ) {
                            comp_elem.count -= stack_elem.count();
                        }
                        temp_map[stack_elem.typeId()] += stack_elem.count();
                    }
                }
            }
            for( std::vector<tool_comp> &elem : tool_comps ) {
                for( tool_comp &comp_elem : elem ) {
                    if( comp_elem.type == stack_elem.typeId() ) {
                        if( !pickup_task &&
                            std::find( already_there_spots.begin(), already_there_spots.end(),
                                       point_elem ) != already_there_spots.end() ) {
                            comp_elem.count -= stack_elem.count();
                        }
                        if( comp_elem.by_charges() ) {
                            // we dont care if there are 10 welders with 5 charges each
                            // we only want the one welder that has the required charge.
                            if( stack_elem.ammo_remaining() >= comp_elem.count ) {
                                temp_map[stack_elem.typeId()] += stack_elem.ammo_remaining();
                            }
                        } else {
                            temp_map[stack_elem.typeId()] += stack_elem.count();
                        }
                    }
                }
            }

            for( std::vector<quality_requirement> &elem : quality_comps ) {
                for( quality_requirement &comp_elem : elem ) {
                    const quality_id tool_qual = comp_elem.type;
                    const int qual_level = comp_elem.level;
                    if( stack_elem.has_quality( tool_qual, qual_level ) ) {
                        if( !pickup_task &&
                            std::find( already_there_spots.begin(), already_there_spots.end(),
                                       point_elem ) != already_there_spots.end() ) {
                            comp_elem.count -= stack_elem.count();
                        }
                        temp_map[stack_elem.typeId()] += stack_elem.count();
                    }
                }
            }
        }
        for( auto map_elem : temp_map ) {
            total_map[map_elem.first] += map_elem.second;
            // if its a construction/crafting task, we can discount any items already near the work spot.
            // we dont need to fetch those, they will be used automatically in the construction.
            // a shovel for tilling, for example, however, needs to be picked up, no matter if its near the spot or not.
            if( !pickup_task ) {
                if( std::find( already_there_spots.begin(), already_there_spots.end(),
                               point_elem ) != already_there_spots.end() ) {
                    continue;
                }
            }
            requirement_map.push_back( std::make_tuple( point_elem, map_elem.first, map_elem.second ) );
        }
    }
    // Ok we now have a list of all the items that match the requirements, their points, and a quantity for each one.
    // we need to consolidate them, and winnow it down to the minimum required counts, instead of all matching.
    for( std::vector<item_comp> &elem : req_comps ) {
        bool line_found = false;
        for( item_comp &comp_elem : elem ) {
            if( line_found || comp_elem.count <= 0 ) {
                break;
            }
            int quantity_required = comp_elem.count;
            int item_quantity = 0;
            auto it = requirement_map.begin();
            int remainder = 0;
            while( it != requirement_map.end() ) {
                tripoint pos_here = std::get<0>( *it );
                itype_id item_here = std::get<1>( *it );
                int quantity_here = std::get<2>( *it );
                if( comp_elem.type == item_here ) {
                    item_quantity += quantity_here;
                }
                if( item_quantity >= quantity_required ) {
                    // it's just this spot that can fulfil the requirement on its own
                    final_map.push_back( std::make_tuple( pos_here, item_here, std::min<int>( quantity_here,
                                                          quantity_required ) ) );
                    if( quantity_here >= quantity_required ) {
                        line_found = true;
                        break;
                    } else {
                        remainder = quantity_required - quantity_here;
                    }
                    break;
                }
                it++;
            }
            if( line_found ) {
                while( true ) {
                    // go back over things
                    if( it == requirement_map.begin() ) {
                        break;
                    }
                    if( remainder <= 0 ) {
                        line_found = true;
                        break;
                    }
                    tripoint pos_here2 = std::get<0>( *it );
                    itype_id item_here2 = std::get<1>( *it );
                    int quantity_here2 = std::get<2>( *it );
                    if( comp_elem.type == item_here2 ) {
                        if( quantity_here2 >= remainder ) {
                            final_map.push_back( std::make_tuple( pos_here2, item_here2, remainder ) );
                            line_found = true;
                        } else {
                            final_map.push_back( std::make_tuple( pos_here2, item_here2, remainder ) );
                            remainder -= quantity_here2;
                        }
                    }
                    it--;
                }
            }
        }
    }
    for( std::vector<tool_comp> &elem : tool_comps ) {
        bool line_found = false;
        for( tool_comp &comp_elem : elem ) {
            if( line_found || comp_elem.count <= 0 ) {
                break;
            }
            int quantity_required = comp_elem.count;
            int item_quantity = 0;
            auto it = requirement_map.begin();
            int remainder = 0;
            while( it != requirement_map.end() ) {
                tripoint pos_here = std::get<0>( *it );
                itype_id item_here = std::get<1>( *it );
                int quantity_here = std::get<2>( *it );
                if( comp_elem.type == item_here ) {
                    item_quantity += quantity_here;
                }
                if( item_quantity >= quantity_required ) {
                    // it's just this spot that can fulfil the requirement on its own
                    final_map.push_back( std::make_tuple( pos_here, item_here, std::min<int>( quantity_here,
                                                          quantity_required ) ) );
                    if( quantity_here >= quantity_required ) {
                        line_found = true;
                        break;
                    } else {
                        remainder = quantity_required - quantity_here;
                    }
                    break;
                }
                it++;
            }
            if( line_found ) {
                while( true ) {
                    // go back over things
                    if( it == requirement_map.begin() ) {
                        break;
                    }
                    if( remainder <= 0 ) {
                        line_found = true;
                        break;
                    }
                    tripoint pos_here2 = std::get<0>( *it );
                    itype_id item_here2 = std::get<1>( *it );
                    int quantity_here2 = std::get<2>( *it );
                    if( comp_elem.type == item_here2 ) {
                        if( quantity_here2 >= remainder ) {
                            final_map.push_back( std::make_tuple( pos_here2, item_here2, remainder ) );
                            line_found = true;
                        } else {
                            final_map.push_back( std::make_tuple( pos_here2, item_here2, remainder ) );
                            remainder -= quantity_here2;
                        }
                    }
                    it--;
                }
            }
        }
    }
    for( std::vector<quality_requirement> &elem : quality_comps ) {
        bool line_found = false;
        for( quality_requirement &comp_elem : elem ) {
            if( line_found || comp_elem.count <= 0 ) {
                break;
            }
            const quality_id tool_qual = comp_elem.type;
            const int qual_level = comp_elem.level;
            for( auto it = requirement_map.begin(); it != requirement_map.end(); ) {
                tripoint pos_here = std::get<0>( *it );
                itype_id item_here = std::get<1>( *it );
                item test_item = item( item_here, 0 );
                if( test_item.has_quality( tool_qual, qual_level ) ) {
                    // it's just this spot that can fulfil the requirement on its own
                    final_map.push_back( std::make_tuple( pos_here, item_here, 1 ) );
                    line_found = true;
                    break;
                }
                it++;
            }
        }
    }
    for( const std::tuple<tripoint, itype_id, int> elem : final_map ) {
        add_msg( m_debug, "%s is fetching %s from x: %d y: %d ", p.disp_name(), std::get<1>( elem ),
                 std::get<0>( elem ).x, std::get<0>( elem ).y );
    }
    return final_map;
}

static bool plant_activity( player &p, const zone_data *zone, const tripoint src_loc )
{
    const std::string seed = dynamic_cast<const plot_options &>( zone->get_options() ).get_seed();
    std::vector<item *> seed_inv = p.items_with( [seed]( const item & itm ) {
        return itm.typeId() == itype_id( seed );
    } );
    // we dont have the required seed, even though we should at this point.
    // move onto the next tile, and if need be that will prompt a fetch seeds activity.
    if( seed_inv.empty() ) {
        return false;
    }
    iexamine::plant_seed( p, src_loc, itype_id( seed ) );
    return true;
}

static void construction_activity( player &p, const zone_data *zone, const tripoint src_loc,
                                   const activity_reason_info &act_info, const std::vector<construction> &list_constructions,
                                   activity_id activity_to_restore )
{
    const blueprint_options options = dynamic_cast<const blueprint_options &>( zone->get_options() );
    // the actual desired construction
    if( !act_info.con_idx ) {
        debugmsg( "no construction selected" );
        return;
    }
    const construction &built_chosen = list_constructions[*act_info.con_idx];
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
    p.backlog.push_front( activity_to_restore );
    p.assign_activity( activity_id( "ACT_BUILD" ) );
    p.activity.placement = g->m.getabs( src_loc );
}

static bool tidy_activity( player &p, const tripoint src_loc, activity_id activity_to_restore )
{
    auto &mgr = zone_manager::get_manager();
    tripoint loot_abspos = g->m.getabs( src_loc );
    tripoint loot_src_lot;
    if( mgr.has_near( z_loot_unsorted, loot_abspos, 60 ) ) {
        const auto &zone_src_set = mgr.get_near( zone_type_id( "LOOT_UNSORTED" ), loot_abspos, 60 );
        const auto &zone_src_sorted = get_sorted_tiles_by_distance( loot_abspos, zone_src_set );
        // Find the nearest unsorted zone to dump objects at
        for( auto &src_elem : zone_src_sorted ) {
            if( !g->m.can_put_items_ter_furn( g->m.getlocal( src_elem ) ) ) {
                continue;
            }
            loot_src_lot = g->m.getlocal( src_elem );
            break;
        }
    }
    if( loot_src_lot == tripoint_zero ) {
        return false;
    }
    auto items_there = g->m.i_at( src_loc );
    vehicle *dest_veh;
    int dest_part;
    if( const cata::optional<vpart_reference> vp = g->m.veh_at(
                loot_src_lot ).part_with_feature( "CARGO",
                        false ) ) {
        dest_veh = &vp->vehicle();
        dest_part = vp->part_index();
    } else {
        dest_veh = nullptr;
        dest_part = -1;
    }
    for( auto &it : items_there ) {
        if( it.has_var( "activity_var" ) && it.get_var( "activity_var", "" ) == p.name ) {
            move_item( p, it, it.count(), src_loc, loot_src_lot, dest_veh, dest_part,
                       activity_to_restore );
            break;
        }
    }
    // we are adjacent to an unsorted zone, we came here to just drop items we are carrying
    if( mgr.has( zone_type_id( z_loot_unsorted ), g->m.getabs( src_loc ) ) ) {
        for( auto inv_elem : p.inv_dump() ) {
            if( inv_elem->has_var( "activity_var" ) ) {
                inv_elem->erase_var( "activity_var" );
                p.drop( p.get_item_position( inv_elem ), src_loc );
            }
        }
    }
    return true;
}

static void fetch_activity( player &p, const tripoint src_loc, activity_id activity_to_restore )
{
    if( !g->m.can_put_items_ter_furn( g->m.getlocal( p.backlog.front().coords.back() ) ) ) {
        return;
    }
    const std::vector<std::tuple<tripoint, itype_id, int>> mental_map_2 = requirements_map( p );
    int pickup_count = 1;
    auto items_there = g->m.i_at( src_loc );
    vehicle *src_veh = nullptr;
    int src_part = 0;
    if( const cata::optional<vpart_reference> vp = g->m.veh_at( src_loc ).part_with_feature( "CARGO",
            false ) ) {
        src_veh = &vp->vehicle();
        src_part = vp->part_index();
    }
    std::string picked_up;
    const units::volume volume_allowed = p.volume_capacity() - p.volume_carried();
    const units::mass weight_allowed = p.weight_capacity() - p.weight_carried();
    // TODO : vehicle_stack and map_stack into one loop.
    if( src_veh ) {
        for( auto &veh_elem : src_veh->get_items( src_part ) ) {
            for( auto elem : mental_map_2 ) {
                if( std::get<0>( elem ) == src_loc && veh_elem.typeId() == std::get<1>( elem ) ) {
                    if( !p.backlog.empty() && p.backlog.front().id() == activity_id( "ACT_MULTIPLE_CONSTRUCTION" ) ) {
                        move_item( p, veh_elem, veh_elem.count_by_charges() ? std::get<2>( elem ) : 1, src_loc,
                                   g->m.getlocal( p.backlog.front().coords.back() ), src_veh, src_part, activity_to_restore );
                        return;
                    }
                }
            }
        }
    }
    for( auto it = items_there.begin(); it != items_there.end(); it++ ) {
        for( auto elem : mental_map_2 ) {
            if( std::get<0>( elem ) == src_loc && it->typeId() == std::get<1>( elem ) ) {
                // construction/crafting tasks want the requred item moved near the work spot.
                if( !p.backlog.empty() && p.backlog.front().id() == activity_id( "ACT_MULTIPLE_CONSTRUCTION" ) ) {
                    move_item( p, *it, it->count_by_charges() ? std::get<2>( elem ) : 1, src_loc,
                               g->m.getlocal( p.backlog.front().coords.back() ), src_veh, src_part, activity_to_restore );
                    return;
                    // other tasks want the tool picked up
                } else if( !p.backlog.empty() && ( p.backlog.front().id() == activity_id( "ACT_MULTIPLE_FARM" ) ||
                                                   p.backlog.front().id() == activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ) ||
                                                   p.backlog.front().id() == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ||
                                                   p.backlog.front().id() == activity_id( "ACT_VEHICLE_REPAIR" ) ||
                                                   p.backlog.front().id() == activity_id( "ACT_MULTIPLE_BUTCHER" ) ||
                                                   p.backlog.front().id() == activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ||
                                                   p.backlog.front().id() == activity_id( "ACT_MULTIPLE_FISH" ) ) ) {
                    if( it->volume() > volume_allowed || it->weight() > weight_allowed ) {
                        continue;
                    }
                    item leftovers = *it;

                    if( pickup_count != 1 && it->count_by_charges() ) {
                        // Reinserting leftovers happens after item removal to avoid stacking issues.
                        leftovers.charges = it->charges - pickup_count;
                        if( leftovers.charges > 0 ) {
                            it->charges = pickup_count;
                        }
                    } else {
                        leftovers.charges = 0;
                    }
                    it->set_var( "activity_var", p.name );
                    p.i_add( *it );
                    picked_up = it->tname();
                    items_there.erase( it );
                    // If we didn't pick up a whole stack, put the remainder back where it came from.
                    if( leftovers.charges > 0 ) {
                        g->m.add_item_or_charges( src_loc, leftovers );
                    }
                    if( p.is_npc() && !picked_up.empty() ) {
                        if( pickup_count == 1 ) {
                            add_msg( _( "%1$s picks up a %2$s." ), p.disp_name(), picked_up );
                        } else {
                            add_msg( _( "%s picks up several items." ), p.disp_name() );
                        }
                    }
                    return;
                }
            }
        }
    }
}

static bool butcher_corpse_activity( player &p, tripoint src_loc, do_activity_reason reason )
{
    map_stack items = g->m.i_at( src_loc );
    for( auto &elem : items ) {
        if( elem.is_corpse() && !elem.has_var( "activity_var" ) ) {
            const mtype corpse = *elem.get_mtype();
            if( corpse.size >= MS_MEDIUM && reason != NEEDS_BIG_BUTCHERING ) {
                continue;
            }
            elem.set_var( "activity_var", p.name );
            p.assign_activity( activity_id( "ACT_BUTCHER_FULL" ), 0, true );
            p.activity.targets.emplace_back( map_cursor( src_loc ), &elem );
            p.activity.placement = g->m.getabs( src_loc );
            return true;
        }
    }
    return false;
}

static item *best_quality_item( player &p, quality_id qual )
{
    std::vector<item *> qual_inv = p.items_with( [qual]( const item & itm ) {
        return itm.has_quality( qual );
    } );
    item *best_qual = random_entry( qual_inv );
    for( const auto elem : qual_inv ) {
        if( elem->get_quality( qual ) > best_qual->get_quality( qual ) ) {
            best_qual = elem;
        }
    }
    return best_qual;
}

static bool chop_plank_activity( player &p, tripoint src_loc )
{
    item *best_qual = best_quality_item( p, quality_id( "AXE" ) );
    if( !best_qual ) {
        return false;
    }
    p.consume_charges( *best_qual, best_qual->type->charges_to_use() );
    for( auto &i : g->m.i_at( src_loc ) ) {
        if( i.typeId() == "log" ) {
            g->m.i_rem( src_loc, &i );
            int moves = to_moves<int>( 20_minutes );
            p.add_msg_if_player( _( "You cut the log into planks." ) );
            p.assign_activity( activity_id( "ACT_CHOP_PLANKS" ), moves, -1 );
            p.activity.placement = g->m.getabs( src_loc );
            return true;
        }
    }
    return false;
}

void activity_on_turn_move_loot( player_activity &act, player &p )
{
    const int search_range = 60;

    enum activity_stage : int {
        INIT = 0,     //Initial stage
        THINK,        //Think about what to do first: choose destination
        DO,           //Do activity
    };

    int &stage = act.index;
    //Prepare activity stage
    if( stage < 0 ) {
        stage = INIT;
        act.values.push_back( 0 ); //num_processed
    }
    int &num_processed = act.values[ 0 ];

    const auto abspos = g->m.getabs( p.pos() );
    auto &mgr = zone_manager::get_manager();
    if( g->m.check_vehicle_zones( g->get_levz() ) ) {
        mgr.cache_vzones();
    }

    if( stage == INIT ) {
        act.coord_set = mgr.get_near( zone_type_id( "LOOT_UNSORTED" ), abspos, search_range );
        stage = THINK;
    }

    if( stage == THINK ) {
        num_processed = 0; //initialize num_processed
        const auto &src_set = act.coord_set;
        // sort source tiles by distance
        const auto &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );

        for( auto &src : src_sorted ) {
            act.placement = src;
            act.coord_set.erase( src );

            const auto &src_loc = g->m.getlocal( src );
            if( !g->m.inbounds( src_loc ) ) {
                if( !g->m.inbounds( p.pos() ) ) {
                    // p is implicitly an NPC that has been moved off the map, so reset the activity
                    // and unload them
                    p.cancel_activity();
                    p.assign_activity( activity_id( "ACT_MOVE_LOOT" ) );
                    p.set_moves( 0 );
                    g->reload_npcs();
                    return;
                }
                std::vector<tripoint> route;
                route = g->m.route( p.pos(), src_loc, p.get_pathfinding_settings(),
                                    p.get_path_avoid() );
                if( route.empty() ) {
                    // can't get there, can't do anything, skip it
                    continue;
                }
                stage = DO;
                p.set_destination( route, act );
                p.activity.set_to_null();
                return;
            }

            // skip tiles in IGNORE zone and tiles on fire
            // (to prevent taking out wood off the lit brazier)
            // and inaccessible furniture, like filled charcoal kiln
            if( mgr.has( zone_type_id( "LOOT_IGNORE" ), src ) ||
                g->m.get_field( src_loc, fd_fire ) != nullptr ||
                !g->m.can_put_items_ter_furn( src_loc ) ) {
                continue;
            }

            //nothing to sort?
            const cata::optional<vpart_reference> vp = g->m.veh_at( src_loc ).part_with_feature( "CARGO",
                    false );
            if( !vp && g->m.i_at( src_loc ).empty( ) ) {
                continue;
            }

            bool is_adjacent_or_closer = square_dist( p.pos(), src_loc ) <= 1;
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
                    continue;
                }

                // shorten the route to adjacent tile, if necessary
                if( !adjacent ) {
                    route.pop_back();
                }

                // set the destination and restart activity after player arrives there
                // we don't need to check for safe mode,
                // activity will be restarted only if
                // player arrives on destination tile
                stage = DO;
                p.set_destination( route, act );
                p.activity.set_to_null();
                return;
            }
            stage = DO;
            break;
        }
    }
    if( stage == DO ) {
        const tripoint &src = act.placement;
        const tripoint &src_loc = g->m.getlocal( src );

        bool is_adjacent_or_closer = square_dist( p.pos(), src_loc ) <= 1;
        // before we move any item, check if player is at or
        // adjacent to the loot source tile
        if( !is_adjacent_or_closer ) {
            stage = THINK;
            return;
        }

        // the boolean in this pair being true indicates the item is from a vehicle storage space
        auto items = std::vector<std::pair<item *, bool>>();
        vehicle *src_veh, *dest_veh;
        int src_part, dest_part;

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
        for( auto it = items.begin() + num_processed; it < items.end(); ++it ) {
            ++num_processed;
            item &thisitem = *it->first;

            // skip unpickable liquid
            if( thisitem.made_of_from_type( LIQUID ) ) {
                continue;
            }

            // Only if it's from a vehicle do we use the vehicle source location information.
            vehicle *this_veh = it->second ? src_veh : nullptr;
            const int this_part = it->second ? src_part : -1;

            const zone_type_id id = mgr.get_near_zone_type_for_item( thisitem, abspos, search_range );

            // checks whether the item is already on correct loot zone or not
            // if it is, we can skip such item, if not we move the item to correct pile
            // think empty bag on food pile, after you ate the content
            if( mgr.has( id, src ) ) {
                continue;
            }

            const std::unordered_set<tripoint> &dest_set = mgr.get_near( id, abspos, search_range, &thisitem );
            for( const tripoint &dest : dest_set ) {
                const tripoint &dest_loc = g->m.getlocal( dest );

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
                if( free_space >= thisitem.volume() ) {
                    move_item( p, thisitem, thisitem.count(), src_loc, dest_loc, this_veh, this_part );

                    // moved item away from source so decrement
                    if( num_processed > 0 ) {
                        --num_processed;
                    }
                    break;
                }
            }
            if( p.moves <= 0 ) {
                return;
            }
        }

        //this location is sorted
        stage = THINK;
        return;
    }

    // If we got here without restarting the activity, it means we're done
    add_msg( m_info, _( "%s sorted out every item possible." ), p.disp_name() );
    if( p.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &p );
        guy->revert_after_activity();
        guy->current_activity_id = activity_id::NULL_ID();
    }
    p.activity.set_to_null();
}

static int chop_moves( player &p, item *it )
{
    // quality of tool
    const int quality = it->get_quality( quality_id( "AXE" ) );

    // attribute; regular tools - based on STR, powered tools - based on DEX
    const int attr = it->has_flag( "POWERED" ) ? p.dex_cur : p.str_cur;

    int moves = to_moves<int>( time_duration::from_minutes( 60 - attr ) / std::pow( 2, quality - 1 ) );
    const int helpersize = p.get_num_crafting_helpers( 3 );
    moves = moves * ( 1 - ( helpersize / 10 ) );
    return moves;
}

static bool chop_tree_activity( player &p, const tripoint &src_loc )
{
    item *best_qual = best_quality_item( p, quality_id( "AXE" ) );
    if( !best_qual ) {
        return false;
    }
    int moves = chop_moves( p, best_qual );
    p.consume_charges( *best_qual, best_qual->type->charges_to_use() );
    const ter_id ter = g->m.ter( src_loc );
    if( g->m.has_flag( "TREE", src_loc ) ) {
        p.assign_activity( activity_id( "ACT_CHOP_TREE" ), moves, -1, p.get_item_position( best_qual ) );
        p.activity.placement = g->m.getabs( src_loc );
        return true;
    } else if( ter == t_trunk || ter == t_stump ) {
        p.assign_activity( activity_id( "ACT_CHOP_LOGS" ), moves, -1, p.get_item_position( best_qual ) );
        p.activity.placement = g->m.getabs( src_loc );
        return true;
    }
    return false;
}

static void check_npc_revert( player &p )
{
    if( p.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &p );
        if( guy ) {
            guy->revert_after_activity();
        }
    }
}

void generic_multi_activity_handler( player_activity &act, player &p )
{
    // First get the things that are activity-agnostic.
    zone_manager &mgr = zone_manager::get_manager();
    const tripoint abspos = g->m.getabs( p.pos() );
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    activity_id activity_to_restore = act.id();
    const tripoint localpos = p.pos();
    bool dark_capable = false;
    // the set of target work spots - potentally after we have fetched required tools.
    std::unordered_set<tripoint> src_set;
    // we may need a list of all constructions later.
    const std::vector<construction> &list_constructions = get_constructions();
    // Nuke the current activity, leaving the backlog alone
    p.activity = player_activity();
    // now we setup the target spots based on whch activity is occuring
    if( activity_to_restore == activity_id( "ACT_TIDY_UP" ) ) {
        dark_capable = true;
        tripoint unsorted_spot;
        for( const tripoint elem : g->m.points_in_radius( g->m.getlocal( abspos ), 60 ) ) {
            if( mgr.has( zone_type_id( z_loot_unsorted ), g->m.getabs( elem ) ) ) {
                // it already has a unsorted loot spot, and therefore dont need to go and pick up items there.
                if( unsorted_spot == tripoint_zero ) {
                    unsorted_spot = elem;
                }
                continue;
            }
            for( const auto &stack_elem : g->m.i_at( elem ) ) {
                if( stack_elem.has_var( "activity_var" ) && stack_elem.get_var( "activity_var", "" ) == p.name ) {
                    const furn_t &f = g->m.furn( elem ).obj();
                    if( !f.has_flag( "PLANT" ) ) {
                        src_set.insert( g->m.getabs( elem ) );
                        break;
                    }
                }
            }
        }
        if( src_set.empty() && unsorted_spot != tripoint_zero ) {
            for( auto inv_elem : p.inv_dump() ) {
                if( inv_elem->has_var( "activity_var" ) ) {
                    // we've gone to tidy up all the thngs lying around, now tidy up the things we picked up.
                    src_set.insert( g->m.getabs( unsorted_spot ) );
                    break;
                }
            }
        }
    }
    if( activity_to_restore == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ) {
        src_set = mgr.get_near( zone_type_id( "VEHICLE_DECONSTRUCT" ), abspos, 60 );
    }
    if( activity_to_restore == activity_id( "ACT_VEHICLE_REPAIR" ) ) {
        src_set = mgr.get_near( zone_type_id( "VEHICLE_REPAIR" ), abspos, 60 );
    }
    if( activity_to_restore == activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ) {
        src_set = mgr.get_near( zone_type_id( "CHOP_TREES" ), abspos, 60 );
    }
    // multiple construction will form a list of targets based on blueprint zones and unfinished constructions
    if( activity_to_restore == activity_id( "ACT_MULTIPLE_CONSTRUCTION" ) ) {
        src_set = mgr.get_near( zone_type_id( "CONSTRUCTION_BLUEPRINT" ), abspos, 60 );
        for( const tripoint &elem : g->m.points_in_radius( localpos, 40 ) ) {
            partial_con *pc = g->m.partial_con_at( elem );
            if( pc ) {
                src_set.insert( g->m.getabs( elem ) );
            }
        }
        // farming activies encompass tilling, planting, harvesting.
    }
    if( activity_to_restore == activity_id( "ACT_MULTIPLE_FARM" ) ) {
        src_set = mgr.get_near( zone_type_id( "FARM_PLOT" ), abspos, 60 );
        // fetch required will always be following on from a previous activity
    }
    if( activity_to_restore == activity_id( "ACT_FETCH_REQUIRED" ) ) {
        dark_capable = true;
        // get the right zones for the items in the requirements.
        // we previously checked if the items are nearby before we set the fetch task
        // but we will check again later, to be sure nothings changed.
        std::vector<std::tuple<tripoint, itype_id, int>> mental_map = requirements_map( p );
        for( auto elem : mental_map ) {
            tripoint elem_point = std::get<0>( elem );
            src_set.insert( g->m.getabs( elem_point ) );
        }
    }
    if( activity_to_restore == activity_id( "ACT_MULTIPLE_BUTCHER" ) ) {
        src_set = mgr.get_near( zone_type_id( "LOOT_CORPSE" ), abspos, 60 );
    }
    if( activity_to_restore == activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ) ) {
        // we want to chop logs so get loot wood spots
        src_set = mgr.get_near( zone_type_id( "LOOT_WOOD" ), abspos, 60 );
    }
    if( activity_to_restore == activity_id( "ACT_MULTIPLE_FISH" ) ) {
        src_set = mgr.get_near( zone_type_id( "FISHING_SPOT" ), abspos, 60 );
    }
    // prune the set to remove tiles that are never gonna work out.
    for( auto it2 = src_set.begin(); it2 != src_set.end(); ) {
        // remove dangerous tiles
        tripoint set_pt = g->m.getlocal( *it2 );
        if( g->m.dangerous_field_at( set_pt ) ) {
            it2 = src_set.erase( it2 );
            // remove tiles in darkness, if we arent lit-up ourselves
        } else if( !dark_capable && p.fine_detail_vision_mod( set_pt ) > 4.0 ) {
            it2 = src_set.erase( it2 );
        } else if( activity_to_restore == activity_id( "ACT_MULTIPLE_FISH" ) ) {
            const ter_id terrain_id = g->m.ter( set_pt );
            if( !terrain_id.obj().has_flag( TFLAG_DEEP_WATER ) ) {
                it2 = src_set.erase( it2 );
            } else {
                ++it2;
            }
        } else {
            ++it2;
        }
    }
    // now we have our final set of points
    std::vector<tripoint> src_sorted = get_sorted_tiles_by_distance( abspos, src_set );
    // now loop through the work-spot tiles and judge whether its worth travelling to it yet
    // or if we need to fetch something first.
    for( const tripoint &src : src_sorted ) {
        const tripoint &src_loc = g->m.getlocal( src );
        if( !g->m.inbounds( src_loc ) ) {
            if( !g->m.inbounds( p.pos() ) ) {
                // p is implicitly an NPC that has been moved off the map, so reset the activity
                // and unload them
                p.assign_activity( activity_to_restore );
                p.set_moves( 0 );
                g->reload_npcs();
                return;
            }
            const std::vector<tripoint> route = route_adjacent( p, src_loc );
            if( route.empty() ) {
                // can't get there, can't do anything, skip it
                continue;
            }
            p.set_moves( 0 );
            p.set_destination( route, player_activity( activity_to_restore ) );
            return;
        }
        activity_reason_info act_info = can_do_activity_there( activity_to_restore, p,
                                        src_loc );
        const bool &can_do_it = act_info.can_do;
        const do_activity_reason &reason = act_info.reason;
        const zone_data *zone = mgr.get_zone_at( src );
        const bool needs_to_be_in_zone = activity_to_restore == activity_id( "ACT_FETCH_REQUIRED" ) ||
                                         activity_to_restore == activity_id( "ACT_MULTIPLE_FARM" ) ||
                                         activity_to_restore == activity_id( "ACT_MULTIPLE_BUTCHER" ) ||
                                         activity_to_restore == activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ) ||
                                         activity_to_restore == activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ||
                                         activity_to_restore == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ||
                                         activity_to_restore == activity_id( "ACT_VEHICLE_REPAIR" ) ||
                                         activity_to_restore == activity_id( "ACT_MULTIPLE_FISH" ) ||
                                         ( activity_to_restore == activity_id( "ACT_MULTIPLE_CONSTRUCTION" ) &&
                                           !g->m.partial_con_at( src_loc ) );
        // some activities require the target tile to be part of a zone.
        // tidy up activity dosnt - it wants things that may not be in a zone already - things that may have been left lying around.
        if( needs_to_be_in_zone && !zone ) {
            continue;
        }
        if( ( !can_do_it ) && ( reason == DONT_HAVE_SKILL || reason == NO_ZONE || reason == ALREADY_DONE ||
                                reason == BLOCKING_TILE || reason == UNKNOWN_ACTIVITY ) ) {
            // we can discount this tile, the work can't be done.
            if( reason == DONT_HAVE_SKILL ) {
                p.add_msg_if_player( m_info, _( "You don't have the skill for this task." ) );
            } else if( reason == BLOCKING_TILE ) {
                p.add_msg_if_player( m_info, _( "There is something blocking the location for this task." ) );
            }
            continue;
        } else if( ( !can_do_it ) && ( reason == NO_COMPONENTS || reason == NEEDS_PLANTING ||
                                       reason == NEEDS_TILLING || reason == NEEDS_CHOPPING || reason == NEEDS_BUTCHERING ||
                                       reason == NEEDS_BIG_BUTCHERING || reason == NEEDS_VEH_DECONST || reason == NEEDS_VEH_REPAIR ||
                                       reason == NEEDS_TREE_CHOPPING ||
                                       reason == NEEDS_FISHING ) ) {
            // we can do it, but we need to fetch some stuff first
            // before we set the task to fetch components - is it even worth it? are the components anywhere?
            requirement_id what_we_need;
            std::vector<tripoint> loot_zone_spots;
            std::vector<tripoint> combined_spots;
            for( const tripoint elem : mgr.get_point_set_loot( abspos, 60, p.is_npc() ) ) {
                loot_zone_spots.push_back( elem );
                combined_spots.push_back( elem );
            }
            for( const tripoint elem : g->m.points_in_radius( src_loc, PICKUP_RANGE - 1 ) ) {
                combined_spots.push_back( elem );
            }
            if( ( reason == NO_COMPONENTS || reason == NO_COMPONENTS_PREREQ ||
                  reason == NO_COMPONENTS_PREREQ_2 ) &&
                activity_to_restore == activity_id( "ACT_MULTIPLE_CONSTRUCTION" ) ) {
                if( !act_info.con_idx ) {
                    debugmsg( "no construction selected" );
                    continue;
                }
                // its a construction and we need the components.
                const construction &built_chosen = list_constructions[ *act_info.con_idx ];
                what_we_need = built_chosen.requirements;
            } else if( reason == NEEDS_VEH_DECONST || reason == NEEDS_VEH_REPAIR ) {
                vehicle *veh = veh_pointer_or_null( g->m.veh_at( src_loc ) );
                // we already checked this in can_do_activity() but check again just incase.
                if( !veh ) {
                    p.activity_vehicle_part_index = -1;
                    continue;
                }
                const vpart_info &vpinfo = veh->part_info( p.activity_vehicle_part_index );
                requirement_data reqs;
                if( reason == NEEDS_VEH_DECONST ) {
                    reqs = vpinfo.removal_requirements();
                } else if( reason == NEEDS_VEH_REPAIR ) {
                    reqs = vpinfo.repair_requirements();
                }
                const std::string ran_str = random_string( 10 );
                const requirement_id req_id( ran_str );
                requirement_data::save_requirement( reqs, req_id );
                what_we_need = req_id;
            } else if( reason == NEEDS_TILLING || reason == NEEDS_PLANTING || reason == NEEDS_CHOPPING ||
                       reason == NEEDS_BUTCHERING || reason == NEEDS_BIG_BUTCHERING || reason == NEEDS_TREE_CHOPPING ||
                       reason == NEEDS_FISHING ) {
                std::vector<std::vector<item_comp>> requirement_comp_vector;
                std::vector<std::vector<quality_requirement>> quality_comp_vector;
                std::vector<std::vector<tool_comp>> tool_comp_vector;
                if( reason == NEEDS_TILLING ) {
                    quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( quality_id( "DIG" ), 1, 1 ) } );
                } else if( reason == NEEDS_CHOPPING || reason == NEEDS_TREE_CHOPPING ) {
                    quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( quality_id( "AXE" ), 1, 1 ) } );
                } else if( reason == NEEDS_PLANTING ) {
                    requirement_comp_vector.push_back( std::vector<item_comp> { item_comp( itype_id( dynamic_cast<const plot_options &>
                                                       ( zone->get_options() ).get_seed() ), 1 )
                                                                              } );
                } else if( reason == NEEDS_BUTCHERING || reason == NEEDS_BIG_BUTCHERING ) {
                    quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( quality_id( "BUTCHER" ), 1, 1 ) } );
                    if( reason == NEEDS_BIG_BUTCHERING ) {
                        quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( quality_id( "SAW_M" ), 1, 1 ), quality_requirement( quality_id( "SAW_W" ), 1, 1 ) } );
                    }

                } else if( reason == NEEDS_FISHING ) {
                    quality_comp_vector.push_back( std::vector<quality_requirement> {quality_requirement( quality_id( "FISHING" ), 1, 1 )} );
                }
                // ok, we need a shovel/hoe/axe/etc
                // this is an activity that only requires this one tool, so we will fetch and wield it.
                requirement_data reqs_data = requirement_data( tool_comp_vector, quality_comp_vector,
                                             requirement_comp_vector );
                const std::string ran_str = random_string( 10 );
                const requirement_id req_id( ran_str );
                requirement_data::save_requirement( reqs_data, req_id );
                what_we_need = req_id;
            }
            bool tool_pickup = reason == NEEDS_TILLING || reason == NEEDS_PLANTING ||
                               reason == NEEDS_CHOPPING || reason == NEEDS_BUTCHERING || reason == NEEDS_BIG_BUTCHERING ||
                               reason == NEEDS_TREE_CHOPPING || reason == NEEDS_VEH_DECONST || reason == NEEDS_VEH_REPAIR;
            // is it even worth fetching anything if there isnt enough nearby?
            if( !are_requirements_nearby( tool_pickup ? loot_zone_spots : combined_spots, what_we_need, p,
                                          activity_to_restore, tool_pickup ) ) {
                p.add_msg_if_player( m_info, _( "The required items are not available to complete this task." ) );
                if( reason == NEEDS_VEH_DECONST || reason == NEEDS_VEH_REPAIR ) {
                    p.activity_vehicle_part_index = -1;
                }
                continue;
            } else {
                p.backlog.push_front( activity_to_restore );
                p.assign_activity( activity_id( "ACT_FETCH_REQUIRED" ) );
                p.backlog.front().str_values.push_back( what_we_need.str() );
                p.backlog.front().values.push_back( reason );
                // come back here after succesfully fetching your stuff
                std::vector<tripoint> candidates;
                if( p.backlog.front().coords.empty() ) {
                    std::vector<tripoint> local_src_set;
                    for( const auto elem : src_set ) {
                        local_src_set.push_back( g->m.getlocal( elem ) );
                    }
                    std::vector<tripoint> candidates;
                    for( const auto point_elem : g->m.points_in_radius( src_loc, PICKUP_RANGE - 1 ) ) {
                        // we dont want to place the components where they could interfere with our ( or someone elses ) construction spots
                        if( ( std::find( local_src_set.begin(), local_src_set.end(),
                                         point_elem ) != local_src_set.end() ) || !g->m.can_put_items_ter_furn( point_elem ) ) {
                            continue;
                        }
                        candidates.push_back( point_elem );
                    }
                    if( candidates.empty() ) {
                        p.activity = player_activity();
                        p.backlog.clear();
                        check_npc_revert( p );
                        return;
                    }
                    p.backlog.front().coords.push_back( g->m.getabs( candidates[std::max( 0,
                                                                  static_cast<int>( candidates.size() / 2 ) )] ) );
                }
                p.backlog.front().placement = src;

                return;
            }
        }
        if( square_dist( p.pos(), src_loc ) > 1 ) { // not adjacent
            std::vector<tripoint> route = route_adjacent( p, src_loc );

            // check if we found path to source / adjacent tile
            if( route.empty() ) {
                check_npc_revert( p );
                return;
            }
            if( p.moves <= 0 ) {
                // Restart activity and break from cycle.
                p.assign_activity( activity_to_restore );
                return;
            }
            // set the destination and restart activity after player arrives there
            // we don't need to check for safe mode,
            // activity will be restarted only if
            // player arrives on destination tile
            p.set_destination( route, player_activity( activity_to_restore ) );
            return;
        }
        // something needs to be done, now we are there.
        // it was here earlier, in the space of one turn, maybe it got harvested by someone else.
        if( reason == NEEDS_HARVESTING && g->m.has_flag_furn( "GROWTH_HARVEST", src_loc ) ) {
            iexamine::harvest_plant( p, src_loc, true );
        } else if( reason == NEEDS_TILLING && g->m.has_flag( "PLOWABLE", src_loc ) &&
                   p.has_quality( quality_id( "DIG" ), 1 ) && !g->m.has_furn( src_loc ) ) {
            p.assign_activity( activity_id( "ACT_CHURN" ), 18000, -1 );
            p.backlog.push_front( activity_to_restore );
            p.activity.placement = src;
            return;
        } else if( reason == NEEDS_PLANTING && g->m.has_flag_ter_or_furn( "PLANTABLE", src_loc ) ) {
            if( !plant_activity( p, zone, src_loc ) ) {
                continue;
            }
        } else if( reason == NEEDS_CHOPPING && p.has_quality( quality_id( "AXE" ), 1 ) ) {
            if( chop_plank_activity( p, src_loc ) ) {
                p.backlog.push_front( activity_to_restore );
                return;
            }
        } else if( reason == NEEDS_BUTCHERING || reason == NEEDS_BIG_BUTCHERING ) {
            p.backlog.push_front( activity_to_restore );
            if( butcher_corpse_activity( p, src_loc, reason ) ) {
                return;
            }
        } else if( reason == CAN_DO_CONSTRUCTION || reason == CAN_DO_PREREQ ) {
            if( g->m.partial_con_at( src_loc ) ) {
                p.backlog.push_front( activity_to_restore );
                p.assign_activity( activity_id( "ACT_BUILD" ) );
                p.activity.placement = src;
                return;
            }
            construction_activity( p, zone, src_loc, act_info, list_constructions, activity_to_restore );
            return;
        } else if( reason == CAN_DO_FETCH && activity_to_restore == activity_id( "ACT_TIDY_UP" ) ) {
            if( !tidy_activity( p, src_loc, activity_to_restore ) ) {
                return;
            }
        } else if( reason == CAN_DO_FETCH && activity_to_restore == activity_id( "ACT_FETCH_REQUIRED" ) ) {
            fetch_activity( p, src_loc, activity_to_restore );
            return;
        } else if( reason == NEEDS_TREE_CHOPPING && p.has_quality( quality_id( "AXE" ), 1 ) ) {
            p.backlog.push_front( activity_to_restore );
            if( chop_tree_activity( p, src_loc ) ) {
                return;
            }
        } else if( reason == NEEDS_FISHING && p.has_quality( quality_id( "FISHING" ), 1 ) ) {
            p.backlog.push_front( activity_to_restore );
            // we dont want to keep repeating the fishing activity, just piggybacking on this functions structure to find requirements.
            p.activity = player_activity();
            item *best_rod = best_quality_item( p, quality_id( "FISHING" ) );
            p.assign_activity( activity_id( "ACT_FISH" ), to_moves<int>( 5_hours ), 0,
                               p.get_item_position( best_rod ), best_rod->tname() );
            p.activity.coord_set = g->get_fishable_locations( 60, src_loc );
            return;
        } else if( reason == NEEDS_VEH_DECONST ) {
            p.backlog.push_front( activity_to_restore );
            vehicle_activity( p, src_loc, p.activity_vehicle_part_index, 'o' );
            p.activity_vehicle_part_index = -1;
            return;
        } else if( reason == NEEDS_VEH_REPAIR ) {
            p.backlog.push_front( activity_to_restore );
            vehicle_activity( p, src_loc, p.activity_vehicle_part_index, 'r' );
            p.activity_vehicle_part_index = -1;
            return;
        }
    }
    if( p.moves <= 0 ) {
        // Restart activity and break from cycle.
        p.assign_activity( activity_to_restore );
        p.activity_vehicle_part_index = -1;
        return;
    }
    // if we got here, we need to revert otherwise NPC will be stuck in AI Limbo and have a head explosion.
    if( p.backlog.empty() || src_set.empty() ) {
        check_npc_revert( p );
        // tidy up leftover moved parts and tools left lying near the work spots.
        if( activity_to_restore == activity_id( "ACT_MULTIPLE_FARM" ) ||
            activity_to_restore == activity_id( "ACT_MULTIPLE_CONSTRUCTION" ) ||
            activity_to_restore == activity_id( "ACT_MULTIPLE_CHOP_PLANKS" ) ||
            activity_to_restore == activity_id( "ACT_MULTIPLE_BUTCHER" ) ||
            activity_to_restore == activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) ||
            activity_to_restore == activity_id( "ACT_VEHICLE_REPAIR" ) ||
            activity_to_restore == activity_id( "ACT_MULTIPLE_CHOP_TREES" ) ) {
            p.assign_activity( activity_id( "ACT_TIDY_UP" ) );
            if( p.is_npc() ) {
                npc *guy = dynamic_cast<npc *>( &p );
                if( guy ) {
                    guy->set_attitude( NPCATT_ACTIVITY );
                    guy->set_mission( NPC_MISSION_ACTIVITY );
                }
            }
        }
    }
    p.activity_vehicle_part_index = -1;
}

static cata::optional<tripoint> find_best_fire( const std::vector<tripoint> &from,
        const tripoint &center )
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

static inline bool has_clear_path_to_pickup_items( const tripoint &from, const tripoint &to )
{
    return g->m.has_items( to ) &&
           g->m.accessible_items( to ) &&
           g->m.clear_path( from, to, PICKUP_RANGE, 1, 100 );
}

static cata::optional<tripoint> find_refuel_spot_zone( const tripoint &center )
{
    const zone_manager &mgr = zone_manager::get_manager();
    const tripoint center_abs = g->m.getabs( center );

    const std::unordered_set<tripoint> &tiles_abs_unordered =
        mgr.get_near( zone_source_firewood, center_abs, PICKUP_RANGE );
    const std::vector<tripoint> &tiles_abs =
        get_sorted_tiles_by_distance( center_abs, tiles_abs_unordered );

    for( const tripoint &tile_abs : tiles_abs ) {
        const tripoint tile = g->m.getlocal( tile_abs );
        if( has_clear_path_to_pickup_items( center, tile ) ) {
            return tile;
        }
    }

    return {};
}

static cata::optional<tripoint> find_refuel_spot_trap( const std::vector<tripoint> &from,
        const tripoint &center )
{
    const auto tile = std::find_if( from.begin(), from.end(), [center]( const tripoint & pt ) {
        // Hacky - firewood spot is a trap and it's ID-checked
        return g->m.tr_at( pt ).id == tr_firewood_source
               && has_clear_path_to_pickup_items( center, pt );
    } );

    if( tile != from.end() ) {
        return *tile;
    }

    return {};
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

    cata::optional<tripoint> refuel_spot = find_refuel_spot_zone( pos );
    if( !refuel_spot ) {
        refuel_spot = find_refuel_spot_trap( adjacent, pos );
        if( !refuel_spot ) {
            return;
        }
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
