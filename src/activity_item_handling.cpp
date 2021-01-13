#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "activity_handlers.h" // IWYU pragma: associated
#include "activity_type.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_assert.h"
#include "character.h"
#include "clzones.h"
#include "colony.h"
#include "construction.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "field.h"
#include "field_type.h"
#include "fire.h"
#include "flag.h"
#include "flat_set.h"
#include "game.h"
#include "game_constants.h"
#include "iexamine.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "optional.h"
#include "output.h"
#include "pickup.h"
#include "player.h"
#include "player_activity.h"
#include "point.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_id.h"
#include "temp_crafting_inventory.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "units_fwd.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "weather.h"

static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_BUTCHER_FULL( "ACT_BUTCHER_FULL" );
static const activity_id ACT_CHOP_LOGS( "ACT_CHOP_LOGS" );
static const activity_id ACT_CHOP_PLANKS( "ACT_CHOP_PLANKS" );
static const activity_id ACT_CHOP_TREE( "ACT_CHOP_TREE" );
static const activity_id ACT_CHURN( "ACT_CHURN" );
static const activity_id ACT_FETCH_REQUIRED( "ACT_FETCH_REQUIRED" );
static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_JACKHAMMER( "ACT_JACKHAMMER" );
static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );
static const activity_id ACT_MULTIPLE_BUTCHER( "ACT_MULTIPLE_BUTCHER" );
static const activity_id ACT_MULTIPLE_CHOP_PLANKS( "ACT_MULTIPLE_CHOP_PLANKS" );
static const activity_id ACT_MULTIPLE_CHOP_TREES( "ACT_MULTIPLE_CHOP_TREES" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );
static const activity_id ACT_MULTIPLE_FARM( "ACT_MULTIPLE_FARM" );
static const activity_id ACT_MULTIPLE_FISH( "ACT_MULTIPLE_FISH" );
static const activity_id ACT_MULTIPLE_MINE( "ACT_MULTIPLE_MINE" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_TIDY_UP( "ACT_TIDY_UP" );
static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );
static const activity_id ACT_VEHICLE_DECONSTRUCTION( "ACT_VEHICLE_DECONSTRUCTION" );
static const activity_id ACT_VEHICLE_REPAIR( "ACT_VEHICLE_REPAIR" );

static const efftype_id effect_incorporeal( "incorporeal" );

static const itype_id itype_battery( "battery" );
static const itype_id itype_detergent( "detergent" );
static const itype_id itype_log( "log" );
static const itype_id itype_soap( "soap" );
static const itype_id itype_soldering_iron( "soldering_iron" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_welder( "welder" );

static const trap_str_id tr_firewood_source( "tr_firewood_source" );
static const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

static const zone_type_id zone_type_SOURCE_FIREWOOD( "SOURCE_FIREWOOD" );

static const zone_type_id zone_type_CHOP_TREES( "CHOP_TREES" );
static const zone_type_id zone_type_CONSTRUCTION_BLUEPRINT( "CONSTRUCTION_BLUEPRINT" );
static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );
static const zone_type_id zone_type_FISHING_SPOT( "FISHING_SPOT" );
static const zone_type_id zone_type_LOOT_CORPSE( "LOOT_CORPSE" );
static const zone_type_id zone_type_LOOT_IGNORE( "LOOT_IGNORE" );
static const zone_type_id zone_type_LOOT_IGNORE_FAVORITES( "LOOT_IGNORE_FAVORITES" );
static const zone_type_id zone_type_MINING( "MINING" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_LOOT_WOOD( "LOOT_WOOD" );
static const zone_type_id zone_type_VEHICLE_DECONSTRUCT( "VEHICLE_DECONSTRUCT" );
static const zone_type_id zone_type_VEHICLE_REPAIR( "VEHICLE_REPAIR" );
static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );

static const quality_id qual_AXE( "AXE" );
static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_DIG( "DIG" );
static const quality_id qual_FISHING( "FISHING" );
static const quality_id qual_SAW_M( "SAW_M" );
static const quality_id qual_SAW_W( "SAW_W" );
static const quality_id qual_WELD( "WELD" );

static const std::string flag_BUTCHER_EQ( "BUTCHER_EQ" );
static const std::string flag_FISHABLE( "FISHABLE" );
static const std::string flag_GROWTH_HARVEST( "GROWTH_HARVEST" );
static const std::string flag_PLANT( "PLANT" );
static const std::string flag_PLANTABLE( "PLANTABLE" );
static const std::string flag_PLOWABLE( "PLOWABLE" );
static const std::string flag_TREE( "TREE" );

//Generic activity: maximum search distance for zones, constructions, etc.
static const int ACTIVITY_SEARCH_DISTANCE = 60;

/** Activity-associated item */
struct act_item {
    /// inventory item
    item_location loc;
    /// How many items need to be processed
    int count;
    /// Amount of moves that processing will consume
    int consumed_moves;

    act_item( const item_location &loc, int count, int consumed_moves )
        : loc( loc ),
          count( count ),
          consumed_moves( consumed_moves ) {}
};

// TODO: Deliberately unified with multidrop. Unify further.
using drop_location = std::pair<item_location, int>;
using drop_locations = std::list<std::pair<item_location, int>>;

static bool same_type( const std::list<item> &items )
{
    return std::all_of( items.begin(), items.end(), [&items]( const item & it ) {
        return it.type == items.begin()->type;
    } );
}

// Deprecated. See `contents_change_handler` and `Character::handle_contents_changed`
// for contents handling with item_location.
static bool handle_spillable_contents( Character &c, item &it, map &m )
{
    if( it.is_bucket_nonempty() ) {
        it.contents.spill_open_pockets( c, /*avoid=*/&it );

        // If bucket is still not empty then player opted not to handle the
        // rest of the contents
        if( !it.contents.empty() ) {
            c.add_msg_player_or_npc(
                _( "To avoid spilling its contents, you set your %1$s on the %2$s." ),
                _( "To avoid spilling its contents, <npcname> sets their %1$s on the %2$s." ),
                it.display_name(), m.name( c.pos() )
            );
            m.add_item_or_charges( c.pos(), it );
            return true;
        }
    }

    return false;
}

