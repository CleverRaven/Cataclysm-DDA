#include "activity_handlers.h" // IWYU pragma: associated

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iosfwd>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "activity_type.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "clzones.h"
#include "colony.h"
#include "construction.h"
#include "contents_change_handler.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enums.h"
#include "field.h"
#include "field_type.h"
#include "fire.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "iexamine.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "line.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "messages.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "overmapbuffer.h"
#include "pickup.h"
#include "player_activity.h"
#include "point.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "stomach.h"
#include "temp_crafting_inventory.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "weather.h"
#include "recipe_dictionary.h"
#include "activity_actor_definitions.h"

static const activity_id ACT_BUILD( "ACT_BUILD" );
static const activity_id ACT_BUTCHER_FULL( "ACT_BUTCHER_FULL" );
static const activity_id ACT_FETCH_REQUIRED( "ACT_FETCH_REQUIRED" );
static const activity_id ACT_FISH( "ACT_FISH" );
static const activity_id ACT_JACKHAMMER( "ACT_JACKHAMMER" );
static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );
static const activity_id ACT_MULTIPLE_BUTCHER( "ACT_MULTIPLE_BUTCHER" );
static const activity_id ACT_MULTIPLE_CHOP_PLANKS( "ACT_MULTIPLE_CHOP_PLANKS" );
static const activity_id ACT_MULTIPLE_CHOP_TREES( "ACT_MULTIPLE_CHOP_TREES" );
static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );
static const activity_id ACT_MULTIPLE_CRAFT( "ACT_MULTIPLE_CRAFT" );
static const activity_id ACT_MULTIPLE_DIS( "ACT_MULTIPLE_DIS" );
static const activity_id ACT_MULTIPLE_FARM( "ACT_MULTIPLE_FARM" );
static const activity_id ACT_MULTIPLE_FISH( "ACT_MULTIPLE_FISH" );
static const activity_id ACT_MULTIPLE_MINE( "ACT_MULTIPLE_MINE" );
static const activity_id ACT_MULTIPLE_MOP( "ACT_MULTIPLE_MOP" );
static const activity_id ACT_MULTIPLE_READ( "ACT_MULTIPLE_READ" );
static const activity_id ACT_PICKAXE( "ACT_PICKAXE" );
static const activity_id ACT_TIDY_UP( "ACT_TIDY_UP" );
static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );
static const activity_id ACT_VEHICLE_DECONSTRUCTION( "ACT_VEHICLE_DECONSTRUCTION" );
static const activity_id ACT_VEHICLE_REPAIR( "ACT_VEHICLE_REPAIR" );

static const addiction_id addiction_alcohol( "alcohol" );

static const efftype_id effect_incorporeal( "incorporeal" );

static const flag_id json_flag_CUT_HARVEST( "CUT_HARVEST" );
static const flag_id json_flag_MOP( "MOP" );
static const flag_id json_flag_NO_AUTO_CONSUME( "NO_AUTO_CONSUME" );

static const itype_id itype_battery( "battery" );
static const itype_id itype_disassembly( "disassembly" );
static const itype_id itype_log( "log" );
static const itype_id itype_soldering_iron( "soldering_iron" );
static const itype_id itype_welder( "welder" );

static const quality_id qual_AXE( "AXE" );
static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_DIG( "DIG" );
static const quality_id qual_FISHING_ROD( "FISHING_ROD" );
static const quality_id qual_GRASS_CUT( "GRASS_CUT" );
static const quality_id qual_SAW_M( "SAW_M" );
static const quality_id qual_SAW_W( "SAW_W" );
static const quality_id qual_WELD( "WELD" );

static const requirement_id requirement_data_mining_standard( "mining_standard" );

static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );

static const trap_str_id tr_firewood_source( "tr_firewood_source" );

