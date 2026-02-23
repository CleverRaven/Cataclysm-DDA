#include "activity_handlers.h" // IWYU pragma: associated
#include "activity_item_handling.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "activity_type.h"
#include "avatar.h"
#include "butchery.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "clzones.h"
#include "construction.h"
#include "coordinates.h"
#include "craft_command.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enums.h"
#include "faction.h"
#include "field.h"
#include "field_type.h"
#include "fire.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "iexamine.h"
#include "inventory.h"
#include "item.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse.h"
#include "lightmap.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "map_selector.h"
#include "mapdata.h"
#include "messages.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pickup.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "ret_val.h"
#include "stomach.h"
#include "temp_crafting_inventory.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vpart_position.h"
#include "weather.h"

static const activity_id ACT_FETCH_REQUIRED( "ACT_FETCH_REQUIRED" );
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
static const activity_id ACT_MULTIPLE_STUDY( "ACT_MULTIPLE_STUDY" );
static const activity_id ACT_VEHICLE_DECONSTRUCTION( "ACT_VEHICLE_DECONSTRUCTION" );
static const activity_id ACT_VEHICLE_REPAIR( "ACT_VEHICLE_REPAIR" );

static const addiction_id addiction_alcohol( "alcohol" );

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
static const requirement_id requirement_data_multi_butcher( "multi_butcher" );
static const requirement_id requirement_data_multi_butcher_big( "multi_butcher_big" );
static const requirement_id requirement_data_multi_chopping_planks( "multi_chopping_planks" );
static const requirement_id requirement_data_multi_chopping_trees( "multi_chopping_trees" );
static const requirement_id
requirement_data_multi_farm_cut_harvesting( "multi_farm_cut_harvesting" );
static const requirement_id requirement_data_multi_farm_tilling( "multi_farm_tilling" );
static const requirement_id requirement_data_multi_fishing( "multi_fishing" );

static const ter_str_id ter_t_stump( "t_stump" );
static const ter_str_id ter_t_trunk( "t_trunk" );

static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );

static const trap_str_id tr_firewood_source( "tr_firewood_source" );

static const zone_type_id zone_type_( "" );
static const zone_type_id zone_type_AUTO_DRINK( "AUTO_DRINK" );
static const zone_type_id zone_type_AUTO_EAT( "AUTO_EAT" );
static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );
static const zone_type_id zone_type_CHOP_TREES( "CHOP_TREES" );
static const zone_type_id zone_type_CONSTRUCTION_BLUEPRINT( "CONSTRUCTION_BLUEPRINT" );
static const zone_type_id zone_type_DISASSEMBLE( "DISASSEMBLE" );
static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );
static const zone_type_id zone_type_FISHING_SPOT( "FISHING_SPOT" );
static const zone_type_id zone_type_LOOT_CORPSE( "LOOT_CORPSE" );
static const zone_type_id zone_type_LOOT_CUSTOM( "LOOT_CUSTOM" );
static const zone_type_id zone_type_LOOT_IGNORE( "LOOT_IGNORE" );
static const zone_type_id zone_type_LOOT_IGNORE_FAVORITES( "LOOT_IGNORE_FAVORITES" );
static const zone_type_id zone_type_LOOT_ITEM_GROUP( "LOOT_ITEM_GROUP" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_LOOT_WOOD( "LOOT_WOOD" );
static const zone_type_id zone_type_MINING( "MINING" );
static const zone_type_id zone_type_MOPPING( "MOPPING" );
static const zone_type_id zone_type_SOURCE_FIREWOOD( "SOURCE_FIREWOOD" );
static const zone_type_id zone_type_STRIP_CORPSES( "STRIP_CORPSES" );
static const zone_type_id zone_type_STUDY_ZONE( "STUDY_ZONE" );
static const zone_type_id zone_type_UNLOAD_ALL( "UNLOAD_ALL" );
static const zone_type_id zone_type_VEHICLE_DECONSTRUCT( "VEHICLE_DECONSTRUCT" );
static const zone_type_id zone_type_VEHICLE_REPAIR( "VEHICLE_REPAIR" );

namespace
{
faction_id _fac_id( Character &you )
{
    faction const *fac = you.get_faction();
    return fac == nullptr ? faction_id() : fac->id;
}
} // namespace

static bool is_valid_study_book( const item &it, const Character &you,
                                 const std::set<skill_id> *skill_prefs )
{
    if( it.has_var( "activity_var" ) && it.get_var( "activity_var" ) != you.name ) {
        return false;
    }
    if( you.check_read_condition( it ) != read_condition_result::SUCCESS ) {
        return false;
    }
    // if zone has skill preferences, filter by them
    if( skill_prefs && it.is_book() && it.type->book ) {
        const skill_id &book_skill = it.type->book->skill;
        if( book_skill == skill_id::NULL_ID() ) {
            return false;
        }
        if( skill_prefs->count( book_skill ) == 0 ) {
            return false;
        }
    }
    return true;
}

static item_location find_study_book( const tripoint_abs_ms &zone_pos, Character &you )
{
    map &here = get_map();
    zone_manager &mgr = zone_manager::get_manager();
    const zone_data *zone = mgr.get_zone_at( zone_pos, zone_type_STUDY_ZONE, _fac_id( you ) );
    const std::set<skill_id> *skill_prefs = nullptr;
    if( zone && zone->has_options() ) {
        const study_zone_options *options = dynamic_cast<const study_zone_options *>
                                            ( &zone->get_options() );
        if( options ) {
            skill_prefs = options->get_skill_preferences( you.name );
        }
    }
    const tripoint_bub_ms zone_loc = here.get_bub( zone_pos );

    // items on the ground
    for( item &it : here.i_at( zone_loc ) ) {
        if( is_valid_study_book( it, you, skill_prefs ) ) {
            return item_location( map_cursor( zone_loc ), &it );
        }

        // books in containers
        if( it.has_pocket_type( pocket_type::CONTAINER ) ) {
            for( item *contained : it.all_items_container_top() ) {
                if( is_valid_study_book( *contained, you, skill_prefs ) ) {
                    item_location container_loc( map_cursor( zone_loc ), &it );
                    return item_location( container_loc, contained );
                }
            }
        }
    }
    return item_location();
}

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

namespace multi_activity_actor
{
void check_npc_revert( Character &you )
{
    if( you.is_npc() ) {
        npc *guy = dynamic_cast<npc *>( &you );
        if( guy ) {
            guy->revert_after_activity();
        }
    }
}
} //namespace multi_activity_actor

// TODO: Deliberately unified with multidrop. Unify further.
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
                it.display_name(), m.name( c.pos_bub() )
            );
            m.add_item_or_charges( c.pos_bub(), it );
            return true;
        }
    }

    return false;
}

//try to put items into_vehicle .If fail,first try to add to character bag, then character try wield  it, last drop.
static std::vector<item_location> try_to_put_into_vehicle( Character &c, item_drop_reason reason,
        const std::list<item> &items,
        const vpart_reference &vpr )
{
    map &here = get_map();
    std::vector<item_location> result;
    if( items.empty() ) {
        return result;
    }
    c.invalidate_weight_carried_cache();
    vehicle_part &vp = vpr.part();
    vehicle &veh = vpr.vehicle();
    const tripoint_bub_ms where = veh.bub_part_pos( here, vp );
    int items_did_not_fit_count = 0;
    int into_vehicle_count = 0;
    const std::string part_name = vp.info().name();
    // can't use constant reference here because of the spill_contents()
    for( item it : items ) {
        if( handle_spillable_contents( c, it, here ) ) {
            continue;
        }

        if( it.made_of( phase_id::LIQUID ) ) {
            here.add_item_or_charges( c.pos_bub(), it );
            it.charges = 0;
        }

        if( std::optional<vehicle_stack::iterator> maybe_item = veh.add_item( here, vp, it ) ) {
            into_vehicle_count += it.count();
            result.emplace_back( vehicle_cursor( veh, vpr.part_index() ), &*maybe_item.value() );
        } else {
            if( it.count_by_charges() ) {
                // Maybe we can add a few charges in the trunk and the rest on the ground.
                const int charges_added = veh.add_charges( here, vp, it );
                it.mod_charges( -charges_added );
                into_vehicle_count += charges_added;
            }
            items_did_not_fit_count += it.count();
            //~ %1$s is item name, %2$s is vehicle name, %3$s is vehicle part name
            add_msg( m_mixed, _( "Unable to fit %1$s in the %2$s's %3$s." ), it.tname(), veh.name, part_name );
            // Retain item in inventory if overflow not too large/heavy or wield if possible otherwise drop on the ground
            if( c.can_pickVolume( it ) && c.can_pickWeight( it, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
                result.push_back( c.i_add( it ) );
            } else if( !c.has_wield_conflicts( it ) && c.can_wield( it ).success() ) {
                c.wield( it );
                result.emplace_back( c.get_wielded_item() );
            } else {
                const std::string ter_name = here.name( where );
                //~ %1$s - item name, %2$s - terrain name
                add_msg( _( "The %1$s falls to the %2$s." ), it.tname(), ter_name );
                result.push_back( here.add_item_or_charges_ret_loc( where, it ) );
            }
        }
        it.handle_pickup_ownership( c );
    }

    if( same_type( items ) ) {
        const item &it = items.front();
        const int dropcount = items.size() * it.count();
        const std::string it_name = it.tname( dropcount );

        switch( reason ) {
            case item_drop_reason::deliberate:
                if( items_did_not_fit_count == 0 ) {
                    c.add_msg_player_or_npc(
                        n_gettext( "You put your %1$s in the %2$s's %3$s.",
                                   "You put your %1$s in the %2$s's %3$s.", dropcount ),
                        n_gettext( "<npcname> puts their %1$s in the %2$s's %3$s.",
                                   "<npcname> puts their %1$s in the %2$s's %3$s.", dropcount ),
                        it_name, veh.name, part_name
                    );
                } else if( into_vehicle_count > 0 ) {
                    c.add_msg_player_or_npc(
                        n_gettext( "You put some of your %1$s in the %2$s's %3$s.",
                                   "You put some of your %1$s in the %2$s's %3$s.", dropcount ),
                        n_gettext( "<npcname> puts some of their %1$s in the %2$s's %3$s.",
                                   "<npcname> puts some of their %1$s in the %2$s's %3$s.", dropcount ),
                        it_name, veh.name, part_name
                    );
                }
                break;
            case item_drop_reason::too_large:
                c.add_msg_if_player(
                    n_gettext(
                        "There's no room in your inventory for the %1$s, so you drop it into the %2$s's %3$s.",
                        "There's no room in your inventory for the %1$s, so you drop them into the %2$s's %3$s.",
                        dropcount ),
                    it_name, veh.name, part_name
                );
                break;
            case item_drop_reason::too_heavy:
                c.add_msg_if_player(
                    n_gettext( "The %1$s is too heavy to carry, so you drop it into the %2$s's %3$s.",
                               "The %1$s are too heavy to carry, so you drop them into the %2$s's %3$s.", dropcount ),
                    it_name, veh.name, part_name
                );
                break;
            case item_drop_reason::tumbling:
                c.add_msg_if_player(
                    m_bad,
                    n_gettext( "Your %1$s tumbles into the %2$s's %3$s.",
                               "Your %1$s tumble into the %2$s's %3$s.", dropcount ),
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
    return result;
}

static itype_id get_first_fertilizer_itype( Character &you, const tripoint_abs_ms &src )
{
    const zone_manager &mgr = zone_manager::get_manager();
    std::vector<zone_data> zones = mgr.get_zones( zone_type_FARM_PLOT, src, _fac_id( you ) );
    for( const zone_data &zone : zones ) {
        const itype_id fertilizer =
            dynamic_cast<const plot_options &>( zone.get_options() ).get_fertilizer();
        if( fertilizer.is_valid() ) {
            std::vector<item_location> fertilizer_inv = you.cache_get_items_with( fertilizer );
            if( fertilizer_inv.empty() ) {
                continue;
            }
            return fertilizer_inv.front()->typeId();
        }
    }
    return itype_id::NULL_ID();
}

std::vector<item_location> drop_on_map( Character &you, item_drop_reason reason,
                                        const std::list<item> &items,
                                        map *here, const tripoint_bub_ms &where )
{
    if( items.empty() ) {
        return {};
    }
    const std::string ter_name = here->name( where );
    const bool can_move_there = here->passable_through( where );

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

        if( get_option<bool>( "AUTO_NOTES_DROPPED_FAVORITES" )
            && ( it.is_favorite
        || it.has_any_with( []( const item & it ) {
        return it.is_favorite;
    }, pocket_type::CONTAINER ) ) ) {
            const tripoint_abs_omt your_pos = you.pos_abs_omt();
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
    std::vector<item_location> items_dropped;
    for( const item &it : items ) {
        // Use ret_loc variant so the item_location tracks the actual position,
        // which may differ from 'where' if the tile overflowed to an adjacent one.
        item_location dropped_loc = here->add_item_or_charges_ret_loc( where, it );
        if( dropped_loc.get_item() ) {
            items_dropped.push_back( std::move( dropped_loc ) );
        }
        item( it ).handle_pickup_ownership( you );
    }

    you.recoil = MAX_RECOIL;
    you.invalidate_weight_carried_cache();

    return items_dropped;
}

void put_into_vehicle_or_drop( Character &you, item_drop_reason reason,
                               const std::list<item> &items )
{
    map &here = get_map();

    put_into_vehicle_or_drop( you, reason, items, &here, you.pos_bub( here ) );
}

void put_into_vehicle_or_drop( Character &you, item_drop_reason reason,
                               const std::list<item> &items,
                               map *here, const tripoint_bub_ms &where, bool force_ground )
{
    const std::optional<vpart_reference> vp = here->veh_at( where ).cargo();
    if( vp && !force_ground ) {
        try_to_put_into_vehicle( you, reason, items, *vp );
        return;
    }
    drop_on_map( you, reason, items, here, where );
}

std::vector<item_location> put_into_vehicle_or_drop_ret_locs( Character &you,
        item_drop_reason reason,
        const std::list<item> &items, tripoint_bub_ms dest )
{
    map &here = get_map();

    // If they didn't specify somewhere to place, drop at the character's feet.
    if( dest == tripoint_bub_ms::invalid ) {
        dest = you.pos_bub( here );
    }

    return put_into_vehicle_or_drop_ret_locs( you, reason, items, &here, dest );
}

std::vector<item_location> put_into_vehicle_or_drop_ret_locs( Character &you,
        item_drop_reason reason,
        const std::list<item> &items,
        map *here, const tripoint_bub_ms &where, bool force_ground )
{
    const std::optional<vpart_reference> vp = here->veh_at( where ).cargo();
    if( vp && !force_ground ) {
        return try_to_put_into_vehicle( you, reason, items, *vp );
    }
    return drop_on_map( you, reason, items, here, where );
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
    const int inventory_capacity = units::to_milliliter( player_character.free_space() );

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
    // ?!??!?! This always checks the player regardless of who's doing the activity
    avatar &player_character = get_avatar();
    if( player_character.get_grab_type() == object_type::VEHICLE ) {
        const tripoint_bub_ms cart_position = player_character.pos_bub() + player_character.grab_point;
        if( const std::optional<vpart_reference> ovp = get_map().veh_at( cart_position ).cargo() ) {
            return move_cost_cart( it, src, dest, ovp->items().free_volume() );
        }
    }

    return move_cost_inv( it, src, dest );
}

// return true if activity was assigned.
// return false if it was not possible.
static bool vehicle_activity( Character &you, const tripoint_bub_ms &src_loc, int vpindex,
                              vehicle_action type )
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
        if( type == VEHICLE_REPAIR ) {
            return false;
        } else if( type == VEHICLE_REMOVE ) {
            vpindex = veh->get_next_shifted_index( vpindex, you );
            if( vpindex == -1 ) {
                return false;
            }
        }
    }
    const vehicle_part &vp = veh->part( vpindex );
    const vpart_info &vpi = vp.info();
    if( type == VEHICLE_REPAIR ) {
        const int frac = ( vp.damage() - vp.degradation() ) / ( vp.max_damage() - vp.degradation() );
        time_to_take = vpi.repair_time( you ) * frac;
    } else if( type == VEHICLE_REMOVE ) {
        time_to_take = vpi.removal_time( you );
    }
    you.assign_activity( vehicle_activity_actor( type, time_to_take, veh, here.get_abs( src_loc ),
                         tripoint_rel_ms::zero, veh->index_of_part( &vp ), vpi.id ) );
    // so , NPCs can remove the last part on a position, then there is no vehicle there anymore,
    // for someone else who stored that position at the start of their activity.
    // so we may need to go looking a bit further afield to find it , at activities end.
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
        if( activity_to_restore == ACT_FETCH_REQUIRED ) {
            it.set_var( "activity_var", you.name );
        }
        put_into_vehicle_or_drop( you, item_drop_reason::deliberate, { it }, &here, dest );
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
            if( !vpr_src->vehicle().add_item( here, vpr_src->part(), leftovers ) ) {
                debugmsg( "SortLoot: Source vehicle failed to receive leftover charges." );
            }
        } else {
            here.add_item_or_charges( src, leftovers );
        }
    }
}

namespace zone_sorting
{

// Number of grab direction slots: 3x3 grid encoding (x+1)*3 + (y+1), index 4 = center = unused.
static constexpr int GRAB_DIRS = 9;

static int grab_to_idx( const tripoint_rel_ms &g )
{
    return ( g.x() + 1 ) * 3 + ( g.y() + 1 );
}

static tripoint_rel_ms idx_to_grab( int idx )
{
    return tripoint_rel_ms( idx / 3 - 1, idx % 3 - 1, 0 );
}

static int grab_state_index( const point_bub_ms &pos, int grab_idx )
{
    return ( pos.x() * MAPSIZE_Y + pos.y() ) * GRAB_DIRS + grab_idx;
}

static point_bub_ms state_to_pos( int state_idx )
{
    int pos_idx = state_idx / GRAB_DIRS;
    return point_bub_ms( pos_idx / MAPSIZE_Y, pos_idx % MAPSIZE_Y );
}

// Estimate vehicle drag difficulty using FLAT/ROAD terrain flags (matching
// wheel terrain_modifiers): non-FLAT +4, FLAT non-ROAD +3, FLAT+ROAD +0.
static int veh_drag_cost( const map &here, const tripoint_bub_ms &pos )
{
    const int base = here.move_cost_ter_furn( pos );
    if( base <= 0 ) {
        return 0;
    }
    if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAT, pos ) ) {
        return base + 4;
    }
    if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_ROAD, pos ) ) {
        return base + 3;
    }
    return base;
}

