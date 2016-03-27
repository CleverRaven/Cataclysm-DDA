#include "gates.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "generic_factory.h"
#include "messages.h"
#include "json.h"

#include <string>

namespace
{

struct gate_data;
using gate_id = string_id<gate_data>;

struct gate_data {
    gate_data() : moves( 0 ), bash_dmg( 0 ), was_loaded( false ) {};

    gate_id id;
    std::string wall_id;
    std::string door_id;
    std::string floor_id;

    std::string pull_message;
    std::string open_message;
    std::string close_message;
    std::string fail_message;

    int moves;
    int bash_dmg;
    bool was_loaded;

    void load( JsonObject &jo );
};

gate_id get_gate_id( const tripoint &pos )
{
    return static_cast<gate_id>( g->m.get_ter( pos ) );
}

generic_factory<gate_data> gates_data( "gate type", "handle" );

}

void gate_data::load( JsonObject &jo )
{
    mandatory( jo, was_loaded, "wall", wall_id );
    mandatory( jo, was_loaded, "door", door_id );
    mandatory( jo, was_loaded, "floor", floor_id );

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

void gates::load_gates( JsonObject &jo )
{
    gates_data.load( jo );
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

    const ter_id wall = terfind( gate.wall_id );
    const ter_id door = terfind( gate.door_id );
    const ter_id floor = terfind( gate.floor_id );

    bool open = false;
    bool close = false;
    bool fail = false;

    for( int i = 0; i < 4; ++i ) {
        static const int dx[4] = { 1, 0, -1, 0 };
        static const int dy[4] = { 0, 1, 0, -1 };

        const int wall_x = pos.x + dx[i];
        const int wall_y = pos.y + dy[i];

        if( g->m.ter( wall_x, wall_y ) != wall ) {
            continue;
        }

        for( int j = 0; j < 4; ++j ) {
            const int gate_x = wall_x + dx[j];
            const int gate_y = wall_y + dy[j];

            if( !open ) {  //closing the gate...
                int x = gate_x;
                int y = gate_y;
                while( g->m.ter( x, y ) == floor ) {
                    const tripoint gate_pos( x, y, pos.z );

                    fail = !g->forced_door_closing( gate_pos, door, gate.bash_dmg ) || fail;
                    close = !fail;
                    x += dx[j];
                    y += dy[j];
                }
            }

            if( !close ) {  //opening the gate...
                int x = gate_x;
                int y = gate_y;
                while( true ) {
                    const ter_id ter = g->m.ter( x, y );

                    if( ter == door ) {
                        g->m.ter_set( x, y, floor );
                        open = !fail;
                    } else if( ter != floor ) {
                        break;
                    }
                    x += dx[j];
                    y += dy[j];
                }
            }
        }
    }

    if( g->u.sees( pos ) ) {
        if( open ) {
            add_msg( gate.open_message.c_str() );
        } else if( close ) {
            add_msg( gate.close_message.c_str() );
        } else if( fail ) {
            add_msg( gate.fail_message.c_str() );
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

    p.add_msg_if_player( gate.pull_message.c_str() );
    p.assign_activity( ACT_OPEN_GATE, gate.moves );
    p.activity.placement = pos;
}
