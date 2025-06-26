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
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "units.h"
#include "veh_interact.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "vpart_range.h"

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static const activity_id ACT_FILL_LIQUID( "ACT_FILL_LIQUID" );
static const flag_id json_flag_FROM_FROZEN_LIQUID( "FROM_FROZEN_LIQUID" );

// All serialize_liquid_source functions should add the same number of elements to the vectors of
// the activity. This makes it easier to distinguish the values of the source and the values of the target.
static void serialize_liquid_source( player_activity &act, const vehicle &veh, const int part_num,
                                     const item &liquid )
{
    map &here = get_map();
    act.values.push_back( static_cast<int>( liquid_source_type::VEHICLE ) );
    act.values.push_back( part_num );
    if( part_num != -1 ) {
        act.coords.push_back( here.get_abs( veh.bub_part_pos( here, part_num ) ) );
    } else {
        act.coords.push_back( veh.pos_abs() );
    }
    act.str_values.push_back( serialize( liquid ) );
}

static void serialize_liquid_source( player_activity &act, const tripoint_bub_ms &pos,
                                     const item &liquid )
{
    map &here = get_map();

    const map_stack stack = here.i_at( pos );
    // Need to store the *index* of the item on the ground, but it may be a virtual item from
    // an infinite liquid source.
    const auto iter = std::find_if( stack.begin(), stack.end(), [&]( const item & i ) {
        return &i == &liquid;
    } );
    if( iter == stack.end() ) {
        act.values.push_back( static_cast<int>( liquid_source_type::INFINITE_MAP ) );
        act.values.push_back( 0 ); // dummy
    } else {
        act.values.push_back( static_cast<int>( liquid_source_type::MAP_ITEM ) );
        act.values.push_back( std::distance( stack.begin(), iter ) );
    }
    act.coords.push_back( here.get_abs( pos ) );
    act.str_values.push_back( serialize( liquid ) );
}

static void serialize_liquid_target( player_activity &act, const vpart_reference &vp )
{
    map &here = get_map();
    act.values.push_back( static_cast<int>( liquid_target_type::VEHICLE ) );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( here.get_abs( vp.vehicle().bub_part_pos( here,  0 ) ) );
    act.values.push_back( vp.part_index() ); // tank part index
}

static void serialize_liquid_target( player_activity &act, const item_location &container_item )
{
    act.values.push_back( static_cast<int>( liquid_target_type::CONTAINER ) );
    act.values.push_back( 0 ); // dummy
    act.targets.push_back( container_item );
    act.coords.emplace_back( ); // dummy
}

static void serialize_liquid_target( player_activity &act, const tripoint_bub_ms &pos )
{
    act.values.push_back( static_cast<int>( liquid_target_type::MAP ) );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( get_map().get_abs( pos ) );
}