// Check if a tile would cause a vehicle collision, matching part_collision
// logic: impassable tiles block, bashable non-flat terrain/furniture blocks.
// allow_doors: pull moves pass true (vehicle follows player through opened
// doors); push/zigzag pass false (vehicle goes to unvisited tiles).
static bool tile_blocks_vehicle( const map &here, const tripoint_bub_ms &pos,
                                 bool allow_doors = true )
{
    const int ter_furn_cost = here.move_cost_ter_furn( pos );

    // Impassable terrain (walls, closed windows, locked doors).
    if( ter_furn_cost == 0 ) {
        if( allow_doors ) {
            // Openable doors: player opens them during auto-move, vehicle follows.
            const bool is_door = ( here.ter( pos ).obj().open &&
                                   here.ter( pos ).obj().has_flag( ter_furn_flag::TFLAG_DOOR ) ) ||
                                 ( here.has_furn( pos ) && here.furn( pos ).obj().open &&
                                   here.furn( pos ).obj().has_flag( ter_furn_flag::TFLAG_DOOR ) );
            if( is_door ) {
                return false;
            }
        }
        return true;
    }

    // Flat ground (move_cost 2) never causes collision.
    if( ter_furn_cost == 2 ) {
        return false;
    }

    // Bashable non-flat terrain/furniture causes collision (bushes, open
    // windows, fences). NOCOLLIDE excluded (e.g. railroad tracks).
    if( here.is_bashable_ter_furn( pos, false ) &&
        !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NOCOLLIDE, pos ) ) {
        return true;
    }

    return false;
}

// Cache routes computed by route_length() for reuse by route_to_destination().
// Avoids recomputing the same A* when the sorter probes route distance and
// then immediately routes to the same destination.
namespace
{
struct zone_route_cache {
    tripoint_bub_ms start;
    object_type grab_type = object_type::NONE;
    tripoint_rel_ms grab_point;
    // (destination center, route). Empty route means unreachable.
    std::vector<std::pair<tripoint_bub_ms, std::vector<tripoint_bub_ms>>> entries;
    bool initialized = false;

    void ensure_valid( const Character &who ) {
        object_type gt = object_type::NONE;
        tripoint_rel_ms gp;
        if( who.is_avatar() ) {
            gt = who.as_avatar()->get_grab_type();
            gp = who.as_avatar()->grab_point;
        }
        if( !initialized || start != who.pos_bub() || grab_type != gt || grab_point != gp ) {
            start = who.pos_bub();
            grab_type = gt;
            grab_point = gp;
            entries.clear();
            initialized = true;
        }
    }

    const std::vector<tripoint_bub_ms> *find( const tripoint_bub_ms &dest ) const {
        for( const auto &e : entries ) {
            if( e.first == dest ) {
                return &e.second;
            }
        }
        return nullptr;
    }

    void store( const tripoint_bub_ms &dest, std::vector<tripoint_bub_ms> route ) {
        entries.emplace_back( dest, std::move( route ) );
    }
};

zone_route_cache g_route_cache;
} // namespace

// Check if a straight push or pull path works, skipping the full A*.
// Only applies when grab direction is aligned with travel direction
// (push) or opposite (pull), and every tile on the line is passable
// for both the player and the dragged vehicle.
static std::vector<tripoint_bub_ms> try_straight_grab_path(
    const map &here, const tripoint_bub_ms &start, const pathfinding_target &target,
    const tripoint_rel_ms &cur_grab, vehicle &grabbed_veh )
{
    const tripoint_bub_ms &dest = target.center;
    const point d( dest.x() - start.x(), dest.y() - start.y() );
    const point ad( std::abs( d.x ), std::abs( d.y ) );

    // Must be a pure cardinal or diagonal direction
    if( ( ad.x != ad.y && ad.x > 0 && ad.y > 0 ) || ( ad.x == 0 && ad.y == 0 ) ) {
        return {};
    }

    const point s( d.x > 0 ? 1 : ( d.x < 0 ? -1 : 0 ), d.y > 0 ? 1 : ( d.y < 0 ? -1 : 0 ) );
    const tripoint_rel_ms dir( s.x, s.y, 0 );

    const bool is_push = cur_grab == dir;
    const bool is_pull = cur_grab == -dir;
    if( !is_push && !is_pull ) {
        return {};
    }

    const int num_steps = std::max( ad.x, ad.y );
    std::vector<tripoint_bub_ms> route;
    route.reserve( num_steps );

    tripoint_bub_ms player = start;
    tripoint_bub_ms vehicle_pos = start + cur_grab;

    for( int step = 0; step < num_steps; step++ ) {
        const tripoint_bub_ms next_player = player + dir;
        tripoint_bub_ms next_vehicle;
        if( is_push ) {
            next_vehicle = vehicle_pos + dir;
        } else {
            // Pull: vehicle follows to player's old position
            next_vehicle = player;
        }

        // Player passability (exclude grabbed vehicle - it vacates on push)
        if( here.move_cost( next_player, &grabbed_veh ) == 0 ) {
            return {};
        }

        // Vehicle passability
        if( tile_blocks_vehicle( here, next_vehicle, is_pull ) ) {
            return {};
        }
        const optional_vpart_position ovp_check = here.veh_at( next_vehicle );
        if( ovp_check && &ovp_check->vehicle() != &grabbed_veh ) {
            return {};
        }

        route.push_back( next_player );
        if( target.contains( next_player ) ) {
            return route;
        }

        player = next_player;
        vehicle_pos = next_vehicle;
    }

    return {};
}

// State is (player_position, grab_direction), so it finds routes where
// both the player and the dragged vehicle can physically move.
// Returns an empty vector if no path exists or if the player isn't
// dragging a single-tile vehicle.
static std::vector<tripoint_bub_ms> route_with_grab(
    const map &here, const Character &you, const pathfinding_target &target )
{
    std::vector<tripoint_bub_ms> ret;

    if( !you.is_avatar() || you.as_avatar()->get_grab_type() != object_type::VEHICLE ) {
        return ret;
    }

    const tripoint_bub_ms start = you.pos_bub();
    const tripoint_rel_ms start_grab = you.as_avatar()->grab_point;
    const tripoint_bub_ms veh_pos = start + start_grab;
    const optional_vpart_position ovp = here.veh_at( veh_pos );
    if( !ovp ) {
        add_msg_debug( debugmode::DF_ACTIVITY,
                       "route_with_grab: no vehicle at grab point (%d,%d,%d)+(%d,%d,%d)",
                       start.x(), start.y(), start.z(),
                       start_grab.x(), start_grab.y(), start_grab.z() );
        return ret;
    }
    vehicle &grabbed_veh = ovp->vehicle();
    if( grabbed_veh.get_points().size() > 1 ) {
        add_msg_debug( debugmode::DF_ACTIVITY,
                       "route_with_grab: multi-tile vehicle (%zu parts), skipping",
                       grabbed_veh.get_points().size() );
        return ret;
    }

    add_msg_debug( debugmode::DF_ACTIVITY,
                   "route_with_grab: start=(%d,%d) grab=(%d,%d) target=(%d,%d) r=%d",
                   start.x(), start.y(), start_grab.x(), start_grab.y(),
                   target.center.x(), target.center.y(), target.r );

    // Fast path: if grab is aligned with travel direction, check the
    // straight line before spinning up the full A* with its 157K-state arrays.
    ret = try_straight_grab_path( here, start, target, start_grab, grabbed_veh );
    if( !ret.empty() ) {
        add_msg_debug( debugmode::DF_ACTIVITY,
                       "route_with_grab: straight line path len=%zu", ret.size() );
        return ret;
    }

    const int max_length = you.get_pathfinding_settings().max_length;
    const int pad = 16;
    const tripoint_bub_ms &t = target.center;

    point_bub_ms min_bound( std::min( start.x(), t.x() ) - pad,
                            std::min( start.y(), t.y() ) - pad );
    point_bub_ms max_bound( std::max( start.x(), t.x() ) + pad,
                            std::max( start.y(), t.y() ) + pad );
    min_bound.x() = std::max( min_bound.x(), 0 );
    min_bound.y() = std::max( min_bound.y(), 0 );
    max_bound.x() = std::min( max_bound.x(), MAPSIZE_X );
    max_bound.y() = std::min( max_bound.y(), MAPSIZE_Y );

    const int total_states = MAPSIZE_X * MAPSIZE_Y * GRAB_DIRS;

    // Reuse heap-allocated arrays across calls
    static std::vector<bool> closed;
    static std::vector<bool> open;
    static std::vector<int> gscore;
    static std::vector<int> parent;

    if( static_cast<int>( closed.size() ) != total_states ) {
        closed.resize( total_states );
        open.resize( total_states );
        gscore.resize( total_states );
        parent.resize( total_states );
    }

    // Only closed and open need clearing. gscore and parent retain stale data
    // but are never read for states where open[state] is false, which is reset
    // above. This invariant must be maintained if the A* logic is modified.
    std::fill( closed.begin(), closed.end(), false );
    std::fill( open.begin(), open.end(), false );

    // Priority queue: (f-score, state_index), smallest f-score first
    using pq_entry = std::pair<int, int>;
    std::priority_queue<pq_entry, std::vector<pq_entry>, std::greater<>> pq;

    const int start_grab_idx = grab_to_idx( start_grab );
    const int start_state = grab_state_index( start.xy(), start_grab_idx );
    gscore[start_state] = 0;
    open[start_state] = true;
    parent[start_state] = start_state;
    // Tighter heuristic: minimum cost per tile is 4 (player move_cost=2 +
    // vehicle drag=2 on flat road), minus 2 for one possible sideways move
    // (vehicle stays put, cost=2). Uses square_dist (Chebyshev = min steps)
    // instead of rl_dist to stay admissible regardless of trigdist setting.
    pq.emplace( std::max( 0, 4 * square_dist( start, t ) - 2 ), start_state );

    // Movement offsets: W, E, N, S, NE, SW, NW, SE
    constexpr std::array<int, 8> x_off{ { -1,  1,  0,  0,  1, -1, -1, 1 } };
    constexpr std::array<int, 8> y_off{ {  0,  0, -1,  1, -1,  1, -1, 1 } };

    bool done = false;
    int found_state = -1;
    int states_explored = 0;

    while( !pq.empty() ) {
        const auto [cur_score, cur_state] = pq.top();
        pq.pop();

        if( closed[cur_state] ) {
            continue;
        }

        states_explored++;
        const int cur_g = gscore[cur_state];
        if( cur_g > max_length ) {
            add_msg_debug( debugmode::DF_ACTIVITY,
                           "route_with_grab: ABORTED max_length=%d explored=%d grab=(%d,%d)",
                           max_length, states_explored, start_grab.x(), start_grab.y() );
            return ret;
        }

        const point_bub_ms cur_pos = state_to_pos( cur_state );
        const int cur_grab_idx = cur_state % GRAB_DIRS;
        const tripoint_rel_ms cur_grab = idx_to_grab( cur_grab_idx );
        const tripoint_bub_ms cur3d( cur_pos, start.z() );

        if( target.contains( cur3d ) ) {
            done = true;
            found_state = cur_state;
            break;
        }

        closed[cur_state] = true;

        for( size_t i = 0; i < 8; i++ ) {
            const point_bub_ms next_pos( cur_pos.x() + x_off[i], cur_pos.y() + y_off[i] );

            if( next_pos.x() < min_bound.x() || next_pos.x() >= max_bound.x() ||
                next_pos.y() < min_bound.y() || next_pos.y() >= max_bound.y() ) {
                continue;
            }

            const tripoint_bub_ms next3d( next_pos, start.z() );

            // Player passability (ignore grabbed vehicle - it vacates the tile on push)
            int tile_cost = here.move_cost( next3d, &grabbed_veh );
            if( tile_cost == 0 ) {
                // Allow closed doors (cost 4, matching main pathfinder).
                // Exclude windows - multi-step open, vehicle can't follow through.
                const bool is_door = ( here.ter( next3d ).obj().open &&
                                       here.ter( next3d ).obj().has_flag( ter_furn_flag::TFLAG_DOOR ) ) ||
                                     ( here.furn( next3d ).obj().open &&
                                       here.furn( next3d ).obj().has_flag( ter_furn_flag::TFLAG_DOOR ) );
                if( is_door ) {
                    tile_cost = 4;
                } else {
                    continue;
                }
            }

            const tripoint_rel_ms dp( x_off[i], y_off[i], 0 );
            tripoint_rel_ms dp_veh = -cur_grab;
            tripoint_rel_ms next_grab = cur_grab;
            bool vehicle_moves = true;
            bool is_zigzag = false;
            bool is_push = false;

            if( dp == cur_grab ) {
                // PUSH: vehicle moves in same direction as player
                dp_veh = dp;
                is_push = true;
            } else if( std::abs( dp.x() + dp_veh.x() ) != 2 &&
                       std::abs( dp.y() + dp_veh.y() ) != 2 ) {
                // SIDEWAYS: vehicle stays put, grab rotates
                next_grab = -( dp + dp_veh );
                vehicle_moves = false;
            } else if( ( dp.x() == cur_grab.x() || dp.y() == cur_grab.y() ) &&
                       cur_grab.x() != 0 && cur_grab.y() != 0 ) {
                // ZIGZAG: player is diagonal to vehicle, moves partially away
                dp_veh.x() = dp.x() == -dp_veh.x() ? 0 : dp_veh.x();
                dp_veh.y() = dp.y() == -dp_veh.y() ? 0 : dp_veh.y();
                next_grab = -dp_veh;
                is_zigzag = true;
            } else {
                // PULL: vehicle moves to player's old position
                next_grab = -dp;
                // dp_veh stays as -cur_grab (initialized above)
            }

            // Vehicle terrain cost: FLAT/ROAD flag penalties for dragging.
            int veh_terrain_cost = 0;

            if( vehicle_moves ) {
                const tripoint_bub_ms veh_new( cur_pos + cur_grab.xy() + dp_veh.xy(), start.z() );
                // Use veh_at() for other-vehicle check - multi-tile vehicles have
                // passable parts (aisles) where move_cost > 0 but collision still occurs.
                const optional_vpart_position ovp_new = here.veh_at( veh_new );
                const bool other_veh = ovp_new && &ovp_new->vehicle() != &grabbed_veh;
                // Push/zigzag: vehicle goes to unvisited tiles, doors stay closed.
                const bool allow_doors = !( is_push || is_zigzag );
                const bool veh_blocked = other_veh || tile_blocks_vehicle( here, veh_new, allow_doors );
                if( veh_blocked ) {
                    // Zigzag recovery: fall back to pull when vehicle move collides.
                    // Only for zigzag - game doesn't recover push collisions.
                    if( is_zigzag ) {
                        dp_veh = -cur_grab;
                        next_grab = -dp;
                        const tripoint_bub_ms veh_recover( cur_pos + cur_grab.xy() + dp_veh.xy(),
                                                           start.z() );
                        const optional_vpart_position ovp_rec = here.veh_at( veh_recover );
                        const bool rec_other_veh = ovp_rec && &ovp_rec->vehicle() != &grabbed_veh;
                        if( rec_other_veh || tile_blocks_vehicle( here, veh_recover ) ) {
                            continue;
                        }
                        veh_terrain_cost = veh_drag_cost( here, veh_recover );
                    } else {
                        continue;
                    }
                } else {
                    veh_terrain_cost = veh_drag_cost( here, veh_new );
                }
            }

            const int next_grab_idx = grab_to_idx( next_grab );
            const int next_state = grab_state_index( next_pos, next_grab_idx );

            if( closed[next_state] ) {
                continue;
            }

            // Diagonal penalty (same as main A*)
            const bool diagonal = cur_pos.x() != next_pos.x() && cur_pos.y() != next_pos.y();
            const int newg = cur_g + tile_cost + veh_terrain_cost + ( diagonal ? 1 : 0 );

            if( open[next_state] && newg >= gscore[next_state] ) {
                continue;
            }

            open[next_state] = true;
            gscore[next_state] = newg;
            parent[next_state] = cur_state;
            const int h = std::max( 0, 4 * square_dist( next3d, t ) - 2 );
            pq.emplace( newg + h, next_state );
        }
    }

    if( !done ) {
        add_msg_debug( debugmode::DF_ACTIVITY,
                       "route_with_grab: NO PATH FOUND (%d,%d) grab=(%d,%d) to (%d,%d) explored=%d bounds=(%d,%d)-(%d,%d)",
                       start.x(), start.y(), start_grab.x(), start_grab.y(),
                       target.center.x(), target.center.y(), states_explored,
                       min_bound.x(), min_bound.y(), max_bound.x(), max_bound.y() );
        return ret;
    }

    // Reconstruct path: collect player positions from found_state back to start
    int trace = found_state;
    while( trace != start_state ) {
        const point_bub_ms pos = state_to_pos( trace );
        ret.emplace_back( pos, start.z() );
        trace = parent[trace];
    }
    std::reverse( ret.begin(), ret.end() );

    add_msg_debug( debugmode::DF_ACTIVITY,
                   "route_with_grab: found path len=%zu from (%d,%d) to (%d,%d) first_step=(%d,%d)",
                   ret.size(), start.x(), start.y(),
                   target.center.x(), target.center.y(),
                   ret.empty() ? -1 : ret.front().x(), ret.empty() ? -1 : ret.front().y() );
    return ret;
}

