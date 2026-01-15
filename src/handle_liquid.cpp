#include "handle_liquid.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "action.h"
#include "activity_actor_definitions.h"
#include "cached_options.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "game_inventory.h"
#include "iexamine.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "messages.h"
#include "monster.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "units.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "vpart_range.h"


#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

void liquid_wrapper::validate() const
{
    if( type == liquid_source_type::MAP_ITEM ) {

        if( standard_item == item_location::nowhere || !standard_item || standard_item->is_null() ) {
            debugmsg( "Passed empty or invalid item_location to liquid_wrapper; must be handled before instantiating liquid_wrapper" );
            return;
        }
    } else {
        const tripoint_bub_ms liquid_pos = get_item_pos_bub( get_map() );

        if( !liquid_source_item ) {
            debugmsg( "Passed invalid item to liquid_wrapper; must be handled before instantiating liquid_wrapper" );
            return;
        }
        if( liquid_pos == tripoint_bub_ms::invalid ) {
            debugmsg( "Passed invalid tripoint to liquid_wrapper; must be handled before instantiating liquid_wrapper" );
            return;
        }
    }
}

std::string liquid_wrapper::get_menu_prompt() const
{
    const std::string prompt_base = source_item_was_new() ?
                                    _( "The %s from %s has no container:" ) :
                                    _( "What to do with the %s from %s?" );
    std::string prompt_text;

    if( type == liquid_source_type::MAP_ITEM ) {

        const item &liquid_item = *standard_item;
        const std::string &liquid_name = liquid_item.display_name();
        const item_location::type liquid_source_type = standard_item.where();
        if( liquid_source_type == item_location::type::container ) {
            const item_location parent_container = standard_item.parent_item();
            if( parent_container->is_bucket_nonempty() ) {
                prompt_text = string_format(
                                  //~ %1$s: liquid name, %2$s: container name
                                  _( "The %s would spill.\nWhat to do with the %s inside it?" ),
                                  parent_container->display_name(), liquid_name );
            } else {
                prompt_text = string_format( pgettext( "liquid", "What to do with the %s in the %s?" ),
                                             liquid_name, parent_container->display_name() );
            }
        } else {
            prompt_text = string_format( _( "What to do with the %s?" ), liquid_name );
        }
    } else {
        map &here = get_map();
        const item &liquid_item = *liquid_source_item;
        const std::string &liquid_name = liquid_item.display_name();
        const tripoint_bub_ms liquid_pos = get_item_pos_bub( here );

        if( type == liquid_source_type::INFINITE_MAP ) {
            prompt_text = string_format( prompt_base,
                                         liquid_name,
                                         here.name( get_item_pos_bub( here ) ) );
        } else if( type == liquid_source_type::VEHICLE ) {
            prompt_text = string_format( prompt_base,
                                         liquid_name,
                                         here.veh_at( get_item_pos_bub( here ) )->vehicle().disp_name() );
        } else if( type == liquid_source_type::MONSTER ) {
            prompt_text = string_format( prompt_base,
                                         liquid_name,
                                         get_creature_tracker().creature_at<monster>( get_item_pos_bub( here ) )->disp_name() );
        } else if( type == liquid_source_type::NEW_ITEM ) {
            prompt_text = string_format( _( "The %s has no container:" ), liquid_name );
        }
    }
    return prompt_text;
}

const item *liquid_wrapper::get_item() const
{
    if( standard_item ) {
        return standard_item.get_item();
    }
    return liquid_source_item.get();
}

item *liquid_wrapper::get_item()
{
    if( standard_item ) {
        return standard_item.get_item();
    }
    return liquid_source_item.get();
}

const item *liquid_wrapper::get_container() const
{
    if( standard_item ) {
        return standard_item.parent_item().get_item();
    }
    return nullptr;
}

tripoint_abs_ms liquid_wrapper::get_item_pos() const
{
    const map &here = get_map();
    if( standard_item ) {
        return here.get_abs( standard_item.pos_bub( here ) );
    } else if( type == liquid_source_type::INFINITE_MAP ) {
        return *as_infinite_terrain;
    } else if( type == liquid_source_type::VEHICLE ) {
        return *as_vehicle_part;
    } else if( type == liquid_source_type::MONSTER ) {
        return *as_monster;
    }
    return tripoint_abs_ms::invalid;
}

tripoint_bub_ms liquid_wrapper::get_item_pos_bub( map &here ) const
{
    return here.get_bub( get_item_pos() );
}

