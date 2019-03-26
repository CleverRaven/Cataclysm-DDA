#include "activity_handlers.h" // IWYU pragma: associated

#include <algorithm>
#include <cassert>
#include <list>
#include <vector>

#include "action.h"
#include "clzones.h"
#include "creature.h"
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
#include "vpart_reference.h"

void cancel_aim_processing();

const efftype_id effect_controlled( "controlled" );
const efftype_id effect_pet( "pet" );

const trap_str_id tr_firewood_source( "tr_firewood_source" );

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

// @todo: Deliberately unified with multidrop. Unify further.
typedef std::list<std::pair<int, int>> drop_indexes;

bool same_type( const std::list<item> &items )
{
    return std::all_of( items.begin(), items.end(), [ &items ]( const item & it ) {
        return it.type == items.begin()->type;
    } );
}

void put_into_vehicle( Character &c, item_drop_reason reason, const std::list<item> &items,
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

void stash_on_pet( const std::list<item> &items, monster &pet )
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
    }
}

void drop_on_map( const Character &c, item_drop_reason reason, const std::list<item> &items,
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
    for( const auto &it : items ) {
        g->m.add_item_or_charges( where, it );
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

drop_indexes convert_to_indexes( const player_activity &act )
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

drop_indexes convert_to_indexes( const player &p, const std::list<act_item> &items )
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

std::list<act_item> convert_to_items( const player &p, const drop_indexes &drop,
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
                res.emplace_back( &it, qty, 100 ); // @todo: Use a calculated cost
            }
        } else {
            res.emplace_back( &p.i_at( pos ), count, ( pos == -1 ) ? 0 : 100 ); // @todo: Use a calculated cost
        }
    }

    return res;
}