bool route_to_destination( Character &you, player_activity &act,
                           const tripoint_bub_ms &dest, zone_activity_stage &stage )
{
    const map &here = get_map();
    std::vector<tripoint_bub_ms> route;

    // Check if route_length() already computed a path for this destination.
    g_route_cache.ensure_valid( you );
    bool used_grab_routing = false;
    bool from_cache = false;
    if( const std::vector<tripoint_bub_ms> *cached = g_route_cache.find( dest ) ) {
        if( !cached->empty() ) {
            route = *cached;
            from_cache = true;
            used_grab_routing = ( you.is_avatar() &&
                                  you.as_avatar()->get_grab_type() == object_type::VEHICLE );
        }
    }

    if( route.empty() && !from_cache ) {
        // Use grab-aware A* when dragging a single-tile vehicle - searches over
        // (position, grab_direction) state so both player and cart can move.
        if( you.is_avatar() && you.as_avatar()->get_grab_type() == object_type::VEHICLE ) {
            const tripoint_bub_ms veh_pos = you.pos_bub() + you.as_avatar()->grab_point;
            const optional_vpart_position ovp = here.veh_at( veh_pos );
            if( ovp && ovp->vehicle().get_points().size() == 1 ) {
                // Single-tile vehicle: use grab-aware A*. If no path found,
                // treat as unreachable (don't fall back to player-only routing
                // which would cause cart collisions).
                used_grab_routing = true;
                route = route_with_grab( here, you, pathfinding_target::adjacent( dest ) );
            } else {
                // Multi-tile vehicle or no vehicle at grab point: use normal
                // pathfinding (grab-aware A* doesn't support multi-tile).
                route = here.route( you, pathfinding_target::adjacent( dest ) );
            }
        } else {
            route = here.route( you, pathfinding_target::adjacent( dest ) );
        }
    }

    add_msg_debug( debugmode::DF_ACTIVITY,
                   "route_to_dest: dest=(%d,%d) %s route_len=%zu %s",
                   dest.x(), dest.y(),
                   from_cache ? "cached" :
                   ( used_grab_routing ? "grab_astar" : "normal_astar" ),
                   route.size(),
                   route.empty() ? "FAILED" : "OK" );

    if( route.empty() ) {
        add_msg( m_info, _( "%s can't reach the source tile.  Try to sort out loot without a cart." ),
                 you.disp_name() );
        return false;
    }
    // set the destination and restart activity after player arrives there
    // note: we don't need to check for safe mode,
    // the activity will be restarted only if player arrives on destination tile
    stage = DO;
    you.set_destination( route, act );
    you.activity.set_to_null();
    return true;
}

bool sort_skip_item( Character &you, const item *it,
                     const std::vector<item_location> &other_activity_items,
                     bool ignore_favorite, const tripoint_abs_ms &src,
                     bool from_vehicle )
{
    const zone_manager &mgr = zone_manager::get_manager();

    // skip unpickable liquid
    if( !it->made_of_from_type( phase_id::SOLID ) ) {
        return true;
    }

    // don't steal disassembly in progress
    if( it->has_var( "activity_var" ) ) {
        return true;
    }

    // don't steal items from other activities, e.g. crafts in progress
    if( std::find_if( other_activity_items.begin(), other_activity_items.end(),
    [&it]( const item_location & activity_it ) {
    return activity_it.get_item() == it;
    } ) != other_activity_items.end() ) {
        return true;
    }

    // skip items not owned by you
    if( !it->is_owned_by( you, true ) ) {
        return true;
    }

    if( it->is_favorite && ignore_favorite ) {
        return true;
    }

    const faction_id fac_id = _fac_id( you );
    const zone_type_id zt_id = mgr.get_near_zone_type_for_item( *it, you.pos_abs(),
                               MAX_VIEW_DISTANCE, fac_id );
    // Skip items already at their destination. Use binding-aware lookup so a
    // vehicle item is only considered "at destination" if the destination zone
    // is bound to vehicle cargo, and likewise for ground items.
    // LOOT_CUSTOM and LOOT_ITEM_GROUP need filter-based checking below.
    if( zt_id != zone_type_LOOT_CUSTOM && zt_id != zone_type_LOOT_ITEM_GROUP &&
        ( from_vehicle ? mgr.has_vehicle( zt_id, src, fac_id )
          : mgr.has_terrain( zt_id, src, fac_id ) ) ) {
        return true;
    }
    // Custom and item-group zones: check filters with binding awareness
    if( ( zt_id == zone_type_LOOT_CUSTOM || zt_id == zone_type_LOOT_ITEM_GROUP ) &&
        mgr.custom_loot_has( src, it, zt_id, fac_id, from_vehicle ) ) {
        return true;
    }

    return false;
}

unload_sort_options set_unload_options( Character &you, const tripoint_abs_ms &src,
                                        bool use_zone_type )
{
    const zone_manager &mgr = zone_manager::get_manager();
    unload_sort_options zone_sort_options;

    std::vector<zone_data const *> const zones = mgr.get_zones_at( src, zone_type_UNLOAD_ALL,
            _fac_id( you ) );

    // set rules from all zones in stack
    for( zone_data const *zone : zones ) {
        if( !zone->get_enabled() ) {
            continue;
        };
        unload_options const &options = dynamic_cast<const unload_options &>( zone->get_options() );
        zone_sort_options.unload_molle |= options.unload_molle();
        zone_sort_options.unload_mods |= options.unload_mods();
        zone_sort_options.unload_always |= options.unload_always();

        zone_sort_options.unload_sparse_only |= options.unload_sparse_only();
        if( options.unload_sparse_only() &&
            options.unload_sparse_threshold() > zone_sort_options.unload_sparse_threshold ) {
            zone_sort_options.unload_sparse_threshold = options.unload_sparse_threshold();
        }

        if( use_zone_type ) {
            zone_sort_options.ignore_favorite |= zone->get_type() == zone_type_LOOT_IGNORE_FAVORITES;
            zone_sort_options.unload_all |= zone->get_type() == zone_type_UNLOAD_ALL;
            zone_sort_options.unload_corpses |= zone->get_type() == zone_type_STRIP_CORPSES;
        }
    }
    return zone_sort_options;
}

zone_items populate_items( const tripoint_bub_ms &src_bub )
{
    map &here = get_map();
    const std::optional<vpart_reference> vp = here.veh_at( src_bub ).cargo();

    zone_items items;
    // Collect items from both vehicle cargo and ground at this tile.
    // The bool in each pair tracks whether the item is from vehicle cargo.
    if( vp ) {
        for( item &it : vp->items() ) {
            items.emplace_back( &it, true );
        }
    }
    for( item &it : here.i_at( src_bub ) ) {
        items.emplace_back( &it, false );
    }
    return items;
}

bool ignore_contents( Character &you, const tripoint_abs_ms &src )
{
    const zone_manager &mgr = zone_manager::get_manager();
    // check ignorable zones for ignore_contents enabled
    for( const auto &zone_type : ignorable_zone_types ) {
        // add zones using mgr.get_zones_at
        for( zone_data const *zone : mgr.get_zones_at( src, zone_type, _fac_id( you ) ) ) {
            ignorable_options const &options = dynamic_cast<const ignorable_options &>( zone->get_options() );
            if( options.get_ignore_contents() ) {
                return true;
            }
        }
    }
    return false;
}

bool ignore_zone_position( Character &you, const tripoint_abs_ms &src,
                           bool ignore_contents )
{
    const zone_manager &mgr = zone_manager::get_manager();
    map &here = get_map();
    const tripoint_bub_ms src_bub = here.get_bub( src );

    return mgr.has( zone_type_LOOT_IGNORE, src, _fac_id( you ) ) ||
           ignore_contents ||
           here.get_field( src_bub, fd_fire ) != nullptr ||
           !here.can_put_items_ter_furn( src_bub ) ||
           here.impassable_field_at( src_bub );
}

bool has_items_to_sort( Character &you, const tripoint_abs_ms &src,
                        unload_sort_options zone_unload_options,
                        const std::vector<item_location> &other_activity_items,
                        const zone_items &items, bool *pickup_failure )
{
    const zone_manager &mgr = zone_manager::get_manager();
    const faction_id fac_id = _fac_id( you );
    const tripoint_abs_ms &abspos = you.pos_abs();

    *pickup_failure = false;

    // Any UNSORTED zone at src (terrain or vehicle) makes all items at that
    // tile eligible for sorting. The terrain/vehicle distinction only matters
    // at the destination (where items get placed), not at the source.
    const bool src_has_unsorted = mgr.has( zone_type_LOOT_UNSORTED, src, fac_id );
    const bool src_has_vehicle_unsorted = mgr.has_vehicle( zone_type_LOOT_UNSORTED, src, fac_id );

    // When grabbed cart sits on the source tile, items stay in cart cargo
    // (virtual pickup) so the player carry capacity check doesn't apply.
    bool virtual_pickup_available = false;
    if( you.is_avatar() && you.as_avatar()->get_grab_type() == object_type::VEHICLE ) {
        const tripoint_bub_ms cart_pos = you.pos_bub() + you.as_avatar()->grab_point;
        if( get_map().get_abs( cart_pos ) == src ) {
            virtual_pickup_available = get_map().veh_at( cart_pos ).cargo().has_value();
        }
    }

    // When the grabbed cart is at a terrain-only unsorted zone (no vehicle
    // zone), it's being used for transport - don't re-sort its cargo.
    // If there IS a vehicle zone on the cart, the user explicitly wants
    // the cart's cargo sorted.
    const bool skip_cart_cargo = virtual_pickup_available && !src_has_vehicle_unsorted;

    for( std::pair<item *, bool> it_pair : items ) {
        if( !src_has_unsorted ) {
            continue;
        }
        if( it_pair.second && skip_cart_cargo ) {
            continue;
        }

        item *it = it_pair.first;
        const zone_type_id dest_zone_type_id = mgr.get_near_zone_type_for_item( *it, abspos,
                                               MAX_VIEW_DISTANCE, fac_id );

        if( dest_zone_type_id == zone_type_id::NULL_ID() ) {
            continue;
        }

        // Virtual pickup only applies to vehicle items
        if( !( virtual_pickup_available && it_pair.second ) && !you.can_add( *it ) ) {
            bool vehicle_can_hold = false;
            if( you.is_avatar() && you.as_avatar()->get_grab_type() == object_type::VEHICLE ) {
                const tripoint_bub_ms cart_pos = you.pos_bub() + you.as_avatar()->grab_point;
                if( std::optional<vpart_reference> ovp = get_map().veh_at( cart_pos ).cargo() ) {
                    // Approximation: only checks volume, not weight or other cargo constraints.
                    // If this over-reports, the empty-pickup guard in stage_do handles it.
                    vehicle_can_hold = ovp->items().free_volume() >= it->volume();
                }
            }
            if( !vehicle_can_hold ) {
                *pickup_failure = true;
                continue;
            }
        }

        if( sort_skip_item( you, it, other_activity_items,
                            zone_unload_options.ignore_favorite, src, it_pair.second ) ) {
            continue;
        }

        const std::unordered_set<tripoint_abs_ms> dest_set =
            mgr.get_near( dest_zone_type_id, abspos, MAX_VIEW_DISTANCE, it, fac_id );

        //if we're unloading all or a corpse
        if( zone_unload_options.unload_all || ( zone_unload_options.unload_corpses && it->is_corpse() ) ) {
            if( dest_set.empty() || zone_unload_options.unload_always ) {
                if( you.rate_action_unload( *it ) == hint_rating::good &&
                    !it->any_pockets_sealed() ) {
                    //we can unload this item, so stop here
                    return true;
                }

                // if unloading mods
                if( zone_unload_options.unload_mods ) {
                    // remove each mod, skip irremovable
                    for( const item *mod : it->gunmods() ) {
                        if( mod->is_irremovable() ) {
                            continue;
                        }
                        // we can remove a mod, so stop here
                        return true;
                    }
                }

                // if unloading molle
                if( zone_unload_options.unload_molle && !it->get_contents().get_added_pockets().empty() ) {
                    // we can unload the MOLLE, so stop here
                    return true;
                }
            }
        }
        // if item has destination
        if( !dest_set.empty() ) {
            return true;
        }
    }
    return false;
}