bool liquid_wrapper::can_consume() const
{
    Character &player_character = get_player_character();
    bool pc_can_consume = player_character.can_consume_as_is( *get_item() );
    return pc_can_consume && !as_monster;
}

bool liquid_wrapper::source_item_was_new() const
{
    return source_item_new;
}

void liquid_wrapper::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "type", static_cast<int>( type ) );
    jsout.member( "standard_item", standard_item );
    jsout.member( "as_infinite_terrain", as_infinite_terrain );
    jsout.member( "as_vehicle_part", as_vehicle_part );
    jsout.member( "vp_index", static_cast<uint64_t>( vp_index ) );
    jsout.member( "as_monster", as_monster );

    // these items don't actually exist and must be serialized
    if( type == liquid_source_type::INFINITE_MAP ||
        type == liquid_source_type::MONSTER ) {
        if( liquid_source_item ) {
            jsout.member( "fake_item", ::serialize( *liquid_source_item ) );
        }
    }

    jsout.end_object();
}

void liquid_wrapper::deserialize( const JsonValue &jsin )
{
    JsonObject data = jsin.get_object();

    int liquid_source_as_int;
    data.read( "type", liquid_source_as_int );
    type = static_cast<liquid_source_type>( liquid_source_as_int );

    data.read( "standard_item", standard_item );
    data.read( "as_infinite_terrain", as_infinite_terrain );
    data.read( "as_vehicle_part", as_vehicle_part );

    uint64_t vp_index_as_int;
    data.read( "vp_index", vp_index_as_int );
    vp_index = static_cast<size_t>( vp_index_as_int );

    data.read( "as_monster", as_monster );

    if( data.has_member( "fake_item" ) ) {
        item fake_item;
        std::string fake_item_as_string;
        data.read( "fake_item", fake_item_as_string );
        ::deserialize_from_string( fake_item, fake_item_as_string );
        liquid_source_item = std::make_shared<item>( fake_item );
    }
}
liquid_dest dest_opt = LD_NULL;
tripoint_bub_ms pos;
item_location item_loc;
size_t vp_index; // of vehicle at pos

void liquid_dest_opt::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "dest_opt", static_cast<int>( dest_opt ) );
    jsout.member( "pos", pos );
    jsout.member( "item_loc", item_loc );
    jsout.member( "vp_index", static_cast<uint64_t>( vp_index ) );
    jsout.end_object();
}

void liquid_dest_opt::deserialize( const JsonValue &jsin )
{
    JsonObject data = jsin.get_object();

    int dest_opt_as_int;
    data.read( "dest_opt", dest_opt_as_int );
    dest_opt = static_cast<liquid_dest>( dest_opt_as_int );

    data.read( "pos", pos );
    data.read( "item_loc", item_loc );

    uint64_t vp_index_as_int;
    data.read( "vp_index", vp_index_as_int );
    vp_index = static_cast<size_t>( vp_index_as_int );
}

