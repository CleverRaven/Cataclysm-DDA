#include "gates.h"

#include <algorithm>
#include <array>
#include <memory>
#include <set>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "character.h"
#include "colony.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enums.h"
#include "game.h" // TODO: This is a circular dependency
#include "generic_factory.h"
#include "iexamine.h"
#include "item.h"
#include "json.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "optional.h"
#include "player_activity.h"
#include "point.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"

static const efftype_id effect_stunned( "stunned" );

static const furn_str_id furn_f_crate_o( "f_crate_o" );
static const furn_str_id furn_f_safe_o( "f_safe_o" );

// Gates namespace

namespace
{

struct gate_data;

using gate_id = string_id<gate_data>;

struct gate_data {

    gate_data() :
        moves( 0 ),
        bash_dmg( 0 ),
        was_loaded( false ) {}

    gate_id id;

    ter_str_id door;
    ter_str_id floor;
    std::vector<ter_str_id> walls;

    translation pull_message;
    translation open_message;
    translation close_message;
    translation fail_message;

    int moves;
    int bash_dmg;
    bool was_loaded;

    void load( const JsonObject &jo, const std::string &src );
    void check() const;

    bool is_suitable_wall( const tripoint &pos ) const;
};

gate_id get_gate_id( const tripoint &pos )
{
    return gate_id( get_map().ter( pos ).id().str() );
}

generic_factory<gate_data> gates_data( "gate type" );

} // namespace

void gate_data::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "door", door );
    mandatory( jo, was_loaded, "floor", floor );
    mandatory( jo, was_loaded, "walls", walls, string_id_reader<ter_t> {} );

    if( !was_loaded || jo.has_member( "messages" ) ) {
        JsonObject messages_obj = jo.get_object( "messages" );

        optional( messages_obj, was_loaded, "pull", pull_message );
        optional( messages_obj, was_loaded, "open", open_message );
        optional( messages_obj, was_loaded, "close", close_message );
        optional( messages_obj, was_loaded, "fail", fail_message );
    }

    optional( jo, was_loaded, "moves", moves, 0 );
    optional( jo, was_loaded, "bashing_damage", bash_dmg, 0 );
}

void gate_data::check() const
{
    const ter_str_id winch_tid( id.str() );

    if( !winch_tid.is_valid() ) {
        debugmsg( "Gates \"%s\" have no terrain of the same name, working as a winch.", id.c_str() );
    } else if( !winch_tid->has_examine( iexamine::controls_gate ) ) {
        debugmsg( "Terrain \"%s\" can't control gates, but gates \"%s\" depend on it.",
                  winch_tid.c_str(), id.c_str() );
    }

    if( !door.is_valid() ) {
        debugmsg( "Invalid door \"%s\" in \"%s\".", door.c_str(), id.c_str() );
    }
    if( !floor.is_valid() ) {
        debugmsg( "Invalid floor \"%s\" in \"%s\".", floor.c_str(), id.c_str() );
    }
    for( const auto &elem : walls ) {
        if( !elem.is_valid() ) {
            debugmsg( "Invalid wall \"%s\" in \"%s\".", elem.c_str(), id.c_str() );
        }
    }

    if( moves < 0 ) {
        debugmsg( "Gates \"%s\" grant moves.", id.c_str() );
    }
}

bool gate_data::is_suitable_wall( const tripoint &pos ) const
{
    const auto wid = get_map().ter( pos );
    const auto iter = std::find_if( walls.begin(), walls.end(), [ wid ]( const ter_str_id & wall ) {
        return wall.id() == wid;
    } );
    return iter != walls.end();
}

void gates::load( const JsonObject &jo, const std::string &src )
{
    gates_data.load( jo, src );
}

void gates::check()
{
    gates_data.check();
}

void gates::reset()
{
    gates_data.reset();
}

// A gate handle is adjacent to a wall section, and next to that wall section on one side or
// another is the gate.  There may be a handle on the other side, but this is optional.
// The gate continues until it reaches a non-floor tile, so they can be arbitrary length.
//
//   |  !|!  -++-++-  !|++++-
//   +   +      !      +
//   +   +   -++-++-   +
//   +   +             +
//   +   +   !|++++-   +
//  !|   |!        !   |
//