bool can_unload( item *it )
{
    return it->made_of( phase_id::SOLID );
}

void add_item( const std::optional<vpart_reference> &vp,
               const tripoint_bub_ms &src_bub,
               const item &it )
{
    map &here = get_map();
    if( vp ) {
        vp->vehicle().add_item( here, vp->part(), it );
    } else {
        here.add_item_or_charges( src_bub, it );
    }
}

void remove_item( const std::optional<vpart_reference> &vp,
                  const tripoint_bub_ms &src_bub,
                  item *it )
{
    map &here = get_map();
    if( vp ) {
        vp->vehicle().remove_item( vp->part(), it );
    } else {
        here.i_rem( src_bub, it );
    }
}

std::optional<bool> unload_item( Character &you, const tripoint_abs_ms &src,
                                 unload_sort_options zone_unload_options, const std::optional<vpart_reference> &vpr_src,
                                 item *it, const std::unordered_set<tripoint_abs_ms> &dest_set,
                                 int &num_processed )
{
    const zone_manager &mgr = zone_manager::get_manager();
    const faction_id fac_id = _fac_id( you );
    map &here = get_map();
    const tripoint_bub_ms src_bub = here.get_bub( src );
    const tripoint_abs_ms &abspos = you.pos_abs();
    // if this item isn't going anywhere and its not sealed
    // check if it is in a unload zone or a strip corpse zone
    // then we should unload it and see what is inside
    bool move_and_reset = false;
    bool moved_something = false;

    // teleport an item from container to ground
    // TODO: less egregious than teleporting over a distance, but still not good
    auto unload_teleport_item = [&you, &src_bub, &vpr_src, &it]( item * contained ) {
        move_item( you, *contained, contained->count(), src_bub, src_bub, vpr_src );
        it->remove_item( *contained );
    };

    if( mgr.has_near( zone_type_UNLOAD_ALL, abspos, 1, fac_id ) ||
        ( mgr.has_near( zone_type_STRIP_CORPSES, abspos, 1, fac_id ) && it->is_corpse() ) ) {
        if( dest_set.empty() || zone_unload_options.unload_always ) {

            if( you.rate_action_unload( *it ) == hint_rating::good &&
                !it->any_pockets_sealed() ) {

                //first, count items by type at top-level
                std::unordered_map<itype_id, int> item_counts;
                if( zone_unload_options.unload_sparse_only ) {
                    for( item *contained : it->all_items_top( pocket_type::CONTAINER ) ) {
                        if( zone_sorting::can_unload( contained ) ) {
                            item_counts[contained->typeId()]++;
                        }
                        if( you.get_moves() <= 0 ) {
                            return std::nullopt;
                        }
                    }
                }

                //unload all containers
                //if the item count is below the sparse threshold set above, don't unload
                for( item *contained : it->all_items_top( pocket_type::CONTAINER ) ) {
                    if( zone_sorting::can_unload( contained ) ) {
                        if( zone_unload_options.unload_sparse_only &&
                            item_counts[contained->typeId()] > zone_unload_options.unload_sparse_threshold ) {
                            continue;
                        }
                        unload_teleport_item( contained );
                    }
                    if( you.get_moves() <= 0 ) {
                        return std::nullopt;
                    }
                }

                //unload all magazines
                for( item *contained : it->all_items_top( pocket_type::MAGAZINE ) ) {
                    if( zone_sorting::can_unload( contained ) ) {
                        if( it->is_ammo_belt() ) {
                            if( it->type->magazine->linkage ) {
                                item link( *it->type->magazine->linkage, calendar::turn, contained->count() );
                                zone_sorting::add_item( vpr_src, src_bub, link );
                            }
                        }
                        unload_teleport_item( contained );
                    }

                    // destroy fully unloaded magazines
                    if( it->has_flag( flag_MAG_DESTROY ) && it->ammo_remaining() == 0 ) {
                        zone_sorting::remove_item( vpr_src, src_bub, it );
                        num_processed = std::max( num_processed - 1, 0 );
                        return std::nullopt;
                    }

                    if( you.get_moves() <= 0 ) {
                        return std::nullopt;
                    }
                }

                //unload all magazine wells
                for( item *contained : it->all_items_top( pocket_type::MAGAZINE_WELL ) ) {
                    if( zone_sorting::can_unload( contained ) ) {
                        unload_teleport_item( contained );
                    }
                }
                moved_something = true;

            }

            // if unloading mods
            if( zone_unload_options.unload_mods ) {
                // remove each mod, skip irremovable
                for( item *mod : it->gunmods() ) {
                    if( mod->is_irremovable() ) {
                        continue;
                    }
                    you.gunmod_remove( *it, *mod );
                    // need to return so the remove gunmod activity starts
                    return std::nullopt;
                }
            }

            // if unloading molle
            if( zone_unload_options.unload_molle ) {
                while( !it->get_contents().get_added_pockets().empty() ) {
                    item removed = it->get_contents().remove_pocket( 0 );
                    move_item( you, removed, 1, src_bub, src_bub, vpr_src );
                    moved_something = true;
                    if( you.get_moves() <= 0 ) {
                        return std::nullopt;
                    }
                }
            }

            // after dumping items go back to start of activity loop
            // so that can re-assess the items in the tile
            // perhaps move the last item first however
            if( zone_unload_options.unload_always && moved_something ) {
                move_and_reset = true;
            } else if( moved_something ) {
                return std::nullopt;
            }
        }
    }
    return move_and_reset;
}

//TODO: teleports item without picking it up; remove this behavior
void move_item( Character &you, const std::optional<vpart_reference> &vpr_src,
                const tripoint_bub_ms &src_bub, const std::unordered_set<tripoint_abs_ms> &dest_set,
                item &it, int &num_processed )
{
    map &here = get_map();

    for( const tripoint_abs_ms &dest : dest_set ) {
        const tripoint_bub_ms dest_loc = here.get_bub( dest );
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
        if( free_space >= it.volume() ) {
            move_item( you, it, it.count(), src_bub, dest_loc, vpr_src );

            // moved item away from source so decrement
            if( num_processed > 0 ) {
                --num_processed;
            }
            break;
        }
    }
}
int route_length( const Character &you, const tripoint_bub_ms &dest )
{
    if( square_dist( you.pos_bub(), dest ) <= 1 ) {
        return 0;
    }

    g_route_cache.ensure_valid( you );

    if( const std::vector<tripoint_bub_ms> *cached = g_route_cache.find( dest ) ) {
        return cached->empty() ? INT_MAX : static_cast<int>( cached->size() );
    }

    const map &here = get_map();
    std::vector<tripoint_bub_ms> route;

    if( you.is_avatar() && you.as_avatar()->get_grab_type() == object_type::VEHICLE ) {
        const tripoint_bub_ms veh_pos = you.pos_bub() + you.as_avatar()->grab_point;
        const optional_vpart_position ovp = here.veh_at( veh_pos );
        if( ovp && ovp->vehicle().get_points().size() == 1 ) {
            route = route_with_grab( here, you, pathfinding_target::adjacent( dest ) );
        } else {
            route = here.route( you, pathfinding_target::adjacent( dest ) );
        }
    } else {
        route = here.route( you, pathfinding_target::adjacent( dest ) );
    }

    g_route_cache.store( dest, route );
    return route.empty() ? INT_MAX : static_cast<int>( route.size() );
}
} //namespace zone_sorting