namespace liquid_handler
{

void handle_npc_liquid( item &liquid, Character &who )
{
    if( !liquid.count_by_charges() ) {
        debugmsg( "Attempted to run handle_npc_liquid on an item that doesn't have count_by_charges: %s",
                  liquid.type_name() );
        return;
    }
    if( liquid.charges <= 0 ) {
        return;
    }

    std::vector<item_location> container_locs;
    std::vector<int> capacities;
    std::vector<int> priorities;

    map &here = get_map();
    const tripoint_bub_ms char_pos = who.pos_bub();
    // Look for containers in adjacent tiles or in the character's inventory
    for( const tripoint_bub_ms &pos : closest_points_first( char_pos, 1 ) ) {
        for( item &it : here.i_at( pos ) ) {
            if( it.is_watertight_container() ) {
                container_locs.emplace_back( map_cursor( pos ), &it );
            }
        }
        const std::optional<vpart_reference> ovp = here.veh_at( pos ).cargo();
        if( ovp ) {
            for( item &it : ovp->items() ) {
                if( it.is_watertight_container() ) {
                    container_locs.emplace_back( vehicle_cursor( ovp->vehicle(), ovp->part_index() ), &it );
                }
            }
        }
    }
    for( item_location &item_loc : who.all_items_loc() ) {
        if( item_loc->is_watertight_container() ) {
            container_locs.push_back( item_loc );
        }
    }
    for( item_location &container_loc : container_locs ) {
        const bool is_carried = container_loc.carrier() != nullptr;
        const bool allow_buckets = container_loc.where() == item_location::type::map;
        // The amount of liquid that can fit into the container
        const int capacity = container_loc->get_remaining_capacity_for_liquid( liquid, allow_buckets );
        capacities.push_back( capacity );
        int priority = 0;
        if( capacity > 0 ) {
            // Adding to a partially filled container is ideal
            if( !container_loc->empty() ) {
                priority += 40;
            }
            // Prefer containers that are not being carried
            if( !is_carried ) {
                priority += 20;
            }
            // Prefer containers that don't spill
            if( !container_loc->will_spill() ) {
                priority += 10;
            }
        }
        priorities.push_back( priority );
    }
    const int num_containers = container_locs.size();
    while( liquid.charges > 0 ) {
        int best_idx = -1;
        for( int idx = 0; idx < num_containers; idx++ ) {
            if( capacities[idx] <= 0 ) {
                continue;
            }
            if( best_idx < 0 ) {
                // This is the first valid container we have seen so far
                best_idx = idx;
            } else if( priorities[idx] != priorities[best_idx] ) {
                // Prefer containers with higher priority
                if( priorities[idx] > priorities[best_idx] ) {
                    best_idx = idx;
                }
            } else if( capacities[idx] != capacities[best_idx] ) {
                // Within the same priority, prefer containers that can fit more
                if( capacities[idx] > capacities[best_idx] ) {
                    best_idx = idx;
                }
            } else if( container_locs[idx]->base_volume() != container_locs[best_idx]->base_volume() ) {
                // For the same [remaining] capacity, prefer the smallest container
                if( container_locs[idx]->base_volume() < container_locs[best_idx]->base_volume() ) {
                    best_idx = idx;
                }
            }
        }
        if( best_idx < 0 ) {
            // No suitable container, spill on the ground
            here.add_item_or_charges( char_pos, liquid );
            liquid.charges = 0;
            break;
        }
        const int amount = std::min( liquid.charges, capacities[best_idx] );
        liquid.charges -= container_locs[best_idx]->fill_with( liquid, amount );
        capacities[best_idx] = 0;
    }
    who.invalidate_weight_carried_cache();
}

void handle_all_or_npc_liquid( Character &p, item &newit, int radius )
{
    if( p.is_avatar() ) {
        liquid_handler::handle_liquid( liquid_wrapper( newit ), std::nullopt, radius );
    } else {
        liquid_handler::handle_npc_liquid( newit, p );
    }
}

bool handle_all_liquids_from_container( item_location &container, int radius )
{
    bool handled = false;
    for( item *contained : container->all_items_top() ) {
        const item_location loc( container, contained );
        if( handle_liquid( liquid_wrapper( loc ), std::nullopt, radius ) ) {
            handled = true;
        }
    }
    return handled;
}

static bool handle_liquid_menu( const std::string &menu_prompt,
                                const std::vector <uilist_entry> &menu_entries,
                                const std::vector<std::function<void()>> &actions,
                                bool liquid_is_rotten,
                                liquid_dest &dest_opt_result,
                                bool source_was_new )
{
    while( dest_opt_result == LD_NULL ) {

        uilist menu( menu_prompt, menu_entries );
        if( liquid_is_rotten ) {
            // Pre-select this one as it is the most likely one for rotten liquids
            menu.selected = menu.entries.size() - 1;
        }

        if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= actions.size() ) {
            if( source_was_new ) {
                continue;
            }
            add_msg( _( "Never mind." ) );
            // Explicitly canceled all options (container, drink, pour).
            return false;
        }

        actions[menu.ret]();
    }
    return true;
}

