#include "handle_liquid.h"

#include <limits.h>
#include <stddef.h>
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include "action.h"
#include "avatar.h"
#include "game.h"
#include "game_inventory.h"
#include "iexamine.h"
#include "item.h"
#include "map.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "translations.h"
#include "ui.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "cata_utility.h"
#include "colony.h"
#include "debug.h"
#include "enums.h"
#include "line.h"
#include "optional.h"
#include "player_activity.h"
#include "string_formatter.h"

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

// All serialize_liquid_source functions should add the same number of elements to the vectors of
// the activity. This makes it easier to distinguish the values of the source and the values of the target.
static void serialize_liquid_source( player_activity &act, const vehicle &veh, const int part_num,
                                     const item &liquid )
{
    act.values.push_back( LST_VEHICLE );
    act.values.push_back( part_num );
    act.coords.push_back( veh.global_pos3() );
    act.str_values.push_back( serialize( liquid ) );
}

static void serialize_liquid_source( player_activity &act, const tripoint &pos, const item &liquid )
{
    const auto stack = g->m.i_at( pos );
    // Need to store the *index* of the item on the ground, but it may be a virtual item from
    // an infinite liquid source.
    const auto iter = std::find_if( stack.begin(), stack.end(), [&]( const item & i ) {
        return &i == &liquid;
    } );
    if( iter == stack.end() ) {
        act.values.push_back( LST_INFINITE_MAP );
        act.values.push_back( 0 ); // dummy
    } else {
        act.values.push_back( LST_MAP_ITEM );
        act.values.push_back( std::distance( stack.begin(), iter ) );
    }
    act.coords.push_back( pos );
    act.str_values.push_back( serialize( liquid ) );
}

static void serialize_liquid_target( player_activity &act, const vehicle &veh )
{
    act.values.push_back( LTT_VEHICLE );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( veh.global_pos3() );
}

static void serialize_liquid_target( player_activity &act, int container_item_pos )
{
    act.values.push_back( LTT_CONTAINER );
    act.values.push_back( container_item_pos );
    act.coords.push_back( tripoint() ); // dummy
}

static void serialize_liquid_target( player_activity &act, const tripoint &pos )
{
    act.values.push_back( LTT_MAP );
    act.values.push_back( 0 ); // dummy
    act.coords.push_back( pos );
}