std::vector<tripoint_bub_ms> route_adjacent( const Character &you, const tripoint_bub_ms &dest )
{
    std::unordered_set<tripoint_bub_ms> passable_tiles;
    map &here = get_map();

    for( const tripoint_bub_ms &tp : here.points_in_radius( dest, 1 ) ) {
        if( tp != you.pos_bub() && here.passable_through( tp ) ) {
            passable_tiles.emplace( tp );
        }
    }

    const std::vector<tripoint_bub_ms> &sorted =
        get_sorted_tiles_by_distance( you.pos_bub(), passable_tiles );

    for( const tripoint_bub_ms &tp : sorted ) {
        std::vector<tripoint_bub_ms> route =
            here.route( you, pathfinding_target::point( tp ) );

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
        if( tp == you.pos_bub() || ( here.passable_through( tp ) && !creatures.creature_at( tp ) ) ) {
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
    if( sorted.front() == you.pos_bub() ) {
        // We are on the best tile
        return {};
    }
    for( const tripoint_bub_ms &tp : sorted ) {
        std::vector<tripoint_bub_ms> route =
            here.route( you, pathfinding_target::point( tp ) );

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
           ( ( check.pre_terrain.find( idx->post_terrain ) == check.pre_terrain.end() ) &&
             can_construct( check, loc ) );
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
               ( idx->pre_terrain.find( it.post_terrain ) != idx->pre_terrain.end() )  &&
               // don't get stuck building and deconstructing the top level post_terrain
               ( it.pre_terrain.find( top_idx->post_terrain ) == it.pre_terrain.end() )  &&
               ( it.pre_flags.empty() || !has_pre_flags( it, f, t ) );
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
    const furn_id &furn = here.furn( loc );
    const ter_id &ter = here.ter( loc );
    return !build.post_terrain.empty() &&
           ( ( !build.post_is_furniture && ter_id( build.post_terrain ) == ter ) ||
             ( build.post_is_furniture && furn_id( build.post_terrain ) == furn ) );
}

} // namespace

static activity_reason_info find_base_construction(
    Character &you,
    const tripoint_bub_ms &inv_from_loc,
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
    if( already_done( build, loc ) ) {
        return activity_reason_info::build( do_activity_reason::ALREADY_DONE, false, build.id );
    }
    if( !you.meets_skill_requirements( build ) ) {
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
    if( !cc ) {
        return activity_reason_info::build( do_activity_reason::BLOCKING_TILE, false, idx );
    }
    const inventory &inv = you.crafting_inventory( inv_from_loc, PICKUP_RANGE );
    if( !player_can_build( you, inv, build, true ) ) {
        //can't build with current inventory, do not look for pre-req
        return activity_reason_info::build( do_activity_reason::NO_COMPONENTS, false, build.id );
    }
    return activity_reason_info::build( do_activity_reason::CAN_DO_CONSTRUCTION, true, build.id );
}

namespace multi_activity_actor
{

bool are_requirements_nearby(
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
        volume_allowed = you.free_space();
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
            if( mgr.has( zone_type_LOOT_IGNORE, here.get_abs( elem ), _fac_id( you ) ) ||
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
            const std::optional<vpart_reference> &vp = here.veh_at( elem ).part_with_tool( here, itype_welder );

            if( vp ) {
                const int veh_battery = vp->vehicle().fuel_left( here, itype_battery );

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

} //namespace multi_activity_actor

//common function for deconstruction/repair
static activity_reason_info vehicle_work_can_do( const activity_id &, Character &you,
        const tripoint_bub_ms &src_loc, std::vector<int> &already_working_indexes,
        vehicle *veh )
{

    Character &player_character = get_player_character();
    map &here = get_map();

    if( !veh || veh->is_appliance() ) {
        return activity_reason_info::fail( do_activity_reason::NO_VEHICLE );
    }
    // if the vehicle is moving or player is controlling it.
    if( std::abs( veh->velocity ) > 100 || veh->player_in_control( here, player_character ) ) {
        return activity_reason_info::fail( do_activity_reason::NO_VEHICLE );
    }
    do_activity_reason result = do_activity_reason::NO_ZONE;
    for( const npc &guy : g->all_npcs() ) {
        if( &guy == &you ) {
            continue;
        }
        // If the NPC has an activity - make sure they're not duplicating work.
        tripoint_bub_ms guy_work_spot;
        if( guy.has_player_activity() &&
            guy.activity.placement != player_activity::invalid_place ) {
            guy_work_spot = here.get_bub( guy.activity.placement );
        }
        // If their position or intended position or player position/intended position
        // then discount, don't need to move each other out of the way.
        if( here.get_bub( player_character.activity.placement ) == src_loc ||
            guy_work_spot == src_loc || guy.pos_bub() == src_loc ||
            ( you.is_npc() && player_character.pos_bub() == src_loc ) ) {
            return activity_reason_info::fail( do_activity_reason::ALREADY_WORKING );
        }
        if( guy_work_spot != tripoint_bub_ms::zero ) {
            vehicle *other_veh = veh_pointer_or_null( here.veh_at( guy_work_spot ) );
            // working on same vehicle - store the index to check later.
            if( other_veh && other_veh == veh && guy.activity_vehicle_part_index != -1 ) {
                already_working_indexes.push_back( guy.activity_vehicle_part_index );
            }
        }
    }
    if( you.is_npc() && player_character.activity_vehicle_part_index != -1 ) {
        already_working_indexes.push_back( player_character.activity_vehicle_part_index );
    }
    return activity_reason_info::ok( result );
}

activity_reason_info multi_vehicle_deconstruct_activity_actor::multi_activity_can_do(
    Character &you,
    const tripoint_bub_ms &src_loc )
{
    const activity_id act = get_type();
    Character &player_character = get_player_character();
    map &here = get_map();

    std::vector<int> already_working_indexes;
    vehicle *veh = veh_pointer_or_null( here.veh_at( src_loc ) );

    activity_reason_info common_vehicle_work = vehicle_work_can_do( act, you, src_loc,
            already_working_indexes, veh );
    if( !common_vehicle_work.can_do ) {
        return common_vehicle_work;
    }
    do_activity_reason failed_work = common_vehicle_work.reason;

    // find out if there is a vehicle part here we can remove.
    std::vector<vehicle_part *> parts =
        veh->get_parts_at( &here, src_loc, "", part_status_flag::any );
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
            if( failed_work == do_activity_reason::NO_ZONE ) {
                failed_work = do_activity_reason::DONT_HAVE_SKILL;
            }
            continue;
        }
        item base( vpinfo.base_item );
        const units::mass max_lift = you.best_nearby_lifting_assist( src_loc );
        const bool use_aid = max_lift >= base.weight();
        const bool use_str = you.can_lift( base );
        if( !( use_aid || use_str ) ) {
            failed_work = do_activity_reason::NO_COMPONENTS;
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
    you.activity_vehicle_part_index = -1;
    return activity_reason_info::fail( failed_work );
}

activity_reason_info multi_vehicle_repair_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{
    const activity_id act = get_type();
    Character &player_character = get_player_character();
    map &here = get_map();

    std::vector<int> already_working_indexes;
    vehicle *veh = veh_pointer_or_null( here.veh_at( src_loc ) );

    activity_reason_info common_vehicle_work = vehicle_work_can_do( act, you, src_loc,
            already_working_indexes, veh );
    if( !common_vehicle_work.can_do ) {
        return common_vehicle_work;
    }
    do_activity_reason failed_work = common_vehicle_work.reason;

    // find out if there is a vehicle part here we can repair.
    std::vector<vehicle_part *> parts = veh->get_parts_at( &here, src_loc, "", part_status_flag::any );
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
            if( failed_work == do_activity_reason::NO_ZONE ) {
                failed_work = do_activity_reason::DONT_HAVE_SKILL;
            }
            continue;
        }
        const requirement_data &reqs = vpinfo.repair_requirements();
        const inventory &inv =
            you.crafting_inventory( src_loc, PICKUP_RANGE - 1, false );
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
    you.activity_vehicle_part_index = -1;
    return activity_reason_info::fail( failed_work );
}

activity_reason_info multi_mine_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{

    map &here = get_map();

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

activity_reason_info multi_mop_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{

    map &here = get_map();

    if( !here.terrain_moppable( src_loc ) ) {
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }

    if( you.cache_has_item_with( json_flag_MOP ) ) {
        return activity_reason_info::ok( do_activity_reason::NEEDS_MOP );
    } else {
        return activity_reason_info::fail( do_activity_reason::NEEDS_MOP );
    }
}

activity_reason_info multi_fish_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{

    map &here = get_map();

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

activity_reason_info multi_chop_trees_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{

    map &here = get_map();

    const ter_id &t = here.ter( src_loc );
    if( t == ter_t_trunk || t == ter_t_stump || here.has_flag( ter_furn_flag::TFLAG_TREE, src_loc ) ) {
        if( you.has_quality( qual_AXE ) ) {
            return activity_reason_info::ok( do_activity_reason::NEEDS_TREE_CHOPPING );
        } else {
            return activity_reason_info::fail( do_activity_reason::NEEDS_TREE_CHOPPING );
        }
    } else {
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }
}

activity_reason_info multi_butchery_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{
    map &here = get_map();

    std::vector<item> corpses;
    int big_count = 0;
    int small_count = 0;
    for( const item &i : here.i_at( src_loc ) ) {
        // make sure nobody else is working on that corpse right now
        if( i.is_corpse() &&
            ( !i.has_var( "activity_var" ) || i.get_var( "activity_var" ) == you.name ) ) {
            const mtype corpse = *i.get_mtype();
            if( corpse.size > creature_size::medium ) {
                big_count += 1;
            } else {
                small_count += 1;
            }
            corpses.push_back( i );
        }
    }
    if( corpses.empty() ) {
        return activity_reason_info::fail( do_activity_reason::NO_COMPONENTS );
    }
    bool b_rack_present = false;
    for( const tripoint_bub_ms &pt : here.points_in_radius( src_loc, 2 ) ) {
        if( here.has_flag_furn( ter_furn_flag::TFLAG_BUTCHER_EQ, pt ) ) {
            b_rack_present = true;
        }
    }
    if( !corpses.empty() ) {
        for( item &body : corpses ) {
            const mtype &corpse = *body.get_mtype();
            for( species_id species : corpse.species ) {
                if( you.empathizes_with_species( species ) ) {
                    return activity_reason_info::fail( do_activity_reason::REFUSES_THIS_WORK );
                }
            }
        }
        if( big_count > 0 && small_count == 0 ) {
            if( !b_rack_present ) {
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

activity_reason_info multi_read_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms & )
{
    const item_filter filter = [&you]( const item & i ) {
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

activity_reason_info multi_study_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{
    map &here = get_map();
    zone_manager &mgr = zone_manager::get_manager();
    const tripoint_abs_ms abspos = here.get_abs( src_loc );
    if( !mgr.has( zone_type_STUDY_ZONE, abspos, _fac_id( you ) ) ) {
        return activity_reason_info::fail( do_activity_reason::NO_ZONE );
    }

    item_location book = find_study_book( abspos, you );
    if( book ) {
        book->set_var( "activity_var", you.name );
        return activity_reason_info::ok( do_activity_reason::NEEDS_BOOK_TO_LEARN );
    }

    return activity_reason_info::fail( do_activity_reason::ALREADY_DONE );
}

activity_reason_info multi_chop_planks_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{

    map &here = get_map();
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

activity_reason_info multi_build_construction_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{

    map &here = get_map();
    zone_manager &mgr = zone_manager::get_manager();
    std::vector<zone_data> zones;

    zones = mgr.get_zones( zone_type_CONSTRUCTION_BLUEPRINT,
                           here.get_abs( src_loc ), _fac_id( you ) );
    const partial_con *part_con = here.partial_con_at( src_loc );
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
    if( !zones.empty() ) {
        const blueprint_options &options = dynamic_cast<const blueprint_options &>
                                           ( zones.front().get_options() );
        const construction_id index = options.get_index();
        return find_base_construction( you, nearest_src_loc, src_loc, part_con_idx,
                                       index );
    }
    return activity_reason_info::fail( do_activity_reason::NO_ZONE );
}
activity_reason_info multi_farm_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{

    map &here = get_map();
    zone_manager &mgr = zone_manager::get_manager();
    std::vector<zone_data> zones;

    zones = mgr.get_zones( zone_type_FARM_PLOT, here.get_abs( src_loc ), _fac_id( you ) );
    for( const zone_data &zone : zones ) {
        const plot_options &options = dynamic_cast<const plot_options &>( zone.get_options() );
        const itype_id seed = options.get_seed();
        ret_val<void>can_plant = !seed.is_empty() ?
                                 warm_enough_to_plant( src_loc, seed ) : ret_val<void>::make_success();

        if( here.has_flag_furn( ter_furn_flag::TFLAG_GROWTH_OVERGROWN, src_loc ) ) {
            return activity_reason_info::ok( do_activity_reason::NEEDS_CLEARING );
        }

        if( here.has_flag_furn( ter_furn_flag::TFLAG_GROWTH_HARVEST, src_loc ) ) {
            map_stack items = here.i_at( src_loc );
            const map_stack::iterator seed_iter =
            std::find_if( items.begin(), items.end(), []( const item & it ) {
                return it.is_seed();
            } );
            if( seed_iter == items.end() ) {
                debugmsg( "Missing seed item at %s", src_loc.to_string() );
                return activity_reason_info::fail( do_activity_reason::ALREADY_DONE );
            } else if( seed_iter->has_flag( json_flag_CUT_HARVEST ) ) {
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
        }
        // there's a plant that isn't overgrown or harvestable, apply fertilizer if possible
        else if( here.has_flag_furn( ter_furn_flag::TFLAG_PLANT, src_loc ) &&
                 multi_farm_activity_actor::can_fertilize( you, src_loc ).success() &&
                 !get_first_fertilizer_itype( you, here.get_abs( src_loc ) ).is_null() ) {
            return activity_reason_info::ok( do_activity_reason::NEEDS_FERTILIZING );
        } else if( here.has_flag( ter_furn_flag::TFLAG_PLOWABLE, src_loc ) && !here.has_furn( src_loc ) ) {
            if( you.has_quality( qual_DIG, 1 ) ) {
                // we have a shovel/hoe already, great
                return activity_reason_info::ok( do_activity_reason::NEEDS_TILLING );
            } else {
                // we need a shovel/hoe
                return activity_reason_info::fail( do_activity_reason::NEEDS_TILLING );
            }
            // do we have the required seed on our person?
            // If its a farm zone with no specified seed, and we've checked for tilling and harvesting.
            // then it means no further work can be done here
        } else if( !seed.is_empty() &&
                   can_plant.success() &&
                   here.has_flag_ter_or_furn( seed->seed->required_terrain_flag, src_loc ) ) {
            if( here.has_items( src_loc ) ) {
                return activity_reason_info::fail( do_activity_reason::BLOCKING_TILE );
            } else {
                if( you.cache_has_item_with( "is_seed", &item::is_seed, [&seed]( const item & it ) {
                return it.typeId() == itype_id( seed );
                } ) ) {
                    return activity_reason_info::ok( do_activity_reason::NEEDS_PLANTING );
                }
                // didn't find the seed, but maybe there are overlapping farm zones
                // and another of the zones is for a seed that we have
                // so loop again, and return false once all zones are done.
            }

        } else {
            // Extra, specific messaging returned from warm_enough_to_plant()
            if( !can_plant.success() ) {
                you.add_msg_if_player( can_plant.c_str() );
            }
            // can't plant, till or harvest
            return activity_reason_info::fail( do_activity_reason::ALREADY_DONE );
        }

    }
    // looped through all zones, and only got here if its plantable, but have no seeds.
    return activity_reason_info::fail( do_activity_reason::NEEDS_PLANTING );
}

activity_reason_info fetch_required_activity_actor::multi_activity_can_do( Character &,
        const tripoint_bub_ms & )
{
    // We check if it's possible to get all the requirements for fetching at two other places:
    // 1. before we even assign the fetch activity and;
    // 2. when we form the src_set to loop through at the beginning of the fetch activity.
    return activity_reason_info::ok( do_activity_reason::CAN_DO_FETCH );
}

activity_reason_info multi_craft_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{
    // only npc is supported
    npc *p = you.as_npc();
    if( p ) {
        item_location to_craft = p->get_item_to_craft();
        if( to_craft && to_craft->is_craft() ) {
            const inventory &inv = you.crafting_inventory( src_loc, PICKUP_RANGE, false );
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
}

activity_reason_info multi_disassemble_activity_actor::multi_activity_can_do( Character &you,
        const tripoint_bub_ms &src_loc )
{

    map &here = get_map();
    // Is there anything to be disassembled?
    const inventory &inv = you.crafting_inventory( src_loc, PICKUP_RANGE, false );
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
        return activity_reason_info::fail( do_activity_reason::NO_COMPONENTS );
    }
}

namespace multi_activity_actor
{

bool can_do_in_dark( const activity_id &act_id )
{
    return act_id == ACT_MULTIPLE_MOP ||
           act_id == ACT_MOVE_LOOT ||
           act_id == ACT_FETCH_REQUIRED;
}

bool activity_reason_quit( do_activity_reason reason )
{
    return reason == do_activity_reason::DONT_HAVE_SKILL ||
           reason == do_activity_reason::NO_ZONE ||
           reason == do_activity_reason::NO_VEHICLE ||
           reason == do_activity_reason::ALREADY_DONE ||
           reason == do_activity_reason::BLOCKING_TILE ||
           reason == do_activity_reason::UNKNOWN_ACTIVITY;
}

bool activity_reason_continue( do_activity_reason reason )
{
    return
        reason == do_activity_reason::NO_COMPONENTS ||
        reason == do_activity_reason::NEEDS_CUT_HARVESTING ||
        reason == do_activity_reason::NEEDS_PLANTING ||
        reason == do_activity_reason::NEEDS_TILLING ||
        reason == do_activity_reason::NEEDS_CHOPPING ||
        reason == do_activity_reason::NEEDS_BUTCHERING ||
        reason == do_activity_reason::NEEDS_BIG_BUTCHERING ||
        reason == do_activity_reason::NEEDS_VEH_DECONST ||
        reason == do_activity_reason::NEEDS_VEH_REPAIR ||
        reason == do_activity_reason::NEEDS_TREE_CHOPPING ||
        reason == do_activity_reason::NEEDS_FISHING ||
        reason == do_activity_reason::NEEDS_MINING ||
        reason == do_activity_reason::NEEDS_CRAFT ||
        reason == do_activity_reason::NEEDS_DISASSEMBLE;
}

bool activity_reason_picks_up_tools( do_activity_reason reason )
{
    return
        reason == do_activity_reason::NEEDS_CUT_HARVESTING ||
        reason == do_activity_reason::NEEDS_PLANTING ||
        reason == do_activity_reason::NEEDS_TILLING ||
        reason == do_activity_reason::NEEDS_CHOPPING ||
        reason == do_activity_reason::NEEDS_BUTCHERING ||
        reason == do_activity_reason::NEEDS_BIG_BUTCHERING ||
        reason == do_activity_reason::NEEDS_VEH_DECONST ||
        reason == do_activity_reason::NEEDS_VEH_REPAIR ||
        reason == do_activity_reason::NEEDS_TREE_CHOPPING ||
        reason == do_activity_reason::NEEDS_MINING;
}

bool activity_must_be_in_zone( activity_id act_id, const tripoint_bub_ms &src_loc )
{
    map &here = get_map();
    return
        act_id == ACT_FETCH_REQUIRED ||
        act_id == ACT_MULTIPLE_FARM ||
        act_id == ACT_MULTIPLE_BUTCHER ||
        act_id == ACT_MULTIPLE_CHOP_PLANKS ||
        act_id == ACT_MULTIPLE_CHOP_TREES ||
        act_id == ACT_VEHICLE_DECONSTRUCTION ||
        act_id == ACT_VEHICLE_REPAIR ||
        act_id == ACT_MULTIPLE_FISH ||
        act_id == ACT_MULTIPLE_MINE ||
        act_id == ACT_MULTIPLE_DIS ||
        act_id == ACT_MULTIPLE_STUDY ||
        ( act_id == ACT_MULTIPLE_CONSTRUCTION &&
          !here.partial_con_at( src_loc ) );
}

requirement_check_result fetch_requirements( Character &you, requirement_id what_we_need,
        const activity_id &act_id,
        activity_reason_info &act_info, const tripoint_abs_ms &src, const tripoint_bub_ms &src_loc,
        const std::unordered_set<tripoint_abs_ms> &src_set )
{

    map &here = get_map();

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
    act_prev.values.push_back( static_cast<int>( act_info.reason ) );
    // come back here after successfully fetching your stuff
    if( act_prev.coords.empty() ) {
        std::vector<tripoint_bub_ms> local_src_set;
        local_src_set.reserve( src_set.size() );
        for( const tripoint_abs_ms &elem : src_set ) {
            local_src_set.push_back( here.get_bub( elem ) );
        }
        std::vector<tripoint_bub_ms> candidates;
        for( const tripoint_bub_ms &point_elem :
             here.points_in_radius( src_loc, PICKUP_RANGE - 1, 0 ) ) {
            // we don't want to place the components where they could interfere with our ( or someone else's ) construction spots
            if( ( std::find( local_src_set.begin(), local_src_set.end(),
                             point_elem ) != local_src_set.end() ) || !here.can_put_items_ter_furn( point_elem ) ) {
                continue;
            }
            candidates.push_back( point_elem );
        }
        if( candidates.empty() ) {
            you.activity = player_activity();
            you.backlog.clear();
            check_npc_revert( you );
            return requirement_check_result::SKIP_LOCATION_NO_LOCATION;
        }
        act_prev.coords.push_back(
            here.get_abs(
                candidates[std::max( 0, static_cast<int>( candidates.size() / 2 ) )] )
        );
    }
    act_prev.placement = src;
    return requirement_check_result::RETURN_EARLY;
}

requirement_check_result requirement_fail( Character &you, const do_activity_reason &reason,
        const activity_id &act_id, const zone_data *zone )
{
    // we can discount this tile, the work can't be done.
    if( reason == do_activity_reason::DONT_HAVE_SKILL ) {
        if( zone ) {
            you.add_msg_if_player( m_info, _( "You don't have the skill for the %1$s task at zone %2$s." ),
                                   act_id.c_str(), zone->get_name() );
        } else {
            you.add_msg_if_player( m_info, _( "You don't have the skill for the %s task." ), act_id.c_str() );
        }
    } else if( reason == do_activity_reason::BLOCKING_TILE ) {
        if( zone ) {
            you.add_msg_if_player( m_info,
                                   _( "There is something blocking the location for the %1$s task at zone %2$s." ), act_id.c_str(),
                                   zone->get_name() );
        } else {
            you.add_msg_if_player( m_info, _( "There is something blocking the location for the %s task." ),
                                   act_id.c_str() );
        }
    }
    if( you.is_npc() ) {
        if( reason == do_activity_reason::DONT_HAVE_SKILL ) {
            return requirement_check_result::SKIP_LOCATION_NO_SKILL;
        } else if( reason == do_activity_reason::NO_ZONE ) {
            return requirement_check_result::SKIP_LOCATION_NO_ZONE;
        } else if( reason == do_activity_reason::NO_VEHICLE ) {
            return requirement_check_result::SKIP_LOCATION_NO_MATCH;
        } else if( reason == do_activity_reason::ALREADY_DONE ) {
            return requirement_check_result::SKIP_LOCATION;
        } else if( reason == do_activity_reason::BLOCKING_TILE ) {
            return requirement_check_result::SKIP_LOCATION_BLOCKING;
        } else if( reason == do_activity_reason::UNKNOWN_ACTIVITY ) {
            return requirement_check_result::SKIP_LOCATION_UNKNOWN_ACTIVITY;
        }
    } else {
        return requirement_check_result::SKIP_LOCATION;
    }
    return requirement_check_result::SKIP_LOCATION_NO_MATCH;
}

requirement_id hash_and_cache_requirement_data( const requirement_data &reqs_data )
{
    const requirement_id req_id( std::to_string( reqs_data.make_hash() ) );
    if( requirement_data::all().count( req_id ) == 0 ) {
        requirement_data::save_requirement( reqs_data, req_id );
    }
    return req_id;
}

requirement_id synthesize_requirements(
    const requirement_data::alter_item_comp_vector &requirement_comp_vector,
    const requirement_data::alter_quali_req_vector &quality_comp_vector,
    const requirement_data::alter_tool_comp_vector &tool_comp_vector )
{

    requirement_data reqs_data = requirement_data( tool_comp_vector, quality_comp_vector,
                                 requirement_comp_vector );

    return hash_and_cache_requirement_data( reqs_data );
}

requirement_id remove_met_requirements( requirement_id base_req_id, Character &you )
{

    requirement_data reqs = base_req_id.obj();
    // Remove the requirements already met
    requirement_data::alter_tool_comp_vector tool_reqs_vector = reqs.get_tools();
    requirement_data::alter_quali_req_vector quality_reqs_vector = reqs.get_qualities();
    requirement_data::alter_item_comp_vector component_reqs_vector = reqs.get_components();
    requirement_data::alter_tool_comp_vector reduced_tool_reqs_vector;
    requirement_data::alter_quali_req_vector reduced_quality_reqs_vector;

    // We cannot assume that required items on the ground when the multi-activity starts
    // will *always* be in the multi-activity work area to meet requirements.
    // Therefore, items on the ground aren't counted here for met requirements.
    const inventory &inv = you.crafting_inventory( tripoint_bub_ms::zero, -1 );

    for( std::vector<tool_comp> &tools : tool_reqs_vector ) {
        bool found = false;
        for( tool_comp &tool : tools ) {
            if( inv.has_tools( tool.type, tool.count ) ) {
                found = true;
            }
        }
        if( !found ) {
            reduced_tool_reqs_vector.push_back( tools );
        }
    }

    for( std::vector<quality_requirement> &qualities : quality_reqs_vector ) {
        bool found = false;
        for( quality_requirement &qual : qualities ) {
            if( inv.has_quality( qual.type, qual.level ) ) {
                found = true;
            }
        }
        if( !found ) {
            reduced_quality_reqs_vector.push_back( qualities );
        }
    }

    requirement_data reduced_reqs = requirement_data( reduced_tool_reqs_vector,
                                    reduced_quality_reqs_vector,
                                    component_reqs_vector );
    return hash_and_cache_requirement_data( reduced_reqs );
}

} //namespace multi_activity_actor

// returns nullopt if src_loc should be skipped
std::optional<requirement_id> multi_build_construction_activity_actor::multi_activity_requirements(
    Character &,
    activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data * )
{
    if( act_info.reason == do_activity_reason::NO_COMPONENTS ) {
        if( !act_info.con_idx ) {
            debugmsg( "no construction selected" );
            return std::nullopt;
        }
        // its a construction and we need the components.
        const construction &built_chosen = act_info.con_idx->obj();
        return built_chosen.requirements;
    }
    return requirement_id::NULL_ID();
}

namespace multi_activity_actor
{

std::optional<requirement_id> vehicle_work_requirements( Character &you,
        activity_reason_info &act_info, const tripoint_bub_ms &src_loc )
{

    const do_activity_reason &reason = act_info.reason;
    if( reason == do_activity_reason::NEEDS_VEH_DECONST ||
        reason == do_activity_reason::NEEDS_VEH_REPAIR ) {

        map &here = get_map();
        const vehicle *veh = veh_pointer_or_null( here.veh_at( src_loc ) );
        // we already checked this in can_do_activity() but check again just incase.
        if( !veh ) {
            you.activity_vehicle_part_index = 1;
            return std::nullopt;
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
        return multi_activity_actor::hash_and_cache_requirement_data( reqs );
    }
    return requirement_id::NULL_ID();
}

} //namespace multi_activity_actor

std::optional<requirement_id> multi_mine_activity_actor::multi_activity_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data * )
{
    if( act_info.reason == do_activity_reason::NEEDS_MINING ) {
        return requirement_data_mining_standard;
    }
    return requirement_id::NULL_ID();
}

std::optional<requirement_id> multi_farm_activity_actor::multi_activity_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data *zone )
{

    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_TILLING ) {
        return requirement_data_multi_farm_tilling;
    } else if( reason == do_activity_reason::NEEDS_FERTILIZING ) {
        // no requirements
    } else if( reason == do_activity_reason::NEEDS_PLANTING ) {
        // we can't hardcode individual seed types in JSON, so make a custom requirement
        requirement_data::alter_item_comp_vector requirement_comp_vector = { {
                item_comp( itype_id( dynamic_cast<const plot_options &>
                                     ( zone->get_options() ).get_seed() ), 1 )
            }
        };
        requirement_data::alter_quali_req_vector quality_comp_vector; //no qualities required
        requirement_data::alter_tool_comp_vector tool_comp_vector; //no tools required
        return multi_activity_actor::synthesize_requirements( requirement_comp_vector, quality_comp_vector,
                tool_comp_vector );
    } else if( reason == do_activity_reason::NEEDS_CUT_HARVESTING ) {
        return requirement_data_multi_farm_cut_harvesting;
    }
    return requirement_id::NULL_ID();
}

std::optional<requirement_id> multi_chop_planks_activity_actor::multi_activity_requirements(
    Character &,
    activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data * )
{
    if( act_info.reason == do_activity_reason::NEEDS_CHOPPING ) {
        return requirement_data_multi_chopping_planks;
    }
    return requirement_id::NULL_ID();
}

std::optional<requirement_id> multi_chop_trees_activity_actor::multi_activity_requirements(
    Character &,
    activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data * )
{
    if( act_info.reason == do_activity_reason::NEEDS_TREE_CHOPPING ) {
        return requirement_data_multi_chopping_trees;
    }
    return requirement_id::NULL_ID();
}

std::optional<requirement_id> multi_butchery_activity_actor::multi_activity_requirements(
    Character &,
    activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data * )
{
    if( act_info.reason == do_activity_reason::NEEDS_BUTCHERING ) {
        return requirement_data_multi_butcher;
    } else if( act_info.reason == do_activity_reason::NEEDS_BIG_BUTCHERING ) {
        return requirement_data_multi_butcher_big;
    }
    return requirement_id::NULL_ID();
}

std::optional<requirement_id> multi_fish_activity_actor::multi_activity_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data * )
{
    if( act_info.reason == do_activity_reason::NEEDS_FISHING ) {
        return requirement_data_multi_fishing;
    }
    return requirement_id::NULL_ID();
}

std::optional<requirement_id> multi_craft_activity_actor::multi_activity_requirements( Character &,
        activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data * )
{
    if( act_info.reason == do_activity_reason::NEEDS_CRAFT ) {
        return multi_activity_actor::synthesize_requirements( act_info.req.get_components(),
                act_info.req.get_qualities(), act_info.req.get_tools() );
    }
    return requirement_id::NULL_ID();
}

std::optional<requirement_id> multi_disassemble_activity_actor::multi_activity_requirements(
    Character &,
    activity_reason_info &act_info, const tripoint_bub_ms &, const zone_data * )
{
    if( act_info.reason == do_activity_reason::NEEDS_DISASSEMBLE ) {
        requirement_data::alter_item_comp_vector requirement_comp_vector; //no items required
        return multi_activity_actor::synthesize_requirements( requirement_comp_vector,
                act_info.req.get_qualities(), act_info.req.get_tools() );
    }
    return requirement_id::NULL_ID();
}

namespace multi_activity_actor
{

void add_basecamp_storage_to_loot_zone_list(
    zone_manager &mgr, const tripoint_bub_ms &src_loc, Character &you,
    std::vector<tripoint_bub_ms> &loot_zone_spots, std::vector<tripoint_bub_ms> &combined_spots )
{
    if( npc *const guy = dynamic_cast<npc *>( &you ) ) {
        map &here = get_map();
        if( guy->assigned_camp &&
            mgr.has_near( zone_type_CAMP_STORAGE, here.get_abs( src_loc ), MAX_VIEW_DISTANCE,
                          _fac_id( you ) ) ) {
            std::unordered_set<tripoint_abs_ms> bc_storage_set =
                mgr.get_near( zone_type_CAMP_STORAGE, here.get_abs( src_loc ),
                              MAX_VIEW_DISTANCE, nullptr, _fac_id( you ) );
            for( const tripoint_abs_ms &elem : bc_storage_set ) {
                tripoint_bub_ms here_local = here.get_bub( elem );

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
} //namespace multi_activity_actor

std::vector<std::tuple<tripoint_bub_ms, itype_id, int>>
        fetch_required_activity_actor::requirements_map( Character &you,
                const int distance )
{
    std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> requirement_map;
    if( !fetch_activity_valid( you ) ) {
        return requirement_map;
    }
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    requirement_id things_to_fetch_id = fetch_requirements;
    const requirement_data things_to_fetch = *fetch_requirements;
    const activity_id activity_to_restore = you.backlog.front().id();
    std::vector<std::vector<item_comp>> req_comps = things_to_fetch.get_components();
    std::vector<std::vector<tool_comp>> tool_comps = things_to_fetch.get_tools();
    std::vector<std::vector<quality_requirement>> quality_comps = things_to_fetch.get_qualities();
    zone_manager &mgr = zone_manager::get_manager();
    const bool pickup_task = activity_to_restore->fetch_items_to_zone();
    // where it is, what it is, how much of it, and how much in total is required of that item.
    std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> final_map;
    std::vector<tripoint_bub_ms> loot_spots;
    std::vector<tripoint_bub_ms> already_there_spots;
    std::vector<tripoint_bub_ms> combined_spots;
    std::map<itype_id, int> total_map;
    map &here = get_map();
    const tripoint_bub_ms &src_loc = here.get_bub( fetch_for_activity_position );
    for( const tripoint_bub_ms &elem : here.points_in_radius( src_loc,
            PICKUP_RANGE - 1 ) ) {
        already_there_spots.push_back( elem );
        combined_spots.push_back( elem );
    }
    for( const tripoint_bub_ms &elem : mgr.get_point_set_loot(
             you.pos_abs(), distance, you.is_npc(), _fac_id( you ) ) ) {
        // if there is a loot zone that's already near the work spot, we don't want it to be added twice.
        if( std::find( already_there_spots.begin(), already_there_spots.end(),
                       elem ) != already_there_spots.end() ) {
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
    multi_activity_actor::add_basecamp_storage_to_loot_zone_list( mgr, src_loc, you, loot_spots,
            combined_spots );
    // if the requirements aren't available, then stop.
    if( !multi_activity_actor::are_requirements_nearby( pickup_task ? loot_spots : combined_spots,
            things_to_fetch_id, you,
            activity_to_restore, pickup_task, src_loc ) ) {
        return requirement_map;
    }
    // if the requirements are already near the work spot and its a construction/crafting task, then no need to fetch anything more.
    if( !pickup_task &&
        multi_activity_actor::are_requirements_nearby( already_there_spots, things_to_fetch_id, you,
                activity_to_restore,
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
                ++it;
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
                    --it;
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
                ++it;
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
                    --it;
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
                ++it;
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
        for( const item_comp &comp : it ) {
            comp_selection<item_comp> sel;
            sel.use_from = usage_from::both;
            sel.comp = comp;
            std::list<item> consumed = you.consume_items( sel, 1, is_crafting_component );
            used.splice( used.end(), consumed );
        }
    }
    pc.components = used;
    here.partial_con_set( src_loc, pc );
    for( const std::vector<tool_comp> &it : built_chosen.requirements->get_tools() ) {
        you.consume_tools( it );
    }
    you.backlog.emplace_front( activity_to_restore );
    you.assign_activity( build_construction_activity_actor( here.get_abs( src_loc ) ) );
    return true;
}

bool fetch_required_activity_actor::fetch_activity(
    Character &you, const tripoint_bub_ms &src_loc, const activity_id &activity_to_restore,
    int distance )
{
    if( !fetch_activity_valid( you ) ) {
        return false;
    }
    const activity_id fetch_for_activity = you.backlog.front().id();
    map &here = get_map();
    if( !here.can_put_items_ter_furn( here.get_bub( fetched_item_destination ) ) ) {
        return false;
    }
    const std::vector<std::tuple<tripoint_bub_ms, itype_id, int>> mental_item_map =
                requirements_map( you, distance );
    int pickup_count = 1;
    map_stack items_there = here.i_at( src_loc );
    const units::volume volume_allowed = you.free_space();
    const units::mass weight_allowed = you.weight_capacity() - you.weight_carried();

    // if the items are in vehicle cargo, move them to their position
    const std::optional<vpart_reference> ovp = here.veh_at( src_loc ).cargo();
    if( ovp ) {
        for( item &veh_elem : ovp->items() ) {
            for( auto elem : mental_item_map ) {
                if( std::get<0>( elem ) == src_loc && veh_elem.typeId() == std::get<1>( elem ) ) {
                    if( fetch_for_activity == ACT_MULTIPLE_CONSTRUCTION ) {
                        //TODO: this teleports the item, assign an item moving activity
                        move_item( you, veh_elem, veh_elem.count_by_charges() ? std::get<2>( elem ) : 1, src_loc,
                                   here.get_bub( fetched_item_destination ), ovp,
                                   activity_to_restore );
                        return true;
                    }
                }
            }
        }
    }
    for( auto item_iter = items_there.begin(); item_iter != items_there.end(); item_iter++ ) {
        item &it = *item_iter;
        for( auto elem : mental_item_map ) {
            if( std::get<0>( elem ) == src_loc && it.typeId() == std::get<1>( elem ) ) {
                // some tasks (construction/crafting) want the required item moved near the work spot.
                if( !fetch_for_activity->fetch_items_to_zone() ) {
                    //TODO: this teleports the item, assign an item moving activity
                    move_item( you, it, it.count_by_charges() ? std::get<2>( elem ) : 1, src_loc,
                               here.get_bub( fetched_item_destination ), ovp, activity_to_restore );

                    return true;
                    // other tasks want the item picked up
                } else {
                    // TODO: most of this `else` block should be handled by exactly one function call

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
    // if we got here, then the fetch failed for reasons that weren't predicted before assigning it.
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
    std::vector<butchery_data> bd;
    for( item &elem : items ) {
        if( elem.is_corpse() &&
            ( !elem.has_var( "activity_var" ) || elem.get_var( "activity_var" ) == you.name ) ) {
            const mtype corpse = *elem.get_mtype();
            if( corpse.size > creature_size::medium && reason != do_activity_reason::NEEDS_BIG_BUTCHERING ) {
                continue;
            }
            elem.set_var( "activity_var", you.name );
            item_location corpse_loc = item_location( map_cursor( src_loc ), &elem );
            bd.emplace_back( corpse_loc, butcher_type::FULL );
        }
    }
    if( !bd.empty() ) {
        you.assign_activity( butchery_activity_actor( bd ) );
        return true;
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
            you.activity.placement = here.get_abs( src_loc );
            return true;
        }
    }
    return false;
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
    map &here = get_map();
    // We pre-exclude jackhammers for wall mining here, just in case the character is carrying both a jackhammer and a pickaxe.
    // If we don't then our iteration will preferentially select the (powered) jackhammer and always fail in subsequent calls to dig_tool(). Very troublesome!
    const bool is_wall_mining = here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_WALL, src_loc );
    std::vector<item *> mining_inv = you.items_with( [&you, is_wall_mining]( const item & itm ) {
        return ( itm.has_flag( flag_DIG_TOOL ) && !itm.type->can_use( "JACKHAMMER" ) ) ||
               ( !is_wall_mining && ( itm.type->can_use( "JACKHAMMER" ) && itm.ammo_sufficient( &you ) ) );
    } );
    // All other failure conditions are handled in subsequent calls to dig_tool() with specific messaging for the player. This is just an early short circuit.
    if( mining_inv.empty() ) {
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
    std::optional<int> did_we_mine = std::nullopt;
    if( powered ) {
        did_we_mine = iuse::jackhammer( &you, chosen_item, src_loc );
    } else {
        did_we_mine = iuse::pickaxe( &you, chosen_item, src_loc );
    }
    // Any value at all means that we succeeded and started the activity. Actual contained value will be 0 on success, for iuse's normal returns.
    return did_we_mine.has_value();
}

// Not really an activity like the others; relies on zone activity alerting on enemies
static bool mop_activity( Character &you, const tripoint_bub_ms &src_loc )
{
    // iuse::mop costs 15 moves per use
    you.assign_activity( mop_activity_actor( 15 ) );
    you.activity.placement = get_map().get_abs( src_loc );
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
    const ter_id &ter = here.ter( src_loc );
    if( here.has_flag( ter_furn_flag::TFLAG_TREE, src_loc ) ) {
        you.assign_activity( chop_tree_activity_actor( moves, item_location( you, &best_qual ) ) );
        you.activity.placement = here.get_abs( src_loc );
        return true;
    } else if( ter == ter_t_trunk || ter == ter_t_stump ) {
        you.assign_activity( chop_logs_activity_actor( moves, item_location( you, &best_qual ) ) );
        you.activity.placement = here.get_abs( src_loc );
        return true;
    }
    return false;
}

namespace multi_activity_actor
{

zone_type_id get_zone_for_act( const tripoint_bub_ms &src_loc, const zone_manager &mgr,
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
        ret = zone_type_DISASSEMBLE;
    }
    if( act_id == ACT_MULTIPLE_STUDY ) {
        ret = zone_type_STUDY_ZONE;
    }
    if( src_loc != tripoint_bub_ms() && act_id == ACT_FETCH_REQUIRED ) {
        const zone_data *zd = mgr.get_zone_at( get_map().get_abs( src_loc ), false, fac_id );
        if( zd ) {
            ret = zd->get_type();
        }
    }
    return ret;
}

void prune_same_tile_locations( Character &you, std::unordered_set<tripoint_abs_ms> &src_set )
{

    for( auto src_set_iter = src_set.begin(); src_set_iter != src_set.end(); ) {
        bool skip = false;

        for( const npc &guy : g->all_npcs() ) {
            if( &guy == &you ) {
                continue;
            }
            if( guy.has_player_activity() && guy.activity.placement == *src_set_iter ) {
                src_set_iter = src_set.erase( src_set_iter );
                skip = true;
                break;
            }
        }
        if( skip ) {
            continue;
        } else {
            ++src_set_iter;
        }
    }
}

void prune_dark_locations( Character &you, std::unordered_set<tripoint_abs_ms> &src_set,
                           const activity_id &act_id )
{
    map &here = get_map();
    bool src_set_empty = src_set.empty();

    for( auto src_set_iter = src_set.begin(); src_set_iter != src_set.end(); ) {
        const tripoint_bub_ms set_pt = here.get_bub( *src_set_iter );
        if( you.fine_detail_vision_mod( set_pt ) > LIGHT_AMBIENT_DIM ) {
            src_set_iter = src_set.erase( src_set_iter );
            continue;
        }
        ++src_set_iter;
    }

    // if all pruned locations were too dark
    if( !src_set_empty && src_set.empty() ) {
        you.add_msg_if_player( m_info, _( "It is too dark to do the %s activity." ), act_id.c_str() );
    }
}

void prune_dangerous_field_locations( std::unordered_set<tripoint_abs_ms> &src_set )
{

    map &here = get_map();

    for( auto src_set_iter = src_set.begin(); src_set_iter != src_set.end(); ) {
        // remove dangerous tiles
        const tripoint_bub_ms set_pt = here.get_bub( *src_set_iter );
        if( here.dangerous_field_at( set_pt ) ) {
            src_set_iter = src_set.erase( src_set_iter );
            continue;
        }
        ++src_set_iter;
    }
}

std::unordered_set<tripoint_abs_ms> generic_locations( Character &you, const activity_id &act_id )
{
    zone_manager &mgr = zone_manager::get_manager();
    std::unordered_set<tripoint_abs_ms> src_set;

    zone_type_id zone_type = multi_activity_actor::get_zone_for_act( tripoint_bub_ms::zero, mgr, act_id,
                             _fac_id( you ) );
    src_set = mgr.get_near( zone_type, you.pos_abs(), MAX_VIEW_DISTANCE, nullptr, _fac_id( you ) );

    // prune the set to remove tiles that are never gonna work out.
    multi_activity_actor::prune_dangerous_field_locations( src_set );
    if( !multi_activity_actor::can_do_in_dark( act_id ) ) {
        multi_activity_actor::prune_dark_locations( you, src_set, act_id );
    }
    return src_set;
}

std::unordered_set<tripoint_abs_ms> no_same_tile_locations( Character &you,
        const activity_id &act_id )
{
    std::unordered_set<tripoint_abs_ms> src_set = multi_activity_actor::generic_locations( you,
            act_id );
    multi_activity_actor::prune_same_tile_locations( you, src_set );

    return src_set;
}

} // namespace multi_activity_actor

std::unordered_set<tripoint_abs_ms>
multi_build_construction_activity_actor::multi_activity_locations(
    Character &you )
{
    const activity_id act_id = get_type();
    map &here = get_map();

    // multiple construction will form a list of targets based on blueprint zones and unfinished constructions
    std::unordered_set<tripoint_abs_ms> src_set = multi_activity_actor::generic_locations( you,
            act_id );
    for( const tripoint_bub_ms &elem : here.points_in_radius( you.pos_bub(), MAX_VIEW_DISTANCE ) ) {
        partial_con *pc = here.partial_con_at( elem );
        if( pc ) {
            src_set.insert( here.get_abs( elem ) );
        }
    }
    multi_activity_actor::prune_dangerous_field_locations( src_set );
    multi_activity_actor::prune_same_tile_locations( you, src_set );
    multi_activity_actor::prune_dark_locations( you, src_set, act_id );

    return src_set;
}

std::unordered_set<tripoint_abs_ms> multi_read_activity_actor::multi_activity_locations(
    Character &you )
{
    const activity_id act_id = get_type();
    map &here = get_map();
    const tripoint_bub_ms localpos = you.pos_bub();
    std::unordered_set<tripoint_abs_ms> src_set;

    // anywhere well lit
    for( const tripoint_bub_ms &elem : here.points_in_radius( localpos, MAX_VIEW_DISTANCE ) ) {
        src_set.insert( here.get_abs( elem ) );
    }
    multi_activity_actor::prune_dangerous_field_locations( src_set );
    multi_activity_actor::prune_dark_locations( you, src_set, act_id );
    return src_set;
}

std::unordered_set<tripoint_abs_ms> multi_study_activity_actor::multi_activity_locations(
    Character &you )
{
    const activity_id act_id = get_type();
    map &here = get_map();
    zone_manager &mgr = zone_manager::get_manager();
    const tripoint_abs_ms abspos = here.get_abs( you.pos_bub() );
    std::unordered_set<tripoint_abs_ms> src_set;

    std::unordered_set<tripoint_abs_ms> all_zone_tiles = mgr.get_near( zone_type_STUDY_ZONE, abspos,
            MAX_VIEW_DISTANCE, nullptr, _fac_id( you ) );
    for( const tripoint_abs_ms &zone_pos : all_zone_tiles ) {
        if( find_study_book( zone_pos, you ) ) {
            src_set.insert( zone_pos );
        }
    }
    multi_activity_actor::prune_dangerous_field_locations( src_set );
    multi_activity_actor::prune_dark_locations( you, src_set, act_id );
    return src_set;
}

std::unordered_set<tripoint_abs_ms> multi_craft_activity_actor::multi_activity_locations(
    Character &you )
{
    const activity_id act_id = get_type();
    map &here = get_map();
    const tripoint_bub_ms localpos = you.pos_bub();
    std::unordered_set<tripoint_abs_ms> src_set;

    // Craft only with what is on the spot
    // TODO: add zone type like zone_type_CRAFT?
    src_set.insert( here.get_abs( localpos ) );
    multi_activity_actor::prune_dangerous_field_locations( src_set );
    multi_activity_actor::prune_dark_locations( you, src_set, act_id );

    return src_set;
}

std::unordered_set<tripoint_abs_ms> multi_fish_activity_actor::multi_activity_locations(
    Character &you )
{
    const activity_id act_id = get_type();
    map &here = get_map();
    std::unordered_set<tripoint_abs_ms> src_set = multi_activity_actor::generic_locations( you,
            act_id );

    for( auto src_set_iter = src_set.begin(); src_set_iter != src_set.end(); ) {
        const tripoint_bub_ms set_pt = here.get_bub( *src_set_iter );
        const ter_id &terrain_id = here.ter( set_pt );
        if( !terrain_id.obj().has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) ) {
            src_set_iter = src_set.erase( src_set_iter );
        } else {
            ++src_set_iter;
        }
    }
    return src_set;
}

std::unordered_set<tripoint_abs_ms> multi_mop_activity_actor::multi_activity_locations(
    Character &you )
{
    const activity_id act_id = get_type();
    map &here = get_map();
    std::unordered_set<tripoint_abs_ms> src_set = multi_activity_actor::generic_locations( you,
            act_id );
    for( auto src_set_iter = src_set.begin(); src_set_iter != src_set.end(); ) {
        const tripoint_bub_ms set_pt = here.get_bub( *src_set_iter );
        if( !here.mopsafe_field_at( set_pt ) ) {
            src_set_iter = src_set.erase( src_set_iter );
            continue;
        }
        ++src_set_iter;
    }
    return src_set;
}

bool multi_farm_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &src, const tripoint_bub_ms &src_loc )
{

    const map &here = get_map();
    const zone_manager &mgr = zone_manager::get_manager();
    const do_activity_reason &reason = act_info.reason;

    // it was here earlier, in the space of one turn, maybe it got harvested by someone else.
    if( ( ( reason == do_activity_reason::NEEDS_HARVESTING ) ||
          ( reason == do_activity_reason::NEEDS_CUT_HARVESTING ) ) &&
        here.has_flag_furn( ter_furn_flag::TFLAG_GROWTH_HARVEST, src_loc ) ) {
        iexamine::harvest_plant( you, src_loc, true );
    } else if( ( reason == do_activity_reason::NEEDS_CLEARING ) &&
               here.has_flag_furn( ter_furn_flag::TFLAG_GROWTH_OVERGROWN, src_loc ) ) {
        iexamine::clear_overgrown( you, src_loc );
    } else if( reason == do_activity_reason::NEEDS_TILLING &&
               here.has_flag( ter_furn_flag::TFLAG_PLOWABLE, src_loc ) &&
               you.has_quality( qual_DIG, 1 ) && !here.has_furn( src_loc ) ) {
        you.assign_activity( churn_activity_actor( 18000, item_location() ) );
        you.activity.placement = src;
        return false;
    } else if( reason == do_activity_reason::NEEDS_PLANTING ) {
        std::vector<zone_data> zones = mgr.get_zones( zone_type_FARM_PLOT, src, _fac_id( you ) );
        for( const zone_data &zone : zones ) {
            const itype_id seed =
                dynamic_cast<const plot_options &>( zone.get_options() ).get_seed();
            std::vector<item_location> seed_inv = you.cache_get_items_with( "is_seed", itype_id( seed ), {},
                                                  &item::is_seed );
            if( seed_inv.empty() ) {
                // we don't have the required seed, even though we should at this point.
                // move onto the next tile, and if need be that will prompt a fetch seeds activity.
                continue;
            }
            if( !here.has_flag_ter_or_furn( seed->seed->required_terrain_flag, src_loc ) ) {
                continue;
            }
            iexamine::plant_seed( you, src_loc, itype_id( seed ) );
            return false;
        }
    } else if( reason == do_activity_reason::NEEDS_FERTILIZING ) {
        itype_id used_fertilizer = get_first_fertilizer_itype( you, src );
        if( !used_fertilizer.is_null() ) {
            iexamine::fertilize_plant( you, src_loc, used_fertilizer );
        }
        return false;
    }
    return true;
}

bool multi_chop_planks_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{

    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_CHOPPING && you.has_quality( qual_AXE, 1 ) ) {
        if( chop_plank_activity( you, src_loc ) ) {
            return false;
        }
    }
    return true;
}

bool multi_butchery_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{

    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_BUTCHERING ||
        reason == do_activity_reason::NEEDS_BIG_BUTCHERING ) {
        if( butcher_corpse_activity( you, src_loc, reason ) ) {
            return false;
        }
    }
    return true;
}

bool multi_read_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms & )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_BOOK_TO_LEARN ) {
        const item_filter filter = [&you]( const item & i ) {
            read_condition_result condition = you.check_read_condition( i );
            return condition == read_condition_result::SUCCESS;
        };
        std::vector<item *> books = you.items_with( filter );
        if( !books.empty() && books[0] ) {
            const time_duration time_taken = you.time_to_read( *books[0], you );
            item_location book = item_location( you, books[0] );
            item_location ereader;
            you.assign_activity( read_activity_actor( time_taken, book, ereader, true ) );
            return false;
        }
    }
    return true;
}

bool multi_study_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &src, const tripoint_bub_ms & )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_BOOK_TO_LEARN ) {
        item_location book_loc = find_study_book( src, you );
        if( book_loc ) {
            book_loc->set_var( "activity_var", you.name );
            you.may_activity_occupancy_after_end_items_loc.push_back( book_loc );
            const time_duration time_taken = you.time_to_read( *book_loc, you );
            item_location ereader;
            you.assign_activity( read_activity_actor( time_taken, book_loc, ereader, true ) );
            return false;
        }
        if( !book_loc && you.is_npc() ) {
            add_msg_if_player_sees( you, m_info, _( "%s found no readable books at this location." ),
                                    you.disp_name() );
        }
    }
    return true;
}

bool multi_build_construction_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &src, const tripoint_bub_ms &src_loc )
{
    map &here = get_map();
    const do_activity_reason &reason = act_info.reason;
    const zone_manager &mgr = zone_manager::get_manager();
    const zone_data *zone =
        mgr.get_zone_at( src, multi_activity_actor::get_zone_for_act( src_loc, mgr,
                         ACT_MULTIPLE_CONSTRUCTION, _fac_id( you ) ),
                         _fac_id( you ) );

    if( reason == do_activity_reason::CAN_DO_CONSTRUCTION ) {
        if( here.partial_con_at( src_loc ) ) {
            you.assign_activity( build_construction_activity_actor( src ) );
            return false;
        }
        if( construction_activity( you, zone, src_loc, act_info, ACT_MULTIPLE_CONSTRUCTION ) ) {
            return false;
        }
    }
    return true;
}

bool multi_chop_trees_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_TREE_CHOPPING && you.has_quality( qual_AXE, 1 ) ) {
        if( chop_tree_activity( you, src_loc ) ) {
            return false;
        }
    }
    return true;
}

bool multi_fish_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_FISHING && you.has_quality( qual_FISHING_ROD, 1 ) ) {
        // we don't want to keep repeating the fishing activity, just piggybacking on this functions structure to find requirements.
        you.activity = player_activity();
        item_location best_rod_loc( you, &you.best_item_with_quality( qual_FISHING_ROD ) );
        you.assign_activity( fish_activity_actor( best_rod_loc,
                             g->get_fishable_locations_abs( MAX_VIEW_DISTANCE, src_loc ), 5_hours ) );
        return false;
    }
    return true;
}

bool multi_mine_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_MINING ) {
        // if have enough batteries to continue etc.
        if( mine_activity( you, src_loc ) ) {
            return false;
        }
    }
    return true;
}

bool multi_mop_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_MOP ) {
        if( mop_activity( you, src_loc ) ) {
            return false;
        }
    }
    return true;
}

bool multi_vehicle_deconstruct_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_VEH_DECONST ) {
        if( vehicle_activity( you, src_loc, you.activity_vehicle_part_index, VEHICLE_REMOVE ) ) {
            return false;
        }
        you.activity_vehicle_part_index = -1;
    }
    return true;
}