void gates::open_gate( const tripoint &pos )
{
    const gate_id gid = get_gate_id( pos );

    if( !gates_data.is_valid( gid ) ) {
        return;
    }

    const gate_data &gate = gates_data.obj( gid );

    bool open = false;
    bool close = false;
    bool fail = false;

    map &here = get_map();
    for( const point &wall_offset : four_adjacent_offsets ) {
        const tripoint wall_pos = pos + wall_offset;

        if( !gate.is_suitable_wall( wall_pos ) ) {
            continue;
        }

        for( const point &gate_offset : four_adjacent_offsets ) {
            const tripoint gate_pos = wall_pos + gate_offset;

            if( gate_pos == pos ) {
                continue; // Never comes back
            }

            if( !open ) { // Closing the gate...
                tripoint cur_pos = gate_pos;
                while( here.ter( cur_pos ) == gate.floor.id() ) {
                    fail = !g->forced_door_closing( cur_pos, gate.door.id(), gate.bash_dmg ) || fail;
                    close = !fail;
                    cur_pos += gate_offset;
                }
            }

            if( !close ) { // Opening the gate...
                tripoint cur_pos = gate_pos;
                while( true ) {
                    const ter_id ter = here.ter( cur_pos );

                    if( ter == gate.door.id() ) {
                        here.ter_set( cur_pos, gate.floor.id() );
                        open = !fail;
                    } else if( ter != gate.floor.id() ) {
                        break;
                    }
                    cur_pos += gate_offset;
                }
            }
        }
    }

    if( get_player_view().sees( pos ) ) {
        if( open ) {
            add_msg( gate.open_message );
        } else if( close ) {
            add_msg( gate.close_message );
        } else if( fail ) {
            add_msg( gate.fail_message );
        } else {
            add_msg( _( "Nothing happens." ) );
        }
    }
}

void gates::open_gate( const tripoint &pos, Character &p )
{
    const gate_id gid = get_gate_id( pos );

    if( !gates_data.is_valid( gid ) ) {
        p.add_msg_if_player( _( "Nothing happens." ) );
        return;
    }

    const gate_data &gate = gates_data.obj( gid );

    p.add_msg_if_player( gate.pull_message );
    p.assign_activity( player_activity( open_gate_activity_actor(
                                            gate.moves,
                                            pos
                                        ) ) );
}

// Doors namespace

/**
 * Get movement cost of opening or closing doors for given character.
 *
 * @param who the character interacting the door.
 * @param what the door terrain type.
 * @param open whether to open the door (close if false).
 * @return movement cost of interacting with doors.
 */
static unsigned get_door_interact_cost( const Character &who, int_id<ter_t> what, const bool open )
{
    // movement point cost of opening doors
    int move_cost = 100;

    if( what != t_null ) {
        ter_t door = what.obj();
        move_cost = open ? door.open_cost : door.close_cost;
    }
    if( move_cost > 100 ) {
        const int strength = who.get_str();
        int cost_extra = move_cost - 100;
        if( strength > 8 ) {
            // weak characters open heavy doors slower
            // maximum move cost reduction of 100% with 12 strength
            cost_extra *= 1 - ( std::min( strength, 12 ) - 8 ) * 0.25;
        } else {
            // strong characters open heavy doors faster
            // maximum move cost increase of 300% at 0 strength
            cost_extra *= 1 + ( 8 - std::max( strength, 0 ) ) * 0.25;
        }
        move_cost = 100 + cost_extra;
    }
    // apply movement point modifiers
    if( who.is_crouching() ) {
        move_cost *= 3;
    } else if( who.is_running() ) {
        move_cost /= 2;
    }
    return move_cost;
}

/**
 * Check if the given character can open or close door at location.
 * This function performs only a handful of checks needed internally by this class
 * It will also print an in-game message if the opening or closing doors is not possible.
 *
 * @param who character interacting with door.
 * @param what name of the door (used in messages).
 * @param open check if door can be opened (closed if false).
 * @return true if the character can activate the door, false otherwise.
 */
static bool can_activate_door( const Character &who, std::string what, bool open )
{
    // prefix to use in player message
    std::string msg_prefix = open ? "open" : "close";

    // can't open doors when lying on the ground
    if( who.is_prone() ) {
        std::string msg = "You can't %s the %s while lying on the ground.";
        who.add_msg_if_player( m_info, _( msg ), msg_prefix, what );
        return false;
    }
    // check if both hands are holding items
    // one wielding an item while the other is wearing one
    bool wearing_hand_l = who.wearing_something_on( body_part_hand_l );
    bool wearing_hand_r = who.wearing_something_on( body_part_hand_r );
    if( !who.get_wielded_item().is_null() && ( wearing_hand_l || wearing_hand_r ) ) {
        std::string msg = "You can't %s the %s when your hands are not free.";
        who.add_msg_if_player( m_info, _( msg ), msg_prefix, what );
        return false;
    }
    // character has both arms broken
    if( who.is_limb_broken( body_part_arm_l ) && who.is_limb_broken( body_part_arm_r ) ) {
        std::string msg = "Can't %s the %s with broken arms.";
        who.add_msg_if_player( m_info, _( msg ), msg_prefix, what );
        return false;
    }
    return true;
}

