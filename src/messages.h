#pragma once
#ifndef CATA_SRC_MESSAGES_H
#define CATA_SRC_MESSAGES_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "string_formatter.h"
#include "enums.h"
#include "debug.h"

class Creature;
class JsonOut;
class JsonObject;
class translation;

struct tripoint;

namespace catacurses
{
class window;
} // namespace catacurses

namespace Messages
{
std::vector<std::pair<std::string, std::string>> recent_messages( size_t count );
void add_msg( std::string msg );
void add_msg( const game_message_params &params, std::string msg );
void clear_messages();
void deactivate();
size_t size();
bool has_undisplayed_messages();
void display_messages();
void display_messages( const catacurses::window &ipk_target, int left, int top, int right,
                       int bottom );
void serialize( JsonOut &json );
void deserialize( const JsonObject &json );
} // namespace Messages

void add_msg( std::string msg );
template<typename ...Args>
inline void add_msg( const std::string &msg, Args &&... args )
{
    return add_msg( string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg( const char *const msg, Args &&... args )
{
    return add_msg( string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg( const translation &msg, Args &&... args )
{
    return add_msg( string_format( msg, std::forward<Args>( args )... ) );
}

// Prevent potentially expensive evaluation of arguments which won't be printed.
#define add_msg_debug( ... )\
    if( debug_mode ) {\
        add_msg( m_debug, __VA_ARGS__ );\
    };

void add_msg( const game_message_params &params, std::string msg );
template<typename ...Args>
inline void add_msg( const game_message_params &params, const std::string &msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg( params, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg( const game_message_params &params, const char *const msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg( params, string_format( msg, std::forward<Args>( args )... ) );
}

void add_msg_if_player_sees( const tripoint &target, std::string msg );
void add_msg_if_player_sees( const Creature &target, std::string msg );
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const std::string &msg,
                                    Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const std::string &msg,
                                    Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const char *const msg, Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const char *const msg, Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const translation &msg,
                                    Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const translation &msg,
                                    Args &&... args )
{
    return add_msg_if_player_sees( target, string_format( msg, std::forward<Args>( args )... ) );
}

void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                             std::string msg );
void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                             std::string msg );
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                                    const std::string &msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg_if_player_sees( target, params, string_format( msg,
                                   std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                                    const std::string &msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg_if_player_sees( target, params, string_format( msg,
                                   std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const tripoint &target, const game_message_params &params,
                                    const char *const msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg_if_player_sees( target, params, string_format( msg,
                                   std::forward<Args>( args )... ) );
}
template<typename ...Args>
inline void add_msg_if_player_sees( const Creature &target, const game_message_params &params,
                                    const char *const msg, Args &&... args )
{
    if( params.type == m_debug && !debug_mode ) {
        return;
    }
    return add_msg_if_player_sees( target, params, string_format( msg,
                                   std::forward<Args>( args )... ) );
}

#endif // CATA_SRC_MESSAGES_H
