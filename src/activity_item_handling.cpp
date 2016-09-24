#include "activity_handlers.h"

#include "game.h"
#include "map.h"
#include "item.h"
#include "player_activity.h"
#include "action.h"
#include "enums.h"
#include "creature.h"
#include "pickup.h"
#include "translations.h"
#include "messages.h"
#include "monster.h"
#include "vehicle.h"
#include "veh_type.h"
#include "player.h"
#include "debug.h"

#include <list>
#include <vector>
#include <cassert>
#include <algorithm>

const efftype_id effect_controlled( "controlled" );
const efftype_id effect_pet( "pet" );

/** Activity-associated item */
struct act_item {
    const item *it;         /// Pointer to the inventory item
    int count;              /// How many items need to be processed
    int consumed_moves;     /// Amount of moves that processing will consume

    act_item( const item *it, int count, int consumed_moves )
        : it( it ),
          count( count ),
          consumed_moves( consumed_moves ) {};
};

// @todo Deliberately unified with multidrop. Unify further.
typedef std::list<std::pair<int, int>> drop_indexes;

bool same_type( const std::list<item> &items )
{
    return std::all_of( items.begin(), items.end(), [ &items ]( const item & it ) {
        return it.type == items.begin()->type;
    } );
}

void put_into_vehicle( player &p, const std::list<item> &items, vehicle &veh, int part )
{
    if( items.empty() ) {
        return;
    }

    const tripoint where = veh.global_part_pos3( part );
    const std::string ter_name = g->m.name( where );
    int fallen_count = 0;

    for( auto it : items ) { // cant use constant reference here because of the spill_contents()
        if( it.is_bucket_nonempty() && !it.spill_contents( p ) ) {
            p.add_msg_player_or_npc(
                _( "To avoid spilling its contents, you set your %1$s on the %2$s." ),
                _( "To avoid spilling its contents, <npcname> sets their %1$s on the %2$s." ),
                it.display_name().c_str(), ter_name.c_str() );
            g->m.add_item_or_charges( where, it, 2 );
            continue;
        }
        if( !veh.add_item( part, it ) ) {
            g->m.add_item_or_charges( where, it, 1 );
            ++fallen_count;
        }
    }

    const std::string part_name = veh.part_info( part ).name();

    if( same_type( items ) ) {
        const item &it = items.front();
        const int dropcount = items.size() * ( it.count_by_charges() ? it.charges : 1 );
        add_msg( ngettext( "You put your %1$s in the %2$s's %3$s.",
                           "You put your %1$s in the %2$s's %3$s.", dropcount ),
                 it.tname( dropcount ).c_str(), veh.name.c_str(), part_name.c_str() );
    } else {
        add_msg( _( "You put several items in the %1$s's %2$s." ), veh.name.c_str(), part_name.c_str() );
    }

    if( fallen_count > 0 ) {
        add_msg( m_warning, _( "The trunk is full, so some items fell to the %s." ), ter_name.c_str() );
    }
}

void stash_on_pet( const std::list<item> &items, monster &pet )
{
    units::volume remaining_volume = pet.inv.empty() ? units::volume( 0 ) :
                                     pet.inv.front().get_storage();
    int remaining_weight = pet.weight_capacity();

    for( const auto &it : pet.inv ) {
        remaining_volume -= it.volume();
        remaining_weight -= it.weight();
    }

    for( auto &it : items ) {
        pet.add_effect( effect_controlled, 5 );
        if( it.volume() > remaining_volume ) {
            add_msg( m_bad, _( "%1$s did not fit and fell to the %2$s." ),
                     it.display_name().c_str(), g->m.name( pet.pos() ).c_str() );
            g->m.add_item_or_charges( pet.pos(), it, 1 );
        } else if( it.weight() > remaining_weight ) {
            add_msg( m_bad, _( "%1$s is too heavy and fell to the %2$s." ),
                     it.display_name().c_str(), g->m.name( pet.pos() ).c_str() );
            g->m.add_item_or_charges( pet.pos(), it, 1 );
        } else {
            pet.add_item( it );
            remaining_volume -= it.volume();
            remaining_weight -= it.weight();
        }
    }
}

void drop_on_map( const std::list<item> &items, const tripoint &where )
{
    if( items.empty() ) {
        return;
    }
    const std::string ter_name = g->m.name( where );
    const bool can_move_there = g->m.passable( where );

    if( same_type( items ) ) {
        const item &it = items.front();
        const int dropcount = items.size() * ( it.count_by_charges() ? it.charges : 1 );
        const std::string it_name = it.tname( dropcount );

        if( can_move_there ) {
            add_msg( ngettext( "You drop your %1$s on the %2$s.",
                               "You drop your %1$s on the %2$s.", dropcount ), it_name.c_str(), ter_name.c_str() );
        } else {
            add_msg( ngettext( "You put your %1$s in the %2$s.",
                               "You put your %1$s in the %2$s.", dropcount ), it_name.c_str(), ter_name.c_str() );
        }
    } else {
        if( can_move_there ) {
            add_msg( _( "You drop several items on the %s." ), ter_name.c_str() );
        } else {
            add_msg( _( "You put several items in the %s." ), ter_name.c_str() );
        }
    }
    for( const auto &it : items ) {
        g->m.add_item_or_charges( where, it, 2 );
    }
}