static const zone_type_id zone_type_( "" );
static const zone_type_id zone_type_AUTO_DRINK( "AUTO_DRINK" );
static const zone_type_id zone_type_AUTO_EAT( "AUTO_EAT" );
static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );
static const zone_type_id zone_type_CHOP_TREES( "CHOP_TREES" );
static const zone_type_id zone_type_CONSTRUCTION_BLUEPRINT( "CONSTRUCTION_BLUEPRINT" );
static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );
static const zone_type_id zone_type_FISHING_SPOT( "FISHING_SPOT" );
static const zone_type_id zone_type_LOOT_CORPSE( "LOOT_CORPSE" );
static const zone_type_id zone_type_LOOT_CUSTOM( "LOOT_CUSTOM" );
static const zone_type_id zone_type_LOOT_IGNORE( "LOOT_IGNORE" );
static const zone_type_id zone_type_LOOT_IGNORE_FAVORITES( "LOOT_IGNORE_FAVORITES" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_LOOT_WOOD( "LOOT_WOOD" );
static const zone_type_id zone_type_MINING( "MINING" );
static const zone_type_id zone_type_MOPPING( "MOPPING" );
static const zone_type_id zone_type_SOURCE_FIREWOOD( "SOURCE_FIREWOOD" );
static const zone_type_id zone_type_VEHICLE_DECONSTRUCT( "VEHICLE_DECONSTRUCT" );
static const zone_type_id zone_type_VEHICLE_REPAIR( "VEHICLE_REPAIR" );
static const zone_type_id zone_type_zone_disassemble( "zone_disassemble" );
static const zone_type_id zone_type_zone_strip( "zone_strip" );
static const zone_type_id zone_type_zone_unload_all( "zone_unload_all" );

namespace
{
faction_id _fac_id( Character &you )
{
    faction const *fac = you.get_faction();
    return fac == nullptr ? faction_id() : fac->id;
}
} // namespace

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
        it.spill_open_pockets( c, /*avoid=*/&it );

        // If bucket is still not empty then player opted not to handle the
        // rest of the contents
        if( !it.empty() ) {
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
                              const vpart_reference &vpr )
{
    c.invalidate_weight_carried_cache();
    if( items.empty() ) {
        return;
    }
    vehicle_part &vp = vpr.part();
    vehicle &veh = vpr.vehicle();
    const tripoint where = veh.global_part_pos3( vp );
    map &here = get_map();
    int fallen_count = 0;
    int into_vehicle_count = 0;

    // can't use constant reference here because of the spill_contents()
    for( item it : items ) {
        if( handle_spillable_contents( c, it, here ) ) {
            continue;
        }

        if( it.made_of( phase_id::LIQUID ) ) {
            here.add_item_or_charges( c.pos(), it );
            it.charges = 0;
        }

        if( veh.add_item( vp, it ) ) {
            into_vehicle_count += it.count();
        } else {
            if( it.count_by_charges() ) {
                // Maybe we can add a few charges in the trunk and the rest on the ground.
                const int charges_added = veh.add_charges( vp, it );
                it.mod_charges( -charges_added );
                into_vehicle_count += charges_added;
            }
            here.add_item_or_charges( where, it );
            fallen_count += it.count();
        }
        it.handle_pickup_ownership( c );
    }

    const std::string part_name = vp.info().name();

    if( same_type( items ) ) {
        const item &it = items.front();
        const int dropcount = items.size() * it.count();
        const std::string it_name = it.tname( dropcount );

        switch( reason ) {
            case item_drop_reason::deliberate:
                c.add_msg_player_or_npc(
                    n_gettext( "You put your %1$s in the %2$s's %3$s.",
                               "You put your %1$s in the %2$s's %3$s.", dropcount ),
                    n_gettext( "<npcname> puts their %1$s in the %2$s's %3$s.",
                               "<npcname> puts their %1$s in the %2$s's %3$s.", dropcount ),
                    it_name, veh.name, part_name
                );
                break;
            case item_drop_reason::too_large:
                c.add_msg_if_player(
                    n_gettext(
                        "There's no room in your inventory for the %s, so you drop it into the %s's %s.",
                        "There's no room in your inventory for the %s, so you drop them into the %s's %s.",
                        dropcount ),
                    it_name, veh.name, part_name
                );
                break;
            case item_drop_reason::too_heavy:
                c.add_msg_if_player(
                    n_gettext( "The %s is too heavy to carry, so you drop it into the %s's %s.",
                               "The %s are too heavy to carry, so you drop them into the %s's %s.", dropcount ),
                    it_name, veh.name, part_name
                );
                break;
            case item_drop_reason::tumbling:
                c.add_msg_if_player(
                    m_bad,
                    n_gettext( "Your %s tumbles into the %s's %s.",
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
        const std::string ter_name = here.name( where );
        if( into_vehicle_count > 0 ) {
            c.add_msg_if_player(
                m_warning,
                n_gettext( "The %s is full, so something fell to the %s.",
                           "The %s is full, so some items fell to the %s.", fallen_count ),
                part_name, ter_name
            );
        } else {
            c.add_msg_if_player(
                m_warning,
                n_gettext( "The %s is full, so it fell to the %s.",
                           "The %s is full, so they fell to the %s.", fallen_count ),
                part_name, ter_name
            );
        }
    }
}

void drop_on_map( Character &you, item_drop_reason reason, const std::list<item> &items,
                  const tripoint_bub_ms &where )
{
    you.invalidate_weight_carried_cache();
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
                    you.add_msg_player_or_npc(
                        n_gettext( "You drop your %1$s on the %2$s.",
                                   "You drop your %1$s on the %2$s.", dropcount ),
                        n_gettext( "<npcname> drops their %1$s on the %2$s.",
                                   "<npcname> drops their %1$s on the %2$s.", dropcount ),
                        it_name, ter_name
                    );
                } else {
                    you.add_msg_player_or_npc(
                        n_gettext( "You put your %1$s in the %2$s.",
                                   "You put your %1$s in the %2$s.", dropcount ),
                        n_gettext( "<npcname> puts their %1$s in the %2$s.",
                                   "<npcname> puts their %1$s in the %2$s.", dropcount ),
                        it_name, ter_name
                    );
                }
                break;
            case item_drop_reason::too_large:
                you.add_msg_if_player(
                    n_gettext( "There's no room in your inventory for the %s, so you drop it.",
                               "There's no room in your inventory for the %s, so you drop them.", dropcount ),
                    it_name
                );
                break;
            case item_drop_reason::too_heavy:
                you.add_msg_if_player(
                    n_gettext( "The %s is too heavy to carry, so you drop it.",
                               "The %s is too heavy to carry, so you drop them.", dropcount ),
                    it_name
                );
                break;
            case item_drop_reason::tumbling:
                you.add_msg_if_player(
                    m_bad,
                    n_gettext( "Your %1$s tumbles to the %2$s.",
                               "Your %1$s tumble to the %2$s.", dropcount ),
                    it_name, ter_name
                );
                break;
        }

        if( get_option<bool>( "AUTO_NOTES_DROPPED_FAVORITES" ) && it.is_favorite ) {
            const tripoint_abs_omt your_pos = you.global_omt_location();
            if( !overmap_buffer.has_note( your_pos ) ) {
                overmap_buffer.add_note( your_pos, it.display_name() );
            } else {
                overmap_buffer.add_note( your_pos, overmap_buffer.note( your_pos ) + "; " + it.display_name() );
            }
        }
    } else {
        switch( reason ) {
            case item_drop_reason::deliberate:
                if( can_move_there ) {
                    you.add_msg_player_or_npc(
                        _( "You drop several items on the %s." ),
                        _( "<npcname> drops several items on the %s." ),
                        ter_name
                    );
                } else {
                    you.add_msg_player_or_npc(
                        _( "You put several items in the %s." ),
                        _( "<npcname> puts several items in the %s." ),
                        ter_name
                    );
                }
                break;
            case item_drop_reason::too_large:
            case item_drop_reason::too_heavy:
            case item_drop_reason::tumbling:
                you.add_msg_if_player( m_bad, _( "Some items tumble to the %s." ), ter_name );
                break;
        }
    }
    for( const item &it : items ) {
        here.add_item_or_charges( where, it );
        item( it ).handle_pickup_ownership( you );
    }

    you.recoil = MAX_RECOIL;
}

void put_into_vehicle_or_drop( Character &you, item_drop_reason reason,
                               const std::list<item> &items )
{
    return put_into_vehicle_or_drop( you, reason, items, you.pos_bub() );
}

void put_into_vehicle_or_drop( Character &you, item_drop_reason reason,
                               const std::list<item> &items,
                               const tripoint_bub_ms &where, bool force_ground )
{
    map &here = get_map();
    const std::optional<vpart_reference> vp = here.veh_at( where ).cargo();
    if( vp && !force_ground ) {
        put_into_vehicle( you, reason, items, *vp );
        return;
    }
    drop_on_map( you, reason, items, where );
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

int activity_handlers::move_cost_inv( const item &it, const tripoint_bub_ms &src,
                                      const tripoint_bub_ms &dest )
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

int activity_handlers::move_cost_cart(
    const item &it, const tripoint_bub_ms &src, const tripoint_bub_ms &dest,
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

int activity_handlers::move_cost( const item &it, const tripoint_bub_ms &src,
                                  const tripoint_bub_ms &dest )
{
    avatar &player_character = get_avatar();
    if( player_character.get_grab_type() == object_type::VEHICLE ) {
        const tripoint cart_position = player_character.pos() + player_character.grab_point;
        if( const std::optional<vpart_reference> ovp = get_map().veh_at( cart_position ).cargo() ) {
            return move_cost_cart( it, src, dest, ovp->items().free_volume() );
        }
    }

    return move_cost_inv( it, src, dest );
}

// return true if activity was assigned.
// return false if it was not possible.
static bool vehicle_activity( Character &you, const tripoint_bub_ms &src_loc, int vpindex,
                              char type )
{
    map &here = get_map();
    vehicle *veh = veh_pointer_or_null( here.veh_at( src_loc ) );
    if( !veh ) {
        return false;
    }
    time_duration time_to_take = 0_seconds;
    if( vpindex >= veh->part_count() ) {
        // if parts got removed during our work, we can't just carry on removing, we want to repair parts!
        // so just bail out, as we don't know if the next shifted part is suitable for repair.
        if( type == 'r' ) {
            return false;
        } else if( type == 'o' ) {
            vpindex = veh->get_next_shifted_index( vpindex, you );
            if( vpindex == -1 ) {
                return false;
            }
        }
    }
    const vehicle_part &vp = veh->part( vpindex );
    const vpart_info &vpi = vp.info();
    if( type == 'r' ) {
        const int frac = ( vp.damage() - vp.degradation() ) / ( vp.max_damage() - vp.degradation() );
        time_to_take = vpi.repair_time( you ) * frac;
    } else if( type == 'o' ) {
        time_to_take = vpi.removal_time( you );
    }
    you.assign_activity( ACT_VEHICLE, to_moves<int>( time_to_take ), static_cast<int>( type ) );
    // so , NPCs can remove the last part on a position, then there is no vehicle there anymore,
    // for someone else who stored that position at the start of their activity.
    // so we may need to go looking a bit further afield to find it , at activities end.
    for( const tripoint &pt : veh->get_points( true ) ) {
        you.activity.coord_set.insert( here.getabs( pt ) );
    }
    // values[0]
    you.activity.values.push_back( here.getabs( src_loc ).x );
    // values[1]
    you.activity.values.push_back( here.getabs( src_loc ).y );
    // values[2]
    you.activity.values.push_back( point_zero.x );
    // values[3]
    you.activity.values.push_back( point_zero.y );
    // values[4]
    you.activity.values.push_back( -point_zero.x );
    // values[5]
    you.activity.values.push_back( -point_zero.y );
    // values[6]
    you.activity.values.push_back( veh->index_of_part( &vp ) );
    you.activity.str_values.push_back( vpi.id.str() );
    you.activity.str_values.push_back( vp.variant );
    // this would only be used for refilling tasks
    item_location target;
    you.activity.targets.emplace_back( std::move( target ) );
    you.activity.placement = here.getglobal( src_loc );
    you.activity_vehicle_part_index = -1;
    return true;
}

static void move_item( Character &you, item &it, const int quantity, const tripoint_bub_ms &src,
                       const tripoint_bub_ms &dest, const std::optional<vpart_reference> &vpr_src,
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
        you.mod_moves( -activity_handlers::move_cost( it, src, dest ) );
        if( activity_to_restore == ACT_TIDY_UP ) {
            it.erase_var( "activity_var" );
        } else if( activity_to_restore == ACT_FETCH_REQUIRED ) {
            it.set_var( "activity_var", you.name );
        }
        put_into_vehicle_or_drop( you, item_drop_reason::deliberate, { it }, dest );
        // Remove from map or vehicle.
        if( vpr_src ) {
            vpr_src->vehicle().remove_item( vpr_src->part(), &it );
        } else {
            here.i_rem( src, &it );
        }
    }

    // If we didn't pick up a whole stack, put the remainder back where it came from.
    if( leftovers.charges > 0 ) {
        if( vpr_src ) {
            if( !vpr_src->vehicle().add_item( vpr_src->part(), leftovers ) ) {
                debugmsg( "SortLoot: Source vehicle failed to receive leftover charges." );
            }
        } else {
            here.add_item_or_charges( src, leftovers );
        }
    }
}

std::vector<tripoint_bub_ms> route_adjacent( const Character &you, const tripoint_bub_ms &dest )
{
    std::unordered_set<tripoint_bub_ms> passable_tiles;
    map &here = get_map();

    for( const tripoint_bub_ms &tp : here.points_in_radius( dest, 1 ) ) {
        if( tp != you.pos_bub() && here.passable( tp ) ) {
            passable_tiles.emplace( tp );
        }
    }

    const std::vector<tripoint_bub_ms> &sorted =
        get_sorted_tiles_by_distance( you.pos_bub(), passable_tiles );

    const std::set<tripoint> &avoid = you.get_path_avoid();
    for( const tripoint_bub_ms &tp : sorted ) {
        std::vector<tripoint_bub_ms> route =
            here.route( you.pos_bub(), tp, you.get_pathfinding_settings(), avoid );

        if( !route.empty() ) {
            return route;
        }
    }

    return {};
}

static std::vector<tripoint_bub_ms> route_best_workbench(
    const Character &you, const tripoint_bub_ms &dest )
{
    std::unordered_set<tripoint_bub_ms> passable_tiles;
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &tp : here.points_in_radius( dest, 1 ) ) {
        if( tp == you.pos_bub() || ( here.passable( tp ) && !creatures.creature_at( tp ) ) ) {
            passable_tiles.insert( tp );
        }
    }
    // Make sure current tile is at first
    // so that the "best" tile doesn't change on reaching our destination
    // if we are near the best workbench
    std::vector<tripoint_bub_ms> sorted =
        get_sorted_tiles_by_distance( you.pos_bub(), passable_tiles );

    const auto cmp = [&]( const tripoint_bub_ms & a, const tripoint_bub_ms & b ) {
        float best_bench_multi_a = 0.0f;
        float best_bench_multi_b = 0.0f;
        for( const tripoint_bub_ms &adj : here.points_in_radius( a, 1 ) ) {
            if( here.dangerous_field_at( adj ) ) {
                continue;
            }
            if( const cata::value_ptr<furn_workbench_info> &wb = here.furn( adj ).obj().workbench ) {
                if( wb->multiplier > best_bench_multi_a ) {
                    best_bench_multi_a = wb->multiplier;
                }
            } else if( const std::optional<vpart_reference> vp = here.veh_at(
                           adj ).part_with_feature( "WORKBENCH", true ) ) {
                if( const std::optional<vpslot_workbench> &wb_info = vp->part().info().workbench_info ) {
                    if( wb_info->multiplier > best_bench_multi_a ) {
                        best_bench_multi_a = wb_info->multiplier;
                    }
                } else {
                    debugmsg( "part '%s' with WORKBENCH flag has no workbench info", vp->part().name() );
                }
            }
        }
        for( const tripoint_bub_ms &adj : here.points_in_radius( b, 1 ) ) {
            if( here.dangerous_field_at( adj ) ) {
                continue;
            }
            if( const cata::value_ptr<furn_workbench_info> &wb = here.furn( adj ).obj().workbench ) {
                if( wb->multiplier > best_bench_multi_b ) {
                    best_bench_multi_b = wb->multiplier;
                }
            } else if( const std::optional<vpart_reference> vp = here.veh_at(
                           adj ).part_with_feature( "WORKBENCH", true ) ) {
                if( const std::optional<vpslot_workbench> &wb_info = vp->part().info().workbench_info ) {
                    if( wb_info->multiplier > best_bench_multi_b ) {
                        best_bench_multi_b = wb_info->multiplier;
                    }
                } else {
                    debugmsg( "part '%s' with WORKBENCH flag has no workbench info", vp->part().name() );
                }
            }
        }
        return best_bench_multi_a > best_bench_multi_b;
    };
    std::stable_sort( sorted.begin(), sorted.end(), cmp );
    const std::set<tripoint> &avoid = you.get_path_avoid();
    if( sorted.front() == you.pos_bub() ) {
        // We are on the best tile
        return {};
    }
    for( const tripoint_bub_ms &tp : sorted ) {
        std::vector<tripoint_bub_ms> route =
            here.route( you.pos_bub(), tp, you.get_pathfinding_settings(), avoid );

        if( !route.empty() ) {
            return route;
        }
    }
    return {};

}

namespace
{

bool _can_construct(
    tripoint_bub_ms const &loc, construction_id const &idx, construction const &check,
    std::optional<construction_id> const &part_con_idx )
{
    return ( part_con_idx && *part_con_idx == check.id ) ||
           ( check.pre_terrain != idx->post_terrain && can_construct( check, loc ) );
}

construction const *
_find_alt_construction( tripoint_bub_ms const &loc, construction_id const &idx,
                        std::optional<construction_id> const &part_con_idx,
                        std::function<bool( construction const & )> const &filter )
{
    std::vector<construction *> cons = constructions_by_filter( filter );
    for( construction const *el : cons ) {
        if( _can_construct( loc, idx, *el, part_con_idx ) ) {
            return el;
        }
    }
    return nullptr;
}

template <class ID>
ID _get_id( construction_id const &idx )
{
    return idx->post_terrain.empty() ? ID() : ID( idx->post_terrain );
}

using checked_cache_t = std::vector<construction_id>;
construction const *_find_prereq( tripoint_bub_ms const &loc, construction_id const &idx,
                                  construction_id const &top_idx,
                                  std::optional<construction_id> const &part_con_idx, checked_cache_t &checked_cache )
{
    construction const *con = nullptr;
    std::vector<construction *> cons = constructions_by_filter( [&idx, &top_idx](
    construction const & it ) {
        furn_id const f = top_idx->post_is_furniture ? _get_id<furn_id>( top_idx ) : furn_id();
        ter_id const t = top_idx->post_is_furniture ? ter_id() : _get_id<ter_id>( top_idx );
        return it.group != idx->group && !it.post_terrain.empty() &&
               it.post_terrain == idx->pre_terrain &&
               // don't get stuck building and deconstructing the top level post_terrain
               it.pre_terrain != top_idx->post_terrain &&
               ( it.pre_flags.empty() || !can_construct_furn_ter( it, f, t ) );
    } );

    for( construction const *gcon : cons ) {
        if( std::find( checked_cache.begin(), checked_cache.end(), gcon->id ) !=
            checked_cache.end() ) {
            continue;
        }
        checked_cache.emplace_back( gcon->id );
        if( _can_construct( loc, idx, *gcon, part_con_idx ) ) {
            return gcon;
        }
        // try to find a prerequisite of this prerequisite
        if( !gcon->pre_terrain.empty() || !gcon->pre_flags.empty() ) {
            con = _find_prereq( loc, gcon->id, top_idx, part_con_idx, checked_cache );
        }
        if( con != nullptr ) {
            return con;
        }
    }
    return nullptr;
}

bool already_done( construction const &build, tripoint_bub_ms const &loc )
{
    map &here = get_map();
    const furn_id furn = here.furn( loc );
    const ter_id ter = here.ter( loc );
    return !build.post_terrain.empty() &&
           ( ( !build.post_is_furniture && ter_id( build.post_terrain ) == ter ) ||
             ( build.post_is_furniture && furn_id( build.post_terrain ) == furn ) );
}

} // namespace

static activity_reason_info find_base_construction(
    Character &you,
    const inventory &inv,
    const tripoint_bub_ms &loc,
    const std::optional<construction_id> &part_con_idx,
    const construction_id &idx )
{
    if( already_done( idx.obj(), loc ) ) {
        return activity_reason_info::build( do_activity_reason::ALREADY_DONE, false, idx->id );
    }
    bool cc = can_construct( idx.obj(), loc );
    construction const *con = nullptr;

    if( !cc ) {
        // try to build a variant from the same group
        con = _find_alt_construction( loc, idx, part_con_idx, [&idx]( construction const & it ) {
            return it.group == idx->group;
        } );
        if( !idx->strict && con == nullptr ) {
            // try to build a pre-requisite from a different group, recursively
            checked_cache_t checked_cache;
            for( construction const *vcon : constructions_by_group( idx->group ) ) {
                con = _find_prereq( loc, vcon->id, vcon->id, part_con_idx, checked_cache );
                if( con != nullptr ) {
                    break;
                }
            }
        }
        cc = con != nullptr;
    }

    const construction &build = con == nullptr ? idx.obj() : *con;
    bool pcb = player_can_build( you, inv, build, true );
    //already done?
    if( already_done( build, loc ) ) {
        return activity_reason_info::build( do_activity_reason::ALREADY_DONE, false, build.id );
    }

    const bool has_skill = you.meets_skill_requirements( build );
    if( !has_skill ) {
        return activity_reason_info::build( do_activity_reason::DONT_HAVE_SKILL, false, build.id );
    }
    //if there's an appropriate partial construction on the tile, then we can work on it, no need to check inventories.
    if( part_con_idx ) {
        if( *part_con_idx != build.id ) {
            //have a partial construction which is not leading to the required construction
            return activity_reason_info::build( do_activity_reason::BLOCKING_TILE, false, idx );
        }
        return activity_reason_info::build( do_activity_reason::CAN_DO_CONSTRUCTION, true, build.id );
    }
    //can build?
    if( cc ) {
        if( pcb ) {
            return activity_reason_info::build( do_activity_reason::CAN_DO_CONSTRUCTION, true, build.id );
        }
        //can't build with current inventory, do not look for pre-req
        return activity_reason_info::build( do_activity_reason::NO_COMPONENTS, false, build.id );
    }

    return activity_reason_info::build( do_activity_reason::BLOCKING_TILE, false, idx );
}

static bool are_requirements_nearby(
    const std::vector<tripoint_bub_ms> &loot_spots, const requirement_id &needed_things,
    Character &you, const activity_id &activity_to_restore, const bool in_loot_zones,
    const tripoint_bub_ms &src_loc )
{
    zone_manager &mgr = zone_manager::get_manager();
    temp_crafting_inventory temp_inv;
    units::volume volume_allowed;
    units::mass weight_allowed;
    static const auto check_weight_if = []( const activity_id & id ) {
        return id == ACT_MULTIPLE_FARM ||
               id == ACT_MULTIPLE_CHOP_PLANKS ||
               id == ACT_MULTIPLE_BUTCHER ||
               id == ACT_VEHICLE_DECONSTRUCTION ||
               id == ACT_VEHICLE_REPAIR ||
               id == ACT_MULTIPLE_CHOP_TREES ||
               id == ACT_MULTIPLE_FISH ||
               id == ACT_MULTIPLE_MINE ||
               id == ACT_MULTIPLE_MOP;
    };
    const bool check_weight = check_weight_if( activity_to_restore ) || ( !you.backlog.empty() &&
                              check_weight_if( you.backlog.front().id() ) );

    if( check_weight ) {
        volume_allowed = you.volume_capacity() - you.volume_carried();
        weight_allowed = you.weight_capacity() - you.weight_carried();
    }

    bool found_welder = false;
    for( item *elem : you.inv_dump() ) {
        if( elem->has_quality( qual_WELD ) ) {
            found_welder = true;
        }
        temp_inv.add_item_ref( *elem );
    }
    map &here = get_map();
    for( const tripoint_bub_ms &elem : loot_spots ) {
        // if we are searching for things to fetch, we can skip certain things.
        // if, however they are already near the work spot, then the crafting / inventory functions will have their own method to use or discount them.
        if( in_loot_zones ) {
            // skip tiles in IGNORE zone and tiles on fire
            // (to prevent taking out wood off the lit brazier)
            // and inaccessible furniture, like filled charcoal kiln
            if( mgr.has( zone_type_LOOT_IGNORE, here.getglobal( elem ), _fac_id( you ) ) ||
                here.dangerous_field_at( elem ) ||
                !here.can_put_items_ter_furn( elem ) ) {
                continue;
            }
        }
        for( item &elem2 : here.i_at( elem ) ) {
            if( in_loot_zones ) {
                if( elem2.made_of_from_type( phase_id::LIQUID ) ) {
                    continue;
                }

                if( check_weight ) {
                    // this fetch task will need to pick up an item. so check for its weight/volume before setting off.
                    if( elem2.volume() > volume_allowed ||
                        elem2.weight() > weight_allowed ) {
                        continue;
                    }
                }
            }
            temp_inv.add_item_ref( elem2 );
        }

        if( !in_loot_zones ) {
            if( const std::optional<vpart_reference> ovp = here.veh_at( elem ).cargo() ) {
                for( item &it : ovp->items() ) {
                    temp_inv.add_item_ref( it );
                }
            }
        }
    }
    // use nearby welding rig without needing to drag it or position yourself on the right side of the vehicle.
    if( !found_welder ) {
        for( const tripoint_bub_ms &elem : here.points_in_radius( src_loc, PICKUP_RANGE - 1,
                PICKUP_RANGE - 1 ) ) {
            const std::optional<vpart_reference> &vp = here.veh_at( elem ).part_with_tool( itype_welder );

            if( vp ) {
                const int veh_battery = vp->vehicle().fuel_left( itype_battery );

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

static activity_reason_info can_do_activity_there( const activity_id &act, Character &you,
        const tripoint_bub_ms &src_loc, const int distance = ACTIVITY_SEARCH_DISTANCE )
{
    // see activity_handlers.h cant_do_activity_reason enums
    you.invalidate_crafting_inventory();
    zone_manager &mgr = zone_manager::get_manager();
    std::vector<zone_data> zones;
    Character &player_character = get_player_character();
    map &here = get_map();
    if( act == ACT_VEHICLE_DECONSTRUCTION ||
        act == ACT_VEHICLE_REPAIR ) {
        std::vector<int> already_working_indexes;
        vehicle *veh = veh_pointer_or_null( here.veh_at( src_loc ) );
        if( !veh || veh->is_appliance() ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
        // if the vehicle is moving or player is controlling it.
        if( std::abs( veh->velocity ) > 100 || veh->player_in_control( player_character ) ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
        for( const npc &guy : g->all_npcs() ) {
            if( &guy == &you ) {
                continue;
            }
            // If the NPC has an activity - make sure they're not duplicating work.
            tripoint_bub_ms guy_work_spot;
            if( guy.has_player_activity() &&
                guy.activity.placement != player_activity::invalid_place ) {
                guy_work_spot = here.bub_from_abs( guy.activity.placement );
            }
            // If their position or intended position or player position/intended position
            // then discount, don't need to move each other out of the way.
            if( here.bub_from_abs( player_character.activity.placement ) == src_loc ||
                guy_work_spot == src_loc || guy.pos_bub() == src_loc ||
                ( you.is_npc() && player_character.pos_bub() == src_loc ) ) {
                return activity_reason_info::fail( do_activity_reason::ALREADY_WORKING );
            }
            if( guy_work_spot != tripoint_bub_ms{} ) {
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
            // TODO: fix point types
            std::vector<vehicle_part *> parts =
                veh->get_parts_at( src_loc.raw(), "", part_status_flag::any );
            for( vehicle_part *part_elem : parts ) {
                const int vpindex = veh->index_of_part( part_elem, true );
                // if part is not on this vehicle, or if its attached to another part that needs to be removed first.
                if( vpindex < 0 || !veh->can_unmount( *part_elem ).success() ) {
                    continue;
                }
                const vpart_info &vpinfo = part_elem->info();
                // If removing this part would make the vehicle non-flyable, avoid it
                if( veh->would_removal_prevent_flyable( *part_elem, player_character ) ) {
                    return activity_reason_info::fail( do_activity_reason::WOULD_PREVENT_VEH_FLYING );
                }
                // this is the same part that somebody else wants to work on, or already is.
                if( std::find( already_working_indexes.begin(), already_working_indexes.end(),
                               vpindex ) != already_working_indexes.end() ) {
                    continue;
                }
                // don't have skill to remove it
                if( !you.meets_skill_requirements( vpinfo.removal_skills ) ) {
                    continue;
                }
                item base( vpinfo.base_item );
                // TODO: fix point types
                const units::mass max_lift = you.best_nearby_lifting_assist( src_loc.raw() );
                const bool use_aid = max_lift >= base.weight();
                const bool use_str = you.can_lift( base );
                if( !( use_aid || use_str ) ) {
                    continue;
                }
                const requirement_data &reqs = vpinfo.removal_requirements();
                const inventory &inv = you.crafting_inventory( false );

                const bool can_make = reqs.can_make_with_inventory( inv, is_crafting_component );
                you.set_value( "veh_index_type", vpinfo.name() );
                // temporarily store the intended index, we do this so two NPCs don't try and work on the same part at same time.
                you.activity_vehicle_part_index = vpindex;
                if( !can_make ) {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_VEH_DECONST );
                } else {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_VEH_DECONST );
                }
            }
        } else if( act == ACT_VEHICLE_REPAIR ) {
            // find out if there is a vehicle part here we can repair.
            // TODO: fix point types
            std::vector<vehicle_part *> parts = veh->get_parts_at( src_loc.raw(), "", part_status_flag::any );
            for( vehicle_part *part_elem : parts ) {
                const vpart_info &vpinfo = part_elem->info();
                int vpindex = veh->index_of_part( part_elem, true );
                // if part is undamaged or beyond repair - can skip it.
                if( !part_elem->is_repairable() ) {
                    continue;
                }
                // If repairing this part would make the vehicle non-flyable, avoid it
                if( veh->would_repair_prevent_flyable( *part_elem, player_character ) ) {
                    return activity_reason_info::fail( do_activity_reason::WOULD_PREVENT_VEH_FLYING );
                }
                if( std::find( already_working_indexes.begin(), already_working_indexes.end(),
                               vpindex ) != already_working_indexes.end() ) {
                    continue;
                }
                // don't have skill to repair it

                if( !you.meets_skill_requirements( vpinfo.repair_skills ) ) {
                    continue;
                }
                const requirement_data &reqs = vpinfo.repair_requirements();
                // TODO: fix point types
                const inventory &inv =
                    you.crafting_inventory( src_loc.raw(), PICKUP_RANGE - 1, false );
                const bool can_make = reqs.can_make_with_inventory( inv, is_crafting_component );
                you.set_value( "veh_index_type", vpinfo.name() );
                // temporarily store the intended index, we do this so two NPCs don't try and work on the same part at same time.
                you.activity_vehicle_part_index = vpindex;
                if( !can_make ) {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_VEH_REPAIR );
                } else {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_VEH_REPAIR );
                }
            }
        }
        you.activity_vehicle_part_index = -1;
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
    if( act == ACT_MULTIPLE_MINE ) {
        if( !here.has_flag( ter_furn_flag::TFLAG_MINEABLE, src_loc ) ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
        std::vector<item *> mining_inv = you.items_with( [&you]( const item & itm ) {
            return ( itm.has_flag( flag_DIG_TOOL ) && !itm.type->can_use( "JACKHAMMER" ) ) ||
                   ( itm.type->can_use( "JACKHAMMER" ) && itm.ammo_sufficient( &you ) );
        } );
        if( mining_inv.empty() ) {
            return activity_reason_info::fail( do_activity_reason::NEEDS_MINING );
        } else {
            return activity_reason_info::ok( do_activity_reason::NEEDS_MINING );
        }
    }
    if( act == ACT_MULTIPLE_MOP ) {
        if( !here.terrain_moppable( src_loc ) ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }

        if( you.has_item_with( []( const item & itm ) {
        return itm.has_flag( json_flag_MOP );
        } ) ) {
            return activity_reason_info::ok( do_activity_reason::NEEDS_MOP );
        } else {
            return activity_reason_info::fail( do_activity_reason::NEEDS_MOP );
        }
    }
    if( act == ACT_MULTIPLE_FISH ) {
        if( !here.has_flag( ter_furn_flag::TFLAG_FISHABLE, src_loc ) ) {
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
        std::vector<item *> rod_inv = you.items_with( []( const item & itm ) {
            return itm.has_quality( qual_FISHING_ROD );
        } );
        if( rod_inv.empty() ) {
            return activity_reason_info::fail( do_activity_reason::NEEDS_FISHING );
        } else {
            return activity_reason_info::ok( do_activity_reason::NEEDS_FISHING );
        }
    }
    if( act == ACT_MULTIPLE_CHOP_TREES ) {
        if( here.has_flag( ter_furn_flag::TFLAG_TREE, src_loc ) || here.ter( src_loc ) == t_trunk ||
            here.ter( src_loc ) == t_stump ) {
            if( you.has_quality( qual_AXE ) ) {
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
        for( const tripoint_bub_ms &pt : here.points_in_radius( src_loc, 2 ) ) {
            if( here.has_flag_furn( ter_furn_flag::TFLAG_BUTCHER_EQ, pt ) ) {
                b_rack_present = true;
            }
        }
        if( !corpses.empty() ) {
            if( big_count > 0 && small_count == 0 ) {
                if( !b_rack_present || !here.has_nearby_table( src_loc, 2 ) ) {
                    return activity_reason_info::fail( do_activity_reason::NO_ZONE );
                }
                if( you.has_quality( quality_id( qual_BUTCHER ), 1 ) && ( you.has_quality( qual_SAW_W ) ||
                        you.has_quality( qual_SAW_M ) ) ) {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_BIG_BUTCHERING );
                } else {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_BIG_BUTCHERING );
                }
            }
            if( ( big_count > 0 && small_count > 0 ) || ( big_count == 0 ) ) {
                // there are small corpses here, so we can ignore any big corpses here for the moment.
                if( you.has_quality( qual_BUTCHER, 1 ) ) {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_BUTCHERING );
                } else {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_BUTCHERING );
                }
            }
        }
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
    if( act == ACT_MULTIPLE_READ ) {
        const item_filter filter = [ &you ]( const item & i ) {
            // Check well lit after
            read_condition_result condition = you.check_read_condition( i );
            return condition == read_condition_result::SUCCESS ||
                   condition == read_condition_result::TOO_DARK;
        };
        if( !you.items_with( filter ).empty() ) {
            return activity_reason_info::ok( do_activity_reason::NEEDS_BOOK_TO_LEARN );
        }
        // TODO: find books from zone?
        return activity_reason_info::fail( do_activity_reason::ALREADY_DONE );
    }
    if( act == ACT_MULTIPLE_CHOP_PLANKS ) {
        //are there even any logs there?
        for( item &i : here.i_at( src_loc ) ) {
            if( i.typeId() == itype_log ) {
                // do we have an axe?
                if( you.has_quality( qual_AXE, 1 ) ) {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_CHOPPING );
                } else {
                    return activity_reason_info::fail( do_activity_reason::NEEDS_CHOPPING );
                }
            }
        }
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
    if( act == ACT_TIDY_UP ) {
        if( mgr.has_near( zone_type_LOOT_UNSORTED, here.getglobal( src_loc ), distance, _fac_id( you ) ) ||
            mgr.has_near( zone_type_CAMP_STORAGE, here.getglobal( src_loc ), distance, _fac_id( you ) ) ) {
            return activity_reason_info::ok( do_activity_reason::CAN_DO_FETCH );
        }
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
    if( act == ACT_MULTIPLE_CONSTRUCTION ) {
        zones = mgr.get_zones( zone_type_CONSTRUCTION_BLUEPRINT,
                               here.getglobal( src_loc ), _fac_id( you ) );
        // TODO: fix point types
        const partial_con *part_con = here.partial_con_at( tripoint_bub_ms( src_loc ) );
        std::optional<construction_id> part_con_idx;
        if( part_con ) {
            part_con_idx = part_con->id;
        }

        tripoint_bub_ms nearest_src_loc;
        if( square_dist( you.pos_bub(), src_loc ) == 1 ) {
            nearest_src_loc = you.pos_bub();
        } else {
            std::vector<tripoint_bub_ms> const route = route_adjacent( you, src_loc );
            if( route.empty() ) {
                return activity_reason_info::fail( do_activity_reason::BLOCKING_TILE );
            }
            nearest_src_loc = route.back();
        }
        // TODO: fix point types
        const inventory pre_inv = you.crafting_inventory( nearest_src_loc.raw(), PICKUP_RANGE );
        if( !zones.empty() ) {
            const blueprint_options &options = dynamic_cast<const blueprint_options &>
                                               ( zones.front().get_options() );
            const construction_id index = options.get_index();
            // TODO: fix point types
            return find_base_construction( you, pre_inv, tripoint_bub_ms( src_loc ), part_con_idx,
                                           index );
        }
    } else if( act == ACT_MULTIPLE_FARM ) {
        zones = mgr.get_zones( zone_type_FARM_PLOT, here.getglobal( src_loc ), _fac_id( you ) );
        for( const zone_data &zone : zones ) {
            if( here.has_flag_furn( ter_furn_flag::TFLAG_GROWTH_HARVEST, src_loc ) ) {
                map_stack items = here.i_at( src_loc );
                const map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
                    return it.is_seed();
                } );
                if( seed->has_flag( json_flag_CUT_HARVEST ) ) {
                    // The plant in this location needs a grass cutting tool.
                    if( you.has_quality( quality_id( qual_GRASS_CUT ), 1 ) ) {
                        return activity_reason_info::ok( do_activity_reason::NEEDS_CUT_HARVESTING );
                    } else {
                        return activity_reason_info::fail( do_activity_reason::NEEDS_CUT_HARVESTING );
                    }
                } else {
                    // We can harvest this plant without any tools.
                    return activity_reason_info::ok( do_activity_reason::NEEDS_HARVESTING );
                }
            } else if( here.has_flag( ter_furn_flag::TFLAG_PLOWABLE, src_loc ) && !here.has_furn( src_loc ) ) {
                if( you.has_quality( qual_DIG, 1 ) ) {
                    // we have a shovel/hoe already, great
                    return activity_reason_info::ok( do_activity_reason::NEEDS_TILLING );
                } else {
                    // we need a shovel/hoe
                    return activity_reason_info::fail( do_activity_reason::NEEDS_TILLING );
                }
            } else if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_PLANTABLE, src_loc ) &&
                       // TODO: fix point types
                       warm_enough_to_plant( src_loc.raw() ) ) {
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
                    std::vector<item *> seed_inv = you.items_with( []( const item & itm ) {
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
    } else if( act == ACT_MULTIPLE_CRAFT ) {
        // only npc is supported
        npc *p = you.as_npc();
        if( p ) {
            item_location to_craft = p->get_item_to_craft();
            if( to_craft && to_craft->is_craft() ) {
                const inventory &inv = you.crafting_inventory( src_loc.raw(), PICKUP_RANGE - 1, false );
                const recipe &r = to_craft->get_making();
                std::vector<std::vector<item_comp>> item_comp_vector =
                                                     to_craft->get_continue_reqs().get_components();
                std::vector<std::vector<quality_requirement>> quality_comp_vector =
                            r.simple_requirements().get_qualities();
                std::vector<std::vector<tool_comp>> tool_comp_vector = r.simple_requirements().get_tools();
                requirement_data req = requirement_data( tool_comp_vector, quality_comp_vector, item_comp_vector );
                if( req.can_make_with_inventory( inv, is_crafting_component ) ) {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_CRAFT );
                } else {
                    return activity_reason_info( do_activity_reason::NEEDS_CRAFT, false, req );
                }
            }
        }
        return activity_reason_info::fail( do_activity_reason::ALREADY_DONE );
    } else if( act == ACT_MULTIPLE_DIS ) {
        // Is there anything to be disassembled?
        // TODO: fix point types
        const inventory &inv = you.crafting_inventory( src_loc.raw(), PICKUP_RANGE - 1, false );
        requirement_data req;
        for( item &i : here.i_at( src_loc ) ) {
            // Skip items marked by other ppl.
            if( i.has_var( "activity_var" ) && i.get_var( "activity_var" ) != you.name ) {
                continue;
            }
            //unmark the item before check
            i.erase_var( "activity_var" );
            if( i.is_disassemblable() ) {
                // Are the requirements fulfilled?
                const recipe &r = recipe_dictionary::get_uncraft( ( i.typeId() == itype_disassembly ) ?
                                  i.components.only_item().typeId() : i.typeId() );
                req = r.disassembly_requirements();
                if( !std::all_of( req.get_qualities().begin(),
                req.get_qualities().end(), [&inv]( const std::vector<quality_requirement> &cur ) {
                return cur.empty() ||
                    std::any_of( cur.begin(), cur.end(), [&inv]( const quality_requirement & curr ) {
                        return curr.has( inv, return_true<item> );
                    } );
                } ) ) {
                    continue;
                }
                if( !std::all_of( req.get_tools().begin(),
                req.get_tools().end(), [&inv]( const std::vector<tool_comp> &cur ) {
                return cur.empty() || std::any_of( cur.begin(), cur.end(), [&inv]( const tool_comp & curr ) {
                        return  curr.has( inv, return_true<item> );
                    } );
                } ) ) {
                    continue;
                }
                // check passed, mark the item
                i.set_var( "activity_var", you.name );
                return activity_reason_info::ok( do_activity_reason::NEEDS_DISASSEMBLE );
            }
        }
        if( !req.is_null() || !req.is_empty() ) {
            // need tools
            return activity_reason_info( do_activity_reason::NEEDS_DISASSEMBLE, false, req );
        } else {
            // nothing to disassemble
            return activity_reason_info::fail( do_activity_reason::NO_ZONE );
        }
    }
    // Shouldn't get here because the zones were checked previously. if it does, set enum reason as "no zone"
    return activity_reason_info::fail( do_activity_reason::NO_ZONE );
}

static void add_basecamp_storage_to_loot_zone_list(
    zone_manager &mgr, const tripoint_bub_ms &src_loc, Character &you,
    std::vector<tripoint_bub_ms> &loot_zone_spots, std::vector<tripoint_bub_ms> &combined_spots )
{
    if( npc *const guy = dynamic_cast<npc *>( &you ) ) {
        map &here = get_map();
        if( guy->assigned_camp &&
            mgr.has_near( zone_type_CAMP_STORAGE, here.getglobal( src_loc ), ACTIVITY_SEARCH_DISTANCE,
                          _fac_id( you ) ) ) {
            std::unordered_set<tripoint_abs_ms> bc_storage_set =
                mgr.get_near( zone_type_CAMP_STORAGE, here.getglobal( src_loc ),
                              ACTIVITY_SEARCH_DISTANCE, nullptr, _fac_id( you ) );
            for( const tripoint_abs_ms &elem : bc_storage_set ) {
                tripoint_bub_ms here_local = here.bub_from_abs( elem );

                // Check that a coordinate is not already in the combined list, otherwise actions
                // like construction may erroneously count materials twice if an object is both
                // in the camp zone and in a loot zone.
                if( std::find( combined_spots.begin(), combined_spots.end(),
                               here_local ) == combined_spots.end() ) {
                    loot_zone_spots.push_back( here_local );
                    combined_spots.push_back( here_local );
                }
            }
        }
    }
}

static std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> requirements_map( Character &you,
        const int distance = ACTIVITY_SEARCH_DISTANCE )
{
    std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> requirement_map;
    if( you.backlog.empty() || you.backlog.front().str_values.empty() ) {
        return requirement_map;
    }
    const requirement_data things_to_fetch = requirement_id( you.backlog.front().str_values[0] ).obj();
    const activity_id activity_to_restore = you.backlog.front().id();
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    requirement_id things_to_fetch_id = things_to_fetch.id();
    std::vector<std::vector<item_comp>> req_comps = things_to_fetch.get_components();
    std::vector<std::vector<tool_comp>> tool_comps = things_to_fetch.get_tools();
    std::vector<std::vector<quality_requirement>> quality_comps = things_to_fetch.get_qualities();
    zone_manager &mgr = zone_manager::get_manager();
    const bool pickup_task = you.backlog.front().id() == ACT_MULTIPLE_FARM ||
                             you.backlog.front().id() == ACT_MULTIPLE_CHOP_PLANKS ||
                             you.backlog.front().id() == ACT_MULTIPLE_BUTCHER ||
                             you.backlog.front().id() == ACT_MULTIPLE_CHOP_TREES ||
                             you.backlog.front().id() == ACT_VEHICLE_DECONSTRUCTION ||
                             you.backlog.front().id() == ACT_VEHICLE_REPAIR ||
                             you.backlog.front().id() == ACT_MULTIPLE_FISH ||
                             you.backlog.front().id() == ACT_MULTIPLE_MINE;
    // where it is, what it is, how much of it, and how much in total is required of that item.
    std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> final_map;
    std::vector<tripoint_bub_ms> loot_spots;
    std::vector<tripoint_bub_ms> already_there_spots;
    std::vector<tripoint_bub_ms> combined_spots;
    std::map<itype_id, int> total_map;
    map &here = get_map();
    tripoint_bub_ms src_loc = here.bub_from_abs( you.backlog.front().placement );
    for( const tripoint_bub_ms &elem : here.points_in_radius( src_loc,
            PICKUP_RANGE - 1 ) ) {
        already_there_spots.push_back( elem );
        combined_spots.push_back( elem );
    }
    // TODO: fix point types
    for( const tripoint &elem : mgr.get_point_set_loot(
             you.get_location(), distance, you.is_npc(), _fac_id( you ) ) ) {
        // if there is a loot zone that's already near the work spot, we don't want it to be added twice.
        if( std::find( already_there_spots.begin(), already_there_spots.end(),
                       // TODO: fix point types
                       tripoint_bub_ms( elem ) ) != already_there_spots.end() ) {
            // construction tasks don't need the loot spot *and* the already_there/combined spots both added.
            // but a farming task will need to go and fetch the tool no matter if its near the work spot.
            // whereas the construction will automatically use what's nearby anyway.
            if( pickup_task ) {
                loot_spots.emplace_back( elem );
            } else {
                continue;
            }
        } else {
            loot_spots.emplace_back( elem );
            combined_spots.emplace_back( elem );
        }
    }
    add_basecamp_storage_to_loot_zone_list( mgr, src_loc, you, loot_spots, combined_spots );
    // if the requirements aren't available, then stop.
    if( !are_requirements_nearby( pickup_task ? loot_spots : combined_spots, things_to_fetch_id, you,
                                  activity_to_restore, pickup_task, src_loc ) ) {
        return requirement_map;
    }
    // if the requirements are already near the work spot and its a construction/crafting task, then no need to fetch anything more.
    if( !pickup_task &&
        are_requirements_nearby( already_there_spots, things_to_fetch_id, you, activity_to_restore,
                                 false, src_loc ) ) {
        return requirement_map;
    }
    // a vector of every item in every tile that matches any part of the requirements.
    // will be filtered for amounts/charges afterwards.
    for( const tripoint_bub_ms &point_elem : pickup_task ? loot_spots : combined_spots ) {
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
                            if( stack_elem.ammo_remaining( &you ) >= comp_elem.count ) {
                                temp_map[stack_elem.typeId()] += stack_elem.ammo_remaining( &you );
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
            requirement_map.emplace_back( point_elem, map_elem.first, map_elem.second );
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
                tripoint_bub_ms pos_here = std::get<0>( *it );
                itype_id item_here = std::get<1>( *it );
                int quantity_here = std::get<2>( *it );
                if( comp_elem.type == item_here ) {
                    item_quantity += quantity_here;
                }
                if( item_quantity >= quantity_required ) {
                    // it's just this spot that can fulfil the requirement on its own
                    final_map.emplace_back( pos_here, item_here, std::min<int>( quantity_here,
                                            quantity_required ) );
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
                    tripoint_bub_ms pos_here2 = std::get<0>( *it );
                    itype_id item_here2 = std::get<1>( *it );
                    int quantity_here2 = std::get<2>( *it );
                    if( comp_elem.type == item_here2 ) {
                        if( quantity_here2 >= remainder ) {
                            final_map.emplace_back( pos_here2, item_here2, remainder );
                            line_found = true;
                        } else {
                            final_map.emplace_back( pos_here2, item_here2, remainder );
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
                tripoint_bub_ms pos_here = std::get<0>( *it );
                itype_id item_here = std::get<1>( *it );
                int quantity_here = std::get<2>( *it );
                if( comp_elem.type == item_here ) {
                    item_quantity += quantity_here;
                }
                if( item_quantity >= quantity_required ) {
                    // it's just this spot that can fulfil the requirement on its own
                    final_map.emplace_back( pos_here, item_here, std::min<int>( quantity_here,
                                            quantity_required ) );
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
                    tripoint_bub_ms pos_here2 = std::get<0>( *it );
                    itype_id item_here2 = std::get<1>( *it );
                    int quantity_here2 = std::get<2>( *it );
                    if( comp_elem.type == item_here2 ) {
                        if( quantity_here2 >= remainder ) {
                            final_map.emplace_back( pos_here2, item_here2, remainder );
                            line_found = true;
                        } else {
                            final_map.emplace_back( pos_here2, item_here2, remainder );
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
                tripoint_bub_ms pos_here = std::get<0>( *it );
                itype_id item_here = std::get<1>( *it );
                item test_item = item( item_here, calendar::turn_zero );
                if( test_item.has_quality( tool_qual, qual_level ) ) {
                    // it's just this spot that can fulfil the requirement on its own
                    final_map.emplace_back( pos_here, item_here, 1 );
                    line_found = true;
                    break;
                }
                it++;
            }
        }
    }
    for( const std::tuple<tripoint_bub_ms, itype_id, int> &elem : final_map ) {
        add_msg_debug( debugmode::DF_REQUIREMENTS_MAP, "%s is fetching %s from %s ",
                       you.disp_name(),
                       std::get<1>( elem ).str(), std::get<0>( elem ).to_string_writable() );
    }
    return final_map;
}

namespace
{

void _tidy_move_items( Character &you, item_stack &stack, tripoint_bub_ms const &src_loc,
                       tripoint_bub_ms const &dst_loc, const std::optional<vpart_reference> &vpr_src,
                       activity_id const &activity_to_restore )
{
    for( item &it : stack ) {
        if( it.has_var( "activity_var" ) && it.get_var( "activity_var", "" ) == you.name ) {
            move_item( you, it, it.count(), src_loc, dst_loc, vpr_src, activity_to_restore );
            break;
        }
    }
}

} // namespace

static bool construction_activity( Character &you, const zone_data * /*zone*/,
                                   const tripoint_bub_ms &src_loc,
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
    // Use up the components
    for( const std::vector<item_comp> &it : built_chosen.requirements->get_components() ) {
        std::list<item> tmp = you.consume_items( it, 1, is_crafting_component );
        used.splice( used.end(), tmp );
    }
    pc.components = used;
    // TODO: fix point types
    here.partial_con_set( tripoint_bub_ms( src_loc ), pc );
    for( const std::vector<tool_comp> &it : built_chosen.requirements->get_tools() ) {
        you.consume_tools( it );
    }
    you.backlog.emplace_front( activity_to_restore );
    you.assign_activity( ACT_BUILD );
    you.activity.placement = here.getglobal( src_loc );
    return true;
}

static bool tidy_activity( Character &you, const tripoint_bub_ms &src_loc,
                           const activity_id &activity_to_restore,
                           const int distance = ACTIVITY_SEARCH_DISTANCE )
{
    zone_manager &mgr = zone_manager::get_manager();
    map &here = get_map();
    tripoint_abs_ms loot_abspos = here.getglobal( src_loc );
    tripoint_bub_ms loot_src_lot;
    const auto &zone_src_set = mgr.get_near( zone_type_LOOT_UNSORTED, loot_abspos, distance, nullptr,
                               _fac_id( you ) );
    if( !zone_src_set.empty() ) {
        const std::vector<tripoint_abs_ms> &zone_src_sorted = get_sorted_tiles_by_distance(
                    loot_abspos, zone_src_set );
        // Find the nearest unsorted zone to dump objects at
        for( const tripoint_abs_ms &src_elem : zone_src_sorted ) {
            if( !here.can_put_items_ter_furn( here.getlocal( src_elem ) ) ) {
                continue;
            }
            loot_src_lot = here.bub_from_abs( src_elem );
            break;
        }
    }
    if( loot_src_lot == tripoint_bub_ms() ) {
        return false;
    }
    if( const std::optional<vpart_reference> ovp = here.veh_at( src_loc ).cargo() ) {
        vehicle_stack vs = ovp->items();
        _tidy_move_items( you, vs, src_loc, loot_src_lot, ovp, activity_to_restore );
    }
    map_stack stack = here.i_at( src_loc );
    _tidy_move_items( you, stack, src_loc, loot_src_lot, std::nullopt, activity_to_restore );

    // we are adjacent to an unsorted zone, we came here to just drop items we are carrying
    if( mgr.has( zone_type_LOOT_UNSORTED, here.getglobal( src_loc ), _fac_id( you ) ) ) {
        for( item *inv_elem : you.inv_dump() ) {
            if( inv_elem->has_var( "activity_var" ) ) {
                inv_elem->erase_var( "activity_var" );
                put_into_vehicle_or_drop( you, item_drop_reason::deliberate, { *inv_elem }, src_loc );
                you.i_rem( inv_elem );
            }
        }
    }
    return true;
}

static bool fetch_activity(
    Character &you, const tripoint_bub_ms &src_loc, const activity_id &activity_to_restore,
    const int distance = ACTIVITY_SEARCH_DISTANCE )
{
    map &here = get_map();
    if( !here.can_put_items_ter_furn( here.getlocal( you.backlog.front().coords.back() ) ) ) {
        return false;
    }
    const std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> mental_map_2 =
                requirements_map( you, distance );
    int pickup_count = 1;
    map_stack items_there = here.i_at( src_loc );
    const units::volume volume_allowed = you.volume_capacity() - you.volume_carried();
    const units::mass weight_allowed = you.weight_capacity() - you.weight_carried();
    const std::optional<vpart_reference> ovp = here.veh_at( src_loc ).cargo();
    if( ovp ) {
        for( item &veh_elem : ovp->items() ) {
            for( auto elem : mental_map_2 ) {
                if( std::get<0>( elem ) == src_loc && veh_elem.typeId() == std::get<1>( elem ) ) {
                    if( !you.backlog.empty() && you.backlog.front().id() == ACT_MULTIPLE_CONSTRUCTION ) {
                        move_item( you, veh_elem, veh_elem.count_by_charges() ? std::get<2>( elem ) : 1, src_loc,
                                   here.bub_from_abs( you.backlog.front().coords.back() ), ovp, activity_to_restore );
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
                if( !you.backlog.empty() && ( you.backlog.front().id() == ACT_MULTIPLE_CONSTRUCTION ||
                                              you.backlog.front().id() == ACT_MULTIPLE_DIS ) ) {
                    move_item( you, it, it.count_by_charges() ? std::get<2>( elem ) : 1, src_loc,
                               here.bub_from_abs( you.backlog.front().coords.back() ), ovp, activity_to_restore );

                    return true;
                    // other tasks want the tool picked up
                } else if( !you.backlog.empty() && ( you.backlog.front().id() == ACT_MULTIPLE_FARM ||
                                                     you.backlog.front().id() == ACT_MULTIPLE_CHOP_PLANKS ||
                                                     you.backlog.front().id() == ACT_VEHICLE_DECONSTRUCTION ||
                                                     you.backlog.front().id() == ACT_VEHICLE_REPAIR ||
                                                     you.backlog.front().id() == ACT_MULTIPLE_BUTCHER ||
                                                     you.backlog.front().id() == ACT_MULTIPLE_CHOP_TREES ||
                                                     you.backlog.front().id() == ACT_MULTIPLE_FISH ||
                                                     you.backlog.front().id() == ACT_MULTIPLE_MINE ||
                                                     you.backlog.front().id() == ACT_MULTIPLE_MOP ) ) {
                    if( it.volume() > volume_allowed || it.weight() > weight_allowed ) {
                        add_msg_if_player_sees( you, _( "%1s failed to fetch tools." ), you.name );
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
                    it.set_var( "activity_var", you.name );
                    you.i_add( it );
                    if( you.is_npc() ) {
                        if( pickup_count == 1 ) {
                            const std::string item_name = it.tname();
                            add_msg( _( "%1$s picks up a %2$s." ), you.disp_name(), item_name );
                        } else {
                            add_msg( _( "%s picks up several items." ),  you.disp_name() );
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
    you.set_moves( 0 );
    you.activity = player_activity();
    you.backlog.clear();
    return false;
}

static bool butcher_corpse_activity( Character &you, const tripoint_bub_ms &src_loc,
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
            elem.set_var( "activity_var", you.name );
            you.assign_activity( ACT_BUTCHER_FULL, 0, true );
            // TODO: fix point types
            you.activity.targets.emplace_back( map_cursor( src_loc.raw() ), &elem );
            you.activity.placement = here.getglobal( src_loc );
            return true;
        }
    }
    return false;
}

static bool chop_plank_activity( Character &you, const tripoint_bub_ms &src_loc )
{
    item &best_qual = you.best_item_with_quality( qual_AXE );
    if( best_qual.is_null() ) {
        return false;
    }
    if( best_qual.type->can_have_charges() ) {
        you.consume_charges( best_qual, best_qual.type->charges_to_use() );
    }
    map &here = get_map();
    for( item &i : here.i_at( src_loc ) ) {
        if( i.typeId() == itype_log ) {
            here.i_rem( src_loc, &i );
            int moves = to_moves<int>( 20_minutes );
            you.add_msg_if_player( _( "You cut the log into planks." ) );
            you.assign_activity( chop_planks_activity_actor( moves ) );
            you.activity.placement = here.getglobal( src_loc );
            return true;
        }
    }
    return false;
}

void activity_on_turn_move_loot( player_activity &act, Character &you )
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
    const tripoint_abs_ms abspos = you.get_location();
    zone_manager &mgr = zone_manager::get_manager();
    if( here.check_vehicle_zones( here.get_abs_sub().z() ) ) {
        mgr.cache_vzones();
    }

    if( stage == INIT ) {
        // TODO: fix point types
        act.coord_set.clear();
        for( const tripoint_abs_ms &p :
             mgr.get_near( zone_type_LOOT_UNSORTED, abspos, ACTIVITY_SEARCH_DISTANCE, nullptr,
                           _fac_id( you ) ) ) {
            act.coord_set.insert( p.raw() );
        }
        stage = THINK;
    }

    if( stage == THINK ) {
        //initialize num_processed
        num_processed = 0;
        // TODO: fix point types
        std::vector<tripoint_abs_ms> src_set;
        src_set.reserve( act.coord_set.size() );
        for( const tripoint &p : act.coord_set ) {
            src_set.emplace_back( p );
        }
        // sort source tiles by distance
        const auto &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );

        for( const tripoint_abs_ms &src : src_sorted ) {
            act.placement = src;
            // TODO: fix point types
            act.coord_set.erase( src.raw() );

            const tripoint_bub_ms src_loc = here.bub_from_abs( src );
            if( !here.inbounds( src_loc ) ) {
                if( !here.inbounds( you.pos() ) ) {
                    // p is implicitly an NPC that has been moved off the map, so reset the activity
                    // and unload them
                    you.cancel_activity();
                    you.assign_activity( ACT_MOVE_LOOT );
                    you.set_moves( 0 );
                    g->reload_npcs();
                    return;
                }
                std::vector<tripoint_bub_ms> route;
                route = here.route( you.pos_bub(), src_loc, you.get_pathfinding_settings(),
                                    you.get_path_avoid() );
                if( route.empty() ) {
                    // can't get there, can't do anything, skip it
                    continue;
                }
                stage = DO;
                you.set_destination( route, act );
                you.activity.set_to_null();
                return;
            }

            // skip tiles in IGNORE zone and tiles on fire
            // (to prevent taking out wood off the lit brazier)
            // and inaccessible furniture, like filled charcoal kiln
            if( mgr.has( zone_type_LOOT_IGNORE, src, _fac_id( you ) ) ||
                here.get_field( src_loc, fd_fire ) != nullptr ||
                !here.can_put_items_ter_furn( src_loc ) ) {
                continue;
            }

            //nothing to sort?
            const std::optional<vpart_reference> vp = here.veh_at( src_loc ).cargo();
            if( ( !vp || vp->items().empty() ) && here.i_at( src_loc ).empty() ) {
                continue;
            }

            bool is_adjacent_or_closer = square_dist( you.pos_bub(), src_loc ) <= 1;
            // before we move any item, check if player is at or
            // adjacent to the loot source tile
            if( !is_adjacent_or_closer ) {
                std::vector<tripoint_bub_ms> route;
                bool adjacent = false;

                // get either direct route or route to nearest adjacent tile if
                // source tile is impassable
                if( here.passable( src_loc ) ) {
                    route = here.route( you.pos_bub(), src_loc, you.get_pathfinding_settings(),
                                        you.get_path_avoid() );
                } else {
                    // impassable source tile (locker etc.),
                    // get route to nearest adjacent tile instead
                    route = route_adjacent( you, src_loc );
                    adjacent = true;
                }

                // check if we found path to source / adjacent tile
                if( route.empty() ) {
                    add_msg( m_info, _( "%s can't reach the source tile.  Try to sort out loot without a cart." ),
                             you.disp_name() );
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
                you.set_destination( route, act );
                you.activity.set_to_null();
                return;
            }
            stage = DO;
            break;
        }
    }
    if( stage == DO ) {
        // TODO: fix point types
        const tripoint_abs_ms src( act.placement );
        const tripoint_bub_ms src_loc = here.bub_from_abs( src );

        bool is_adjacent_or_closer = square_dist( you.pos_bub(), src_loc ) <= 1;
        // before we move any item, check if player is at or
        // adjacent to the loot source tile
        if( !is_adjacent_or_closer ) {
            stage = THINK;
            return;
        }

        // the boolean in this pair being true indicates the item is from a vehicle storage space
        auto items = std::vector<std::pair<item *, bool>>();
        const std::optional<vpart_reference> vpr = here.veh_at( src_loc ).cargo();
        //Check source for cargo part
        //map_stack and vehicle_stack are different types but inherit from item_stack
        // TODO: use one for loop
        if( vpr ) {
            for( item &it : vpr->items() ) {
                items.emplace_back( &it, true );
            }
        }
        for( item &it : here.i_at( src_loc ) ) {
            items.emplace_back( &it, false );
        }

        bool unload_mods = false;
        bool unload_molle = false;
        bool unload_always = false;

        std::vector<zone_data const *> const zones = mgr.get_zones_at( src, zone_type_zone_unload_all,
                _fac_id( you ) );

        // get most open rules out of all stacked zones
        for( zone_data const *zone : zones ) {
            unload_options const &options = dynamic_cast<const unload_options &>( zone->get_options() );
            unload_molle |= options.unload_molle();
            unload_mods |= options.unload_mods();
            unload_always |= options.unload_always();
        }

        //Skip items that have already been processed
        for( auto it = items.begin() + num_processed; it < items.end(); ++it ) {
            ++num_processed;
            item &thisitem = *it->first;

            // skip unpickable liquid
            if( !thisitem.made_of_from_type( phase_id::SOLID ) ) {
                continue;
            }

            // skip favorite items in ignore favorite zones
            if( thisitem.is_favorite && mgr.has( zone_type_LOOT_IGNORE_FAVORITES, src, _fac_id( you ) ) ) {
                continue;
            }

            // Only if it's from a vehicle do we use the vehicle source location information.
            const std::optional<vpart_reference> vpr_src = it->second ? vpr : std::nullopt;
            const zone_type_id id = mgr.get_near_zone_type_for_item( thisitem, abspos,
                                    ACTIVITY_SEARCH_DISTANCE, _fac_id( you ) );

            // checks whether the item is already on correct loot zone or not
            // if it is, we can skip such item, if not we move the item to correct pile
            // think empty bag on food pile, after you ate the content
            if( id != zone_type_LOOT_CUSTOM && mgr.has( id, src, _fac_id( you ) ) ) {
                continue;
            }

            if( id == zone_type_LOOT_CUSTOM &&
                mgr.custom_loot_has( src, &thisitem, zone_type_LOOT_CUSTOM, _fac_id( you ) ) ) {
                continue;
            }

            const std::unordered_set<tripoint_abs_ms> dest_set =
                mgr.get_near( id, abspos, ACTIVITY_SEARCH_DISTANCE, &thisitem, _fac_id( you ) );

            // if this item isn't going anywhere and its not sealed
            // check if it is in a unload zone or a strip corpse zone
            // then we should unload it and see what is inside
            bool move_and_reset = false;
            bool moved_something = false;

            if( mgr.has_near( zone_type_zone_unload_all, abspos, 1, _fac_id( you ) ) ||
                ( mgr.has_near( zone_type_zone_strip, abspos, 1, _fac_id( you ) ) && it->first->is_corpse() ) ) {
                if( dest_set.empty() || unload_always ) {
                    if( you.rate_action_unload( *it->first ) == hint_rating::good &&
                        !it->first->any_pockets_sealed() ) {
                        for( item *contained : it->first->all_items_top( item_pocket::pocket_type::CONTAINER ) ) {
                            // no liquids don't want to spill stuff
                            if( !contained->made_of( phase_id::LIQUID ) && !contained->made_of( phase_id::GAS ) ) {
                                move_item( you, *contained, contained->count(), src_loc, src_loc, vpr_src );
                                it->first->remove_item( *contained );
                            }
                        }
                        for( item *contained : it->first->all_items_top( item_pocket::pocket_type::MAGAZINE ) ) {
                            // no liquids don't want to spill stuff
                            if( !contained->made_of( phase_id::LIQUID ) && !contained->made_of( phase_id::GAS ) ) {
                                if( it->first->is_ammo_belt() ) {
                                    if( it->first->type->magazine->linkage ) {
                                        item link( *it->first->type->magazine->linkage, calendar::turn, contained->count() );
                                        if( vpr_src ) {
                                            vpr_src->vehicle().add_item( vpr_src->part(), link );
                                        } else {
                                            here.add_item_or_charges( src_loc, link );
                                        }
                                    }
                                }
                                move_item( you, *contained, contained->count(), src_loc, src_loc, vpr_src );
                                it->first->remove_item( *contained );
                            }
                        }
                        for( item *contained : it->first->all_items_top( item_pocket::pocket_type::MAGAZINE_WELL ) ) {
                            // no liquids don't want to spill stuff
                            if( !contained->made_of( phase_id::LIQUID ) && !contained->made_of( phase_id::GAS ) ) {
                                move_item( you, *contained, contained->count(), src_loc, src_loc, vpr_src );
                                it->first->remove_item( *contained );
                            }
                        }
                        moved_something = true;

                    }

                    // if unloading mods
                    if( unload_mods ) {
                        // remove each mod, skip irremovable
                        for( item *mod : it->first->gunmods() ) {
                            if( mod->is_irremovable() ) {
                                continue;
                            }
                            you.gunmod_remove( *it->first, *mod );
                            // need to return so the activity starts
                            return;
                        }
                    }

                    // if unloading molle
                    if( unload_molle ) {
                        while( !it->first->get_contents().get_added_pockets().empty() ) {
                            item removed = it->first->get_contents().remove_pocket( 0 );
                            move_item( you, removed, 1, src_loc, src_loc, vpr_src );
                            moved_something = true;
                        }
                    }
                    if( it->first->has_flag( flag_MAG_DESTROY ) && it->first->ammo_remaining() == 0 ) {
                        if( vpr_src ) {
                            vpr_src->vehicle().remove_item( vpr_src->part(), it->first );
                        } else {
                            here.i_rem( src_loc, it->first );
                        }
                        num_processed = std::max( num_processed - 1, 0 );
                        return;
                    }

                    // after dumping items go back to start of activity loop
                    // so that can re-assess the items in the tile
                    // perhaps move the last item first however
                    if( unload_always && moved_something ) {
                        move_and_reset = true;
                    } else if( moved_something ) {
                        return;
                    }

                }

            }

            for( const tripoint_abs_ms &dest : dest_set ) {
                const tripoint_bub_ms dest_loc = here.bub_from_abs( dest );
                units::volume free_space;

                //Check destination for cargo part
                if( const std::optional<vpart_reference> ovp = here.veh_at( dest_loc ).cargo() ) {
                    free_space = ovp->items().free_volume();
                } else {
                    free_space = here.free_volume( dest_loc );
                }

                // skip tiles with inaccessible furniture, like filled charcoal kiln
                if( !here.can_put_items_ter_furn( dest_loc ) ||
                    static_cast<int>( here.i_at( dest_loc ).size() ) >= MAX_ITEM_IN_SQUARE ) {
                    continue;
                }

                // check free space at destination
                if( free_space >= thisitem.volume() ) {
                    move_item( you, thisitem, thisitem.count(), src_loc, dest_loc, vpr_src );

                    // moved item away from source so decrement
                    if( num_processed > 0 ) {
                        --num_processed;
                    }
                    break;
                }
            }
            if( you.moves <= 0 || move_and_reset ) {
                return;
            }
        }

        //this location is sorted
        stage = THINK;
        return;
    }

    // If we got here without restarting the activity, it means we're done
    add_msg( m_info, _( "%s sorted out every item possible." ), you.disp_name( false, true ) );
    if( you.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &you );
        guy->revert_after_activity();
    }
    you.activity.set_to_null();
}

static int chop_moves( Character &you, item &it )
{
    // quality of tool
    const int quality = it.get_quality( qual_AXE );

    // attribute; regular tools - based on STR, powered tools - based on DEX
    const int attr = it.has_flag( flag_POWERED ) ? you.dex_cur : you.get_arm_str();

    int moves = to_moves<int>( time_duration::from_minutes( 60 - attr ) / std::pow( 2, quality - 1 ) );
    const int helpersize = you.get_num_crafting_helpers( 3 );
    moves *= ( 1.0f - ( helpersize / 10.0f ) );
    return moves;
}

static bool mine_activity( Character &you, const tripoint_bub_ms &src_loc )
{
    std::vector<item *> mining_inv = you.items_with( [&you]( const item & itm ) {
        return ( itm.has_flag( flag_DIG_TOOL ) && !itm.type->can_use( "JACKHAMMER" ) ) ||
               ( itm.type->can_use( "JACKHAMMER" ) && itm.ammo_sufficient( &you ) );
    } );
    map &here = get_map();
    if( mining_inv.empty() || you.is_mounted() || you.is_underwater() || here.veh_at( src_loc ) ||
        !here.has_flag( ter_furn_flag::TFLAG_MINEABLE, src_loc ) || you.has_effect( effect_incorporeal ) ) {
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
        moves += ( ( MAX_STAT + 4 ) - std::min( you.get_arm_str(),
                                                MAX_STAT ) ) * to_moves<int>( 5_minutes );
    }
    if( here.move_cost( src_loc ) == 2 ) {
        // We're breaking up some flat surface like pavement, which is much easier
        moves /= 2;
    }
    you.assign_activity( powered ? ACT_JACKHAMMER : ACT_PICKAXE, moves );
    you.activity.targets.emplace_back( you, chosen_item );
    you.activity.placement = here.getglobal( src_loc );
    return true;

}

// Not really an activity like the others; relies on zone activity alerting on enemies
static bool mop_activity( Character &you, const tripoint_bub_ms &src_loc )
{
    // iuse::mop costs 15 moves per use
    you.assign_activity( mop_activity_actor( 15 ) );
    you.activity.placement = get_map().getglobal( src_loc );
    return true;
}

static bool chop_tree_activity( Character &you, const tripoint_bub_ms &src_loc )
{
    item &best_qual = you.best_item_with_quality( qual_AXE );
    if( best_qual.is_null() ) {
        return false;
    }
    int moves = chop_moves( you, best_qual );
    if( best_qual.type->can_have_charges() ) {
        you.consume_charges( best_qual, best_qual.type->charges_to_use() );
    }
    map &here = get_map();
    const ter_id ter = here.ter( src_loc );
    if( here.has_flag( ter_furn_flag::TFLAG_TREE, src_loc ) ) {
        you.assign_activity( chop_tree_activity_actor( moves, item_location( you, &best_qual ) ) );
        you.activity.placement = here.getglobal( src_loc );
        return true;
    } else if( ter == t_trunk || ter == t_stump ) {
        you.assign_activity( chop_logs_activity_actor( moves, item_location( you, &best_qual ) ) );
        you.activity.placement = here.getglobal( src_loc );
        return true;
    }
    return false;
}

static void check_npc_revert( Character &you )
{
    if( you.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &you );
        if( guy ) {
            guy->revert_after_activity();
        }
    }
}

static zone_type_id get_zone_for_act( const tripoint_bub_ms &src_loc, const zone_manager &mgr,
                                      const activity_id &act_id, const faction_id &fac_id )
{
    zone_type_id ret = zone_type_;
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
    if( act_id == ACT_MULTIPLE_MOP ) {
        ret = zone_type_MOPPING;
    }
    if( act_id == ACT_MULTIPLE_DIS ) {
        ret = zone_type_zone_disassemble;
    }
    if( src_loc != tripoint_bub_ms() && act_id == ACT_FETCH_REQUIRED ) {
        const zone_data *zd = mgr.get_zone_at( get_map().getglobal( src_loc ), false, fac_id );
        if( zd ) {
            ret = zd->get_type();
        }
    }
    return ret;
}

/** Determine all locations for this generic activity */
/** Returns locations */
static std::unordered_set<tripoint_abs_ms> generic_multi_activity_locations(
    Character &you, const activity_id &act_id )
{
    bool dark_capable = false;
    std::unordered_set<tripoint_abs_ms> src_set;

    zone_manager &mgr = zone_manager::get_manager();
    const tripoint_bub_ms localpos = you.pos_bub();
    map &here = get_map();
    const tripoint_abs_ms abspos = here.getglobal( localpos );
    if( act_id == ACT_TIDY_UP ) {
        dark_capable = true;
        tripoint unsorted_spot;
        std::unordered_set<tripoint_abs_ms> unsorted_set =
            mgr.get_near( zone_type_LOOT_UNSORTED, abspos, ACTIVITY_SEARCH_DISTANCE, nullptr, _fac_id( you ) );
        if( !unsorted_set.empty() ) {
            unsorted_spot = here.getlocal( random_entry( unsorted_set ) );
        }
        bool found_one_point = false;
        bool found_route = true;
        for( const tripoint_bub_ms &elem : here.points_in_radius( localpos,
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
                if( stack_elem.has_var( "activity_var" ) && stack_elem.get_var( "activity_var", "" ) == you.name ) {
                    const furn_t &f = here.furn( elem ).obj();
                    if( !f.has_flag( ter_furn_flag::TFLAG_PLANT ) ) {
                        src_set.insert( here.getglobal( elem ) );
                        found_one_point = true;
                        // only check for a valid path, as that is all that is needed to tidy something up.
                        if( square_dist( you.pos_bub(), elem ) > 1 ) {
                            std::vector<tripoint_bub_ms> route = route_adjacent( you, elem );
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
            for( const item *inv_elem : you.inv_dump() ) {
                if( inv_elem->has_var( "activity_var" ) ) {
                    // we've gone to tidy up all the things lying around, now tidy up the things we picked up.
                    src_set.insert( here.getglobal( unsorted_spot ) );
                    break;
                }
            }
        }
    } else if( act_id == ACT_MULTIPLE_READ ) {
        // anywhere well lit
        for( const tripoint_bub_ms &elem : here.points_in_radius( localpos, ACTIVITY_SEARCH_DISTANCE ) ) {
            src_set.insert( here.getglobal( elem ) );
        }
    } else if( act_id == ACT_MULTIPLE_CRAFT ) {
        // Craft only with what is on the spot
        // TODO: add zone type like zone_type_CRAFT?
        src_set.insert( here.getglobal( localpos ) );
    } else if( act_id != ACT_FETCH_REQUIRED ) {
        zone_type_id zone_type = get_zone_for_act( tripoint_bub_ms{}, mgr, act_id, _fac_id( you ) );
        src_set = mgr.get_near( zone_type, abspos, ACTIVITY_SEARCH_DISTANCE, nullptr, _fac_id( you ) );
        // multiple construction will form a list of targets based on blueprint zones and unfinished constructions
        if( act_id == ACT_MULTIPLE_CONSTRUCTION ) {
            for( const tripoint_bub_ms &elem : here.points_in_radius( localpos, ACTIVITY_SEARCH_DISTANCE ) ) {
                partial_con *pc = here.partial_con_at( elem );
                if( pc ) {
                    src_set.insert( here.getglobal( elem ) );
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
        std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> mental_map =
                    requirements_map( you, ACTIVITY_SEARCH_DISTANCE );
        for( const auto &elem : mental_map ) {
            const tripoint_bub_ms &elem_point = std::get<0>( elem );
            src_set.insert( here.getglobal( elem_point ) );
        }
    }
    // prune the set to remove tiles that are never gonna work out.
    const bool pre_dark_check = src_set.empty();
    const bool MOP_ACTIVITY = act_id == ACT_MULTIPLE_MOP;
    if( MOP_ACTIVITY ) {
        dark_capable = true;
    }

    for( auto it2 = src_set.begin(); it2 != src_set.end(); ) {
        // remove dangerous tiles
        const tripoint set_pt = here.getlocal( *it2 );
        if( MOP_ACTIVITY ) {
            if( !here.mopsafe_field_at( set_pt ) ) {
                it2 = src_set.erase( it2 );
                continue;
            }
        } else {
            if( here.dangerous_field_at( set_pt ) ) {
                it2 = src_set.erase( it2 );
                continue;
            }
        }
        // remove tiles in darkness, if we aren't lit-up ourselves
        if( !dark_capable && you.fine_detail_vision_mod( set_pt ) > 4.0 ) {
            it2 = src_set.erase( it2 );
            continue;
        }
        if( act_id == ACT_MULTIPLE_FISH ) {
            const ter_id terrain_id = here.ter( set_pt );
            if( !terrain_id.obj().has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) ) {
                it2 = src_set.erase( it2 );
            } else {
                ++it2;
            }
        } else //  Exclude activities that can't have multiple characters working on the same tile.
            if( act_id == ACT_MULTIPLE_CHOP_TREES ||
                act_id == ACT_MULTIPLE_CONSTRUCTION ||
                act_id == ACT_MULTIPLE_MINE ) {
                bool skip = false;

                for( const npc &guy : g->all_npcs() ) {
                    if( &guy == &you ) {
                        continue;
                    }
                    if( guy.has_player_activity() && tripoint_abs_ms( guy.activity.placement ) == *it2 ) {
                        it2 = src_set.erase( it2 );
                        skip = true;
                        break;
                    }
                }
                if( skip ) {
                    continue;
                } else {
                    ++it2;
                }
            } else {
                ++it2;
            }
    }
    const bool post_dark_check = src_set.empty();
    if( !pre_dark_check && post_dark_check && !MOP_ACTIVITY ) {
        you.add_msg_if_player( m_info, _( "It is too dark to do this activity." ) );
    }
    return src_set;
}

/** Check if this activity can not be done immediately because it has some requirements */
static requirement_check_result generic_multi_activity_check_requirement(
    Character &you, const activity_id &act_id, activity_reason_info &act_info,
    const tripoint_abs_ms &src, const tripoint_bub_ms &src_loc,
    const std::unordered_set<tripoint_abs_ms> &src_set, const bool check_only = false )
{
    map &here = get_map();
    const tripoint_abs_ms abspos = here.getglobal( you.pos() );
    zone_manager &mgr = zone_manager::get_manager();

    bool &can_do_it = act_info.can_do;
    const do_activity_reason &reason = act_info.reason;
    const zone_data *zone =
        mgr.get_zone_at( src, get_zone_for_act( src_loc, mgr, act_id, _fac_id( you ) ),
                         _fac_id( you ) );

    const bool needs_to_be_in_zone = act_id == ACT_FETCH_REQUIRED ||
                                     act_id == ACT_MULTIPLE_FARM ||
                                     act_id == ACT_MULTIPLE_BUTCHER ||
                                     act_id == ACT_MULTIPLE_CHOP_PLANKS ||
                                     act_id == ACT_MULTIPLE_CHOP_TREES ||
                                     act_id == ACT_VEHICLE_DECONSTRUCTION ||
                                     act_id == ACT_VEHICLE_REPAIR ||
                                     act_id == ACT_MULTIPLE_FISH ||
                                     act_id == ACT_MULTIPLE_MINE ||
                                     act_id == ACT_MULTIPLE_DIS ||
                                     ( act_id == ACT_MULTIPLE_CONSTRUCTION &&
                                       // TODO: fix point types
                                       !here.partial_con_at( tripoint_bub_ms( src_loc ) ) );
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
            you.add_msg_if_player( m_info, _( "You don't have the skill for this task." ) );
        } else if( reason == do_activity_reason::BLOCKING_TILE ) {
            you.add_msg_if_player( m_info, _( "There is something blocking the location for this task." ) );
        }
        return requirement_check_result::SKIP_LOCATION;
    } else if( reason == do_activity_reason::NO_COMPONENTS ||
               reason == do_activity_reason::NEEDS_CUT_HARVESTING ||
               reason == do_activity_reason::NEEDS_PLANTING ||
               reason == do_activity_reason::NEEDS_TILLING ||
               reason == do_activity_reason::NEEDS_CHOPPING ||
               reason == do_activity_reason::NEEDS_BUTCHERING ||
               reason == do_activity_reason::NEEDS_BIG_BUTCHERING ||
               reason == do_activity_reason::NEEDS_VEH_DECONST ||
               reason == do_activity_reason::NEEDS_VEH_REPAIR ||
               reason == do_activity_reason::NEEDS_TREE_CHOPPING ||
               reason == do_activity_reason::NEEDS_FISHING || reason == do_activity_reason::NEEDS_MINING ||
               reason == do_activity_reason::NEEDS_CRAFT ||
               reason == do_activity_reason::NEEDS_DISASSEMBLE ) {
        // we can do it, but we need to fetch some stuff first
        // before we set the task to fetch components - is it even worth it? are the components anywhere?
        if( you.is_npc() ) {
            add_msg_if_player_sees( you, m_info, _( "%s is trying to find necessary items to do the job" ),
                                    you.disp_name() );
        }
        requirement_id what_we_need;
        std::vector<tripoint_bub_ms> loot_zone_spots;
        std::vector<tripoint_bub_ms> combined_spots;
        // TODO: fix point types
        for( const tripoint &elem : mgr.get_point_set_loot(
                 abspos, ACTIVITY_SEARCH_DISTANCE, you.is_npc(), _fac_id( you ) ) ) {
            loot_zone_spots.emplace_back( elem );
            combined_spots.emplace_back( elem );
        }
        for( const tripoint_bub_ms &elem : here.points_in_radius( src_loc, PICKUP_RANGE - 1,
                PICKUP_RANGE - 1 ) ) {
            combined_spots.push_back( elem );
        }
        add_basecamp_storage_to_loot_zone_list( mgr, src_loc, you, loot_zone_spots, combined_spots );

        if( reason == do_activity_reason::NO_COMPONENTS && act_id == ACT_MULTIPLE_CONSTRUCTION ) {
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
                you.activity_vehicle_part_index = 1;
                return requirement_check_result::SKIP_LOCATION;
            }
            requirement_data reqs;
            if( you.activity_vehicle_part_index >= 0 &&
                you.activity_vehicle_part_index < static_cast<int>( veh->part_count() ) ) {
                const vpart_info &vpi = veh->part( you.activity_vehicle_part_index ).info();
                if( reason == do_activity_reason::NEEDS_VEH_DECONST ) {
                    reqs = vpi.removal_requirements();
                } else if( reason == do_activity_reason::NEEDS_VEH_REPAIR ) {
                    reqs = vpi.repair_requirements();
                }
            }
            const std::string ran_str = random_string( 10 );
            const requirement_id req_id( ran_str );
            requirement_data::save_requirement( reqs, req_id );
            what_we_need = req_id;
        } else if( reason == do_activity_reason::NEEDS_MINING ) {
            what_we_need = requirement_data_mining_standard;
        } else if( reason == do_activity_reason::NEEDS_TILLING ||
                   reason == do_activity_reason::NEEDS_PLANTING ||
                   reason == do_activity_reason::NEEDS_CHOPPING ||
                   reason == do_activity_reason::NEEDS_CUT_HARVESTING ||
                   reason == do_activity_reason::NEEDS_BUTCHERING ||
                   reason == do_activity_reason::NEEDS_BIG_BUTCHERING ||
                   reason == do_activity_reason::NEEDS_TREE_CHOPPING ||
                   reason == do_activity_reason::NEEDS_FISHING ||
                   reason == do_activity_reason::NEEDS_CRAFT ||
                   reason == do_activity_reason::NEEDS_DISASSEMBLE ) {
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
            } else if( reason == do_activity_reason::NEEDS_CUT_HARVESTING ) {
                quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( qual_GRASS_CUT, 1, 1 ) } );
            } else if( reason == do_activity_reason::NEEDS_BUTCHERING ||
                       reason == do_activity_reason::NEEDS_BIG_BUTCHERING ) {
                quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( qual_BUTCHER, 1, 1 ) } );
                if( reason == do_activity_reason::NEEDS_BIG_BUTCHERING ) {
                    quality_comp_vector.push_back( std::vector<quality_requirement> { quality_requirement( qual_SAW_M, 1, 1 ), quality_requirement( qual_SAW_W, 1, 1 ) } );
                }

            } else if( reason == do_activity_reason::NEEDS_FISHING ) {
                quality_comp_vector.push_back( std::vector<quality_requirement> {quality_requirement( qual_FISHING_ROD, 1, 1 )} );
            } else if( reason == do_activity_reason::NEEDS_CRAFT ) {
                requirement_comp_vector = act_info.req.get_components();
                quality_comp_vector = act_info.req.get_qualities();
                tool_comp_vector = act_info.req.get_tools();
            } else if( reason == do_activity_reason::NEEDS_DISASSEMBLE ) {
                quality_comp_vector = act_info.req.get_qualities();
                tool_comp_vector = act_info.req.get_tools();
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
                           reason == do_activity_reason::NEEDS_CUT_HARVESTING ||
                           reason == do_activity_reason::NEEDS_BUTCHERING ||
                           reason == do_activity_reason::NEEDS_BIG_BUTCHERING ||
                           reason == do_activity_reason::NEEDS_TREE_CHOPPING ||
                           reason == do_activity_reason::NEEDS_VEH_DECONST ||
                           reason == do_activity_reason::NEEDS_VEH_REPAIR ||
                           reason == do_activity_reason::NEEDS_MINING;
        // is it even worth fetching anything if there isn't enough nearby?
        if( !are_requirements_nearby( tool_pickup ? loot_zone_spots : combined_spots, what_we_need, you,
                                      act_id, tool_pickup, src_loc ) ) {
            you.add_msg_player_or_npc( m_info,
                                       _( "The required items are not available to complete this task." ),
                                       _( "The required items are not available to complete this task." ) );
            if( reason == do_activity_reason::NEEDS_VEH_DECONST ||
                reason == do_activity_reason::NEEDS_VEH_REPAIR ) {
                you.activity_vehicle_part_index = -1;
            }
            return requirement_check_result::SKIP_LOCATION;
        } else {
            if( !check_only ) {
                if( you.as_npc() && you.as_npc()->job.fetch_history.count( what_we_need.str() ) != 0 &&
                    you.as_npc()->job.fetch_history[what_we_need.str()] == calendar::turn ) {
                    // this may be faild fetch already. give up task for a while to avoid infinite loop.
                    you.activity = player_activity();
                    you.backlog.clear();
                    check_npc_revert( you );
                    return requirement_check_result::SKIP_LOCATION;
                }
                you.backlog.emplace_front( act_id );
                you.assign_activity( ACT_FETCH_REQUIRED );
                player_activity &act_prev = you.backlog.front();
                act_prev.str_values.push_back( what_we_need.str() );
                act_prev.values.push_back( static_cast<int>( reason ) );
                // come back here after successfully fetching your stuff
                if( act_prev.coords.empty() ) {
                    std::vector<tripoint_bub_ms> local_src_set;
                    local_src_set.reserve( src_set.size() );
                    for( const tripoint_abs_ms &elem : src_set ) {
                        local_src_set.push_back( here.bub_from_abs( elem ) );
                    }
                    std::vector<tripoint_bub_ms> candidates;
                    for( const tripoint_bub_ms &point_elem :
                         here.points_in_radius( src_loc, /*radius=*/PICKUP_RANGE - 1, /*radiusz=*/0 ) ) {
                        // we don't want to place the components where they could interfere with our ( or someone else's ) construction spots
                        if( !you.sees( point_elem ) || ( std::find( local_src_set.begin(), local_src_set.end(),
                                                         point_elem ) != local_src_set.end() ) || !here.can_put_items_ter_furn( point_elem ) ) {
                            continue;
                        }
                        candidates.push_back( point_elem );
                    }
                    if( candidates.empty() ) {
                        you.activity = player_activity();
                        you.backlog.clear();
                        check_npc_revert( you );
                        return requirement_check_result::SKIP_LOCATION;
                    }
                    act_prev.coords.push_back(
                        here.getabs(
                            candidates[std::max( 0, static_cast<int>( candidates.size() / 2 ) )] )
                    );
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
static bool generic_multi_activity_do(
    Character &you, const activity_id &act_id, const activity_reason_info &act_info,
    const tripoint_abs_ms &src, const tripoint_bub_ms &src_loc )
{
    // If any of the following activities return without processing
    // then they MUST return true here, to stop infinite loops.
    zone_manager &mgr = zone_manager::get_manager();

    const do_activity_reason &reason = act_info.reason;
    const zone_data *zone =
        mgr.get_zone_at( src, get_zone_for_act( src_loc, mgr, act_id, _fac_id( you ) ), _fac_id( you ) );
    map &here = get_map();
    // something needs to be done, now we are there.
    // it was here earlier, in the space of one turn, maybe it got harvested by someone else.
    if( ( ( reason == do_activity_reason::NEEDS_HARVESTING ) ||
          ( reason == do_activity_reason::NEEDS_CUT_HARVESTING ) ) &&
        here.has_flag_furn( ter_furn_flag::TFLAG_GROWTH_HARVEST, src_loc ) ) {
        // TODO: fix point types
        iexamine::harvest_plant( you, src_loc.raw(), true );
    } else if( reason == do_activity_reason::NEEDS_TILLING &&
               here.has_flag( ter_furn_flag::TFLAG_PLOWABLE, src_loc ) &&
               you.has_quality( qual_DIG, 1 ) && !here.has_furn( src_loc ) ) {
        you.assign_activity( churn_activity_actor( 18000, item_location() ) );
        you.backlog.emplace_front( act_id );
        you.activity.placement = src;
        return false;
    } else if( reason == do_activity_reason::NEEDS_PLANTING &&
               here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_PLANTABLE, src_loc ) ) {
        std::vector<zone_data> zones = mgr.get_zones( zone_type_FARM_PLOT, src, _fac_id( you ) );
        for( const zone_data &zone : zones ) {
            const itype_id seed =
                dynamic_cast<const plot_options &>( zone.get_options() ).get_seed();
            std::vector<item *> seed_inv = you.items_with( [seed]( const item & itm ) {
                return itm.typeId() == itype_id( seed );
            } );
            // we don't have the required seed, even though we should at this point.
            // move onto the next tile, and if need be that will prompt a fetch seeds activity.
            if( seed_inv.empty() ) {
                continue;
            }
            // TODO: fix point types
            iexamine::plant_seed( you, src_loc.raw(), itype_id( seed ) );
            you.backlog.emplace_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_CHOPPING && you.has_quality( qual_AXE, 1 ) ) {
        if( chop_plank_activity( you, src_loc ) ) {
            you.backlog.emplace_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_BUTCHERING ||
               reason == do_activity_reason::NEEDS_BIG_BUTCHERING ) {
        if( butcher_corpse_activity( you, src_loc, reason ) ) {
            you.backlog.emplace_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_BOOK_TO_LEARN ) {
        const item_filter filter = [ &you ]( const item & i ) {
            read_condition_result condition = you.check_read_condition( i );
            return condition == read_condition_result::SUCCESS;
        };
        std::vector<item *> books = you.items_with( filter );
        if( !books.empty() && books[0] ) {
            const time_duration time_taken = you.time_to_read( *books[0], you );
            item_location book = item_location( you, books[0] );
            item_location ereader;
            you.backlog.emplace_front( act_id );
            you.assign_activity( read_activity_actor( time_taken, book, ereader, true ) );
            return false;
        }
    } else if( reason == do_activity_reason::CAN_DO_CONSTRUCTION ) {
        if( here.partial_con_at( src_loc ) ) {
            you.backlog.emplace_front( act_id );
            you.assign_activity( ACT_BUILD );
            you.activity.placement = src;
            return false;
        }
        if( construction_activity( you, zone, src_loc, act_info, act_id ) ) {
            return false;
        }
    } else if( reason == do_activity_reason::CAN_DO_FETCH && act_id == ACT_TIDY_UP ) {
        if( !tidy_activity( you, src_loc, act_id, ACTIVITY_SEARCH_DISTANCE ) ) {
            return false;
        }
    } else if( reason == do_activity_reason::CAN_DO_FETCH && act_id == ACT_FETCH_REQUIRED ) {
        if( fetch_activity( you, src_loc, act_id, ACTIVITY_SEARCH_DISTANCE ) ) {
            if( !you.is_npc() ) {
                // Npcs will automatically start the next thing in the backlog, players need to be manually prompted
                // Because some player activities are necessarily not marked as auto-resume.
                activity_handlers::resume_for_multi_activities( you );
            }
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_TREE_CHOPPING && you.has_quality( qual_AXE, 1 ) ) {
        if( chop_tree_activity( you, src_loc ) ) {
            you.backlog.emplace_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_FISHING && you.has_quality( qual_FISHING_ROD, 1 ) ) {
        you.backlog.emplace_front( act_id );
        // we don't want to keep repeating the fishing activity, just piggybacking on this functions structure to find requirements.
        you.activity = player_activity();
        item &best_rod = you.best_item_with_quality( qual_FISHING_ROD );
        you.assign_activity( ACT_FISH, to_moves<int>( 5_hours ), 0,
                             0, best_rod.tname() );
        you.activity.targets.emplace_back( you, &best_rod );
        // TODO: fix point types
        you.activity.coord_set =
            g->get_fishable_locations( ACTIVITY_SEARCH_DISTANCE, src_loc.raw() );
        return false;
    } else if( reason == do_activity_reason::NEEDS_MINING ) {
        // if have enough batteries to continue etc.
        you.backlog.emplace_front( act_id );
        if( mine_activity( you, src_loc ) ) {
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_MOP ) {
        if( mop_activity( you, src_loc ) ) {
            you.backlog.emplace_front( act_id );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_VEH_DECONST ) {
        if( vehicle_activity( you, src_loc, you.activity_vehicle_part_index, 'o' ) ) {
            you.backlog.emplace_front( act_id );
            return false;
        }
        you.activity_vehicle_part_index = -1;
    } else if( reason == do_activity_reason::NEEDS_VEH_REPAIR ) {
        if( vehicle_activity( you, src_loc, you.activity_vehicle_part_index, 'r' ) ) {
            you.backlog.emplace_front( act_id );
            return false;
        }

        you.activity_vehicle_part_index = -1;
    } else if( reason == do_activity_reason::NEEDS_CRAFT ) {
        // only npc is supported
        npc *p = you.as_npc();
        if( p ) {
            item_location to_craft = p->get_item_to_craft();
            if( to_craft && to_craft->is_craft() &&
                you.lighting_craft_speed_multiplier( to_craft->get_making() ) > 0 ) {
                player_activity act = player_activity( craft_activity_actor( to_craft, false ) );
                you.assign_activity( act );
                you.backlog.emplace_front( ACT_MULTIPLE_CRAFT );
                you.backlog.front().auto_resume = true;
                return false;
            }
        }
    } else if( reason == do_activity_reason::NEEDS_DISASSEMBLE ) {
        map_stack items = here.i_at( src_loc );
        for( item &elem : items ) {
            if( elem.is_disassemblable() ) {
                // Disassemble the checked one.
                if( elem.get_var( "activity_var" ) == you.name ) {
                    const recipe &r = ( elem.typeId() == itype_disassembly ) ? elem.get_making() :
                                      recipe_dictionary::get_uncraft( elem.typeId() );
                    int const qty = std::max( 1, elem.typeId() == itype_disassembly ? elem.get_making_batch_size() :
                                              elem.charges );
                    player_activity act = player_activity( disassemble_activity_actor( r.time_to_craft_moves( you,
                                                           recipe_time_flag::ignore_proficiencies ) * qty ) );
                    // TODO: fix point types
                    act.targets.emplace_back( map_cursor( src_loc.raw() ), &elem );
                    act.placement = here.getglobal( src_loc );
                    act.position = qty;
                    act.index = false;
                    you.assign_activity( act );
                    // Keep doing
                    // After assignment of disassemble activity (not multitype anymore)
                    // the backlog will not be nuked in do_player_activity()
                    you.backlog.emplace_back( ACT_MULTIPLE_DIS );
                    break;
                }
            }
        }
        return false;
    }
    return true;
}

bool generic_multi_activity_handler( player_activity &act, Character &you, bool check_only )
{
    map &here = get_map();
    const tripoint_abs_ms abspos = here.getglobal( you.pos() );
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    activity_id activity_to_restore = act.id();
    // Nuke the current activity, leaving the backlog alone
    if( !check_only ) {
        you.activity = player_activity();
    }
    // now we setup the target spots based on which activity is occurring
    // the set of target work spots - potentially after we have fetched required tools.
    std::unordered_set<tripoint_abs_ms> src_set =
        generic_multi_activity_locations( you, activity_to_restore );
    // now we have our final set of points
    std::vector<tripoint_abs_ms> src_sorted = get_sorted_tiles_by_distance( abspos, src_set );
    // now loop through the work-spot tiles and judge whether its worth traveling to it yet
    // or if we need to fetch something first.

    // check: if a fetch act was assigned but no need to do that anymore, restore our task
    // may cause infinite loop if something goes wrong
    // Maybe it makes more harm than good? I don't know.
    if( activity_to_restore == activity_id( ACT_FETCH_REQUIRED ) && src_sorted.empty() ) {
        // remind what you failed to fetch
        if( !check_only && !you.backlog.empty() ) {
            player_activity &act_prev = you.backlog.front();
            if( !act_prev.str_values.empty() && you.as_npc() ) {
                you.as_npc()->job.fetch_history[act_prev.str_values.back()] = calendar::turn;
            }
        }
        return true;
    }
    for( const tripoint_abs_ms &src : src_sorted ) {
        const tripoint_bub_ms &src_loc = here.bub_from_abs( src );
        if( !here.inbounds( src_loc ) && !check_only ) {
            if( !here.inbounds( you.pos() ) ) {
                // p is implicitly an NPC that has been moved off the map, so reset the activity
                // and unload them
                you.assign_activity( activity_to_restore );
                you.set_moves( 0 );
                g->reload_npcs();
                return false;
            }
            const std::vector<tripoint_bub_ms> route = route_adjacent( you, src_loc );
            if( route.empty() ) {
                // can't get there, can't do anything, skip it
                continue;
            }
            you.set_moves( 0 );
            you.set_destination( route, player_activity( activity_to_restore ) );
            return false;
        }
        activity_reason_info act_info = can_do_activity_there( activity_to_restore, you,
                                        src_loc, ACTIVITY_SEARCH_DISTANCE );
        // see activity_handlers.h enum for requirement_check_result
        const requirement_check_result req_res = generic_multi_activity_check_requirement(
                    you, activity_to_restore, act_info, src, src_loc, src_set, check_only );
        if( req_res == requirement_check_result::SKIP_LOCATION ) {
            continue;
        } else if( req_res == requirement_check_result::RETURN_EARLY ) {
            return true;
        }
        std::vector<tripoint_bub_ms> route_workbench;
        if( activity_to_restore == ACT_MULTIPLE_CRAFT || activity_to_restore == ACT_MULTIPLE_DIS ) {
            // returns empty vector if we can't reach best tile
            // or we are already on the best tile
            route_workbench = route_best_workbench( you, src_loc );
        }
        // If we are doing disassemble we need to stand on the "best" tile
        if( square_dist( you.pos_bub(), src_loc ) > 1 || !route_workbench.empty() ) {
            std::vector<tripoint_bub_ms> route;
            // find best workbench if possible
            if( !route_workbench.empty() ) {
                route = route_workbench;
            } else {
                route = route_adjacent( you, src_loc );
            }
            // check if we found path to source / adjacent tile
            if( route.empty() ) {
                check_npc_revert( you );
                continue;
            }
            if( !check_only ) {
                if( you.moves <= 0 ) {
                    // Restart activity and break from cycle.
                    you.assign_activity( activity_to_restore );
                    return true;
                }
                // set the destination and restart activity after player arrives there
                // we don't need to check for safe mode,
                // activity will be restarted only if
                // player arrives on destination tile
                you.set_destination( route, player_activity( activity_to_restore ) );
                return true;
            }
        }

        // we checked if the work spot was in darkness earlier
        // but there is a niche case where the player is in darkness but the work spot is not
        // this can create infinite loops
        // and we can't check player.pos() for darkness before they've traveled to where they are going to be.
        // but now we are here, we check
        if( activity_to_restore != ACT_TIDY_UP &&
            activity_to_restore != ACT_MULTIPLE_MOP &&
            activity_to_restore != ACT_MOVE_LOOT &&
            activity_to_restore != ACT_FETCH_REQUIRED &&
            you.fine_detail_vision_mod( you.pos() ) > 4.0 ) {
            you.add_msg_if_player( m_info, _( "It is too dark to work here." ) );
            return false;
        }
        if( !check_only ) {
            if( !generic_multi_activity_do( you, activity_to_restore, act_info, src, src_loc ) ) {
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
        if( you.moves <= 0 ) {
            // Restart activity and break from cycle.
            you.assign_activity( activity_to_restore );
            you.activity_vehicle_part_index = -1;
            return false;
        }
        // if we got here, we need to revert otherwise NPC will be stuck in AI Limbo and have a head explosion.
        if( you.backlog.empty() || src_set.empty() ) {
            check_npc_revert( you );
            if( player_activity( activity_to_restore ).is_multi_type() ) {
                you.assign_activity( activity_id::NULL_ID() );
            }
        }
        you.activity_vehicle_part_index = -1;
    }
    // scanned every location, tried every path.
    return false;
}

static std::optional<tripoint_bub_ms> find_best_fire( const std::vector<tripoint_bub_ms> &from,
        const tripoint_bub_ms &center )
{
    std::optional<tripoint_bub_ms> best_fire;
    time_duration best_fire_age = 1_days;
    map &here = get_map();
    for( const tripoint_bub_ms &pt : from ) {
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
        if( here.has_flag_furn( ter_furn_flag::TFLAG_FIRE_CONTAINER, pt ) ) {
            return pt;
        }
    }

    return best_fire;
}

static bool has_clear_path_to_pickup_items(
    const tripoint_bub_ms &from, const tripoint_bub_ms &to )
{
    map &here = get_map();
    return here.has_items( to ) &&
           here.accessible_items( to ) &&
           here.clear_path( from, to, PICKUP_RANGE, 1, 100 );
}

static std::optional<tripoint_bub_ms> find_refuel_spot_zone( const tripoint_bub_ms &center,
        const faction_id &fac )
{
    const zone_manager &mgr = zone_manager::get_manager();
    map &here = get_map();
    const tripoint_abs_ms center_abs = here.getglobal( center );

    const std::unordered_set<tripoint_abs_ms> tiles_abs_unordered =
        mgr.get_near( zone_type_SOURCE_FIREWOOD, center_abs, PICKUP_RANGE, nullptr, fac );
    const std::vector<tripoint_abs_ms> &tiles_abs =
        get_sorted_tiles_by_distance( center_abs, tiles_abs_unordered );

    for( const tripoint_abs_ms &tile_abs : tiles_abs ) {
        const tripoint_bub_ms tile = here.bub_from_abs( tile_abs );
        if( has_clear_path_to_pickup_items( center, tile ) ) {
            return tile;
        }
    }

    return {};
}

static std::optional<tripoint_bub_ms> find_refuel_spot_trap(
    const std::vector<tripoint_bub_ms> &from, const tripoint_bub_ms &center )
{
    const auto tile = std::find_if( from.begin(), from.end(),
    [center]( const tripoint_bub_ms & pt ) {
        // Hacky - firewood spot is a trap and it's ID-checked
        return get_map().tr_at( pt ).id == tr_firewood_source
               && has_clear_path_to_pickup_items( center, pt );
    } );

    if( tile != from.end() ) {
        return *tile;
    }

    return {};
}

// Visits an item and all its contents.
static VisitResponse visit_item_contents( item_location &loc,
        const std::function<VisitResponse( item_location & )> &func )
{
    switch( func( loc ) ) {
        case VisitResponse::ABORT:
            return VisitResponse::ABORT;

        case VisitResponse::NEXT:
            for( item_pocket *pocket : loc->get_all_contained_pockets() ) {
                for( item &i : pocket->edit_contents() ) {
                    item_location child_loc( loc, &i );
                    if( visit_item_contents( child_loc, func ) == VisitResponse::ABORT ) {
                        return VisitResponse::ABORT;
                    }
                }
            }
        /* intentional fallthrough */

        case VisitResponse::SKIP:
            return VisitResponse::NEXT;
    }

    /* never reached; suppress the warning */
    return VisitResponse::ABORT;
}

static int get_comestible_order( Character &you, const item_location &loc,
                                 const time_duration &time )
{
    if( loc->rotten() ) {
        if( you.has_trait( trait_SAPROPHAGE ) || you.has_trait( trait_SAPROVORE ) ) {
            return 1;
        } else {
            return 5;
        }
    } else if( time == 0_turns ) {
        return 4;
    } else if( loc.has_parent() &&
               loc.parent_pocket()->spoil_multiplier() == 0.0f ) {
        return 3;
    } else {
        return 2;
    }
}

static time_duration get_comestible_time_left( const item_location &loc )
{
    time_duration time_left = 0_turns;
    const time_duration shelf_life = loc->is_comestible() ? loc->get_comestible()->spoils :
                                     calendar::INDEFINITELY_LONG_DURATION;
    if( shelf_life > 0_turns ) {
        const item &it = *loc;
        const double relative_rot = it.get_relative_rot();
        time_left = shelf_life - shelf_life * relative_rot;

        // Correct for an estimate that exceeds shelf life -- this happens especially with
        // fresh items.
        if( time_left > shelf_life ) {
            time_left = shelf_life;
        }
    }

    return time_left;
}

static bool comestible_sort_compare( Character &you, const item_location &lhs,
                                     const item_location &rhs )
{
    time_duration time_a = get_comestible_time_left( lhs );
    time_duration time_b = get_comestible_time_left( rhs );
    int order_a = get_comestible_order( you, lhs, time_a );
    int order_b = get_comestible_order( you, rhs, time_b );

    return order_a < order_b
           || ( order_a == order_b && time_a < time_b )
           || ( order_a == order_b && time_a == time_b );
}

int get_auto_consume_moves( Character &you, const bool food )
{
    if( you.is_npc() ) {
        return 0;
    }

    const tripoint pos = you.pos();
    zone_manager &mgr = zone_manager::get_manager();
    zone_type_id consume_type_zone( "" );
    if( food ) {
        consume_type_zone = zone_type_AUTO_EAT;
    } else {
        consume_type_zone = zone_type_AUTO_DRINK;
    }
    map &here = get_map();
    const std::unordered_set<tripoint_abs_ms> &dest_set =
        mgr.get_near( consume_type_zone, here.getglobal( pos ), ACTIVITY_SEARCH_DISTANCE, nullptr,
                      _fac_id( you ) );
    if( dest_set.empty() ) {
        return 0;
    }
    item_location best_comestible;
    for( const tripoint_abs_ms &loc : dest_set ) {
        if( loc.z() != you.pos().z ) {
            continue;
        }

        const auto visit = [&]( item_location & it ) {
            if( !you.can_consume_as_is( *it ) ) {
                return VisitResponse::NEXT;
            }
            if( it->has_flag( json_flag_NO_AUTO_CONSUME ) ) {
                // ignored due to NO_AUTO_CONSUME flag
                return VisitResponse::NEXT;
            }
            if( it->is_null() || it->is_craft() || !it->is_food() ||
                you.fun_for( *it ).first < -5 ) {
                // not good eatings.
                return VisitResponse::NEXT;
            }
            if( food && you.compute_effective_nutrients( *it ).kcal() < 50 ) {
                // not filling enough
                return VisitResponse::NEXT;
            }
            if( !you.will_eat( *it, false ).success() ) {
                // wont like it, cannibal meat etc
                return VisitResponse::NEXT;
            }
            if( !it->is_owned_by( you, true ) ) {
                // it aint ours.
                return VisitResponse::NEXT;
            }
            if( !food && it->get_comestible()->quench < 15 ) {
                // not quenching enough
                return VisitResponse::NEXT;
            }
            if( !food && it->is_watertight_container() && it->made_of( phase_id::SOLID ) ) {
                // it's frozen
                return VisitResponse::NEXT;
            }
            const use_function *usef = it->type->get_use( "BLECH_BECAUSE_UNCLEAN" );
            if( usef ) {
                // it's unclean
                return VisitResponse::NEXT;
            }
            if( it->get_comestible()->addictions.count( addiction_alcohol ) &&
                !you.has_addiction( addiction_alcohol ) ) {
                return VisitResponse::NEXT;
            }

            if( !best_comestible || comestible_sort_compare( you, it, best_comestible ) ) {
                best_comestible = it;
            }

            return VisitResponse::NEXT;
        };

        const optional_vpart_position vp = here.veh_at( here.getlocal( loc ) );
        if( vp ) {
            if( const std::optional<vpart_reference> vp_cargo = vp.cargo() ) {
                for( item &it : vp_cargo->items() ) {
                    item_location i_loc( vehicle_cursor( vp->vehicle(), vp->part_index() ), &it );
                    visit_item_contents( i_loc, visit );
                }
            }
        } else {
            for( item &it : here.i_at( here.getlocal( loc ) ) ) {
                item_location i_loc( map_cursor( here.getlocal( loc ) ), &it );
                visit_item_contents( i_loc, visit );
            }
        }
    }

    if( best_comestible ) {
        int consume_moves = -Pickup::cost_to_move_item( you,
                            *best_comestible ) * std::max( rl_dist( you.pos(),
                                    here.getlocal( best_comestible.position() ) ), 1 );
        consume_moves += to_moves<int>( you.get_consume_time( *best_comestible ) );

        you.consume( best_comestible );
        // eat() may have removed the item, so check its still there.
        if( best_comestible.get_item() && best_comestible->is_container() ) {
            best_comestible->on_contents_changed();
        }

        return consume_moves;
    }
    return 0;
}

// Try to add fuel to a fire. Return true if there is both fire and fuel; return false otherwise.
bool try_fuel_fire( player_activity &act, Character &you, const bool starting_fire )
{
    const tripoint_bub_ms pos = you.pos_bub();
    std::vector<tripoint_bub_ms> adjacent = closest_points_first( pos, PICKUP_RANGE );
    adjacent.erase( adjacent.begin() );

    map &here = get_map();
    std::optional<tripoint_bub_ms> best_fire =
        starting_fire ? here.bub_from_abs( act.placement ) : find_best_fire( adjacent, pos );

    if( !best_fire || !here.accessible_items( *best_fire ) ) {
        return false;
    }

    std::optional<tripoint_bub_ms> refuel_spot = find_refuel_spot_zone( pos, _fac_id( you ) );
    if( !refuel_spot ) {
        refuel_spot = find_refuel_spot_trap( adjacent, pos );
        if( !refuel_spot ) {
            return false;
        }
    }

    // Special case: fire containers allow burning logs, so use them as fuel if fire is contained
    bool contained = here.has_flag_furn( ter_furn_flag::TFLAG_FIRE_CONTAINER, *best_fire );
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
            move_item( you, it, 0, *best_fire, *refuel_spot, std::nullopt );
            return true;
        }
    }

    // Enough to sustain the fire
    // TODO: It's not enough in the rain
    if( !starting_fire && ( fd.fuel_produced >= 1.0f || fire_age < 10_minutes ) ) {
        return true;
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
            move_item( you, it, quantity, *refuel_spot, *best_fire, std::nullopt );
            return true;
        }
    }
    return true;
}