static void put_into_vehicle( Character &c, item_drop_reason reason, const std::list<item> &items,
                              vehicle &veh, int part )
{
    if( items.empty() ) {
        return;
    }

    const tripoint where = veh.global_part_pos3( part );
    map &here = get_map();
    const std::string ter_name = here.name( where );
    int fallen_count = 0;
    int into_vehicle_count = 0;

    // can't use constant reference here because of the spill_contents()
    for( item it : items ) {
        if( handle_spillable_contents( c, it, here ) ) {
            continue;
        }
        if( veh.add_item( part, it ) ) {
            into_vehicle_count += it.count();
        } else {
            if( it.count_by_charges() ) {
                // Maybe we can add a few charges in the trunk and the rest on the ground.
                int charges_added = veh.add_charges( part, it );
                it.mod_charges( -charges_added );
                into_vehicle_count += charges_added;
            }
            here.add_item_or_charges( where, it );
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

void drop_on_map( Character &c, item_drop_reason reason, const std::list<item> &items,
                  const tripoint &where )
{
    if( items.empty() ) {
        return;
    }
    map &here = get_map();
    const std::string ter_name = here.name( where );
    const bool can_move_there = here.passable( where );

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
    for( const item &it : items ) {
        here.add_item_or_charges( where, it );
        item( it ).handle_pickup_ownership( c );
    }
}

void put_into_vehicle_or_drop( Character &c, item_drop_reason reason, const std::list<item> &items )
{
    return put_into_vehicle_or_drop( c, reason, items, c.pos() );
}

void put_into_vehicle_or_drop( Character &c, item_drop_reason reason, const std::list<item> &items,
                               const tripoint &where, bool force_ground )
{
    map &here = get_map();
    const cata::optional<vpart_reference> vp = here.veh_at( where ).part_with_feature( "CARGO", false );
    if( vp && !force_ground ) {
        put_into_vehicle( c, reason, items, vp->vehicle(), vp->part_index() );
        return;
    }
    drop_on_map( c, reason, items, where );
}

static std::list<act_item> convert_to_act_item( const player_activity &act, Character &guy )
{
    std::list<act_item> res;

    if( act.values.size() != act.targets.size() ) {
        debugmsg( "Drop/stash activity contains an odd number of values." );
        return res;
    }
    for( size_t i = 0; i < act.values.size(); i++ ) {
        // locations may have become invalid as items are forcefully dropped
        // when they exceed the storage volume of the character
        if( !act.targets[i] || !act.targets[i].get_item() ) {
            continue;
        }
        res.emplace_back( act.targets[i], act.values[i], act.targets[i].obtain_cost( guy, act.values[i] ) );
    }
    return res;
}

void activity_on_turn_wear( player_activity &act, player &p )
{
    // ACT_WEAR has item_location targets, and int quantities
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
    std::list<act_item> items = convert_to_act_item( *act, *p );

    // Check again that we have enough water and soap incase the amount in our inventory changed somehow
    // Consume the water and soap
    units::volume total_volume = 0_ml;

    for( const act_item &filthy_item : items ) {
        total_volume += filthy_item.loc->volume();
    }
    washing_requirements required = washing_requirements_for_volume( total_volume );

    const auto is_liquid_crafting_component = []( const item & it ) {
        return is_crafting_component( it ) && ( !it.count_by_charges() || it.made_of( phase_id::LIQUID ) );
    };
    const inventory &crafting_inv = p->crafting_inventory();
    if( !crafting_inv.has_charges( itype_water, required.water, is_liquid_crafting_component ) &&
        !crafting_inv.has_charges( itype_water_clean, required.water, is_liquid_crafting_component ) ) {
        p->add_msg_if_player( _( "You need %1$i charges of water or clean water to wash these items." ),
                              required.water );
        act->set_to_null();
        return;
    } else if( !crafting_inv.has_charges( itype_soap, required.cleanser ) &&
               !crafting_inv.has_charges( itype_detergent, required.cleanser ) ) {
        p->add_msg_if_player( _( "You need %1$i charges of cleansing agent to wash these items." ),
                              required.cleanser );
        act->set_to_null();
        return;
    }

    for( const act_item &ait : items ) {
        item *filthy_item = const_cast<item *>( &*ait.loc );
        filthy_item->unset_flag( flag_FILTHY );
        p->on_worn_item_washed( *filthy_item );
    }

    std::vector<item_comp> comps;
    comps.push_back( item_comp( itype_water, required.water ) );
    comps.push_back( item_comp( itype_water_clean, required.water ) );
    p->consume_items( comps, 1, is_liquid_crafting_component );

    std::vector<item_comp> comps1;
    comps1.push_back( item_comp( itype_soap, required.cleanser ) );
    comps1.push_back( item_comp( itype_detergent, required.cleanser ) );
    p->consume_items( comps1 );

    p->add_msg_if_player( m_good, _( "You washed your items." ) );

    // Make sure newly washed components show up as available if player attempts to craft immediately
    p->invalidate_crafting_inventory();

    act->set_to_null();
}

static double get_capacity_fraction( int capacity, int volume )
{
    // fraction of capacity the item would occupy
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
    // in function pick_one_up, variable moves_taken has initial value of 100
    // and never changes until it is finally used in function
    // remove_from_map_or_vehicle
    const int pickup_cost = 100;

    // drop cost for non-tumbling items (from inventory overload) is also flat 100
    // according to convert_to_items (it does contain todo to use calculated costs)
    const int drop_cost = 100;

    // typical flat ground move cost
    const int mc_per_tile = 100;

    Character &player_character = get_player_character();
    // only free inventory capacity
    const int inventory_capacity = units::to_milliliter( player_character.volume_capacity() -
                                   player_character.volume_carried() );

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
    const int pickup_cost = Pickup::cost_to_move_item( get_player_character(), it );

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
    avatar &player_character = get_avatar();
    if( player_character.get_grab_type() == object_type::VEHICLE ) {
        tripoint cart_position = player_character.pos() + player_character.grab_point;

        if( const cata::optional<vpart_reference> vp = get_map().veh_at(
                    cart_position ).part_with_feature( "CARGO", false ) ) {
            vehicle veh = vp->vehicle();
            size_t vstor = vp->part_index();
            units::volume capacity = veh.free_volume( vstor );

            return move_cost_cart( it, src, dest, capacity );
        }
    }

    return move_cost_inv( it, src, dest );
}

// return true if activity was assigned.
// return false if it was not possible.
static bool vehicle_activity( player &p, const tripoint &src_loc, int vpindex, char type )
{
    map &here = get_map();
    vehicle *veh = veh_pointer_or_null( here.veh_at( src_loc ) );
    if( !veh ) {
        return false;
    }
    int time_to_take = 0;
    if( vpindex >= veh->part_count() ) {
        // if parts got removed during our work, we can't just carry on removing, we want to repair parts!
        // so just bail out, as we don't know if the next shifted part is suitable for repair.
        if( type == 'r' ) {
            return false;
        } else if( type == 'o' ) {
            vpindex = veh->get_next_shifted_index( vpindex, p );
            if( vpindex == -1 ) {
                return false;
            }
        }
    }
    const vpart_info &vp = veh->part_info( vpindex );
    if( type == 'r' ) {
        const vehicle_part part = veh->part( vpindex );
        time_to_take = vp.repair_time( p ) * part.damage() / part.max_damage();
    } else if( type == 'o' ) {
        time_to_take = vp.removal_time( p );
    }
    p.assign_activity( ACT_VEHICLE, time_to_take, static_cast<int>( type ) );
    // so , NPCs can remove the last part on a position, then there is no vehicle there anymore,
    // for someone else who stored that position at the start of their activity.
    // so we may need to go looking a bit further afield to find it , at activities end.
    for( const tripoint &pt : veh->get_points( true ) ) {
        p.activity.coord_set.insert( here.getabs( pt ) );
    }
    // values[0]
    p.activity.values.push_back( here.getabs( src_loc ).x );
    // values[1]
    p.activity.values.push_back( here.getabs( src_loc ).y );
    // values[2]
    p.activity.values.push_back( point_zero.x );
    // values[3]
    p.activity.values.push_back( point_zero.y );
    // values[4]
    p.activity.values.push_back( -point_zero.x );
    // values[5]
    p.activity.values.push_back( -point_zero.y );
    // values[6]
    p.activity.values.push_back( veh->index_of_part( &veh->part( vpindex ) ) );
    p.activity.str_values.push_back( vp.get_id().str() );
    // this would only be used for refilling tasks
    item_location target;
    p.activity.targets.emplace_back( std::move( target ) );
    p.activity.placement = here.getabs( src_loc );
    p.activity_vehicle_part_index = -1;
    return true;
}

static void move_item( player &p, item &it, const int quantity, const tripoint &src,
                       const tripoint &dest, vehicle *src_veh, int src_part,
                       const activity_id &activity_to_restore = activity_id::NULL_ID() )
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

    map &here = get_map();
    // Check that we can pick it up.
    if( !it.made_of_from_type( phase_id::LIQUID ) ) {
        p.mod_moves( -move_cost( it, src, dest ) );
        if( activity_to_restore == ACT_TIDY_UP ) {
            it.erase_var( "activity_var" );
        } else if( activity_to_restore == ACT_FETCH_REQUIRED ) {
            it.set_var( "activity_var", p.name );
        }
        put_into_vehicle_or_drop( p, item_drop_reason::deliberate, { it }, dest );
        // Remove from map or vehicle.
        if( src_veh ) {
            src_veh->remove_item( src_part, &it );
        } else {
            here.i_rem( src, &it );
        }
    }

    // If we didn't pick up a whole stack, put the remainder back where it came from.
    if( leftovers.charges > 0 ) {
        if( src_veh ) {
            if( !src_veh->add_item( src_part, leftovers ) ) {
                debugmsg( "SortLoot: Source vehicle failed to receive leftover charges." );
            }
        } else {
            here.add_item_or_charges( src, leftovers );
        }
    }
}

std::vector<tripoint> route_adjacent( const player &p, const tripoint &dest )
{
    std::unordered_set<tripoint> passable_tiles = std::unordered_set<tripoint>();
    map &here = get_map();

    for( const tripoint &tp : here.points_in_radius( dest, 1 ) ) {
        if( tp != p.pos() && here.passable( tp ) ) {
            passable_tiles.emplace( tp );
        }
    }

    const std::vector<tripoint> &sorted = get_sorted_tiles_by_distance( p.pos(), passable_tiles );

    const std::set<tripoint> &avoid = p.get_path_avoid();
    for( const tripoint &tp : sorted ) {
        std::vector<tripoint> route = here.route( p.pos(), tp, p.get_pathfinding_settings(), avoid );

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
    const cata::optional<construction_id> &part_con_idx,
    const construction_id &idx,
    std::set<construction_id> &used )
{
    const construction &build = idx.obj();
    //already done?
    map &here = get_map();
    const furn_id furn = here.furn( loc );
    const ter_id ter = here.ter( loc );
    if( !build.post_terrain.empty() ) {
        if( build.post_is_furniture ) {
            if( furn_id( build.post_terrain ) == furn ) {
                return activity_reason_info::build( do_activity_reason::ALREADY_DONE, false, idx );
            }
        } else {
            if( ter_id( build.post_terrain ) == ter ) {
                return activity_reason_info::build( do_activity_reason::ALREADY_DONE, false, idx );
            }
        }
    }
    //if there's an appropriate partial construction on the tile, then we can work on it, no need to check inventories.
    const bool has_skill = p.meets_skill_requirements( build );
    if( part_con_idx && *part_con_idx == idx ) {
        if( !has_skill ) {
            return activity_reason_info::build( do_activity_reason::DONT_HAVE_SKILL, false, idx );
        }
        return activity_reason_info::build( do_activity_reason::CAN_DO_CONSTRUCTION, true, idx );
    }
    //can build?
    const bool cc = can_construct( build, loc );
    const bool pcb = player_can_build( p, inv, build );
    if( !has_skill ) {
        return activity_reason_info::build( do_activity_reason::DONT_HAVE_SKILL, false, idx );
    }
    if( cc ) {
        if( pcb ) {
            return activity_reason_info::build( do_activity_reason::CAN_DO_CONSTRUCTION, true, idx );
        }
        //can't build with current inventory, do not look for pre-req
        return activity_reason_info::build( do_activity_reason::NO_COMPONENTS, false, idx );
    }

    // there are no pre-requisites.
    // so we need to potentially fetch components
    if( build.pre_terrain.empty() && build.pre_special( loc ) ) {
        return activity_reason_info::build( do_activity_reason::NO_COMPONENTS, false, idx );
    } else if( !build.pre_special( loc ) ) {
        return activity_reason_info::build( do_activity_reason::BLOCKING_TILE, false, idx );
    }

    // can't build it
    // maybe we can build the pre-requisite instead
    // see if the reason is because of pre-terrain requirement
    if( !build.pre_terrain.empty() &&
        ( ( build.pre_is_furniture &&
            furn_id( build.pre_terrain ) == furn ) ||
          ( !build.pre_is_furniture &&
            ter_id( build.pre_terrain ) == ter ) ) ) {
        // the pre-req is already built, so the reason is due to lack of tools/components
        return activity_reason_info::build( do_activity_reason::NO_COMPONENTS, false, idx );
    }

    //we can't immediately build it, looking for pre-req
    used.insert( idx );
    cata::optional<do_activity_reason> reason;
    construction_id pre_req_idx( -1 );
    //first step: try only constructions with the same group
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
            //at first step, try to get building with the same group
            if( try_num == 0 &&
                pre_build.group != build.group ) {
                continue;
            }
            activity_reason_info act_info_pre = find_base_construction( list_constructions,
                                                p, inv, loc, part_con_idx, pre_build.id, used );
            if( act_info_pre.can_do ) {
                return activity_reason_info::build( do_activity_reason::CAN_DO_PREREQ, true,
                                                    *act_info_pre.con_idx );
            }
            //find first pre-req failed reason
            if( !reason ) {
                reason = act_info_pre.reason;
                pre_req_idx = *act_info_pre.con_idx;
            }
            if( act_info_pre.reason == do_activity_reason::ALREADY_DONE ) {
                //pre-req is already here, but we still can't build over it
                reason.reset();
                break;
            }
        }
    }
    //have a partial construction which is not leading to the required construction
    if( part_con_idx ) {
        return activity_reason_info::build( do_activity_reason::BLOCKING_TILE, false, idx );
    }
    //pre-req failed?
    if( reason ) {
        if( *reason == do_activity_reason::NO_COMPONENTS ) {
            return activity_reason_info::build( do_activity_reason::NO_COMPONENTS_PREREQ, false, pre_req_idx );
        }
        return activity_reason_info::build( *reason, false, pre_req_idx );
    }
    if( !pcb ) {
        return activity_reason_info::build( do_activity_reason::NO_COMPONENTS, false, idx );
    }
    //only cc failed, no pre-req
    return activity_reason_info::build( do_activity_reason::BLOCKING_TILE, false, idx );
}

static std::string random_string( size_t length )
{
    auto randchar = []() -> char {
        static constexpr char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        static constexpr size_t num_chars = sizeof( charset ) - 1;
        return charset[rng( 0, num_chars - 1 )];
    };
    std::string str( length, 0 );
    std::generate_n( str.begin(), length, randchar );
    return str;
}

static bool are_requirements_nearby( const std::vector<tripoint> &loot_spots,
                                     const requirement_id &needed_things, player &p, const activity_id &activity_to_restore,
                                     const bool in_loot_zones, const tripoint &src_loc )
{
    zone_manager &mgr = zone_manager::get_manager();
    temp_crafting_inventory temp_inv;
    units::volume volume_allowed = p.volume_capacity() - p.volume_carried();
    units::mass weight_allowed = p.weight_capacity() - p.weight_carried();
    static const auto check_weight_if = []( const activity_id & id ) {
        return id == ACT_MULTIPLE_FARM ||
               id == ACT_MULTIPLE_CHOP_PLANKS ||
               id == ACT_MULTIPLE_BUTCHER ||
               id == ACT_VEHICLE_DECONSTRUCTION ||
               id == ACT_VEHICLE_REPAIR ||
               id == ACT_MULTIPLE_CHOP_TREES ||
               id == ACT_MULTIPLE_FISH ||
               id == ACT_MULTIPLE_MINE;
    };
    const bool check_weight = check_weight_if( activity_to_restore ) || ( !p.backlog.empty() &&
                              check_weight_if( p.backlog.front().id() ) );
    bool found_welder = false;
    for( item *elem : p.inv_dump() ) {
        if( elem->has_quality( qual_WELD ) ) {
            found_welder = true;
        }
        temp_inv.add_item_ref( *elem );
    }
    map &here = get_map();
    for( const tripoint &elem : loot_spots ) {
        // if we are searching for things to fetch, we can skip certain things.
        // if, however they are already near the work spot, then the crafting / inventory functions will have their own method to use or discount them.
        if( in_loot_zones ) {
            // skip tiles in IGNORE zone and tiles on fire
            // (to prevent taking out wood off the lit brazier)
            // and inaccessible furniture, like filled charcoal kiln
            if( mgr.has( zone_type_LOOT_IGNORE, here.getabs( elem ) ) ||
                here.dangerous_field_at( elem ) ||
                !here.can_put_items_ter_furn( elem ) ) {
                continue;
            }
        }
        for( item &elem2 : here.i_at( elem ) ) {
            if( in_loot_zones && elem2.made_of_from_type( phase_id::LIQUID ) ) {
                continue;
            }
            if( check_weight ) {
                // this fetch task will need to pick up an item. so check for its weight/volume before setting off.
                if( in_loot_zones && ( elem2.volume() > volume_allowed ||
                                       elem2.weight() > weight_allowed ) ) {
                    continue;
                }
            }
            temp_inv.add_item_ref( elem2 );
        }
        if( !in_loot_zones ) {
            if( const cata::optional<vpart_reference> vp = here.veh_at( elem ).part_with_feature( "CARGO",
                    false ) ) {
                vehicle &src_veh = vp->vehicle();
                int src_part = vp->part_index();
                for( item &it : src_veh.get_items( src_part ) ) {
                    temp_inv.add_item_ref( it );
                }
            }
        }
    }
    // use nearby welding rig without needing to drag it or position yourself on the right side of the vehicle.
    if( !found_welder ) {
        for( const tripoint &elem : here.points_in_radius( src_loc, PICKUP_RANGE - 1 ) ) {
            const cata::optional<vpart_reference> &vp = here.veh_at( elem ).part_with_tool( itype_welder );

            if( vp ) {
                const int veh_battery = vp->vehicle().fuel_left( itype_battery, true );

                item welder( itype_welder, calendar::turn_zero );
                welder.charges = veh_battery;
                welder.set_flag( flag_PSEUDO );
                temp_inv.add_item_copy( welder );
                item soldering_iron( itype_soldering_iron, calendar::turn_zero );
                soldering_iron.charges = veh_battery;
                soldering_iron.set_flag( flag_PSEUDO );
                temp_inv.add_item_copy( soldering_iron );
            }
        }
    }
    return needed_things.obj().can_make_with_inventory( temp_inv, is_crafting_component );
}

static bool has_skill_for_vehicle_work( const std::map<skill_id, int> &required_skills, player &p )
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
        const tripoint &src_loc, const int distance = ACTIVITY_SEARCH_DISTANCE )
{
    // see activity_handlers.h cant_do_activity_reason enums
    p.invalidate_crafting_inventory();
    zone_manager &mgr = zone_manager::get_manager();
    std::vector<zone_data> zones;
    Character &player_character = get_player_character();
    map &here = get_map();
    if( act == ACT_VEHICLE_DECONSTRUCTION ||
        act == ACT_VEHICLE_REPAIR ) {
        std::vector<int> already_working_indexes;
        vehicle *veh = veh_pointer_or_null( here.veh_at( src_loc ) );
        if( !veh ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
        // if the vehicle is moving or player is controlling it.
        if( std::abs( veh->velocity ) > 100 || veh->player_in_control( player_character ) ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
        for( const npc &guy : g->all_npcs() ) {
            if( &guy == &p ) {
                continue;
            }
            // If the NPC has an activity - make sure they're not duplicating work.
            tripoint guy_work_spot;
            if( guy.has_player_activity() && guy.activity.placement != tripoint_min ) {
                guy_work_spot = here.getlocal( guy.activity.placement );
            }
            // If their position or intended position or player position/intended position
            // then discount, don't need to move each other out of the way.
            if( here.getlocal( player_character.activity.placement ) == src_loc ||
                guy_work_spot == src_loc || guy.pos() == src_loc ||
                ( p.is_npc() && player_character.pos() == src_loc ) ) {
                return activity_reason_info::fail( do_activity_reason::ALREADY_WORKING );
            }
            if( guy_work_spot != tripoint_zero ) {
                vehicle *other_veh = veh_pointer_or_null( here.veh_at( guy_work_spot ) );
                // working on same vehicle - store the index to check later.
                if( other_veh && other_veh == veh && guy.activity_vehicle_part_index != -1 ) {
                    already_working_indexes.push_back( guy.activity_vehicle_part_index );
                }
            }
            if( player_character.activity_vehicle_part_index != -1 ) {
                already_working_indexes.push_back( player_character.activity_vehicle_part_index );
            }
        }
        if( act == ACT_VEHICLE_DECONSTRUCTION ) {
            // find out if there is a vehicle part here we can remove.
            std::vector<vehicle_part *> parts = veh->get_parts_at( src_loc, "", part_status_flag::any );
            for( vehicle_part *part_elem : parts ) {
                const vpart_info &vpinfo = part_elem->info();
                int vpindex = veh->index_of_part( part_elem, true );
                // if part is not on this vehicle, or if its attached to another part that needs to be removed first.
                if( vpindex == -1 || !veh->can_unmount( vpindex ) ) {
                    continue;
                }
                // If removing this part would make the vehicle non-flyable, avoid it
                if( veh->would_removal_prevent_flyable( *part_elem,
                                                        player_character ) ) {
                    return activity_reason_info::fail( do_activity_reason::WOULD_PREVENT_VEH_FLYING );
                }
                // this is the same part that somebody else wants to work on, or already is.
                if( std::find( already_working_indexes.begin(), already_working_indexes.end(),
                               vpindex ) != already_working_indexes.end() ) {
                    continue;
                }
                // don't have skill to remove it
                if( !has_skill_for_vehicle_work( vpinfo.removal_skills, p ) ) {
                    continue;
                }
                item base( vpinfo.base_item );
                if( base.is_wheel() ) {
                    // no wheel removal yet
                    continue;
                }
                const units::mass max_lift = p.best_nearby_lifting_assist( src_loc );
                const bool use_aid = max_lift >= base.weight();
                const bool use_str = p.can_lift( base );
                if( !( use_aid || use_str ) ) {
                    continue;
                }
                const requirement_data &reqs = vpinfo.removal_requirements();
                const inventory &inv = p.crafting_inventory( false );

                const bool can_make = reqs.can_make_with_inventory( inv, is_crafting_component );
                p.set_value( "veh_index_type", vpinfo.name() );
                // temporarily store the intended index, we do this so two NPCs don't try and work on the same part at same time.
                p.activity_vehicle_part_index = vpindex;
                if( !can_make ) {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_VEH_DECONST );
                } else {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_VEH_DECONST );
                }
            }
        } else if( act == ACT_VEHICLE_REPAIR ) {
            // find out if there is a vehicle part here we can repair.
            std::vector<vehicle_part *> parts = veh->get_parts_at( src_loc, "", part_status_flag::any );
            for( vehicle_part *part_elem : parts ) {
                const vpart_info &vpinfo = part_elem->info();
                int vpindex = veh->index_of_part( part_elem, true );
                // if part is undamaged or beyond repair - can skip it.
                if( part_elem->is_broken() || part_elem->damage() == 0 ||
                    part_elem->info().repair_requirements().is_empty() ) {
                    continue;
                }
                // If repairing this part would make the vehicle non-flyable, avoid it
                if( veh->would_repair_prevent_flyable( *part_elem,
                                                       player_character ) ) {
                    return activity_reason_info::fail( do_activity_reason::WOULD_PREVENT_VEH_FLYING );
                }
                if( std::find( already_working_indexes.begin(), already_working_indexes.end(),
                               vpindex ) != already_working_indexes.end() ) {
                    continue;
                }
                // don't have skill to repair it
                if( !has_skill_for_vehicle_work( vpinfo.repair_skills, p ) ) {
                    continue;
                }
                const requirement_data &reqs = vpinfo.repair_requirements();
                const inventory &inv = p.crafting_inventory( src_loc, PICKUP_RANGE - 1, false );
                const bool can_make = reqs.can_make_with_inventory( inv, is_crafting_component );
                p.set_value( "veh_index_type", vpinfo.name() );
                // temporarily store the intended index, we do this so two NPCs don't try and work on the same part at same time.
                p.activity_vehicle_part_index = vpindex;
                if( !can_make ) {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_VEH_REPAIR );
                } else {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_VEH_REPAIR );
                }
            }
        }
        p.activity_vehicle_part_index = -1;
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
    if( act == ACT_MULTIPLE_MINE ) {
        if( !here.has_flag( "MINEABLE", src_loc ) ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
        std::vector<item *> mining_inv = p.items_with( []( const item & itm ) {
            return ( itm.has_flag( flag_DIG_TOOL ) && !itm.type->can_use( "JACKHAMMER" ) ) ||
                   ( itm.type->can_use( "JACKHAMMER" ) && itm.ammo_sufficient() );
        } );
        if( mining_inv.empty() ) {
            return activity_reason_info::fail( do_activity_reason::NEEDS_MINING );
        } else {
            return activity_reason_info::ok( do_activity_reason::NEEDS_MINING );
        }
    }
    if( act == ACT_MULTIPLE_FISH ) {
        if( !here.has_flag( flag_FISHABLE, src_loc ) ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
        std::vector<item *> rod_inv = p.items_with( []( const item & itm ) {
            return itm.has_flag( flag_FISH_POOR ) || itm.has_flag( flag_FISH_GOOD );
        } );
        if( rod_inv.empty() ) {
            return activity_reason_info::fail( do_activity_reason::NEEDS_FISHING );
        } else {
            return activity_reason_info::ok( do_activity_reason::NEEDS_FISHING );
        }
    }
    if( act == ACT_MULTIPLE_CHOP_TREES ) {
        if( here.has_flag( flag_TREE, src_loc ) || here.ter( src_loc ) == t_trunk ||
            here.ter( src_loc ) == t_stump ) {
            if( p.has_quality( qual_AXE ) ) {
                return activity_reason_info::ok( do_activity_reason::NEEDS_TREE_CHOPPING );
            } else {
                return activity_reason_info::fail( do_activity_reason::NEEDS_TREE_CHOPPING );
            }
        } else {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
    }
    if( act == ACT_MULTIPLE_BUTCHER ) {
        std::vector<item> corpses;
        int big_count = 0;
        int small_count = 0;
        for( const item &i : here.i_at( src_loc ) ) {
            // make sure nobody else is working on that corpse right now
            if( i.is_corpse() && !i.has_var( "activity_var" ) ) {
                const mtype corpse = *i.get_mtype();
                if( corpse.size >= creature_size::medium ) {
                    big_count += 1;
                } else {
                    small_count += 1;
                }
                corpses.push_back( i );
            }
        }
        bool b_rack_present = false;
        for( const tripoint &pt : here.points_in_radius( src_loc, 2 ) ) {
            if( here.has_flag_furn( flag_BUTCHER_EQ, pt ) ) {
                b_rack_present = true;
            }
        }
        if( !corpses.empty() ) {
            if( big_count > 0 && small_count == 0 ) {
                if( !b_rack_present || !here.has_nearby_table( src_loc, 2 ) ) {
                    return activity_reason_info::fail( do_activity_reason::NO_ZONE );
                }
                if( p.has_quality( quality_id( qual_BUTCHER ), 1 ) && ( p.has_quality( qual_SAW_W ) ||
                        p.has_quality( qual_SAW_M ) ) ) {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_BIG_BUTCHERING );
                } else {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_BIG_BUTCHERING );
                }
            }
            if( ( big_count > 0 && small_count > 0 ) || ( big_count == 0 ) ) {
                // there are small corpses here, so we can ignore any big corpses here for the moment.
                if( p.has_quality( qual_BUTCHER, 1 ) ) {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_BUTCHERING );
                } else {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_BUTCHERING );
                }
            }
        }
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
    if( act == ACT_MULTIPLE_CHOP_PLANKS ) {
        //are there even any logs there?
        for( item &i : here.i_at( src_loc ) ) {
            if( i.typeId() == itype_log ) {
                // do we have an axe?
                if( p.has_quality( qual_AXE, 1 ) ) {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_CHOPPING );
                } else {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_CHOPPING );
                }
            }
        }
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
    if( act == ACT_TIDY_UP ) {
        if( mgr.has_near( zone_type_LOOT_UNSORTED, here.getabs( src_loc ), distance ) ||
            mgr.has_near( zone_type_CAMP_STORAGE, here.getabs( src_loc ), distance ) ) {
            return activity_reason_info::ok( do_activity_reason::CAN_DO_FETCH );
        }
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
    if( act == ACT_MULTIPLE_CONSTRUCTION ) {
        const std::vector<construction> &list_constructions = get_constructions();
        zones = mgr.get_zones( zone_type_CONSTRUCTION_BLUEPRINT,
                               here.getabs( src_loc ) );
        const partial_con *part_con = here.partial_con_at( src_loc );
        cata::optional<construction_id> part_con_idx;
        if( part_con ) {
            part_con_idx = part_con->id;
        }
        const map_stack stuff_there = here.i_at( src_loc );

        // PICKUP_RANGE -1 because we will be adjacent to the spot when arriving.
        const inventory pre_inv = p.crafting_inventory( src_loc, PICKUP_RANGE - 1 );
        for( const zone_data &zone : zones ) {
            const blueprint_options &options = dynamic_cast<const blueprint_options &>( zone.get_options() );
            const construction_id index = options.get_index();
            if( !stuff_there.empty() ) {
                return activity_reason_info::build( do_activity_reason::BLOCKING_TILE, false, index );
            }
            std::set<construction_id> used_idx;
            const activity_reason_info act_info = find_base_construction( list_constructions, p, pre_inv,
                                                  src_loc, part_con_idx, index, used_idx );
            return act_info;
        }
    } else if( act == ACT_MULTIPLE_FARM ) {
        zones = mgr.get_zones( zone_type_FARM_PLOT,
                               here.getabs( src_loc ) );
        for( const zone_data &zone : zones ) {
            if( here.has_flag_furn( flag_GROWTH_HARVEST, src_loc ) ) {
                // simple work, pulling up plants, nothing else required.
                return activity_reason_info::ok( do_activity_reason::NEEDS_HARVESTING );
            } else if( here.has_flag( flag_PLOWABLE, src_loc ) && !here.has_furn( src_loc ) ) {
                if( p.has_quality( qual_DIG, 1 ) ) {
                    // we have a shovel/hoe already, great
                    return activity_reason_info::ok( do_activity_reason::NEEDS_TILLING );
                } else {
                    // we need a shovel/hoe
                    return activity_reason_info::fail( do_activity_reason::NEEDS_TILLING );
                }
            } else if( here.has_flag_ter_or_furn( flag_PLANTABLE, src_loc ) &&
                       warm_enough_to_plant( src_loc ) ) {
                if( here.has_items( src_loc ) ) {
                    return activity_reason_info::fail( do_activity_reason::BLOCKING_TILE );
                } else {
                    // do we have the required seed on our person?
                    const plot_options &options = dynamic_cast<const plot_options &>( zone.get_options() );
                    const itype_id seed = options.get_seed();
                    // If its a farm zone with no specified seed, and we've checked for tilling and harvesting.
                    // then it means no further work can be done here
                    if( seed.is_empty() ) {
                        return activity_reason_info::fail( do_activity_reason::ALREADY_DONE );
                    }
                    std::vector<item *> seed_inv = p.items_with( []( const item & itm ) {
                        return itm.is_seed();
                    } );
                    for( const item *elem : seed_inv ) {
                        if( elem->typeId() == itype_id( seed ) ) {
                            return activity_reason_info::ok( do_activity_reason::NEEDS_PLANTING );
                        }
                    }
                    // didn't find the seed, but maybe there are overlapping farm zones
                    // and another of the zones is for a seed that we have
                    // so loop again, and return false once all zones done.
                }

            } else {
                // can't plant, till or harvest
                return activity_reason_info::fail( do_activity_reason::ALREADY_DONE );
            }

        }
        // looped through all zones, and only got here if its plantable, but have no seeds.
        return activity_reason_info::fail( do_activity_reason::NEEDS_PLANTING );
    } else if( act == ACT_FETCH_REQUIRED ) {
        // we check if its possible to get all the requirements for fetching at two other places.
        // 1. before we even assign the fetch activity and;
        // 2. when we form the src_set to loop through at the beginning of the fetch activity.
        return activity_reason_info::ok( do_activity_reason::CAN_DO_FETCH );
    }
    // Shouldn't get here because the zones were checked previously. if it does, set enum reason as "no zone"
    return activity_reason_info::fail( do_activity_reason::NO_ZONE );
}

