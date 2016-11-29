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

    gate_data() :
        moves( 0 ),
        bash_dmg( 0 ),
        was_loaded( false ) {};

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

}

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
        static const tripoint dir[4] = { { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 } };
        const tripoint wall_pos = pos + dir[i];

        if( !gate.is_suitable_wall( wall_pos ) ) {
            continue;
        }

        for( int j = 0; j < 4; ++j ) {
            const tripoint gate_pos = wall_pos + dir[j];

            if( gate_pos == pos ) {
                continue; // Never comes back
            }

            if( !open ) { // Closing the gate...
                tripoint cur_pos = gate_pos;
                while( g->m.ter( cur_pos ) == gate.floor.id() ) {
                    fail = !g->forced_door_closing( cur_pos, gate.door.id(), gate.bash_dmg ) || fail;
                    close = !fail;
                    cur_pos += dir[j];
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
                    cur_pos += dir[j];
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
    p.assign_activity( activity_id( "ACT_OPEN_GATE" ), gate.moves );
    p.activity.placement = pos;
}
