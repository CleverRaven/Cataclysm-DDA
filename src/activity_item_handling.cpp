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

#include <functional>

const efftype_id effect_controlled( "controlled" );
const efftype_id effect_pet( "pet" );

typedef std::list<std::pair<const item *, int>> activity_items;

bool game::make_drop_activity( enum activity_type act,
                               const std::list<std::pair<int, int>> &dropped,
                               const tripoint &target )
{
    if( dropped.empty() ) {
        return false;
    }
    u.assign_activity( act, 0 );
    u.activity.placement = target - u.pos();
    for( auto item_pair : dropped ) {
        u.activity.values.push_back( item_pair.first );
        u.activity.values.push_back( item_pair.second );
    }
    return true;
}

activity_items retrieve_items( const player_activity &act,
                               const std::function<void( activity_items &, int, int )> &retriever )
{
    activity_items retrieved;

    if( act.values.size() % 2 != 0 ) {
        debugmsg( "Activity \"%s\"contains odd number of values.", act.name.c_str() );
        return retrieved;
    }
    for( size_t index = 0; index < act.values.size(); index += 2 ) {
        const int position = act.values[index];
        const int quantity = act.values[index + 1];

        retriever( retrieved, position, quantity );
    }
    return retrieved;
}

activity_items retrieve_weapons( const player_activity &act, const player &p )
{
    return retrieve_items( act, [ &p ]( activity_items & retrieved, int position, int quantity ) {
        if( position == -1 ) {
            retrieved.emplace_back( &p.weapon, quantity );
        }
    } );
}

activity_items retrieve_inventory_items( const player_activity &act, const player &p )
{
    return retrieve_items( act, [ &p ]( activity_items & retrieved, int position, int quantity ) {
        if( position < 0 ) {
            return;
        }
        int obtained = 0;
        const auto &stack = p.inv.const_stack( position );
        for( const auto &it : stack ) {
            const int qty = it.count_by_charges() ? std::min( ( int )it.charges, quantity - obtained ) : 1;
            retrieved.emplace_back( &it, qty );
            obtained += qty;

            if( obtained >= quantity ) {
                break;
            }
        }
    } );
}

activity_items retrieve_worn_items( const player_activity &act, const player &p )
{
    return retrieve_items( act, [ &p ]( activity_items & retrieved, int position, int quantity ) {
        if( position < -1 ) {
            retrieved.emplace_back( &p.i_at( position ), quantity );
        }
    } );
}

bool same_type( const std::vector<item> &items )
{
    if( items.size() <= 1 ) {
        return true;
    }
    for( auto it = items.begin()++; it != items.end(); ++it ) {
        if( it->type != items.begin()->type ) {
            return false;
        }
    }
    return true;
}

void put_into_vehicle( player &p, const std::vector<item> &items, vehicle &veh, int part )
{
    if( items.empty() ) {
        return;
    }

    const tripoint where = veh.global_part_pos3( part );
    const std::string ter_name =  g->m.name( where );
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
        add_msg( m_warning, _( "The trunk is full, so some items fell to the %2$s." ), ter_name.c_str() );
    }
}

void stash_on_pet( const std::vector<item> &items, monster &pet )
{
    if( pet.inv.empty() ) {
        return;
    }
    for( auto &it : items ) {
        int max_volume = pet.inv.front().get_storage();
        int max_weight = pet.weight_capacity();

        for( const auto &it : pet.inv ) {
            max_volume -= it.volume();
            max_weight -= it.weight();
        }

        const bool too_heavy = it.weight() > max_weight;
        const bool too_big = it.volume() > max_volume;

        pet.add_effect( effect_controlled, 5 );

        if( too_heavy || too_big ) {
            const std::string it_name = it.display_name();
            const std::string ter_name = g->m.name( pet.pos() );

            if( too_big ) {
                add_msg( m_bad, _( "%s did not fit and fell to the %1$s." ), it_name.c_str(), ter_name.c_str() );
            } else {
                add_msg( m_bad, _( "%s is too heavy and fell to the %1$s." ), it_name.c_str(), ter_name.c_str() );
            }

            g->m.add_item_or_charges( pet.pos(), it, 1 );
        } else {
            pet.inv.push_back( it );
        }
    }
}

void drop_on_map( const std::vector<item> &items, const tripoint &where )
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

void put_into_vehicle_or_drop( player &p, const std::vector<item> &items, const tripoint &where )
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

// Returns number of items that fit into the volume. Order of the items matters.
int count_contained_items( const std::vector<item> &items, int volume )
{
    int count = 0;
    for( const auto &it : items ) {
        volume -= it.volume();
        if( volume < 0 ) {
            break;
        }
        ++count;
    }
    return count;
}

std::vector<item> withdraw_items_to_dispose( player_activity &act, player &p )
{
    auto weapons = retrieve_weapons( act, p );
    auto inventory_items = retrieve_inventory_items( act, p );
    auto worn_items = retrieve_worn_items( act, p );

    std::vector<item> result;

    const auto process_unworn_item = [ &p, &result ]( activity_items & items ) {
        auto &it = items.front();

        if( it.first->count_by_charges() ) {
            result.push_back( p.reduce_charges( const_cast<item *>( it.first ), it.second ) );
        } else {
            result.push_back( p.i_rem( it.first ) );
            if( --it.second > 0 ) {
                return; // Process one item at a time
            }
        }
        items.pop_front();
    };

    while( !weapons.empty() ) {
        process_unworn_item( weapons ); // Just let go. Don't consume moves.
    }

    while( p.moves >= 0 && !inventory_items.empty() ) {
        process_unworn_item( inventory_items );
        p.mod_moves( -100 );
    }

    while( p.moves >= 0 && !worn_items.empty() ) {
        p.takeoff( *worn_items.front().first, [ &result ]( const item & it ) {
            result.push_back( it );
            return true;
        } );
        worn_items.pop_front();
    }

    // Load anything left (if any) into a new activity.
    if( !weapons.empty() || !inventory_items.empty() || !worn_items.empty() ) {
        const player_activity prev_activity = act;
        const auto append_items = [ &p ]( const activity_items & items ) {
            for( const auto &it : items ) {
                p.activity.values.push_back( p.get_item_position( it.first ) );
                p.activity.values.push_back( it.second );
            }
        };
        p.cancel_activity();
        p.assign_activity( prev_activity.type, 0 );
        p.activity.placement = prev_activity.placement;
        p.activity.ignore_trivial = prev_activity.ignore_trivial;

        append_items( weapons );
        append_items( inventory_items );
        append_items( worn_items );
    } else {
        p.cancel_activity();
    }

    return result;
}

void activity_handlers::drop_do_turn( player_activity *act, player *p )
{
    const tripoint pos = act->placement + p->pos();
    put_into_vehicle_or_drop( *p, withdraw_items_to_dispose( *act, *p ), pos );
}

void activity_handlers::stash_do_turn( player_activity *act, player *p )
{
    const tripoint pos = act->placement + p->pos();
    Creature *critter = g->critter_at( pos );

    if( critter != nullptr ) {
        monster *pet = dynamic_cast<monster *>( critter );
        if( pet != nullptr && pet->has_effect( effect_pet ) ) {
            stash_on_pet( withdraw_items_to_dispose( *act, *p ), *pet );
        }
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