namespace liquid_handler
{
void handle_all_liquid( item liquid, const int radius )
{
    while( liquid.charges > 0 ) {
        // handle_liquid allows to pour onto the ground, which will handle all the liquid and
        // set charges to 0. This allows terminating the loop.
        // The result of handle_liquid is ignored, the player *has* to handle all the liquid.
        handle_liquid( liquid, nullptr, radius );
    }
}

bool consume_liquid( item &liquid, const int radius )
{
    const auto original_charges = liquid.charges;
    while( liquid.charges > 0 && handle_liquid( liquid, nullptr, radius ) ) {
        // try again with the remaining charges
    }
    return original_charges != liquid.charges;
}

bool handle_liquid_from_ground( map_stack::iterator on_ground,
                                const tripoint &pos,
                                const int radius )
{
    // TODO: not all code paths on handle_liquid consume move points, fix that.
    handle_liquid( *on_ground, nullptr, radius, &pos );
    if( on_ground->charges > 0 ) {
        return false;
    }
    g->m.i_at( pos ).erase( on_ground );
    return true;
}

bool handle_liquid_from_container( std::list<item>::iterator in_container,
                                   item &container, int radius )
{
    // TODO: not all code paths on handle_liquid consume move points, fix that.
    const int old_charges = in_container->charges;
    handle_liquid( *in_container, &container, radius );
    if( in_container->charges != old_charges ) {
        container.on_contents_changed();
    }

    if( in_container->charges > 0 ) {
        return false;
    }
    container.contents.erase( in_container );
    return true;
}

bool handle_liquid_from_container( item &container, int radius )
{
    return handle_liquid_from_container( container.contents.begin(), container, radius );
}

static bool get_liquid_target( item &liquid, item *const source, const int radius,
                               const tripoint *const source_pos,
                               const vehicle *const source_veh,
                               const monster *const source_mon,
                               liquid_dest_opt &target )
{
    if( !liquid.made_of_from_type( LIQUID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return false;
    }

    uilist menu;

    const std::string liquid_name = liquid.display_name( liquid.charges );
    if( source_pos != nullptr ) {
        //~ %1$s: liquid name, %2$s: terrain name
        menu.text = string_format( pgettext( "liquid", "What to do with the %1$s from %2$s?" ), liquid_name,
                                   g->m.name( *source_pos ) );
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
    std::vector<std::function<void()>> actions;

    if( g->u.can_consume( liquid ) && !source_mon ) {
        menu.addentry( -1, true, 'e', _( "Consume it" ) );
        actions.emplace_back( [&]() {
            target.dest_opt = LD_CONSUME;
        } );
    }
    // This handles containers found anywhere near the player, including on the map and in vehicle storage.
    menu.addentry( -1, true, 'c', _( "Pour into a container" ) );
    actions.emplace_back( [&]() {
        target.item_loc = game_menus::inv::container_for( g->u, liquid, radius );
        item *const cont = target.item_loc.get_item();

        if( cont == nullptr || cont->is_null() ) {
            add_msg( _( "Never mind." ) );
            return;
        }
        if( source != nullptr && cont == source ) {
            add_msg( m_info, _( "That's the same container!" ) );
            return; // The user has intended to do something, but mistyped.
        }
        target.dest_opt = LD_ITEM;
    } );
    // This handles liquids stored in vehicle parts directly (e.g. tanks).
    std::set<vehicle *> opts;
    for( const auto &e : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        auto veh = veh_pointer_or_null( g->m.veh_at( e ) );
        if( veh && std::any_of( veh->parts.begin(), veh->parts.end(), [&liquid]( const vehicle_part & pt ) {
        return pt.can_reload( liquid );
        } ) ) {
            opts.insert( veh );
        }
    }
    for( auto veh : opts ) {
        if( veh == source_veh ) {
            continue;
        }
        menu.addentry( -1, true, MENU_AUTOASSIGN, _( "Fill nearby vehicle %s" ), veh->name );
        actions.emplace_back( [ &, veh]() {
            target.veh = veh;
            target.dest_opt = LD_VEH;
        } );
    }

    for( auto &target_pos : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        if( !iexamine::has_keg( target_pos ) ) {
            continue;
        }
        if( source_pos != nullptr && *source_pos == target_pos ) {
            continue;
        }
        const std::string dir = direction_name( direction_from( g->u.pos(), target_pos ) );
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
            add_msg( m_info, _( "Clearing out the %s would take forever." ), g->m.name( *source_pos ) );
            return;
        }

        const std::string liqstr = string_format( _( "Pour %s where?" ), liquid_name );

        g->refresh_all();
        const cata::optional<tripoint> target_pos_ = choose_adjacent( liqstr );
        if( !target_pos_ ) {
            return;
        }
        target.pos = *target_pos_;

        if( source_pos != nullptr && *source_pos == target.pos ) {
            add_msg( m_info, _( "That's where you took it from!" ) );
            return;
        }
        if( !g->m.can_put_items_ter_furn( target.pos ) ) {
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
    g->refresh_all();
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
    if( !liquid.made_of_from_type( LIQUID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return transfer_ok;
    }

    const auto create_activity = [&]() {
        if( source_veh != nullptr ) {
            g->u.assign_activity( activity_id( "ACT_FILL_LIQUID" ) );
            serialize_liquid_source( g->u.activity, *source_veh, part_num, liquid );
            return true;
        } else if( source_pos != nullptr ) {
            g->u.assign_activity( activity_id( "ACT_FILL_LIQUID" ) );
            serialize_liquid_source( g->u.activity, *source_pos, liquid );
            return true;
        } else if( source_mon != nullptr ) {
            return false;
        } else {
            return false;
        }
    };

    switch( target.dest_opt ) {
        case LD_CONSUME:
            g->u.consume_item( liquid );
            transfer_ok = true;
            break;
        case LD_ITEM: {
            item *const cont = target.item_loc.get_item();
            const int item_index = g->u.get_item_position( cont );
            // Currently activities can only store item position in the players inventory,
            // not on ground or similar. TODO: implement storing arbitrary container locations.
            if( item_index != INT_MIN && create_activity() ) {
                serialize_liquid_target( g->u.activity, item_index );
            } else if( g->u.pour_into( *cont, liquid ) ) {
                if( cont->needs_processing() ) {
                    // Polymorphism fail, have to introspect into the type to set the target container as active.
                    switch( target.item_loc.where() ) {
                        case item_location::type::map:
                            g->m.make_active( target.item_loc );
                            break;
                        case item_location::type::vehicle:
                            g->m.veh_at( target.item_loc.position() )->vehicle().make_active( target.item_loc );
                            break;
                        case item_location::type::character:
                        case item_location::type::invalid:
                            break;
                    }
                }
                g->u.mod_moves( -100 );
            }
            transfer_ok = true;
            break;
        }
        case LD_VEH:
            if( target.veh == nullptr ) {
                break;
            }
            if( create_activity() ) {
                serialize_liquid_target( g->u.activity, *target.veh );
            } else if( g->u.pour_into( *target.veh, liquid ) ) {
                g->u.mod_moves( -1000 ); // consistent with veh_interact::do_refill activity
            }
            transfer_ok = true;
            break;
        case LD_KEG:
        case LD_GROUND:
            if( create_activity() ) {
                serialize_liquid_target( g->u.activity, target.pos );
            } else {
                if( target.dest_opt == LD_KEG ) {
                    iexamine::pour_into_keg( target.pos, liquid );
                } else {
                    g->m.add_item_or_charges( target.pos, liquid );
                    liquid.charges = 0;
                }
                g->u.mod_moves( -100 );
            }
            transfer_ok = true;
            break;
        case LD_NULL:
        default:
            break;
    }
    return transfer_ok;
}

bool handle_liquid( item &liquid, item *const source, const int radius,
                    const tripoint *const source_pos,
                    const vehicle *const source_veh, const int part_num,
                    const monster *const source_mon )
{
    if( liquid.made_of_from_type( SOLID ) ) {
        dbg( D_ERROR ) << "game:handle_liquid: Tried to handle_liquid a non-liquid!";
        debugmsg( "Tried to handle_liquid a non-liquid!" );
        // "canceled by the user" because we *can* not handle it.
        return false;
    }
    if( !liquid.made_of( LIQUID ) ) {
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