bool multi_vehicle_repair_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_VEH_REPAIR ) {
        if( vehicle_activity( you, src_loc, you.activity_vehicle_part_index, VEHICLE_REPAIR ) ) {
            return false;
        }

        you.activity_vehicle_part_index = -1;
    }
    return true;
}

bool multi_craft_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms & )
{
    const do_activity_reason &reason = act_info.reason;

    if( reason == do_activity_reason::NEEDS_CRAFT ) {
        // only npc is supported
        npc *p = you.as_npc();
        if( p ) {
            item_location to_craft = p->get_item_to_craft();
            if( to_craft && to_craft->is_craft() &&
                you.lighting_craft_speed_multiplier( to_craft->get_making() ) > 0 ) {
                player_activity act = player_activity( craft_activity_actor( to_craft, false ) );
                you.assign_activity( act );
                return false;
            }
        }
    }
    return true;
}

bool multi_disassemble_activity_actor::multi_activity_do( Character &you,
        const activity_reason_info &act_info,
        const tripoint_abs_ms &, const tripoint_bub_ms &src_loc )
{
    const do_activity_reason &reason = act_info.reason;
    map &here = get_map();

    if( reason == do_activity_reason::NEEDS_DISASSEMBLE ) {
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
                    act.targets.emplace_back( map_cursor( src_loc ), &elem );
                    act.placement = here.get_abs( src_loc );
                    act.position = qty;
                    act.index = false;
                    you.assign_activity( act );
                    break;
                }
            }
        }
        return false;
    }
    return true;
}