bool doors::open_door( map &m, Character &who, tripoint where )
{
    int dpart = -1;
    const optional_vpart_position vp0 = m.veh_at( who.pos() );
    vehicle *const veh0 = veh_pointer_or_null( vp0 );
    const optional_vpart_position vp1 = m.veh_at( where );
    vehicle *const veh1 = veh_pointer_or_null( vp1 );

    bool veh_closed_door = false;
    bool outside_vehicle = veh0 == nullptr || veh0 != veh1;
    if( veh1 != nullptr ) {
        dpart = veh1->next_part_to_open( vp1->part_index(), outside_vehicle );
        veh_closed_door = dpart >= 0 && !veh1->part( dpart ).open;
    }
    if( veh0 != nullptr && std::abs( veh0->velocity ) > 100 ) {
        if( veh1 == nullptr ) {
            if( query_yn( _( "Dive from moving vehicle?" ) ) ) {
                g->moving_vehicle_dismount( where );
            }
            return false;
        } else if( veh1 != veh0 ) {
            add_msg( m_info, _( "There is another vehicle in the way." ) );
            return false;
        } else if( !vp1.part_with_feature( "BOARDABLE", true ) ) {
            add_msg( m_info, _( "That part of the vehicle is currently unsafe." ) );
            return false;
        }
    }
    std::string msg_prefix = who.is_crouching() ? "carefully " : "";
    std::string door_name = m.obstacle_name( where );
    if( !can_activate_door( who, door_name, true ) ) {
        return false;
    }
    //Wooden Fence Gate (or equivalently walkable doors):
    if( m.passable_ter_furn( where ) && who.is_walking()
        && !veh_closed_door && m.open_door( where, !m.is_outside( who.pos() ) ) ) {
        // apply movement point cost to player
        who.mod_moves( -get_door_interact_cost( who, t_null, true ) );
        who.add_msg_if_player( _( "You %sopen the %s." ), msg_prefix, door_name );
        // if auto-move is on, continue moving next turn
        if( who.is_auto_moving() ) {
            who.defer_move( where );
        }
        return true;
    }
    if( veh_closed_door ) {
        if( !veh1->handle_potential_theft( dynamic_cast< Character & >( who ) ) ) {
            return true;
        }
        door_name = veh1->part( dpart ).name();
        if( outside_vehicle ) {
            veh1->open_all_at( dpart );
        } else {
            veh1->open( dpart );
        }
        //~ %1$s - vehicle name, %2$s - part name
        who.add_msg_if_player( _( "You %sopen the %1$s's %2$s." ), msg_prefix, veh1->name, door_name );

        // apply movement point cost to player
        who.mod_moves( -get_door_interact_cost( who, t_null, true ) );
        // if auto-move is on, continue moving next turn
        if( who.is_auto_moving() ) {
            who.defer_move( where );
        }
        return true;
    }
    if( m.furn( where ) != f_safe_c && m.open_door( where, !m.is_outside( who.pos() ) ) ) {
        // door or window that just opened
        ter_t door = m.ter( where ).obj();
        if( who.is_running() ) {
            // dash through doors with blinding speed
            if( !door.has_flag( ter_furn_flag::TFLAG_WINDOW ) ) {
                g->walk_move( where, false );
            }
        }
        // apply movement point cost to player
        who.mod_moves( -get_door_interact_cost( who, door.close, true ) );
        if( veh1 != nullptr ) {
            //~ %1$s - vehicle name, %2$s - part name
            who.add_msg_if_player( _( "You %sopen the %1$s's %2$s." ), msg_prefix, veh1->name, door_name );
        } else {
            who.add_msg_if_player( _( "You %sopen the %s." ), msg_prefix, door_name );
        }
        // if auto-move is on, continue moving next turn
        if( who.is_auto_moving() ) {
            who.defer_move( where );
        }
        return true;
    }
    // Invalid move
    const bool waste_moves = who.is_blind() || who.has_effect( effect_stunned );
    if( waste_moves || where.z != who.posz() ) {
        add_msg( _( "You bump into the %s!" ), m.obstacle_name( where ) );
        // Only lose movement if we're blind
        if( waste_moves ) {
            // apply movement point cost to player
            who.mod_moves( -get_door_interact_cost( who, t_null, true ) );
        }
    } else if( m.ter( where ) == t_door_locked || m.ter( where ) == t_door_locked_peep ||
               m.ter( where ) == t_door_locked_alarm || m.ter( where ) == t_door_locked_interior ) {
        // Don't drain move points for learning something you could learn just by looking
        add_msg( _( "That door is locked!" ) );
    } else if( m.ter( where ) == t_door_bar_locked ) {
        add_msg( _( "You rattle the bars but the door is locked!" ) );
    }
    return false;
}

