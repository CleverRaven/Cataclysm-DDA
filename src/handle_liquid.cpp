#include "handle_liquid.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <list>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "action.h"
#include "activity_actor.h"
#include "activity_type.h"
#include "cata_utility.h"
#include "character.h"
#include "colony.h"
#include "debug.h"
#include "enums.h"
#include "game_inventory.h"
#include "iexamine.h"
#include "item.h"
#include "item_contents.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "optional.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

// All serialize_liquid_source functions should add the same number of elements to the vectors of
// the activity. This makes it easier to distinguish the values of the source and the values of the target.
static void serialize_liquid_source( player_activity &act, const vehicle &veh, const int part_num,
                                     const item &liquid )
{
    act.values.push_back( static_cast<int>( liquid_source_type::VEHICLE ) );
    act.values.push_back( part_num );
    act.coords.push_back( veh.global_pos3() );
    act.str_values.push_back( serialize( liquid ) );
}

static void serialize_liquid_source( player_activity &act, const tripoint &pos, const item &liquid )
{
    const map_stack stack = get_map().i_at( pos );
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
    act.coords.push_back( pos );
    act.str_values.push_back( serialize( liquid ) );
}

static void serialize_liquid_target( player_activity &act, const vehicle &veh )
{
    act.values.push_back( static_cast<int>( liquid_target_type::VEHICLE ) );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( veh.global_pos3() );
}

static void serialize_liquid_target( player_activity &act, const item_location &container_item )
{
    act.values.push_back( static_cast<int>( liquid_target_type::CONTAINER ) );
    act.values.push_back( 0 ); // dummy
    act.targets.push_back( container_item );
    act.coords.push_back( tripoint() ); // dummy
}

static void serialize_liquid_target( player_activity &act, const tripoint &pos )
{
    act.values.push_back( static_cast<int>( liquid_target_type::MAP ) );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( pos );
}

namespace liquid_handler
{
void handle_all_liquid( item liquid, const int radius, const item *const avoid )
{
    while( liquid.charges > 0 ) {
        // handle_liquid allows to pour onto the ground, which will handle all the liquid and
        // set charges to 0. This allows terminating the loop.
        // The result of handle_liquid is ignored, the player *has* to handle all the liquid.
        handle_liquid( liquid, avoid, radius );
    }
}

bool consume_liquid( item &liquid, const int radius, const item *const avoid )
{
    const int original_charges = liquid.charges;
    while( liquid.charges > 0 && handle_liquid( liquid, avoid, radius ) ) {
        // try again with the remaining charges
    }
    return original_charges != liquid.charges;
}

bool handle_liquid_from_ground( const map_stack::iterator &on_ground,
                                const tripoint &pos,
                                const int radius )
{
    // TODO: not all code paths on handle_liquid consume move points, fix that.
    handle_liquid( *on_ground, nullptr, radius, &pos );
    if( on_ground->charges > 0 ) {
        return false;
    }
    get_map().i_at( pos ).erase( on_ground );
    return true;
}

bool handle_liquid_from_container( item *in_container,
                                   item &container, int radius )
{
    // TODO: not all code paths on handle_liquid consume move points, fix that.
    const int old_charges = in_container->charges;
    handle_liquid( *in_container, &container, radius );
    if( in_container->charges != old_charges ) {
        container.on_contents_changed();
    }

    return in_container->charges <= 0;
}

bool handle_liquid_from_container( item &container, int radius )
{
    std::vector<item *> remove;
    bool handled = false;
    for( item *contained : container.contents.all_items_top() ) {
        if( handle_liquid_from_container( contained, container, radius ) ) {
            remove.push_back( contained );
            handled = true;
        }
    }
    for( item *contained : remove ) {
        container.remove_item( *contained );
    }
    return handled;
}

static bool get_liquid_target( item &liquid, const item *const source, const int radius,
                               const tripoint *const source_pos,
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
    } else {
        //~ %s: liquid name
        menu.text = string_format( pgettext( "liquid", "What to do with the %s?" ), liquid_name );
    }
    Character &player_character = get_player_character();
    std::vector<std::function<void()>> actions;
    if( player_character.can_consume( liquid ) && !source_mon && ( source_veh || source_pos ) ) {
        if( player_character.can_consume_for_bionic( liquid ) ) {
            menu.addentry( -1, true, 'e', _( "Fuel bionic with it" ) );
        } else {
            menu.addentry( -1, true, 'e', _( "Consume it" ) );
        }

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
        if( cont == source || ( !cont->contents.empty() && cont->has_item( liquid ) ) ) {
            add_msg( m_info, _( "That's the same container!" ) );
            return; // The user has intended to do something, but mistyped.
        }
        target.dest_opt = LD_ITEM;
    } );
    // This handles liquids stored in vehicle parts directly (e.g. tanks).
    std::set<vehicle *> opts;
    for( const tripoint &e : here.points_in_radius( player_character.pos(), 1 ) ) {
        vehicle *veh = veh_pointer_or_null( here.veh_at( e ) );
        vehicle_part_range vpr = veh->get_all_parts();
        if( veh && std::any_of( vpr.begin(), vpr.end(), [&liquid]( const vpart_reference & pt ) {
        return pt.part().can_reload( liquid );
        } ) ) {
            opts.insert( veh );
        }
    }
    for( vehicle *veh : opts ) {
        if( veh == source_veh ) {
            continue;
        }
        menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Fill nearby vehicle %s" ), veh->name );
        actions.emplace_back( [ &, veh]() {
            target.veh = veh;
            target.dest_opt = LD_VEH;
        } );
    }

    for( const tripoint &target_pos : here.points_in_radius( player_character.pos(), 1 ) ) {
        if( !iexamine::has_keg( target_pos ) ) {
            continue;
        }
        if( source_pos != nullptr && *source_pos == target_pos ) {
            continue;
        }
        const std::string dir = direction_name( direction_from( player_character.pos(), target_pos ) );
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

        const cata::optional<tripoint> target_pos_ = choose_adjacent( liqstr );
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

    menu.query();
    if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= actions.size() ) {
        add_msg( _( "Never mind." ) );
        // Explicitly canceled all options (container, drink, pour).
        return false;
    }

    actions[menu.ret]();
    return true;
}