// Prepares items for dropping by reordering them so that the drop
// cost is minimal and "dependent" items get taken off first.
// Implements the "backpack" logic.
std::list<act_item> reorder_for_dropping( const player &p, const drop_indexes &drop )
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
                worn.emplace_front( dit, dit->count(), 100 ); // @todo: Use a calculated cost
            }
        }
    }
    // Sort worn items by storage in descending order, but dependent items always go first.
    worn.sort( []( const act_item & first, const act_item & second ) {
        return first.it->is_worn_only_with( *second.it )
               || ( ( first.it->get_storage() > second.it->get_storage() )
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

//@todo: Display costs in the multidrop menu
void debug_drop_list( const std::list<act_item> &list )
{
    if( !debug_mode ) {
        return;
    }

    std::string res( "Items ordered to drop:\n" );
    for( const auto &ait : list ) {
        res += string_format( "Drop %d %s for %d moves\n",
                              ait.count, ait.it->display_name( ait.count ).c_str(), ait.consumed_moves );
    }
    popup( res, PF_GET_KEY );
}

std::list<item> obtain_activity_items( player_activity &act, player &p )
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

void activity_on_turn_wear()
{
    // Wear activity has source square, bools indicating source type,
    // indices of items on map or position of items in inventory, and quantities of same.
    tripoint source = g->u.activity.placement + g->u.pos();
    bool from_inventory = g->u.activity.values[0];
    bool from_vehicle = g->u.activity.values[1];

    // load vehicle information if requested
    int s_cargo = -1;
    vehicle *s_veh;
    s_veh = nullptr;

    if( from_vehicle ) {
        const cata::optional<vpart_reference> vp = g->m.veh_at( source ).part_with_feature( "CARGO",
                false );
        assert( vp );
        s_veh = &vp->vehicle();
        s_cargo = vp->part_index();
        assert( s_cargo >= 0 );
    }

    std::list<int> indices;
    std::list<int> quantities;

    if( g->u.activity.values.size() % 2 != 0 ) {
        debugmsg( "ACT_WEAR started with uneven number of values." );
        g->u.cancel_activity();
        return;
    } else {
        // Note i = 2, skipping first 2 elements.
        for( size_t i = 2; i < g->u.activity.values.size(); i += 2 ) {
            indices.push_back( g->u.activity.values[i] );
            quantities.push_back( g->u.activity.values[ i + 1 ] );
        }
    }
    g->u.cancel_activity();

    while( g->u.moves > 0 && !indices.empty() ) {
        int index = indices.back();
        int quantity = quantities.back();
        indices.pop_back();
        quantities.pop_back();

        if( from_inventory ) {
            if( g->u.wear( index ) ) {
                if( --quantity > 0 ) {
                    indices.push_back( index );
                    quantities.push_back( quantity );
                }
            }
        } else {
            item *temp_item = from_vehicle ? g->m.item_from( s_veh, s_cargo, index ) : g->m.item_from( source,
                              index );
            if( temp_item == nullptr ) {
                continue; // No such item.
            }
            // On successful wear remove from map or vehicle.
            if( g->u.wear_item( *temp_item ) ) {
                if( from_vehicle ) {
                    s_veh->remove_item( s_cargo, index );
                } else {
                    g->m.i_rem( source, index );
                }
            }
        }
    }
    // If there are items left, we ran out of moves, so make a new activity with the remainder.
    if( !indices.empty() ) {
        g->u.assign_activity( activity_id( "ACT_WEAR" ) );
        g->u.activity.placement = source - g->u.pos();
        g->u.activity.values.push_back( from_inventory );
        g->u.activity.values.push_back( from_vehicle );
        while( !indices.empty() ) {
            g->u.activity.values.push_back( indices.front() );
            indices.pop_front();
            g->u.activity.values.push_back( quantities.front() );
            quantities.pop_front();
        }
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

    auto is_liquid = []( const item & it ) {
        return it.made_of( LIQUID ) || it.contents_made_of( LIQUID );
    };
    const inventory &crafting_inv = p->crafting_inventory();
    if( !crafting_inv.has_charges( "water", required.water, is_liquid ) &&
        !crafting_inv.has_charges( "water_clean", required.water, is_liquid ) ) {
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
    p->consume_items( comps, 1, is_crafting_component, is_liquid );

    std::vector<item_comp> comps1;
    comps1.push_back( item_comp( "soap", required.cleanser ) );
    comps1.push_back( item_comp( "detergent", required.cleanser ) );
    p->consume_items( comps1 );

    p->add_msg_if_player( m_good, _( "You washed your clothing." ) );

    // Make sure newly washed components show up as available if player attempts to craft immediately
    p->invalidate_crafting_inventory();

    act->set_to_null();
}

void activity_handlers::stash_do_turn( player_activity *act, player *p )
{
    const tripoint pos = act->placement + p->pos();

    monster *pet = g->critter_at<monster>( pos );
    if( pet != nullptr && pet->has_effect( effect_pet ) ) {
        stash_on_pet( obtain_activity_items( *act, *p ), *pet );
    } else {
        p->add_msg_if_player( _( "The pet has moved somewhere else." ) );
        p->cancel_activity();
    }
}

void activity_on_turn_pickup()
{
    // Pickup activity has source square, bool indicating source type,
    // indices of items on map, and quantities of same.
    bool from_vehicle = g->u.activity.values.front();
    tripoint pickup_target = g->u.activity.placement;
    tripoint true_target = g->u.pos();
    true_target += pickup_target;
    // Auto_resume implies autopickup.
    bool autopickup = g->u.activity.auto_resume;
    std::list<int> indices;
    std::list<int> quantities;
    auto map_stack = g->m.i_at( true_target );

    if( !from_vehicle && map_stack.empty() ) {
        g->u.cancel_activity();
        return;
    }
    // Note i = 1, skipping first element.
    for( size_t i = 1; i < g->u.activity.values.size(); i += 2 ) {
        indices.push_back( g->u.activity.values[i] );
        quantities.push_back( g->u.activity.values[ i + 1 ] );
    }
    g->u.cancel_activity();

    bool keep_going = Pickup::do_pickup( pickup_target, from_vehicle, indices, quantities, autopickup );

    // If there are items left, we ran out of moves, so make a new activity with the remainder.
    if( keep_going && !indices.empty() ) {
        g->u.assign_activity( activity_id( "ACT_PICKUP" ) );
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

    // @todo: Move this to advanced inventory instead of hacking it in here
    if( !keep_going ) {
        cancel_aim_processing();
    }
}

// I'd love to have this not duplicate so much code from Pickup::pick_one_up(),
// but I don't see a clean way to do that.
static void move_items( const tripoint &src, bool from_vehicle,
                        const tripoint &dest, bool to_vehicle,
                        std::list<int> &indices, std::list<int> &quantities )
{
    tripoint source = src + g->u.pos();
    tripoint destination = dest + g->u.pos();

    int s_cargo = -1;
    int d_cargo = -1;
    vehicle *s_veh, *d_veh;
    s_veh = d_veh = nullptr;

    // load vehicle information if requested
    if( from_vehicle ) {
        const cata::optional<vpart_reference> vp = g->m.veh_at( source ).part_with_feature( "CARGO",
                false );
        assert( vp );
        s_veh = &vp->vehicle();
        s_cargo = vp->part_index();
        assert( s_cargo >= 0 );
    }
    if( to_vehicle ) {
        const cata::optional<vpart_reference> vp = g->m.veh_at( destination ).part_with_feature( "CARGO",
                false );
        assert( vp );
        d_veh = &vp->vehicle();
        d_cargo = vp->part_index();
        assert( d_cargo >= 0 );
    }

    while( g->u.moves > 0 && !indices.empty() ) {
        int index = indices.back();
        int quantity = quantities.back();
        indices.pop_back();
        quantities.pop_back();

        item *temp_item = from_vehicle ? g->m.item_from( s_veh, s_cargo, index ) : g->m.item_from( source,
                          index );

        if( temp_item == nullptr ) {
            continue; // No such item.
        }

        item leftovers = *temp_item;

        if( quantity != 0 && temp_item->count_by_charges() ) {
            // Reinserting leftovers happens after item removal to avoid stacking issues.
            leftovers.charges = temp_item->charges - quantity;
            if( leftovers.charges > 0 ) {
                temp_item->charges = quantity;
            }
        } else {
            leftovers.charges = 0;
        }

        // Check that we can pick it up.
        if( !temp_item->made_of_from_type( LIQUID ) ) {
            // This is for hauling across zlevels, remove when going up and down stairs is no longer teleportation
            int distance = src.z == dest.z ? std::max( rl_dist( src, dest ), 1 ) : 1;
            g->u.mod_moves( -Pickup::cost_to_move_item( g->u, *temp_item ) * distance );
            if( to_vehicle ) {
                put_into_vehicle_or_drop( g->u, item_drop_reason::deliberate, { *temp_item }, destination );
            } else {
                drop_on_map( g->u, item_drop_reason::deliberate, { *temp_item }, destination );
            }
            // Remove from map or vehicle.
            if( from_vehicle ) {
                s_veh->remove_item( s_cargo, index );
            } else {
                g->m.i_rem( source, index );
            }
        }

        // If we didn't pick up a whole stack, put the remainder back where it came from.
        if( leftovers.charges > 0 ) {
            bool to_map = !from_vehicle;
            if( !to_map ) {
                to_map = !s_veh->add_item( s_cargo, leftovers );
            }
            if( to_map ) {
                g->m.add_item_or_charges( source, leftovers );
            }
        }
    }
}

/*      values explanation
 *      0: items from vehicle?
 *      1: items to a vehicle?
 *      2: index <-+
 *      3: amount  |
 *      n: ^-------+
 */
void activity_on_turn_move_items()
{
    // Drop activity if we don't know destination coordinates.
    if( g->u.activity.coords.empty() ) {
        g->u.activity = player_activity();
        return;
    }

    // Move activity has source square, target square,
    // indices of items on map, and quantities of same.
    const tripoint destination = g->u.activity.coords[0];
    const tripoint source = g->u.activity.placement;
    bool from_vehicle = g->u.activity.values[0];
    bool to_vehicle = g->u.activity.values[1];
    std::list<int> indices;
    std::list<int> quantities;

    // Note i = 4, skipping first few elements.
    for( size_t i = 2; i < g->u.activity.values.size(); i += 2 ) {
        indices.push_back( g->u.activity.values[i] );
        quantities.push_back( g->u.activity.values[i + 1] );
    }
    // Nuke the current activity, leaving the backlog alone.
    g->u.activity = player_activity();

    // *puts on 3d glasses from 90s cereal box*
    move_items( source, from_vehicle, destination, to_vehicle, indices, quantities );

    if( !indices.empty() ) {
        g->u.assign_activity( activity_id( "ACT_MOVE_ITEMS" ) );
        g->u.activity.placement = source;
        g->u.activity.coords.push_back( destination );
        g->u.activity.values.push_back( from_vehicle );
        g->u.activity.values.push_back( to_vehicle );
        while( !indices.empty() ) {
            g->u.activity.values.push_back( indices.front() );
            indices.pop_front();
            g->u.activity.values.push_back( quantities.front() );
            quantities.pop_front();
        }
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

static void move_item( item &it, int quantity, const tripoint &src, const tripoint &dest,
                       vehicle *src_veh, int src_part )
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
        g->u.mod_moves( -move_cost( it, src, dest ) );
        put_into_vehicle_or_drop( g->u, item_drop_reason::deliberate, { it }, dest );
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

void activity_on_turn_move_loot( player_activity &, player &p )
{
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
        bool is_adjacent_or_closer = square_dist( p.pos(), src_loc ) <= 1;

        // skip tiles in IGNORE zone and tiles on fire (to prevent taking out wood off the lit brazier)
        // and inaccessible furniture, like filled charcoal kiln
        if( mgr.has( zone_type_id( "LOOT_IGNORE" ), src ) ||
            g->m.get_field( src_loc, fd_fire ) != nullptr ||
            !g->m.can_put_items_ter_furn( src_loc ) ) {
            continue;
        }

        auto items = std::vector<item *>();

        //Check source for cargo part
        //map_stack and vehicle_stack are different types but inherit from item_stack
        //TODO: use one for loop
        if( const cata::optional<vpart_reference> vp = g->m.veh_at( src_loc ).part_with_feature( "CARGO",
                false ) ) {
            src_veh = &vp->vehicle();
            src_part = vp->part_index();
            for( auto &it : src_veh->get_items( src_part ) ) {
                if( !it.made_of_from_type( LIQUID ) ) { // skip unpickable liquid
                    items.push_back( &it );
                }
            }
        } else {
            src_veh = nullptr;
            src_part = -1;
            for( auto &it : g->m.i_at( src_loc ) ) {
                if( !it.made_of_from_type( LIQUID ) ) { // skip unpickable liquid
                    items.push_back( &it );
                }
            }
        }

        //Skip items that have already been processed
        for( auto it = items.begin() + mgr.get_num_processed( src ); it < items.end(); it++ ) {

            mgr.increment_num_processed( src );

            const auto id = mgr.get_near_zone_type_for_item( **it, abspos );

            // checks whether the item is already on correct loot zone or not
            // if it is, we can skip such item, if not we move the item to correct pile
            // think empty bag on food pile, after you ate the content
            if( !mgr.has( id, src ) ) {
                const auto &dest_set = mgr.get_near( id, abspos );

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
                    if( free_space >= ( *it )->volume() ) {
                        // before we move any item, check if player is at or adjacent to the loot source tile
                        if( !is_adjacent_or_closer ) {
                            std::vector<tripoint> route;
                            bool adjacent = false;

                            // get either direct route or route to nearest adjacent tile if source tile is impassable
                            if( g->m.passable( src_loc ) ) {
                                route = g->m.route( p.pos(), src_loc, p.get_pathfinding_settings(), p.get_path_avoid() );
                            } else {
                                // immpassable source tile (locker etc.), get route to nerest adjacent tile instead
                                route = route_adjacent( p, src_loc );
                                adjacent = true;
                            }

                            // check if we found path to source / adjacent tile
                            if( route.empty() ) {
                                add_msg( m_info, _( "You can't reach the source tile. Try to sort out loot without a cart." ) );
                                return;
                            }

                            // shorten the route to adjacent tile, if necessary
                            if( !adjacent ) {
                                route.pop_back();
                            }

                            // set the destination and restart activity after player arrives there
                            // we don't need to check for safe mode, activity will be restarted only if
                            // player arrives on destination tile
                            p.set_destination( route, player_activity( activity_id( "ACT_MOVE_LOOT" ) ) );

                            // didn't actually process so decrement
                            mgr.decrement_num_processed( src );
                            return;
                        }
                        move_item( **it, ( *it )->count(), src_loc, dest_loc, src_veh, src_part );

                        // moved item away from source so decrement
                        mgr.decrement_num_processed( src );

                        break;
                    }
                }
                if( p.moves <= 0 ) {
                    // Restart activity and break from cycle.
                    p.assign_activity( activity_id( "ACT_MOVE_LOOT" ) );
                    return;
                }
            }
        }
    }

    // If we got here without restarting the activity, it means we're done
    add_msg( m_info, _( "You sorted out every item you could." ) );
    mgr.end_sort();
}

cata::optional<tripoint> find_best_fire( const std::vector<tripoint> &from, const tripoint &center )
{
    cata::optional<tripoint> best_fire;
    time_duration best_fire_age = 1_days;
    for( const tripoint &pt : from ) {
        field_entry *fire = g->m.get_field( pt, fd_fire );
        if( fire == nullptr || fire->getFieldDensity() > 1 ||
            !g->m.clear_path( center, pt, PICKUP_RANGE, 1, 100 ) ) {
            continue;
        }
        time_duration fire_age = fire->getFieldAge();
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

void try_refuel_fire( player &p )
{
    const tripoint pos = p.pos();
    auto adjacent = closest_tripoints_first( PICKUP_RANGE, pos );
    adjacent.erase( adjacent.begin() );
    cata::optional<tripoint> best_fire = find_best_fire( adjacent, pos );

    if( !best_fire || !g->m.accessible_items( *best_fire ) ) {
        return;
    }

    const auto refuel_spot = std::find_if( adjacent.begin(), adjacent.end(),
    [pos]( const tripoint & pt ) {
        // Hacky - firewood spot is a trap and it's ID-checked
        // @todo Something cleaner than ID-checking a trap
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

    // Maybe @todo - refuelling in the rain could use more fuel
    // First, simulate expected burn per turn, to see if we need more fuel
    auto fuel_on_fire = g->m.i_at( *best_fire );
    for( size_t i = 0; i < fuel_on_fire.size(); i++ ) {
        fuel_on_fire[i].simulate_burn( fd );
        // Uncontained fires grow below -50_minutes age
        if( !contained && fire_age < -40_minutes && fd.fuel_produced > 1.0f &&
            !fuel_on_fire[i].made_of( LIQUID ) ) {
            // Too much - we don't want a firestorm!
            // Put first item back to refuelling pile
            std::list<int> indices_to_remove{ static_cast<int>( i ) };
            std::list<int> quantities_to_remove{ 0 };
            move_items( *best_fire - pos, false, *refuel_spot - pos, false, indices_to_remove,
                        quantities_to_remove );
            return;
        }
    }

    // Enough to sustain the fire
    // @todo It's not enough in the rain
    if( fd.fuel_produced >= 1.0f || fire_age < 10_minutes ) {
        return;
    }

    // We need to move fuel from stash to fire
    auto potential_fuel = g->m.i_at( *refuel_spot );
    for( size_t i = 0; i < potential_fuel.size(); i++ ) {
        if( potential_fuel[i].made_of( LIQUID ) ) {
            continue;
        }

        float last_fuel = fd.fuel_produced;
        potential_fuel[i].simulate_burn( fd );
        if( fd.fuel_produced > last_fuel ) {
            std::list<int> indices{ static_cast<int>( i ) };
            std::list<int> quantities{ 0 };
            // Note: move_items handles messages (they're the generic "you drop x")
            move_items( *refuel_spot - p.pos(), false, *best_fire - p.pos(), false, indices, quantities );
            return;
        }
    }
}