void doors::close_door( map &m, Creature &who, const tripoint &closep )
{
    bool didit = false;
    const bool inside = !m.is_outside( who.pos() );

    const Creature *const mon = get_creature_tracker().creature_at( closep );
    if( mon ) {
        if( mon->is_avatar() ) {
            who.add_msg_if_player( m_info, _( "There's some buffoon in the way!" ) );
        } else if( mon->is_monster() ) {
            // TODO: Houseflies, mosquitoes, etc shouldn't count
            who.add_msg_if_player( m_info, _( "The %s is in the way!" ), mon->get_name() );
        } else {
            who.add_msg_if_player( m_info, _( "%s is in the way!" ), mon->disp_name() );
        }
        return;
    }
    Character *ch = who.as_character();
    const std::string door_name = m.obstacle_name( closep );
    if( ch && !can_activate_door( *ch, door_name, false ) ) {
        return;
    }
    if( optional_vpart_position vp = m.veh_at( closep ) ) {
        // There is a vehicle part here; see if it has anything that can be closed
        vehicle *const veh = &vp->vehicle();
        const int vpart = vp->part_index();
        const int closable = veh->next_part_to_close( vpart,
                             veh_pointer_or_null( m.veh_at( who.pos() ) ) != veh );
        const int inside_closable = veh->next_part_to_close( vpart );
        const int openable = veh->next_part_to_open( vpart );
        if( closable >= 0 ) {
            if( !veh->handle_potential_theft( get_avatar() ) ) {
                return;
            }
            if( ch && veh->can_close( closable, *ch ) ) {
                veh->close( closable );
                //~ %1$s - vehicle name, %2$s - part name
                who.add_msg_if_player( _( "You close the %1$s's %2$s." ), veh->name, veh->part( closable ).name() );
                didit = true;
            }
        } else if( inside_closable >= 0 ) {
            who.add_msg_if_player( m_info, _( "That %s can only be closed from the inside." ),
                                   veh->part( inside_closable ).name() );
        } else if( openable >= 0 ) {
            who.add_msg_if_player( m_info, _( "That %s is already closed." ),
                                   veh->part( openable ).name() );
        } else {
            who.add_msg_if_player( m_info, _( "You cannot close the %s." ), veh->part( vpart ).name() );
        }
    } else if( m.furn( closep ) == furn_f_crate_o ) {
        who.add_msg_if_player( m_info, _( "You'll need to construct a seal to close the crate!" ) );
    } else if( !m.close_door( closep, inside, true ) ) {
        if( m.close_door( closep, true, true ) ) {
            who.add_msg_if_player( m_info,
                                   _( "You cannot close the %s from outside.  You must be inside the building." ),
                                   m.name( closep ) );
        } else {
            who.add_msg_if_player( m_info, _( "You cannot close the %s." ), m.name( closep ) );
        }
    } else {
        map_stack items_in_way = m.i_at( closep );
        // Scoot up to 25 liters of items out of the way
        if( m.furn( closep ) != furn_f_safe_o && !items_in_way.empty() ) {
            const units::volume max_nudge = 25_liter;

            const auto toobig = std::find_if( items_in_way.begin(), items_in_way.end(),
            [&max_nudge]( const item & it ) {
                return it.volume() > max_nudge;
            } );
            if( toobig != items_in_way.end() ) {
                who.add_msg_if_player( m_info, _( "The %s is too big to just nudge out of the way." ),
                                       toobig->tname() );
            } else if( items_in_way.stored_volume() > max_nudge ) {
                who.add_msg_if_player( m_info, _( "There is too much stuff in the way." ) );
            } else {
                m.close_door( closep, inside, false );
                didit = true;
                who.add_msg_if_player( m_info, _( "You push the %s out of the way." ),
                                       items_in_way.size() == 1 ? items_in_way.only_item().tname() : _( "stuff" ) );
                who.mod_moves( -std::min( items_in_way.stored_volume() / ( max_nudge / 50 ), 100 ) );

                if( m.has_flag( ter_furn_flag::TFLAG_NOITEM, closep ) ) {
                    // Just plopping items back on their origin square will displace them to adjacent squares
                    // since the door is closed now.
                    for( auto &elem : items_in_way ) {
                        m.add_item_or_charges( closep, elem );
                    }
                    m.i_clear( closep );
                }
            }
        } else {
            m.close_door( closep, inside, false );
            who.add_msg_if_player( _( "You close the %s." ), door_name );
            didit = true;
        }
    }

    if( didit ) {
        ter_str_id door = m.ter( closep ).obj().open;
        who.mod_moves( -get_door_interact_cost( *who.as_character(), door, false ) );
    }
}