#pragma optimize("", off)
static bool get_liquid_destination( const liquid_wrapper &source, liquid_dest_opt &target,
                                    const int radius = 0 )
{
    map &here = get_map();
    const item *liquid_item = source.get_item();
    const tripoint_bub_ms liquid_pos = here.get_bub( source.get_item_pos() );
    if( !liquid_item->made_of_from_type( phase_id::LIQUID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return false;
    }

    Character &player_character = get_player_character();
    if( test_mode ) {
        switch( test_mode_spilling_action ) {
            case test_mode_spilling_action_t::spill_all:
                target.pos = player_character.pos_bub();
                target.dest_opt = LD_GROUND;
                return true;
            case test_mode_spilling_action_t::cancel_spill:
                return false;
        }
    }

    std::vector<std::function<void()>> actions;
    std::vector<uilist_entry> menu_entries;

    if( source.can_consume() ) {
        menu_entries.emplace_back( -1, true, 'e', _( "Consume it" ) );
        actions.emplace_back( [&]() {
            target.dest_opt = LD_CONSUME;
        } );
    }
    // This handles containers found anywhere near the player, including on the map and in vehicle storage.
    menu_entries.emplace_back( -1, true, 'c', _( "Pour into a container" ) );
    actions.emplace_back( [&]() {
        target.item_loc = game_menus::inv::container_for( player_character, *source.get_item(),
                          radius, source.get_container() );
        item *const cont = target.item_loc.get_item();

        if( cont == nullptr || cont->is_null() ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        // Sometimes the cont parameter is omitted, but the liquid is still within a container that counts
        // as valid target for the liquid. So check for that.
        if( cont == liquid_item || ( !cont->empty() && cont->has_item( *liquid_item ) ) ) {
            add_msg( m_info, _( "That's the same container!" ) );
            return; // The user has intended to do something, but mistyped.
        }
        target.dest_opt = LD_ITEM;
    } );
    // This handles liquids stored in vehicle parts directly (e.g. tanks).

    std::set<vehicle *> adjacent_vehicles;
    std::set<vpart_reference> adjacent_fluid_tanks;

    vehicle *source_veh = nullptr;
    if( source.type == liquid_source_type::VEHICLE ) {
        source_veh = &here.veh_at( *source.as_vehicle_part )->vehicle();
    }

    const auto veh_accepts_liquid = [&liquid_item]( const vpart_reference & pt ) {
        return pt.part().can_reload( *liquid_item );
    };
    for( const tripoint_bub_ms &e : here.points_in_radius( player_character.pos_bub(), 1 ) ) {

        optional_vpart_position ovp = here.veh_at( e );
        if( ovp ) {
            vehicle_part_range vp_range = ovp->vehicle().get_all_parts();

            for( const vpart_reference &pt : vp_range ) {
                if( veh_accepts_liquid( pt ) && e != liquid_pos ) {
                    adjacent_fluid_tanks.insert( pt );
                }
            }
        }

    }
    for( const vpart_reference &vpr : adjacent_fluid_tanks ) {

        vehicle *fluid_tank_veh = &vpr.vehicle();
        if( fluid_tank_veh == source_veh && fluid_tank_veh->has_part( "FLUIDTANK" ) ) {
            for( const vpart_reference &vp : fluid_tank_veh->get_avail_parts( "FLUIDTANK" ) ) {
                if( vp.part().get_base().can_reload_with( *liquid_item, true ) ) {

                    menu_entries.emplace_back( -1, true, MENU_AUTOASSIGN,
                                               string_format( _( "Fill available tank of %s" ), fluid_tank_veh->name ) );
                    actions.emplace_back( [ &, vp]() {
                        target.pos = vp.pos_bub( here );
                        target.vp_index = vp.part_index();
                        target.dest_opt = LD_VEH;
                    } );
                    break;
                }
            }
        } else {
            menu_entries.emplace_back( -1, true, MENU_AUTOASSIGN, string_format( _( "Fill nearby vehicle %s" ),
                                       fluid_tank_veh->name ) );
            actions.emplace_back( [ &, vpr]() {
                target.pos = vpr.pos_bub( here );
                target.vp_index = vpr.part_index();
                target.dest_opt = LD_VEH;
            } );
        }
    }

    for( const tripoint_bub_ms &target_pos : here.points_in_radius( player_character.pos_bub(), 1 ) ) {
        if( !iexamine::has_keg( target_pos ) ) {
            continue;
        }
        if( liquid_pos == target_pos ) {
            continue;
        }
        const std::string dir = direction_name( direction_from( player_character.pos_bub(), target_pos ) );
        menu_entries.emplace_back( -1, true, MENU_AUTOASSIGN, _( "Pour into an adjacent keg (%s)" ), dir );
        actions.emplace_back( [ &, target_pos]() {
            target.pos = target_pos;
            target.dest_opt = LD_KEG;
        } );
    }

    menu_entries.emplace_back( -1, true, 'g', _( "Pour on the ground" ) );
    actions.emplace_back( [&]() {
        // From infinite source to the ground somewhere else. The target has
        // infinite space and the liquid can not be used from there anyway.
        if( !!source.as_infinite_terrain && source.get_item()->has_infinite_charges() ) {
            add_msg( m_info, _( "Clearing out the %s would take forever." ),
                     here.name( liquid_pos ) );
            return;
        }

        const std::string liqstr = string_format( _( "Pour %s where?" ), liquid_item->display_name() );

        const std::optional<tripoint_bub_ms> target_pos_ = choose_adjacent( liqstr );
        if( !target_pos_ ) {
            return;
        }
        target.pos = *target_pos_;

        if( liquid_pos == target.pos ) {
            add_msg( m_info, _( "That's where you took it from!" ) );
            return;
        }
        if( !here.can_put_items_ter_furn( target.pos ) ) {
            add_msg( m_info, _( "You can't pour there!" ) );
            return;
        }
        target.dest_opt = LD_GROUND;
    } );

    if( menu_entries.empty() ) {
        return false;
    }

    return handle_liquid_menu( source.get_menu_prompt(), menu_entries, actions, liquid_item->rotten(),
                               target.dest_opt, source.source_item_was_new() );
}

static bool check_liquid( const item &liquid )
{
    if( !liquid.made_of_from_type( phase_id::LIQUID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        return false;
    }
    return true;
}

bool can_handle_liquid( const item &liquid )
{
    if( liquid.made_of_from_type( phase_id::SOLID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return false;
    }
    if( !liquid.made_of( phase_id::LIQUID ) ) {
        add_msg( _( "The %s froze solid before you could finish." ), liquid.tname() );
        return false;
    }
    return true;
}

#pragma optimize("", off)
bool handle_liquid( const liquid_wrapper &source, const std::optional<liquid_dest_opt> &destination,
                    const int dest_container_search_radius )
{
    const item &liquid_item = *source.get_item();
    if( !can_handle_liquid( liquid_item ) ) {
        return false;
    }
    liquid_dest_opt found_destination;

    if( !!destination ) {
        found_destination = *destination;
    } else {
        // requires a player UI input
        bool found = get_liquid_destination( source, found_destination, dest_container_search_radius );
        if( !found ) {
            return false;
        }
    }

    if( !check_liquid( liquid_item ) ) {
        // "canceled by the user" because we cannot handle it.
        return false;
    }

    Character &player_character = get_player_character();
    if( found_destination.dest_opt == LD_CONSUME ) {
        player_character.assign_activity( consume_activity_actor( source ) );
    } else {
        player_character.assign_activity( fill_liquid_activity_actor( source, found_destination,
                                          dest_container_search_radius ) );
    }
    return true;
}

liquid_dest_opt select_liquid_target( item &liquid, const int radius )
{
    liquid_dest_opt target;

    if( !liquid.made_of_from_type( phase_id::LIQUID ) ) {
        dbg( D_ERROR ) << "select_liquid_target: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        return target;
    }

    Character &player_character = get_player_character();

    uilist menu;

    map &here = get_map();
    const std::string liquid_name = liquid.display_name( liquid.charges );
    //~ %s: liquid name
    menu.text = string_format( pgettext( "liquid", "What to do with the %s?" ), liquid_name );

    std::vector<std::function<void()>> actions;
    // This handles containers found anywhere near the player, including on the map and in vehicle storage.
    menu.addentry( -1, true, 'c', _( "Select a container" ) );
    actions.emplace_back( [&]() {
        target.item_loc = game_menus::inv::container_for( player_character, liquid,
                          radius );
        item *const cont = target.item_loc.get_item();

        if( cont == nullptr || cont->is_null() ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        target.dest_opt = LD_ITEM;
    } );

    for( const tripoint_bub_ms &target_pos : here.points_in_radius( player_character.pos_bub(), 1 ) ) {
        if( !iexamine::has_keg( target_pos ) ) {
            continue;
        }
        const std::string dir = direction_name( direction_from( player_character.pos_bub(), target_pos ) );
        menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Select an adjacent keg (%s)" ), dir );
        actions.emplace_back( [ &, target_pos]() {
            target.pos = target_pos;
            target.dest_opt = LD_KEG;
        } );
    }

    if( menu.entries.empty() ) {
        return target;
    }

    while( target.dest_opt == LD_NULL ) {
        menu.query();
        if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= actions.size() ) {
            add_msg( _( "Never mind." ) );
            // Explicitly canceled all options (container, drink, pour).
            return target;
        }

        actions[menu.ret]();
    }
    return target;
}

} // namespace liquid_handler
