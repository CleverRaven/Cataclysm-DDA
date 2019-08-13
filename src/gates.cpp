#include "gates.h"

#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <set>

#include "avatar.h"
#include "game.h" // TODO: This is a circular dependency
#include "generic_factory.h"
#include "iexamine.h"
#include "json.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "player.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "int_id.h"
#include "item.h"
#include "optional.h"
#include "player_activity.h"
#include "string_id.h"
#include "translations.h"
#include "units.h"
#include "type_id.h"
#include "colony.h"
#include "point.h"

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

    std::string pull_message;
    std::string open_message;
    std::string close_message;
    std::string fail_message;

    int moves;
    int bash_dmg;
    bool was_loaded;

    void load( JsonObject &jo, const std::string &src );
    void check() const;

    bool is_suitable_wall( const tripoint &pos ) const;
};

gate_id get_gate_id( const tripoint &pos )
{
    return gate_id( g->m.ter( pos ).id().str() );
}

generic_factory<gate_data> gates_data( "gate type", "handle", "other_handles" );

} // namespace

void gate_data::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "door", door );
    mandatory( jo, was_loaded, "floor", floor );
    mandatory( jo, was_loaded, "walls", walls, string_id_reader<ter_t> {} );

    if( !was_loaded || jo.has_member( "messages" ) ) {
        JsonObject messages_obj = jo.get_object( "messages" );

        optional( messages_obj, was_loaded, "pull", pull_message, translated_string_reader );
        optional( messages_obj, was_loaded, "open", open_message, translated_string_reader );
        optional( messages_obj, was_loaded, "close", close_message, translated_string_reader );
        optional( messages_obj, was_loaded, "fail", fail_message, translated_string_reader );
    }

    optional( jo, was_loaded, "moves", moves, 0 );
    optional( jo, was_loaded, "bashing_damage", bash_dmg, 0 );
}

void gate_data::check() const
{
    static const iexamine_function controls_gate( iexamine_function_from_string( "controls_gate" ) );
    const ter_str_id winch_tid( id.str() );

    if( !winch_tid.is_valid() ) {
        debugmsg( "Gates \"%s\" have no terrain of the same name, working as a winch.", id.c_str() );
    } else if( winch_tid->examine != controls_gate ) {
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
    const auto wid = g->m.ter( pos );
    const auto iter = std::find_if( walls.begin(), walls.end(), [ wid ]( const ter_str_id & wall ) {
        return wall.id() == wid;
    } );
    return iter != walls.end();
}

void gates::load( JsonObject &jo, const std::string &src )
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

    for( int i = 0; i < 4; ++i ) {
        static constexpr tripoint dir[4] = {
            { tripoint_east }, { tripoint_south }, { tripoint_west }, { tripoint_north }
        };
        const tripoint wall_pos = pos + dir[i];

        if( !gate.is_suitable_wall( wall_pos ) ) {
            continue;
        }

        for( auto j : dir ) {
            const tripoint gate_pos = wall_pos + j;

            if( gate_pos == pos ) {
                continue; // Never comes back
            }

            if( !open ) { // Closing the gate...
                tripoint cur_pos = gate_pos;
                while( g->m.ter( cur_pos ) == gate.floor.id() ) {
                    fail = !g->forced_door_closing( cur_pos, gate.door.id(), gate.bash_dmg ) || fail;
                    close = !fail;
                    cur_pos += j;
                }
            }

            if( !close ) { // Opening the gate...
                tripoint cur_pos = gate_pos;
                while( true ) {
                    const ter_id ter = g->m.ter( cur_pos );

                    if( ter == gate.door.id() ) {
                        g->m.ter_set( cur_pos, gate.floor.id() );
                        open = !fail;
                    } else if( ter != gate.floor.id() ) {
                        break;
                    }
                    cur_pos += j;
                }
            }
        }
    }

    if( g->u.sees( pos ) ) {
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

void gates::open_gate( const tripoint &pos, player &p )
{
    const gate_id gid = get_gate_id( pos );

    if( !gates_data.is_valid( gid ) ) {
        p.add_msg_if_player( _( "Nothing happens." ) );
        return;
    }

    const gate_data &gate = gates_data.obj( gid );

    p.add_msg_if_player( gate.pull_message );
    p.assign_activity( activity_id( "ACT_OPEN_GATE" ), gate.moves );
    p.activity.placement = pos;
}

// Doors namespace

void doors::close_door( map &m, Character &who, const tripoint &closep )
{
    bool didit = false;
    const bool inside = !m.is_outside( who.pos() );

    const Creature *const mon = g->critter_at( closep );
    if( mon ) {
        if( mon->is_player() ) {
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
        vehicle *const veh = &vp->vehicle();
        const int vpart = vp->part_index();
        const int closable = veh->next_part_to_close( vpart,
                             veh_pointer_or_null( m.veh_at( who.pos() ) ) != veh );
        const int inside_closable = veh->next_part_to_close( vpart );
        const int openable = veh->next_part_to_open( vpart );
        if( closable >= 0 ) {
            if( !veh->handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                return;
            }
            veh->close( closable );
            didit = true;
        } else if( inside_closable >= 0 ) {
            who.add_msg_if_player( m_info, _( "That %s can only be closed from the inside." ),
                                   veh->parts[inside_closable].name() );
        } else if( openable >= 0 ) {
            who.add_msg_if_player( m_info, _( "That %s is already closed." ),
                                   veh->parts[openable].name() );
        } else {
            who.add_msg_if_player( m_info, _( "You cannot close the %s." ), veh->parts[vpart].name() );
        }
    } else if( m.furn( closep ) == furn_str_id( "f_crate_o" ) ) {
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
        auto items_in_way = m.i_at( closep );
        // Scoot up to 25 liters of items out of the way
        if( m.furn( closep ) != furn_str_id( "f_safe_o" ) && !items_in_way.empty() ) {
            const units::volume max_nudge = 25000_ml;

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

                if( m.has_flag( "NOITEM", closep ) ) {
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
            didit = true;
        }
    }

    if( didit ) {
        who.mod_moves( -90 ); // TODO: Vary this? Based on strength, broken legs, and so on.
    }
}