static void add_basecamp_storage_to_loot_zone_list( zone_manager &mgr, const tripoint &src_loc,
        player &p, std::vector<tripoint> &loot_zone_spots, std::vector<tripoint> &combined_spots )
{
    if( npc *const guy = dynamic_cast<npc *>( &p ) ) {
        map &here = get_map();
        if( guy->assigned_camp &&
            mgr.has_near( zone_type_CAMP_STORAGE, here.getabs( src_loc ), ACTIVITY_SEARCH_DISTANCE ) ) {
            std::unordered_set<tripoint> bc_storage_set = mgr.get_near( zone_type_id( "CAMP_STORAGE" ),
                    here.getabs( src_loc ), ACTIVITY_SEARCH_DISTANCE );
            for( const tripoint &elem : bc_storage_set ) {
                loot_zone_spots.push_back( here.getlocal( elem ) );
                combined_spots.push_back( here.getlocal( elem ) );
            }
        }
    }
}

static std::vector<std::tuple<tripoint, itype_id, int>> requirements_map( player &p,
        const int distance = ACTIVITY_SEARCH_DISTANCE )
{
    std::vector<std::tuple<tripoint, itype_id, int>> requirement_map;
    if( p.backlog.empty() || p.backlog.front().str_values.empty() ) {
        return requirement_map;
    }
    const requirement_data things_to_fetch = requirement_id( p.backlog.front().str_values[0] ).obj();
    const activity_id activity_to_restore = p.backlog.front().id();
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    requirement_id things_to_fetch_id = things_to_fetch.id();
    std::vector<std::vector<item_comp>> req_comps = things_to_fetch.get_components();
    std::vector<std::vector<tool_comp>> tool_comps = things_to_fetch.get_tools();
    std::vector<std::vector<quality_requirement>> quality_comps = things_to_fetch.get_qualities();
    zone_manager &mgr = zone_manager::get_manager();
    const bool pickup_task = p.backlog.front().id() == ACT_MULTIPLE_FARM ||
                             p.backlog.front().id() == ACT_MULTIPLE_CHOP_PLANKS ||
                             p.backlog.front().id() == ACT_MULTIPLE_BUTCHER ||
                             p.backlog.front().id() == ACT_MULTIPLE_CHOP_TREES ||
                             p.backlog.front().id() == ACT_VEHICLE_DECONSTRUCTION ||
                             p.backlog.front().id() == ACT_VEHICLE_REPAIR ||
                             p.backlog.front().id() == ACT_MULTIPLE_FISH ||
                             p.backlog.front().id() == ACT_MULTIPLE_MINE;
    // where it is, what it is, how much of it, and how much in total is required of that item.
    std::vector<std::tuple<tripoint, itype_id, int>> final_map;
    std::vector<tripoint> loot_spots;
    std::vector<tripoint> already_there_spots;
    std::vector<tripoint> combined_spots;
    std::map<itype_id, int> total_map;
    map &here = get_map();
    tripoint src_loc = here.getlocal( p.backlog.front().placement );
    for( const tripoint &elem : here.points_in_radius( src_loc,
            PICKUP_RANGE - 1 ) ) {
        already_there_spots.push_back( elem );
        combined_spots.push_back( elem );
    }
    for( const tripoint &elem : mgr.get_point_set_loot( here.getabs( p.pos() ), distance,
            p.is_npc() ) ) {
        // if there is a loot zone that's already near the work spot, we don't want it to be added twice.
        if( std::find( already_there_spots.begin(), already_there_spots.end(),
                       elem ) != already_there_spots.end() ) {
            // construction tasks don't need the loot spot *and* the already_there/combined spots both added.
            // but a farming task will need to go and fetch the tool no matter if its near the work spot.
            // whereas the construction will automatically use what's nearby anyway.
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
    add_basecamp_storage_to_loot_zone_list( mgr, src_loc, p, loot_spots, combined_spots );
    // if the requirements aren't available, then stop.
    if( !are_requirements_nearby( pickup_task ? loot_spots : combined_spots, things_to_fetch_id, p,
                                  activity_to_restore, pickup_task, src_loc ) ) {
        return requirement_map;
    }
    // if the requirements are already near the work spot and its a construction/crafting task, then no need to fetch anything more.
    if( !pickup_task &&
        are_requirements_nearby( already_there_spots, things_to_fetch_id, p, activity_to_restore,
                                 false, src_loc ) ) {
        return requirement_map;
    }
    // a vector of every item in every tile that matches any part of the requirements.
    // will be filtered for amounts/charges afterwards.
    for( const tripoint &point_elem : pickup_task ? loot_spots : combined_spots ) {
        std::map<itype_id, int> temp_map;
        for( const item &stack_elem : here.i_at( point_elem ) ) {
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
                            // we don't care if there are 10 welders with 5 charges each
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
        for( const auto &map_elem : temp_map ) {
            total_map[map_elem.first] += map_elem.second;
            // if its a construction/crafting task, we can discount any items already near the work spot.
            // we don't need to fetch those, they will be used automatically in the construction.
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
    for( const std::vector<item_comp> &elem : req_comps ) {
        bool line_found = false;
        for( const item_comp &comp_elem : elem ) {
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
    for( const std::vector<tool_comp> &elem : tool_comps ) {
        bool line_found = false;
        for( const tool_comp &comp_elem : elem ) {
            if( line_found || comp_elem.count < -1 ) {
                break;
            }
            int quantity_required = std::max( 1, comp_elem.count );
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
    for( const std::vector<quality_requirement> &elem : quality_comps ) {
        bool line_found = false;
        for( const quality_requirement &comp_elem : elem ) {
            if( line_found || comp_elem.count <= 0 ) {
                break;
            }
            const quality_id tool_qual = comp_elem.type;
            const int qual_level = comp_elem.level;
            for( auto it = requirement_map.begin(); it != requirement_map.end(); ) {
                tripoint pos_here = std::get<0>( *it );
                itype_id item_here = std::get<1>( *it );
                item test_item = item( item_here, calendar::turn_zero );
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
    for( const std::tuple<tripoint, itype_id, int> &elem : final_map ) {
        add_msg_debug( "%s is fetching %s from x: %d y: %d ", p.disp_name(),
                       std::get<1>( elem ).str(), std::get<0>( elem ).x, std::get<0>( elem ).y );
    }
    return final_map;
}

static bool construction_activity( player &p, const zone_data * /*zone*/, const tripoint &src_loc,
                                   const activity_reason_info &act_info,
                                   const activity_id &activity_to_restore )
{
    // the actual desired construction
    if( !act_info.con_idx ) {
        debugmsg( "no construction selected" );
        return false;
    }
    const construction &built_chosen = act_info.con_idx->obj();
    std::list<item> used;
    // create the partial construction struct
    partial_con pc;
    pc.id = built_chosen.id;
    map &here = get_map();
    // Set the trap that has the examine function
    if( here.tr_at( src_loc ).is_null() ) {
        here.trap_set( src_loc, tr_unfinished_construction );
    }
    // Use up the components
    for( const std::vector<item_comp> &it : built_chosen.requirements->get_components() ) {
        std::list<item> tmp = p.consume_items( it, 1, is_crafting_component );
        used.splice( used.end(), tmp );
    }
    pc.components = used;
    here.partial_con_set( src_loc, pc );
    for( const std::vector<tool_comp> &it : built_chosen.requirements->get_tools() ) {
        p.consume_tools( it );
    }
    p.backlog.push_front( activity_to_restore );
    p.assign_activity( ACT_BUILD );
    p.activity.placement = here.getabs( src_loc );
    return true;
}

static bool tidy_activity( player &p, const tripoint &src_loc,
                           const activity_id &activity_to_restore, const int distance = ACTIVITY_SEARCH_DISTANCE )
{
    zone_manager &mgr = zone_manager::get_manager();
    map &here = get_map();
    tripoint loot_abspos = here.getabs( src_loc );
    tripoint loot_src_lot;
    const auto &zone_src_set = mgr.get_near( zone_type_LOOT_UNSORTED, loot_abspos, distance );
    if( !zone_src_set.empty() ) {
        const std::vector<tripoint> &zone_src_sorted = get_sorted_tiles_by_distance(
                    loot_abspos, zone_src_set );
        // Find the nearest unsorted zone to dump objects at
        for( const tripoint &src_elem : zone_src_sorted ) {
            if( !here.can_put_items_ter_furn( here.getlocal( src_elem ) ) ) {
                continue;
            }
            loot_src_lot = here.getlocal( src_elem );
            break;
        }
    }
    if( loot_src_lot == tripoint_zero ) {
        return false;
    }
    map_stack items_there = here.i_at( src_loc );
    vehicle *dest_veh;
    int dest_part;
    if( const cata::optional<vpart_reference> vp = here.veh_at(
                loot_src_lot ).part_with_feature( "CARGO",
                        false ) ) {
        dest_veh = &vp->vehicle();
        dest_part = vp->part_index();
    } else {
        dest_veh = nullptr;
        dest_part = -1;
    }
    for( item &it : items_there ) {
        if( it.has_var( "activity_var" ) && it.get_var( "activity_var", "" ) == p.name ) {
            move_item( p, it, it.count(), src_loc, loot_src_lot, dest_veh, dest_part,
                       activity_to_restore );
            break;
        }
    }
    // we are adjacent to an unsorted zone, we came here to just drop items we are carrying
    if( mgr.has( zone_type_LOOT_UNSORTED, here.getabs( src_loc ) ) ) {
        for( item *inv_elem : p.inv_dump() ) {
            if( inv_elem->has_var( "activity_var" ) ) {
                inv_elem->erase_var( "activity_var" );
                put_into_vehicle_or_drop( p, item_drop_reason::deliberate, { *inv_elem }, src_loc );
                p.i_rem( inv_elem );
            }
        }
    }
    return true;
}

static bool fetch_activity( player &p, const tripoint &src_loc,
                            const activity_id &activity_to_restore, const int distance = ACTIVITY_SEARCH_DISTANCE )
{
    map &here = get_map();
    if( !here.can_put_items_ter_furn( here.getlocal( p.backlog.front().coords.back() ) ) ) {
        return false;
    }
    const std::vector<std::tuple<tripoint, itype_id, int>> mental_map_2 = requirements_map( p,
            distance );
    int pickup_count = 1;
    map_stack items_there = here.i_at( src_loc );
    vehicle *src_veh = nullptr;
    int src_part = 0;
    if( const cata::optional<vpart_reference> vp = here.veh_at( src_loc ).part_with_feature( "CARGO",
            false ) ) {
        src_veh = &vp->vehicle();
        src_part = vp->part_index();
    }
    const units::volume volume_allowed = p.volume_capacity() - p.volume_carried();
    const units::mass weight_allowed = p.weight_capacity() - p.weight_carried();
    // TODO: vehicle_stack and map_stack into one loop.
    if( src_veh ) {
        for( auto &veh_elem : src_veh->get_items( src_part ) ) {
            for( auto elem : mental_map_2 ) {
                if( std::get<0>( elem ) == src_loc && veh_elem.typeId() == std::get<1>( elem ) ) {
                    if( !p.backlog.empty() && p.backlog.front().id() == ACT_MULTIPLE_CONSTRUCTION ) {
                        move_item( p, veh_elem, veh_elem.count_by_charges() ? std::get<2>( elem ) : 1, src_loc,
                                   here.getlocal( p.backlog.front().coords.back() ), src_veh, src_part, activity_to_restore );
                        return true;
                    }
                }
            }
        }
    }
    for( auto item_iter = items_there.begin(); item_iter != items_there.end(); item_iter++ ) {
        item &it = *item_iter;
        for( auto elem : mental_map_2 ) {
            if( std::get<0>( elem ) == src_loc && it.typeId() == std::get<1>( elem ) ) {
                // construction/crafting tasks want the required item moved near the work spot.
                if( !p.backlog.empty() && p.backlog.front().id() == ACT_MULTIPLE_CONSTRUCTION ) {
                    move_item( p, it, it.count_by_charges() ? std::get<2>( elem ) : 1, src_loc,
                               here.getlocal( p.backlog.front().coords.back() ), src_veh, src_part, activity_to_restore );
                    return true;
                    // other tasks want the tool picked up
                } else if( !p.backlog.empty() && ( p.backlog.front().id() == ACT_MULTIPLE_FARM ||
                                                   p.backlog.front().id() == ACT_MULTIPLE_CHOP_PLANKS ||
                                                   p.backlog.front().id() == ACT_VEHICLE_DECONSTRUCTION ||
                                                   p.backlog.front().id() == ACT_VEHICLE_REPAIR ||
                                                   p.backlog.front().id() == ACT_MULTIPLE_BUTCHER ||
                                                   p.backlog.front().id() == ACT_MULTIPLE_CHOP_TREES ||
                                                   p.backlog.front().id() == ACT_MULTIPLE_FISH ||
                                                   p.backlog.front().id() == ACT_MULTIPLE_MINE ) ) {
                    if( it.volume() > volume_allowed || it.weight() > weight_allowed ) {
                        continue;
                    }
                    item leftovers = it;
                    if( pickup_count != 1 && it.count_by_charges() ) {
                        // Reinserting leftovers happens after item removal to avoid stacking issues.
                        leftovers.charges = it.charges - pickup_count;
                        if( leftovers.charges > 0 ) {
                            it.charges = pickup_count;
                        }
                    } else {
                        leftovers.charges = 0;
                    }
                    it.set_var( "activity_var", p.name );
                    p.i_add( it );
                    if( p.is_npc() ) {
                        if( pickup_count == 1 ) {
                            const std::string item_name = it.tname();
                            add_msg( _( "%1$s picks up a %2$s." ), p.disp_name(), item_name );
                        } else {
                            add_msg( _( "%s picks up several items." ),  p.disp_name() );
                        }
                    }
                    items_there.erase( item_iter );
                    // If we didn't pick up a whole stack, put the remainder back where it came from.
                    if( leftovers.charges > 0 ) {
                        here.add_item_or_charges( src_loc, leftovers );
                    }
                    return true;
                }
            }
        }
    }
    // if we got here, then the fetch failed for reasons that weren't predicted before setting it.
    // nothing was moved or picked up, and nothing can be moved or picked up
    // so call the whole thing off to stop it looping back to this point ad nauseum.
    p.set_moves( 0 );
    p.activity = player_activity();
    p.backlog.clear();
    return false;
}

static bool butcher_corpse_activity( player &p, const tripoint &src_loc,
                                     const do_activity_reason &reason )
{
    map &here = get_map();
    map_stack items = here.i_at( src_loc );
    for( item &elem : items ) {
        if( elem.is_corpse() && !elem.has_var( "activity_var" ) ) {
            const mtype corpse = *elem.get_mtype();
            if( corpse.size >= creature_size::medium && reason != do_activity_reason::NEEDS_BIG_BUTCHERING ) {
                continue;
            }
            elem.set_var( "activity_var", p.name );
            p.assign_activity( ACT_BUTCHER_FULL, 0, true );
            p.activity.targets.emplace_back( map_cursor( src_loc ), &elem );
            p.activity.placement = here.getabs( src_loc );
            return true;
        }
    }
    return false;
}

static bool chop_plank_activity( player &p, const tripoint &src_loc )
{
    item *best_qual = p.best_quality_item( qual_AXE );
    if( !best_qual ) {
        return false;
    }
    if( best_qual->type->can_have_charges() ) {
        p.consume_charges( *best_qual, best_qual->type->charges_to_use() );
    }
    map &here = get_map();
    for( item &i : here.i_at( src_loc ) ) {
        if( i.typeId() == itype_log ) {
            here.i_rem( src_loc, &i );
            int moves = to_moves<int>( 20_minutes );
            p.add_msg_if_player( _( "You cut the log into planks." ) );
            p.assign_activity( ACT_CHOP_PLANKS, moves, -1 );
            p.activity.placement = here.getabs( src_loc );
            return true;
        }
    }
    return false;
}

void activity_on_turn_move_loot( player_activity &act, player &p )
{
    enum activity_stage : int {
        //Initial stage
        INIT = 0,
        //Think about what to do first: choose destination
        THINK,
        //Do activity
        DO,
    };

    int &stage = act.index;
    //Prepare activity stage
    if( stage < 0 ) {
        stage = INIT;
        //num_processed
        act.values.push_back( 0 );
    }
    int &num_processed = act.values[ 0 ];

    map &here = get_map();
    const tripoint abspos = here.getabs( p.pos() );
    auto &mgr = zone_manager::get_manager();
    if( here.check_vehicle_zones( here.get_abs_sub().z ) ) {
        mgr.cache_vzones();
    }

    if( stage == INIT ) {
        act.coord_set = mgr.get_near( zone_type_LOOT_UNSORTED, abspos, ACTIVITY_SEARCH_DISTANCE );
        stage = THINK;
    }

    if( stage == THINK ) {
        //initialize num_processed
        num_processed = 0;
        const auto &src_set = act.coord_set;
        // sort source tiles by distance
        const auto &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );

        for( const tripoint &src : src_sorted ) {
            act.placement = src;
            act.coord_set.erase( src );

            const auto &src_loc = here.getlocal( src );
            if( !here.inbounds( src_loc ) ) {
                if( !here.inbounds( p.pos() ) ) {
                    // p is implicitly an NPC that has been moved off the map, so reset the activity
                    // and unload them
                    p.cancel_activity();
                    p.assign_activity( ACT_MOVE_LOOT );
                    p.set_moves( 0 );
                    g->reload_npcs();
                    return;
                }
                std::vector<tripoint> route;
                route = here.route( p.pos(), src_loc, p.get_pathfinding_settings(),
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
            if( mgr.has( zone_type_LOOT_IGNORE, src ) ||
                here.get_field( src_loc, fd_fire ) != nullptr ||
                !here.can_put_items_ter_furn( src_loc ) ) {
                continue;
            }

            //nothing to sort?
            const cata::optional<vpart_reference> vp = here.veh_at( src_loc ).part_with_feature( "CARGO",
                    false );
            if( ( !vp || vp->vehicle().get_items( vp->part_index() ).empty() )
                && here.i_at( src_loc ).empty() ) {
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
                if( here.passable( src_loc ) ) {
                    route = here.route( p.pos(), src_loc, p.get_pathfinding_settings(),
                                        p.get_path_avoid() );
                } else {
                    // impassable source tile (locker etc.),
                    // get route to nearest adjacent tile instead
                    route = route_adjacent( p, src_loc );
                    adjacent = true;
                }

                // check if we found path to source / adjacent tile
                if( route.empty() ) {
                    add_msg( m_info, _( "%s can't reach the source tile.  Try to sort out loot without a cart." ),
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
        const tripoint &src_loc = here.getlocal( src );

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
        if( const cata::optional<vpart_reference> vp = here.veh_at( src_loc ).part_with_feature( "CARGO",
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
        for( item &it : here.i_at( src_loc ) ) {
            items.push_back( std::make_pair( &it, false ) );
        }

        //Skip items that have already been processed
        for( auto it = items.begin() + num_processed; it < items.end(); ++it ) {
            ++num_processed;
            item &thisitem = *it->first;

            // skip unpickable liquid
            if( thisitem.made_of_from_type( phase_id::LIQUID ) ) {
                continue;
            }

            // skip favorite items in ignore favorite zones
            if( thisitem.is_favorite && mgr.has( zone_type_LOOT_IGNORE_FAVORITES, src ) ) {
                continue;
            }

            // Only if it's from a vehicle do we use the vehicle source location information.
            vehicle *this_veh = it->second ? src_veh : nullptr;
            const int this_part = it->second ? src_part : -1;

            const zone_type_id id = mgr.get_near_zone_type_for_item( thisitem, abspos,
                                    ACTIVITY_SEARCH_DISTANCE );

            // checks whether the item is already on correct loot zone or not
            // if it is, we can skip such item, if not we move the item to correct pile
            // think empty bag on food pile, after you ate the content
            if( id != zone_type_id( "LOOT_CUSTOM" ) && mgr.has( id, src ) ) {
                continue;
            }

            if( id == zone_type_id( "LOOT_CUSTOM" ) && mgr.custom_loot_has( src, &thisitem ) ) {
                continue;
            }

            const std::unordered_set<tripoint> &dest_set = mgr.get_near( id, abspos, ACTIVITY_SEARCH_DISTANCE,
                    &thisitem );
            for( const tripoint &dest : dest_set ) {
                const tripoint &dest_loc = here.getlocal( dest );

                //Check destination for cargo part
                if( const cata::optional<vpart_reference> vp = here.veh_at( dest_loc ).part_with_feature( "CARGO",
                        false ) ) {
                    dest_veh = &vp->vehicle();
                    dest_part = vp->part_index();
                } else {
                    dest_veh = nullptr;
                    dest_part = -1;
                }

                // skip tiles with inaccessible furniture, like filled charcoal kiln
                if( !here.can_put_items_ter_furn( dest_loc ) ||
                    static_cast<int>( here.i_at( dest_loc ).size() ) >= MAX_ITEM_IN_SQUARE ) {
                    continue;
                }

                units::volume free_space;
                // if there's a vehicle with space do not check the tile beneath
                if( dest_veh ) {
                    free_space = dest_veh->free_volume( dest_part );
                } else {
                    free_space = here.free_volume( dest_loc );
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
    add_msg( m_info, _( "%s sorted out every item possible." ), p.disp_name( false, true ) );
    if( p.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &p );
        guy->revert_after_activity();
    }
    p.activity.set_to_null();
}

static int chop_moves( player &p, item *it )
{
    // quality of tool
    const int quality = it->get_quality( qual_AXE );

    // attribute; regular tools - based on STR, powered tools - based on DEX
    const int attr = it->has_flag( flag_POWERED ) ? p.dex_cur : p.str_cur;

    int moves = to_moves<int>( time_duration::from_minutes( 60 - attr ) / std::pow( 2, quality - 1 ) );
    const int helpersize = p.get_num_crafting_helpers( 3 );
    moves *= ( 1.0f - ( helpersize / 10.0f ) );
    return moves;
}

static bool mine_activity( player &p, const tripoint &src_loc )
{
    std::vector<item *> mining_inv = p.items_with( []( const item & itm ) {
        return ( itm.has_flag( flag_DIG_TOOL ) && !itm.type->can_use( "JACKHAMMER" ) ) ||
               ( itm.type->can_use( "JACKHAMMER" ) && itm.ammo_sufficient() );
    } );
    map &here = get_map();
    if( mining_inv.empty() || p.is_mounted() || p.is_underwater() || here.veh_at( src_loc ) ||
        !here.has_flag( "MINEABLE", src_loc ) || p.has_effect( effect_incorporeal ) ) {
        return false;
    }
    item *chosen_item = nullptr;
    bool powered = false;
    // is it a pickaxe or jackhammer?
    for( item *elem : mining_inv ) {
        if( chosen_item == nullptr ) {
            chosen_item = elem;
            if( elem->type->can_use( "JACKHAMMER" ) ) {
                powered = true;
            }
        } else {
            // prioritise powered tools
            if( chosen_item->type->can_use( "PICKAXE" ) && elem->type->can_use( "JACKHAMMER" ) ) {
                chosen_item = elem;
                powered = true;
                break;
            }
        }
    }
    if( chosen_item == nullptr ) {
        return false;
    }
    int moves = to_moves<int>( powered ? 30_minutes : 20_minutes );
    if( !powered ) {
        moves += ( ( MAX_STAT + 4 ) - std::min( p.str_cur, MAX_STAT ) ) * to_moves<int>( 5_minutes );
    }
    if( here.move_cost( src_loc ) == 2 ) {
        // We're breaking up some flat surface like pavement, which is much easier
        moves /= 2;
    }
    p.assign_activity( powered ? ACT_JACKHAMMER : ACT_PICKAXE, moves );
    p.activity.targets.push_back( item_location( p, chosen_item ) );
    p.activity.placement = here.getabs( src_loc );
    return true;

}

static bool chop_tree_activity( player &p, const tripoint &src_loc )
{
    item *best_qual = p.best_quality_item( qual_AXE );
    if( !best_qual ) {
        return false;
    }
    int moves = chop_moves( p, best_qual );
    if( best_qual->type->can_have_charges() ) {
        p.consume_charges( *best_qual, best_qual->type->charges_to_use() );
    }
    map &here = get_map();
    const ter_id ter = here.ter( src_loc );
    if( here.has_flag( flag_TREE, src_loc ) ) {
        p.assign_activity( ACT_CHOP_TREE, moves, -1, p.get_item_position( best_qual ) );
        p.activity.placement = here.getabs( src_loc );
        return true;
    } else if( ter == t_trunk || ter == t_stump ) {
        p.assign_activity( ACT_CHOP_LOGS, moves, -1, p.get_item_position( best_qual ) );
        p.activity.placement = here.getabs( src_loc );
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

static zone_type_id get_zone_for_act( const tripoint &src_loc, const zone_manager &mgr,
                                      const activity_id &act_id )
{
    zone_type_id ret = zone_type_id( "" );
    if( act_id == ACT_VEHICLE_DECONSTRUCTION ) {
        ret = zone_type_VEHICLE_DECONSTRUCT;
    }
    if( act_id == ACT_VEHICLE_REPAIR ) {
        ret = zone_type_VEHICLE_REPAIR;
    }
    if( act_id == ACT_MULTIPLE_CHOP_TREES ) {
        ret = zone_type_CHOP_TREES;
    }
    if( act_id == ACT_MULTIPLE_CONSTRUCTION ) {
        ret = zone_type_CONSTRUCTION_BLUEPRINT;
    }
    if( act_id == ACT_MULTIPLE_FARM ) {
        ret = zone_type_FARM_PLOT;
    }
    if( act_id == ACT_MULTIPLE_BUTCHER ) {
        ret = zone_type_LOOT_CORPSE;
    }
    if( act_id == ACT_MULTIPLE_CHOP_PLANKS ) {
        ret = zone_type_LOOT_WOOD;
    }
    if( act_id == ACT_MULTIPLE_FISH ) {
        ret = zone_type_FISHING_SPOT;
    }
    if( act_id == ACT_MULTIPLE_MINE ) {
        ret = zone_type_MINING;
    }
    if( src_loc != tripoint_zero && act_id == ACT_FETCH_REQUIRED ) {
        const zone_data *zd = mgr.get_zone_at( get_map().getabs( src_loc ) );
        if( zd ) {
            ret = zd->get_type();
        }
    }
    return ret;
}

/** Determine all locations for this generic activity */
/** Returns locations */
static std::unordered_set<tripoint> generic_multi_activity_locations( player &p,
        const activity_id &act_id )
{
    bool dark_capable = false;
    std::unordered_set<tripoint> src_set;

    zone_manager &mgr = zone_manager::get_manager();
    const tripoint localpos = p.pos();
    map &here = get_map();
    const tripoint abspos = here.getabs( localpos );
    if( act_id == ACT_TIDY_UP ) {
        dark_capable = true;
        tripoint unsorted_spot;
        std::unordered_set<tripoint> unsorted_set = mgr.get_near( zone_type_LOOT_UNSORTED, abspos,
                ACTIVITY_SEARCH_DISTANCE );
        if( !unsorted_set.empty() ) {
            unsorted_spot = here.getlocal( random_entry( unsorted_set ) );
        }
        bool found_one_point = false;
        bool found_route = true;
        for( const tripoint &elem : here.points_in_radius( localpos,
                ACTIVITY_SEARCH_DISTANCE ) ) {
            // There's no point getting the entire list of all items to tidy up now.
            // the activity will run again after pathing to the first tile anyway.
            // tidy up activity has no requirements that will discount a square and
            // have the requirement to skip and scan the next one, ( other than checking path )
            // shortcircuiting the need to scan the entire map continuously can improve performance
            // especially if NPCs have a backlog of moves or there is a lot of them
            if( !found_route ) {
                found_route = true;
                continue;
            }
            if( found_one_point ) {
                break;
            }
            for( const item &stack_elem : here.i_at( elem ) ) {
                if( stack_elem.has_var( "activity_var" ) && stack_elem.get_var( "activity_var", "" ) == p.name ) {
                    const furn_t &f = here.furn( elem ).obj();
                    if( !f.has_flag( flag_PLANT ) ) {
                        src_set.insert( here.getabs( elem ) );
                        found_one_point = true;
                        // only check for a valid path, as that is all that is needed to tidy something up.
                        if( square_dist( p.pos(), elem ) > 1 ) {
                            std::vector<tripoint> route = route_adjacent( p, elem );
                            if( route.empty() ) {
                                found_route = false;
                            }
                        }
                        break;
                    }
                }
            }
        }
        if( src_set.empty() && unsorted_spot != tripoint_zero ) {
            for( const item *inv_elem : p.inv_dump() ) {
                if( inv_elem->has_var( "activity_var" ) ) {
                    // we've gone to tidy up all the things lying around, now tidy up the things we picked up.
                    src_set.insert( here.getabs( unsorted_spot ) );
                    break;
                }
            }
        }
    } else if( act_id != ACT_FETCH_REQUIRED ) {
        zone_type_id zone_type = get_zone_for_act( tripoint_zero, mgr, act_id );
        src_set = mgr.get_near( zone_type_id( zone_type ), abspos, ACTIVITY_SEARCH_DISTANCE );
        // multiple construction will form a list of targets based on blueprint zones and unfinished constructions
        if( act_id == ACT_MULTIPLE_CONSTRUCTION ) {
            for( const tripoint &elem : here.points_in_radius( localpos, ACTIVITY_SEARCH_DISTANCE ) ) {
                partial_con *pc = here.partial_con_at( elem );
                if( pc ) {
                    src_set.insert( here.getabs( elem ) );
                }
            }
            // farming activities encompass tilling, planting, harvesting.
        } else if( act_id == ACT_MULTIPLE_FARM ) {
            dark_capable = true;
        }
    } else {
        dark_capable = true;
        // get the right zones for the items in the requirements.
        // we previously checked if the items are nearby before we set the fetch task
        // but we will check again later, to be sure nothings changed.
        std::vector<std::tuple<tripoint, itype_id, int>> mental_map = requirements_map( p,
                ACTIVITY_SEARCH_DISTANCE );
        for( const auto &elem : mental_map ) {
            const tripoint &elem_point = std::get<0>( elem );
            src_set.insert( here.getabs( elem_point ) );
        }
    }
    // prune the set to remove tiles that are never gonna work out.
    const bool pre_dark_check = src_set.empty();
    for( auto it2 = src_set.begin(); it2 != src_set.end(); ) {
        // remove dangerous tiles
        const tripoint set_pt = here.getlocal( *it2 );
        if( here.dangerous_field_at( set_pt ) ) {
            it2 = src_set.erase( it2 );
            // remove tiles in darkness, if we aren't lit-up ourselves
        } else if( !dark_capable && p.fine_detail_vision_mod( set_pt ) > 4.0 ) {
            it2 = src_set.erase( it2 );
        } else if( act_id == ACT_MULTIPLE_FISH ) {
            const ter_id terrain_id = here.ter( set_pt );
            if( !terrain_id.obj().has_flag( TFLAG_DEEP_WATER ) ) {
                it2 = src_set.erase( it2 );
            } else {
                ++it2;
            }
        } else {
            ++it2;
        }
    }
    const bool post_dark_check = src_set.empty();
    if( !pre_dark_check && post_dark_check ) {
        p.add_msg_if_player( m_info, _( "It is too dark to do this activity." ) );
    }
    return src_set;
}

/** Check if this activity can not be done immediately because it has some requirements */
static requirement_check_result generic_multi_activity_check_requirement( player &p,
        const activity_id &act_id, activity_reason_info &act_info,
        const tripoint &src, const tripoint &src_loc, const std::unordered_set<tripoint> &src_set,
        const bool check_only = false )
{
    map &here = get_map();
    const tripoint abspos = here.getabs( p.pos() );
    zone_manager &mgr = zone_manager::get_manager();

    bool &can_do_it = act_info.can_do;
    const do_activity_reason &reason = act_info.reason;
    const zone_data *zone = mgr.get_zone_at( src, get_zone_for_act( src_loc, mgr, act_id ) );

    const bool needs_to_be_in_zone = act_id == ACT_FETCH_REQUIRED ||
                                     act_id == ACT_MULTIPLE_FARM ||
                                     act_id == ACT_MULTIPLE_BUTCHER ||
                                     act_id == ACT_MULTIPLE_CHOP_PLANKS ||
                                     act_id == ACT_MULTIPLE_CHOP_TREES ||
                                     act_id == ACT_VEHICLE_DECONSTRUCTION ||
                                     act_id == ACT_VEHICLE_REPAIR ||
                                     act_id == ACT_MULTIPLE_FISH ||
                                     act_id == ACT_MULTIPLE_MINE ||
                                     ( act_id == ACT_MULTIPLE_CONSTRUCTION &&
                                       !here.partial_con_at( src_loc ) );
    // some activities require the target tile to be part of a zone.
    // tidy up activity doesn't - it wants things that may not be in a zone already - things that may have been left lying around.
    if( needs_to_be_in_zone && !zone ) {
        can_do_it = false;
        return requirement_check_result::SKIP_LOCATION;
    }
    if( can_do_it ) {
        return requirement_check_result::CAN_DO_LOCATION;
    }
    if( reason == do_activity_reason::DONT_HAVE_SKILL ||
        reason == do_activity_reason::NO_ZONE ||
        reason == do_activity_reason::ALREADY_DONE ||
        reason == do_activity_reason::BLOCKING_TILE ||
        reason == do_activity_reason::UNKNOWN_ACTIVITY ) {
        // we can discount this tile, the work can't be done.
        if( reason == do_activity_reason::DONT_HAVE_SKILL ) {
            p.add_msg_if_player( m_info, _( "You don't have the skill for this task." ) );
        } else if( reason == do_activity_reason::BLOCKING_TILE ) {
            p.add_msg_if_player( m_info, _( "There is something blocking the location for this task." ) );
        }
        return requirement_check_result::SKIP_LOCATION;
    } else if( reason == do_activity_reason::NO_COMPONENTS ||
               reason == do_activity_reason::NO_COMPONENTS_PREREQ ||
               reason == do_activity_reason::NO_COMPONENTS_PREREQ_2 ||
               reason == do_activity_reason::NEEDS_PLANTING ||
               reason == do_activity_reason::NEEDS_TILLING ||
               reason == do_activity_reason::NEEDS_CHOPPING ||
               reason == do_activity_reason::NEEDS_BUTCHERING ||
               reason == do_activity_reason::NEEDS_BIG_BUTCHERING ||
               reason == do_activity_reason::NEEDS_VEH_DECONST ||
               reason == do_activity_reason::NEEDS_VEH_REPAIR ||
               reason == do_activity_reason::NEEDS_TREE_CHOPPING ||
               reason == do_activity_reason::NEEDS_FISHING || reason == do_activity_reason::NEEDS_MINING ) {
        // we can do it, but we need to fetch some stuff first
        // before we set the task to fetch components - is it even worth it? are the components anywhere?
        requirement_id what_we_need;
        std::vector<tripoint> loot_zone_spots;
        std::vector<tripoint> combined_spots;
        for( const tripoint &elem : mgr.get_point_set_loot( abspos, ACTIVITY_SEARCH_DISTANCE,
                p.is_npc() ) ) {
            loot_zone_spots.push_back( elem );
            combined_spots.push_back( elem );
        }
        for( const tripoint &elem : here.points_in_radius( src_loc, PICKUP_RANGE - 1 ) ) {
            combined_spots.push_back( elem );
        }
        add_basecamp_storage_to_loot_zone_list( mgr, src_loc, p, loot_zone_spots, combined_spots );

        if( ( reason == do_activity_reason::NO_COMPONENTS ||
              reason == do_activity_reason::NO_COMPONENTS_PREREQ ||
              reason == do_activity_reason::NO_COMPONENTS_PREREQ_2 ) &&
            act_id == ACT_MULTIPLE_CONSTRUCTION ) {
            if( !act_info.con_idx ) {
                debugmsg( "no construction selected" );
                return requirement_check_result::SKIP_LOCATION;
            }
            // its a construction and we need the components.
            const construction &built_chosen = act_info.con_idx->obj();
            what_we_need = built_chosen.requirements;
        } else if( reason == do_activity_reason::NEEDS_VEH_DECONST ||
                   reason == do_activity_reason::NEEDS_VEH_REPAIR ) {
            const vehicle *veh = veh_pointer_or_null( here.veh_at( src_loc ) );
            // we already checked this in can_do_activity() but check again just incase.
            if( !veh ) {
                p.activity_vehicle_part_index = 1;
                return requirement_check_result::SKIP_LOCATION;
            }
            const vpart_info &vpinfo = veh->part_info( p.activity_vehicle_part_index );
            requirement_data reqs;
            if( reason == do_activity_reason::NEEDS_VEH_DECONST ) {
                reqs = vpinfo.removal_requirements();
            } else if( reason == do_activity_reason::NEEDS_VEH_REPAIR ) {
                reqs = vpinfo.repair_requirements();
            }
            const std::string ran_str = random_string( 10 );
            const requirement_id req_id( ran_str );
            requirement_data::save_requirement( reqs, req_id );
            what_we_need = req_id;
        } else if( reason == do_activity_reason::NEEDS_MINING ) {
            what_we_need = requirement_id( "mining_standard" );
        } else if( reason == do_activity_reason::NEEDS_TILLING ||
                   reason == do_activity_reason::NEEDS_PLANTING ||
                   reason == do_activity_reason::NEEDS_CHOPPING ||
                   reason == do_activity_reason::NEEDS_BUTCHERING ||
                   reason == do_activity_reason::NEEDS_BIG_BUTCHERING ||
                   reason == do_activity_reason::NEEDS_TREE_CHOPPING ||
                   reason == do_activity_reason::NEEDS_FISHING ) {
            std::vector<std::vector<item_comp>> requirement_comp_vector;
            std::vector<std::vector<quality_requirement>> quality_comp_vector;
            std::vector<std::vector<tool_comp>> tool_comp_vector;
            if( reason == do_activity_reason::NEEDS_TILLING ) {
                quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( qual_DIG, 1, 1 ) } );
            } else if( reason == do_activity_reason::NEEDS_CHOPPING ||
                       reason == do_activity_reason::NEEDS_TREE_CHOPPING ) {
                quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( qual_AXE, 1, 1 ) } );
            } else if( reason == do_activity_reason::NEEDS_PLANTING ) {
                requirement_comp_vector.push_back( std::vector<item_comp> { item_comp( itype_id( dynamic_cast<const plot_options &>
                                                   ( zone->get_options() ).get_seed() ), 1 )
                                                                          } );
            } else if( reason == do_activity_reason::NEEDS_BUTCHERING ||
                       reason == do_activity_reason::NEEDS_BIG_BUTCHERING ) {
                quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( qual_BUTCHER, 1, 1 ) } );
                if( reason == do_activity_reason::NEEDS_BIG_BUTCHERING ) {
                    quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( qual_SAW_M, 1, 1 ), quality_requirement( qual_SAW_W, 1, 1 ) } );
                }

            } else if( reason == do_activity_reason::NEEDS_FISHING ) {
                quality_comp_vector.push_back( std::vector<quality_requirement> {quality_requirement( qual_FISHING, 1, 1 )} );
            }
            // ok, we need a shovel/hoe/axe/etc.
            // this is an activity that only requires this one tool, so we will fetch and wield it.
            requirement_data reqs_data = requirement_data( tool_comp_vector, quality_comp_vector,
                                         requirement_comp_vector );
            const std::string ran_str = random_string( 10 );
            const requirement_id req_id( ran_str );
            requirement_data::save_requirement( reqs_data, req_id );
            what_we_need = req_id;
        }
        bool tool_pickup = reason == do_activity_reason::NEEDS_TILLING ||
                           reason == do_activity_reason::NEEDS_PLANTING ||
                           reason == do_activity_reason::NEEDS_CHOPPING ||
                           reason == do_activity_reason::NEEDS_BUTCHERING ||
                           reason == do_activity_reason::NEEDS_BIG_BUTCHERING ||
                           reason == do_activity_reason::NEEDS_TREE_CHOPPING ||
                           reason == do_activity_reason::NEEDS_VEH_DECONST ||
                           reason == do_activity_reason::NEEDS_VEH_REPAIR ||
                           reason == do_activity_reason::NEEDS_MINING;
        // is it even worth fetching anything if there isn't enough nearby?
        if( !are_requirements_nearby( tool_pickup ? loot_zone_spots : combined_spots, what_we_need, p,
                                      act_id, tool_pickup, src_loc ) ) {
            p.add_msg_if_player( m_info, _( "The required items are not available to complete this task." ) );
            if( reason == do_activity_reason::NEEDS_VEH_DECONST ||
                reason == do_activity_reason::NEEDS_VEH_REPAIR ) {
                p.activity_vehicle_part_index = -1;
            }
            return requirement_check_result::SKIP_LOCATION;
        } else {
            if( !check_only ) {
                p.backlog.push_front( act_id );
                p.assign_activity( ACT_FETCH_REQUIRED );
                player_activity &act_prev = p.backlog.front();
                act_prev.str_values.push_back( what_we_need.str() );
                act_prev.values.push_back( static_cast<int>( reason ) );
                // come back here after successfully fetching your stuff
                if( act_prev.coords.empty() ) {
                    std::vector<tripoint> local_src_set;
                    for( const tripoint &elem : src_set ) {
                        local_src_set.push_back( here.getlocal( elem ) );
                    }
                    std::vector<tripoint> candidates;
                    for( const tripoint &point_elem : here.points_in_radius( src_loc, PICKUP_RANGE - 1 ) ) {
                        // we don't want to place the components where they could interfere with our ( or someone else's ) construction spots
                        if( !p.sees( point_elem ) || ( std::find( local_src_set.begin(), local_src_set.end(),
                                                       point_elem ) != local_src_set.end() ) || !here.can_put_items_ter_furn( point_elem ) ) {
                            continue;
                        }
                        candidates.push_back( point_elem );
                    }
                    if( candidates.empty() ) {
                        p.activity = player_activity();
                        p.backlog.clear();
                        check_npc_revert( p );
                        return requirement_check_result::SKIP_LOCATION;
                    }
                    act_prev.coords.push_back( here.getabs( candidates[std::max( 0,
                                                                      static_cast<int>( candidates.size() / 2 ) )] ) );
                }
                act_prev.placement = src;
            }
            return requirement_check_result::RETURN_EARLY;
        }
    }
    return requirement_check_result::SKIP_LOCATION;
}

/** Do activity at this location */
/** Returns true if this multi activity may be processed further */
static bool generic_multi_activity_do( player &p, const activity_id &act_id,
                                       const activity_reason_info &act_info,
                                       const tripoint &src, const tripoint &src_loc )
{
    // If any of the following activities return without processing
    // then they MUST return true here, to stop infinite loops.
    zone_manager &mgr = zone_manager::get_manager();

    const do_activity_reason &reason = act_info.reason;
    const zone_data *zone = mgr.get_zone_at( src, get_zone_for_act( src_loc, mgr, act_id ) );
    map &here = get_map();
    // something needs to be done, now we are there.
    // it was here earlier, in the space of one turn, maybe it got harvested by someone else.
    if( reason == do_activity_reason::NEEDS_HARVESTING &&
        here.has_flag_furn( flag_GROWTH_HARVEST, src_loc ) ) {
        iexamine::harvest_plant( p, src_loc, true );
    } else if( reason == do_activity_reason::NEEDS_TILLING && here.has_flag( flag_PLOWABLE, src_loc ) &&
               p.has_quality( qual_DIG, 1 ) && !here.has_furn( src_loc ) ) {
        p.assign_activity( ACT_CHURN, 18000, -1 );
        p.backlog.push_front( act_id );
        p.activity.placement = src;
        return false;
    } else if( reason == do_activity_reason::NEEDS_PLANTING &&
               here.has_flag_ter_or_furn( flag_PLANTABLE, src_loc ) ) {
        std::vector<zone_data> zones = mgr.get_zones( zone_type_FARM_PLOT,
                                       here.getabs( src_loc ) );
        for( const zone_data &zone : zones ) {
            const itype_id seed =
                dynamic_cast<const plot_options &>( zone.get_options() ).get_seed();
            std::vector<item *> seed_inv = p.items_with( [seed]( const item & itm ) {
                return itm.typeId() == itype_id( seed );
            } );
            // we don't have the required seed, even though we should at this point.
            // move onto the next tile, and if need be that will prompt a fetch seeds activity.
            if( seed_inv.empty() ) {
                continue;
            }
            iexamine::plant_seed( p, src_loc, itype_id( seed ) );
            p.backlog.push_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_CHOPPING && p.has_quality( qual_AXE, 1 ) ) {
        if( chop_plank_activity( p, src_loc ) ) {
            p.backlog.push_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_BUTCHERING ||
               reason == do_activity_reason::NEEDS_BIG_BUTCHERING ) {
        if( butcher_corpse_activity( p, src_loc, reason ) ) {
            p.backlog.push_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::CAN_DO_CONSTRUCTION ||
               reason == do_activity_reason::CAN_DO_PREREQ ) {
        if( here.partial_con_at( src_loc ) ) {
            p.backlog.push_front( act_id );
            p.assign_activity( ACT_BUILD );
            p.activity.placement = src;
            return false;
        }
        if( construction_activity( p, zone, src_loc, act_info, act_id ) ) {
            return false;
        }
    } else if( reason == do_activity_reason::CAN_DO_FETCH && act_id == ACT_TIDY_UP ) {
        if( !tidy_activity( p, src_loc, act_id, ACTIVITY_SEARCH_DISTANCE ) ) {
            return false;
        }
    } else if( reason == do_activity_reason::CAN_DO_FETCH && act_id == ACT_FETCH_REQUIRED ) {
        if( fetch_activity( p, src_loc, act_id, ACTIVITY_SEARCH_DISTANCE ) ) {
            if( !p.is_npc() ) {
                // Npcs will automatically start the next thing in the backlog, players need to be manually prompted
                // Because some player activities are necessarily not marked as auto-resume.
                activity_handlers::resume_for_multi_activities( p );
            }
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_TREE_CHOPPING && p.has_quality( qual_AXE, 1 ) ) {
        if( chop_tree_activity( p, src_loc ) ) {
            p.backlog.push_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_FISHING && p.has_quality( qual_FISHING, 1 ) ) {
        p.backlog.push_front( act_id );
        // we don't want to keep repeating the fishing activity, just piggybacking on this functions structure to find requirements.
        p.activity = player_activity();
        item *best_rod = p.best_quality_item( qual_FISHING );
        p.assign_activity( ACT_FISH, to_moves<int>( 5_hours ), 0,
                           0, best_rod->tname() );
        p.activity.targets.push_back( item_location( p, best_rod ) );
        p.activity.coord_set = g->get_fishable_locations( ACTIVITY_SEARCH_DISTANCE, src_loc );
        return false;
    } else if( reason == do_activity_reason::NEEDS_MINING ) {
        // if have enough batteries to continue etc.
        p.backlog.push_front( act_id );
        if( mine_activity( p, src_loc ) ) {
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_VEH_DECONST ) {
        if( vehicle_activity( p, src_loc, p.activity_vehicle_part_index, 'o' ) ) {
            p.backlog.push_front( act_id );
            return false;
        }
        p.activity_vehicle_part_index = -1;
    } else if( reason == do_activity_reason::NEEDS_VEH_REPAIR ) {
        if( vehicle_activity( p, src_loc, p.activity_vehicle_part_index, 'r' ) ) {
            p.backlog.push_front( act_id );
            return false;
        }
        p.activity_vehicle_part_index = -1;
    }
    return true;
}

bool generic_multi_activity_handler( player_activity &act, player &p, bool check_only )
{
    map &here = get_map();
    const tripoint abspos = here.getabs( p.pos() );
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    activity_id activity_to_restore = act.id();
    // Nuke the current activity, leaving the backlog alone
    if( !check_only ) {
        p.activity = player_activity();
    }
    // now we setup the target spots based on which activity is occurring
    // the set of target work spots - potentially after we have fetched required tools.
    std::unordered_set<tripoint> src_set = generic_multi_activity_locations( p, activity_to_restore );
    // now we have our final set of points
    std::vector<tripoint> src_sorted = get_sorted_tiles_by_distance( abspos, src_set );
    // now loop through the work-spot tiles and judge whether its worth traveling to it yet
    // or if we need to fetch something first.
    for( const tripoint &src : src_sorted ) {
        const tripoint &src_loc = here.getlocal( src );
        if( !here.inbounds( src_loc ) && !check_only ) {
            if( !here.inbounds( p.pos() ) ) {
                // p is implicitly an NPC that has been moved off the map, so reset the activity
                // and unload them
                p.assign_activity( activity_to_restore );
                p.set_moves( 0 );
                g->reload_npcs();
                return false;
            }
            const std::vector<tripoint> route = route_adjacent( p, src_loc );
            if( route.empty() ) {
                // can't get there, can't do anything, skip it
                continue;
            }
            p.set_moves( 0 );
            p.set_destination( route, player_activity( activity_to_restore ) );
            return false;
        }
        activity_reason_info act_info = can_do_activity_there( activity_to_restore, p,
                                        src_loc, ACTIVITY_SEARCH_DISTANCE );
        // see activity_handlers.h enum for requirement_check_result
        const requirement_check_result req_res = generic_multi_activity_check_requirement( p,
                activity_to_restore, act_info, src, src_loc, src_set, check_only );
        if( req_res == requirement_check_result::SKIP_LOCATION ) {
            continue;
        } else if( req_res == requirement_check_result::RETURN_EARLY ) {
            return true;
        }

        if( square_dist( p.pos(), src_loc ) > 1 ) {
            std::vector<tripoint> route = route_adjacent( p, src_loc );

            // check if we found path to source / adjacent tile
            if( route.empty() ) {
                check_npc_revert( p );
                continue;
            }
            if( !check_only ) {
                if( p.moves <= 0 ) {
                    // Restart activity and break from cycle.
                    p.assign_activity( activity_to_restore );
                    return true;
                }
                // set the destination and restart activity after player arrives there
                // we don't need to check for safe mode,
                // activity will be restarted only if
                // player arrives on destination tile
                p.set_destination( route, player_activity( activity_to_restore ) );
                return true;
            }
        }
        // we checked if the work spot was in darkness earlier
        // but there is a niche case where the player is in darkness but the work spot is not
        // this can create infinite loops
        // and we can't check player.pos() for darkness before they've traveled to where they are going to be.
        // but now we are here, we check
        if( activity_to_restore != ACT_TIDY_UP &&
            activity_to_restore != ACT_MOVE_LOOT &&
            activity_to_restore != ACT_FETCH_REQUIRED &&
            p.fine_detail_vision_mod( p.pos() ) > 4.0 ) {
            p.add_msg_if_player( m_info, _( "It is too dark to work here." ) );
            return false;
        }
        if( !check_only ) {
            if( !generic_multi_activity_do( p, activity_to_restore, act_info, src, src_loc ) ) {
                // if the activity was successful
                // then a new activity was assigned
                // and the backlog was given the multi-act
                return false;
            }
        } else {
            return true;
        }
    }
    if( !check_only ) {
        if( p.moves <= 0 ) {
            // Restart activity and break from cycle.
            p.assign_activity( activity_to_restore );
            p.activity_vehicle_part_index = -1;
            return false;
        }
        // if we got here, we need to revert otherwise NPC will be stuck in AI Limbo and have a head explosion.
        if( p.backlog.empty() || src_set.empty() ) {
            check_npc_revert( p );
            // tidy up leftover moved parts and tools left lying near the work spots.
            if( player_activity( activity_to_restore ).is_multi_type() ) {
                p.assign_activity( ACT_TIDY_UP );
            }
        }
        p.activity_vehicle_part_index = -1;
    }
    // scanned every location, tried every path.
    return false;
}

static cata::optional<tripoint> find_best_fire( const std::vector<tripoint> &from,
        const tripoint &center )
{
    cata::optional<tripoint> best_fire;
    time_duration best_fire_age = 1_days;
    map &here = get_map();
    for( const tripoint &pt : from ) {
        field_entry *fire = here.get_field( pt, fd_fire );
        if( fire == nullptr || fire->get_field_intensity() > 1 ||
            !here.clear_path( center, pt, PICKUP_RANGE, 1, 100 ) ) {
            continue;
        }
        time_duration fire_age = fire->get_field_age();
        // Refuel only the best fueled fire (if it needs it)
        if( fire_age < best_fire_age ) {
            best_fire = pt;
            best_fire_age = fire_age;
        }
        // If a contained fire exists, ignore any other fires
        if( here.has_flag_furn( TFLAG_FIRE_CONTAINER, pt ) ) {
            return pt;
        }
    }

    return best_fire;
}

static inline bool has_clear_path_to_pickup_items( const tripoint &from, const tripoint &to )
{
    map &here = get_map();
    return here.has_items( to ) &&
           here.accessible_items( to ) &&
           here.clear_path( from, to, PICKUP_RANGE, 1, 100 );
}

static cata::optional<tripoint> find_refuel_spot_zone( const tripoint &center )
{
    const zone_manager &mgr = zone_manager::get_manager();
    map &here = get_map();
    const tripoint center_abs = here.getabs( center );

    const std::unordered_set<tripoint> &tiles_abs_unordered =
        mgr.get_near( zone_type_SOURCE_FIREWOOD, center_abs, PICKUP_RANGE );
    const std::vector<tripoint> &tiles_abs =
        get_sorted_tiles_by_distance( center_abs, tiles_abs_unordered );

    for( const tripoint &tile_abs : tiles_abs ) {
        const tripoint tile = here.getlocal( tile_abs );
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
        return get_map().tr_at( pt ).id == tr_firewood_source
               && has_clear_path_to_pickup_items( center, pt );
    } );

    if( tile != from.end() ) {
        return *tile;
    }

    return {};
}

int get_auto_consume_moves( player &p, const bool food )
{
    if( p.is_npc() ) {
        return 0;
    }

    const tripoint pos = p.pos();
    zone_manager &mgr = zone_manager::get_manager();
    zone_type_id consume_type_zone( "" );
    if( food ) {
        consume_type_zone = zone_type_id( "AUTO_EAT" );
    } else {
        consume_type_zone = zone_type_id( "AUTO_DRINK" );
    }
    map &here = get_map();
    const std::unordered_set<tripoint> &dest_set = mgr.get_near( consume_type_zone, here.getabs( pos ),
            ACTIVITY_SEARCH_DISTANCE );
    if( dest_set.empty() ) {
        return 0;
    }
    for( const tripoint &loc : dest_set ) {
        if( loc.z != p.pos().z ) {
            continue;
        }

        const optional_vpart_position vp = here.veh_at( here.getlocal( loc ) );
        std::vector<item *> items_here;
        if( vp ) {
            vehicle &veh = vp->vehicle();
            int index = veh.part_with_feature( vp->part_index(), "CARGO", false );
            if( index >= 0 ) {
                vehicle_stack vehitems = veh.get_items( index );
                for( item &it : vehitems ) {
                    items_here.push_back( &it );
                }
            }
        } else {
            map_stack mapitems = here.i_at( here.getlocal( loc ) );
            for( item &it : mapitems ) {
                items_here.push_back( &it );
            }
        }
        for( item *it : items_here ) {
            item &comest = p.get_consumable_from( *it );
            if( comest.is_null() || comest.is_craft() || !comest.is_food() ||
                p.fun_for( comest ).first < -5 ) {
                // not good eatings.
                continue;
            }
            if( !p.can_consume( comest ) ) {
                continue;
            }
            if( food && p.compute_effective_nutrients( comest ).kcal < 50 ) {
                // not filling enough
                continue;
            }
            if( !p.will_eat( comest, false ).success() ) {
                // wont like it, cannibal meat etc
                continue;
            }
            if( !it->is_owned_by( p, true ) ) {
                // it aint ours.
                continue;
            }
            if( !food && comest.get_comestible()->quench < 15 ) {
                // not quenching enough
                continue;
            }
            if( !food && it->is_watertight_container() && comest.made_of( phase_id::SOLID ) ) {
                // its frozen
                continue;
            }
            int consume_moves = -Pickup::cost_to_move_item( p, *it ) * std::max( rl_dist( p.pos(),
                                here.getlocal( loc ) ), 1 );
            consume_moves += to_moves<int>( p.get_consume_time( comest ) );
            item_location item_loc;
            if( vp ) {
                item_loc = item_location( vehicle_cursor( vp->vehicle(), vp->part_index() ), &comest );
            } else {
                item_loc = item_location( map_cursor( here.getlocal( loc ) ), &comest );
            }

            p.consume( item_loc );
            // eat() may have removed the item, so check its still there.
            if( item_loc.get_item() && item_loc->is_container() ) {
                item_loc->on_contents_changed();
            }

            return consume_moves;
        }
    }
    return 0;
}

void try_fuel_fire( player_activity &act, player &p, const bool starting_fire )
{
    const tripoint pos = p.pos();
    std::vector<tripoint> adjacent = closest_points_first( pos, PICKUP_RANGE );
    adjacent.erase( adjacent.begin() );

    cata::optional<tripoint> best_fire = starting_fire ? act.placement : find_best_fire( adjacent,
                                         pos );

    map &here = get_map();
    if( !best_fire || !here.accessible_items( *best_fire ) ) {
        return;
    }

    cata::optional<tripoint> refuel_spot = find_refuel_spot_zone( pos );
    if( !refuel_spot ) {
        refuel_spot = find_refuel_spot_trap( adjacent, pos );
        if( !refuel_spot ) {
            return;
        }
    }

    // Special case: fire containers allow burning logs, so use them as fuel if fire is contained
    bool contained = here.has_flag_furn( TFLAG_FIRE_CONTAINER, *best_fire );
    fire_data fd( 1, contained );
    time_duration fire_age = here.get_field_age( *best_fire, fd_fire );

    // Maybe TODO: - refueling in the rain could use more fuel
    // First, simulate expected burn per turn, to see if we need more fuel
    map_stack fuel_on_fire = here.i_at( *best_fire );
    for( item &it : fuel_on_fire ) {
        it.simulate_burn( fd );
        // Unconstrained fires grow below -50_minutes age
        if( !contained && fire_age < -40_minutes && fd.fuel_produced > 1.0f &&
            !it.made_of( phase_id::LIQUID ) ) {
            // Too much - we don't want a firestorm!
            // Move item back to refueling pile
            // Note: move_item() handles messages (they're the generic "you drop x")
            move_item( p, it, 0, *best_fire, *refuel_spot, nullptr, -1 );
            return;
        }
    }

    // Enough to sustain the fire
    // TODO: It's not enough in the rain
    if( !starting_fire && ( fd.fuel_produced >= 1.0f || fire_age < 10_minutes ) ) {
        return;
    }

    // We need to move fuel from stash to fire
    map_stack potential_fuel = here.i_at( *refuel_spot );
    for( item &it : potential_fuel ) {
        if( it.made_of( phase_id::LIQUID ) ) {
            continue;
        }

        float last_fuel = fd.fuel_produced;
        it.simulate_burn( fd );
        if( fd.fuel_produced > last_fuel ) {
            int quantity = std::max( 1, std::min( it.charges, it.charges_per_volume( 250_ml ) ) );
            // Note: move_item() handles messages (they're the generic "you drop x")
            move_item( p, it, quantity, *refuel_spot, *best_fire, nullptr, -1 );
            return;
        }
    }
}