namespace liquid_handler
{
void handle_all_liquid( item liquid, const int radius, const item *const avoid )
{
    while( liquid.charges > 0 && can_handle_liquid( liquid ) ) {
        // handle_liquid allows to pour onto the ground, which will handle all the liquid and
        // set charges to 0. This allows terminating the loop.
        // The result of handle_liquid is ignored, the player *has* to handle all the liquid.
        liquid_dest_opt liquid_target;
        handle_liquid( liquid, liquid_target, avoid, radius );
    }
}

bool consume_liquid( item &liquid, const int radius, const item *const avoid )
{
    const int original_charges = liquid.charges;
    liquid_dest_opt liquid_target;
    while( liquid.charges > 0 && handle_liquid( liquid, liquid_target, avoid, radius ) ) {
        liquid_target.dest_opt = LD_NULL;
        // try again with the remaining charges
    }
    return original_charges != liquid.charges;
}

bool handle_all_liquids_from_container( item_location &container, int radius )
{
    bool handled = false;
    for( item *contained : container->all_items_top() ) {
        item_location loc( container, contained );
        if( handle_liquid( loc, &*container, radius ) ) {
            handled = true;
        }
    }
    return handled;
}

// todo: remove in favor of the item_location version
static bool get_liquid_target( item &liquid, const item *const source, const int radius,
                               const tripoint_bub_ms *const source_pos,
                               const vehicle *const source_veh,
                               const monster *const source_mon,
                               liquid_dest_opt &target )
{
    if( !liquid.made_of_from_type( phase_id::LIQUID ) ) {
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

    uilist menu;

    map &here = get_map();
    const std::string liquid_name = liquid.display_name( liquid.charges );
    if( source_pos != nullptr ) {
        //~ %1$s: liquid name, %2$s: terrain name
        menu.text = string_format( pgettext( "liquid", "What to do with the %1$s from %2$s?" ), liquid_name,
                                   here.name( *source_pos ) );
    } else if( source_veh != nullptr ) {
        //~ %1$s: liquid name, %2$s: vehicle name
        menu.text = string_format( pgettext( "liquid", "What to do with the %1$s from %2$s?" ), liquid_name,
                                   source_veh->disp_name() );
    } else if( source_mon != nullptr ) {
        //~ %1$s: liquid name, %2$s: monster name
        menu.text = string_format( pgettext( "liquid", "What to do with the %1$s from the %2$s?" ),
                                   liquid_name, source_mon->get_name() );
    } else if( source != nullptr ) {
        if( source->is_bucket_nonempty() ) {
            menu.text = string_format( pgettext( "liquid",
                                                 //~ %1$s: liquid name, %2$s: container name
                                                 "The %1$s would spill.\nWhat to do with the %2$s inside it?" ),
                                       source->display_name(), liquid_name );
        } else {
            //~ %1$s: liquid name, %2$s: container name
            menu.text = string_format( pgettext( "liquid", "What to do with the %1$s in the %2$s?" ),
                                       liquid_name, source->display_name() );
        }
    } else {
        //~ %s: liquid name
        menu.text = string_format( pgettext( "liquid", "What to do with the %s?" ), liquid_name );
    }
    std::vector<std::function<void()>> actions;
    if( player_character.can_consume_as_is( liquid ) && !source_mon && ( source_veh || source_pos ) ) {
        menu.addentry( -1, true, 'e', _( "Consume it" ) );
        actions.emplace_back( [&]() {
            target.dest_opt = LD_CONSUME;
        } );
    }
    // This handles containers found anywhere near the player, including on the map and in vehicle storage.
    menu.addentry( -1, true, 'c', _( "Pour into a container" ) );
    actions.emplace_back( [&]() {
        target.item_loc = game_menus::inv::container_for( player_character, liquid,
                          radius, /*avoid=*/source );
        item *const cont = target.item_loc.get_item();

        if( cont == nullptr || cont->is_null() ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        // Sometimes the cont parameter is omitted, but the liquid is still within a container that counts
        // as valid target for the liquid. So check for that.
        if( cont == source || ( !cont->empty() && cont->has_item( liquid ) ) ) {
            add_msg( m_info, _( "That's the same container!" ) );
            return; // The user has intended to do something, but mistyped.
        }
        target.dest_opt = LD_ITEM;
    } );
    // This handles liquids stored in vehicle parts directly (e.g. tanks).
    std::set<vehicle *> opts;
    for( const tripoint_bub_ms &e : here.points_in_radius( player_character.pos_bub(), 1 ) ) {
        vehicle *veh = veh_pointer_or_null( here.veh_at( e ) );
        if( veh ) {
            vehicle_part_range vpr = veh->get_all_parts();
            const auto veh_accepts_liquid = [&liquid]( const vpart_reference & pt ) {
                return pt.part().can_reload( liquid );
            };
            if( std::any_of( vpr.begin(), vpr.end(), veh_accepts_liquid ) ) {
                opts.insert( veh );
            }
        }
    }
    for( vehicle *veh : opts ) {
        if( veh == source_veh && veh->has_part( "FLUIDTANK", false ) ) {
            for( const vpart_reference &vp : veh->get_avail_parts( "FLUIDTANK" ) ) {
                if( vp.part().get_base().can_reload_with( liquid, true ) ) {
                    menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Fill available tank" ) );
                    actions.emplace_back( [ &, veh]() {
                        target.veh = veh;
                        target.dest_opt = LD_VEH;
                    } );
                    break;
                }
            }
        } else {
            menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Fill nearby vehicle %s" ), veh->name );
            actions.emplace_back( [ &, veh]() {
                target.veh = veh;
                target.dest_opt = LD_VEH;
            } );
        }
    }

    for( const tripoint_bub_ms &target_pos : here.points_in_radius( player_character.pos_bub(), 1 ) ) {
        if( !iexamine::has_keg( target_pos ) ) {
            continue;
        }
        if( source_pos != nullptr && *source_pos == target_pos ) {
            continue;
        }
        const std::string dir = direction_name( direction_from( player_character.pos_bub(), target_pos ) );
        menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Pour into an adjacent keg (%s)" ), dir );
        actions.emplace_back( [ &, target_pos]() {
            target.pos = target_pos;
            target.dest_opt = LD_KEG;
        } );
    }

    menu.addentry( -1, true, 'g', _( "Pour on the ground" ) );
    actions.emplace_back( [&]() {
        // From infinite source to the ground somewhere else. The target has
        // infinite space and the liquid can not be used from there anyway.
        if( liquid.has_infinite_charges() && source_pos != nullptr ) {
            add_msg( m_info, _( "Clearing out the %s would take forever." ), here.name( *source_pos ) );
            return;
        }

        const std::string liqstr = string_format( _( "Pour %s where?" ), liquid_name );

        const std::optional<tripoint_bub_ms> target_pos_ = choose_adjacent( liqstr );
        if( !target_pos_ ) {
            return;
        }
        target.pos = *target_pos_;

        if( source_pos != nullptr && *source_pos == target.pos ) {
            add_msg( m_info, _( "That's where you took it from!" ) );
            return;
        }
        if( !here.can_put_items_ter_furn( target.pos ) ) {
            add_msg( m_info, _( "You can't pour there!" ) );
            return;
        }
        target.dest_opt = LD_GROUND;
    } );

    if( liquid.rotten() ) {
        // Pre-select this one as it is the most likely one for rotten liquids
        menu.selected = menu.entries.size() - 1;
    }

    if( menu.entries.empty() ) {
        return false;
    }

    while( target.dest_opt == LD_NULL ) {
        menu.query();
        if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= actions.size() ) {
            add_msg( _( "Never mind." ) );
            // Explicitly canceled all options (container, drink, pour).
            return false;
        }

        actions[menu.ret]();
    }
    return true;
}

static bool get_liquid_target( item_location &liquid, const item *const source, const int radius,
                               liquid_dest_opt &target )
{
    const map &here = get_map();

    const tripoint_bub_ms *source_pos = nullptr;
    const vehicle *source_veh = nullptr;
    const monster *source_mon = nullptr;

    tripoint_bub_ms pos;
    switch( liquid.where() ) {
        case item_location::type::container:
            // intentionally empty
            break;
        case item_location::type::map:
            pos = liquid.pos_bub( here );
            source_pos = &pos;
            break;
        case item_location::type::vehicle:
            source_veh = &liquid.veh_cursor()->veh;
            break;
        default:
            debugmsg( "Tried to get liquid target %s from invalid location %s", liquid->display_name(),
                      liquid.where() );
            return false;
    }

    return get_liquid_target( *liquid, source, radius, source_pos, source_veh, source_mon, target );
}

static bool handle_keg_or_ground_target( Character &player_character, item &liquid,
        liquid_dest_opt &target, const std::function<bool()> &create_activity, bool silent )
{
    if( create_activity() ) {
        serialize_liquid_target( player_character.activity, target.pos );
    } else {
        if( target.dest_opt == LD_KEG ) {
            iexamine::pour_into_keg( target.pos, liquid, silent );
        } else {
            get_map().add_item_or_charges( target.pos, liquid );
            liquid.charges = 0;
        }
        player_character.mod_moves( -100 );
    }
    return true;
}

static bool handle_item_target( Character &player_character, item &liquid, liquid_dest_opt &target,
                                const std::function<bool()> &create_activity, bool silent )
{
    // Currently activities can only store item position in the players inventory,
    // not on ground or similar. TODO: implement storing arbitrary container locations.
    if( target.item_loc && create_activity() ) {
        serialize_liquid_target( player_character.activity, target.item_loc );
    } else if( player_character.pour_into( target.item_loc, liquid, true, silent ) ) {
        target.item_loc.make_active();
        player_character.mod_moves( -100 );
    }
    return true;
}

static bool handle_vehicle_target( Character &player_character, item &liquid,
                                   liquid_dest_opt &target, const std::function<bool()> &create_activity )
{
    map &here = get_map();
    if( target.veh == nullptr ) {
        return false;
    }
    auto sel = [&]( const map &, const vehicle_part & pt ) {
        return pt.is_tank() && pt.can_reload( liquid );
    };

    const units::volume stack = 250_ml / liquid.type->stack_size;
    const std::string title = string_format( _( "Select target tank for <color_%s>%.1fL %s</color>" ),
                              get_all_colors().get_name( liquid.color() ),
                              round_up( to_liter( liquid.charges * stack ), 1 ),
                              liquid.tname() );

    const std::optional<vpart_reference> vpr = veh_interact::select_part( here, *target.veh, sel,
            title );
    if( !vpr ) {
        return false;
    }

    if( create_activity() ) {
        serialize_liquid_target( player_character.activity, *vpr );
        return true;
    } else if( player_character.pour_into( *vpr, liquid ) ) {
        // this branch is used in milking and magiclysm butchery blood draining
        player_character.mod_moves( -1000 ); // consistent with veh_interact::do_refill activity
        return true;
    } else {
        // unclear what can reach this branch but return false just in case
        return false;
    }
}

static bool check_liquid( item &liquid )
{
    if( !liquid.made_of_from_type( phase_id::LIQUID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        return false;
    }
    return true;
}

bool perform_liquid_transfer( item_location &liquid, liquid_dest_opt &target )
{
    map &here = get_map();

    if( !check_liquid( *liquid ) ) {
        // "canceled by the user" because we *can* not handle it.
        return false;
    }

    // todo: migrate ACT_FILL_LIQUID to activity_actor
    Character &player_character = get_player_character();
    const auto create_activity = [&]() {
        if( liquid.where() == item_location::type::vehicle ) {
            player_character.assign_activity( ACT_FILL_LIQUID );
            const vehicle_cursor *veh_cur = liquid.veh_cursor();
            serialize_liquid_source( player_character.activity, veh_cur->veh, veh_cur->part, *liquid );
            return true;
        } else if( liquid.where() == item_location::type::map ) {
            player_character.assign_activity( ACT_FILL_LIQUID );
            serialize_liquid_source( player_character.activity, liquid.pos_bub( here ), *liquid );
            return true;
        } else {
            return false;
        }
    };

    switch( target.dest_opt ) {
        case LD_CONSUME:
            player_character.assign_activity( consume_activity_actor( liquid ) );
            return true;
        case LD_ITEM: {
            return handle_item_target( player_character, *liquid, target, create_activity, false );
        }
        case LD_VEH: {
            return handle_vehicle_target( player_character, *liquid, target, create_activity );
        }
        case LD_KEG:
        case LD_GROUND:
            return handle_keg_or_ground_target( player_character, *liquid, target, create_activity, false );
        case LD_NULL:
        default:
            return false;
    }
}

// todo: Remove in favor of the item_location version.
bool perform_liquid_transfer( item &liquid, const tripoint_bub_ms *const source_pos,
                              const vehicle *const source_veh, const int part_num,
                              const monster *const /*source_mon*/, liquid_dest_opt &target, bool silent )
{
    if( !check_liquid( liquid ) ) {
        // "canceled by the user" because we *can* not handle it.
        return false;
    }

    Character &player_character = get_player_character();
    const auto create_activity = [&]() {
        if( source_veh != nullptr ) {
            player_character.assign_activity( ACT_FILL_LIQUID );
            serialize_liquid_source( player_character.activity, *source_veh, part_num, liquid );
            return true;
        } else if( source_pos != nullptr ) {
            player_character.assign_activity( ACT_FILL_LIQUID );
            serialize_liquid_source( player_character.activity, *source_pos, liquid );
            return true;
        } else {
            return false;
        }
    };

    switch( target.dest_opt ) {
        case LD_CONSUME:
            player_character.assign_activity( consume_activity_actor( liquid ) );
            liquid.charges--;
            return true;
        case LD_ITEM: {
            return handle_item_target( player_character, liquid, target, create_activity, silent );
        }
        case LD_VEH: {
            return handle_vehicle_target( player_character, liquid, target, create_activity );
        }
        case LD_KEG:
        case LD_GROUND:
            return handle_keg_or_ground_target( player_character, liquid, target, create_activity, silent );
        case LD_NULL:
        default:
            return false;
    }
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

bool handle_liquid( item &liquid, liquid_dest_opt &liquid_target, const item *const source,
                    const int radius,
                    const tripoint_bub_ms *const source_pos,
                    const vehicle *const source_veh, const int part_num,
                    const monster *const source_mon, bool silent )
{
    bool success = false;
    if( !can_handle_liquid( liquid ) ) {
        return false;
    }
    if( liquid_target.dest_opt != LD_NULL ||
        get_liquid_target( liquid, source, radius, source_pos, source_veh, source_mon,
                           liquid_target ) ) {
        success = perform_liquid_transfer( liquid, source_pos, source_veh, part_num, source_mon,
                                           liquid_target, silent );
        if( success && ( ( liquid_target.dest_opt == LD_ITEM &&
                           liquid_target.item_loc->is_watertight_container() ) || liquid_target.dest_opt == LD_KEG ) ) {
            liquid.unset_flag( flag_id( json_flag_FROM_FROZEN_LIQUID ) );
        }
        return success;
    }
    return false;
}

bool handle_liquid( item_location &liquid, const item *const source, const int radius )
{
    bool success = false;
    if( !can_handle_liquid( *liquid ) ) {
        return false;
    }
    struct liquid_dest_opt liquid_target;
    if( get_liquid_target( liquid, source, radius, liquid_target ) ) {
        success = perform_liquid_transfer( liquid, liquid_target );
        if( success && ( ( liquid_target.dest_opt == LD_ITEM &&
                           liquid_target.item_loc->is_watertight_container() ) || liquid_target.dest_opt == LD_KEG ) ) {
            liquid->unset_flag( flag_id( json_flag_FROM_FROZEN_LIQUID ) );
            if( liquid.where() == item_location::type::container ) {
                liquid.parent_item().on_contents_changed();
            }
            if( liquid->charges == 0 ) {
                liquid.remove_item();
            }
        }
        return success;
    }
    return false;
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
