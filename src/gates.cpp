#include "gates.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
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
#include "player_activity.h"
#include "point.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"

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
    std::vector<std::pair<gate_id, mod_id>> src;

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

    void load( const JsonObject &jo, std::string_view src );
    void check() const;

    bool is_suitable_wall( const tripoint_bub_ms &pos ) const;
};

gate_id get_gate_id( const tripoint_bub_ms &pos )
{
    return gate_id( get_map().ter( pos ).id().str() );
}

generic_factory<gate_data> gates_data( "gate type" );

} // namespace

void gate_data::load( const JsonObject &jo, const std::string_view )
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

bool gate_data::is_suitable_wall( const tripoint_bub_ms &pos ) const
{
    const ter_id wid = get_map().ter( pos );
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

void gates::open_gate( const tripoint_bub_ms &pos )
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
        const tripoint_bub_ms wall_pos = pos + wall_offset;

        if( !gate.is_suitable_wall( wall_pos ) ) {
            continue;
        }

        for( const point &gate_offset : four_adjacent_offsets ) {
            const tripoint_bub_ms gate_pos = wall_pos + gate_offset;

            if( gate_pos == pos ) {
                continue; // Never comes back
            }

            if( !open ) { // Closing the gate...
                tripoint_bub_ms cur_pos = gate_pos;
                while( here.ter( cur_pos ) == gate.floor.id() ) {
                    fail = !g->forced_door_closing( cur_pos, gate.door.id(), gate.bash_dmg ) || fail;
                    close = !fail;
                    cur_pos += gate_offset;
                }
            }

            if( !close ) { // Opening the gate...
                tripoint_bub_ms cur_pos = gate_pos;
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

void gates::open_gate( const tripoint_bub_ms &pos, Character &p )
{
    const gate_id gid = get_gate_id( pos );

    if( !gates_data.is_valid( gid ) ) {
        p.add_msg_if_player( _( "Nothing happens." ) );
        return;
    }

    const gate_data &gate = gates_data.obj( gid );

    p.add_msg_if_player( gate.pull_message );
    p.assign_activity( open_gate_activity_actor( gate.moves, pos ) );
}

// Doors namespace
// TODO: move door functions from maps namespace here, or vice versa.

void doors::close_door( map &m, Creature &who, const tripoint_bub_ms &closep )
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
            Character *ch = who.as_character();
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
                    for( item &elem : items_in_way ) {
                        m.add_item_or_charges( closep, elem );
                    }
                    m.i_clear( closep );
                }
            }
        } else {
            const std::string door_name = m.obstacle_name( closep );
            m.close_door( closep, inside, false );
            who.add_msg_if_player( _( "You close the %s." ), door_name );
            didit = true;
        }
    }

    if( didit ) {
        // TODO: Vary this? Based on strength, broken legs, and so on.
        who.mod_moves( -90 );
    }
}

// If you update this, look at doors::can_lock_door too.
bool doors::lock_door( map &m, Creature &who, const tripoint_bub_ms &lockp )
{
    bool didit = false;

    if( optional_vpart_position vp = m.veh_at( lockp ) ) {
        vehicle *const veh = &vp->vehicle();
        const int vpart = vp->part_index();
        const bool inside_vehicle = m.veh_at( who.pos() ) &&
                                    &vp->vehicle() == &m.veh_at( who.pos() )->vehicle();
        const int lockable = veh->next_part_to_lock( vpart, !inside_vehicle );
        const int inside_lockable = veh->next_part_to_lock( vpart );
        const int already_locked_part = veh->next_part_to_unlock( vpart );

        if( lockable >= 0 ) {
            if( const Character *const ch = who.as_character() ) {
                if( !veh->handle_potential_theft( *ch ) ) {
                    return false;
                }
                veh->lock( lockable );
                who.add_msg_if_player( _( "You lock the %1$s's %2$s." ), veh->name, veh->part( lockable ).name() );
                didit = true;
            }
        } else if( inside_lockable >= 0 ) {
            who.add_msg_if_player( m_info, _( "That %s can only be locked from the inside." ),
                                   veh->part( inside_lockable ).name() );
        } else if( already_locked_part >= 0 ) {
            who.add_msg_if_player( m_info, _( "That %s is already locked." ),
                                   veh->part( already_locked_part ).name() );
        } else {
            who.add_msg_if_player( m_info, _( "You cannot lock the %s." ), veh->part( vpart ).name() );
        }
    }
    if( didit ) {
        sounds::sound( lockp, 1, sounds::sound_t::activity, _( "a soft chk." ) );
        who.mod_moves( -90 );
    }
    return didit;
}

bool doors::can_lock_door( const map &m, const Creature &who, const tripoint_bub_ms &lockp )
{
    int lockable = -1;
    if( const optional_vpart_position vp = m.veh_at( lockp ) ) {
        const vehicle *const veh = &vp->vehicle();
        const bool inside_vehicle = m.veh_at( who.pos() ) &&
                                    &vp->vehicle() == &m.veh_at( who.pos() )->vehicle();
        const int vpart = vp->part_index();
        lockable = veh->next_part_to_lock( vpart, !inside_vehicle );
    }
    return lockable >= 0;
}

// If you update this, look at doors::can_unlock_door too.
bool doors::unlock_door( map &m, Creature &who, const tripoint_bub_ms &lockp )
{
    bool didit = false;

    if( optional_vpart_position vp = m.veh_at( lockp ) ) {
        vehicle *const veh = &vp->vehicle();
        const int vpart = vp->part_index();
        const bool inside_vehicle = m.veh_at( who.pos() ) &&
                                    &vp->vehicle() == &m.veh_at( who.pos() )->vehicle();
        const int already_unlocked_part = veh->next_part_to_lock( vpart );
        const int inside_unlockable = veh->next_part_to_unlock( vpart );
        const int unlockable = veh->next_part_to_unlock( vpart, !inside_vehicle );

        if( unlockable >= 0 ) {
            if( const Character *const ch = who.as_character() ) {
                if( !veh->handle_potential_theft( *ch ) ) {
                    return false;
                }
                veh->unlock( unlockable );
                who.add_msg_if_player( _( "You unlock the %1$s's %2$s." ), veh->name,
                                       veh->part( unlockable ).name() );
                didit = true;
            }
        } else if( inside_unlockable >= 0 ) {
            who.add_msg_if_player( m_info, _( "That %s can only be unlocked from the inside." ),
                                   veh->part( inside_unlockable ).name() );
        } else if( already_unlocked_part >= 0 ) {
            who.add_msg_if_player( m_info, _( "That %s is already unlocked." ),
                                   veh->part( already_unlocked_part ).name() );
        } else {
            who.add_msg_if_player( m_info, _( "You cannot unlock the %s." ), veh->part( vpart ).name() );
        }
    }
    if( didit ) {
        sounds::sound( lockp, 1, sounds::sound_t::activity, _( "a soft click." ) );
        who.mod_moves( -90 );
    }
    return didit;
}

bool doors::can_unlock_door( const map &m, const Creature &who, const tripoint_bub_ms &lockp )
{
    int unlockable = -1;
    if( const optional_vpart_position vp = m.veh_at( lockp ) ) {
        const vehicle *const veh = &vp->vehicle();
        const bool inside_vehicle = m.veh_at( who.pos() ) &&
                                    &vp->vehicle() == &m.veh_at( who.pos() )->vehicle();
        const int vpart = vp->part_index();
        unlockable = veh->next_part_to_unlock( vpart, !inside_vehicle );
    }

    return unlockable >= 0;
}
