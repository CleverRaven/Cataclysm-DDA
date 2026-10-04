#include "cata3d_bridge.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string_view>

#include "avatar.h"
#include "calendar.h"
#include "creature_tracker.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "monster.h"
#include "npc.h"

#if defined(_WIN32)
#   define NOMINMAX
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "ws2_32.lib")
using cata3d_socket_t = SOCKET;
static constexpr cata3d_socket_t CATA3D_INVALID_SOCKET = INVALID_SOCKET;
#else
#   include <arpa/inet.h>
#   include <fcntl.h>
#   include <netinet/in.h>
#   include <sys/socket.h>
#   include <unistd.h>
using cata3d_socket_t = int;
static constexpr cata3d_socket_t CATA3D_INVALID_SOCKET = -1;
#endif

namespace cata3d_bridge {
namespace {

constexpr int COMMAND_PORT = 7777;
constexpr int STATE_PORT = 7778;
constexpr int RADIUS = 8;
constexpr int MIN_MONSTER_RADIUS = 20;

cata3d_socket_t command_socket = CATA3D_INVALID_SOCKET;
cata3d_socket_t state_socket = CATA3D_INVALID_SOCKET;
bool initialized = false;

void close_socket( cata3d_socket_t &sock ) {
    if( sock == CATA3D_INVALID_SOCKET ) {
        return;
    }
#if defined(_WIN32)
    closesocket( sock );
#else
    close( sock );
#endif
    sock = CATA3D_INVALID_SOCKET;
}

bool make_nonblocking( cata3d_socket_t sock ) {
#if defined(_WIN32)
    u_long mode = 1;
    return ioctlsocket( sock, FIONBIO, &mode ) == 0;
#else
    const int flags = fcntl( sock, F_GETFL, 0 );
    return flags >= 0 && fcntl( sock, F_SETFL, flags | O_NONBLOCK ) == 0;
#endif
}

void init_once() {
    if( initialized ) {
        return;
    }

#if defined(_WIN32)
    WSADATA wsa_data{};
    if( WSAStartup( MAKEWORD( 2, 2 ), &wsa_data ) != 0 ) {
        return;
    }
#endif

    command_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    state_socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( command_socket == CATA3D_INVALID_SOCKET || state_socket == CATA3D_INVALID_SOCKET ) {
        close_socket( command_socket );
        close_socket( state_socket );
#if defined(_WIN32)
        WSACleanup();
#endif
        return;
    }

    int reuse = 1;
    sockaddr_in bind_addr{};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    bind_addr.sin_port = htons( COMMAND_PORT );

    setsockopt( command_socket, SOL_SOCKET, SO_REUSEADDR,
                reinterpret_cast<const char *>( &reuse ), sizeof( reuse ) );

    if( bind( command_socket, reinterpret_cast<const sockaddr *>( &bind_addr ),
              sizeof( bind_addr ) ) != 0 ) {
        close_socket( command_socket );
        close_socket( state_socket );
#if defined(_WIN32)
        WSACleanup();
#endif
        return;
    }

    if( !make_nonblocking( command_socket ) ) {
        close_socket( command_socket );
        close_socket( state_socket );
#if defined(_WIN32)
        WSACleanup();
#endif
        return;
    }

    initialized = true;
}

std::string json_escape( std::string_view value ) {
    std::string out;
    out.reserve( value.size() + 8 );
    out.push_back( '"' );
    for( const unsigned char c : value ) {
        switch( c ) {
            case '"': out += "\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if( c < 0x20 ) {
                    char tmp[7];
                    std::snprintf( tmp, sizeof( tmp ), "\\u%04x", c );
                    out += tmp;
                } else {
                    out.push_back( static_cast<char>( c ) );
                }
        }
    }
    out.push_back( '"' );
    return out;
}

void append_cell( std::string &json, const map &here, const tripoint_bub_ms &player_pos,
                  const tripoint_bub_ms &p ) {
    json += '[';
    json += std::to_string( p.x() - player_pos.x() );
    json += ',';
    json += std::to_string( p.y() - player_pos.y() );
    json += ',';
    json += std::to_string( p.z() - player_pos.z() );
    json += ',';
    json += json_escape( here.ter( p ).id().str() );
    json += ',';
    json += json_escape( here.furn( p ).id().str() );
    json += ',';
    json += here.impassable( p ) ? '1' : '0';
    json += ',';
    json += here.is_outside( p ) ? '1' : '0';
    json += ']';
}

void append_items( std::string &json, const map &here, const tripoint_bub_ms &player_pos,
                   const tripoint_bub_ms &p, bool &first ) {
    if( !here.has_items( p ) ) {
        return;
    }

    const map_stack items = here.i_at( p );
    for( const item &it : items ) {
        if( it.is_null() ) {
            continue;
        }

        if( !first ) {
            json += ',';
        }
        first = false;
        json += '[';
        json += std::to_string( p.x() - player_pos.x() );
        json += ',';
        json += std::to_string( p.y() - player_pos.y() );
        json += ',';
        json += std::to_string( p.z() - player_pos.z() );
        json += ',';
        json += json_escape( it.typeId().str() );
        json += ']';
    }
}

void append_monsters( std::string &json, const tripoint_bub_ms &player_pos ) {
    bool first = true;
    for( const shared_ptr_fast<monster> &mon : get_creature_tracker().get_monsters_list() ) {
        if( !mon || mon->is_dead() || mon->is_hallucination() ) {
            continue;
        }

        const tripoint_bub_ms p = mon->pos_bub();
        if( rl_dist( p, player_pos ) > MIN_MONSTER_RADIUS ) {
            continue;
        }

        if( !first ) {
            json += ',';
        }
        first = false;

        json += '[';
        json += std::to_string( p.x() - player_pos.x() );
        json += ',';
        json += std::to_string( p.y() - player_pos.y() );
        json += ',';
        json += std::to_string( p.z() - player_pos.z() );
        json += ',';
        json += json_escape( mon->type ? mon->type->id.str() : std::string( "mon_null" ) );
        json += ',';
        json += std::to_string( mon->get_hp() );
        json += ']';
    }
}

bool parse_command( std::string_view cmd, action_id &result ) {
    if( cmd == "MOVE_FWD" ) {
        result = ACTION_MOVE_FORTH;
    } else if( cmd == "MOVE_BACK" ) {
        result = ACTION_MOVE_BACK;
    } else if( cmd == "MOVE_LEFT" ) {
        result = ACTION_MOVE_LEFT;
    } else if( cmd == "MOVE_RIGHT" ) {
        result = ACTION_MOVE_RIGHT;
    } else if( cmd == "MOVE_FWD_LEFT" ) {
        result = ACTION_MOVE_FORTH_LEFT;
    } else if( cmd == "MOVE_FWD_RIGHT" ) {
        result = ACTION_MOVE_FORTH_RIGHT;
    } else if( cmd == "MOVE_BACK_LEFT" ) {
        result = ACTION_MOVE_BACK_LEFT;
    } else if( cmd == "MOVE_BACK_RIGHT" ) {
        result = ACTION_MOVE_BACK_RIGHT;
    } else if( cmd == "OPEN" ) {
        result = ACTION_OPEN;
    } else if( cmd == "CLOSE" ) {
        result = ACTION_CLOSE;
    } else if( cmd == "SMASH" ) {
        result = ACTION_SMASH;
    } else if( cmd == "PICKUP" ) {
        result = ACTION_PICKUP;
    } else if( cmd == "PICKUP_ALL" ) {
        result = ACTION_PICKUP_ALL;
    } else if( cmd == "INTERACT" ) {
        result = ACTION_INTERACT;
    } else if( cmd == "INVENTORY" ) {
        result = ACTION_INVENTORY;
    } else if( cmd == "FIRE" ) {
        result = ACTION_FIRE;
    } else if( cmd == "WAIT" ) {
        result = ACTION_WAIT;
    } else if( cmd == "MOVE_UP" ) {
        result = ACTION_MOVE_UP;
    } else if( cmd == "MOVE_DOWN" ) {
        result = ACTION_MOVE_DOWN;
    } else {
        return false;
    }
    return true;
}

} // namespace