void put_into_vehicle_or_drop( player &p, const std::list<item> &items, const tripoint &where )
{
    int veh_part = 0;
    vehicle *veh = g->m.veh_at( where, veh_part );
    if( veh != nullptr ) {
        veh_part = veh->part_with_feature( veh_part, "CARGO" );
        if( veh_part >= 0 ) {
            put_into_vehicle( p, items, *veh, veh_part );
            return;
        }
    }
    drop_on_map( items, where );
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
                res.emplace_back( &it, qty, 100 ); // @todo Use a calculated cost
            }
        } else {
            res.emplace_back( &p.i_at( pos ), count, ( pos == -1 ) ? 0 : 100 ); // @todo Use a calculated cost
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
                worn.emplace_front( dit, dit->count_by_charges() ? dit->charges : 1,
                                    100 ); // @todo Use a calculated cost
            }
        }
    }
    // Sort worn items by storage in descending order, but dependent items always go first.
    worn.sort( []( const act_item & first, const act_item & second ) {
        return first.it->is_worn_only_with( *second.it )
               || ( ( first.it->get_storage() > second.it->get_storage() )
                    && !second.it->is_worn_only_with( *first.it ) );
    } );

    units::volume storage_loss = 0;                        // Cumulatively increases
    units::volume remaining_storage = p.volume_capacity(); // Cumulatively decreases

    while( !worn.empty() && !inv.empty() ) {
        storage_loss += worn.front().it->get_storage();
        remaining_storage -= p.volume_capacity_reduced_by( storage_loss );

        if( remaining_storage < inv.front().it->volume() ) {
            break; // Does not fit
        }

        while( !inv.empty() && remaining_storage >= inv.front().it->volume() ) {
            remaining_storage -= inv.front().it->volume();

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

//@todo Display costs in the multidrop menu
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
    if( excessive_volume > 0 ) {
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
    // And either cancel if it's empty, or restart if it's not.
    if( act.values.empty() ) {
        p.cancel_activity();
    } else {
        p.assign_activity( act );
    }

    return res;
}

void activity_handlers::drop_do_turn( player_activity *act, player *p )
{
    const tripoint pos = act->placement + p->pos();
    put_into_vehicle_or_drop( *p, obtain_activity_items( *act, *p ), pos );
}

void activity_handlers::stash_do_turn( player_activity *act, player *p )
{
    const tripoint pos = act->placement + p->pos();

    monster *pet = dynamic_cast<monster *>( g->critter_at( pos ) );
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
static void move_items( const tripoint &src, bool from_vehicle,
                        const tripoint &dest, bool to_vehicle,
                        std::list<int> &indices, std::list<int> &quantities )
{
    tripoint source = src + g->u.pos();
    tripoint destination = dest + g->u.pos();

    int s_cargo, d_cargo;   // oui oui, mon frere
    s_cargo = d_cargo = -1;
    vehicle *s_veh, *d_veh; // 2diva4me
    s_veh = d_veh = nullptr;

    // load vehicle information if requested
    if( from_vehicle == true ) {
        s_veh = g->m.veh_at( source, s_cargo );
        assert( s_veh != nullptr );
        s_cargo = s_veh->part_with_feature( s_cargo, "CARGO", false );
        assert( s_cargo >= 0 );
    }
    if( to_vehicle == true ) {
        d_veh = g->m.veh_at( destination, d_cargo );
        assert( d_veh != nullptr );
        d_cargo = d_veh->part_with_feature( d_cargo, "CARGO", false );
        assert( d_cargo >= 0 );
    }

    while( g->u.moves > 0 && !indices.empty() ) {
        int index = indices.back();
        int quantity = quantities.back();
        indices.pop_back();
        quantities.pop_back();

        item *temp_item = nullptr;

        temp_item = ( from_vehicle == true ) ?
                    g->m.item_from( s_veh, s_cargo, index ) :
                    g->m.item_from( source, index );

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
        if( !temp_item->made_of( LIQUID ) ) {
            // Is it too bulky? We'll have to use our hands, then.
            if( !g->u.can_pickVolume( *temp_item ) && g->u.is_armed() ) {
                g->u.moves -= 20; // Pretend to be unwielding our gun.
            }

            // Is it too heavy? It'll take a while...
            if( !g->u.can_pickWeight( *temp_item, true ) ) {
                int overweight = temp_item->weight() - ( g->u.weight_capacity() - g->u.weight_carried() );

                // ...like one move cost per 100 grams over your leftover carry capacity.
                g->u.moves -= int( overweight / 100 );
            }

            if( to_vehicle ) {
                put_into_vehicle_or_drop( g->u, { *temp_item }, destination );
            } else {
                drop_on_map( { *temp_item }, destination );
            }
            // Remove from map or vehicle.
            if( from_vehicle == true ) {
                s_veh->remove_item( s_cargo, index );
            } else {
                g->m.i_rem( source, index );
            }
            g->u.mod_moves( -200 ); // I kept the logic. -100 from 'drop' and -100 was here.
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
    // Drop activity if we don't know destination coords.
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
        g->u.assign_activity( ACT_MOVE_ITEMS, 0 );
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