static bool perform_liquid_transfer( item &liquid, const tripoint *const source_pos,
                                     const vehicle *const source_veh, const int part_num,
                                     const monster *const source_mon, liquid_dest_opt &target )
{
    bool transfer_ok = false;
    if( !liquid.made_of_from_type( phase_id::LIQUID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return transfer_ok;
    }

    Character &player_character = get_player_character();
    const auto create_activity = [&]() {
        if( source_veh != nullptr ) {
            player_character.assign_activity( activity_id( "ACT_FILL_LIQUID" ) );
            serialize_liquid_source( player_character.activity, *source_veh, part_num, liquid );
            return true;
        } else if( source_pos != nullptr ) {
            player_character.assign_activity( activity_id( "ACT_FILL_LIQUID" ) );
            serialize_liquid_source( player_character.activity, *source_pos, liquid );
            return true;
        } else if( source_mon != nullptr ) {
            return false;
        } else {
            return false;
        }
    };

    map &here = get_map();
    switch( target.dest_opt ) {
        case LD_CONSUME:
            player_character.assign_activity( player_activity( consume_activity_actor( liquid ) ) );
            liquid.charges--;
            transfer_ok = true;
            break;
        case LD_ITEM: {
            // Currently activities can only store item position in the players inventory,
            // not on ground or similar. TODO: implement storing arbitrary container locations.
            if( target.item_loc && create_activity() ) {
                serialize_liquid_target( player_character.activity, target.item_loc );
            } else if( player_character.pour_into( *target.item_loc, liquid ) ) {
                if( target.item_loc->needs_processing() ) {
                    // Polymorphism fail, have to introspect into the type to set the target container as active.
                    switch( target.item_loc.where() ) {
                        case item_location::type::map:
                            here.make_active( target.item_loc );
                            break;
                        case item_location::type::vehicle:
                            here.veh_at( target.item_loc.position() )->vehicle().make_active( target.item_loc );
                            break;
                        case item_location::type::container:
                        case item_location::type::character:
                        case item_location::type::invalid:
                            break;
                    }
                }
                player_character.mod_moves( -100 );
            }
            transfer_ok = true;
            break;
        }
        case LD_VEH:
            if( target.veh == nullptr ) {
                break;
            }
            if( create_activity() ) {
                serialize_liquid_target( player_character.activity, *target.veh );
            } else if( player_character.pour_into( *target.veh, liquid ) ) {
                player_character.mod_moves( -1000 ); // consistent with veh_interact::do_refill activity
            }
            transfer_ok = true;
            break;
        case LD_KEG:
        case LD_GROUND:
            if( create_activity() ) {
                serialize_liquid_target( player_character.activity, target.pos );
            } else {
                if( target.dest_opt == LD_KEG ) {
                    iexamine::pour_into_keg( target.pos, liquid );
                } else {
                    here.add_item_or_charges( target.pos, liquid );
                    liquid.charges = 0;
                }
                player_character.mod_moves( -100 );
            }
            transfer_ok = true;
            break;
        case LD_NULL:
        default:
            break;
    }
    return transfer_ok;
}

bool handle_liquid( item &liquid, const item *const source, const int radius,
                    const tripoint *const source_pos,
                    const vehicle *const source_veh, const int part_num,
                    const monster *const source_mon )
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
    struct liquid_dest_opt liquid_target;
    if( get_liquid_target( liquid, source, radius, source_pos, source_veh, source_mon,
                           liquid_target ) ) {
        return perform_liquid_transfer( liquid, source_pos, source_veh, part_num, source_mon,
                                        liquid_target );
    }
    return false;
}
} // namespace liquid_handler