namespace multi_activity_actor
{

void activity_failure_message( Character &you, activity_id new_activity,
                               const requirement_failure_reasons &fail_reason, bool no_locations )
{

    if( no_locations ) {
        add_msg( m_neutral,
                 _( "%1$s failed to perform the %2$s activity because no suitable locations were found." ),
                 you.disp_name(), new_activity.c_str() );
    } else if( fail_reason.no_path ) {
        add_msg( m_neutral,
                 _( "%1$s failed to perform the %2$s activity because no path to a suitable location could be found." ),
                 you.disp_name(), new_activity.c_str() );
    } else if( fail_reason.skip_location_no_zone ) {
        add_msg( m_neutral,
                 _( "%1$s failed to perform the %2$s activity because no required zone was found." ),
                 you.disp_name(), new_activity.c_str() );
    } else if( fail_reason.skip_location_blocking ) {
        add_msg( m_neutral,
                 _( "%1$s failed to perform the %2$s activity because the target location is blocked or cannot be reached." ),
                 you.disp_name(), new_activity.c_str() );
    } else if( fail_reason.skip_location_no_skill ) {
        add_msg( m_neutral, _( "%1$s failed to perform the %2$s activity because of insufficient skills." ),
                 you.disp_name(), new_activity.c_str() );
    } else if( fail_reason.skip_location_unknown_activity ) {
        add_msg( m_neutral,
                 _( "%1$s failed to perform the %2$s activity because the activity couldn't be found.  This is probably an error." ),
                 you.disp_name(), new_activity.c_str() );
    } else if( fail_reason.skip_location_no_location ) {
        add_msg( m_neutral,
                 _( "%1$s failed to perform the %2$s activity because no suitable location could be found." ),
                 you.disp_name(), new_activity.c_str() );
    } else if( fail_reason.skip_location_no_match ) {
        add_msg( m_neutral,
                 _( "%1$s failed to perform the %2$s activity because no criteria could be matched." ),
                 you.disp_name(), new_activity.c_str() );
    } else if( fail_reason.skip_location ) {
        // Assumed to have been reported already.
    } else if( fail_reason.no_craft_disassembly_location_route_found ) {
        add_msg( m_neutral,
                 _( "%1$s failed to perform the %2$s activity because no path to a suitable crafting/disassembly location could be found." ),
                 you.disp_name(), new_activity.c_str() );
    }
}

//returns nullopt for no route to src_bub, true for restarted activity, false otherwise
std::optional<bool> route( Character &you, player_activity &act, const tripoint_bub_ms &src_bub,
                           requirement_failure_reasons &fail_reason, bool check_only )
{
    activity_id act_id = act.id();
    //route to destination if needed
    std::vector<tripoint_bub_ms> route_workbench;
    if( act_id == ACT_MULTIPLE_CRAFT || act_id == ACT_MULTIPLE_DIS ) {
        // returns empty vector if we can't reach best tile
        // or we are already on the best tile
        route_workbench = route_best_workbench( you, src_bub );
    }
    // If we are doing disassemble we need to stand on the "best" tile
    if( square_dist( you.pos_bub(), src_bub ) > 1 || !route_workbench.empty() ) {
        std::vector<tripoint_bub_ms> route;
        // find best workbench if possible
        if( !route_workbench.empty() ) {
            route = route_workbench;
        } else {
            route = route_adjacent( you, src_bub );
        }
        // check if we found path to source / adjacent tile
        if( route.empty() ) {
            check_npc_revert( you );
            fail_reason.no_craft_disassembly_location_route_found = true;
            return std::nullopt;
        }
        if( !check_only ) {
            if( you.get_moves() <= 0 ) {
                // Restart activity and break from cycle.
                you.assign_activity( act );
                return true;
            }
            // set the destination and restart activity after player arrives there
            // we don't need to check for safe mode,
            // activity will be restarted only if
            // player arrives on destination tile
            you.set_destination( route, act );
            return true;
        }
    }
    return false;
}

bool out_of_moves( Character &you )
{
    if( you.get_moves() <= 0 ) {
        // Restart activity and break from cycle.
        you.activity_vehicle_part_index = -1;
        return true;
    }
    return false;
}

void revert_npc_post_activity( Character &you, activity_id, bool no_locations )
{
    // if we got here, we need to revert otherwise NPC will be stuck in AI Limbo and have a head explosion.
    if( you.is_npc() && ( you.backlog.empty() || no_locations ) ) {
        /**
        * This should really be a debug message, but too many places rely on this broken behavior.
        * debugmsg( "Reverting %s activity for %s, probable infinite loop", activity_to_restore.c_str(),
        *           you.get_name() );
        */
        check_npc_revert( you );
    }
    you.activity_vehicle_part_index = -1;
}

} //namespace multi_activity_actor