std::optional<action_id> poll_action() {
#if !defined(TILES)
    return std::nullopt;
#else
    init_once();
    if( !initialized ) {
        return std::nullopt;
    }

    char buffer[128] = {};
    sockaddr_in from{};
#if defined(_WIN32)
    int from_len = sizeof( from );
#else
    socklen_t from_len = sizeof( from );
#endif

    const int received = recvfrom( command_socket, buffer, sizeof( buffer ) - 1, 0,
                                   reinterpret_cast<sockaddr *>( &from ), &from_len );
    if( received <= 0 ) {
        return std::nullopt;
    }

    buffer[received] = '\0';
    const std::string_view cmd( buffer );

    if( cmd == "REQUEST_STATE" ) {
        publish_state();
        return std::nullopt;
    }

    action_id result = ACTION_NULL;
    if( parse_command( cmd, result ) ) {
        return result;
    }

    return std::nullopt;
#endif
}

void publish_state() {
#if !defined(TILES)
    return;
#else
    init_once();
    if( !initialized ) {
        return;
    }

    map &here = get_map();
    avatar &player = get_avatar();
    const tripoint_bub_ms player_pos = player.pos_bub();

    std::string json;
    json.reserve( 100000 );
    json += "{\"v\":1,\"turn\":";
    json += std::to_string( to_turn<int>( calendar::turn ) );
    json += ",\"player\":[";
    json += std::to_string( player_pos.x() );
    json += ',';
    json += std::to_string( player_pos.y() );
    json += ',';
    json += std::to_string( player_pos.z() );
    json += "],\"cells\":[";

    bool first_cell = true;
    for( int dz = -1; dz <= 1; ++dz ) {
        for( int dy = -RADIUS; dy <= RADIUS; ++dy ) {
            for( int dx = -RADIUS; dx <= RADIUS; ++dx ) {
                const tripoint_bub_ms p = player_pos + tripoint{ dx, dy, dz };
                if( !here.inbounds( p ) ) {
                    continue;
                }

                if( !first_cell ) {
                    json += ',';
                }
                first_cell = false;
                append_cell( json, here, player_pos, p );
            }
        }
    }

    json += "],\"items\":[";
    bool first_item = true;
    for( int dz = -1; dz <= 1; ++dz ) {
        for( int dy = -RADIUS; dy <= RADIUS; ++dy ) {
            for( int dx = -RADIUS; dx <= RADIUS; ++dx ) {
                const tripoint_bub_ms p = player_pos + tripoint{ dx, dy, dz };
                if( here.inbounds( p ) ) {
                    append_items( json, here, player_pos, p, first_item );
                }
            }
        }
    }

    json += "],\"monsters\":[";
    append_monsters( json, player_pos );
    json += "]}";

    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    target.sin_port = htons( STATE_PORT );

    sendto( state_socket, json.data(), static_cast<int>( json.size() ), 0,
            reinterpret_cast<const sockaddr *>( &target ), sizeof( target ) );
#endif
}

void shutdown() {
    if( !initialized ) {
        return;
    }

    close_socket( command_socket );
    close_socket( state_socket );
#if defined(_WIN32)
    WSACleanup();
#endif
    initialized = false;
}

} // namespace cata3d_bridge