void requirement_failure_reasons::convert_requirement_check_result( requirement_check_result
        req_res )
{
    if( req_res == requirement_check_result::SKIP_LOCATION ) {
        skip_location = true;
    } else if( req_res == requirement_check_result::SKIP_LOCATION_NO_ZONE ) {
        skip_location_no_zone = true;
    } else if( req_res == requirement_check_result::SKIP_LOCATION_NO_SKILL ) {
        skip_location_no_skill = true;
    } else if( req_res == requirement_check_result::SKIP_LOCATION_BLOCKING ) {
        skip_location_blocking = true;
    } else if( req_res == requirement_check_result::SKIP_LOCATION_UNKNOWN_ACTIVITY ) {
        skip_location_unknown_activity = true;
    } else if( req_res == requirement_check_result::SKIP_LOCATION_NO_LOCATION ) {
        skip_location_no_location = true;
    } else if( req_res == requirement_check_result::SKIP_LOCATION_NO_MATCH ) {
        skip_location_no_match = true;
    }
}

bool requirement_failure_reasons::check_skip_location() const
{
    return
        skip_location ||
        skip_location_no_zone ||
        skip_location_no_skill ||
        skip_location_blocking ||
        skip_location_unknown_activity ||
        skip_location_no_location ||
        skip_location_no_match;
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
    const tripoint_abs_ms center_abs = here.get_abs( center );

    const std::unordered_set<tripoint_abs_ms> tiles_abs_unordered =
        mgr.get_near( zone_type_SOURCE_FIREWOOD, center_abs, PICKUP_RANGE, nullptr, fac );
    const std::vector<tripoint_abs_ms> &tiles_abs =
        get_sorted_tiles_by_distance( center_abs, tiles_abs_unordered );

    for( const tripoint_abs_ms &tile_abs : tiles_abs ) {
        const tripoint_bub_ms tile = here.get_bub( tile_abs );
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
            for( item_pocket *pocket : loc->get_container_pockets() ) {
                for( item &i : pocket->edit_contents() ) {
                    item_location child_loc( loc, &i );
                    if( visit_item_contents( child_loc, func ) == VisitResponse::ABORT ) {
                        return VisitResponse::ABORT;
                    }
                }
            }
            [[fallthrough]];

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

    const tripoint_bub_ms pos = you.pos_bub();
    zone_manager &mgr = zone_manager::get_manager();
    zone_type_id consume_type_zone( "" );
    if( food ) {
        consume_type_zone = zone_type_AUTO_EAT;
    } else {
        consume_type_zone = zone_type_AUTO_DRINK;
    }
    map &here = get_map();
    const std::unordered_set<tripoint_abs_ms> &dest_set =
        mgr.get_near( consume_type_zone, here.get_abs( pos ), MAX_VIEW_DISTANCE, nullptr,
                      _fac_id( you ) );
    if( dest_set.empty() ) {
        return 0;
    }
    item_location best_comestible;
    for( const tripoint_abs_ms &loc : dest_set ) {
        if( loc.z() != pos.z() ) {
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

        const optional_vpart_position vp = here.veh_at( here.get_bub( loc ) );
        if( vp ) {
            if( const std::optional<vpart_reference> vp_cargo = vp.cargo() ) {
                for( item &it : vp_cargo->items() ) {
                    item_location i_loc( vehicle_cursor( vp->vehicle(), vp->part_index() ), &it );
                    visit_item_contents( i_loc, visit );
                }
            }
        } else {
            for( item &it : here.i_at( here.get_bub( loc ) ) ) {
                item_location i_loc( map_cursor( loc ), &it );
                visit_item_contents( i_loc, visit );
            }
        }
    }

    if( best_comestible ) {
        //The moves it takes you to walk there and back.
        int consume_moves = 2 * you.run_cost( 100, false ) * std::max( rl_dist( you.pos_abs(),
                            best_comestible.pos_abs() ), 1 );
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
bool try_fuel_fire( Character &you, std::optional<tripoint_bub_ms> fire_target )
{
    const tripoint_bub_ms pos = you.pos_bub();
    std::vector<tripoint_bub_ms> adjacent = closest_points_first( pos, 1, PICKUP_RANGE );

    map &here = get_map();
    std::optional<tripoint_bub_ms> best_fire =
        fire_target ? fire_target : find_best_fire( adjacent, pos );

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
    if( !fire_target && ( fd.fuel_produced >= 1.0f || fire_age < 10_minutes ) ) {
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

void activity_handlers::clean_may_activity_occupancy_items_var_if_is_avatar_and_no_activity_now(
    Character &you )
{
    if( you.is_avatar() && ( !you.activity )  && ( !you.get_destination_activity() ) ) {
        clean_may_activity_occupancy_items_var( you );
    }
}
void activity_handlers::clean_may_activity_occupancy_items_var( Character &you )
{
    std::string character_name = you.name;
    const std::function<bool( const item *const )> activity_var_checker = [character_name](
    const item * const it )->bool{
        return it->has_var( "activity_var" )
        && it->get_var( "activity_var", "" ) == character_name;
    };
    for( item_location &loc : you.may_activity_occupancy_after_end_items_loc ) {
        if( loc && activity_var_checker( loc.get_item() ) ) {
            loc.get_item()->erase_var( "activity_var" );
        }
    }
    you.may_activity_occupancy_after_end_items_loc.clear();

}
